# server_watchdog.ps1 — MCL 服务器自愈 + 端口转发（每 2 分钟运行）
#   1) 虚拟机没运行就启动（nogui）
#   2) 读取虚拟机 IP，写入 host\target.txt
#   3) 对齐 hosts（pocket4 / pocket4-MCL.com）与端口转发（0.0.0.0:7891 / 0.0.0.0:8080）
#   4) 放行防火墙 443 / 8443 / 7891 / 8080（所有配置文件）
#   5) 反向代理进程没了、或 443 没监听在当前 IP 上 → 重启
#   只改 hosts / portproxy / 防火墙 / 进程，不修改任何网卡配置。
$ErrorActionPreference = 'Continue'
$root = 'E:\MCL'
$vmrun = 'C:\Program Files (x86)\VMware\VMware Workstation\vmrun.exe'
$vmx = 'E:\文件\Virtual Machines\Windows 10 x64\Windows 10 x64.vmx'
$node = 'C:\nvm4w\nodejs\node.exe'
$log = Join-Path $root 'host\watchdog.log'
$hosts = Join-Path $env:WINDIR 'System32\drivers\etc\hosts'

function Log([string]$m) {
    $line = '[' + (Get-Date -Format 'yyyy-MM-dd HH:mm:ss') + '] ' + $m
    try { Add-Content -Path $log -Value $line -Encoding UTF8 } catch {}
}

# ---- 1) 虚拟机 ----
$vmList = ''
try { $vmList = (& $vmrun -T ws list 2>$null) -join "`n" } catch {}
if ($vmList -notmatch 'Windows 10 x64\.vmx') {
    Log 'VM not running, starting (nogui)'
    try { & $vmrun -T ws start $vmx nogui 2>&1 | Out-Null } catch {}
    Log 'VM start requested'
    exit 0
}

# ---- 2) 虚拟机 IP ----
$ip = ''
try { $ip = (& $vmrun -T ws getGuestIPAddress $vmx -wait 2>$null | Select-Object -Last 1) } catch {}
if (-not ($ip -match '^\d+\.\d+\.\d+\.\d+$')) { exit 0 }

$targetFile = Join-Path $root 'host\target.txt'
$oldTarget = ''
if (Test-Path $targetFile) { try { $oldTarget = ([System.IO.File]::ReadAllText($targetFile)).Trim() } catch {} }
if ($oldTarget -ne "$ip`:7891") {
    [System.IO.File]::WriteAllText($targetFile, "$ip`:7891", (New-Object System.Text.UTF8Encoding($false)))
    Log "guest target -> $ip`:7891"
}

# ---- 3) 主机 IP / hosts ----
$hostIp = (Get-NetIPAddress -AddressFamily IPv4 -InterfaceAlias 'WLAN' -ErrorAction SilentlyContinue |
           Select-Object -First 1).IPAddress
if ($hostIp) {
    $needHosts = $true
    try {
        $txt = [System.IO.File]::ReadAllText($hosts)
        if ($txt -match "(?m)^\s*$([regex]::Escape($hostIp))\s+pocket4(\s|$)") { $needHosts = $false }
    } catch {}
    if ($needHosts) {
        try {
            $lines = [System.IO.File]::ReadAllLines($hosts)
            $out = New-Object System.Collections.Generic.List[string]
            foreach ($l in $lines) { if ($l -notmatch '(?i)\bpocket4(-mcl\.com)?\b') { $out.Add($l) } }
            $out.Add('# MCL launcher server')
            $out.Add("$hostIp pocket4")
            $out.Add("$hostIp pocket4-MCL.com")
            $out.Add("$hostIp www.pocket4-MCL.com")
            [System.IO.File]::WriteAllLines($hosts, $out, (New-Object System.Text.ASCIIEncoding))
            Log "hosts updated -> $hostIp"
        } catch { Log "hosts update failed: $($_.Exception.Message)" }
    }
}

# ---- 4) 端口转发（0.0.0.0 覆盖所有网卡：WLAN / 校园网 / Radmin）----
function Ensure-PortProxy([int]$listenPort, [string]$targetIp, [int]$targetPort) {
    $show = ''
    try { $show = (netsh interface portproxy show v4tov4 | Out-String) } catch {}
    $ruleOk = $show -match ('0\.0\.0\.0\s+' + $listenPort + '\s+' + [regex]::Escape($targetIp) + '\s+' + $targetPort)
    $listening = Get-NetTCPConnection -State Listen -LocalPort $listenPort -ErrorAction SilentlyContinue
    if (-not $ruleOk -or -not $listening) {
        netsh interface portproxy delete v4tov4 listenaddress=0.0.0.0 listenport=$listenPort 2>$null | Out-Null
        netsh interface portproxy add v4tov4 listenaddress=0.0.0.0 listenport=$listenPort connectaddress=$targetIp connectport=$targetPort | Out-Null
        Log "portproxy 0.0.0.0:$listenPort -> $targetIp`:$targetPort"
    }
}
Ensure-PortProxy 7891 $ip 7891
Ensure-PortProxy 8080 $ip 7891

# 防火墙放行（所有配置文件）
foreach ($port in 443, 8443, 7891, 8080) {
    $ruleName = "MCL Server $port"
    $exists = Get-NetFirewallRule -DisplayName $ruleName -ErrorAction SilentlyContinue
    if (-not $exists) {
        New-NetFirewallRule -DisplayName $ruleName -Direction Inbound -Action Allow -Protocol TCP -LocalPort $port -Profile Any | Out-Null
        Log "firewall allow tcp/$port"
    }
}

# ---- 5) 反向代理进程 ----
$proxy = $null
try {
    $proxy = Get-CimInstance Win32_Process -Filter "Name='node.exe'" -ErrorAction SilentlyContinue |
             Where-Object { $_.CommandLine -like '*https_proxy.js*' }
} catch {}

$listenOk = $false
try {
    $conns = Get-NetTCPConnection -State Listen -LocalPort 443 -ErrorAction SilentlyContinue
    if ($hostIp) { $listenOk = [bool]($conns | Where-Object { $_.LocalAddress -eq $hostIp }) }
} catch {}
if ($proxy -and -not $listenOk) {
    Log 'proxy listener missing (ip changed?), restarting'
    $proxy | ForEach-Object { try { Stop-Process -Id $_.ProcessId -Force } catch {} }
    Start-Sleep -Milliseconds 600
    $proxy = $null
}

if (-not $proxy) {
    try {
        Start-Process -FilePath $node -ArgumentList "`"$root\host\https_proxy.js`"" -WindowStyle Hidden
        Log 'proxy started'
    } catch { Log "proxy start failed: $($_.Exception.Message)" }
}
