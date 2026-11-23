# MCL 项目状态与运维手册（DSH 主设计）

更新时间：2026-11-23 21:20

## 当前版本

- 启动器 / 服务器：**v1.28.1**
- 站点：<https://pocket4>（本机 hosts 解析到 192.168.3.173，Node 反代到 Win10 虚拟机）
- 兼容域名：<https://pocket4-MCL.com>（同样走本机 hosts）
- 局域网回退：<http://192.168.3.173:7891>（portproxy → 虚拟机 192.168.201.129:7891）
- 管理页：<https://pocket4/admin>（token 见 `C:\MCLServer\admin_token.txt`）
- GitHub：<https://github.com/Tiantian124446/MCL>
- 验收报告：[docs/acceptance.md](docs/acceptance.md)

## 关键资产

- 源码：`E:\MCL\src`（C++17 / Win32 / GDI+），构建 `build.ps1`（含 `windres` 图标资源）
- 图标：`res/mcl.ico`（多尺寸），生成脚本 `tools/make_icon.py`
- 发布包：`E:\MCL\server\payload` → `E:\MCL\server\stage\package.zip`
- 一键部署：`E:\MCL\tools\deploy.ps1 [-Version x.y.z] [-Notes ...]`
- 域名/端口修复：`E:\MCL\tools\update_domain.ps1`（主机 IP 或 VM IP 变化后运行）
- 自愈看护：`E:\MCL\tools\server_watchdog.ps1`（计划任务 `MCLServerWatch`，每 2 分钟；`MCLServerWatchLogon` 登录时）
- 反向代理：`E:\MCL\host\https_proxy.js`（计划任务 `MCLProxy`，开机自启）
- GitHub 工具：`E:\MCL\tools\gh.ps1`（whoami / ensureRepo / push / release / syncasset）
- 主机反馈接收：`http://192.168.3.155:8098/feedback` → `E:\MCL\feedback`

## Win10 服务器

- VM：`E:\文件\Virtual Machines\Windows 10 x64\Windows 10 x64.vmx`
- 网络：**NAT（VMnet8）**，guest DHCP → `192.168.201.129`，网关/DNS `192.168.201.2`
  - 2026-11-23 修复：guest 之前残留静态 IP `192.168.3.173`（与主机 WLAN 冲突且不在 NAT 网段），
    已恢复 DHCP；**主机 WLAN 始终是 DHCP，未做任何网卡修改**
- 主机转发：
  - `netsh portproxy 192.168.3.173:7891 -> 192.168.201.129:7891`
  - Node 反代 `192.168.3.173:443 -> 192.168.201.129:7891`（https://pocket4）
  - hosts：`192.168.3.173 pocket4 / pocket4-MCL.com / www.pocket4-MCL.com`
- 证书：自签 `host/cert/mcl.pfx`（SAN：pocket4 等三个名字），已导入本机 `LocalMachine\Root`
- **开机自启**：虚拟机内 `MCLServerBoot`（ONSTART/SYSTEM）拉起服务器；主机 `MCLServerWatch`（每 2 分钟）
  负责启动 VM、同步 IP/hosts/端口转发、保活反向代理；若主机停在下拉登录界面，VM 要等登录后才会被拉起
- 代理每次请求都会重读 `host/target.txt`（5 秒缓存），虚拟机 IP 变化后无需重启代理
- 三个计划任务均已关闭“仅交流电时运行/电池停止”限制（笔记本用电池时也能自愈）
- **禁止自动关机**：虚拟机已关闭睡眠/休眠、自动更新与自动重启（`tools/guest_server_safe.ps1`）
- 账户：`ThinkBook`（管理员组），密码在 `E:\MCL\tools\vm.env`（**不提交**）
- 目录：`C:\MCLServer`；计划任务 `MCLServer` / `MCLServerWatchdog`
- 远程输入：VNC `127.0.0.1:5907`（`tools/vnc.py`；先按一次 Shift 切英文，避免拼音吞空格）
- 部署：`vmrun copyFileFromHostToGuest` + `runProgramInGuest ... update.ps1`

## 站点

- 深色落地页：`下载最新版本` + `BUG反馈`；`/api/status` 动态显示版本
- `/feedback` → guest 保存 `BUG反馈_*.txt` / `BUG反馈.txt`，返回 txt 下载并通知主机
- 反馈限流：同 IP 60 秒 1 条（429 + `Retry-After`），请求体 > 20KB 返回 413
- 管理页：`/admin` 输入 token → 查看/搜索记录（10 条/页，上一页/下一页）；`/admin/feedback.csv?token=…&q=…` 导出 CSV
- `?feedback=1` / `#feedback` 自动打开反馈弹窗

## 启动器 v1.28.1

- **图标**：绿色草方块多尺寸 ico（资源管理器/任务栏/标题栏）
- **服务器地址**：`https://pocket4`（兼容 `pocket4-MCL.com`），不可达自动回退 `http://192.168.3.173:7891`
- **开机自启 / 防自动关机**：虚拟机内 ONSTART 任务 + 主机看护任务；关闭睡眠/休眠/自动更新/自动重启
- **首次运行**：无 `mcl_settings.txt` 时自动拉取 `/api/default-config` 并保存
- **自恢复保护**：`%LOCALAPPDATA%\MCL\guard` 备份 + `HKCU Run` 登录还原；「关于」页可「删除自恢复」
- **UI 修复**：首页昵称标签/输入框不再重叠；侧边栏文字不再截断；版本列表「搜索」标签贴齐；`MCL.log` 中文正常
- 继承自 v1.27.x：资源下载、自更新、拖拽导入、继承版本、natives 扁平化、加载器 Java 检查、单实例、文件日志、版本隔离、存档备份/恢复、游戏选项、自动备份、服务器测速、账户、模组管理、版本搜索、诊断、崩溃分析

## 已安装版本（本机）

- 1.20.1
- 1.20.1-forge-47.4.28（已实际启动到 Render thread）
- 1.21.1-Forge
- 26.4-snapshot-2
- fabric-loader-0.20.1-1.20.1（已实际启动，出现 Minecraft 1.20.1 进程）

## 已知问题

- **域名 `pocket4` / `pocket4-MCL.com` 都未注册**：目前只在本机 hosts 生效；
  其它电脑要用这个地址，需要注册域名并做公网入口，或在每台机器加 hosts + 信任自签证书。
- 自签证书只在本机受信任；启动器对自建服务器跳过证书校验，浏览器在其它机器会提示不安全。
- 课堂管理软件（极域）的清理策略无法被完全对抗：自恢复是“被删后自动还原”，不是“无法删除”。
- 虚拟机走 NAT，IP 由 DHCP 分配；IP 变化由 `MCLServerWatch` 自动同步（也可手动跑 `update_domain.ps1`）。
- 主机 443 被 Steam++ 占用，反代只监听 `192.168.3.173:443`；不要改动 Steam++ 的监听设置。
- 主机停在下拉登录界面时，计划任务不会运行，虚拟机需登录后才会被自动拉起。
- Forge 1.20.1 在本次测试环境完成资源图集初始化后进程退出，无 launcher 错误；如需排查可查看 `latest.log` / 显卡环境。

## 自动反馈闭环

- DSH 定时任务 `schedule-b741b6d1-da18-4f72-ae33-ee50a2dea856`：每 15 分钟检查 `E:\MCL\feedback\PENDING`
- 已处理 `20261122_000219`（v1.1.1），记录 `docs/fixes/20261122_000219.md`
- 队列当前无 PENDING/PROCESSING

## 协作

- DSH：主设计 / 实现 / 发布
- 豆包：桌面端可对话（上一轮评审已采纳限流/认证建议）；本轮尝试再次咨询时桌面被其它全屏程序占用，未取到回复
- Marvis：`MarvisHost` 监听 127.0.0.1:10123（401）/ 39099（404），需要内部鉴权，无公开聊天 API
- 记录：`docs/collaboration.md`

## 待办

1. 域名注册 + 公网入口（当前只能本机/局域网使用）。
2. Marvis 评审或替代评审。
3. Microsoft 登录（需要 Azure 客户端 ID）。
4. 自签证书换成正式证书（需要域名控制权）。
