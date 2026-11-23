$out = 'C:\diag.txt'
"time=$(Get-Date -Format o)" | Out-File $out -Encoding UTF8
try { Get-PSDrive C | Format-List Name,Used,Free | Out-File $out -Append -Encoding UTF8 } catch { "disk_err=$($_.Exception.Message)" | Out-File $out -Append -Encoding UTF8 }
try { Get-Process MCLServer,powershell -ErrorAction SilentlyContinue | Select-Object Name,Id,StartTime | Out-File $out -Append -Encoding UTF8 } catch { "proc_err=$($_.Exception.Message)" | Out-File $out -Append -Encoding UTF8 }
try { Get-NetIPAddress -AddressFamily IPv4 -ErrorAction SilentlyContinue | Select-Object IPAddress,InterfaceAlias | Out-File $out -Append -Encoding UTF8 } catch { "ip_err=$($_.Exception.Message)" | Out-File $out -Append -Encoding UTF8 }
try { Test-NetConnection 192.168.3.155 -Port 8098 -InformationLevel Quiet | Out-File $out -Append -Encoding UTF8 } catch { "net_err=$($_.Exception.Message)" | Out-File $out -Append -Encoding UTF8 }
"done" | Out-File $out -Append -Encoding UTF8
