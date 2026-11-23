# build.ps1 — 构建 MCL 启动器
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Push-Location $root
try {
    # 资源（图标）：先编译 .rc 为 .o，再参与链接
    windres "src\mcl.rc" -O coff -o "build\mcl_res.o"
    if ($LASTEXITCODE -ne 0) { throw "windres failed: $LASTEXITCODE" }

    g++ -std=c++17 -O2 -municode -mwindows `
        -finput-charset=UTF-8 -fexec-charset=UTF-8 -fwide-exec-charset=UTF-16LE `
        -o "build\MCLauncher.exe" `
        "src\main.cpp" "src\ui.cpp" "src\core.cpp" "src\http.cpp" "src\util.cpp" `
        "build\mcl_res.o" `
        -lgdiplus -lwinhttp -lcrypt32 -lole32 -loleaut32 -luuid -lshlwapi -ldwmapi -lcomctl32 -lws2_32 -static
    if ($LASTEXITCODE -ne 0) { throw "build failed: $LASTEXITCODE" }
    try { Copy-Item "build\MCLauncher.exe" "MCLauncher.exe" -Force -ErrorAction Stop }
    catch { Write-Host "skip copy to root (file in use): $($_.Exception.Message)" }
    Write-Host "build ok: build\MCLauncher.exe"
} finally { Pop-Location }
