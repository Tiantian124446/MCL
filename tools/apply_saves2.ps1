$ErrorActionPreference = 'Stop'
$path = 'E:\MCL\src\ui.cpp'
$u = ([IO.File]::ReadAllText($path, [Text.Encoding]::UTF8)) -replace "`r`n", "`n"
function Rep([string]$old, [string]$new) {
    $old = $old -replace "`r`n", "`n"
    $new = $new -replace "`r`n", "`n"
    $count = ([regex]::Matches($script:u, [regex]::Escape($old))).Count
    if ($count -ne 1) { throw "replacement count $count for: $($old.Substring(0,[Math]::Min(80,$old.Length)))" }
    $script:u = $script:u.Replace($old, $new)
}

Rep '#include <algorithm>' "#include <algorithm>`n#include <filesystem>"

Rep '    BTN_NAV_MODMGR,' @'
    BTN_NAV_MODMGR,
    BTN_NAV_SAVES,
'@

Rep '    BTN_MODMGR_DEL_BASE    = 920000,   // +列表下标: 删除' @'
    BTN_MODMGR_DEL_BASE    = 920000,   // +列表下标: 删除
    BTN_SAVE_OPEN_BASE     = 930000,   // +列表下标: 打开存档目录
    BTN_SAVE_BACKUP_BASE   = 940000,   // +列表下标: 备份存档
    BTN_SAVE_DEL_BASE      = 950000,   // +列表下标: 删除存档
'@

Rep '    void drawModMgr(Graphics& g, int W, int H);' @'
    void drawModMgr(Graphics& g, int W, int H);
    void drawSaves(Graphics& g, int W, int H);
'@

Rep '    void deleteInstalledMod(int idx);    // 删除模组' @'
    void deleteInstalledMod(int idx);    // 删除模组
    void loadSaves();                    // 扫描 saves 目录
    void backupSave(int idx);            // 备份存档
    void deleteSave(int idx);            // 删除存档
'@

Rep '    int modMgrScroll_ = 0;' @'
    int modMgrScroll_ = 0;
    struct SaveEntry { std::wstring name; std::wstring path; unsigned long long size; FILETIME modified; };
    std::vector<SaveEntry> saves_;
'@

Rep '        { L"模组管理", BTN_NAV_MODMGR, 7 },' @'
        { L"模组管理", BTN_NAV_MODMGR, 7 },
        { L"存档管理", BTN_NAV_SAVES, 8 },
'@

Rep '        case 7: drawModMgr(g, W, H); break;' @'
        case 7: drawModMgr(g, W, H); break;
        case 8: drawSaves(g, W, H); break;
'@

Rep '    case 7: return L"模组管理";' @'
    case 7: return L"模组管理";
    case 8: return L"存档管理";
'@

Rep '    } else if (page_ == 7) {' '    } else if (page_ == 7 || page_ == 8) {'

Rep '    if (id >= BTN_MODMGR_DEL_BASE && id < BTN_MODMGR_DEL_BASE + 1000) {' @'
    if (id >= BTN_SAVE_OPEN_BASE && id < BTN_SAVE_OPEN_BASE + 1000) {
        int i = id - BTN_SAVE_OPEN_BASE;
        if (i >= 0 && i < (int)saves_.size()) ShellExecuteW(nullptr, L"open", saves_[i].path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        return;
    }
    if (id >= BTN_SAVE_BACKUP_BASE && id < BTN_SAVE_BACKUP_BASE + 1000) {
        backupSave(id - BTN_SAVE_BACKUP_BASE);
        return;
    }
    if (id >= BTN_SAVE_DEL_BASE && id < BTN_SAVE_DEL_BASE + 1000) {
        deleteSave(id - BTN_SAVE_DEL_BASE);
        return;
    }
    if (id >= BTN_MODMGR_DEL_BASE && id < BTN_MODMGR_DEL_BASE + 1000) {
'@

Rep '    if (id == BTN_MODMGR_REFRESH) {
        loadInstalledMods();
        pushLog(L"模组列表已刷新。");
        dirty_ = true;
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }' @'
    if (id == BTN_MODMGR_REFRESH) {
        if (page_ == 8) {
            loadSaves();
            pushLog(L"存档列表已刷新。");
        } else {
            loadInstalledMods();
            pushLog(L"模组列表已刷新。");
        }
        dirty_ = true;
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
'@

Rep '    if (id == BTN_MODMGR_OPEN) {
        std::wstring d = modsDir();
        if (!pathExists(d)) createDirs(d);
        ShellExecuteW(nullptr, L"open", d.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        return;
    }' @'
    if (id == BTN_MODMGR_OPEN) {
        std::wstring d = (page_ == 8) ? joinPath(gameDir_, L"backups") : modsDir();
        if (!pathExists(d)) createDirs(d);
        ShellExecuteW(nullptr, L"open", d.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        return;
    }
'@

Rep '    // ---- 模组管理页
    case BTN_NAV_MODMGR:' @'
    // ---- 存档管理页
    case BTN_NAV_SAVES:
        page_ = 8; loadSaves(); layoutControls(); dirty_ = true; break;

    // ---- 模组管理页
    case BTN_NAV_MODMGR:
'@

$code = @'
// ------------------------------ 存档管理页 ---------------------------------
static void dirStats(const std::wstring& dir, unsigned long long& total, FILETIME& newest) {
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((dir + L"\\*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
        std::wstring p = dir + L"\\" + fd.cFileName;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            dirStats(p, total, newest);
        } else {
            total += ((unsigned long long)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
            if (CompareFileTime(&fd.ftLastWriteTime, &newest) > 0) newest = fd.ftLastWriteTime;
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}

static std::wstring safeName(const std::wstring& s) {
    std::wstring out = s;
    for (auto& c : out) {
        if (c == L'\\' || c == L'/' || c == L':' || c == L'*' || c == L'?' || c == L'"' ||
            c == L'<' || c == L'>' || c == L'|') c = L'_';
    }
    return out;
}

static std::wstring fileTimeText(const FILETIME& ft) {
    SYSTEMTIME utc{}, local{};
    if (!FileTimeToSystemTime(&ft, &utc)) return L"-";
    SystemTimeToTzSpecificLocalTime(nullptr, &utc, &local);
    wchar_t buf[64];
    swprintf(buf, 64, L"%04d-%02d-%02d %02d:%02d", local.wYear, local.wMonth, local.wDay, local.wHour, local.wMinute);
    return buf;
}

void MainWindow::loadSaves() {
    saves_.clear();
    std::wstring dir = joinPath(gameDir_, L"saves");
    if (!pathExists(dir)) createDirs(dir);
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((dir + L"\\*").c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
            if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
            SaveEntry e;
            e.name = fd.cFileName;
            e.path = dir + L"\\" + fd.cFileName;
            e.size = 0;
            e.modified = fd.ftLastWriteTime;
            dirStats(e.path, e.size, e.modified);
            saves_.push_back(e);
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    std::sort(saves_.begin(), saves_.end(),
              [](const SaveEntry& a, const SaveEntry& b) { return _wcsicmp(a.name.c_str(), b.name.c_str()) < 0; });
    if (modMgrScroll_ >= (int)saves_.size()) modMgrScroll_ = 0;
}

void MainWindow::backupSave(int idx) {
    if (idx < 0 || idx >= (int)saves_.size()) return;
    const SaveEntry& e = saves_[idx];
    std::wstring backups = joinPath(gameDir_, L"backups");
    createDirs(backups);
    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t stamp[64];
    swprintf(stamp, 64, L"%04d%02d%02d_%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    std::wstring dst = joinPath(backups, L"saves_" + safeName(e.name) + L"_" + stamp);
    std::error_code ec;
    std::filesystem::copy(e.path, dst, std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing, ec);
    if (ec) {
        MessageBoxW(hwnd_, (L"备份失败：" + utf8ToWide(ec.message())).c_str(), L"MCL 存档管理", MB_ICONWARNING);
        return;
    }
    pushLog(L"已备份存档: " + e.name + L" -> " + dst);
    MessageBoxW(hwnd_, (L"备份完成：\n" + dst).c_str(), L"MCL 存档管理", MB_ICONINFORMATION);
}

void MainWindow::deleteSave(int idx) {
    if (idx < 0 || idx >= (int)saves_.size()) return;
    SaveEntry e = saves_[idx];
    std::wstring msg = L"确定删除存档？此操作不可撤销。\n\n" + e.name;
    if (MessageBoxW(hwnd_, msg.c_str(), L"MCL 存档管理", MB_YESNO | MB_ICONWARNING) != IDYES) return;
    std::error_code ec;
    std::filesystem::remove_all(e.path, ec);
    if (ec) {
        MessageBoxW(hwnd_, L"删除失败，存档可能被占用。", L"MCL 存档管理", MB_ICONWARNING);
        return;
    }
    pushLog(L"已删除存档: " + e.name);
    loadSaves();
    dirty_ = true;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::drawSaves(Graphics& g, int W, int H) {
    FontFamily ff(L"Microsoft YaHei");
    Font fL(&ff, 12.0f, FontStyleRegular, UnitPixel);
    Font fB(&ff, 14.0f, FontStyleBold, UnitPixel);
    Font fD(&ff, 11.5f, FontStyleRegular, UnitPixel);
    Font fH(&ff, 11.0f, FontStyleRegular, UnitPixel);

    int cx = SIDEBAR_W + 24;
    int cw = W - SIDEBAR_W - 48;
    int y = HEADER_H + 16;

    RECT info = mkRect(cx, y, cx + cw, y + 64);
    drawCard(g, info);
    RECT t1 = mkRect(info.left + 16, info.top + 8, info.right - 320, info.top + 30);
    drawText(g, L"存档目录", fL, kTextDim, t1, 0);
    RECT t2 = mkRect(info.left + 16, info.top + 28, info.right - 320, info.top + 50);
    drawText(g, joinPath(gameDir_, L"saves"), fD, kText, t2, 0);
    unsigned long long total = 0;
    for (auto& e : saves_) total += e.size;
    std::wstring cnt = L"共 " + std::to_wstring(saves_.size()) + L" 个存档 · 合计 " + humanSize(total);
    RECT t3 = mkRect(info.left + 16, info.top + 46, info.right - 320, info.top + 62);
    drawText(g, cnt, fD, kTextDim, t3, 0);
    RECT b1 = mkRect(info.right - 300, info.top + 16, info.right - 168, info.top + 46);
    drawButton(g, b1, L"刷新", BTN_MODMGR_REFRESH, false, true, 12);
    RECT b2 = mkRect(info.right - 156, info.top + 16, info.right - 16, info.top + 46);
    drawButton(g, b2, L"打开 backups", BTN_MODMGR_OPEN, false, true, 12);

    int listTop = y + 64 + 12;
    int listBottom = H - 16;
    int rowH = 46;
    int headerH = 28;
    int rows = (listBottom - listTop - headerH - 8) / rowH;
    if (rows < 1) rows = 1;
    int totalCount = (int)saves_.size();
    int maxScroll = totalCount > rows ? totalCount - rows : 0;
    if (modMgrScroll_ > maxScroll) modMgrScroll_ = maxScroll;
    if (modMgrScroll_ < 0) modMgrScroll_ = 0;

    RECT list = mkRect(cx, listTop, cx + cw, listBottom);
    drawCard(g, list);
    RECT hdr = mkRect(list.left + 12, list.top + 8, list.right - 12, list.top + 8 + headerH);
    drawText(g, L"存档名称", fH, kTextDim, mkRect(hdr.left, hdr.top, hdr.left + 300, hdr.bottom), 0);
    drawText(g, L"大小", fH, kTextDim, mkRect(hdr.right - 300, hdr.top, hdr.right - 220, hdr.bottom), 0);
    drawText(g, L"最后修改", fH, kTextDim, mkRect(hdr.right - 210, hdr.top, hdr.right - 140, hdr.bottom), 0);
    drawText(g, L"操作", fH, kTextDim, mkRect(hdr.right - 134, hdr.top, hdr.right - 12, hdr.bottom), 0);

    if (totalCount == 0) {
        RECT e = mkRect(list.left + 12, list.top + headerH + 24, list.right - 12, list.top + headerH + 56);
        drawText(g, L"saves 文件夹为空。进入游戏创建一个世界后，这里会出现存档。", fL, kTextDim, e, 0);
        return;
    }

    int ry = list.top + 8 + headerH;
    for (int i = 0; i < rows; ++i) {
        int idx = modMgrScroll_ + i;
        if (idx >= totalCount) break;
        const SaveEntry& e = saves_[idx];
        RECT row = mkRect(list.left + 8, ry, list.right - 8, ry + rowH - 4);
        if ((idx & 1) == 0) fillRound(g, row, 6, C(250, 251, 253));
        RECT rn = mkRect(row.left + 8, row.top, row.right - 340, row.bottom);
        std::wstring name = e.name;
        if (name.size() > 44) name = name.substr(0, 41) + L"...";
        drawText(g, name, fB, kText, rn, 0);
        RECT rs = mkRect(row.right - 298, row.top, row.right - 220, row.bottom);
        drawText(g, humanSize(e.size), fD, kTextDim, rs, 0);
        RECT rt = mkRect(row.right - 208, row.top, row.right - 140, row.bottom);
        drawText(g, fileTimeText(e.modified), fD, kTextDim, rt, 0);
        RECT bd = mkRect(row.right - 132, row.top + 8, row.right - 92, row.bottom - 8);
        drawButton(g, bd, L"删除", BTN_SAVE_DEL_BASE + idx, false, true, 11);
        RECT bb = mkRect(row.right - 88, row.top + 8, row.right - 48, row.bottom - 8);
        drawButton(g, bb, L"备份", BTN_SAVE_BACKUP_BASE + idx, false, true, 11);
        RECT bo = mkRect(row.right - 44, row.top + 8, row.right - 4, row.bottom - 8);
        drawButton(g, bo, L"打开", BTN_SAVE_OPEN_BASE + idx, false, true, 11);
        ry += rowH;
    }
}

'@
$code = $code -replace "`r`n", "`n"
Rep '// ------------------------------ 资源管理页 ---------------------------------' ($code + '// ------------------------------ 资源管理页 ---------------------------------')

[IO.File]::WriteAllText($path, $u, (New-Object Text.UTF8Encoding($false)))
"saves page patched, length=$($u.Length)"
