// ---------------------------------------------------------------------------
//  main.cpp  -  入口
// ---------------------------------------------------------------------------
#include "ui.h"

#include <windows.h>

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE, LPWSTR, int nCmdShow) {
    // 关闭系统 DPI 虚拟化, 保证自绘清晰
    typedef BOOL(WINAPI* SetDpiAwareFn)(void);
    HMODULE user32 = GetModuleHandleW(L"user32.dll");
    if (user32) {
        SetDpiAwareFn fn = (SetDpiAwareFn)GetProcAddress(user32, "SetProcessDPIAware");
        if (fn) fn();
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
