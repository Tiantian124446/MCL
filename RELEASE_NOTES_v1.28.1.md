# MCL 启动器 v1.28.1

## 地址改为 https://pocket4

- 启动器服务器地址：`https://pocket4`（保留回退 `http://192.168.3.173:7891`）
- 官网、管理页、`/api/status` 的对外地址统一显示 `pocket4`
- 证书重新签发，SAN 同时包含 `pocket4`、`pocket4-MCL.com`、`www.pocket4-MCL.com`
- hosts 增加 `192.168.3.173 pocket4`（本机与局域网解析）
- 说明：`pocket4` 是单标签主机名，只能在能解析它的机器上使用（本机/手动加 hosts 的机器）

## 服务器开机自启与自愈

- 新增 `tools/server_watchdog.ps1`，计划任务 `MCLServerWatch`：**每 2 分钟**检查
  - 虚拟机没运行 → `vmrun start ... nogui`
  - 读取虚拟机 IP → 更新 `host/target.txt`
  - 主机 IP 变化 → 重写 hosts 与 `7891` 端口转发
  - 反向代理进程没了 → 重启
- 另有 `MCLServerWatchLogon`：登录时立即执行一次
- 虚拟机内新增计划任务 `MCLServerBoot`（**开机 ONSTART / SYSTEM**）直接拉起 `server.ps1`，
  不再依赖登录；`MCLServer`（登录）与 `MCLServerWatchdog`（每 5 分钟）保留

## Win10 禁止自动关机

虚拟机上已应用（不影响手动关机）：

- `powercfg`：关闭睡眠/休眠/磁盘超时（`standby-timeout-ac 0`、`hibernate off`）
- `CrashControl\AutoReboot = 0`：崩溃后不自动重启
- Windows Update：`NoAutoUpdate=1`、`AUOptions=2`、`NoAutoRebootWithLoggedOnUsers=1`
- 禁用 `UpdateOrchestrator` 的 `Reboot` / `Schedule Scan` 计划任务
- 关闭快速启动 `HiberbootEnabled=0`

脚本：`tools/guest_server_safe.ps1`（日志 `C:\mcl_safe.log`）

## 故障修复

上一版本用户截图显示 `检查更新失败：请求发送失败（错误码 12029）`：

- 原因：主机反向代理进程退出 + `7891` 端口转发监听丢失（虚拟机内服务器一直是正常的）
- 处理：重启代理、重注册端口转发；并由新的看护任务每 2 分钟自愈
- 现在首页状态为「Win10 服务器在线 · 已是最新版本 v1.28.1」

## 验证

- `https://pocket4/api/status` → 200，`v1.28.1`，`address = pocket4`
- `https://pocket4-MCL.com/api/status` → 200（旧域名保留可用）
- `http://192.168.3.173:7891/api/status` → 200（局域网回退）
- 启动器首页日志：`服务器在线，已是最新版本 v1.28.1。`
- 虚拟机关机自启：`MCLServerBoot` 已创建（ONSTART/SYSTEM）
