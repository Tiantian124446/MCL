# ---------------------------------------------------------------------------
#  mcl-client-setup.ps1  —  校园网/其它电脑一键配置
#  作用：让本机可以用 https://pocket4 访问 MCL 服务器
#    1) 从服务器下载自签证书并导入“受信任的根证书颁发机构”
#    2) 写入 hosts：pocket4 / pocket4-MCL.com -> 服务器 IP
#    3) 自检 https://pocket4/api/status
#  需要管理员权限（右键“以管理员身份运行”）。
# ---------------------------------------------------------------------------
param(
    [string]$ServerIp = '',
    [switch]$NoPause
)

$ErrorActionPreference = 'Continue'
$Host.UI.RawUI.WindowTitle = 'MCL 校园网配置'

function Pause2 { if ($NoPause) { return }; Write-Host ''; Write-Host '按任意键退出...' -ForegroundColor DarkGray; $null = $Host.UI.RawUI.ReadKey('NoEcho,IncludeKeyDown') }

$id = [Security.Principal.WindowsIdentity]::GetCurrent()
$pr = New-Object Security.Principal.WindowsPrincipal($id)
if (-not $pr.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Host ''
    Write-Host '  需要管理员权限：请右键这个文件/脚本，选择“以管理员身份运行”。' -ForegroundColor Red
    Pause2
    exit 1
}

Write-Host ''
Write-Host '  ================================================' -ForegroundColor Cyan
Write-Host '   MCL 校园网访问配置（https://pocket4）' -ForegroundColor Cyan
Write-Host '  ================================================' -ForegroundColor Cyan
Write-Host ''

if (-not $ServerIp) {
    $ServerIp = Read-Host '  请输入服务器电脑在校园网里的 IP（例如 192.168.3.173）'
}
$ServerIp = ($ServerIp -replace '\s', '')
if (-not ($ServerIp -match '^\d{1,3}(\.\d{1,3}){3}$')) {
    Write-Host '  IP 格式不正确，已退出。' -ForegroundColor Red
    Pause2
    exit 1
}

# ---- 1) 下载证书 ----
$cerPath = Join-Path $env:TEMP 'mcl-ca.cer'
$ok = $false
foreach ($port in @('8080', '7891')) {
    try {
        Invoke-WebRequest ("http://{0}:{1}/mcl-ca.cer" -f $ServerIp, $port) -OutFile $cerPath -UseBasicParsing -TimeoutSec 8
        Write-Host ("  已从 {0}:{1} 下载证书。" -f $ServerIp, $port) -ForegroundColor Green
        $ok = $true
        break
    } catch {
        Write-Host ("  连接 {0}:{1} 失败，尝试下一个端口..." -f $ServerIp, $port) -ForegroundColor DarkYellow
    }
}
if (-not $ok) {
    Write-Host ''
    Write-Host '  无法连接服务器。请确认：' -ForegroundColor Red
    Write-Host '    - 服务器电脑已开机、服务器在运行；' -ForegroundColor Red
    Write-Host '    - 你和服务器在同一校园网；' -ForegroundColor Red
    Write-Host ("    - IP 是否正确（当前填的是 {0}）。" -f $ServerIp) -ForegroundColor Red
    Pause2
    exit 1
}

try {
    Import-Certificate -FilePath $cerPath -CertStoreLocation 'Cert:\LocalMachine\Root' | Out-Null
    Write-Host '  证书已导入“受信任的根证书颁发机构”。' -ForegroundColor Green
} catch {
    Write-Host ('  证书导入失败：' + $_.Exception.Message) -ForegroundColor Red
}

# ---- 2) hosts ----
$hosts = Join-Path $env:WINDIR 'System32\drivers\etc\hosts'
try {
    if (-not (Test-Path "$hosts.mclbak")) { Copy-Item $hosts "$hosts.mclbak" -Force -ErrorAction SilentlyContinue }
    $lines = [System.IO.File]::ReadAllLines($hosts)
    $out = New-Object System.Collections.Generic.List[string]
    foreach ($l in $lines) { if ($l -notmatch '(?i)\bpocket4(-mcl\.com)?\b') { $out.Add($l) } }
    $out.Add('# MCL launcher server (added by mcl-client-setup)')
    $out.Add("$ServerIp pocket4")
    $out.Add("$ServerIp pocket4-MCL.com")
    $out.Add("$ServerIp www.pocket4-MCL.com")
    [System.IO.File]::WriteAllLines($hosts, $out, (New-Object System.Text.ASCIIEncoding))
    Write-Host ('  hosts 已更新：pocket4 -> ' + $ServerIp) -ForegroundColor Green
} catch {
    Write-Host ('  hosts 写入失败：' + $_.Exception.Message) -ForegroundColor Red
}
ipconfig /flushdns | Out-Null

# ---- 3) 自检 ----
Write-Host ''
Write-Host '  正在检查 https://pocket4 ...' -ForegroundColor Yellow
Start-Sleep -Seconds 1
try {
    $r = Invoke-WebRequest 'https://pocket4/api/status' -UseBasicParsing -TimeoutSec 10
    $j = $r.Content | ConvertFrom-Json
    Write-Host ''
    Write-Host ('  配置成功！服务器版本 v' + $j.version) -ForegroundColor Green
    Write-Host '  在浏览器打开： https://pocket4' -ForegroundColor Green
} catch {
    Write-Host ''
    Write-Host ('  自检失败：' + $_.Exception.Message) -ForegroundColor Red
    Write-Host '  可先用 http://' -NoNewline -ForegroundColor DarkYellow
    Write-Host ("{0}:8080" -f $ServerIp) -NoNewline -ForegroundColor DarkYellow
    Write-Host ' 访问，确认服务器在线。' -ForegroundColor DarkYellow
}
Pause2
