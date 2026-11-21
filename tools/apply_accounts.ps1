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

Rep '    BTN_NAV_SAVES,' @'
    BTN_NAV_SAVES,
    BTN_NAV_ACCOUNTS,
'@

Rep '    BTN_JAVA_SCAN          = 901007,' @'
    BTN_JAVA_SCAN          = 901007,
    BTN_ACCOUNT_ADD        = 901008,
    BTN_ACCOUNT_USE_BASE   = 903000,   // +列表下标: 使用账户
    BTN_ACCOUNT_DEL_BASE   = 904000,   // +列表下标: 删除账户
'@

Rep '    CTL_SRV_PORT' @'
    CTL_SRV_PORT,
    CTL_ACCOUNT
'@

Rep '    std::wstring playerName_ = L"Steve";' @'
    std::wstring playerName_ = L"Steve";
    std::vector<std::wstring> accounts_;
'@

Rep '    void drawSaves(Graphics& g, int W, int H);' @'
    void drawSaves(Graphics& g, int W, int H);
    void drawAccounts(Graphics& g, int W, int H);
'@

Rep '    void scanJavaVersions();             // 扫描常见 Java 安装' @'
    void scanJavaVersions();             // 扫描常见 Java 安装
    void syncAccountList();              // 确保当前昵称在账户列表
    void addAccount();                   // 添加离线账户
    void useAccount(int idx);            // 使用账户
    void deleteAccount(int idx);         // 删除账户
'@

Rep '    HWND editSrvPort_ = nullptr;' @'
    HWND editSrvPort_ = nullptr;
    HWND editAccount_ = nullptr;
'@

Rep '    editSrvPort_   = mkEdit(CTL_SRV_PORT);' @'
    editSrvPort_   = mkEdit(CTL_SRV_PORT);
    editAccount_   = mkEdit(CTL_ACCOUNT);
'@

Rep @'
    SetWindowTextW(editName_, playerName_.c_str());

    loadSettings();
    loadServers();
'@ @'
    loadSettings();
    if (accounts_.empty()) accounts_.push_back(playerName_);
    syncAccountList();
    SetWindowTextW(editName_, playerName_.c_str());
    loadServers();
'@

Rep '                           editModSearch_, editModMcVer_, editSrvName_, editSrvHost_, editSrvPort_ };' '                           editModSearch_, editModMcVer_, editSrvName_, editSrvHost_, editSrvPort_, editAccount_ };'
Rep '    hide(editSrvName_); hide(editSrvHost_); hide(editSrvPort_);' @'
    hide(editSrvName_); hide(editSrvHost_); hide(editSrvPort_); hide(editAccount_);
'@

Rep '        { L"存档管理", BTN_NAV_SAVES, 8 },' @'
        { L"存档管理", BTN_NAV_SAVES, 8 },
        { L"账户",     BTN_NAV_ACCOUNTS, 10 },
'@

Rep '        case 8: drawSaves(g, W, H); break;' @'
        case 8: drawSaves(g, W, H); break;
        case 10: drawAccounts(g, W, H); break;
'@

Rep '    case 8: return L"存档管理";' @'
    case 8: return L"存档管理";
    case 10: return L"账户";
'@

Rep @'
    } else if (page_ == 7) {
        int y = HEADER_H + 16;
        SetWindowPos(editModSearch_, nullptr, S(cx + 64), S(y + 60), S(cw - 500), S(28), SWP_NOZORDER);
        ShowWindow(editModSearch_, SW_SHOW);
'@ @'
    } else if (page_ == 7) {
        int y = HEADER_H + 16;
        SetWindowPos(editModSearch_, nullptr, S(cx + 64), S(y + 60), S(cw - 500), S(28), SWP_NOZORDER);
        ShowWindow(editModSearch_, SW_SHOW);
    } else if (page_ == 10) {
        int addTop = H - 108;
        SetWindowPos(editAccount_, nullptr, S(cx + 120), S(addTop + 28), S(cw - 300), S(30), SWP_NOZORDER);
        ShowWindow(editAccount_, SW_SHOW);
'@

Rep '    if (id == BTN_JAVA_SCAN) { scanJavaVersions(); return; }' @'
    if (id == BTN_ACCOUNT_ADD) { addAccount(); return; }
    if (id >= BTN_ACCOUNT_USE_BASE && id < BTN_ACCOUNT_DEL_BASE) {
        useAccount(id - BTN_ACCOUNT_USE_BASE);
        return;
    }
    if (id >= BTN_ACCOUNT_DEL_BASE && id < BTN_ACCOUNT_DEL_BASE + 1000) {
        deleteAccount(id - BTN_ACCOUNT_DEL_BASE);
        return;
    }
    if (id == BTN_JAVA_SCAN) { scanJavaVersions(); return; }
'@

Rep '    // ---- 模组管理页
    case BTN_NAV_MODMGR:' @'
    // ---- 账户页
    case BTN_NAV_ACCOUNTS:
        page_ = 10; layoutControls(); dirty_ = true; break;

    // ---- 模组管理页
    case BTN_NAV_MODMGR:
'@

Rep '            else if (k == "verCat")   { try { verCat_ = std::stoi(v); } catch (...) { verCat_ = 0; } }' @'
            else if (k == "verCat")   { try { verCat_ = std::stoi(v); } catch (...) { verCat_ = 0; } }
            else if (k == "accounts") {
                accounts_.clear();
                for (auto& n : splitW(utf8ToWide(v), L',')) {
                    std::wstring t = trimW(n);
                    if (!t.empty()) accounts_.push_back(t);
                }
            }
'@

Rep '    out += "verCat=" + std::to_string(verCat_) + "\r\n";' @'
    out += "verCat=" + std::to_string(verCat_) + "\r\n";
    out += "accounts=";
    for (size_t i = 0; i < accounts_.size(); ++i) {
        if (i) out += ",";
        out += wideToUtf8(accounts_[i]);
    }
    out += "\r\n";
'@

Rep 'void MainWindow::saveSettings() {' @'
void MainWindow::syncAccountList() {
    if (playerName_.empty()) playerName_ = L"Steve";
    for (auto& a : accounts_) if (a == playerName_) return;
    accounts_.push_back(playerName_);
}

void MainWindow::saveSettings() {
    syncAccountList();
'@

$fn = @'
void MainWindow::addAccount() {
    wchar_t buf[128] = { 0 };
    if (editAccount_) GetWindowTextW(editAccount_, buf, 128);
    std::wstring name = trimW(std::wstring(buf));
    if (name.empty()) {
        MessageBoxW(hwnd_, L"请输入账户昵称。", L"账户", MB_ICONINFORMATION);
        return;
    }
    if (name.find(L',') != std::wstring::npos) {
        MessageBoxW(hwnd_, L"昵称不能包含英文逗号。", L"账户", MB_ICONINFORMATION);
        return;
    }
    if (name.size() > 16) name = name.substr(0, 16);
    for (auto& a : accounts_) {
        if (a == name) {
            useAccount((int)(&a - accounts_.data()));
            return;
        }
    }
    accounts_.push_back(name);
    saveSettings();
    if (editAccount_) SetWindowTextW(editAccount_, L"");
    pushLog(L"已添加账户: " + name);
    dirty_ = true;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::useAccount(int idx) {
    if (idx < 0 || idx >= (int)accounts_.size()) return;
    playerName_ = accounts_[idx];
    if (editName_) SetWindowTextW(editName_, playerName_.c_str());
    saveSettings();
    applySettingsToLauncher();
    pushLog(L"已切换到账户: " + playerName_);
    dirty_ = true;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::deleteAccount(int idx) {
    if (idx < 0 || idx >= (int)accounts_.size()) return;
    if ((int)accounts_.size() <= 1) {
        MessageBoxW(hwnd_, L"至少保留一个账户。", L"账户", MB_ICONINFORMATION);
        return;
    }
    if (accounts_[idx] == playerName_) {
        MessageBoxW(hwnd_, L"不能删除当前正在使用的账户，请先切换到其他账户。", L"账户", MB_ICONWARNING);
        return;
    }
    std::wstring name = accounts_[idx];
    if (MessageBoxW(hwnd_, (L"确定删除账户 " + name + L" 吗？").c_str(), L"删除账户",
                    MB_ICONWARNING | MB_YESNO) != IDYES) {
        return;
    }
    accounts_.erase(accounts_.begin() + idx);
    saveSettings();
    pushLog(L"已删除账户: " + name);
    dirty_ = true;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::drawAccounts(Graphics& g, int W, int H) {
    FontFamily ff(L"Microsoft YaHei");
    Font fL(&ff, 12.0f, FontStyleRegular, UnitPixel);
    Font fB(&ff, 14.0f, FontStyleBold, UnitPixel);
    Font fD(&ff, 11.5f, FontStyleRegular, UnitPixel);

    int cx = SIDEBAR_W + 24;
    int cw = W - SIDEBAR_W - 48;
    int y = HEADER_H + 16;

    int addTop = H - 108;
    RECT list = mkRect(cx, y, cx + cw, addTop - 12);
    drawCard(g, list);
    RECT hd = mkRect(list.left + 16, list.top + 8, list.right - 16, list.top + 34);
    drawText(g, L"离线账户 · 共 " + std::to_wstring(accounts_.size()) + L" 个", fL, kTextDim, hd, 0);
    RECT hint = mkRect(list.left + 16, list.top + 30, list.right - 16, list.top + 52);
    drawText(g, L"「使用」切换当前昵称；昵称用于离线登录，不包含正版验证。", fD, kTextDim, hint, 0);

    int rowH = 48;
    int top = list.top + 58;
    int maxRows = (list.bottom - top - 8) / rowH;
    if (maxRows < 1) maxRows = 1;
    int show = (int)accounts_.size();
    if (show > maxRows) show = maxRows;
    for (int i = 0; i < show; ++i) {
        RECT row = mkRect(list.left + 12, top + i * rowH, list.right - 12, top + i * rowH + rowH - 6);
        if (accounts_[i] == playerName_) fillRound(g, row, 7, kSelBg);
        RECT rn = mkRect(row.left + 12, row.top, row.right - 220, row.bottom);
        drawText(g, accounts_[i], fB, kText, rn, 0);
        if (accounts_[i] == playerName_) {
            RECT rb = mkRect(row.right - 330, row.top + 10, row.right - 270, row.bottom - 8);
            drawText(g, L"当前", fD, kAccent, rb, 0);
        }
        RECT bu = mkRect(row.right - 260, row.top + 8, row.right - 180, row.bottom - 8);
        drawButton(g, bu, L"使用", BTN_ACCOUNT_USE_BASE + i, false, true, 11);
        RECT bd = mkRect(row.right - 170, row.top + 8, row.right - 90, row.bottom - 8);
        drawButton(g, bd, L"删除", BTN_ACCOUNT_DEL_BASE + i, false, accounts_[i] != playerName_, 11);
    }

    RECT add = mkRect(cx, addTop, cx + cw, H - 16);
    drawCard(g, add);
    RECT al = mkRect(add.left + 20, add.top, add.left + 110, add.bottom);
    drawText(g, L"添加账户", fL, kTextDim, al, 0);
    RECT ai = mkRect(add.left + 120, add.top + 28, add.right - 140, add.top + 58);
    fillRound(g, ai, 7, kInputBg);
    RECT ab = mkRect(add.right - 120, add.top + 28, add.right - 16, add.top + 58);
    drawButton(g, ab, L"添加", BTN_ACCOUNT_ADD, true, true, 12);
}

'@
Rep 'void MainWindow::drawSettings(Graphics& g, int W, int H) {' ($fn + 'void MainWindow::drawSettings(Graphics& g, int W, int H) {')

Rep 'static const wchar_t* kAppVersion = L"1.10.0";' 'static const wchar_t* kAppVersion = L"1.11.0";'

[IO.File]::WriteAllText($p, $u, (New-Object Text.UTF8Encoding($false)))
"accounts page patched, length=$($u.Length)"
