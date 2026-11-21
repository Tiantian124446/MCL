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

Rep '    BTN_NAV_SERVERS,         // 侧边栏: 联机' @'
    BTN_NAV_SERVERS,
    BTN_NAV_RESOURCES,
'@

Rep '    BTN_VERCAT_BASE        = 800000,' @'
    BTN_VERCAT_BASE        = 800000,
    BTN_RES_OPEN_BASE      = 900000,   // +0..7: 资源管理卡片
    BTN_OPEN_GAMEDIR,
'@

Rep '    void drawServers(Graphics& g, int W, int H);' @'
    void drawServers(Graphics& g, int W, int H);
    void drawResources(Graphics& g, int W, int H);
'@

Rep '    void connectServer(int idx);         // 一键启动并直连该服务器' @'
    void connectServer(int idx);         // 一键启动并直连该服务器
    void openResourceFolder(int idx);    // 打开资源包/光影/存档等目录
'@

Rep '        { L"联机",     BTN_NAV_SERVERS, 5 },' @'
        { L"联机",     BTN_NAV_SERVERS, 5 },
        { L"资源管理", BTN_NAV_RESOURCES, 6 },
'@

Rep '        case 5: drawServers(g, W, H); break;' @'
        case 5: drawServers(g, W, H); break;
        case 6: drawResources(g, W, H); break;
'@

Rep '    case 5: return L"联机  ·  服务器列表";' @'
    case 5: return L"联机  ·  服务器列表";
    case 6: return L"资源管理";
'@

Rep '    switch (id) {
    case BTN_NAV_HOME:' @'
    if (id >= BTN_RES_OPEN_BASE && id < BTN_RES_OPEN_BASE + 8) {
        openResourceFolder(id - BTN_RES_OPEN_BASE);
        return;
    }
    if (id == BTN_OPEN_GAMEDIR) {
        if (!pathExists(gameDir_)) createDirs(gameDir_);
        ShellExecuteW(nullptr, L"open", gameDir_.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        return;
    }
    switch (id) {
    case BTN_NAV_HOME:
'@

Rep '    // ---- 联机页
    case BTN_NAV_SERVERS:' @'
    // ---- 资源管理页
    case BTN_NAV_RESOURCES:
        page_ = 6; layoutControls(); dirty_ = true; break;

    // ---- 联机页
    case BTN_NAV_SERVERS:
'@

$drawRes = @'
// ------------------------------ 资源管理页 ---------------------------------
struct ResEntry {
    const wchar_t* dir;
    const wchar_t* title;
    const wchar_t* desc;
    const wchar_t* filter;
};

static const ResEntry kResEntries[] = {
    { L"resourcepacks", L"资源包",   L"材质、音效与界面资源",       L"*" },
    { L"shaderpacks",   L"光影包",   L"Shaders 光影与渲染配置",     L"*" },
    { L"saves",         L"存档",     L"单人世界与地图",             L"*" },
    { L"screenshots",   L"截图",     L"游戏内截图",                 L"*" },
    { L"mods",          L"模组",     L"Forge / Fabric 模组",        L"*" },
    { L"logs",          L"日志",     L"latest.log 与调试日志",      L"*" },
    { L"crash-reports", L"崩溃报告", L"crash-*.txt 崩溃记录",       L"*.txt" },
    { L"versions",      L"版本",     L"已安装的游戏版本",           L"*" },
};

static int countDirEntries(const std::wstring& dir, const wchar_t* filter) {
    int n = 0;
    WIN32_FIND_DATAW fd;
    std::wstring pattern = dir + L"\\" + (filter ? filter : L"*");
    HANDLE h = FindFirstFileW(pattern.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    do {
        if (wcscmp(fd.cFileName, L".") != 0 && wcscmp(fd.cFileName, L"..") != 0) n++;
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    return n;
}

void MainWindow::openResourceFolder(int idx) {
    int count = (int)(sizeof(kResEntries) / sizeof(kResEntries[0]));
    if (idx < 0 || idx >= count) return;
    std::wstring dir = joinPath(gameDir_, kResEntries[idx].dir);
    if (!pathExists(dir)) createDirs(dir);
    ShellExecuteW(nullptr, L"open", dir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void MainWindow::drawResources(Graphics& g, int W, int H) {
    FontFamily ff(L"Microsoft YaHei");
    Font fT(&ff, 18.0f, FontStyleBold, UnitPixel);
    Font fL(&ff, 12.0f, FontStyleRegular, UnitPixel);
    Font fB(&ff, 16.0f, FontStyleBold, UnitPixel);
    Font fD(&ff, 12.0f, FontStyleRegular, UnitPixel);
    Font fC(&ff, 11.0f, FontStyleRegular, UnitPixel);

    int cx = SIDEBAR_W + 24;
    int cw = W - SIDEBAR_W - 48;
    int y = HEADER_H + 16;

    // 当前游戏目录
    RECT info = mkRect(cx, y, cx + cw, y + 58);
    drawCard(g, info);
    RECT t1 = mkRect(info.left + 16, info.top + 8, info.right - 140, info.top + 30);
    drawText(g, L"当前游戏目录", fL, kTextDim, t1, 0);
    RECT t2 = mkRect(info.left + 16, info.top + 28, info.right - 140, info.top + 50);
    drawText(g, gameDir_, fD, kText, t2, 0);
    RECT b0 = mkRect(info.right - 128, info.top + 14, info.right - 16, info.top + 44);
    drawButton(g, b0, L"打开 .minecraft", BTN_OPEN_GAMEDIR, false, true, 12);

    int gridTop = y + 58 + 14;
    int gap = 14;
    int cardW = (cw - gap) / 2;
    int cardH = 104;
    int total = (int)(sizeof(kResEntries) / sizeof(kResEntries[0]));

    for (int i = 0; i < total; ++i) {
        int row = i / 2;
        int col = i % 2;
        int x = cx + col * (cardW + gap);
        int yy = gridTop + row * (cardH + gap);
        if (yy + cardH > H - 16) break;
        RECT c = mkRect(x, yy, x + cardW, yy + cardH);
        drawCard(g, c);

        RECT rt = mkRect(c.left + 16, c.top + 12, c.right - 90, c.top + 36);
        drawText(g, kResEntries[i].title, fB, kText, rt, 0);

        std::wstring dir = joinPath(gameDir_, kResEntries[i].dir);
        int n = countDirEntries(dir, kResEntries[i].filter);
        std::wstring cnt = std::to_wstring(n) + L" 项";
        RECT rc = mkRect(c.right - 86, c.top + 14, c.right - 16, c.top + 36);
        drawText(g, cnt, fC, n > 0 ? kAccent : kTextDim, rc, 2);

        RECT rd = mkRect(c.left + 16, c.top + 40, c.right - 16, c.top + 62);
        drawText(g, kResEntries[i].desc, fD, kTextDim, rd, 0);

        RECT rb = mkRect(c.left + 16, c.top + 68, c.left + 126, c.top + 98);
        drawButton(g, rb, L"打开文件夹", BTN_RES_OPEN_BASE + i, false, true, 12);
    }

    RECT hint = mkRect(cx, H - 36, cx + cw, H - 16);
    drawText(g, L"提示：把资源包 / 光影放入对应文件夹后，在游戏内「选项 → 资源包 / 视频设置」中启用。",
             fD, kTextDim, hint, 0);
}

'@
$drawRes = $drawRes -replace "`r`n", "`n"
Rep '// ------------------------------ 鼠标 ---------------------------------------
void MainWindow::onMouseMove' ($drawRes + '// ------------------------------ 鼠标 ---------------------------------------
void MainWindow::onMouseMove')

[IO.File]::WriteAllText($path, $u, (New-Object Text.UTF8Encoding($false)))
"resources page patched, length=$($u.Length)"
