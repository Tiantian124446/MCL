# server_watchdog.ps1 — MCL 服务器自愈（每 2 分钟运行）
#   1) 虚拟机没运行就启动（nogui）
#   2) 读取虚拟机 IP，写入 host\target.txt
#   3) 对齐 hosts（pocket4 / pocket4-MCL.com）与 7891 端口转发
#   4) 反向代理进程没了就重启
#   只改 hosts / portproxy / 定时任务 / 进程，不修改任何网卡配置。
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

# ---- 3) 主机 IP / hosts / portproxy ----
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

    # portproxy（0.0.0.0;7891 由 iphlpsvc 监听）
    $rule = netsh interface portproxy show v4tov4 | Select-String -SimpleMatch "$hostIp" | Select-String -SimpleMatch '7891'
    $ruleOk = $false
    if ($rule) {
        $ruleText = ($rule | Out-String)
        if ($ruleText -match [regex]::Escape("$hostIp") + '\s+7891\s+' + [regex]::Escape("$ip")) { $ruleOk = $true }
    }
    if (-not $ruleOk) {
        netsh interface portproxy delete v4tov4 listenaddress=$hostIp listenport=7891 2>$null | Out-Null
        netsh interface portproxy add v4tov4 listenaddress=$hostIp listenport=7891 connectaddress=$ip connectport=7891 | Out-Null
        Log "portproxy -> $ip`:7891"
    }
    # 监听丢失时重新注册一次
    $listening = Get-NetTCPConnection -State Listen -LocalPort 7891 -ErrorAction SilentlyContinue
    if (-not $listening) {
        netsh interface portproxy delete v4tov4 listenaddress=$hostIp listenport=7891 2>$null | Out-Null
        netsh interface portproxy add v4tov4 listenaddress=$hostIp listenport=7891 connectaddress=$ip connectport=7891 | Out-Null
        Log 'portproxy listener re-registered'
    }
}

# ---- 4) 反向代理进程 ----
$proxy = $null
try {
    $proxy = Get-CimInstance Win32_Process -Filter "Name='node.exe'" -ErrorAction SilentlyContinue |
             Where-Object { $_.CommandLine -like '*https_proxy.js*' }
} catch {}
if (-not $proxy) {
    try {
        Start-Process -FilePath $node -ArgumentList "`"$root\host\https_proxy.js`"" -WindowStyle Hidden
        Log 'proxy started'
    } catch { Log "proxy start failed: $($_.Exception.Message)" }
}
