$ErrorActionPreference = 'Continue'
$out = 'C:\mcl_stop.out'
"stop requested $(Get-Date -Format 'HH:mm:ss')" | Out-File -FilePath $out -Encoding utf8
$n = 0
Get-CimInstance Win32_Process -Filter "Name='powershell.exe'" -ErrorAction SilentlyContinue |
    Where-Object { $_.CommandLine -like '*MCLServer\server.ps1*' } |
    ForEach-Object {
        try { Stop-Process -Id $_.ProcessId -Force; $n++; "stopped pid $($_.ProcessId)" | Out-File -FilePath $out -Append -Encoding utf8 } catch {}
    }
"stopped count = $n" | Out-File -FilePath $out -Append -Encoding utf8
