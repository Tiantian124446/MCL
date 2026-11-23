$log = 'C:\server_state.txt'
Start-Process powershell -ArgumentList '-NoProfile','-ExecutionPolicy','Bypass','-File','C:\MCLServer\server.ps1' -WindowStyle Hidden
Start-Sleep -Seconds 5
"time=$(Get-Date -Format o)" | Out-File $log -Encoding UTF8
"listen7891=$((Get-NetTCPConnection -LocalPort 7891 -State Listen -ErrorAction SilentlyContinue | Measure-Object).Count)" | Out-File $log -Append -Encoding UTF8
try { Get-ScheduledTask -TaskName MCLServer,MCLServerWatchdog -ErrorAction SilentlyContinue | Select-Object TaskName,State | Out-File $log -Append -Encoding UTF8 } catch {}
try { Get-Process powershell -ErrorAction SilentlyContinue | Select-Object Id,StartTime | Out-File $log -Append -Encoding UTF8 } catch {}
"done" | Out-File $log -Append -Encoding UTF8
