# MCL 项目状态与运维手册（DSH 主设计）

更新时间：2026-11-22 00:16

## 当前版本

- 启动器 / 服务器：**v1.1.1**
- 站点：<http://192.168.3.173:7891>
- GitHub：<https://github.com/Tiantian124446/MCL>
- Release：<https://github.com/Tiantian124446/MCL/releases/tag/v1.1.1>
- 最新资产 SHA256：`167982EB6CCA35BF0E8B6B0B1E1BBE889B70D242466B2988168F101874D5AE36`（站点下载 = GitHub 资产 = 本地构建）

## 关键资产

- 源码：`E:\MCL\src`（C++17 / Win32 / GDI+），构建 `build.ps1`
- 发布包：`E:\MCL\server\payload` → `E:\MCL\server\stage\package.zip`
- 一键部署：`E:\MCL\tools\deploy.ps1 [-Version x.y.z] [-Notes ...]`
- GitHub 工具：`E:\MCL\tools\gh.ps1`（whoami / ensureRepo / push / release / syncasset）
- 主机暂存：`http://192.168.3.155:8099`（服务 `E:\MCL\server\stage`）
- 主机反馈接收：`http://192.168.3.155:8098/feedback` → `E:\MCL\feedback`

## Win10 服务器

- VM：`E:\文件\Virtual Machines\Windows 10 x64\Windows 10 x64.vmx`（Win10 Education 10240）
- IP：`192.168.3.173`；端口 7891；防火墙已放行
- 账户：`ThinkBook`（管理员组），密码/自动登录在 `E:\MCL\tools\vm.env`（**不提交**）
- 目录：`C:\MCLServer`；日志 `C:\MCLServer\logs\server.log`
- 计划任务：`MCLServer`（登录启动）、`MCLServerWatchdog`（5 分钟）
- 远程输入：VNC `127.0.0.1:5907`，`E:\MCL\tools\vnc.py`（输入前先单独按一次 Shift 切英文，避免微软拼音吞空格）
- 部署入口：`vmrun copyFileFromHostToGuest` + `runProgramInGuest ... update.ps1`

## 站点

- 深色落地页：`下载最新版本` + `BUG反馈`
- `/download/latest` → `C:\MCLServer\downloads\MCLauncher.exe`
- `/feedback` → guest 保存 `BUG反馈_*.txt` / `BUG反馈.txt`，返回 txt 下载并通知主机 8098
- `/api/status` → version / updated / size / feedbackCount / address
- `?feedback=1` 或 `#feedback` → 自动打开反馈弹窗（v1.1.1 已验证）

## 启动器功能

- 版本选择、昵称、内存滑块、开始游戏、安装/更新
- **检查更新**：启动时查询 Win10 服务器 `/api/status`，日志显示服务器在线/最新版
- **官网 / 反馈**：打开站点
- **关于页**：访问官网、**提交 BUG 反馈**（v1.1.1 直接打开 `/?feedback=1`）
- 版本列表（分类/安装/删除）、游戏设置、模组下载（MC百科/Modrinth）、联机（服务器列表/直连）、全屏 F11

## 自动反馈闭环

- DSH 定时任务 `schedule-b741b6d1-da18-4f72-ae33-ee50a2dea856`：每 15 分钟检查 `E:\MCL\feedback\PENDING`
- 2026-11-22 00:12 已自动处理第一条真实反馈 `20261122_000219`：
  - 修复 About 页反馈按钮 → `/?feedback=1`
  - 版本 1.1.0 → 1.1.1
  - 部署 Win10、发布 GitHub Release、三端 SHA256 一致
  - 处理记录：`E:\MCL\feedback\fix_result_20261122_000219.md`、`docs/fixes/20261122_000219.md`
- 队列当前为空（PENDING/PROCESSING 均无）

## 协作

- DSH：主设计 / 实现 / 发布
- 豆包：已做测试评审（其云端无法访问私有 LAN IP，结论有网络局限；无认证/限流建议已记录）
- Marvis：下一轮让其在本机打开站点做界面评审
- 记录：`docs/collaboration.md`

## 待办

1. Marvis 界面评审并合并建议。
2. 继续对齐 PCL2：资源包/光影/存档管理、Microsoft 登录、崩溃日志分析、下载中心分类。
3. 站点反馈限流（同 IP 1 条/分钟）与反馈列表查看页（加 token）。
4. 下一版本建议 v1.2.0（功能批次），继续走 deploy + release 流水线。
