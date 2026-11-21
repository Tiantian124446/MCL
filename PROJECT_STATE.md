# MCL 项目状态与运维手册（DSH 主设计）

更新时间：2026-11-22 01:26

## 当前版本

- 启动器 / 服务器：**v1.7.0**
- 站点：<http://192.168.3.173:7891>
- 管理页：<http://192.168.3.173:7891/admin>（token 见 `C:\MCLServer\admin_token.txt`）
- GitHub：<https://github.com/Tiantian124446/MCL>
- Release：<https://github.com/Tiantian124446/MCL/releases/tag/v1.7.0>
- 最新资产 SHA256：`EF4B44E14D81D4A46C44C9BD722ED7C5CF21D997437EC4C267BF58A367A7EAF0`
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
- 管理页：`/admin` 输入 token → `/admin/feedback?token=…` 查看反馈记录
- `?feedback=1` / `#feedback` 自动打开反馈弹窗

## 启动器 v1.7.0

- 首页：版本选择、昵称、内存滑块、开始游戏、安装/更新、检查更新、官网/反馈
- 资源管理：8 类资源计数 + 打开文件夹
- 崩溃分析：`crash-reports` / `latest.log`，F9，识别内存/Java/模组/OpenGL 等
- **模组管理（v1.7.0）**：
  - `.jar` / `.jar.disabled` 列表、单项启用/禁用、删除、刷新、打开目录
  - **全部启用 / 全部禁用**（失败项计数提示）
  - 已验证：a.jar/b.jar → 全部禁用 `.disabled` → 全部启用恢复
- 存档管理：列表、备份、删除、打开、刷新；备份/删除用 `SHFileOperation`
- 诊断信息：关于页「导出诊断信息」生成 `诊断信息_<时间>.txt`
- 按钮 ID 冲突已修复（资源打开 / 崩溃分析 / 模组刷新 / 诊断）
- 版本列表、游戏设置、模组下载、联机、全屏 F11
- 关于页：v1.7.0 / 访问官网 / 提交 BUG 反馈 / 导出诊断信息

## 已知问题

- 资源下载页（Modrinth 资源包/光影）UI 与搜索可用，但应用内 WinHTTP 拉取 `/project/<id>/version` 会长时间挂起；cdn.modrinth.com 直连约 30KB/s。计划下一轮由 Win10 服务器代理 API/文件（局域网中转）。
- 原生 EDIT 自动测试需用 `WM_CHAR` 模拟真实输入；外部 `SetWindowText` 不会触发应用内读取（真实用户输入正常）。

## 自动反馈闭环

- DSH 定时任务 `schedule-b741b6d1-da18-4f72-ae33-ee50a2dea856`：每 15 分钟检查 `E:\MCL\feedback\PENDING`
- 已处理 `20261122_000219`（v1.1.1），记录 `docs/fixes/20261122_000219.md`
- 队列当前无 PENDING/PROCESSING；新反馈走同一流程

## 协作

- DSH：主设计 / 实现 / 发布
- 豆包：测试评审（云端无法访问私有 LAN IP，结论有网络局限）
- Marvis：浏览器扩展已确认安装（Edge ID `ljlhefjfpbolcfhmjdodmmmkamidkplg`，popup `src/popup/index.html`），下一轮从侧边栏/插件弹出页评审 LAN 站点
- 记录：`docs/collaboration.md`

## 待办

1. 由 Win10 服务器代理 Modrinth API/文件，上线资源下载。
2. Marvis 使用 Edge 插件 popup 评审 LAN 站点。
3. Microsoft 登录方案（需要 Azure 客户端 ID）。
4. 管理页反馈导出/分类。
