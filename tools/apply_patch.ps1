$ErrorActionPreference = 'Stop'
$path = 'E:\MCL\src\ui.cpp'
$u = ([IO.File]::ReadAllText($path, [Text.Encoding]::UTF8)) -replace "`r`n", "`n"
function Norm([string]$s) { return ($s -replace "`r`n", "`n") }
function Rep([string]$old, [string]$new) { $old = Norm $old; $new = Norm $new
    $script:count = ([regex]::Matches($script:u, [regex]::Escape($old))).Count
    if ($script:count -ne 1) { throw "replacement count $($script:count) for: $($old.Substring(0,[Math]::Min(60,$old.Length)))" }
    $script:u = $script:u.Replace($old, $new)
}

$old1 = 'static const float SCALE_MAX = 4.00f;'
$new1 = @'
static const float SCALE_MAX = 4.00f;

// 应用版本与发布服务器（Win10 服务器站点）
static const wchar_t* kAppVersion = L"1.1.0";
static const wchar_t* kServerBase = L"http://192.168.3.173:7891";
'@
Rep $old1 $new1

$old2 = '    BTN_FULLSCREEN,'
$new2 = @'
    BTN_FULLSCREEN,
    BTN_CHECK_UPDATE,
    BTN_OPEN_SITE,
'@
Rep $old2 $new2

$old3 = '    void applySettingsToLauncher();'
$new3 = @'
    void applySettingsToLauncher();
    void checkUpdateAsync();
'@
Rep $old3 $new3

$old4 = @'
    RECT bs = mkRect(cx, by, cx + 180, by + 46);
    drawButton(g, bs, L"开始游戏", BTN_START_GAME, true, !busy_.load(), 15);

    RECT bi = mkRect(cx + 190, by, cx + 350, by + 46);
    std::wstring instText = L"安装 / 更新版本";
    drawButton(g, bi, instText, BTN_INSTALL_SEL, false,
               !busy_.load() && !selVersion_.empty(), 13);

    RECT bc = mkRect(cx + 360, by, cx + 450, by + 46);
'@
$new4 = @'
    RECT bs = mkRect(cx, by, cx + 160, by + 46);
    drawButton(g, bs, L"开始游戏", BTN_START_GAME, true, !busy_.load(), 15);

    RECT bi = mkRect(cx + 170, by, cx + 320, by + 46);
    std::wstring instText = L"安装 / 更新版本";
    drawButton(g, bi, instText, BTN_INSTALL_SEL, false,
               !busy_.load() && !selVersion_.empty(), 13);

    RECT bu = mkRect(cx + 330, by, cx + 445, by + 46);
    drawButton(g, bu, L"检查更新", BTN_CHECK_UPDATE, false, !busy_.load(), 13);

    RECT bweb = mkRect(cx + 455, by, cx + 585, by + 46);
    drawButton(g, bweb, L"官网 / 反馈", BTN_OPEN_SITE, false, true, 13);

    RECT bc = mkRect(cx + 595, by, cx + 680, by + 46);
'@
Rep $old4 $new4

$old5 = '    drawText(g, L"MCL 启动器  v1.0.0", fB, kText, r1, 0);'
$new5 = '    drawText(g, std::wstring(L"MCL 启动器  v") + kAppVersion, fB, kText, r1, 0);'
Rep $old5 $new5

$old6 = @'
    int ly = c.top + 60;
    for (auto s : lines) {
        RECT r = mkRect(c.left + 24, ly, c.right - 24, ly + 22);
        drawText(g, s, fS, (std::wstring(s).empty() ? kTextDim : kText), r, 0);
        ly += 19;
    }
}
'@
$new6 = @'
    int ly = c.top + 60;
    for (auto s : lines) {
        RECT r = mkRect(c.left + 24, ly, c.right - 24, ly + 22);
        drawText(g, s, fS, (std::wstring(s).empty() ? kTextDim : kText), r, 0);
        ly += 19;
    }

    RECT b1 = mkRect(c.left + 24, c.top + 350, c.left + 144, c.top + 390);
    drawButton(g, b1, L"访问官网", BTN_OPEN_SITE, true, true, 13);
    RECT b2 = mkRect(c.left + 154, c.top + 350, c.left + 294, c.top + 390);
    drawButton(g, b2, L"提交 BUG 反馈", BTN_OPEN_SITE, false, true, 13);
}
'@
Rep $old6 $new6

$old7 = '    drawText(g, L"v1.0.0  ·  BMCLAPI", fs, kTextDim, br, 0);'
$new7 = '    drawText(g, std::wstring(L"v") + kAppVersion + L"  ·  BMCLAPI · Win10 服务器在线", fs, kTextDim, br, 0);'
Rep $old7 $new7

$old8 = @'
    case BTN_REFRESH:
        manifestLoaded_ = false;
'@
$new8 = @'
    case BTN_CHECK_UPDATE:
        checkUpdateAsync();
        break;

    case BTN_OPEN_SITE:
        ShellExecuteW(nullptr, L"open", kServerBase, nullptr, nullptr, SW_SHOWNORMAL);
        break;

    case BTN_REFRESH:
        manifestLoaded_ = false;
'@
Rep $old8 $new8

$old9 = @'
    SetTimer(hwnd_, 1, 120, nullptr);
    refreshManifestAsync();
'@
$new9 = @'
    SetTimer(hwnd_, 1, 120, nullptr);
    refreshManifestAsync();
    checkUpdateAsync();
'@
Rep $old9 $new9

$old10 = @'
// ------------------------------ 业务异步 -----------------------------------
void MainWindow::refreshManifestAsync() {
'@
$new10 = @'
// ------------------------------ 业务异步 -----------------------------------
void MainWindow::checkUpdateAsync() {
    pushLog(L"正在检查服务器上的最新版本...");
    std::thread([this]() {
        std::string body;
        std::wstring err;
        bool ok = httpGetToString(std::wstring(kServerBase) + L"/api/status", body, err);
        if (!ok) {
            pushLog(L"检查更新失败: " + err);
            setStatus(-1, L"服务器未连接，可稍后重试");
            return;
        }
        mjson::Value v;
        if (!mjson::parse(body, v) || !v.isObject() || !v.has("version")) {
            pushLog(L"检查更新失败: 服务器返回内容无法解析");
            return;
        }
        std::string sv = v["version"].asString();
        if (sv.empty()) {
            pushLog(L"检查更新失败: 服务器未返回版本号");
            return;
        }
        std::wstring remote = utf8ToWide(sv);
        std::wstring current = kAppVersion;
        if (remote == current) {
            pushLog(L"服务器在线，已是最新版本 v" + current + L"。");
            setStatus(-1, L"Win10 服务器在线 · 已是最新版本 v" + current);
        } else {
            pushLog(L"发现新版本 v" + remote + L"（当前 v" + current + L"），点击「官网 / 反馈」下载。");
            setStatus(-1, L"发现新版本 v" + remote + L"，点击「官网 / 反馈」下载");
        }
        dirty_ = true;
        if (hwnd_) InvalidateRect(hwnd_, nullptr, FALSE);
    }).detach();
}

void MainWindow::refreshManifestAsync() {
'@
Rep $old10 $new10

[IO.File]::WriteAllText($path, $u, (New-Object Text.UTF8Encoding($false)))
"ui.cpp patched, length=$($u.Length)"
