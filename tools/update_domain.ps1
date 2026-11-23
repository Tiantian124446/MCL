# update_domain.ps1 — 对齐 pocket4-MCL.com 的本地解析与端口转发
#   * 只改 hosts / portproxy / 代理进程，绝不修改任何网卡（WLAN 保持 DHCP）
#   * 主机局域网 IP 或虚拟机 IP 变化后，重新运行本脚本即可
param(
    [string]$Vmx = 'E:\文件\Virtual Machines\Windows 10 x64\Windows 10 x64.vmx',
    [string]$Vmrun = 'C:\Program Files (x86)\VMware\VMware Workstation\vmrun.exe',
    [int]$ServerPort = 7891
)
$ErrorActionPreference = 'Stop'
$root = 'E:\MCL'

# 1) 主机局域网地址（WLAN 的 DHCP 地址）
$hostIp = (Get-NetIPAddress -AddressFamily IPv4 -InterfaceAlias 'WLAN' -ErrorAction SilentlyContinue |
           Select-Object -First 1).IPAddress
if (-not $hostIp) { throw '未找到 WLAN 的 IPv4 地址' }
Write-Host "host ip = $hostIp"

# 2) 虚拟机地址（NAT）
$guestIp = (& $Vmrun -T ws getGuestIPAddress $Vmx -wait 2>$null | Select-Object -Last 1)
if (-not ($guestIp -match '^\d+\.\d+\.\d+\.\d+$')) { throw "无法获取虚拟机 IP: $guestIp" }
Write-Host "guest ip = $guestIp"

# 3) 代理后端
$targetFile = Join-Path $root 'host\target.txt'
[System.IO.File]::WriteAllText($targetFile, "$guestIp`:$ServerPort", (New-Object System.Text.UTF8Encoding($false)))
Write-Host "target -> $guestIp`:$ServerPort"

# 4) hosts: pocket4-MCL.com -> 主机局域网地址
$hosts = Join-Path $env:WINDIR 'System32\drivers\etc\hosts'
if (-not (Test-Path "$hosts.bak_mcl")) { Copy-Item $hosts "$hosts.bak_mcl" -Force -ErrorAction SilentlyContinue }
$lines = [System.IO.File]::ReadAllLines($hosts)
$out = New-Object System.Collections.Generic.List[string]
foreach ($l in $lines) {
    if ($l -match '(?i)pocket4-mcl\.com') { continue }
    $out.Add($l)
}
$out.Add("# MCL launcher server")
$out.Add("$hostIp pocket4-MCL.com")
$out.Add("$hostIp www.pocket4-MCL.com")
[System.IO.File]::WriteAllLines($hosts, $out, (New-Object System.Text.ASCIIEncoding))
Write-Host 'hosts updated'

# 5) portproxy: 主机:7891 -> 虚拟机:7891（局域网回退地址）
netsh interface portproxy delete v4tov4 listenaddress=$hostIp listenport=$ServerPort 2>$null | Out-Null
netsh interface portproxy add v4tov4 listenaddress=$hostIp listenport=$ServerPort connectaddress=$guestIp connectport=$ServerPort | Out-Null
Write-Host 'portproxy updated'

# 6) 重启反向代理（有变化才重启）
$node = 'C:\nvm4w\nodejs\node.exe'
$proxy = Join-Path $root 'host\https_proxy.js'
Get-CimInstance Win32_Process -Filter "Name='node.exe'" |
    Where-Object { $_.CommandLine -like '*https_proxy.js*' } |
    ForEach-Object { try { Stop-Process -Id $_.ProcessId -Force } catch {} }
Start-Sleep -Milliseconds 400
Start-Process -FilePath $node -ArgumentList "`"$proxy`"" -WindowStyle Hidden
Start-Sleep -Seconds 2

# 7) 自检
try {
    $r = Invoke-WebRequest "https://pocket4-MCL.com/api/status" -UseBasicParsing -TimeoutSec 8
    $j = $r.Content | ConvertFrom-Json
    Write-Host ("OK: https://pocket4-MCL.com -> v{0} ({1})" -f $j.version, $j.address)
} catch {
    Write-Host ("WARN: https 自检失败: " + $_.Exception.Message)
}
try {
    $r2 = Invoke-WebRequest "http://$hostIp`:$ServerPort/api/status" -UseBasicParsing -TimeoutSec 8
    Write-Host ("OK: http://$hostIp`:$ServerPort reachable")
} catch {
    Write-Host ("WARN: 局域网回退地址自检失败: " + $_.Exception.Message)
}
