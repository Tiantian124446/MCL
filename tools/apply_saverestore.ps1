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
    BTN_SAVE_RESTORE       = 901013,
'@

Rep '    void deleteSave(int idx);            // 删除存档' @'
    void deleteSave(int idx);            // 删除存档
    void restoreLatestBackup();          // 从最近备份恢复
'@

Rep @'
    RECT b1 = mkRect(info.right - 300, info.top + 16, info.right - 168, info.top + 46);
    drawButton(g, b1, L"刷新", BTN_MODMGR_REFRESH, false, true, 12);
    RECT b2 = mkRect(info.right - 156, info.top + 16, info.right - 16, info.top + 46);
    drawButton(g, b2, L"打开 backups", BTN_MODMGR_OPEN, false, true, 12);
'@ @'
    RECT b1 = mkRect(info.right - 420, info.top + 16, info.right - 330, info.top + 46);
    drawButton(g, b1, L"刷新", BTN_MODMGR_REFRESH, false, true, 12);
    RECT b3 = mkRect(info.right - 322, info.top + 16, info.right - 172, info.top + 46);
    drawButton(g, b3, L"恢复最近备份", BTN_SAVE_RESTORE, false, true, 12);
    RECT b2 = mkRect(info.right - 164, info.top + 16, info.right - 16, info.top + 46);
    drawButton(g, b2, L"打开 backups", BTN_MODMGR_OPEN, false, true, 12);
'@

Rep '    if (id == BTN_ACCOUNT_ADD) { addAccount(); return; }' @'
    if (id == BTN_SAVE_RESTORE) { restoreLatestBackup(); return; }
    if (id == BTN_ACCOUNT_ADD) { addAccount(); return; }
'@

$fn = @'
static bool copyTreeOverwrite(const std::wstring& src, const std::wstring& dst) {
    CreateDirectoryW(dst.c_str(), nullptr);
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((src + L"\\*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return false;
    bool ok = true;
    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
        std::wstring s = src + L"\\" + fd.cFileName;
        std::wstring d = dst + L"\\" + fd.cFileName;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!copyTreeOverwrite(s, d)) ok = false;
        } else {
            if (!CopyFileW(s.c_str(), d.c_str(), FALSE)) ok = false;
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    return ok;
}

void MainWindow::restoreLatestBackup() {
    std::wstring backups = joinPath(gameDir_, L"backups");
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((backups + L"\\saves_*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) {
        MessageBoxW(hwnd_, L"还没有存档备份。", L"MCL 存档管理", MB_ICONINFORMATION);
        return;
    }
    std::wstring newest;
    FILETIME best{};
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
        if (CompareFileTime(&fd.ftLastWriteTime, &best) > 0) {
            best = fd.ftLastWriteTime;
            newest = fd.cFileName;
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    if (newest.empty()) {
        MessageBoxW(hwnd_, L"还没有存档备份。", L"MCL 存档管理", MB_ICONINFORMATION);
        return;
    }
    std::wstring base = newest.substr(6);
    if (base.size() > 16 && base[base.size() - 16] == L'_') base = base.substr(0, base.size() - 16);
    if (base.empty()) {
        MessageBoxW(hwnd_, L"备份目录名无法解析。", L"MCL 存档管理", MB_ICONWARNING);
        return;
    }
    std::wstring backupPath = joinPath(backups, newest);
    std::wstring savesDir = joinPath(gameDir_, L"saves");
    std::wstring dest = joinPath(savesDir, base);
    std::wstring msg = L"将用备份恢复存档：\n" + base + L"\n\n当前存档会先改名为 " + base + L".before_restore_<时间>。\n继续吗？";
    if (MessageBoxW(hwnd_, msg.c_str(), L"恢复存档", MB_ICONWARNING | MB_YESNO) != IDYES) return;

    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t stamp[64];
    swprintf(stamp, 64, L"%04d%02d%02d_%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    bool had = pathExists(dest);
    std::wstring old = dest + L".before_restore_" + std::wstring(stamp);
    if (had && !MoveFileW(dest.c_str(), old.c_str())) {
        MessageBoxW(hwnd_, L"当前存档正在被占用，无法重命名。", L"MCL 存档管理", MB_ICONWARNING);
        return;
    }
    createDirs(dest);
    if (!copyTreeOverwrite(backupPath, dest)) {
        if (had) MoveFileW(old.c_str(), dest.c_str());
        MessageBoxW(hwnd_, L"恢复失败，已尝试还原原存档。", L"MCL 存档管理", MB_ICONWARNING);
        return;
    }
    loadSaves();
    pushLog(L"已从备份恢复存档: " + base + L" <- " + backupPath);
    MessageBoxW(hwnd_, (L"恢复完成：\n" + base).c_str(), L"MCL 存档管理", MB_ICONINFORMATION);
}

'@
Rep 'void MainWindow::drawSaves(Graphics& g, int W, int H) {' ($fn + 'void MainWindow::drawSaves(Graphics& g, int W, int H) {')

Rep 'static const wchar_t* kAppVersion = L"1.14.0";' 'static const wchar_t* kAppVersion = L"1.15.0";'

[IO.File]::WriteAllText($p, $u, (New-Object Text.UTF8Encoding($false)))
"save restore patched, length=$($u.Length)"
