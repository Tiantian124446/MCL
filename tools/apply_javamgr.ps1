$ErrorActionPreference = 'Stop'
$p = 'E:\MCL\src\ui.cpp'
$u = ([IO.File]::ReadAllText($p, [Text.Encoding]::UTF8)) -replace "`r`n", "`n"
function Norm([string]$s) { return ($s -replace "`r`n", "`n") }
function Rep([string]$old, [string]$new) {
    $old = Norm $old
    $new = Norm $new
    $count = ([regex]::Matches($script:u, [regex]::Escape($old))).Count
    if ($count -ne 1) { throw "count $count for: $($old.Substring(0, [Math]::Min(70, $old.Length)))" }
    $script:u = $script:u.Replace($old, $new)
}

Rep '    BTN_MODMGR_DISABLE_ALL = 901006,' @'
    BTN_MODMGR_DISABLE_ALL = 901006,
    BTN_JAVA_SCAN          = 901007,
    BTN_JAVA_USE_BASE      = 902000,   // +列表下标: 使用该 Java
'@

Rep '    std::wstring modMgrFilter_;      // 模组列表筛选词' @'
    std::wstring modMgrFilter_;      // 模组列表筛选词
    struct JavaEntry { std::wstring path; int major; };
    std::vector<JavaEntry> javaFound_;
    bool javaScanning_ = false;
'@

Rep '    void setAllMods(bool enable);        // 全部启用 / 全部禁用' @'
    void setAllMods(bool enable);        // 全部启用 / 全部禁用
    void scanJavaVersions();             // 扫描常见 Java 安装
    void useJavaVersion(int idx);        // 选用扫描到的 Java
'@

Rep '    if (id == BTN_MODMGR_ENABLE_ALL) { setAllMods(true); return; }' @'
    if (id == BTN_JAVA_SCAN) { scanJavaVersions(); return; }
    if (id >= BTN_JAVA_USE_BASE && id < BTN_JAVA_USE_BASE + 100) {
        useJavaVersion(id - BTN_JAVA_USE_BASE);
        return;
    }
    if (id == BTN_MODMGR_ENABLE_ALL) { setAllMods(true); return; }
'@

Rep @'
    RECT b2a = mkRect(c2.right - 108, c2.top + 17, c2.right - 16, c2.bottom - 17);
    drawButton(g, b2a, L"浏览", BTN_BROWSE_JAVA, false, true, 12);
    RECT b2b = mkRect(c2.right - 240, c2.top + 17, c2.right - 118, c2.bottom - 17);
    drawButton(g, b2b, L"自动检测", BTN_AUTO_JAVA, false, true, 12);
'@ @'
    RECT b2a = mkRect(c2.right - 108, c2.top + 17, c2.right - 16, c2.bottom - 17);
    drawButton(g, b2a, L"浏览", BTN_BROWSE_JAVA, false, true, 12);
    RECT b2b = mkRect(c2.right - 200, c2.top + 17, c2.right - 118, c2.bottom - 17);
    drawButton(g, b2b, L"扫描", BTN_JAVA_SCAN, false, !javaScanning_, 12);
    RECT b2c = mkRect(c2.right - 300, c2.top + 17, c2.right - 210, c2.bottom - 17);
    drawButton(g, b2c, L"自动检测", BTN_AUTO_JAVA, false, true, 12);
'@

Rep @'
    RECT hr = mkRect(cx + 170, by, cx + cw, by + 44);
    drawText(g, L"提示：修改后需点击「保存设置」生效。", fV, kTextDim, hr, 0);
}
'@ @'
    RECT hr = mkRect(cx + 170, by, cx + cw, by + 44);
    drawText(g, L"提示：修改后需点击「保存设置」生效。", fV, kTextDim, hr, 0);

    int jy = by + 44 + 12;
    RECT c5 = mkRect(cx, jy, cx + cw, H - 16);
    drawCard(g, c5);
    RECT jt = mkRect(c5.left + 20, c5.top + 8, c5.right - 130, c5.top + 30);
    drawText(g, javaScanning_ ? L"已检测到的 Java · 正在扫描…" : L"已检测到的 Java", fL, kTextDim, jt, 0);
    RECT jb = mkRect(c5.right - 116, c5.top + 8, c5.right - 16, c5.top + 36);
    drawButton(g, jb, L"扫描 Java", BTN_JAVA_SCAN, false, !javaScanning_, 12);

    std::vector<JavaEntry> jv;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        jv = javaFound_;
    }
    if (jv.empty()) {
        RECT je = mkRect(c5.left + 20, c5.top + 44, c5.right - 20, c5.top + 76);
        drawText(g, javaScanning_ ? L"正在查找常见 Java 安装位置…" : L"点「扫描 Java」查找本机已安装的 Java；也可以手动填写路径。",
                 fV, kTextDim, je, 0);
    } else {
        int rowH = 34;
        int maxRows = (c5.bottom - c5.top - 44) / rowH;
        if (maxRows < 1) maxRows = 1;
        int show = (int)jv.size();
        if (show > maxRows) show = maxRows;
        int ry = c5.top + 42;
        for (int i = 0; i < show; ++i) {
            const JavaEntry& e = jv[i];
            RECT rp = mkRect(c5.left + 20, ry, c5.right - 220, ry + rowH);
            drawText(g, e.path, fV, kText, rp, 0);
            RECT rm = mkRect(c5.right - 210, ry, c5.right - 90, ry + rowH);
            drawText(g, L"Java " + std::to_wstring(e.major), fV, kAccent, rm, 0);
            RECT rb = mkRect(c5.right - 80, ry + 3, c5.right - 16, ry + 29);
            drawButton(g, rb, L"使用", BTN_JAVA_USE_BASE + i, false, true, 11);
            ry += rowH;
        }
    }
}
'@

Rep '        SetWindowPos(editJava_, nullptr, S(cx + 100), S(c2 + 18), S(cw - 260), S(28), SWP_NOZORDER);' '        SetWindowPos(editJava_, nullptr, S(cx + 100), S(c2 + 18), S(cw - 430), S(28), SWP_NOZORDER);'

$fn = @'
void MainWindow::scanJavaVersions() {
    if (javaScanning_) return;
    javaScanning_ = true;
    dirty_ = true;
    std::thread([this]() {
        std::vector<JavaEntry> found;
        auto add = [&](const std::wstring& p) {
            if (p.empty()) return;
            for (auto& e : found) if (_wcsicmp(e.path.c_str(), p.c_str()) == 0) return;
            int major = Launcher::queryJavaMajor(p);
            if (major > 0) found.push_back({ p, major });
        };
        auto scanBase = [&](const std::wstring& base) {
            WIN32_FIND_DATAW fd;
            HANDLE h = FindFirstFileW((base + L"\\*").c_str(), &fd);
            if (h == INVALID_HANDLE_VALUE) return;
            do {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
                if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
                std::wstring p = base + L"\\" + fd.cFileName + L"\\bin\\java.exe";
                if (GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES) add(p);
            } while (FindNextFileW(h, &fd));
            FindClose(h);
        };
        if (!javaPath_.empty()) add(javaPath_);
        scanBase(L"C:\\Program Files\\Eclipse Adoptium");
        scanBase(L"C:\\Program Files\\Java");
        scanBase(L"C:\\Program Files\\Microsoft");
        scanBase(L"C:\\Program Files\\Zulu");
        scanBase(L"C:\\Program Files\\BellSoft");
        scanBase(L"C:\\Program Files (x86)\\Java");
        scanBase(L"C:\\Program Files (x86)\\Eclipse Adoptium");
        wchar_t local[MAX_PATH] = { 0 };
        if (GetEnvironmentVariableW(L"LOCALAPPDATA", local, MAX_PATH) > 0) {
            scanBase(std::wstring(local) + L"\\Programs\\Eclipse Adoptium");
        }
        scanBase(L"D:\\jdk");
        scanBase(L"D:\\java");
        scanBase(L"E:\\jdk");
        std::sort(found.begin(), found.end(), [](const JavaEntry& a, const JavaEntry& b) {
            return a.major > b.major;
        });
        if (found.size() > 8) found.resize(8);
        {
            std::lock_guard<std::mutex> lk(mtx_);
            javaFound_.swap(found);
            javaScanning_ = false;
        }
        dirty_ = true;
        if (hwnd_) PostMessageW(hwnd_, WM_TIMER, 0, 0);
    }).detach();
}

void MainWindow::useJavaVersion(int idx) {
    std::vector<JavaEntry> jv;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        jv = javaFound_;
    }
    if (idx < 0 || idx >= (int)jv.size()) return;
    javaPath_ = jv[idx].path;
    if (editJava_) SetWindowTextW(editJava_, javaPath_.c_str());
    saveSettings();
    applySettingsToLauncher();
    pushLog(L"已切换 Java: " + javaPath_ + L" (Java " + std::to_wstring(jv[idx].major) + L")");
    dirty_ = true;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

'@
Rep 'void MainWindow::drawSettings(Graphics& g, int W, int H) {' ($fn + 'void MainWindow::drawSettings(Graphics& g, int W, int H) {')

Rep 'static const wchar_t* kAppVersion = L"1.9.0";' 'static const wchar_t* kAppVersion = L"1.10.0";'

[IO.File]::WriteAllText($p, $u, (New-Object Text.UTF8Encoding($false)))
"java manager patched, length=$($u.Length)"
