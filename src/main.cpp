// ---------------------------------------------------------------------------
//  main.cpp  -  入口
// ---------------------------------------------------------------------------
#include "ui.h"
#include "util.h"

#include <windows.h>
#include <shellapi.h>
#include <string>

// 命令行里是否带某个参数（不区分大小写）
static bool hasArg(const wchar_t* name) {
    int argc = 0;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);
    bool found = false;
    if (argv) {
        for (int i = 1; i < argc; ++i) {
            if (_wcsicmp(argv[i], name) == 0) { found = true; break; }
        }
        LocalFree(argv);
    }
    return found;
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow) {
    // 关闭系统 DPI 虚拟化, 保证自绘清晰
    typedef BOOL(WINAPI* SetDpiAwareFn)(void);
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        SetDpiAwareFn fn = (SetDpiAwareFn)GetProcAddress(user32, "SetProcessDPIAware");
        if (fn) fn();
    }

    // ---- 自恢复模式: 被课堂管理软件删除后, 由登录自启的备份把主程序还原并启动 ----
    if (hasArg(L"--guard")) {
        std::wstring target;
        if (selfGuardRestore(target)) {
            // 用 CreateProcess 而不是 ShellExecute: 避免“打开文件-安全警告”弹窗打断还原
            std::wstring cmd = L"\"" + target + L"\"";
            std::wstring workDir = target;
            size_t p = workDir.find_last_of(L"\\/");
            if (p != std::wstring::npos) workDir = workDir.substr(0, p);
            STARTUPINFOW si{};
            si.cb = sizeof(si);
            PROCESS_INFORMATION pi{};
            if (CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, FALSE, 0,
                               nullptr, workDir.c_str(), &si, &pi)) {
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
            }
        }
        return 0;
    }

    // 单实例: 已有启动器时激活旧窗口并退出, 避免设置/备份并发写
    HANDLE instance = CreateMutexW(nullptr, TRUE, L"Local\\MCL_Launcher_SingleInstance");
    if (instance && GetLastError() == ERROR_ALREADY_EXISTS) {
        HWND existing = FindWindowW(L"MCLMainWindow", nullptr);
        if (existing) {
            ShowWindow(existing, SW_RESTORE);
            SetForegroundWindow(existing);
        }
        CloseHandle(instance);
        return 0;
    }
    int rc = runApp(hInst, nCmdShow);
    if (instance) {
        ReleaseMutex(instance);
        CloseHandle(instance);
    }
    return rc;
}
