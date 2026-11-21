# MCL 项目状态与运维手册（DSH 主设计）

更新时间：2026-11-22 00:55

## 当前版本

- 启动器 / 服务器：**v1.5.1**
- 站点：<http://192.168.3.173:7891>
- GitHub：<https://github.com/Tiantian124446/MCL>
- Release：<https://github.com/Tiantian124446/MCL/releases/tag/v1.5.1>
- 最新资产 SHA256：`DD983221655F4093549EC84B813CC9FC0BACC632401F1A2DEC6DA8553A0F64A1`
  （站点下载 = GitHub 资产 = 本地构建，均已校验）

## 关键资产

- 源码：`E:\MCL\src`（C++17 / Win32 / GDI+），构建 `build.ps1`
- 发布包：`E:\MCL\server\payload` → `E:\MCL\server\stage\package.zip`（约 423 KB）
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
- 管理页：`/admin` 输入 token → `/admin/feedback?token=…` 查看 Win10 上的 `BUG反馈_*.txt`；token 在 `C:\MCLServer\admin_token.txt`
- `?feedback=1` / `#feedback` 自动打开反馈弹窗

## 启动器 v1.5.1

- 首页：版本选择、昵称、内存滑块、开始游戏、安装/更新、检查更新、官网/反馈
- 资源管理（v1.2.0）：8 类资源计数 + 打开文件夹
- 崩溃分析（v1.3.0）：`crash-reports` / `latest.log`，F9，识别内存/Java/模组/OpenGL 等
- 模组管理（v1.4.0）：`.jar` / `.jar.disabled` 列表、启用/禁用、删除、刷新
- 存档管理（v1.5.0 / v1.5.1）：
  - 扫描 `saves`，显示名称/大小/最后修改日期
  - 备份到 `backups/saves_<名称>_<时间戳>`
  - 删除（二次确认）、打开存档目录、刷新、打开 backups
  - v1.5.1 起备份/删除使用 Win32 `SHFileOperation`，exe 从 3.6MB 降至 **1.36MB**
- 版本列表（分类/安装/删除）、游戏设置、模组下载（MC百科/Modrinth）、联机、全屏 F11
- 关于页：v1.5.1 / 访问官网 / 提交 BUG 反馈（直达 `/?feedback=1`）

## 自动反馈闭环

- DSH 定时任务 `schedule-b741b6d1-da18-4f72-ae33-ee50a2dea856`：每 15 分钟检查 `E:\MCL\feedback\PENDING`
- 已处理 `20261122_000219`（v1.1.1），记录 `docs/fixes/20261122_000219.md`
- 队列当前无 PENDING/PROCESSING；新反馈会走同一流程
- 流程：读取 → 修复 → `deploy.ps1` 发布 Win10 → `gh.ps1` 推送/Release → 归档 → 清标记

## 协作

- DSH：主设计 / 实现 / 发布
- 豆包：测试评审（云端无法访问私有 LAN IP，结论有网络局限；无认证/限流建议已记录）
- Marvis：浏览器集成式 AI 助手，本机有 `MarvisAgent`/`MarvisHost`/浏览器扩展宿主；无独立聊天窗口，需从浏览器侧边栏唤起，评审留待下一轮
- 记录：`docs/collaboration.md`

## 待办

1. Marvis 从浏览器侧边栏打开 LAN 站点做界面评审并合并建议。
2. PCL2 对齐下一批：下载中心分类（资源包/光影/整合包）、Microsoft 登录、资源包/光影启用状态识别。
3. 站点反馈查看页已完成（`/admin` + token）；可增加导出/分类。
4. 下一版本建议 v1.6.0，继续 deploy + release 流水线。
