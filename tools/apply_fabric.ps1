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

Rep '    BTN_ISOLATION          = 901014,' @'
    BTN_ISOLATION          = 901014,
    BTN_INSTALL_FABRIC     = 901015,
'@

Rep '    void exportDiagnostics();            // 导出诊断信息' @'
    void exportDiagnostics();            // 导出诊断信息
    void installFabricAsync();           // 为当前版本安装 Fabric
'@

Rep @'
    RECT bc2 = mkRect(tb.right - 104 - 130, y + 12, tb.right - 112 - 12, y + 42);
    drawButton(g, bc2, showInstalledOnly_ ? L"显示全部版本" : L"仅看已安装",
               BTN_TOGGLE_INSTALLED, false, true, 12);
'@ @'
    RECT bc2 = mkRect(tb.right - 104 - 130, y + 12, tb.right - 112 - 12, y + 42);
    drawButton(g, bc2, showInstalledOnly_ ? L"显示全部版本" : L"仅看已安装",
               BTN_TOGGLE_INSTALLED, false, true, 12);
    RECT bfab = mkRect(tb.right - 400, y + 12, tb.right - 260, y + 42);
    drawButton(g, bfab, L"安装 Fabric", BTN_INSTALL_FABRIC, false, !busy_.load(), 12);
'@

Rep '    if (id == BTN_GOPT_SAVE) { saveGameOptions(); return; }' @'
    if (id == BTN_GOPT_SAVE) { saveGameOptions(); return; }
    if (id == BTN_INSTALL_FABRIC) { installFabricAsync(); return; }
'@

Rep @'
            if (okFlag) {
                manifest_ = out;
                filtered_.clear();
                for (int i = 0; i < (int)manifest_.size(); ++i) filtered_.push_back(i);
'@ @'
            if (okFlag) {
                manifest_ = out;
                for (auto& s : local) {
                    bool found = false;
                    for (auto& v : manifest_) { if (v.id == s) { found = true; break; } }
                    if (!found) { VersionInfo nv; nv.id = s; nv.type = "installed"; manifest_.push_back(nv); }
                }
                filtered_.clear();
                for (int i = 0; i < (int)manifest_.size(); ++i) filtered_.push_back(i);
'@

Rep '            else if (v.type == "old_alpha")     r.type = L"远古Alpha";' @'
            else if (v.type == "old_alpha")     r.type = L"远古Alpha";
            else if (v.type == "installed")     r.type = L"已安装";
'@

$fn = @'
void MainWindow::installFabricAsync() {
    if (busy_.load()) { pushLog(L"当前有任务在执行，请稍候。"); return; }
    std::string mc = wideToUtf8(selVersion_);
    if (mc.empty()) {
        MessageBoxW(hwnd_, L"请先在首页选择一个游戏版本，再安装 Fabric。", L"安装 Fabric", MB_ICONINFORMATION);
        return;
    }
    if (mc.rfind("fabric-loader-", 0) == 0) {
        MessageBoxW(hwnd_, L"当前选中的已经是 Fabric 版本。", L"安装 Fabric", MB_ICONINFORMATION);
        return;
    }
    std::wstring mcW = utf8ToWide(mc);
    std::wstring baseDir = joinPath(joinPath(gameDir_, L"versions"), mcW);
    if (!pathExists(joinPath(baseDir, mcW + L".jar"))) {
        MessageBoxW(hwnd_, L"请先安装并完成这个原版版本，再安装 Fabric。", L"安装 Fabric", MB_ICONWARNING);
        return;
    }
    std::wstring java = javaPath_.empty() ? Launcher::autoDetectJava() : javaPath_;
    if (java.empty()) {
        MessageBoxW(hwnd_, L"没有可用的 Java，请先在游戏设置里选择。", L"安装 Fabric", MB_ICONWARNING);
        return;
    }
    busy_ = true;
    pushLog(L"开始安装 Fabric: MC " + mcW);
    setStatus(0, L"正在获取 Fabric 版本信息…");
    dirty_ = true;

    std::thread([this, mc, mcW, java]() {
        std::wstring err;
        std::string body;
        std::wstring metaUrl = L"https://meta.fabricmc.net/v2/versions/loader/" + mcW;
        std::wstring fail;
        std::wstring loaderW;
        if (!httpGetToStringEx(metaUrl, kBrowserUA, body, err)) {
            fail = L"获取 Fabric loader 列表失败：" + err;
        } else {
            Json j;
            if (!mjson::parse(body, j) || !j.isArray() || j.size() == 0) {
                fail = L"Fabric loader 列表解析失败";
            } else {
                loaderW = utf8ToWide(getStr(j.at(0).get("loader"), "version"));
                if (loaderW.empty()) fail = L"Fabric loader 版本为空";
            }
        }
        std::wstring installerW;
        if (fail.empty()) {
            setStatus(20, L"正在获取 Fabric 安装器版本…");
            std::string xml;
            std::wstring merr;
            if (!httpGetToStringEx(L"https://maven.fabricmc.net/net/fabricmc/fabric-installer/maven-metadata.xml",
                                   kBrowserUA, xml, merr)) {
                fail = L"获取 Fabric 安装器版本失败：" + merr;
            } else {
                size_t a = xml.find("<release>");
                size_t b = xml.find("</release>", a == std::string::npos ? 0 : a);
                if (a == std::string::npos || b == std::string::npos) fail = L"Fabric 安装器版本解析失败";
                else installerW = utf8ToWide(xml.substr(a + 9, b - a - 9));
            }
        }
        std::wstring name = L"fabric-loader-" + loaderW + L"-" + mcW;
        std::wstring jar;
        if (fail.empty()) {
            wchar_t tmp[MAX_PATH] = { 0 };
            GetTempPathW(MAX_PATH, tmp);
            jar = joinPath(tmp, L"fabric-installer-" + installerW + L".jar");
            std::wstring instUrl = L"https://maven.fabricmc.net/net/fabricmc/fabric-installer/" + installerW +
                                   L"/fabric-installer-" + installerW + L".jar";
            setStatus(35, L"正在下载 Fabric 安装器…");
            if (!httpDownloadToFileEx(instUrl, jar, kBrowserUA, [](unsigned long long, unsigned long long) { return true; }, err)) {
                fail = L"下载 Fabric 安装器失败：" + err;
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
            setStatus(45, L"正在运行 Fabric 安装器…");
            std::wstring cmd = L"\"" + java + L"\" -jar \"" + jar + L"\" client -dir \"" + gameDir_ +
                               L"\" -mcversion \"" + mcW + L"\" -loader \"" + loaderW + L"\" -n \"" + name + L"\"";
            std::vector<wchar_t> cbuf(cmd.begin(), cmd.end());
            cbuf.push_back(0);
            STARTUPINFOW si{};
            si.cb = sizeof(si);
            PROCESS_INFORMATION pi{};
            if (!CreateProcessW(nullptr, cbuf.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                                nullptr, nullptr, &si, &pi)) {
                fail = L"无法启动 Fabric 安装器";
            } else {
                DWORD wait = WaitForSingleObject(pi.hProcess, 300000);
                DWORD code = 1;
                GetExitCodeProcess(pi.hProcess, &code);
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
                if (wait == WAIT_TIMEOUT) fail = L"Fabric 安装器超时";
                else if (code != 0) fail = L"Fabric 安装器退出码 " + std::to_wstring(code);
            }
        }
        if (fail.empty()) {
            std::lock_guard<std::mutex> lk(mtx_);
            std::string nid = wideToUtf8(name);
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
            installedOnly_.push_back(nid);
            pushLog(L"Fabric 安装完成: " + name + L"，请在版本列表选择并启动");
            busy_ = false;
            dirty_ = true;
            if (hwnd_) PostMessageW(hwnd_, WM_TIMER, 0, 0);
            return;
        }
        pushLog(L"Fabric 安装失败: " + fail);
        busy_ = false;
        dirty_ = true;
        if (hwnd_) PostMessageW(hwnd_, WM_TIMER, 0, 0);
        MessageBoxW(hwnd_, fail.c_str(), L"安装 Fabric 失败", MB_ICONWARNING);
    }).detach();
}

'@
Rep 'void MainWindow::launchSelectedAsync() {' ($fn + 'void MainWindow::launchSelectedAsync() {')

Rep 'static const wchar_t* kAppVersion = L"1.17.1";' 'static const wchar_t* kAppVersion = L"1.18.0";'

[IO.File]::WriteAllText($p, $u, (New-Object Text.UTF8Encoding($false)))
"fabric install patched, length=$($u.Length)"
