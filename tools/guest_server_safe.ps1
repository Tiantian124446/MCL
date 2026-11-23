$ErrorActionPreference = 'Continue'
$log = 'C:\mcl_safe.log'
function L($m) { $line = '[' + (Get-Date -Format 'yyyy-MM-dd HH:mm:ss') + '] ' + $m; Add-Content -Path $log -Value $line -Encoding UTF8 }
L '=== server safe config start ==='

# 1) 开机自动启动服务器（SYSTEM / ONSTART，不需要登录）
$action = 'powershell.exe -NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -File "C:\MCLServer\server.ps1"'
$r = schtasks /Create /TN "MCLServerBoot" /TR $action /SC ONSTART /RU SYSTEM /RL HIGHEST /F 2>&1
L ("schtasks ONSTART: " + ($r -join ' '))
$r2 = schtasks /Create /TN "MCLServer" /TR $action /SC ONLOGON /RL HIGHEST /F 2>&1
L ("schtasks ONLOGON: " + ($r2 -join ' '))
schtasks /Run /TN "MCLServerBoot" | Out-Null

# 2) 禁止自动关机 / 休眠 / 睡眠
powercfg /change standby-timeout-ac 0
powercfg /change hibernate-timeout-ac 0
powercfg /change disk-timeout-ac 0
powercfg /change monitor-timeout-ac 15
powercfg /hibernate off
L 'power settings: sleep/hibernate disabled'

# 3) 崩溃后不自动重启
reg add "HKLM\SYSTEM\CurrentControlSet\Control\CrashControl" /v AutoReboot /t REG_DWORD /d 0 /f | Out-Null
L 'AutoReboot=0'

# 4) 关闭 Windows 自动更新（避免更新后自动重启/关机）
$au = 'HKLM\SOFTWARE\Policies\Microsoft\Windows\WindowsUpdate\AU'
reg add $au /v NoAutoUpdate /t REG_DWORD /d 1 /f | Out-Null
reg add $au /v AUOptions /t REG_DWORD /d 2 /f | Out-Null
reg add $au /v NoAutoRebootWithLoggedOnUsers /t REG_DWORD /d 1 /f | Out-Null
L 'Windows Update: NoAutoUpdate=1'

# 5) 禁用 UpdateOrchestrator 的自动重启任务（存在才禁用）
$tasks = @(
  '\Microsoft\Windows\UpdateOrchestrator\Reboot',
  '\Microsoft\Windows\UpdateOrchestrator\Reboot_AC',
  '\Microsoft\Windows\UpdateOrchestrator\Reboot_Battery',
  '\Microsoft\Windows\UpdateOrchestrator\Schedule Scan'
)
foreach ($t in $tasks) {
    $o = schtasks /Change /TN $t /Disable 2>&1
    L ("disable $t -> " + ($o -join ' '))
}

# 6) 关掉快速启动，避免“关机”变休眠导致状态异常
reg add "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Power" /v HiberbootEnabled /t REG_DWORD /d 0 /f | Out-Null
L 'HiberbootEnabled=0'

# 7) 记录最终电源/任务状态
powercfg /a | Out-File -FilePath $log -Append -Encoding UTF8
schtasks /Query /TN "MCLServerBoot" /FO LIST | Out-File -FilePath $log -Append -Encoding UTF8
L '=== done ==='
