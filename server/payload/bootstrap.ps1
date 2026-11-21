# MCL Server bootstrap for Windows 10 guest (run elevated)
param(
  [string]$Stage = 'http://192.168.3.155:8099',
  [string]$Root = 'C:\MCLServer'
)
$ErrorActionPreference = 'Stop'
$log = Join-Path $env:TEMP 'mcl_bootstrap.log'
function Log($m) { $line = ('[{0}] {1}' -f (Get-Date -Format 'HH:mm:ss'), $m); Add-Content -Path $log -Value $line -Encoding UTF8; Write-Host $line }
Log 'bootstrap start'
New-Item -ItemType Directory -Path $Root -Force | Out-Null
$zip = Join-Path $env:TEMP 'mcl_package.zip'
Log ('downloading ' + $Stage + '/package.zip')
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12
$wc = New-Object System.Net.WebClient
$wc.DownloadFile(($Stage + '/package.zip'), $zip)
Log ('downloaded ' + (Get-Item $zip).Length + ' bytes')
Add-Type -AssemblyName System.IO.Compression.FileSystem
$tmp = Join-Path $env:TEMP ('mcl_unzip_' + [guid]::NewGuid().ToString('N'))
[System.IO.Compression.ZipFile]::ExtractToDirectory($zip, $tmp)
Log 'extracted'
Copy-Item (Join-Path $tmp '*') $Root -Recurse -Force
Remove-Item $tmp -Recurse -Force -ErrorAction SilentlyContinue
Remove-Item $zip -Force -ErrorAction SilentlyContinue
Log 'files copied'
# firewall rule
try {
  netsh advfirewall firewall delete rule name="MCL Server 7891" | Out-Null
} catch {}
netsh advfirewall firewall add rule name="MCL Server 7891" dir=in action=allow protocol=TCP localport=7891 profile=any | Out-Null
Log 'firewall rule added'
# stop old server
Get-CimInstance Win32_Process -Filter "Name='powershell.exe'" | Where-Object { $_.CommandLine -like '*MCLServer\server.ps1*' } | ForEach-Object { try { Stop-Process -Id $_.ProcessId -Force } catch {} }
# scheduled task at logon, highest privileges
$action = 'powershell.exe -NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -File "' + $Root + '\server.ps1"'
schtasks /Create /TN "MCLServer" /TR $action /SC ONLOGON /RL HIGHEST /F | Out-Null
schtasks /Create /TN "MCLServerWatchdog" /TR ('powershell.exe -NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -File "' + $Root + '\watchdog.ps1"') /SC MINUTE /MO 5 /RL HIGHEST /F | Out-Null
Log 'scheduled tasks created'
# start now
schtasks /Run /TN "MCLServer" | Out-Null
Start-Sleep -Seconds 3
try {
  $r = Invoke-WebRequest -Uri 'http://127.0.0.1:7891/api/status' -UseBasicParsing -TimeoutSec 8
  Log ('local status ' + $r.StatusCode + ' ' + $r.Content)
} catch {
  Log ('local status failed: ' + $_.Exception.Message)
  Start-Process -FilePath 'powershell.exe' -ArgumentList ('-NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -File "' + $Root + '\server.ps1"') -WindowStyle Hidden
}
Log 'bootstrap done'
