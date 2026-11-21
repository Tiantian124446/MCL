$ErrorActionPreference = 'Stop'
$p = 'E:\MCL\src\ui.cpp'
$u = ([IO.File]::ReadAllText($p, [Text.Encoding]::UTF8)) -replace "`r`n", "`n"
function Rep([string]$old, [string]$new) {
    $old = ($old -replace "`r`n", "`n")
    $new = ($new -replace "`r`n", "`n")
    $count = ([regex]::Matches($script:u, [regex]::Escape($old))).Count
    if ($count -ne 1) { throw "count $count for: $($old.Substring(0, [Math]::Min(70, $old.Length)))" }
    $script:u = $script:u.Replace($old, $new)
}

Rep '    BTN_INSTALL_FABRIC     = 901015,' @'
    BTN_INSTALL_FABRIC     = 901015,
    BTN_INSTALL_FORGE      = 901016,
'@

Rep '    void installFabricAsync();           // 为当前版本安装 Fabric' @'
    void installFabricAsync();           // 为当前版本安装 Fabric
    void installForgeAsync();            // 为当前版本安装 Forge
'@

Rep @'
    RECT bfab = mkRect(tb.right - 400, y + 12, tb.right - 260, y + 42);
    drawButton(g, bfab, L"安装 Fabric", BTN_INSTALL_FABRIC, false, !busy_.load(), 12);
'@ @'
    RECT bfab = mkRect(tb.right - 400, y + 12, tb.right - 260, y + 42);
    drawButton(g, bfab, L"安装 Fabric", BTN_INSTALL_FABRIC, false, !busy_.load(), 12);
    RECT bforge = mkRect(tb.right - 548, y + 12, tb.right - 410, y + 42);
    drawButton(g, bforge, L"安装 Forge", BTN_INSTALL_FORGE, false, !busy_.load(), 12);
'@

Rep '    if (id == BTN_INSTALL_FABRIC) { installFabricAsync(); return; }' @'
    if (id == BTN_INSTALL_FABRIC) { installFabricAsync(); return; }
    if (id == BTN_INSTALL_FORGE) { installForgeAsync(); return; }
'@

$fn = @'
void MainWindow::installForgeAsync() {
    if (busy_.load()) { pushLog(L"当前有任务在执行，请稍候。"); return; }
    std::string mc = wideToUtf8(selVersion_);
    if (mc.empty()) {
        MessageBoxW(hwnd_, L"请先在首页选择一个游戏版本，再安装 Forge。", L"安装 Forge", MB_ICONINFORMATION);
        return;
    }
    if (mc.find("-forge-") != std::string::npos || mc.rfind("fabric-loader-", 0) == 0) {
        MessageBoxW(hwnd_, L"当前选中的已经是模组加载器版本，请先选择原版版本。", L"安装 Forge", MB_ICONINFORMATION);
        return;
    }
    std::wstring mcW = utf8ToWide(mc);
    std::wstring baseDir = joinPath(joinPath(gameDir_, L"versions"), mcW);
    if (!pathExists(joinPath(baseDir, mcW + L".jar"))) {
        MessageBoxW(hwnd_, L"请先安装并完成这个原版版本，再安装 Forge。", L"安装 Forge", MB_ICONWARNING);
        return;
    }
    std::wstring java = javaPath_.empty() ? Launcher::autoDetectJava() : javaPath_;
    if (java.empty()) {
        MessageBoxW(hwnd_, L"没有可用的 Java，请先在游戏设置里选择。", L"安装 Forge", MB_ICONWARNING);
        return;
    }
    busy_ = true;
    pushLog(L"开始安装 Forge: MC " + mcW);
    setStatus(0, L"正在获取 Forge 版本…");
    dirty_ = true;

    std::thread([this, mc, mcW, java]() {
        std::wstring err, fail;
        std::string body;
        std::wstring forgeW;
        if (!httpGetToStringEx(L"https://files.minecraftforge.net/net/minecraftforge/forge/promotions_slim.json",
                               kBrowserUA, body, err)) {
            fail = L"获取 Forge 版本列表失败：" + err;
        } else {
            Json j;
            if (!mjson::parse(body, j) || !j.isObject()) {
                fail = L"Forge promotions 解析失败";
            } else {
                const Json& promos = j.get("promos");
                std::string key = mc + "-recommended";
                forgeW = utf8ToWide(getStr(promos, key.c_str()));
                if (forgeW.empty()) {
                    key = mc + "-latest";
                    forgeW = utf8ToWide(getStr(promos, key.c_str()));
                }
                if (forgeW.empty()) fail = L"没有找到该 MC 版本的 Forge 推荐版本";
            }
        }
        std::wstring jar;
        if (fail.empty()) {
            wchar_t tmp[MAX_PATH] = { 0 };
            GetTempPathW(MAX_PATH, tmp);
            std::wstring name = mcW + L"-forge-" + forgeW;
            std::wstring file = L"forge-" + mcW + L"-" + forgeW + L"-installer.jar";
            jar = joinPath(tmp, file);
            std::wstring url = L"https://maven.minecraftforge.net/net/minecraftforge/forge/" +
                               mcW + L"-" + forgeW + L"/forge-" + mcW + L"-" + forgeW + L"-installer.jar";
            setStatus(20, L"正在下载 Forge 安装器…");
            if (!httpDownloadToFileEx(url, jar, kBrowserUA, [](unsigned long long, unsigned long long) { return true; }, err)) {
                fail = L"下载 Forge 安装器失败：" + err;
            }
        }
        if (fail.empty()) {
            std::wstring lp = joinPath(gameDir_, L"launcher_profiles.json");
            if (!pathExists(lp)) {
                FILE* fp = _wfopen(lp.c_str(), L"wb");
                if (fp) {
                    const char* js = "{\"profiles\":{},\"settings\":{\"language\":\"zh_cn\"},\"version\":3}";
                    fwrite(js, 1, strlen(js), fp);
                    fclose(fp);
                }
            }
            setStatus(40, L"正在运行 Forge 安装器（可能需要 1-2 分钟）…");
            std::wstring cmd = L"\"" + java + L"\" -jar \"" + jar + L"\" --installClient \"" + gameDir_ + L"\"";
            std::vector<wchar_t> cbuf(cmd.begin(), cmd.end());
            cbuf.push_back(0);
            STARTUPINFOW si{};
            si.cb = sizeof(si);
            PROCESS_INFORMATION pi{};
            if (!CreateProcessW(nullptr, cbuf.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                                nullptr, nullptr, &si, &pi)) {
                fail = L"无法启动 Forge 安装器";
            } else {
                DWORD wait = WaitForSingleObject(pi.hProcess, 600000);
                DWORD code = 1;
                GetExitCodeProcess(pi.hProcess, &code);
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
                if (wait == WAIT_TIMEOUT) fail = L"Forge 安装器超时";
                else if (code != 0) fail = L"Forge 安装器退出码 " + std::to_wstring(code);
            }
        }
        if (fail.empty()) {
            std::wstring name = mcW + L"-forge-" + forgeW;
            std::string nid = wideToUtf8(name);
            {
                std::lock_guard<std::mutex> lk(mtx_);
                bool found = false;
                for (auto& v : manifest_) { if (v.id == nid) { found = true; break; } }
                if (!found) {
                    VersionInfo nv;
                    nv.id = nid;
                    nv.type = "installed";
                    manifest_.push_back(nv);
                    filtered_.push_back((int)manifest_.size() - 1);
                }
                installedSet_.insert(nid);
                bool inLocal = false;
                for (auto& s : installedOnly_) { if (s == nid) { inLocal = true; break; } }
                if (!inLocal) installedOnly_.push_back(nid);
            }
            pushLog(L"Forge 安装完成: " + name + L"，请在版本列表选择并启动");
            setStatus(-1, L"Forge 安装完成");
            busy_ = false;
            dirty_ = true;
            if (hwnd_) PostMessageW(hwnd_, WM_TIMER, 0, 0);
            return;
        }
        pushLog(L"Forge 安装失败: " + fail);
        setStatus(-1, L"Forge 安装失败");
        busy_ = false;
        dirty_ = true;
        if (hwnd_) PostMessageW(hwnd_, WM_TIMER, 0, 0);
        MessageBoxW(hwnd_, fail.c_str(), L"安装 Forge 失败", MB_ICONWARNING);
    }).detach();
}

'@
Rep 'void MainWindow::installFabricAsync() {' ($fn + 'void MainWindow::installFabricAsync() {')

Rep 'static const wchar_t* kAppVersion = L"1.18.0";' 'static const wchar_t* kAppVersion = L"1.19.0";'

[IO.File]::WriteAllText($p, $u, (New-Object Text.UTF8Encoding($false)))
"forge install patched, length=$($u.Length)"
