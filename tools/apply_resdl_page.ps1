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
    BTN_NAV_RESDL,
'@

Rep '    BTN_RES_WEB_BASE       = 908000,   // +资源卡片下标：Modrinth 搜索' @'
    BTN_RES_WEB_BASE       = 908000,   // +资源卡片下标：Modrinth 搜索
    BTN_RESDL_SEARCH       = 901018,
    BTN_RESDL_TYPE_PACK    = 901019,
    BTN_RESDL_TYPE_SHADER  = 901020,
    BTN_RESDL_OPEN         = 901021,
    BTN_RESDL_DL_BASE      = 909000,   // +结果下标：下载最新文件
'@

Rep '    std::vector<SaveEntry> saves_;' @'
    std::vector<SaveEntry> saves_;
    struct ResDlItem { std::string id; std::wstring title; std::wstring author; std::wstring desc; int downloads; };
    std::vector<ResDlItem> resDlItems_;
    int resDlType_ = 0;
    bool resDlLoading_ = false;
    std::wstring resDlErr_;
'@

Rep '    void handleDropFiles(HDROP h);       // 拖入 .jar / .zip 导入' @'
    void handleDropFiles(HDROP h);       // 拖入 .jar / .zip 导入
    void drawResDl(Graphics& g, int W, int H);
    void searchResDlAsync();
    void downloadResLatest(int idx);
'@

Rep '        { L"游戏选项", BTN_NAV_GOPT, 11 },' @'
        { L"游戏选项", BTN_NAV_GOPT, 11 },
        { L"资源下载", BTN_NAV_RESDL, 12 },
'@

Rep '        y += 46;' '        y += 42;'

Rep '        case 11: drawGameOptions(g, W, H); break;' @'
        case 11: drawGameOptions(g, W, H); break;
        case 12: drawResDl(g, W, H); break;
'@

Rep '    case 11: return L"游戏选项";' @'
    case 11: return L"游戏选项";
    case 12: return L"资源下载";
'@

Rep @'
    } else if (page_ == 10) {
        int addTop = H - 108;
        SetWindowPos(editAccount_, nullptr, S(cx + 120), S(addTop + 28), S(cw - 300), S(30), SWP_NOZORDER);
        ShowWindow(editAccount_, SW_SHOW);
'@ @'
    } else if (page_ == 12) {
        int y = HEADER_H + 16;
        SetWindowPos(editModSearch_, nullptr, S(cx + 96), S(y + 12), S(cw - 520), S(30), SWP_NOZORDER);
        ShowWindow(editModSearch_, SW_SHOW);
    } else if (page_ == 10) {
        int addTop = H - 108;
        SetWindowPos(editAccount_, nullptr, S(cx + 120), S(addTop + 28), S(cw - 300), S(30), SWP_NOZORDER);
        ShowWindow(editAccount_, SW_SHOW);
'@

Rep '    if (id == BTN_DOWNLOAD_LATEST) { downloadLatestAsync(); return; }' @'
    if (id == BTN_DOWNLOAD_LATEST) { downloadLatestAsync(); return; }
    if (id == BTN_RESDL_SEARCH) { searchResDlAsync(); return; }
    if (id == BTN_RESDL_TYPE_PACK || id == BTN_RESDL_TYPE_SHADER) {
        resDlType_ = (id == BTN_RESDL_TYPE_SHADER) ? 1 : 0;
        pushLog(resDlType_ == 0 ? L"资源类型：资源包" : L"资源类型：光影");
        dirty_ = true;
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    if (id == BTN_RESDL_OPEN) {
        std::wstring d = joinPath(activeGameDir(), resDlType_ == 0 ? L"resourcepacks" : L"shaderpacks");
        if (!pathExists(d)) createDirs(d);
        ShellExecuteW(nullptr, L"open", d.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        return;
    }
    if (id >= BTN_RESDL_DL_BASE && id < BTN_RESDL_DL_BASE + 1000) {
        downloadResLatest(id - BTN_RESDL_DL_BASE);
        return;
    }
'@

Rep '    // ---- 账户页
    case BTN_NAV_ACCOUNTS:' @'
    // ---- 资源下载页
    case BTN_NAV_RESDL:
        page_ = 12; layoutControls(); dirty_ = true; break;

    // ---- 账户页
    case BTN_NAV_ACCOUNTS:
'@

$fn = @'
void MainWindow::searchResDlAsync() {
    if (resDlLoading_) return;
    wchar_t buf[256] = { 0 };
    if (editModSearch_) GetWindowTextW(editModSearch_, buf, 256);
    std::wstring kw = trimW(std::wstring(buf));
    if (kw.empty()) {
        MessageBoxW(hwnd_, L"请先输入关键词，例如 faithful、shader。", L"资源下载", MB_ICONINFORMATION);
        return;
    }
    int type = resDlType_;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        resDlLoading_ = true;
        resDlItems_.clear();
        resDlErr_.clear();
    }
    modMgrScroll_ = 0;
    dirty_ = true;
    std::string kw8 = wideToUtf8(kw);
    std::thread([this, kw8, type]() {
        apiThrottle(300);
        std::string facets = (type == 0) ? "[[\"project_type:resourcepack\"]]" : "[[\"project_type:shader\"]]";
        std::wstring url = std::wstring(kModrinthApi) + L"search?query=" + utf8ToWide(urlEncode(kw8)) +
                           L"&limit=20&index=downloads&facets=" + utf8ToWide(urlEncodeAll(facets));
        std::string body;
        std::wstring err;
        bool ok = httpGetToStringEx(url, kBrowserUA, body, err);
        std::vector<ResDlItem> items;
        std::wstring fail;
        if (!ok) {
            fail = L"Modrinth 搜索失败：" + err;
        } else {
            Json j;
            if (!mjson::parse(body, j) || !j.isObject()) {
                fail = L"Modrinth 返回内容解析失败";
            } else {
                const Json& hits = j.get("hits");
                if (!hits.isArray()) fail = L"Modrinth 返回结果格式异常";
                else {
                    for (size_t i = 0; i < hits.size() && items.size() < 20; ++i) {
                        const Json& h = hits.at(i);
                        ResDlItem it;
                        it.id = getStr(h, "project_id");
                        if (it.id.empty()) continue;
                        it.title = utf8ToWide(getStr(h, "title"));
                        it.author = utf8ToWide(getStr(h, "author"));
                        it.desc = oneLine(utf8ToWide(getStr(h, "description")), 140);
                        it.downloads = (int)h.get("downloads").asNumber(0);
                        items.push_back(it);
                    }
                    if (items.empty()) fail = L"没有找到匹配的资源";
                }
            }
        }
        {
            std::lock_guard<std::mutex> lk(mtx_);
            resDlItems_.swap(items);
            resDlErr_ = fail;
            resDlLoading_ = false;
        }
        dirty_ = true;
        if (hwnd_) PostMessageW(hwnd_, WM_TIMER, 0, 0);
    }).detach();
}

void MainWindow::downloadResLatest(int idx) {
    if (dlActive_.load()) { pushLog(L"已有下载任务在进行。"); return; }
    std::vector<ResDlItem> items;
    int type = resDlType_;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        items = resDlItems_;
    }
    if (idx < 0 || idx >= (int)items.size()) return;
    ResDlItem item = items[idx];
    std::wstring targetDir = joinPath(activeGameDir(), type == 0 ? L"resourcepacks" : L"shaderpacks");
    createDirs(targetDir);
    dlActive_ = true;
    dlPct_ = 0;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        dlName_ = item.title;
        dlResult_ = L"正在获取最新版本…";
    }
    dirty_ = true;
    std::string id = item.id;
    std::wstring title = item.title;
    std::thread([this, id, title, targetDir]() {
        apiThrottle(300);
        std::wstring url = std::wstring(kModrinthApi) + L"project/" + utf8ToWide(id) + L"/version?limit=1";
        std::string body;
        std::wstring err;
        bool ok = httpGetToStringEx(url, kBrowserUA, body, err);
        std::wstring fileUrl, fileName;
        unsigned long long fileSize = 0;
        if (ok) {
            Json j;
            if (mjson::parse(body, j) && j.isArray() && j.size() > 0) {
                const Json& fs = j.at(0).get("files");
                if (fs.isArray() && fs.size() > 0) {
                    int pick = 0;
                    for (size_t k = 0; k < fs.size(); ++k) {
                        if (fs.at(k).get("primary").asBool(false)) { pick = (int)k; break; }
                    }
                    fileName = utf8ToWide(getStr(fs.at(pick), "filename"));
                    fileUrl = utf8ToWide(getStr(fs.at(pick), "url"));
                    fileSize = (unsigned long long)fs.at(pick).get("size").asNumber(0);
                }
            }
        }
        if (fileUrl.empty()) {
            {
                std::lock_guard<std::mutex> lk(mtx_);
                dlResult_ = L"获取文件失败：" + (err.empty() ? L"没有可用文件" : err);
            }
            dlActive_ = false;
            dirty_ = true;
            if (hwnd_) PostMessageW(hwnd_, WM_TIMER, 0, 0);
            return;
        }
        std::wstring file = joinPath(targetDir, safeName(fileName.empty() ? L"download.bin" : fileName));
        {
            std::lock_guard<std::mutex> lk(mtx_);
            dlResult_ = L"正在下载 " + fileName + L"（" + humanSize(fileSize) + L"）…";
        }
        bool dlOk = httpDownloadToFileEx(fileUrl, file, kBrowserUA,
            [this](unsigned long long done, unsigned long long total) {
                if (total > 0) dlPct_ = (int)(done * 100 / total);
                return true;
            }, err);
        dlPct_ = dlOk ? 100 : dlPct_.load();
        {
            std::lock_guard<std::mutex> lk(mtx_);
            dlResult_ = dlOk ? (L"已下载到 " + targetDir + L"：" + fileName) : (L"下载失败：" + err);
        }
        if (dlOk) pushLog(L"资源下载完成: " + title + L" -> " + file);
        dlActive_ = false;
        dirty_ = true;
        if (hwnd_) PostMessageW(hwnd_, WM_TIMER, 0, 0);
    }).detach();
}

void MainWindow::drawResDl(Graphics& g, int W, int H) {
    FontFamily ff(L"Microsoft YaHei");
    Font fL(&ff, 12.0f, FontStyleRegular, UnitPixel);
    Font fB(&ff, 14.0f, FontStyleBold, UnitPixel);
    Font fD(&ff, 11.5f, FontStyleRegular, UnitPixel);
    Font fH(&ff, 11.0f, FontStyleRegular, UnitPixel);

    int cx = SIDEBAR_W + 24;
    int cw = W - SIDEBAR_W - 48;
    int y = HEADER_H + 16;

    RECT sc = mkRect(cx, y, cx + cw, y + 64);
    drawCard(g, sc);
    RECT lab = mkRect(sc.left + 16, sc.top + 12, sc.left + 90, sc.top + 42);
    drawText(g, L"关键词", fL, kTextDim, lab, 0);
    RECT ib = mkRect(sc.left + 96, sc.top + 12, sc.right - 520, sc.top + 42);
    fillRound(g, ib, 7, kInputBg);
    RECT bp = mkRect(sc.right - 400, sc.top + 16, sc.right - 310, sc.top + 46);
    drawButton(g, bp, L"资源包", BTN_RESDL_TYPE_PACK, resDlType_ == 0, true, 12);
    RECT bs = mkRect(sc.right - 302, sc.top + 16, sc.right - 212, sc.top + 46);
    drawButton(g, bs, L"光影", BTN_RESDL_TYPE_SHADER, resDlType_ == 1, true, 12);
    RECT bq = mkRect(sc.right - 204, sc.top + 16, sc.right - 114, sc.top + 46);
    drawButton(g, bq, L"搜索", BTN_RESDL_SEARCH, true, !resDlLoading_, 12);
    RECT bo = mkRect(sc.right - 106, sc.top + 16, sc.right - 16, sc.top + 46);
    drawButton(g, bo, L"打开文件夹", BTN_RESDL_OPEN, false, true, 12);

    std::vector<ResDlItem> items;
    std::wstring err;
    bool loading = false;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        items = resDlItems_;
        err = resDlErr_;
        loading = resDlLoading_;
    }
    int listTop = y + 76;
    int listBottom = H - 16;
    int rowH = 68, headerH = 30;
    int rows = (listBottom - listTop - headerH - 8) / rowH;
    if (rows < 1) rows = 1;
    int total = (int)items.size();
    int maxScroll = total > rows ? total - rows : 0;
    if (modMgrScroll_ > maxScroll) modMgrScroll_ = maxScroll;
    if (modMgrScroll_ < 0) modMgrScroll_ = 0;
    RECT list = mkRect(cx, listTop, cx + cw, listBottom);
    drawCard(g, list);
    std::wstring head = L"搜索结果";
    if (loading) head += L"  ·  正在搜索…"; else head += L"  ·  共 " + std::to_wstring(total) + L" 个";
    head += (resDlType_ == 0) ? L"  ·  资源包" : L"  ·  光影";
    RECT hdr = mkRect(list.left + 12, list.top + 6, list.right - 240, list.top + 36);
    drawText(g, head, fH, kTextDim, hdr, 0);
    if (dlActive_.load()) {
        std::wstring dr;
        { std::lock_guard<std::mutex> lk(mtx_); dr = dlResult_; }
        RECT st = mkRect(list.right - 300, list.top + 6, list.right - 12, list.top + 36);
        drawText(g, dr + L"  " + std::to_wstring(dlPct_.load()) + L"%", fH, kAccent, st, 2);
    }
    if (total == 0) {
        RECT e = mkRect(list.left + 12, list.top + headerH + 20, list.right - 12, list.top + headerH + 56);
        std::wstring msg = loading ? L"正在从 Modrinth 搜索…" : (err.empty() ? L"输入关键词后点「搜索」。" : err);
        drawText(g, msg, fL, kTextDim, e, 0);
        return;
    }
    int ry = list.top + 6 + headerH;
    for (int i = 0; i < rows; ++i) {
        int idx = modMgrScroll_ + i;
        if (idx >= total) break;
        const ResDlItem& it = items[idx];
        RECT row = mkRect(list.left + 8, ry, list.right - 8, ry + rowH - 6);
        if ((idx & 1) == 0) fillRound(g, row, 6, C(250, 251, 253));
        RECT rt = mkRect(row.left + 10, row.top + 4, row.right - 130, row.top + 26);
        drawText(g, it.title, fB, kText, rt, 0);
        std::wstring sub = it.author + L"  ·  下载 " + std::to_wstring(it.downloads);
        RECT rs = mkRect(row.left + 10, row.top + 26, row.right - 130, row.top + 44);
        drawText(g, sub, fD, kTextDim, rs, 0);
        RECT rd = mkRect(row.left + 10, row.top + 44, row.right - 130, row.top + 62);
        drawText(g, it.desc, fD, kTextDim, rd, 0);
        RECT bd = mkRect(row.right - 120, row.top + 18, row.right - 10, row.top + 50);
        drawButton(g, bd, L"下载最新", BTN_RESDL_DL_BASE + idx, true, !dlActive_.load(), 12);
        ry += rowH;
    }
}

'@
Rep 'void MainWindow::handleDropFiles(HDROP h) {' ($fn + 'void MainWindow::handleDropFiles(HDROP h) {')

Rep 'static const wchar_t* kAppVersion = L"1.26.1";' 'static const wchar_t* kAppVersion = L"1.27.0";'

[IO.File]::WriteAllText($p, $u, (New-Object Text.UTF8Encoding($false)))
"resource download page patched, length=$($u.Length)"
