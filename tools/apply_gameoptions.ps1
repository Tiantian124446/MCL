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

Rep '    BTN_AUTO_BACKUP        = 901009,' @'
    BTN_AUTO_BACKUP        = 901009,
    BTN_GOPT_SAVE          = 901010,
    BTN_NAV_GOPT,
    BTN_GOPT_DEC_BASE      = 905000,   // +行: 数值减
    BTN_GOPT_INC_BASE      = 906000,   // +行: 数值加
    BTN_GOPT_TOGGLE_BASE   = 907000,   // +行: 切换
'@

Rep '    std::wstring gameDir_;' @'
    std::wstring gameDir_;
    std::map<std::string, std::string> gopt_;
    std::vector<std::string> goptOrder_;
'@

Rep '    void drawAccounts(Graphics& g, int W, int H);' @'
    void drawAccounts(Graphics& g, int W, int H);
    void drawGameOptions(Graphics& g, int W, int H);
'@

Rep '    void maybeAutoBackup();              // 启动时自动备份存档(每天一次)' @'
    void maybeAutoBackup();              // 启动时自动备份存档(每天一次)
    void loadGameOptions();              // 读取 options.txt
    void saveGameOptions();              // 写回 options.txt
'@

Rep '        { L"账户",     BTN_NAV_ACCOUNTS, 10 },' @'
        { L"账户",     BTN_NAV_ACCOUNTS, 10 },
        { L"游戏选项", BTN_NAV_GOPT, 11 },
'@

Rep '        case 10: drawAccounts(g, W, H); break;' @'
        case 10: drawAccounts(g, W, H); break;
        case 11: drawGameOptions(g, W, H); break;
'@

Rep '    case 10: return L"账户";' @'
    case 10: return L"账户";
    case 11: return L"游戏选项";
'@

Rep '    if (id == BTN_ACCOUNT_ADD) { addAccount(); return; }' @'
    if (id == BTN_NAV_GOPT) {
        page_ = 11; loadGameOptions(); layoutControls(); dirty_ = true; return;
    }
    if (id == BTN_GOPT_SAVE) { saveGameOptions(); return; }
    if (id >= BTN_GOPT_DEC_BASE && id < BTN_GOPT_INC_BASE) {
        int row = id - BTN_GOPT_DEC_BASE;
        const char* key = nullptr; int mn = 0, mx = 0, step = 1;
        if (goptNumeric(row, key, mn, mx, step)) {
            int v = 0;
            auto it = gopt_.find(key);
            if (it != gopt_.end()) { try { v = std::stoi(it->second); } catch (...) {} }
            v -= step; if (v < mn) v = mn;
            gopt_[key] = std::to_string(v);
            dirty_ = true; break;
        }
    }
    if (id >= BTN_GOPT_INC_BASE && id < BTN_GOPT_TOGGLE_BASE) {
        int row = id - BTN_GOPT_INC_BASE;
        const char* key = nullptr; int mn = 0, mx = 0, step = 1;
        if (goptNumeric(row, key, mn, mx, step)) {
            int v = 0;
            auto it = gopt_.find(key);
            if (it != gopt_.end()) { try { v = std::stoi(it->second); } catch (...) {} }
            v += step; if (v > mx) v = mx;
            gopt_[key] = std::to_string(v);
            dirty_ = true; break;
        }
    }
    if (id >= BTN_GOPT_TOGGLE_BASE && id < BTN_GOPT_TOGGLE_BASE + 100) {
        int row = id - BTN_GOPT_TOGGLE_BASE;
        if (row == 4) {
            gopt_["graphicsMode"] = (gopt_["graphicsMode"] == "1") ? "0" : "1";
        } else if (row == 5) {
            int v = 0; try { v = std::stoi(gopt_["particles"]); } catch (...) {}
            v = (v + 1) % 3;
            gopt_["particles"] = std::to_string(v);
        } else if (row == 6) {
            gopt_["renderClouds"] = (gopt_["renderClouds"] == "true") ? "false" : "true";
        } else if (row == 7) {
            gopt_["fullscreen"] = (gopt_["fullscreen"] == "true") ? "false" : "true";
        }
        dirty_ = true; break;
    }
    if (id == BTN_ACCOUNT_ADD) { addAccount(); return; }
'@

Rep '    // ---- 账户页
    case BTN_NAV_ACCOUNTS:' @'
    // ---- 游戏选项页
    case BTN_NAV_GOPT:
        page_ = 11; loadGameOptions(); layoutControls(); dirty_ = true; break;

    // ---- 账户页
    case BTN_NAV_ACCOUNTS:
'@

$fn = @'
static bool goptNumeric(int row, const char*& key, int& mn, int& mx, int& step) {
    switch (row) {
    case 0: key = "renderDistance";     mn = 2;  mx = 32;  step = 1;  return true;
    case 1: key = "simulationDistance"; mn = 5;  mx = 32;  step = 1;  return true;
    case 2: key = "maxFps";             mn = 30; mx = 260; step = 10; return true;
    case 3: key = "fov";                mn = 30; mx = 110; step = 5;  return true;
    default: return false;
    }
}

static std::string goptInt(const std::map<std::string, std::string>& m, const char* key, int defv) {
    auto it = m.find(key);
    if (it == m.end()) return std::to_string(defv);
    return it->second;
}

void MainWindow::loadGameOptions() {
    gopt_.clear();
    goptOrder_.clear();
    std::wstring path = joinPath(gameDir_, L"options.txt");
    FILE* fp = _wfopen(path.c_str(), L"rb");
    if (fp) {
        std::string data;
        char buf[4096];
        size_t n = 0;
        while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) data.append(buf, n);
        fclose(fp);
        size_t pos = 0;
        while (pos < data.size()) {
            size_t e = data.find('\n', pos);
            if (e == std::string::npos) e = data.size();
            std::string line = data.substr(pos, e - pos);
            pos = e + 1;
            if (!line.empty() && line.back() == '\r') line.pop_back();
            size_t c = line.find(':');
            if (c == std::string::npos) continue;
            std::string k = line.substr(0, c);
            std::string v = line.substr(c + 1);
            if (k.empty()) continue;
            gopt_[k] = v;
            goptOrder_.push_back(k);
        }
    }
    const char* defs[][2] = {
        { "renderDistance", "12" }, { "simulationDistance", "12" },
        { "maxFps", "120" }, { "fov", "70" },
        { "graphicsMode", "0" }, { "particles", "0" },
        { "renderClouds", "true" }, { "fullscreen", "false" }
    };
    for (auto& d : defs) {
        if (gopt_.find(d[0]) == gopt_.end()) {
            gopt_[d[0]] = d[1];
            goptOrder_.push_back(d[0]);
        }
    }
    pushLog(L"已读取 options.txt（" + std::to_wstring(gopt_.size()) + L" 项）");
    dirty_ = true;
}

void MainWindow::saveGameOptions() {
    std::wstring path = joinPath(gameDir_, L"options.txt");
    std::string out;
    std::set<std::string> written;
    for (auto& k : goptOrder_) {
        auto it = gopt_.find(k);
        if (it == gopt_.end()) continue;
        out += k + ":" + it->second + "\n";
        written.insert(k);
    }
    for (auto& kv : gopt_) {
        if (written.count(kv.first)) continue;
        out += kv.first + ":" + kv.second + "\n";
    }
    FILE* fp = _wfopen(path.c_str(), L"wb");
    if (!fp) {
        MessageBoxW(hwnd_, L"无法写入 options.txt，请检查游戏目录。", L"游戏选项", MB_ICONWARNING);
        return;
    }
    fwrite(out.data(), 1, out.size(), fp);
    fclose(fp);
    pushLog(L"已保存游戏选项: " + path);
    MessageBoxW(hwnd_, L"游戏选项已保存。\n部分选项需要重启游戏生效。", L"游戏选项", MB_ICONINFORMATION);
}

void MainWindow::drawGameOptions(Graphics& g, int W, int H) {
    FontFamily ff(L"Microsoft YaHei");
    Font fL(&ff, 12.5f, FontStyleRegular, UnitPixel);
    Font fV(&ff, 12.0f, FontStyleRegular, UnitPixel);
    Font fB(&ff, 14.0f, FontStyleBold, UnitPixel);

    int cx = SIDEBAR_W + 24;
    int cw = W - SIDEBAR_W - 48;
    int y = HEADER_H + 16;

    RECT hint = mkRect(cx, y, cx + cw, y + 24);
    drawText(g, L"修改的是游戏目录下的 options.txt；建议关闭游戏后再改，部分选项需重启生效。", fL, kTextDim, hint, 0);

    RECT card = mkRect(cx, y + 30, cx + cw, H - 80);
    drawCard(g, card);

    const wchar_t* labels[8] = {
        L"渲染距离", L"模拟距离", L"最大帧率", L"视场角",
        L"图形模式", L"粒子效果", L"云", L"全屏"
    };
    int rowH = 44;
    int top = card.top + 10;
    for (int i = 0; i < 8; ++i) {
        int ry = top + i * rowH;
        RECT rl = mkRect(card.left + 20, ry, card.right - 280, ry + rowH);
        drawText(g, labels[i], fL, kText, rl, 0);
        std::wstring value;
        if (i == 0) value = utf8ToWide(goptInt(gopt_, "renderDistance", 12));
        else if (i == 1) value = utf8ToWide(goptInt(gopt_, "simulationDistance", 12));
        else if (i == 2) value = utf8ToWide(goptInt(gopt_, "maxFps", 120));
        else if (i == 3) value = utf8ToWide(goptInt(gopt_, "fov", 70));
        else if (i == 4) value = (gopt_["graphicsMode"] == "1") ? L"高品质" : L"流畅";
        else if (i == 5) {
            int p = 0; try { p = std::stoi(gopt_["particles"]); } catch (...) {}
            value = (p == 0) ? L"全部" : (p == 1) ? L"减少" : L"最少";
        }
        else if (i == 6) value = (gopt_["renderClouds"] == "true") ? L"开" : L"关";
        else value = (gopt_["fullscreen"] == "true") ? L"开" : L"关";

        RECT rv = mkRect(card.right - 270, ry, card.right - 190, ry + rowH);
        drawText(g, value, fB, kAccent, rv, 2);
        if (i < 4) {
            RECT bd = mkRect(card.right - 180, ry + 7, card.right - 140, ry + 37);
            drawButton(g, bd, L"−", BTN_GOPT_DEC_BASE + i, false, true, 12);
            RECT bi = mkRect(card.right - 132, ry + 7, card.right - 92, ry + 37);
            drawButton(g, bi, L"+", BTN_GOPT_INC_BASE + i, false, true, 12);
        } else {
            RECT bt = mkRect(card.right - 180, ry + 7, card.right - 92, ry + 37);
            drawButton(g, bt, L"切换", BTN_GOPT_TOGGLE_BASE + i, false, true, 12);
        }
    }

    RECT bs = mkRect(cx + 16, H - 64, cx + 166, H - 20);
    drawButton(g, bs, L"保存游戏选项", BTN_GOPT_SAVE, true, true, 13);
    RECT bh = mkRect(cx + 180, H - 64, cx + cw, H - 20);
    drawText(g, L"提示：保存后会重写 options.txt，未知选项会原样保留。", fV, kTextDim, bh, 0);
}

'@
Rep 'void MainWindow::drawSettings(Graphics& g, int W, int H) {' ($fn + 'void MainWindow::drawSettings(Graphics& g, int W, int H) {')

Rep 'static const wchar_t* kAppVersion = L"1.13.0";' 'static const wchar_t* kAppVersion = L"1.14.0";'

[IO.File]::WriteAllText($p, $u, (New-Object Text.UTF8Encoding($false)))
"game options patched, length=$($u.Length)"
