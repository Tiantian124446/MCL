# update.ps1 — 在 Win10 服务器上应用 C:\MCLServer\package.zip
$ErrorActionPreference = 'Stop'
$Root = 'C:\MCLServer'
$log = Join-Path $Root 'logs\update.log'
New-Item -ItemType Directory -Path (Join-Path $Root 'logs') -Force | Out-Null
function Log($m) { Add-Content -Path $log -Value ('[{0}] {1}' -f (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'), $m) -Encoding UTF8; Write-Host $m }
Log 'update start'
$zip = Join-Path $Root 'package.zip'
if (-not (Test-Path $zip)) { Log 'package.zip missing'; exit 1 }
# stop old server
Get-CimInstance Win32_Process -Filter "Name='powershell.exe'" | Where-Object { $_.CommandLine -like '*MCLServer\server.ps1*' } | ForEach-Object { try { Stop-Process -Id $_.ProcessId -Force } catch {} }
Start-Sleep -Milliseconds 500
Add-Type -AssemblyName System.IO.Compression.FileSystem
$tmp = Join-Path $env:TEMP ('mcl_update_' + [guid]::NewGuid().ToString('N'))
[System.IO.Compression.ZipFile]::ExtractToDirectory($zip, $tmp)
Copy-Item (Join-Path $tmp '*') $Root -Recurse -Force
Remove-Item $tmp -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item $zip -Force -ErrorAction SilentlyContinue
Log 'files updated'
# ensure firewall + tasks
netsh advfirewall firewall delete rule name="MCL Server 7891" | Out-Null
netsh advfirewall firewall add rule name="MCL Server 7891" dir=in action=allow protocol=TCP localport=7891 profile=any | Out-Null
$action = 'powershell.exe -NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -File "' + $Root + '\server.ps1"'
schtasks /Create /TN "MCLServer" /TR $action /SC ONLOGON /RL HIGHEST /F | Out-Null
schtasks /Create /TN "MCLServerWatchdog" /TR ('powershell.exe -NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -File "' + $Root + '\watchdog.ps1"') /SC MINUTE /MO 5 /RL HIGHEST /F | Out-Null
schtasks /Run /TN "MCLServer" | Out-Null
Start-Sleep -Seconds 3
try {
  $r = Invoke-WebRequest -Uri 'http://127.0.0.1:7891/api/status' -UseBasicParsing -TimeoutSec 8
  Log ('local status ' + $r.StatusCode + ' ' + $r.Content)
  Log 'update done'
} catch {
  Log ('local status failed: ' + $_.Exception.Message)
  Start-Process -FilePath 'powershell.exe' -ArgumentList ('-NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -File "' + $Root + '\server.ps1"') -WindowStyle Hidden
  Log 'started server directly'
}

# 桌面“一键启动服务器”脚本（每次更新都确保存在）
try {
  $oneclick = Join-Path $Root 'oneclick.ps1'
  if (Test-Path $oneclick) {
    & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $oneclick
    Log 'oneclick script installed'
  }
} catch { Log ('oneclick install failed: ' + $_.Exception.Message) }
