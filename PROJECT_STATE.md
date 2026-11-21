# MCL 项目状态与运维手册（DSH 主设计）

更新时间：2026-11-22 02:11

## 当前版本

- 启动器 / 服务器：**v1.17.1**
- 站点：<http://192.168.3.173:7891>
- 管理页：<http://192.168.3.173:7891/admin>（token 见 `C:\MCLServer\admin_token.txt`）
- GitHub：<https://github.com/Tiantian124446/MCL>
- Release：<https://github.com/Tiantian124446/MCL/releases/tag/v1.17.1>
- 最新资产 SHA256：`39AC475701E3A54C9728DD0370BE63E2E517F32F5AEE523D35BFF90B47B87563`
  （站点下载 = GitHub 资产 = 本地构建，均已校验）

## 关键资产

- 源码：`E:\MCL\src`（C++17 / Win32 / GDI+），构建 `build.ps1`
- 发布包：`E:\MCL\server\payload` → `E:\MCL\server\stage\package.zip`
- 一键部署：`E:\MCL\tools\deploy.ps1 [-Version x.y.z] [-Notes ...]`
- GitHub 工具：`E:\MCL\tools\gh.ps1`（whoami / ensureRepo / push / release / syncasset）
- 主机反馈接收：`http://192.168.3.155:8098/feedback` → `E:\MCL\feedback`

## Win10 服务器

- VM：`E:\文件\Virtual Machines\Windows 10 x64\Windows 10 x64.vmx`
- IP：`192.168.3.173`；端口 7891；防火墙已放行
- 账户：`ThinkBook`（管理员组），密码/自动登录在 `E:\MCL\tools\vm.env`（**不提交**）
- 目录：`C:\MCLServer`；计划任务 `MCLServer` / `MCLServerWatchdog`
- 远程输入：VNC `127.0.0.1:5907`（`tools/vnc.py`；先按一次 Shift 切英文，避免拼音吞空格）
- 部署：`vmrun copyFileFromHostToGuest` + `runProgramInGuest ... update.ps1`

## 站点

- 深色落地页：`下载最新版本` + `BUG反馈`；`/api/status` 动态显示版本
- `/feedback` → guest 保存 `BUG反馈_*.txt` / `BUG反馈.txt`，返回 txt 下载并通知主机
- 反馈限流：同 IP 60 秒 1 条（429 + `Retry-After`），请求体 > 20KB 返回 413
- 管理页：`/admin` 输入 token → `/admin/feedback?token=…` 查看记录；支持 `&q=关键词` 搜索文件名/内容；`/admin/feedback.csv?token=…&q=…` 导出 CSV
- `?feedback=1` / `#feedback` 自动打开反馈弹窗

## 启动器 v1.17.0

- 首页：版本选择、昵称、内存滑块、开始游戏、安装/更新、检查更新、官网/反馈
- 启动前 Java 检查：1.20.5+→21，1.18–1.20.4→17，1.17→16，更早→8；不匹配阻止启动
- **版本隔离（v1.17.0）**：
  - 游戏设置新增开关
  - 开启后 mods/saves/资源包/光影/截图/日志/崩溃/备份使用 `versions/<版本>/`
  - 版本列表仍指向全局 `versions`
  - 现有全局数据不自动迁移，可随时关闭
  - v1.17.1 修复「一键进服」在隔离下先应用设置后设置版本导致的目录错位
- 账户：离线昵称列表、添加/使用/删除、持久化
- 游戏设置：目录/Java/内存/下载源；Java 扫描与切换；自动备份存档开关
- 游戏选项：可视化修改 `options.txt`
- 自动备份：启动时 24 小时内无 `backups/auto_*` 则复制 `saves`
- 版本列表搜索
- 模组管理：名称筛选、单项/全部启用禁用、删除、刷新、打开目录
- 存档管理：列表、备份、删除、打开/刷新、恢复最近备份
- 联机：服务器列表、添加/编辑/删除/显示切换、一键直连、TCP 测速
- 资源管理、崩溃分析、诊断信息
- 模组下载、全屏 F11
- 关于页：v1.17.0 / 访问官网 / 提交 BUG 反馈 / 导出诊断信息

## 已知问题

- 资源下载（Modrinth）：App 内 WinHTTP 版本请求易挂起；guest API 约 16.6s、CDN TLS 失败；需代理/镜像，暂不发布。
- 原生 EDIT 自动测试需用 `WM_CHAR`；外部 `SetWindowText` 不触发应用内读取（真实用户输入正常）。

## 自动反馈闭环

- DSH 定时任务 `schedule-b741b6d1-da18-4f72-ae33-ee50a2dea856`：每 15 分钟检查 `E:\MCL\feedback\PENDING`
- 已处理 `20261122_000219`（v1.1.1），记录 `docs/fixes/20261122_000219.md`
- 队列当前无 PENDING/PROCESSING

## 协作

- DSH：主设计 / 实现 / 发布
- 豆包：测试评审（云端无法访问私有 LAN IP，结论有网络局限）
- Marvis：Edge 插件已安装并显示「已连接」；popup 仅连接状态，桌面端无独立聊天窗口，评审未完成
- 记录：`docs/collaboration.md`

## 待办

1. 资源下载：Modrinth 代理/镜像。
2. Marvis 评审或替代评审。
3. Microsoft 登录（需要 Azure 客户端 ID）。
4. 管理页反馈分页/分类（搜索与 CSV 已完成）。
