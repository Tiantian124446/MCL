$ErrorActionPreference = 'Stop'
$path = 'E:\MCL\src\ui.cpp'
$u = ([IO.File]::ReadAllText($path, [Text.Encoding]::UTF8)) -replace "`r`n", "`n"
function Rep([string]$old, [string]$new) {
    $old = $old -replace "`r`n", "`n"
    $new = $new -replace "`r`n", "`n"
    $count = ([regex]::Matches($script:u, [regex]::Escape($old))).Count
    if ($count -ne 1) { throw "replacement count $count for: $($old.Substring(0,[Math]::Min(70,$old.Length)))" }
    $script:u = $script:u.Replace($old, $new)
}

Rep '    BTN_NAV_RESOURCES,' @'
    BTN_NAV_RESOURCES,
    BTN_NAV_MODMGR,
'@

Rep '    BTN_ANALYZE_CRASH,   // +0..4: 版本分类 全部/正式版/快照/远古版/愚人节版' @'
    BTN_ANALYZE_CRASH,
    BTN_MODMGR_REFRESH,
    BTN_MODMGR_OPEN,
    BTN_MODMGR_TOGGLE_BASE = 910000,   // +列表下标: 启用/禁用
    BTN_MODMGR_DEL_BASE    = 920000,   // +列表下标: 删除
    // +0..4: 版本分类 全部/正式版/快照/远古版/愚人节版
'@

Rep '    void drawResources(Graphics& g, int W, int H);' @'
    void drawResources(Graphics& g, int W, int H);
    void drawModMgr(Graphics& g, int W, int H);
'@

Rep '    void analyzeCrash();                 // 读取最新崩溃报告并给出建议' @'
    void analyzeCrash();                 // 读取最新崩溃报告并给出建议
    void loadInstalledMods();            // 扫描 mods 目录
    void toggleInstalledMod(int idx);    // 启用/禁用模组
    void deleteInstalledMod(int idx);    // 删除模组
'@

Rep '    std::vector<ModFile> modFiles_;' @'
    std::vector<ModFile> modFiles_;
    struct InstalledMod { std::wstring file; std::wstring name; unsigned long long size; bool enabled; };
    std::vector<InstalledMod> installedMods_;
    int modMgrScroll_ = 0;
'@

Rep '        { L"资源管理", BTN_NAV_RESOURCES, 6 },' @'
        { L"资源管理", BTN_NAV_RESOURCES, 6 },
        { L"模组管理", BTN_NAV_MODMGR, 7 },
'@

Rep '        case 6: drawResources(g, W, H); break;' @'
        case 6: drawResources(g, W, H); break;
        case 7: drawModMgr(g, W, H); break;
'@

Rep '    case 6: return L"资源管理";' @'
    case 6: return L"资源管理";
    case 7: return L"模组管理";
'@

Rep @'
    } else if (page_ == 5) {
        srvScroll_ -= (delta / WHEEL_DELTA) * 2;
        if (srvScroll_ < 0) srvScroll_ = 0;
    } else {
        return;
    }
'@ @'
    } else if (page_ == 5) {
        srvScroll_ -= (delta / WHEEL_DELTA) * 2;
        if (srvScroll_ < 0) srvScroll_ = 0;
    } else if (page_ == 7) {
        modMgrScroll_ -= (delta / WHEEL_DELTA) * 2;
        if (modMgrScroll_ < 0) modMgrScroll_ = 0;
    } else {
        return;
    }
'@
Rep '    if (id == BTN_ANALYZE_CRASH) {' @'
    if (id >= BTN_MODMGR_TOGGLE_BASE && id < BTN_MODMGR_TOGGLE_BASE + 1000) {
        toggleInstalledMod(id - BTN_MODMGR_TOGGLE_BASE);
        return;
    }
    if (id >= BTN_MODMGR_DEL_BASE && id < BTN_MODMGR_DEL_BASE + 1000) {
        deleteInstalledMod(id - BTN_MODMGR_DEL_BASE);
        return;
    }
    if (id == BTN_MODMGR_REFRESH) {
        loadInstalledMods();
        pushLog(L"模组列表已刷新。");
        dirty_ = true;
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    if (id == BTN_MODMGR_OPEN) {
        std::wstring d = modsDir();
        if (!pathExists(d)) createDirs(d);
        ShellExecuteW(nullptr, L"open", d.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        return;
    }
    if (id == BTN_ANALYZE_CRASH) {
'@

Rep '    // ---- 资源管理页
    case BTN_NAV_RESOURCES:' @'
    // ---- 模组管理页
    case BTN_NAV_MODMGR:
        page_ = 7; loadInstalledMods(); layoutControls(); dirty_ = true; break;

    // ---- 资源管理页
    case BTN_NAV_RESOURCES:
'@

$code = @'
// ------------------------------ 模组管理页 ---------------------------------
static bool endsWithNoCase(const std::wstring& s, const std::wstring& tail) {
    if (s.size() < tail.size()) return false;
    return _wcsicmp(s.c_str() + (s.size() - tail.size()), tail.c_str()) == 0;
}

void MainWindow::loadInstalledMods() {
    installedMods_.clear();
    std::wstring dir = modsDir();
    if (!pathExists(dir)) createDirs(dir);
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((dir + L"\\*").c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            std::wstring fn = fd.cFileName;
            bool disabled = endsWithNoCase(fn, L".jar.disabled");
            bool enabled = !disabled && endsWithNoCase(fn, L".jar");
            if (!disabled && !enabled) continue;
            InstalledMod m;
            m.file = fn;
            m.name = disabled ? fn.substr(0, fn.size() - 9) : fn;
            m.size = ((unsigned long long)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
            m.enabled = enabled;
            installedMods_.push_back(m);
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    std::sort(installedMods_.begin(), installedMods_.end(),
              [](const InstalledMod& a, const InstalledMod& b) { return _wcsicmp(a.name.c_str(), b.name.c_str()) < 0; });
    if (modMgrScroll_ >= (int)installedMods_.size()) modMgrScroll_ = 0;
}

void MainWindow::toggleInstalledMod(int idx) {
    if (idx < 0 || idx >= (int)installedMods_.size()) return;
    const InstalledMod& m = installedMods_[idx];
    std::wstring dir = modsDir();
    std::wstring from = joinPath(dir, m.file);
    std::wstring to;
    if (m.enabled) {
        to = from + L".disabled";
    } else {
        if (!endsWithNoCase(from, L".disabled")) return;
        to = from.substr(0, from.size() - 9);
    }
    if (!MoveFileW(from.c_str(), to.c_str())) {
        MessageBoxW(hwnd_, L"无法重命名模组文件，可能被游戏占用。", L"MCL 模组管理", MB_ICONWARNING);
        return;
    }
    pushLog((m.enabled ? L"已禁用模组: " : L"已启用模组: ") + m.name);
    loadInstalledMods();
    dirty_ = true;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::deleteInstalledMod(int idx) {
    if (idx < 0 || idx >= (int)installedMods_.size()) return;
    InstalledMod m = installedMods_[idx];
    std::wstring msg = L"确定删除模组？\n\n" + m.name;
    if (MessageBoxW(hwnd_, msg.c_str(), L"MCL 模组管理", MB_YESNO | MB_ICONQUESTION) != IDYES) return;
    std::wstring path = joinPath(modsDir(), m.file);
    if (!DeleteFileW(path.c_str())) {
        MessageBoxW(hwnd_, L"删除失败，文件可能被占用。", L"MCL 模组管理", MB_ICONWARNING);
        return;
    }
    pushLog(L"已删除模组: " + m.name);
    loadInstalledMods();
    dirty_ = true;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::drawModMgr(Graphics& g, int W, int H) {
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
    drawText(g, L"模组目录", fL, kTextDim, t1, 0);
    RECT t2 = mkRect(info.left + 16, info.top + 28, info.right - 320, info.top + 50);
    drawText(g, modsDir(), fD, kText, t2, 0);
    int en = 0, dis = 0;
    for (auto& m : installedMods_) { if (m.enabled) en++; else dis++; }
    std::wstring cnt = L"共 " + std::to_wstring(installedMods_.size()) + L" 个 · 已启用 " +
                       std::to_wstring(en) + L" · 已禁用 " + std::to_wstring(dis);
    RECT t3 = mkRect(info.left + 16, info.top + 46, info.right - 320, info.top + 62);
    drawText(g, cnt, fD, kTextDim, t3, 0);
    RECT b1 = mkRect(info.right - 300, info.top + 16, info.right - 168, info.top + 46);
    drawButton(g, b1, L"刷新", BTN_MODMGR_REFRESH, false, true, 12);
    RECT b2 = mkRect(info.right - 156, info.top + 16, info.right - 16, info.top + 46);
    drawButton(g, b2, L"打开 mods 文件夹", BTN_MODMGR_OPEN, false, true, 12);

    int listTop = y + 64 + 12;
    int listBottom = H - 16;
    int rowH = 46;
    int headerH = 28;
    int rows = (listBottom - listTop - headerH - 8) / rowH;
    if (rows < 1) rows = 1;
    int total = (int)installedMods_.size();
    int maxScroll = total > rows ? total - rows : 0;
    if (modMgrScroll_ > maxScroll) modMgrScroll_ = maxScroll;
    if (modMgrScroll_ < 0) modMgrScroll_ = 0;

    RECT list = mkRect(cx, listTop, cx + cw, listBottom);
    drawCard(g, list);
    RECT hdr = mkRect(list.left + 12, list.top + 8, list.right - 12, list.top + 8 + headerH);
    drawText(g, L"模组名称", fH, kTextDim, mkRect(hdr.left, hdr.top, hdr.left + 360, hdr.bottom), 0);
    drawText(g, L"大小", fH, kTextDim, mkRect(hdr.right - 300, hdr.top, hdr.right - 220, hdr.bottom), 0);
    drawText(g, L"状态", fH, kTextDim, mkRect(hdr.right - 210, hdr.top, hdr.right - 140, hdr.bottom), 0);
    drawText(g, L"操作", fH, kTextDim, mkRect(hdr.right - 130, hdr.top, hdr.right - 12, hdr.bottom), 0);

    if (total == 0) {
        RECT e = mkRect(list.left + 12, list.top + headerH + 24, list.right - 12, list.top + headerH + 56);
        drawText(g, L"mods 文件夹为空。去「模组下载」搜索并下载模组，或点右上角打开 mods 文件夹手动放入 .jar。",
                 fL, kTextDim, e, 0);
        return;
    }

    int ry = list.top + 8 + headerH;
    for (int i = 0; i < rows; ++i) {
        int idx = modMgrScroll_ + i;
        if (idx >= total) break;
        const InstalledMod& m = installedMods_[idx];
        RECT row = mkRect(list.left + 8, ry, list.right - 8, ry + rowH - 4);
        if ((idx & 1) == 0) fillRound(g, row, 6, C(250, 251, 253));
        RECT rn = mkRect(row.left + 8, row.top, row.right - 330, row.bottom);
        std::wstring name = m.name;
        if (name.size() > 54) name = name.substr(0, 51) + L"...";
        drawText(g, name, fB, kText, rn, 0);
        RECT rs = mkRect(row.right - 298, row.top, row.right - 220, row.bottom);
        drawText(g, humanSize(m.size), fD, kTextDim, rs, 0);
        RECT rst = mkRect(row.right - 208, row.top, row.right - 136, row.bottom);
        drawText(g, m.enabled ? L"已启用" : L"已禁用", fD, m.enabled ? kAccent : kTextDim, rst, 0);
        RECT bt = mkRect(row.right - 128, row.top + 8, row.right - 66, row.bottom - 8);
        drawButton(g, bt, m.enabled ? L"禁用" : L"启用", BTN_MODMGR_TOGGLE_BASE + idx, false, true, 11);
        RECT bd = mkRect(row.right - 58, row.top + 8, row.right - 6, row.bottom - 8);
        drawButton(g, bd, L"删除", BTN_MODMGR_DEL_BASE + idx, false, true, 11);
        ry += rowH;
    }
    if (maxScroll > 0) {
        RECT barBg = mkRect(list.right - 6, list.top + headerH + 4, list.right - 2, list.bottom - 4);
        fillRound(g, barBg, 2, C(232, 234, 238));
        int trackH = barBg.bottom - barBg.top;
        int barH = trackH * rows / total;
        if (barH < 24) barH = 24;
        int barY = barBg.top + (trackH - barH) * modMgrScroll_ / maxScroll;
        fillRound(g, mkRect(barBg.left, barY, barBg.right, barY + barH), 2, C(198, 202, 210));
    }
}

'@
$code = $code -replace "`r`n", "`n"
Rep '// ------------------------------ 资源管理页 ---------------------------------' ($code + '// ------------------------------ 资源管理页 ---------------------------------')

[IO.File]::WriteAllText($path, $u, (New-Object Text.UTF8Encoding($false)))
"mod manager patched, length=$($u.Length)"
