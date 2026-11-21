# MCL 项目状态与运维手册（DSH 主设计）

更新时间：2026-11-22 00:30

## 当前版本

- 启动器 / 服务器：**v1.3.0**
- 站点：<http://192.168.3.173:7891>
- GitHub：<https://github.com/Tiantian124446/MCL>
- Release：<https://github.com/Tiantian124446/MCL/releases/tag/v1.3.0>
- 最新资产 SHA256：`70623816D6DE8555673D2FE48350DB60F3AED79B6C513FA1604D55D60AA35F2B`
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
- `?feedback=1` / `#feedback` 自动打开反馈弹窗

## 启动器 v1.3.0

- 首页：版本选择、昵称、内存滑块、开始游戏、安装/更新、检查更新、官网/反馈
- 资源管理页（v1.2.0）：资源包 / 光影包 / 存档 / 截图 / 模组 / 日志 / 崩溃报告 / 版本；计数 + 一键打开文件夹
- **崩溃分析（v1.3.0）**：
  - 资源管理 → 崩溃报告 → 「分析最新崩溃」；快捷键 **F9**
  - 读取最新 `crash-reports/crash-*.txt`，无则读 `logs/latest.log`
  - 识别内存不足、Java 版本不匹配、模组依赖/重复/不兼容、显卡 OpenGL、主线程崩溃
  - 弹窗可直接打开官网反馈页（`/?feedback=1`）
  - 已用合成 `OutOfMemoryError` 崩溃报告验证
- 版本列表（分类/安装/删除）、游戏设置、模组下载（MC百科/Modrinth）、联机、全屏 F11
- 关于页：v1.3.0 / 访问官网 / 提交 BUG 反馈（直达 `/?feedback=1`）

## 自动反馈闭环

- DSH 定时任务 `schedule-b741b6d1-da18-4f72-ae33-ee50a2dea856`：每 15 分钟检查 `E:\MCL\feedback\PENDING`
- 已处理 `20261122_000219`（v1.1.1），记录 `docs/fixes/20261122_000219.md`
- 队列当前无 PENDING/PROCESSING；新反馈会走同一流程
- 流程：读取 → 修复 → `deploy.ps1` 发布 Win10 → `gh.ps1` 推送/Release → 归档 → 清标记

## 协作

- DSH：主设计 / 实现 / 发布
- 豆包：已做测试评审；其云端无法访问私有 LAN IP，结论有网络局限；无认证/限流建议已记录
- Marvis：本机是浏览器集成式 AI 助手（无独立聊天窗口，后台进程 `Marvis`/`MarvisHost`）；需从浏览器侧边栏唤起，评审留待下一轮
- 记录：`docs/collaboration.md`

## 待办

1. Marvis 从浏览器侧边栏打开 LAN 站点做界面评审并合并建议。
2. 下一批 PCL2 对齐：Microsoft 登录、下载中心分类、资源包/光影启用状态识别、存档备份。
3. 站点反馈限流（同 IP 1 条/分钟）与反馈查看页（加 token）。
4. 下一版本建议 v1.4.0，继续 deploy + release 流水线。
