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

Rep 'static const float SCALE_MAX = 4.00f;' @'
static const float SCALE_MAX = 4.00f;

// 应用版本与发布服务器（Win10 服务器站点）
static const wchar_t* kAppVersion = L"1.1.0";
static const wchar_t* kServerBase = L"http://192.168.3.173:7891";
'@

Rep '    BTN_FULLSCREEN,' @'
    BTN_FULLSCREEN,
    BTN_CHECK_UPDATE,
    BTN_OPEN_SITE,
'@

Rep '    void applySettingsToLauncher();' @'
    void applySettingsToLauncher();
    void checkUpdateAsync();
'@

Rep '    RECT bs = mkRect(cx, by, cx + 180, by + 46);' '    RECT bs = mkRect(cx, by, cx + 160, by + 46);'
Rep '    RECT bi = mkRect(cx + 190, by, cx + 350, by + 46);' '    RECT bi = mkRect(cx + 170, by, cx + 320, by + 46);'
Rep '    RECT bc = mkRect(cx + 360, by, cx + 450, by + 46);' @'
    RECT bu = mkRect(cx + 330, by, cx + 445, by + 46);
    drawButton(g, bu, L"检查更新", BTN_CHECK_UPDATE, false, !busy_.load(), 13);

    RECT bweb = mkRect(cx + 455, by, cx + 585, by + 46);
    drawButton(g, bweb, L"官网 / 反馈", BTN_OPEN_SITE, false, true, 13);

    RECT bc = mkRect(cx + 595, by, cx + 680, by + 46);
'@

Rep '    drawText(g, L"MCL 启动器  v1.0.0", fB, kText, r1, 0);' '    drawText(g, std::wstring(L"MCL 启动器  v") + kAppVersion, fB, kText, r1, 0);'

Rep '    drawText(g, L"v1.0.0  ·  BMCLAPI", fs, kTextDim, br, 0);' '    drawText(g, std::wstring(L"v") + kAppVersion + L"  ·  BMCLAPI · Win10 服务器", fs, kTextDim, br, 0);'

Rep @'
        ly += 19;
    }
}
'@ @'
        ly += 19;
    }

    RECT b1 = mkRect(c.left + 24, c.top + 350, c.left + 144, c.top + 390);
    drawButton(g, b1, L"访问官网", BTN_OPEN_SITE, true, true, 13);
    RECT b2 = mkRect(c.left + 154, c.top + 350, c.left + 294, c.top + 390);
    drawButton(g, b2, L"提交 BUG 反馈", BTN_OPEN_SITE, false, true, 13);
}
'@

Rep '    case BTN_REFRESH:' @'
    case BTN_CHECK_UPDATE:
        checkUpdateAsync();
        break;

    case BTN_OPEN_SITE:
        ShellExecuteW(nullptr, L"open", kServerBase, nullptr, nullptr, SW_SHOWNORMAL);
        break;

    case BTN_REFRESH:
'@

Rep @'
    refreshManifestAsync();
    return true;
'@ @'
    refreshManifestAsync();
    checkUpdateAsync();
    return true;
'@

Rep 'void MainWindow::refreshManifestAsync() {' @'
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

[IO.File]::WriteAllText($path, $u, (New-Object Text.UTF8Encoding($false)))
"ui.cpp patched, length=$($u.Length)"
