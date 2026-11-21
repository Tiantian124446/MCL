# MCL Server watchdog: keep port 7891 alive, rotate logs
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$log = Join-Path $Root 'logs\watchdog.log'
function Log($m) { Add-Content -Path $log -Value ('[{0}] {1}' -f (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'), $m) -Encoding UTF8 }
$ok = $false
try {
  $c = New-Object System.Net.Sockets.TcpClient
  $iar = $c.BeginConnect('127.0.0.1', 7891, $null, $null)
  if ($iar.AsyncWaitHandle.WaitOne(2000)) { $c.EndConnect($iar); $ok = $true }
  $c.Close()
} catch { $ok = $false }
if (-not $ok) {
  Log 'server not responding, restarting'
  Get-CimInstance Win32_Process -Filter "Name='powershell.exe'" | Where-Object { $_.CommandLine -like '*MCLServer\server.ps1*' } | ForEach-Object { try { Stop-Process -Id $_.ProcessId -Force } catch {} }
  Start-Process -FilePath 'powershell.exe' -ArgumentList ('-NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -File "' + $Root + '\server.ps1"') -WindowStyle Hidden
}
# rotate server log > 2MB
$sl = Join-Path $Root 'logs\server.log'
try {
  if ((Test-Path $sl) -and ((Get-Item $sl).Length -gt 2MB)) {
    Copy-Item $sl (Join-Path $Root 'logs\server.old.log') -Force
    Set-Content -Path $sl -Value '' -Encoding UTF8
  }
} catch {}
