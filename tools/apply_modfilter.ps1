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

Rep '    int modMgrScroll_ = 0;' @'
    int modMgrScroll_ = 0;
    std::wstring modMgrFilter_;      // 模组列表筛选词
'@

Rep @'
    } else if (page_ == 5) {
        int y = HEADER_H + 16;
        int hostX = cx + 264;                 // 与 drawServers 的表单卡片保持一致
'@ @'
    } else if (page_ == 7) {
        int y = HEADER_H + 16;
        SetWindowPos(editModSearch_, nullptr, S(cx + 64), S(y + 60), S(cw - 500), S(28), SWP_NOZORDER);
        ShowWindow(editModSearch_, SW_SHOW);
    } else if (page_ == 5) {
        int y = HEADER_H + 16;
        int hostX = cx + 264;                 // 与 drawServers 的表单卡片保持一致
'@

Rep @'
                InvalidateRect(h, nullptr, FALSE);
            }
            // 输入内容变化 -> 稍后由按钮点击时统一读取
'@ @'
                InvalidateRect(h, nullptr, FALSE);
            } else if (LOWORD(w) == CTL_MOD_SEARCH && self.page_ == 7) {
                wchar_t b[256] = { 0 };
                GetWindowTextW((HWND)l, b, 256);
                self.modMgrFilter_ = trimW(std::wstring(b));
                self.modMgrScroll_ = 0;
                self.dirty_ = true;
                InvalidateRect(h, nullptr, FALSE);
            }
            // 输入内容变化 -> 稍后由按钮点击时统一读取
'@

Rep '        page_ = 7; loadInstalledMods(); layoutControls(); dirty_ = true; break;' @"
        page_ = 7;
        modMgrFilter_.clear();
        if (editModSearch_) SetWindowTextW(editModSearch_, L"");
        loadInstalledMods(); layoutControls(); dirty_ = true; break;
"@

Rep @'
    RECT info = mkRect(cx, y, cx + cw, y + 64);
    drawCard(g, info);
    RECT t1 = mkRect(info.left + 16, info.top + 8, info.right - 440, info.top + 30);
    drawText(g, L"模组目录", fL, kTextDim, t1, 0);
'@ @'
    RECT info = mkRect(cx, y, cx + cw, y + 96);
    drawCard(g, info);
    RECT t1 = mkRect(info.left + 16, info.top + 8, info.right - 440, info.top + 30);
    drawText(g, L"模组目录", fL, kTextDim, t1, 0);
'@
Rep @'
    int listTop = y + 64 + 12;
    int listBottom = H - 16;
    int rowH = 46;
    int headerH = 28;
    int rows = (listBottom - listTop - headerH - 8) / rowH;
    if (rows < 1) rows = 1;
    int total = (int)installedMods_.size();
    int maxScroll = total > rows ? total - rows : 0;
'@ @'
    int listTop = y + 96 + 12;
    int listBottom = H - 16;
    int rowH = 46;
    int headerH = 28;
    int rows = (listBottom - listTop - headerH - 8) / rowH;
    if (rows < 1) rows = 1;
    std::vector<int> vis;
    {
        std::wstring q = modMgrFilter_;
        for (auto& c : q) c = (wchar_t)towlower(c);
        for (int i = 0; i < (int)installedMods_.size(); ++i) {
            std::wstring n = installedMods_[i].name;
            for (auto& c : n) c = (wchar_t)towlower(c);
            if (q.empty() || n.find(q) != std::wstring::npos) vis.push_back(i);
        }
    }
    int total = (int)vis.size();
    int maxScroll = total > rows ? total - rows : 0;
'@

Rep @'
    RECT b4 = mkRect(info.right - 106, info.top + 16, info.right - 16, info.top + 46);
    drawButton(g, b4, L"打开目录", BTN_MODMGR_OPEN, false, true, 12);
'@ @'
    RECT b4 = mkRect(info.right - 106, info.top + 16, info.right - 16, info.top + 46);
    drawButton(g, b4, L"打开目录", BTN_MODMGR_OPEN, false, true, 12);
    RECT fl = mkRect(info.left + 16, info.top + 64, info.left + 56, info.top + 92);
    drawText(g, L"筛选", fL, kTextDim, fl, 0);
    RECT fi = mkRect(info.left + 64, info.top + 64, info.right - 440, info.top + 92);
    fillRound(g, fi, 7, kInputBg);
'@

Rep @'
        drawText(g, L"mods 文件夹为空。去「模组下载」搜索并下载模组，或点右上角打开 mods 文件夹手动放入 .jar。",
                 fL, kTextDim, e, 0);
'@ @'
        drawText(g, installedMods_.empty() ? L"mods 文件夹为空。去「模组下载」搜索并下载模组，或打开目录手动放入 .jar。" : L"没有匹配的模组，换个关键词试试。",
                 fL, kTextDim, e, 0);
'@

Rep '        const InstalledMod& m = installedMods_[idx];' @'
        int orig = vis[idx];
        const InstalledMod& m = installedMods_[orig];
'@

Rep '        drawButton(g, bt, m.enabled ? L"禁用" : L"启用", BTN_MODMGR_TOGGLE_BASE + idx, false, true, 11);' '        drawButton(g, bt, m.enabled ? L"禁用" : L"启用", BTN_MODMGR_TOGGLE_BASE + orig, false, true, 11);'
Rep '        drawButton(g, bd, L"删除", BTN_MODMGR_DEL_BASE + idx, false, true, 11);' '        drawButton(g, bd, L"删除", BTN_MODMGR_DEL_BASE + orig, false, true, 11);'
Rep 'static const wchar_t* kAppVersion = L"1.8.0";' 'static const wchar_t* kAppVersion = L"1.9.0";'

[IO.File]::WriteAllText($p, $u, (New-Object Text.UTF8Encoding($false)))
"mod filter patched, length=$($u.Length)"
