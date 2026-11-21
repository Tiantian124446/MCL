# MCL 项目状态与运维手册（DSH 主设计）

更新时间：2026-11-21 23:50

## 关键资产

- 启动器源码：`E:\MCL\src`（C++ Win32），构建产物：`E:\MCL\build\MCLauncher.exe`，当前发布副本：`E:\MCL\MCLauncher.exe`
- 参考启动器：`E:\PCL2.12.2`（PCL2）；HMCL 也在 E:\HMCL
- 服务器打包目录：`E:\MCL\server\payload`，发布包：`E:\MCL\server\stage\package.zip`
- 部署脚本：`E:\MCL\server\payload\bootstrap.ps1`（在 Win10 中提权运行）
- 主机中转/暂存：`http://192.168.3.155:8099`（Python http.server，服务 `E:\MCL\server\stage`）
- 主机反馈接收器：`http://192.168.3.155:8098/feedback`（Node，`E:\MCL\host\feedback_server.js`），反馈落盘 `E:\MCL\feedback`
- 反馈任务标记：`E:\MCL\feedback\PENDING`、`pending.jsonl`

## Win10 服务器

- VM：`E:\文件\Virtual Machines\Windows 10 x64\Windows 10 x64.vmx`（Win10 Education 10240，2 vCPU / 4.3GB）
- Guest IP：`192.168.3.173`（桥接），站点：`http://192.168.3.173:7891`
- 账户：`ThinkBook`（管理员组），当前**未设密码**（登录自动进入桌面）
- 服务器目录：`C:\MCLServer`，服务脚本 `server.ps1`，日志 `C:\MCLServer\logs\server.log`
- 计划任务：`MCLServer`（登录时启动，最高权限）、`MCLServerWatchdog`（每 5 分钟）
- 防火墙规则：`MCL Server 7891`（TCP 入站 7891）
- 远程画面/输入：VMware VNC `127.0.0.1:5907`（无密码），工具 `E:\MCL\tools\vnc.py`
  - 用法：`python vnc.py --keys "win+r"`、`--text-file cmd.txt`、`--click "600,700"`
  - 关键坑：Win10 微软拼音 IME 会吞空格/把字母转中文。必须先按一次单独的 Shift 切到英文模式，且输入命令避免大写字母带来的 Shift 抖动。
  - 提权：在 guest 里 `powershell -c "start-process cmd -verb runas"`，UAC 弹窗按 `alt+y`
- VMware Tools：installed / running
- 部署入口（guest 内提权 cmd）：
  `powershell -executionpolicy bypass -file c:\b.ps1`（b.ps1 先下载 `http://192.168.3.155:8099/bootstrap.ps1`）

## 站点功能

- `GET /`：深色落地页，含「下载最新版本」「BUG反馈」
- `GET /download/latest`：下载 `C:\MCLServer\downloads\MCLauncher.exe`（带版本文件名）
- `POST /feedback`：保存 `C:\MCLServer\feedback\BUG反馈_*.txt` 与 `BUG反馈.txt`，并 POST 到主机 8098
- 当前版本：1.0.0；`version.json` 控制版本、更新时间、说明

## 已知问题 / 下一步

1. 主机反馈副本中文乱码：guest 用 WebClient.UploadString 通知 8098 时编码不一致；guest 自身 txt 与网页响应是正确的。修复：guest 端改用 UTF-8 字节 UploadData，或主机端按 GBK 回退解码。
2. 启动器 UI/功能需要按 PCL2 对齐，并重新构建发布。
3. GitHub：token 在 Windows Credential Manager（target `GitHub - https://api.github.com/Tiantian124446`），用户名 `Tiantian124446`；仓库待创建。
4. 自动修复闭环：主机 watcher 检测 `PENDING` 后触发 DSH 修复/重新发布；需要 DSH 定时任务或常驻脚本配合。
5. Marvis（腾讯，E:\Program Files\Tencent\Marvis）与豆包已安装，可作为评审/测试角色接入。

## 常用命令

- 构建暂存包：`Compress-Archive -Path E:\MCL\server\payload\* -DestinationPath E:\MCL\server\stage\package.zip -Force`
- 主机自测：`Invoke-WebRequest http://192.168.3.173:7891/api/status -UseBasicParsing`
- 看服务器日志：guest 内 `type C:\MCLServer\logs\server.log`
- VNC 截图：`vmcli.exe <vmx> MKS captureScreenshot E:\MCL\shot.png`
