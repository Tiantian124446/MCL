$log = 'C:\netfix.txt'
"start=$(Get-Date -Format o)" | Out-File $log -Encoding UTF8
ipconfig /release 2>&1 | Out-File $log -Append -Encoding UTF8
ipconfig /renew 2>&1 | Out-File $log -Append -Encoding UTF8
Start-Sleep -Seconds 6
Get-NetIPAddress -AddressFamily IPv4 -ErrorAction SilentlyContinue | Select-Object IPAddress,InterfaceAlias | Out-File $log -Append -Encoding UTF8
"ping155=$(Test-Connection 192.168.3.155 -Count 2 -Quiet)" | Out-File $log -Append -Encoding UTF8
"done" | Out-File $log -Append -Encoding UTF8
