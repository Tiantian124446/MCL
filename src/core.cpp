// ---------------------------------------------------------------------------
//  core.cpp  -  Minecraft 启动器核心实现
// ---------------------------------------------------------------------------
#include "core.h"
#include "http.h"
#include "json.h"
#include "util.h"

#include <windows.h>
#include <wincrypt.h>
#include <thread>
#include <mutex>
#include <map>
#include <memory>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdio>

// ===========================================================================
//  内部工具
// ===========================================================================
namespace {

std::vector<std::string> splitStr(const std::string& s, char delim) {
    std::vector<std::string> out;
    std::string cur;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == delim) { out.push_back(cur); cur.clear(); }
        else cur += s[i];
    }
    out.push_back(cur);
    return out;
}

unsigned long long fileSizeOf(const std::wstring& p) {
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExW(p.c_str(), GetFileExInfoStandard, &fad))
        return (unsigned long long)-1;
    if (fad.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        return (unsigned long long)-1;
    return ((unsigned long long)fad.nFileSizeHigh << 32) | (unsigned long long)fad.nFileSizeLow;
}

bool writeAll(const std::wstring& path, const std::string& data) {
    ensureParentDir(path);
    HANDLE h = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD w = 0;
    BOOL ok = WriteFile(h, data.data(), (DWORD)data.size(), &w, nullptr);
    CloseHandle(h);
    return ok != FALSE && w == (DWORD)data.size();
}

bool readAll(const std::wstring& path, std::string& out) {
    HANDLE h = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr,
                           OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (h == INVALID_HANDLE_VALUE) return false;
    DWORD sz = GetFileSize(h, nullptr);
    out.clear();
    if (sz > 0) {
        out.resize(sz);
        DWORD r = 0;
        BOOL ok = ReadFile(h, &out[0], sz, &r, nullptr);
        CloseHandle(h);
        return ok != FALSE && r == sz;
    }
    CloseHandle(h);
    return true;
}

bool runAndWait(const std::wstring& cmdline) {
    std::vector<wchar_t> buf(cmdline.begin(), cmdline.end());
    buf.push_back(0);

    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessW(nullptr, buf.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi))
        return false;

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD code = 1;
    GetExitCodeProcess(pi.hProcess, &code);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return code == 0;
}

// 离线模式 UUID = md5("OfflinePlayer:" + name), 带连字符
std::string offlineUuid(const std::string& name) {
    std::string src = "OfflinePlayer:" + name;
    unsigned char dig[16] = { 0 };
    DWORD dlen = 16;

    HCRYPTPROV prov = 0;
    HCRYPTHASH hash = 0;
    if (CryptAcquireContextW(&prov, nullptr, nullptr, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)) {
        if (CryptCreateHash(prov, CALG_MD5, 0, 0, &hash)) {
            CryptHashData(hash, (BYTE*)src.data(), (DWORD)src.size(), 0);
            CryptGetHashParam(hash, HP_HASHVAL, dig, &dlen, 0);
            CryptDestroyHash(hash);
        }
        CryptReleaseContext(prov, 0);
    }

    char buf[40];
    sprintf(buf,
            "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
            dig[0], dig[1], dig[2], dig[3], dig[4], dig[5], dig[6], dig[7],
            dig[8], dig[9], dig[10], dig[11], dig[12], dig[13], dig[14], dig[15]);
    return std::string(buf);
}

// 判断 rules 数组是否允许当前平台 (Windows x86_64, 无 feature)
bool ruleAllows(const mjson::Value& rules) {
    if (!rules.isArray() || rules.size() == 0) return true;

    bool allowed = false;
    for (size_t i = 0; i < rules.size(); ++i) {
        const mjson::Value& r = rules.at(i);
        if (!r.isObject()) continue;

        std::string action = r.get("action").asString("allow");
        bool applies = true;

        if (r.has("os")) {
            const mjson::Value& os = r.get("os");
            std::string n = os.get("name").asString();
            if (!n.empty() && n != "windows") applies = false;
            std::string a = os.get("arch").asString();
            if (!a.empty() && a != "x86_64") applies = false;
        }
        // 不使用任何可选 feature(演示模式/自定义分辨率等)
        if (r.has("features")) applies = false;

        if (applies) allowed = (action == "allow");
    }
    return allowed;
}

// "org.lwjgl:lwjgl:3.3.1"             -> org/lwjgl/lwjgl/3.3.1/lwjgl-3.3.1.jar
// "org.lwjgl:lwjgl:3.3.1:natives-win" -> .../lwjgl-3.3.1-natives-win.jar
std::string mavenToPath(const std::string& name) {
    std::vector<std::string> p = splitStr(name, ':');
    if (p.size() < 3) return std::string();

    const std::string& group    = p[0];
    const std::string& artifact = p[1];
    const std::string& version  = p[2];
    std::string classifier, ext = "jar";

    if (p.size() >= 5) { ext = p[3]; classifier = p[4]; }
    else if (p.size() == 4) { classifier = p[3]; }

    std::string g = replaceAll(group, ".", "/");
    std::string file = artifact + "-" + version;
    if (!classifier.empty()) file += "-" + classifier;
    file += "." + ext;

    return g + "/" + artifact + "/" + version + "/" + file;
}

struct DlItem {
    std::wstring url;
    std::wstring fallback;   // mirror 失败时回退的官方源 URL
    std::wstring local;
    unsigned long long size;
};

// 多线程批量下载; onTick(done, total) 返回 false 表示取消
bool parallelDownload(const std::vector<DlItem>& items, int threads,
                      std::function<bool(int, int)> onTick,
                      std::atomic<bool>& cancelled,
                      std::wstring& errOut) {
    if (items.empty()) return true;

    std::atomic<size_t> next{ 0 };
    std::atomic<int> done{ 0 };
    std::atomic<bool> failed{ false };
    std::mutex emx;
    std::wstring firstErr;
    std::vector<std::wstring> failures;
    const int total = (int)items.size();

    int n = threads;
    if (n > total) n = total;
    if (n < 1) n = 1;

    std::vector<std::thread> pool;
    for (int t = 0; t < n; ++t) {
        pool.push_back(std::thread([&]() {
            std::map<std::wstring, std::shared_ptr<HttpClient>> clients;
            while (!cancelled.load()) {
                size_t k = next.fetch_add(1);
                if (k >= items.size()) break;
                const DlItem& it = items[k];

                // 已完整则跳过
                if (!it.local.empty()) {
                    unsigned long long have = fileSizeOf(it.local);
                    if (it.size > 0 && have == it.size) {
                        done.fetch_add(1);
                        if (onTick && !onTick(done.load(), total)) { cancelled = true; }
                        continue;
                    }
                    if (it.size == 0 && have != (unsigned long long)-1) {
                        done.fetch_add(1);
                        if (onTick && !onTick(done.load(), total)) { cancelled = true; }
                        continue;
                    }
                }

                std::wstring host, path;
                bool https = true;
                INTERNET_PORT port = 0;
                if (!urlSplit(it.url, host, path, https, port)) {
                    std::lock_guard<std::mutex> lk(emx);
                    failures.push_back(fileNameOf(it.local) + L" (URL 无效)");
                    done.fetch_add(1);
                    if (onTick && !onTick(done.load(), total)) { cancelled = true; break; }
                    continue;
                }

                auto ci = clients.find(host);
                if (ci == clients.end()) {
                    clients[host] = std::make_shared<HttpClient>(host, https, port);
                    ci = clients.find(host);
                }

                std::wstring e1;
                bool okOne = ci->second->download(path, it.local, nullptr, e1);
                if (!okOne && !cancelled.load()) {
                    // 同源重试一次(镜像缓存穿透/瞬时错误)
                    Sleep(400);
                    clients[host] = std::make_shared<HttpClient>(host, https, port);
                    okOne = clients[host]->download(path, it.local, nullptr, e1);
                }
                if (!okOne && !it.fallback.empty() && it.fallback != it.url && !cancelled.load()) {
                    std::wstring h2, p2; bool s2 = true; INTERNET_PORT po2 = 0;
                    if (urlSplit(it.fallback, h2, p2, s2, po2)) {
                        auto ci2 = clients.find(h2);
                        if (ci2 == clients.end()) {
                            clients[h2] = std::make_shared<HttpClient>(h2, s2, po2);
                            ci2 = clients.find(h2);
                        }
                        okOne = ci2->second->download(p2, it.local, nullptr, e1);
                    }
                }
                if (!okOne) {
                    // 单个文件失败不中断整体, 记录后继续
                    std::lock_guard<std::mutex> lk(emx);
                    failures.push_back(fileNameOf(it.local) + L" (" + e1 + L")");
                    done.fetch_add(1);
                    if (onTick && !onTick(done.load(), total)) { cancelled = true; break; }
                    continue;
                }

                done.fetch_add(1);
                if (onTick && !onTick(done.load(), total)) { cancelled = true; break; }
            }
        }));
    }
    for (size_t i = 0; i < pool.size(); ++i) pool[i].join();

    if (cancelled.load()) { errOut = L"下载已取消"; return false; }
    if (!failures.empty()) {
        std::wstring m = L"有 " + std::to_wstring(failures.size()) + L" 个文件下载失败: ";
        for (size_t i = 0; i < failures.size() && i < 3; ++i) {
            if (i > 0) m += L"; ";
            m += failures[i];
        }
        if (failures.size() > 3) m += L" ...";
        errOut = m;
        return false;
    }
    return true;
}

} // namespace

// ===========================================================================
//  Launcher
// ===========================================================================

void Launcher::log(const std::wstring& s) {
    appLog(s);
    if (onLog) onLog(s);
}

bool Launcher::tick(int percent, const std::wstring& text) {
    if (cancelled.load()) return false;
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    if (onProgress) return onProgress(percent, text);
    return true;
}

std::wstring Launcher::mirrorUrl(const std::string& url) const {
    std::string u = url;
    if (opt.useMirror) {
        const std::string& m = opt.mirrorHost;
        u = replaceAll(u, "https://launchermeta.mojang.com", m);
        u = replaceAll(u, "https://piston-meta.mojang.com", m);
        u = replaceAll(u, "https://launcher.mojang.com", m);
        u = replaceAll(u, "https://piston-data.mojang.com", m);
        u = replaceAll(u, "https://libraries.minecraft.net", m + "/maven");
        u = replaceAll(u, "https://resources.download.minecraft.net", m + "/assets");
        u = replaceAll(u, "http://launchermeta.mojang.com", m);
        u = replaceAll(u, "http://piston-meta.mojang.com", m);
        u = replaceAll(u, "http://resources.download.minecraft.net", m + "/assets");
    }
    return utf8ToWide(u);
}

std::wstring Launcher::versionDir(const std::string& id) const {
    return joinPath(joinPath(opt.gameDir, L"versions"), utf8ToWide(id));
}

bool Launcher::fetchManifest(std::vector<VersionInfo>& out, std::wstring& err) {
    out.clear();

    std::string body;
    std::wstring e1;
    bool okOne = false;
    if (opt.useMirror) {
        okOne = httpGetToString(utf8ToWide(opt.mirrorHost + "/mc/game/version_manifest.json"), body, e1);
    }
    if (!okOne) {
        okOne = httpGetToString(L"https://launchermeta.mojang.com/mc/game/version_manifest.json", body, e1);
    }
    if (!okOne) {
        // 再试一次镜像
        okOne = httpGetToString(utf8ToWide(opt.mirrorHost + "/mc/game/version_manifest.json"), body, e1);
    }
    if (!okOne) { err = L"获取版本列表失败: " + e1; return false; }

    mjson::Value root;
    if (!mjson::parse(body, root)) { err = L"版本列表解析失败"; return false; }

    const mjson::Value& vs = root.get("versions");
    for (size_t i = 0; i < vs.size(); ++i) {
        const mjson::Value& v = vs.at(i);
        VersionInfo vi;
        vi.id          = v.get("id").asString();
        vi.type        = v.get("type").asString();
        vi.url         = v.get("url").asString();
        vi.releaseTime = v.get("releaseTime").asString();
        vi.sha1        = v.get("sha1").asString();
        if (!vi.id.empty()) out.push_back(vi);
    }
    if (out.empty()) { err = L"版本列表为空"; return false; }
    return true;
}

bool Launcher::isInstalled(const std::string& id) const {
    std::wstring d = versionDir(id);
    std::wstring j = joinPath(d, utf8ToWide(id) + L".json");
    std::wstring a = joinPath(d, utf8ToWide(id) + L".jar");
    return pathExists(j) && pathExists(a);
}

std::vector<std::string> Launcher::listInstalled() const {
    std::vector<std::string> res;
    std::wstring root = joinPath(opt.gameDir, L"versions");
    if (!isDirectory(root)) return res;

    std::wstring pat = joinPath(root, L"*");
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(pat.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return res;

    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
        if (fd.cFileName[0] == L'.') continue;
        res.push_back(wideToUtf8(fd.cFileName));
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    return res;
}

// ---------------------------------------------------------------------------
//  安装(下载)一个版本
// ---------------------------------------------------------------------------
bool Launcher::install(const VersionInfo& vi, std::wstring& err) {
    createDirs(opt.gameDir);
    std::wstring vdir = versionDir(vi.id);
    std::wstring vidW = utf8ToWide(vi.id);
    std::wstring vjsonPath = joinPath(vdir, vidW + L".json");
    createDirs(vdir);

    // ---- 1. 版本描述文件 ------------------------------------------------
    if (!tick(1, L"获取版本描述文件...")) { err = L"已取消"; return false; }

    std::string verText;
    if (!readAll(vjsonPath, verText) || verText.empty()) {
        std::wstring u = mirrorUrl(vi.url);
        std::wstring e1;
        bool okOne = httpDownloadToFile(u, vjsonPath, nullptr, e1);
        if (!okOne && opt.useMirror) {
            okOne = httpDownloadToFile(utf8ToWide(vi.url), vjsonPath, nullptr, e1);
        }
        if (!okOne) { err = L"下载版本描述失败: " + e1; return false; }
        if (!readAll(vjsonPath, verText)) { err = L"版本描述读取失败"; return false; }
    }

    mjson::Value root;
    if (!mjson::parse(verText, root)) { err = L"版本 JSON 解析失败"; return false; }

    std::string mainClass = root.get("mainClass").asString();
    if (mainClass.empty()) { err = L"版本 JSON 缺少 mainClass"; return false; }

    log(L"开始安装版本 " + vidW);

    // ---- 2. 客户端主 jar ------------------------------------------------
    const mjson::Value& dl = root.get("downloads");
    const mjson::Value& client = dl.get("client");
    std::wstring jarPath = joinPath(vdir, vidW + L".jar");

    if (client.isObject()) {
        unsigned long long csize = (unsigned long long)client.get("size").asNumber();
        std::string curl = client.get("url").asString();
        if (!curl.empty() && fileSizeOf(jarPath) != csize) {
            if (!tick(3, L"下载游戏主程序...")) { err = L"已取消"; return false; }
            std::wstring e1;
            if (!httpDownloadToFile(mirrorUrl(curl), jarPath, nullptr, e1)) {
                if (!httpDownloadToFile(utf8ToWide(curl), jarPath, nullptr, e1)) {
                    err = L"下载客户端失败: " + e1;
                    return false;
                }
            }
        }
    }

    // ---- 3. 依赖库 ------------------------------------------------------
    if (!tick(12, L"整理依赖库...")) { err = L"已取消"; return false; }

    std::wstring libRoot = joinPath(opt.gameDir, L"libraries");
    std::vector<DlItem> libItems;
    std::vector<std::pair<std::string, std::wstring> > nativeJars; // (url, local)

    const mjson::Value& libs = root.get("libraries");
    for (size_t i = 0; i < libs.size(); ++i) {
        const mjson::Value& lib = libs.at(i);
        if (!ruleAllows(lib.get("rules"))) continue;

        std::string name = lib.get("name").asString();
        const mjson::Value& dls = lib.get("downloads");

        if (lib.has("natives")) {
            std::string key = lib.get("natives").get("windows").asString();
            if (!key.empty()) {
                const mjson::Value& cls = dls.get("classifiers").get(key);
                if (cls.isObject()) {
                    std::string path = cls.get("path").asString();
                    std::string url  = cls.get("url").asString();
                    if (path.empty()) path = mavenToPath(name + ":" + key);
                    if (!url.empty() && !path.empty()) {
                        std::wstring local = joinPath(libRoot, utf8ToWide(replaceAll(path, "/", "\\")));
                        DlItem it;
                        it.url = mirrorUrl(url);
                        it.fallback = utf8ToWide(url);
                        it.local = local;
                        it.size = (unsigned long long)cls.get("size").asNumber();
                        libItems.push_back(it);
                        nativeJars.push_back(std::make_pair(url, local));
                    }
                }
            }
            continue;
        }
        {
            std::string n = lib.get("name").asString();
            if (n.find(":natives-windows") != std::string::npos &&
                n.find("arm64") == std::string::npos && n.find("-x86") == std::string::npos) {
                const mjson::Value& art = dls.get("artifact");
                std::string path = art.get("path").asString();
                std::string url = art.get("url").asString();
                if (!path.empty() && !url.empty()) {
                    std::wstring local = joinPath(libRoot, utf8ToWide(replaceAll(path, "/", "\\")));
                    DlItem it;
                    it.url = mirrorUrl(url);
                    it.fallback = utf8ToWide(url);
                    it.local = local;
                    it.size = (unsigned long long)art.get("size").asNumber();
                    libItems.push_back(it);
                    nativeJars.push_back(std::make_pair(url, local));
                }
                continue;
            }
        }

        const mjson::Value& art = dls.get("artifact");
        std::string path, url;
        unsigned long long asize = 0;

        if (art.isObject()) {
            path = art.get("path").asString();
            url  = art.get("url").asString();
            asize = (unsigned long long)art.get("size").asNumber();
        } else if (lib.has("url")) {
            path = mavenToPath(name);
            url  = lib.get("url").asString() + path;
        }
        if (path.empty()) path = mavenToPath(name);
        if (path.empty()) continue;

        std::wstring local = joinPath(libRoot, utf8ToWide(replaceAll(path, "/", "\\")));

        bool isNative = (name.find(":natives-") != std::string::npos);
        if (isNative) nativeJars.push_back(std::make_pair(url, local));

        if (!url.empty()) {
            DlItem it;
            it.url = mirrorUrl(url);
            it.fallback = utf8ToWide(url);
            it.local = local;
            it.size = asize;
            libItems.push_back(it);
        }
    }

    if (!libItems.empty()) {
        std::wstring e1;
        bool okOne = parallelDownload(libItems, 4,
            [&](int d, int t) {
                int p = 12 + (int)((double)(28) * d / (t > 0 ? t : 1));
                return tick(p, L"下载依赖库 " + std::to_wstring(d) + L"/" + std::to_wstring(t));
            }, cancelled, e1);
        if (!okOne) { err = L"依赖库下载失败: " + e1; return false; }
    }

    // ---- 4. 资源索引 ----------------------------------------------------
    if (!tick(41, L"获取资源索引...")) { err = L"已取消"; return false; }

    std::wstring assetsRoot = joinPath(opt.gameDir, L"assets");
    std::string indexId = root.get("assetIndex").get("id").asString();
    if (indexId.empty()) indexId = root.get("assets").asString();

    std::string indexText;
    if (!indexId.empty()) {
        std::wstring indexPath = joinPath(joinPath(assetsRoot, L"indexes"), utf8ToWide(indexId) + L".json");
        if (!readAll(indexPath, indexText) || indexText.empty()) {
            std::string iurl = root.get("assetIndex").get("url").asString();
            if (iurl.empty())
                iurl = "https://piston-meta.mojang.com/v1/packages/" + root.get("assetIndex").get("sha1").asString()
                       + "/" + indexId + ".json";
            std::wstring e1;
            if (!httpDownloadToFile(mirrorUrl(iurl), indexPath, nullptr, e1)) {
                if (!httpDownloadToFile(utf8ToWide(iurl), indexPath, nullptr, e1)) {
                    err = L"下载资源索引失败: " + e1;
                    return false;
                }
            }
            readAll(indexPath, indexText);
        }
    }

    // ---- 5. 资源对象 ----------------------------------------------------
    mjson::Value index;
    if (!indexText.empty() && mjson::parse(indexText, index)) {
        const mjson::Value& objs = index.get("objects");
        std::vector<DlItem> assetItems;
        assetItems.reserve(objs.size());

        std::wstring objRoot = joinPath(assetsRoot, L"objects");
        for (size_t i = 0; i < objs.size(); ++i) {
            const std::string& h = objs.obj[i].second.get("hash").asString();
            if (h.size() < 4) continue;
            unsigned long long sz = (unsigned long long)objs.obj[i].second.get("size").asNumber();

            std::string sub = h.substr(0, 2);
            std::wstring rel = utf8ToWide(sub + "\\" + h);
            std::wstring local = joinPath(objRoot, rel);

            std::string aurl = "https://resources.download.minecraft.net/" + sub + "/" + h;
            DlItem it;
            it.url = mirrorUrl(aurl);
            it.fallback = utf8ToWide(aurl);
            it.local = local;
            it.size = sz;
            assetItems.push_back(it);
        }

        if (!assetItems.empty()) {
            // 先剔除已完成项, 避免海量任务入队
            std::vector<DlItem> todo;
            todo.reserve(assetItems.size());
            for (size_t i = 0; i < assetItems.size(); ++i) {
                const DlItem& it = assetItems[i];
                unsigned long long have = fileSizeOf(it.local);
                if (it.size > 0 && have == it.size) continue;
                todo.push_back(it);
            }
            log(L"资源文件: 共 " + std::to_wstring(assetItems.size()) + L" 个, 待下载 "
                + std::to_wstring(todo.size()) + L" 个");

            if (!todo.empty()) {
                std::wstring e1;
                bool okOne = parallelDownload(todo, 8,
                    [&](int d, int t) {
                        int p = 42 + (int)((double)(52) * d / (t > 0 ? t : 1));
                        return tick(p, L"下载资源文件 " + std::to_wstring(d) + L"/" + std::to_wstring(t));
                    }, cancelled, e1);
                if (!okOne) { err = L"资源文件下载失败: " + e1; return false; }
            }
        }
    }

    // ---- 6. 解压 natives ------------------------------------------------
    if (!tick(95, L"解压本地库...")) { err = L"已取消"; return false; }

    std::wstring nativesDir = joinPath(vdir, L"natives");
    if (!pathExists(joinPath(nativesDir, L"lwjgl.dll"))) {
        createDirs(nativesDir);
        const mjson::Value& nlibs = root.get("libraries");
        for (size_t i = 0; i < nlibs.size(); ++i) {
            const mjson::Value& lib = nlibs.at(i);
            if (!ruleAllows(lib.get("rules"))) continue;
            std::string path;
            if (lib.has("natives")) {
                std::string key = lib.get("natives").get("windows").asString();
                if (key.empty()) continue;
                path = lib.get("downloads").get("classifiers").get(key).get("path").asString();
            } else {
                std::string n = lib.get("name").asString();
                if (n.find(":natives-windows") == std::string::npos ||
                    n.find("arm64") != std::string::npos || n.find("-x86") != std::string::npos) continue;
                path = lib.get("downloads").get("artifact").get("path").asString();
            }
            if (path.empty()) continue;
            std::wstring jar = joinPath(libRoot, utf8ToWide(replaceAll(path, "/", "\\")));
            if (!pathExists(jar)) continue;
            runAndWait(L"tar.exe -xf \"" + jar + L"\" -C \"" + nativesDir + L"\"");
        }
        if (!pathExists(joinPath(nativesDir, L"lwjgl.dll"))) {
            std::wstring pid = utf8ToWide(root.get("inheritsFrom").asString());
            if (!pid.empty()) {
                std::wstring parentNatives = joinPath(joinPath(joinPath(opt.gameDir, L"versions"), pid), L"natives");
                if (pathExists(joinPath(parentNatives, L"lwjgl.dll"))) nativesDir = parentNatives;
            }
        }
    }
    if (!nativeJars.empty()) {
        createDirs(nativesDir);
        for (size_t i = 0; i < nativeJars.size(); ++i) {
            const std::wstring& jar = nativeJars[i].second;
            if (!pathExists(jar)) continue;
            std::wstring cmd = L"tar.exe -xf \"" + jar + L"\" -C \"" + nativesDir + L"\"";
            runAndWait(cmd);
        }
    }

    if (!tick(100, L"安装完成")) { err = L"已取消"; return false; }
    log(L"版本 " + vidW + L" 安装完成");
    return true;
}

// ---------------------------------------------------------------------------
//  启动游戏
// ---------------------------------------------------------------------------
void jsonSetField(mjson::Value& obj, const std::string& key, const mjson::Value& val) {
    for (auto& kv : obj.obj) {
        if (kv.first == key) { kv.second = val; return; }
    }
    obj.obj.push_back({ key, val });
}

void mergeInheritedVersion(const mjson::Value& parent, mjson::Value& child) {
    const char* scalars[] = { "mainClass", "assetIndex", "assets", "downloads", "logging", "javaVersion", "complianceLevel" };
    for (const char* k : scalars) {
        const mjson::Value& cv = child.get(k);
        bool empty = !child.has(k) || cv.isNull() ||
                     (cv.isObject() && cv.size() == 0) ||
                     (cv.isString() && cv.asString().empty());
        if (empty && parent.has(k)) jsonSetField(child, k, parent.get(k));
    }
    if (child.get("type").asString().empty() && parent.has("type")) jsonSetField(child, "type", parent.get("type"));
    if (child.get("releaseTime").asString().empty() && parent.has("releaseTime"))
        jsonSetField(child, "releaseTime", parent.get("releaseTime"));

    mjson::Value libs;
    libs.type = mjson::Value::Type::Array;
    const mjson::Value& pl = parent.get("libraries");
    for (size_t i = 0; i < pl.size(); ++i) libs.arr.push_back(pl.at(i));
    const mjson::Value& cl = child.get("libraries");
    for (size_t i = 0; i < cl.size(); ++i) libs.arr.push_back(cl.at(i));
    jsonSetField(child, "libraries", libs);

    if (parent.has("arguments") || child.has("arguments")) {
        mjson::Value args;
        args.type = mjson::Value::Type::Object;
        const char* keys[] = { "jvm", "game" };
        for (const char* k : keys) {
            mjson::Value arr;
            arr.type = mjson::Value::Type::Array;
            if (parent.has("arguments")) {
                const mjson::Value& pa = parent.get("arguments").get(k);
                for (size_t i = 0; i < pa.size(); ++i) arr.arr.push_back(pa.at(i));
            }
            if (child.has("arguments")) {
                const mjson::Value& ca = child.get("arguments").get(k);
                for (size_t i = 0; i < ca.size(); ++i) arr.arr.push_back(ca.at(i));
            }
            if (!arr.arr.empty()) args.obj.push_back({ k, arr });
        }
        if (!args.obj.empty()) jsonSetField(child, "arguments", args);
    }
    if (child.get("minecraftArguments").asString().empty() && parent.has("minecraftArguments"))
        jsonSetField(child, "minecraftArguments", parent.get("minecraftArguments"));
}
bool Launcher::launch(const std::string& id, std::wstring& err) {
    std::wstring vdir = versionDir(id);
    std::wstring vidW = utf8ToWide(id);
    std::wstring jsonPath = joinPath(vdir, vidW + L".json");
    std::wstring jarPath  = joinPath(vdir, vidW + L".jar");

    std::string text;
    if (!readAll(jsonPath, text)) { err = L"未找到版本文件, 请先下载该版本"; return false; }

    mjson::Value root;
    if (!mjson::parse(text, root)) { err = L"版本 JSON 解析失败"; return false; }

    // 继承版本 (Fabric/Forge 等): 合并父版本 JSON, 子版本没有 jar 时使用父版本 jar
    if (root.has("inheritsFrom")) {
        std::string parentId = root.get("inheritsFrom").asString();
        if (!parentId.empty()) {
            std::wstring parentW = utf8ToWide(parentId);
            std::wstring parentDir = joinPath(joinPath(opt.gameDir, L"versions"), parentW);
            std::wstring parentJson = joinPath(parentDir, parentW + L".json");
            std::string parentText;
            if (!readAll(parentJson, parentText)) {
                err = L"父版本 " + parentW + L" 的 JSON 未找到, 请先安装该版本";
                return false;
            }
            mjson::Value parent;
            if (!mjson::parse(parentText, parent)) { err = L"父版本 JSON 解析失败"; return false; }
            mergeInheritedVersion(parent, root);
            std::wstring parentJar = joinPath(parentDir, parentW + L".jar");
            if (!pathExists(jarPath) && pathExists(parentJar)) jarPath = parentJar;
        }
    }
    if (!pathExists(jarPath)) { err = L"未找到游戏主程序, 请先下载该版本"; return false; }

    std::wstring javaExe = opt.javaPath;
    if (javaExe.empty() || !pathExists(javaExe)) {
        javaExe = autoDetectJava();
        if (javaExe.empty()) { err = L"未找到 Java, 请先在设置中指定 java.exe 路径"; return false; }
    }

    std::string mainClass   = root.get("mainClass").asString();
    std::string versionType = root.get("type").asString("release");
    std::string indexId     = root.get("assetIndex").get("id").asString();
    if (indexId.empty()) indexId = root.get("assets").asString();

    std::wstring libRoot    = joinPath(opt.gameDir, L"libraries");
    std::wstring nativesDir = joinPath(vdir, L"natives");
    if (!pathExists(joinPath(nativesDir, L"lwjgl.dll"))) {
        createDirs(nativesDir);
        const mjson::Value& nlibs = root.get("libraries");
        for (size_t i = 0; i < nlibs.size(); ++i) {
            const mjson::Value& lib = nlibs.at(i);
            if (!ruleAllows(lib.get("rules"))) continue;
            std::string path;
            if (lib.has("natives")) {
                std::string key = lib.get("natives").get("windows").asString();
                if (key.empty()) continue;
                path = lib.get("downloads").get("classifiers").get(key).get("path").asString();
            } else {
                std::string n = lib.get("name").asString();
                if (n.find(":natives-windows") == std::string::npos ||
                    n.find("arm64") != std::string::npos || n.find("-x86") != std::string::npos) continue;
                path = lib.get("downloads").get("artifact").get("path").asString();
            }
            if (path.empty()) continue;
            std::wstring jar = joinPath(libRoot, utf8ToWide(replaceAll(path, "/", "\\")));
            if (!pathExists(jar)) continue;
            runAndWait(L"tar.exe -xf \"" + jar + L"\" -C \"" + nativesDir + L"\"");
        }
        if (!pathExists(joinPath(nativesDir, L"lwjgl.dll"))) {
            std::wstring pid = utf8ToWide(root.get("inheritsFrom").asString());
            if (!pid.empty()) {
                std::wstring parentNatives = joinPath(joinPath(joinPath(opt.gameDir, L"versions"), pid), L"natives");
                if (pathExists(joinPath(parentNatives, L"lwjgl.dll"))) nativesDir = parentNatives;
            }
        }
    }
    std::wstring assetsRoot = joinPath(opt.gameDir, L"assets");
    std::wstring uuid       = utf8ToWide(offlineUuid(opt.playerName));

    // ---- classpath ------------------------------------------------------
    std::wstring classpath;
    const mjson::Value& libs = root.get("libraries");
    for (size_t i = 0; i < libs.size(); ++i) {
        const mjson::Value& lib = libs.at(i);
        if (!ruleAllows(lib.get("rules"))) continue;
        if (lib.has("natives")) continue;

        std::string name = lib.get("name").asString();
        if (name.find(":natives-") != std::string::npos) continue;

        std::string path = lib.get("downloads").get("artifact").get("path").asString();
        if (path.empty()) path = mavenToPath(name);
        if (path.empty()) continue;

        std::wstring local = joinPath(libRoot, utf8ToWide(replaceAll(path, "/", "\\")));
        if (!classpath.empty()) classpath += L";";
        classpath += local;
    }
    if (!classpath.empty()) classpath += L";";
    classpath += jarPath;

    // ---- 参数收集 -------------------------------------------------------
    std::vector<std::string> jvmRaw, gameRaw;

    if (root.has("arguments")) {
        const mjson::Value& jvm = root.get("arguments").get("jvm");
        for (size_t i = 0; i < jvm.size(); ++i) {
            const mjson::Value& e = jvm.at(i);
            if (e.isString()) { jvmRaw.push_back(e.asString()); }
            else if (e.isObject() && ruleAllows(e.get("rules"))) {
                const mjson::Value& v = e.get("value");
                if (v.isString()) jvmRaw.push_back(v.asString());
                else for (size_t k = 0; k < v.size(); ++k)
                    if (v.at(k).isString()) jvmRaw.push_back(v.at(k).asString());
            }
        }
        const mjson::Value& game = root.get("arguments").get("game");
        for (size_t i = 0; i < game.size(); ++i) {
            const mjson::Value& e = game.at(i);
            if (e.isString()) { gameRaw.push_back(e.asString()); }
            else if (e.isObject() && ruleAllows(e.get("rules"))) {
                const mjson::Value& v = e.get("value");
                if (v.isString()) gameRaw.push_back(v.asString());
                else for (size_t k = 0; k < v.size(); ++k)
                    if (v.at(k).isString()) gameRaw.push_back(v.at(k).asString());
            }
        }
    } else {
        std::string ma = root.get("minecraftArguments").asString();
        std::vector<std::string> parts = splitStr(ma, ' ');
        for (size_t i = 0; i < parts.size(); ++i)
            if (!parts[i].empty()) gameRaw.push_back(parts[i]);
    }

    // ---- 占位符替换 -----------------------------------------------------
    std::string cpUtf8 = wideToUtf8(classpath);
    std::string gameDirU8 = wideToUtf8(opt.gameDir);
    std::string nativesU8 = wideToUtf8(nativesDir);
    std::string assetsU8  = wideToUtf8(assetsRoot);

    // ---- 联机直连 (由 UI 注入, 为空时保持原版行为不变)
    std::string serverArg;                      // host:port, 供 ${quickPlayMultiplayer} 使用
    int serverPort = 0;
    if (!opt.serverAddress.empty()) {
        serverPort = opt.serverPort > 0 ? opt.serverPort : 25565;
        serverArg = opt.serverAddress + ":" + std::to_string(serverPort);
    }

    struct Repl { const char* key; std::string val; };
    std::vector<Repl> repls;
    {
        Repl r;
        r.key = "${auth_player_name}";   r.val = opt.playerName;              repls.push_back(r);
        r.key = "${version_name}";       r.val = id;                          repls.push_back(r);
        r.key = "${game_directory}";     r.val = gameDirU8;                   repls.push_back(r);
        r.key = "${assets_root}";        r.val = assetsU8;                    repls.push_back(r);
        r.key = "${assets_index_name}";  r.val = indexId;                     repls.push_back(r);
        r.key = "${auth_uuid}";          r.val = wideToUtf8(uuid);            repls.push_back(r);
        r.key = "${auth_access_token}";  r.val = "0";                         repls.push_back(r);
        r.key = "${auth_session}";       r.val = "0";                         repls.push_back(r);
        r.key = "${user_type}";          r.val = "legacy";                    repls.push_back(r);
        r.key = "${version_type}";       r.val = versionType;                 repls.push_back(r);
        r.key = "${natives_directory}";  r.val = nativesU8;                   repls.push_back(r);
        r.key = "${launcher_name}";      r.val = "MCL";                       repls.push_back(r);
        r.key = "${launcher_version}";   r.val = "1.0";                       repls.push_back(r);
        r.key = "${classpath}";          r.val = cpUtf8;                      repls.push_back(r);
        r.key = "${game_assets}";        r.val = assetsU8;                    repls.push_back(r);
        r.key = "${user_properties}";    r.val = "{}";                        repls.push_back(r);
        r.key = "${clientid}";           r.val = "";                          repls.push_back(r);
        r.key = "${auth_xuid}";          r.val = "";                          repls.push_back(r);
        r.key = "${user_properties}";    r.val = "{}";                        repls.push_back(r);
        r.key = "${resolution_width}";   r.val = "854";                       repls.push_back(r);
        r.key = "${resolution_height}";  r.val = "480";                       repls.push_back(r);
        r.key = "${quickPlayPath}";      r.val = "";                          repls.push_back(r);
        r.key = "${quickPlaySingleplayer}"; r.val = "";                       repls.push_back(r);
        r.key = "${quickPlayMultiplayer}";  r.val = serverArg;                repls.push_back(r);
    }

    std::function<std::string(const std::string&)> fill = [&](const std::string& s) -> std::string {
        std::string o = s;
        for (size_t i = 0; i < repls.size(); ++i)
            o = replaceAll(o, repls[i].key, repls[i].val);
        return o;
    };

    // ---- 组装命令行 -----------------------------------------------------
    std::wstring cmd = L"\"" + javaExe + L"\"";
    cmd += L" -Xmx" + std::to_wstring(opt.maxMemoryMB) + L"M";
    cmd += L" -Xms512M";

    for (size_t i = 0; i < jvmRaw.size(); ++i) {
        std::string a = fill(jvmRaw[i]);
        if (a.empty()) continue;
        cmd += L" \"" + utf8ToWide(a) + L"\"";
    }

    cmd += L" \"" + utf8ToWide(mainClass) + L"\"";

    for (size_t i = 0; i < gameRaw.size(); ++i) {
        std::string a = fill(gameRaw[i]);
        if (a.empty()) continue;
        cmd += L" \"" + utf8ToWide(a) + L"\"";
    }

    // ---- 联机直连: 追加原版客户端原生支持的 --server/--port, 启动后直接进服
    if (!serverArg.empty()) {
        cmd += L" --server \"" + utf8ToWide(opt.serverAddress) + L"\"";
        cmd += L" --port " + std::to_wstring(serverPort);
        log(L"联机直连目标: " + utf8ToWide(serverArg));
    }

    log(L"启动命令: " + cmd);

    std::vector<wchar_t> buf(cmd.begin(), cmd.end());
    buf.push_back(0);

    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessW(nullptr, buf.data(), nullptr, nullptr, FALSE, 0,
                        nullptr, opt.gameDir.c_str(), &si, &pi)) {
        DWORD code = GetLastError();
        err = L"启动进程失败 (错误码 " + std::to_wstring(code) + L")";
        return false;
    }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    log(L"游戏进程已启动");
    return true;
}

// ---------------------------------------------------------------------------
//  Java 探测
// ---------------------------------------------------------------------------
std::wstring Launcher::autoDetectJava() {
    // 1) PATH
    wchar_t found[MAX_PATH] = { 0 };
    if (SearchPathW(nullptr, L"javaw.exe", nullptr, MAX_PATH, found, nullptr)) {
        // 优先返回同一目录下的 java.exe
        std::wstring p = found;
        std::wstring dir = dirNameOf(p);
        std::wstring j = joinPath(dir, L"java.exe");
        if (pathExists(j)) return j;
        if (pathExists(p)) return p;
    }

    // 2) 常见安装位置
    std::vector<std::wstring> roots;
    wchar_t pf[MAX_PATH] = { 0 }, pf86[MAX_PATH] = { 0 }, lad[MAX_PATH] = { 0 };
    if (GetEnvironmentVariableW(L"ProgramFiles", pf, MAX_PATH)) roots.push_back(pf);
    if (GetEnvironmentVariableW(L"ProgramFiles(x86)", pf86, MAX_PATH)) roots.push_back(pf86);
    if (GetEnvironmentVariableW(L"LOCALAPPDATA", lad, MAX_PATH))
        roots.push_back(joinPath(lad, L"Programs"));

    const wchar_t* vendors[] = {
        L"Java", L"Eclipse Adoptium", L"Eclipse Foundation", L"Microsoft",
        L"Zulu", L"BellSoft", L"Amazon Corretto", L"Semeru", L"Common Files\\Oracle\\Java"
    };

    for (size_t i = 0; i < roots.size(); ++i) {
        for (size_t v = 0; v < sizeof(vendors) / sizeof(vendors[0]); ++v) {
            std::wstring base = joinPath(roots[i], vendors[v]);
            if (!isDirectory(base)) continue;

            WIN32_FIND_DATAW fd;
            HANDLE h = FindFirstFileW(joinPath(base, L"*").c_str(), &fd);
            if (h == INVALID_HANDLE_VALUE) continue;
            do {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
                if (fd.cFileName[0] == L'.') continue;
                std::wstring j = joinPath(joinPath(joinPath(base, fd.cFileName), L"bin"), L"java.exe");
                if (pathExists(j)) { FindClose(h); return j; }
            } while (FindNextFileW(h, &fd));
            FindClose(h);
        }
    }

    // 3) 其他盘符的常见目录
    std::vector<std::wstring> extra;
    extra.push_back(L"E:\\ide java");
    extra.push_back(L"E:\\ide");
    extra.push_back(L"E:\\java");
    extra.push_back(L"D:\\java");
    for (size_t i = 0; i < extra.size(); ++i) {
        if (!isDirectory(extra[i])) continue;
        std::wstring j = joinPath(joinPath(extra[i], L"bin"), L"java.exe");
        if (pathExists(j)) return j;

        WIN32_FIND_DATAW fd;
        HANDLE h = FindFirstFileW(joinPath(extra[i], L"*").c_str(), &fd);
        if (h == INVALID_HANDLE_VALUE) continue;
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
            if (fd.cFileName[0] == L'.') continue;
            std::wstring j2 = joinPath(joinPath(joinPath(extra[i], fd.cFileName), L"bin"), L"java.exe");
            if (pathExists(j2)) { FindClose(h); return j2; }
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }

    return std::wstring();
}

int Launcher::queryJavaMajor(const std::wstring& javaExe) {
    if (javaExe.empty()) return 0;

    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = nullptr;
    sa.bInheritHandle = TRUE;

    HANDLE rd = nullptr, wr = nullptr;
    if (!CreatePipe(&rd, &wr, &sa, 0)) return 0;
    SetHandleInformation(rd, HANDLE_FLAG_INHERIT, 0);

    std::wstring cmd = L"\"" + javaExe + L"\" -version";
    std::vector<wchar_t> buf(cmd.begin(), cmd.end());
    buf.push_back(0);

    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdOutput = wr;
    si.hStdError  = wr;
    si.hStdInput  = nullptr;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessW(nullptr, buf.data(), nullptr, nullptr, TRUE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        CloseHandle(rd);
        CloseHandle(wr);
        return 0;
    }
    CloseHandle(wr);

    std::string out;
    char tmp[1024];
    DWORD got = 0;
    while (ReadFile(rd, tmp, sizeof(tmp), &got, nullptr) && got > 0)
        out.append(tmp, got);

    WaitForSingleObject(pi.hProcess, 5000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(rd);

    // 解析 version "17.0.1" 或 "1.8.0_302"
    size_t p = out.find("version \"");
    if (p == std::string::npos) return 0;
    p += 9;
    size_t e = out.find('"', p);
    if (e == std::string::npos) return 0;
    std::string ver = out.substr(p, e - p);

    if (!ver.empty() && ver[0] == '1' && ver.size() > 2 && ver[1] == '.')
        return atoi(ver.c_str() + 2);

    return atoi(ver.c_str());
}
