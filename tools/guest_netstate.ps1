$log = 'C:\netstate.txt'
"time=$(Get-Date -Format o)" | Out-File $log -Encoding UTF8
Get-NetAdapter -ErrorAction SilentlyContinue | Select-Object Name,Status,LinkSpeed,InterfaceDescription | Out-File $log -Append -Encoding UTF8
Get-NetIPAddress -AddressFamily IPv4 -ErrorAction SilentlyContinue | Select-Object IPAddress,PrefixLength,InterfaceAlias | Out-File $log -Append -Encoding UTF8
Get-NetRoute -AddressFamily IPv4 -ErrorAction SilentlyContinue | Where-Object DestinationPrefix -eq '0.0.0.0/0' | Select-Object NextHop,InterfaceAlias | Out-File $log -Append -Encoding UTF8
"gw=$(Test-Connection 192.168.3.1 -Count 2 -Quiet)" | Out-File $log -Append -Encoding UTF8
"host=$(Test-Connection 192.168.3.155 -Count 2 -Quiet)" | Out-File $log -Append -Encoding UTF8
"done" | Out-File $log -Append -Encoding UTF8
