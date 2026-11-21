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

Rep '    BTN_OPEN_GAMEDIR,' @'
    BTN_OPEN_GAMEDIR,
    BTN_ANALYZE_CRASH,
'@

Rep '    void openResourceFolder(int idx);    // 打开资源包/光影/存档等目录' @'
    void openResourceFolder(int idx);    // 打开资源包/光影/存档等目录
    void analyzeCrash();                 // 读取最新崩溃报告并给出建议
'@

Rep '    if (id == BTN_OPEN_GAMEDIR) {' @'
    if (id == BTN_ANALYZE_CRASH) {
        analyzeCrash();
        return;
    }
    if (id == BTN_OPEN_GAMEDIR) {
'@

Rep '        RECT rb = mkRect(c.left + 16, c.top + 68, c.left + 126, c.top + 98);
        drawButton(g, rb, L"打开文件夹", BTN_RES_OPEN_BASE + i, false, true, 12);' @'
        RECT rb = mkRect(c.left + 16, c.top + 68, c.left + 126, c.top + 98);
        drawButton(g, rb, L"打开文件夹", BTN_RES_OPEN_BASE + i, false, true, 12);
        if (wcscmp(kResEntries[i].dir, L"crash-reports") == 0) {
            RECT ra = mkRect(c.left + 136, c.top + 68, c.left + 246, c.top + 98);
            drawButton(g, ra, L"分析最新崩溃", BTN_ANALYZE_CRASH, true, true, 12);
        }
'@

$fn = @'
static bool newestFileInDir(const std::wstring& dir, const std::wstring& filter, std::wstring& out) {
    WIN32_FIND_DATAW fd;
    std::wstring pattern = dir + L"\\" + filter;
    HANDLE h = FindFirstFileW(pattern.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return false;
    bool found = false;
    FILETIME best{};
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        if (!found || CompareFileTime(&fd.ftLastWriteTime, &best) > 0) {
            best = fd.ftLastWriteTime;
            out = dir + L"\\" + fd.cFileName;
            found = true;
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    return found;
}

void MainWindow::analyzeCrash() {
    std::wstring crashDir = joinPath(gameDir_, L"crash-reports");
    std::wstring logsDir  = joinPath(gameDir_, L"logs");
    std::wstring file;
    bool fromCrash = newestFileInDir(crashDir, L"crash-*.txt", file);
    if (!fromCrash) fromCrash = newestFileInDir(crashDir, L"*.txt", file);
    if (!fromCrash) newestFileInDir(logsDir, L"latest.log", file);
    if (file.empty()) {
        MessageBoxW(hwnd_, L"没有找到崩溃报告（crash-reports）或 latest.log。\n\n先启动一次游戏，崩溃后再回来分析。",
                    L"MCL 崩溃分析", MB_ICONINFORMATION);
        return;
    }

    FILE* fp = _wfopen(file.c_str(), L"rb");
    if (!fp) {
        MessageBoxW(hwnd_, L"无法读取崩溃报告文件。", L"MCL 崩溃分析", MB_ICONWARNING);
        return;
    }
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (sz > 1024 * 1024) sz = 1024 * 1024;
    std::vector<char> buf((size_t)(sz > 0 ? sz : 0));
    size_t got = buf.empty() ? 0 : fread(buf.data(), 1, buf.size(), fp);
    fclose(fp);
    if (got == 0) {
        MessageBoxW(hwnd_, L"崩溃报告是空的。", L"MCL 崩溃分析", MB_ICONWARNING);
        return;
    }

    int wlen = MultiByteToWideChar(CP_UTF8, 0, buf.data(), (int)got, nullptr, 0);
    UINT cp = CP_UTF8;
    if (wlen <= 0) { cp = CP_ACP; wlen = MultiByteToWideChar(CP_ACP, 0, buf.data(), (int)got, nullptr, 0); }
    std::wstring text(wlen > 0 ? wlen : 0, L'\0');
    if (wlen > 0) MultiByteToWideChar(cp, 0, buf.data(), (int)got, &text[0], wlen);

    std::wstring low = text;
    for (auto& c : low) c = (wchar_t)towlower(c);

    std::wstring report;
    report += L"分析文件：\n" + file + L"\n\n";
    report += L"检测到的问题：\n";
    int hits = 0;
    auto has = [&](const wchar_t* p) { return low.find(p) != std::wstring::npos; };
    if (has(L"outofmemoryerror") || has(L"java heap space") || has(L"out of memory")) {
        report += L"· 内存不足（OutOfMemoryError）\n  建议：在首页把「最大内存」调高到 4096–8192 MB，并关闭占用内存的程序。\n";
        hits++;
    }
    if (has(L"unsupportedclassversionerror") || has(L"class file version") || has(L"unsupported major.minor")) {
        report += L"· Java 版本不匹配（UnsupportedClassVersionError）\n  建议：在「游戏设置」里换用与 MC 版本匹配的 Java（1.16 及以下用 Java 8，1.17+ 用 Java 17/21）。\n";
        hits++;
    }
    if (has(L"missing or unsupported mandatory dependencies") || has(L"requires") && has(L"mod file")) {
        report += L"· 模组缺少依赖或版本不匹配\n  建议：检查 mods 文件夹，补齐前置模组；或临时移出最近加入的模组再试。\n";
        hits++;
    }
    if (has(L"duplicate mods") || has(L"duplicate mod")) {
        report += L"· 模组重复\n  建议：删除 mods 中重复的 jar（常见于同时放了 Forge 与 Fabric 版本）。\n";
        hits++;
    }
    if (has(L"noclassdeffounderror") || has(L"nosuchmethoderror") || has(L"incompatible")) {
        report += L"· 模组/加载器版本不兼容（NoSuchMethod / NoClassDefFound）\n  建议：确认所有模组支持当前 MC 版本与加载器版本，优先移除最近新增的模组。\n";
        hits++;
    }
    if (has(L"glfw error") || has(L"opengl") || has(L"pixel format") || has(L"wglcreatecontext")) {
        report += L"· 显卡/OpenGL 初始化失败\n  建议：更新显卡驱动；关闭光影；在启动器里尝试关闭 3D 加速或降低渲染距离。\n";
        hits++;
    }
    if (has(L"the game crashed whilst") || has(L"exception in thread \"main\"")) {
        report += L"· 游戏主线程崩溃\n  建议：把本文件通过官网 BUG反馈 提交，附上完整崩溃报告以便定位。\n";
        hits++;
    }
    if (hits == 0) {
        report += L"· 未识别到常见错误特征\n  建议：把崩溃报告通过官网 BUG反馈 提交，或在「日志」文件夹查看 latest.log。\n";
    }
    report += L"\n是否打开官网提交 BUG 反馈？";

    pushLog(L"崩溃分析完成: " + fileNameOf(file));
    int r = MessageBoxW(hwnd_, report.c_str(), L"MCL 崩溃分析", MB_YESNO | MB_ICONINFORMATION);
    if (r == IDYES) {
        std::wstring u = std::wstring(kServerBase) + L"/?feedback=1";
        ShellExecuteW(nullptr, L"open", u.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
}

'@
$fn = $fn -replace "`r`n", "`n"
Rep '// ------------------------------ 资源管理页 ---------------------------------' ($fn + '// ------------------------------ 资源管理页 ---------------------------------')

[IO.File]::WriteAllText($path, $u, (New-Object Text.UTF8Encoding($false)))
"crash analysis patched, length=$($u.Length)"
