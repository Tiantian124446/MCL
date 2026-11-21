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
    return runApp(hInst, nCmdShow);
}
