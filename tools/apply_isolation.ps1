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

Rep '    BTN_SAVE_RESTORE       = 901013,' @'
    BTN_SAVE_RESTORE       = 901013,
    BTN_ISOLATION          = 901014,
'@

Rep '    bool autoBackup_ = true;' @'
    bool autoBackup_ = true;
    bool isolation_ = false;
'@

Rep '    std::wstring modsDir() const;' @'
    std::wstring modsDir() const;
    std::wstring activeGameDir() const;   // 版本隔离后的实际数据目录
'@

Rep '            else if (k == "autobackup") autoBackup_ = (v == "1");' @'
            else if (k == "autobackup") autoBackup_ = (v == "1");
            else if (k == "isolation") isolation_ = (v == "1");
'@

Rep '    out += "autobackup=" + std::string(autoBackup_ ? "1" : "0") + "\r\n";' @'
    out += "autobackup=" + std::string(autoBackup_ ? "1" : "0") + "\r\n";
    out += "isolation=" + std::string(isolation_ ? "1" : "0") + "\r\n";
'@

Rep '    launcher_.opt.gameDir = gameDir_;' @'
    launcher_.opt.gameDir = activeGameDir();
    if (isolation_) {
        std::wstring gd = activeGameDir();
        createDirs(gd);
        createDirs(joinPath(gd, L"mods"));
        createDirs(joinPath(gd, L"saves"));
        createDirs(joinPath(gd, L"resourcepacks"));
        createDirs(joinPath(gd, L"shaderpacks"));
    }
'@

Rep @'
void MainWindow::saveSettings() {
    syncAccountList();
'@ @'
std::wstring MainWindow::activeGameDir() const {
    std::wstring root = gameDir_.empty() ? joinPath(exeDir(), L".minecraft") : gameDir_;
    if (isolation_ && !selVersion_.empty()) {
        return joinPath(joinPath(root, L"versions"), selVersion_);
    }
    return root;
}

void MainWindow::saveSettings() {
    syncAccountList();
'@

Rep @'
    RECT b1 = mkRect(c1.right - 108, c1.top + 17, c1.right - 16, c1.bottom - 17);
    drawButton(g, b1, L"浏览", BTN_BROWSE_DIR, false, true, 12);
'@ @'
    RECT b1 = mkRect(c1.right - 108, c1.top + 17, c1.right - 16, c1.bottom - 17);
    drawButton(g, b1, L"浏览", BTN_BROWSE_DIR, false, true, 12);
    {
        bool hover = (hoverId_ == BTN_ISOLATION);
        RECT sq = mkRect(c1.right - 292, c1.top + 23, c1.right - 274, c1.top + 41);
        fillRound(g, sq, 4, isolation_ ? kAccent : kWhite);
        strokeRound(g, sq, 4, isolation_ ? kAccent : (hover ? C(160, 165, 172) : C(190, 194, 200)));
        if (isolation_) {
            Pen p(kWhite, 2.0f);
            g.DrawLine(&p, (float)(sq.left + 4), (float)(sq.top + 9), (float)(sq.left + 8), (float)(sq.top + 13));
            g.DrawLine(&p, (float)(sq.left + 8), (float)(sq.top + 13), (float)(sq.left + 14), (float)(sq.top + 5));
        }
        RECT tr = mkRect(sq.right + 8, c1.top, c1.right - 120, c1.bottom);
        drawText(g, L"版本隔离", fL, kText, tr, 0);
        hits_.push_back({ mkRect(c1.right - 300, c1.top, c1.right - 110, c1.bottom), BTN_ISOLATION });
    }
'@

Rep '        SetWindowPos(editDir_, nullptr, S(cx + 100), S(y + 18), S(cw - 260), S(28), SWP_NOZORDER);' '        SetWindowPos(editDir_, nullptr, S(cx + 100), S(y + 18), S(cw - 440), S(28), SWP_NOZORDER);'

Rep '    case BTN_AUTO_BACKUP:' @'
    case BTN_ISOLATION:
        isolation_ = !isolation_;
        saveSettings();
        if (page_ == 8) loadSaves();
        pushLog(isolation_ ? L"已开启版本隔离：mods/saves 等从 versions/<版本>/ 读取"
                           : L"已关闭版本隔离：回到全局 .minecraft");
        if (isolation_ && !selVersion_.empty()) {
            MessageBoxW(hwnd_, L"版本隔离已开启。\n\n当前版本的 mods、saves、resourcepacks 等将从 versions/<版本>/ 读取；现有的全局数据不会自动迁移。",
                        L"版本隔离", MB_ICONINFORMATION);
        }
        dirty_ = true;
        break;

    case BTN_AUTO_BACKUP:
'@

Rep @'
    std::wstring gd = gameDir_;
    if (gd.empty()) gd = joinPath(exeDir(), L".minecraft");
    return joinPath(gd, L"mods");
'@ @'
    return joinPath(activeGameDir(), L"mods");
'@

Rep '    std::wstring dir = joinPath(gameDir_, L"saves");' '    std::wstring dir = joinPath(activeGameDir(), L"saves");'
Rep '    std::wstring savesDir = joinPath(gameDir_, L"saves");' '    std::wstring savesDir = joinPath(activeGameDir(), L"saves");'
Rep '    drawText(g, joinPath(gameDir_, L"saves"), fD, kText, t2, 0);' '    drawText(g, joinPath(activeGameDir(), L"saves"), fD, kText, t2, 0);'

# repeated paths: replace all occurrences
$u = $u.Replace('joinPath(gameDir_, L"backups")', 'joinPath(activeGameDir(), L"backups")')
$u = $u.Replace('joinPath(gameDir_, L"saves")', 'joinPath(activeGameDir(), L"saves")')
$u = $u.Replace('joinPath(gameDir_, L"logs")', 'joinPath(activeGameDir(), L"logs")')
$u = $u.Replace('joinPath(gameDir_, L"crash-reports")', 'joinPath(activeGameDir(), L"crash-reports")')
$u = $u.Replace('joinPath(gameDir_, L"resourcepacks")', 'joinPath(activeGameDir(), L"resourcepacks")')
$u = $u.Replace('joinPath(gameDir_, L"shaderpacks")', 'joinPath(activeGameDir(), L"shaderpacks")')

Rep @'
    std::wstring dir = joinPath(gameDir_, kResEntries[idx].dir);
    if (!pathExists(dir)) createDirs(dir);
'@ @'
    std::wstring rbase = (wcscmp(kResEntries[idx].dir, L"versions") == 0) ? gameDir_ : activeGameDir();
    std::wstring dir = joinPath(rbase, kResEntries[idx].dir);
    if (!pathExists(dir)) createDirs(dir);
'@

Rep '        std::wstring dir = joinPath(gameDir_, kResEntries[i].dir);' @'
        std::wstring rbase = (wcscmp(kResEntries[i].dir, L"versions") == 0) ? gameDir_ : activeGameDir();
        std::wstring dir = joinPath(rbase, kResEntries[i].dir);
'@

Rep '    drawText(g, gameDir_, fD, kText, t2, 0);' '    drawText(g, activeGameDir(), fD, kText, t2, 0);'

Rep @'
    applySettingsToLauncher();
    saveSettings();
    selVersion_ = utf8ToWide(id);
'@ @'
    selVersion_ = utf8ToWide(id);
    applySettingsToLauncher();
    saveSettings();
'@

Rep 'static const wchar_t* kAppVersion = L"1.16.0";' 'static const wchar_t* kAppVersion = L"1.17.0";'

[IO.File]::WriteAllText($p, $u, (New-Object Text.UTF8Encoding($false)))
"isolation patched, length=$($u.Length)"
