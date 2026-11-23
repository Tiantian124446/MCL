# guest_install_oneclick.ps1 — 在 Win10(虚拟机) 桌面放一个“一键启动服务器”脚本
# 由主机通过 vmrun 复制并运行；本文件为 UTF-8(带 BOM)，中文文件名/内容可正常写入。
$ErrorActionPreference = 'Continue'
$log = 'C:\mcl_oneclick.log'
function L($m) { Add-Content -Path $log -Value ('[' + (Get-Date -Format 'yyyy-MM-dd HH:mm:ss') + '] ' + $m) -Encoding UTF8 }

$root = 'C:\MCLServer'
$desktop = [Environment]::GetFolderPath('Desktop')
L "desktop = $desktop"

# ---- 1) 真正干活的 PowerShell 脚本 ----
$ps1 = @'
# start_server.ps1 - 一键启动 MCL 服务器（桌面 bat 调用；-NoPause 供自动化测试）
$ErrorActionPreference = 'Continue'
$noPause = $args -contains '-NoPause'
$root = 'C:\MCLServer'
$logDir = Join-Path $root 'logs'
if (-not (Test-Path $logDir)) { New-Item -ItemType Directory -Path $logDir -Force | Out-Null }
$log = Join-Path $logDir 'start.log'
function L($m) { Add-Content -Path $log -Value ('[' + (Get-Date -Format 'yyyy-MM-dd HH:mm:ss') + '] ' + $m) -Encoding UTF8 }

function Test-Server {
    try {
        $r = Invoke-WebRequest 'http://127.0.0.1:7891/api/status' -UseBasicParsing -TimeoutSec 3
        return $true
    } catch { return $false }
}

Write-Host ''
Write-Host '  ============================================' -ForegroundColor Cyan
Write-Host '   MCL 服务器 一键启动' -ForegroundColor Cyan
Write-Host '  ============================================' -ForegroundColor Cyan
Write-Host ''

L 'one-click start requested'

if (Test-Server) {
    Write-Host '  服务器已经在运行。' -ForegroundColor Green
    L 'already running'
} else {
    Write-Host '  正在启动服务器...' -ForegroundColor Yellow
    Get-CimInstance Win32_Process -Filter "Name='powershell.exe'" -ErrorAction SilentlyContinue |
        Where-Object { $_.CommandLine -like '*MCLServer\server.ps1*' } |
        ForEach-Object { try { Stop-Process -Id $_.ProcessId -Force } catch {} }
    Start-Sleep -Milliseconds 800
    Start-Process -FilePath 'powershell.exe' `
        -ArgumentList '-NoProfile', '-WindowStyle', 'Hidden', '-ExecutionPolicy', 'Bypass', '-File', (Join-Path $root 'server.ps1') `
        -WindowStyle Hidden
    L 'server process started'
    for ($i = 0; $i -lt 10; $i++) {
        Start-Sleep -Seconds 1
        if (Test-Server) { break }
    }
}

# 顺带保活看护任务
try { schtasks /Run /TN "MCLServerWatchdog" 2>$null | Out-Null } catch {}

Write-Host ''
if (Test-Server) {
    $ip = '192.168.201.129'
    try {
        $cfg = Get-NetIPConfiguration -ErrorAction SilentlyContinue |
               Where-Object { $_.IPv4Address -and $_.NetProfile.Name -ne 'Loopback' } |
               Select-Object -First 1
        if ($cfg) { $ip = $cfg.IPv4Address.IPAddress }
    } catch {}
    Write-Host '  启动成功！服务器地址：' -ForegroundColor Green
    Write-Host ("    http://{0}:7891/" -f $ip) -ForegroundColor Green
    Write-Host '    （主机上访问：https://pocket4）' -ForegroundColor Gray
    L "ok address=$ip"
} else {
    Write-Host '  启动失败，请查看日志：' -ForegroundColor Red
    Write-Host '    C:\MCLServer\logs\server.log' -ForegroundColor Red
    L 'failed'
}
Write-Host ''
Write-Host '  按任意键关闭窗口...' -ForegroundColor DarkGray
if (-not $noPause) { $null = $Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown') }
'@
[System.IO.File]::WriteAllText((Join-Path $root 'start_server.ps1'), $ps1, (New-Object System.Text.UTF8Encoding($true)))
L 'start_server.ps1 written'

# ---- 2) 桌面 bat（纯 ASCII，避免编码问题）----
$bat = @'
@echo off
title MCL Server
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "C:\MCLServer\start_server.ps1"
'@
$batPath = Join-Path $desktop '一键启动服务器.bat'
[System.IO.File]::WriteAllText($batPath, $bat, [System.Text.Encoding]::ASCII)
L "desktop bat written: $batPath"
$oldBat = Join-Path $desktop 'MCL一键启动服务器.bat'
if (Test-Path $oldBat) { Remove-Item $oldBat -Force -ErrorAction SilentlyContinue; L 'removed old-named bat' }

# ---- 3) 顺便放一个 C:\MCLServer 里的副本，方便从探针工具调用 ----
[System.IO.File]::WriteAllText((Join-Path $root '一键启动服务器.bat'), $bat, [System.Text.Encoding]::ASCII)

L 'done'
