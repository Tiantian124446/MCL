# MCL 项目状态与运维手册（DSH 主设计）

更新时间：2026-11-22 00:02

## 关键资产

- 启动器源码：`E:\MCL\src`（C++17 / Win32 / GDI+），构建：`build.ps1` → `E:\MCL\build\MCLauncher.exe`
- 参考：`E:\PCL2.12.2`（PCL2）、`E:\HMCL`
- 发布包：`E:\MCL\server\payload` → `E:\MCL\server\stage\package.zip`
- 一键部署：`E:\MCL\tools\deploy.ps1 [-Version x.y.z] [-Notes ...]`（构建/打包/vmrun 上传/校验）
- GitHub 工具：`E:\MCL\tools\gh.ps1`（whoami / ensureRepo / push / release / uploadasset）
- 主机暂存：`http://192.168.3.155:8099`（Python http.server，`E:\MCL\server\stage`）
- 主机反馈接收：`http://192.168.3.155:8098/feedback`（Node，`E:\MCL\host\feedback_server.js`）→ `E:\MCL\feedback`

## GitHub

- 仓库：<https://github.com/Tiantian124446/MCL>
- 身份：`Tiantian124446`，token 在 Windows 凭据管理器（target `GitHub - https://api.github.com/Tiantian124446`）
- 最新 Release：<https://github.com/Tiantian124446/MCL/releases/tag/v1.1.0>，资产 `MCLauncher_v1.1.0.exe` 已验证 200

## Win10 服务器

- VM：`E:\文件\Virtual Machines\Windows 10 x64\Windows 10 x64.vmx`（Win10 Education 10240）
- Guest IP：`192.168.3.173`，站点：**http://192.168.3.173:7891**
- 账户：`ThinkBook`（管理员组），密码/自动登录：`E:\MCL\tools\vm.env`（**不提交**）
- 服务器目录：`C:\MCLServer`，日志 `C:\MCLServer\logs\server.log`
- 计划任务：`MCLServer`（登录启动）、`MCLServerWatchdog`（5 分钟）
- 防火墙：入站 TCP 7891 已放行
- VNC：`127.0.0.1:5907`（无密码），输入工具 `E:\MCL\tools\vnc.py`
- vmrun：`vmrun -T ws -gu ThinkBook -gp <见 vm.env> ...`，guest 程序以 High 完整性运行
- 更新入口：`deploy.ps1` 上传 package.zip → `runProgramInGuest ... update.ps1`（停旧服务、解压、重建任务、启动、自检）

## 站点功能

- `GET /`：深色落地页，「下载最新版本」+「BUG反馈」
- `GET /download/latest`：下载 `C:\MCLServer\downloads\MCLauncher.exe`
- `POST /feedback`：guest 保存 `BUG反馈_*.txt` 与 `BUG反馈.txt`，返回 txt 下载，并通知主机 8098
- `GET /api/status`：version / updated / size / feedbackCount / address
- `?feedback=1` 或 `#feedback`：自动打开反馈弹窗
- 当前版本：1.1.0；`version.json` 控制版本与说明（与 `ui.cpp` 的 `kAppVersion` 保持一致）

## 启动器 v1.1.0

- 首页：版本选择、昵称、内存滑块、开始游戏、安装/更新、**检查更新**、**官网 / 反馈**
- 启动时自动检查服务器版本；日志显示「Win10 服务器在线 · 已是最新版本 v1.1.0」
- 关于页：访问官网 / 提交 BUG 反馈（当前打开首页，待改成直接弹反馈窗）
- 其他：版本列表、游戏设置、模组下载、联机、全屏 F11

## 自动修复闭环

- DSH 定时任务：`schedule-b741b6d1-da18-4f72-ae33-ee50a2dea856`，每 15 分钟检查 `E:\MCL\feedback\PENDING`
- 有反馈时：读取 `BUG反馈_*.txt` → 修复 → `deploy.ps1` 重新发布 Win10 → `gh.ps1` 推送/Release → 写 `fix_result_<id>.md` → 清理 PENDING
- 主机接收器写入 `PENDING`、`pending.jsonl`、`BUG反馈.txt`

## 待办 / 下一步

1. 处理队列中的反馈：关于页「提交 BUG 反馈」应直接打开 `/?feedback=1`。
2. 继续对齐 PCL2：资源包/光影/存档管理、Microsoft 登录、崩溃日志分析、下载中心分类。
3. 让 Marvis / 豆包参与评审与测试，记录到 `docs/collaboration.md`。
4. 服务器版本清理与反馈计数展示优化。
