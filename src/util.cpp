// ---------------------------------------------------------------------------
//  util.cpp
// ---------------------------------------------------------------------------
#include "util.h"

#include <cstdio>
#include <cstdarg>
#include <algorithm>

std::wstring utf8ToWide(const std::string& s) {
    if (s.empty()) return std::wstring();
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    if (n <= 0) return std::wstring();
    std::wstring w((size_t)n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}

std::string wideToUtf8(const std::wstring& s) {
    if (s.empty()) return std::string();
    int n = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0, nullptr, nullptr);
    if (n <= 0) return std::string();
    std::string a((size_t)n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), &a[0], n, nullptr, nullptr);
    return a;
}

bool pathExists(const std::wstring& path) {
    DWORD a = GetFileAttributesW(path.c_str());
    return a != INVALID_FILE_ATTRIBUTES;
}

bool isDirectory(const std::wstring& path) {
    DWORD a = GetFileAttributesW(path.c_str());
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool createDirs(const std::wstring& path) {
    if (path.empty()) return false;
    if (isDirectory(path)) return true;
    std::wstring p = path;
    while (!p.empty() && (p.back() == L'\\' || p.back() == L'/')) p.pop_back();
    if (p.empty()) return false;

    std::wstring cur;
    size_t i = 0;
    if (p.size() >= 2 && p[1] == L':') { cur = p.substr(0, 2); i = 2; }
    else if (p.size() >= 1 && p[0] == L'\\') { cur = L"\\"; i = 1; }

    while (i < p.size()) {
        size_t j = p.find_first_of(L"\\/", i);
        std::wstring seg = (j == std::wstring::npos) ? p.substr(i) : p.substr(i, j - i);
        if (!seg.empty()) {
            if (!cur.empty() && cur.back() != L'\\') cur += L'\\';
            cur += seg;
            CreateDirectoryW(cur.c_str(), nullptr);
        }
        if (j == std::wstring::npos) break;
        i = j + 1;
    }
    return isDirectory(path);
}

bool ensureParentDir(const std::wstring& filePath) {
    std::wstring d = dirNameOf(filePath);
    if (d.empty()) return true;
    return createDirs(d);
}

std::wstring joinPath(const std::wstring& a, const std::wstring& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    std::wstring r = a;
    if (r.back() != L'\\' && r.back() != L'/') r += L'\\';
    if (b.front() == L'\\' || b.front() == L'/') r += b.substr(1);
    else r += b;
    return r;
}

std::wstring fileNameOf(const std::wstring& path) {
    size_t p = path.find_last_of(L"\\/");
    return (p == std::wstring::npos) ? path : path.substr(p + 1);
}

std::wstring dirNameOf(const std::wstring& path) {
    size_t p = path.find_last_of(L"\\/");
    return (p == std::wstring::npos) ? std::wstring() : path.substr(0, p);
}

std::wstring trimW(const std::wstring& s) {
    size_t b = 0, e = s.size();
    while (b < e && (s[b] == L' ' || s[b] == L'\t' || s[b] == L'\r' || s[b] == L'\n')) ++b;
    while (e > b && (s[e - 1] == L' ' || s[e - 1] == L'\t' || s[e - 1] == L'\r' || s[e - 1] == L'\n')) --e;
    return s.substr(b, e - b);
}

std::vector<std::wstring> splitW(const std::wstring& s, wchar_t delim) {
    std::vector<std::wstring> out;
    std::wstring cur;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == delim) { out.push_back(cur); cur.clear(); }
        else cur += s[i];
    }
    out.push_back(cur);
    return out;
}

std::string replaceAll(std::string s, const std::string& from, const std::string& to) {
    if (from.empty()) return s;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

std::wstring replaceAllW(std::wstring s, const std::wstring& from, const std::wstring& to) {
    if (from.empty()) return s;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::wstring::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

std::wstring humanSize(unsigned long long bytes) {
    const wchar_t* units[] = { L"B", L"KB", L"MB", L"GB", L"TB" };
    double v = (double)bytes;
    int u = 0;
    while (v >= 1024.0 && u < 4) { v /= 1024.0; ++u; }
    wchar_t buf[64];
    swprintf(buf, 64, (u == 0) ? L"%.0f %s" : L"%.1f %s", v, units[u]);
    return buf;
}

static std::wstring g_logPath;

std::wstring exeDir() {
    wchar_t buf[MAX_PATH] = { 0 };
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring s = buf;
    size_t p = s.find_last_of(L"\\/");
    return (p == std::wstring::npos) ? s : s.substr(0, p);
}

void appLog(const std::wstring& msg) {
    if (g_logPath.empty()) {
        std::wstring d = exeDir();
        g_logPath = joinPath(d, L"MCL.log");
        // 启动时截断
        HANDLE h = CreateFileW(g_logPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                               nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
    }
    FILE* f = _wfopen(g_logPath.c_str(), L"a, ccs=UTF-8");
    if (!f) return;
    SYSTEMTIME st;
    GetLocalTime(&st);
    fwprintf(f, L"[%02d:%02d:%02d] %ls\n", st.wHour, st.wMinute, st.wSecond, msg.c_str());
    fclose(f);
}

// ---------------------------------------------------------------------------
//  自恢复 / 防删除
//  说明: 课堂管理软件（如极域）在开关机时会清理应用。这里在用户目录保存一份备份，
//        并注册“登录时自启”：若主程序被删除就还原，用户可以点程序里的“彻底删除”移除保护。
// ---------------------------------------------------------------------------
static const wchar_t* kGuardRunKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const wchar_t* kGuardRunVal = L"MCLGuard";

std::wstring selfExePath() {
    wchar_t buf[32768] = { 0 };
    DWORD n = GetModuleFileNameW(nullptr, buf, 32768);
    return std::wstring(buf, n);
}

std::wstring selfGuardDir() {
    wchar_t buf[32768] = { 0 };
    DWORD n = GetEnvironmentVariableW(L"LOCALAPPDATA", buf, 32768);
    std::wstring base = (n > 0 && n < 32768) ? std::wstring(buf, n) : exeDir();
    return joinPath(joinPath(base, L"MCL"), L"guard");
}

static bool copyFileOverwrite(const std::wstring& src, const std::wstring& dst) {
    ensureParentDir(dst);
    for (int i = 0; i < 5; ++i) {
        if (CopyFileW(src.c_str(), dst.c_str(), FALSE)) {
            // 去掉“来自网络”标记，避免还原后弹“打开文件-安全警告”
            DeleteFileW((dst + L":Zone.Identifier").c_str());
            return true;
        }
        Sleep(150);
        // 目标被占用时先去掉只读属性再试
        SetFileAttributesW(dst.c_str(), FILE_ATTRIBUTE_NORMAL);
    }
    return false;
}

static bool sameFile(const std::wstring& a, const std::wstring& b) {
    WIN32_FILE_ATTRIBUTE_DATA fa{}, fb{};
    if (!GetFileAttributesExW(a.c_str(), GetFileExInfoStandard, &fa)) return false;
    if (!GetFileAttributesExW(b.c_str(), GetFileExInfoStandard, &fb)) return false;
    return fa.nFileSizeHigh == fb.nFileSizeHigh && fa.nFileSizeLow == fb.nFileSizeLow &&
           CompareFileTime(&fa.ftLastWriteTime, &fb.ftLastWriteTime) == 0;
}

bool selfGuardInstalled() {
    std::wstring guardExe = joinPath(selfGuardDir(), L"MCLauncher.exe");
    if (!pathExists(guardExe)) return false;
    HKEY k = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kGuardRunKey, 0, KEY_READ, &k) != ERROR_SUCCESS) return false;
    wchar_t buf[2048] = { 0 };
    DWORD type = 0, sz = sizeof(buf);
    LONG rc = RegQueryValueExW(k, kGuardRunVal, nullptr, &type, (LPBYTE)buf, &sz);
    RegCloseKey(k);
    return rc == ERROR_SUCCESS && type == REG_SZ;
}

bool selfGuardInstall() {
    std::wstring self = selfExePath();
    if (self.empty()) return false;
    std::wstring dir = selfGuardDir();
    if (!createDirs(dir)) return false;

    std::wstring guardExe = joinPath(dir, L"MCLauncher.exe");
    // 当前运行的若就是备份本体, 不需要再复制
    if (_wcsicmp(self.c_str(), guardExe.c_str()) != 0) {
        if (!pathExists(guardExe) || !sameFile(self, guardExe)) {
            if (!copyFileOverwrite(self, guardExe)) return false;
        }
        // 记录主程序位置, 供登录还原使用
        FILE* f = _wfopen(joinPath(dir, L"state.txt").c_str(), L"wb");
        if (f) {
            std::string line = "path=" + wideToUtf8(self) + "\r\n";
            fwrite(line.data(), 1, line.size(), f);
            fclose(f);
        }
    }

    HKEY k = nullptr;
    if (RegCreateKeyExW(HKEY_CURRENT_USER, kGuardRunKey, 0, nullptr, 0, KEY_SET_VALUE,
                        nullptr, &k, nullptr) != ERROR_SUCCESS) return false;
    std::wstring cmd = L"\"" + guardExe + L"\" --guard";
    LONG rc = RegSetValueExW(k, kGuardRunVal, 0, REG_SZ, (const BYTE*)cmd.c_str(),
                             (DWORD)((cmd.size() + 1) * sizeof(wchar_t)));
    RegCloseKey(k);
    return rc == ERROR_SUCCESS;
}

bool selfGuardRestore(std::wstring& target) {
    std::wstring dir = selfGuardDir();
    std::wstring guardExe = joinPath(dir, L"MCLauncher.exe");
    if (!pathExists(guardExe)) return false;

    std::wstring want;
    FILE* f = _wfopen(joinPath(dir, L"state.txt").c_str(), L"rb");
    if (f) {
        char buf[32768] = { 0 };
        size_t n = fread(buf, 1, sizeof(buf) - 1, f);
        buf[n] = 0;
        fclose(f);
        std::string s(buf, n);
        size_t p = s.find("path=");
        if (p != std::string::npos) {
            size_t e = s.find_first_of("\r\n", p);
            want = utf8ToWide(s.substr(p + 5, (e == std::string::npos ? s.size() : e) - p - 5));
        }
    }
    // 没有记录位置时, 还原到桌面
    if (want.empty()) {
        wchar_t up[32768] = { 0 };
        DWORD n = GetEnvironmentVariableW(L"USERPROFILE", up, 32768);
        std::wstring base = (n > 0) ? std::wstring(up, n) : exeDir();
        want = joinPath(joinPath(base, L"Desktop"), L"MCLauncher.exe");
        if (!isDirectory(dirNameOf(want))) want = joinPath(exeDir(), L"MCLauncher.exe");
    }

    // 主程序还在 (用户自己启动的情况): 直接用它
    if (pathExists(want) && _wcsicmp(want.c_str(), guardExe.c_str()) != 0) {
        target = want;
        return true;
    }
    // 被删了: 从备份还原
    if (copyFileOverwrite(guardExe, want)) {
        target = want;
        return true;
    }
    target = guardExe;   // 还原失败就先用备份本体运行
    return pathExists(guardExe);
}

void selfGuardRemove() {
    HKEY k = nullptr;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, kGuardRunKey, 0, KEY_SET_VALUE, &k) == ERROR_SUCCESS) {
        RegDeleteValueW(k, kGuardRunVal);
        RegCloseKey(k);
    }
    std::wstring dir = selfGuardDir();
    WIN32_FIND_DATAW fd{};
    HANDLE h = FindFirstFileW(joinPath(dir, L"*").c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            std::wstring p = joinPath(dir, fd.cFileName);
            SetFileAttributesW(p.c_str(), FILE_ATTRIBUTE_NORMAL);
            DeleteFileW(p.c_str());
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    RemoveDirectoryW(dir.c_str());
    RemoveDirectoryW(dirNameOf(dir).c_str());
}
