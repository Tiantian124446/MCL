param(
  [string]$Version = '',
  [string]$Notes = '',
  [switch]$SkipBuild
)
$ErrorActionPreference = 'Stop'
$root = 'E:\MCL'
$payload = Join-Path $root 'server\payload'
$stage = Join-Path $root 'server\stage'
$vmx = 'E:\文件\Virtual Machines\Windows 10 x64\Windows 10 x64.vmx'
$vmrun = 'C:\Program Files (x86)\VMware\VMware Workstation\vmrun.exe'
$guestUser = $env:MCL_VM_USER
$guestPass = $env:MCL_VM_PASS
$vmEnv = Join-Path $PSScriptRoot 'vm.env'
if ((!$guestUser -or !$guestPass) -and (Test-Path $vmEnv)) {
    foreach ($line in Get-Content $vmEnv -Encoding UTF8) {
        if ($line -match '^\s*MCL_VM_USER\s*=(.*)$') { $guestUser = $Matches[1].Trim() }
        if ($line -match '^\s*MCL_VM_PASS\s*=(.*)$') { $guestPass = $Matches[1].Trim() }
    }
}
if (-not $guestUser) { $guestUser = 'ThinkBook' }
if (-not $guestPass) { throw 'MCL_VM_PASS not set (put it in E:\MCL\tools\vm.env, which is gitignored)' }

function VmRun([string[]]$argv) { & $vmrun -T ws -gu $guestUser -gp $guestPass @argv }

# 服务器地址: 优先用 vmrun 报告的 guest IP（VM 走 NAT, IP 会变）
$server = 'http://192.168.3.173:7891'
try {
    $ipOut = (& $vmrun -T ws getGuestIPAddress $vmx -wait) 2>$null | Select-Object -Last 1
    if ($ipOut -and $ipOut -match '^\d+\.\d+\.\d+\.\d+$') { $server = "http://$ipOut" + ':7891' }
} catch { }
Write-Host "server = $server"

if (-not $SkipBuild) {
    Write-Host '== build =='
    & (Join-Path $root 'build.ps1')
    if ($LASTEXITCODE -ne 0) { throw 'build failed' }
}
Copy-Item (Join-Path $root 'build\MCLauncher.exe') (Join-Path $payload 'downloads\MCLauncher.exe') -Force

$vj = Join-Path $payload 'version.json'
$v = Get-Content $vj -Raw -Encoding UTF8 | ConvertFrom-Json
if ($Version) { $v.version = $Version }
if ($Notes) { $v.notes = $Notes }
$v.updated = Get-Date -Format 'yyyy-MM-dd HH:mm'
[IO.File]::WriteAllText($vj, ($v | ConvertTo-Json -Depth 4), (New-Object Text.UTF8Encoding($false)))
Write-Host ("== package v{0} ==" -f $v.version)

Copy-Item (Join-Path $payload 'update.ps1') (Join-Path $stage 'update.ps1') -Force
if (-not (Test-Path (Join-Path $stage 'bootstrap.ps1'))) { Copy-Item (Join-Path $payload 'bootstrap.ps1') (Join-Path $stage 'bootstrap.ps1') -Force }
Compress-Archive -Path (Join-Path $payload '*') -DestinationPath (Join-Path $stage 'package.zip') -Force
$zipSize = (Get-Item (Join-Path $stage 'package.zip')).Length
Write-Host "package.zip $zipSize bytes"

Write-Host '== upload package =='
VmRun @('copyFileFromHostToGuest', $vmx, (Join-Path $stage 'package.zip'), 'C:\MCLServer\package.zip')
if ($LASTEXITCODE -ne 0) { throw 'copy package failed' }
Write-Host '== upload update.ps1 =='
VmRun @('copyFileFromHostToGuest', $vmx, (Join-Path $payload 'update.ps1'), 'C:\MCLServer\update.ps1')
if ($LASTEXITCODE -ne 0) { throw 'copy update.ps1 failed' }
Write-Host '== run update.ps1 =='
VmRun @('runProgramInGuest', $vmx, 'C:\Windows\System32\WindowsPowerShell\v1.0\powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', 'C:\MCLServer\update.ps1')
Start-Sleep -Seconds 6
Write-Host '== verify =='
for ($i = 0; $i -lt 10; $i++) {
    try {
        $r = Invoke-WebRequest "$server/api/status" -UseBasicParsing -TimeoutSec 6
        $j = $r.Content | ConvertFrom-Json
        if ($j.version -eq $v.version) {
            Write-Host "OK: server v$($j.version), size=$($j.size), feedback=$($j.feedbackCount), $($j.address)"
            exit 0
        }
        Write-Host "server still v$($j.version), waiting..."
    } catch { Write-Host "waiting: $($_.Exception.Message)" }
    Start-Sleep -Seconds 3
}
throw 'server did not report the new version'
