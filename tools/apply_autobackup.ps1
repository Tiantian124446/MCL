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

Rep '    BTN_ACCOUNT_ADD        = 901008,' @'
    BTN_ACCOUNT_ADD        = 901008,
    BTN_AUTO_BACKUP        = 901009,
'@

Rep '    bool         useMirror_ = true;' @'
    bool         useMirror_ = true;
    bool autoBackup_ = true;
    bool autoBackupChecked_ = false;
'@

Rep '    void syncAccountList();              // 确保当前昵称在账户列表' @'
    void syncAccountList();              // 确保当前昵称在账户列表
    void maybeAutoBackup();              // 启动时自动备份存档(每天一次)
'@

Rep '            else if (k == "mirror")   useMirror_ = (v == "1");' @'
            else if (k == "mirror")   useMirror_ = (v == "1");
            else if (k == "autobackup") autoBackup_ = (v == "1");
'@

Rep '    out += "mirror=" + std::string(useMirror_ ? "1" : "0") + "\r\n";' @'
    out += "mirror=" + std::string(useMirror_ ? "1" : "0") + "\r\n";
    out += "autobackup=" + std::string(autoBackup_ ? "1" : "0") + "\r\n";
'@

Rep '    case BTN_FULLSCREEN:' @'
    case BTN_AUTO_BACKUP:
        autoBackup_ = !autoBackup_;
        saveSettings();
        pushLog(autoBackup_ ? L"已开启启动时自动备份存档" : L"已关闭启动时自动备份存档");
        dirty_ = true;
        break;

    case BTN_FULLSCREEN:
'@

Rep @'
    radio(mkRect(c4.left + 320, c4.top + 4, c4.left + 520, c4.bottom), L"Mojang 官方源", !useMirror_, BTN_MIRROR_OFFICIAL);
'@ @'
    radio(mkRect(c4.left + 320, c4.top + 4, c4.left + 520, c4.bottom), L"Mojang 官方源", !useMirror_, BTN_MIRROR_OFFICIAL);
    {
        bool hover = (hoverId_ == BTN_AUTO_BACKUP);
        RECT sq = mkRect(c4.left + 540, c4.top + 14, c4.left + 558, c4.top + 32);
        fillRound(g, sq, 4, autoBackup_ ? kAccent : kWhite);
        strokeRound(g, sq, 4, autoBackup_ ? kAccent : (hover ? C(160, 165, 172) : C(190, 194, 200)));
        if (autoBackup_) {
            Pen p(kWhite, 2.0f);
            g.DrawLine(&p, sq.left + 4, sq.top + 10, sq.left + 8, sq.top + 14);
            g.DrawLine(&p, sq.left + 8, sq.top + 14, sq.left + 14, sq.top + 6);
        }
        RECT tr = mkRect(sq.right + 8, c4.top, c4.left + 750, c4.bottom);
        drawText(g, L"自动备份存档", fV, kText, tr, 0);
        hits_.push_back({ mkRect(c4.left + 540, c4.top + 4, c4.left + 750, c4.bottom), BTN_AUTO_BACKUP });
    }
'@

Rep @'
void MainWindow::onTimer() {
    if (dirty_.exchange(false)) {
'@ @'
void MainWindow::onTimer() {
    if (!autoBackupChecked_) {
        autoBackupChecked_ = true;
        maybeAutoBackup();
    }
    if (dirty_.exchange(false)) {
'@

$fn = @'
void MainWindow::maybeAutoBackup() {
    if (!autoBackup_) return;
    std::wstring saves = joinPath(gameDir_, L"saves");
    if (!pathExists(saves)) return;
    std::wstring backups = joinPath(exeDir(), L"backups");
    unsigned long long newest = 0;
    {
        WIN32_FIND_DATAW fd;
        HANDLE h = FindFirstFileW((backups + L"\\auto_*").c_str(), &fd);
        if (h != INVALID_HANDLE_VALUE) {
            do {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
                ULARGE_INTEGER t;
                t.LowPart = fd.ftLastWriteTime.dwLowDateTime;
                t.HighPart = fd.ftLastWriteTime.dwHighDateTime;
                if (t.QuadPart > newest) newest = t.QuadPart;
            } while (FindNextFileW(h, &fd));
            FindClose(h);
        }
    }
    if (newest > 0) {
        FILETIME now{};
        GetSystemTimeAsFileTime(&now);
        ULARGE_INTEGER n;
        n.LowPart = now.dwLowDateTime;
        n.HighPart = now.dwHighDateTime;
        if (n.QuadPart > newest && n.QuadPart - newest < 24ULL * 60 * 60 * 10000000ULL) return;
    }
    std::thread([this, backups, saves]() {
        createDirs(backups);
        SYSTEMTIME st{};
        GetLocalTime(&st);
        wchar_t ts[32];
        swprintf(ts, 32, L"auto_%04d%02d%02d_%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        std::wstring dest = joinPath(backups, ts);
        std::wstring from = saves;
        from.push_back(L'\0');
        from.push_back(L'\0');
        std::wstring to = dest;
        to.push_back(L'\0');
        to.push_back(L'\0');
        SHFILEOPSTRUCTW op{};
        op.wFunc = FO_COPY;
        op.pFrom = from.c_str();
        op.pTo = to.c_str();
        op.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;
        int rc = SHFileOperationW(&op);
        if (rc == 0) pushLog(L"自动备份存档完成: " + dest);
        else pushLog(L"自动备份存档失败，代码 " + std::to_wstring(rc));
        dirty_ = true;
        if (hwnd_) PostMessageW(hwnd_, WM_TIMER, 0, 0);
    }).detach();
}

'@
Rep 'void MainWindow::drawSettings(Graphics& g, int W, int H) {' ($fn + 'void MainWindow::drawSettings(Graphics& g, int W, int H) {')

Rep 'static const wchar_t* kAppVersion = L"1.12.0";' 'static const wchar_t* kAppVersion = L"1.13.0";'

[IO.File]::WriteAllText($p, $u, (New-Object Text.UTF8Encoding($false)))
"auto backup patched, length=$($u.Length)"
