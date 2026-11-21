# MCL 启动器验收报告

更新时间：2026-11-22
当前版本：**v1.27.1**

## 1. 目标与交付

| 目标 | 实现 | 证据 |
|---|---|---|
| 把 `MCLauncher.exe` 改造成 PCL2 风格 Java 启动器 | C++17 / Win32 / GDI+ 自绘 UI，12 个页面 | 本报告第 2 节 |
| 界面整洁、不报错 | 统一卡片/按钮/日志区；12 页导航巡检 12/12 通过 | 第 3 节 |
| Win10 作为服务器 | Windows 10 VM，`192.168.3.173:7891`，防火墙放行，计划任务守护 | `/api/status` |
| 网站「下载最新版本」 | 深色落地页，下载最新 exe | <http://192.168.3.173:7891> |
| 旁边「BUG反馈」写入 txt 到 Win10 | 表单 POST → guest 保存 `BUG反馈_*.txt`，并回传 txt | 第 4 节 |
| 自动查看反馈、修复、重发 | DSH 定时任务每 15 分钟检查 `PENDING`；deploy + GitHub Release | 第 5 节 |
| 发布到 GitHub | `https://github.com/Tiantian124446/MCL`，Release 资产 | 第 6 节 |
| 豆包测试 / Marvis 助手 | 豆包已完成评审；Marvis 插件已连接但无可用聊天 API | 第 7 节 |

## 2. 启动器功能

- 首页：版本选择、昵称、内存滑块、开始游戏、安装/更新、检查更新、官网/反馈
- 版本列表：搜索、分类、仅看已安装、安装/更新/删除、安装 Fabric/Forge、已安装版本合并
- 游戏设置：目录、Java 路径/扫描/切换、内存、下载源、自动备份、版本隔离
- 游戏选项：可视化编辑 `options.txt`，保留未知项
- 账户：离线昵称添加/使用/删除，持久化
- 模组下载：MC百科搜索、Modrinth 兜底、版本/加载器筛选、下载
- 模组管理：名称筛选、单项/全部启用禁用、删除、刷新、打开目录
- 资源管理：8 类资源计数、打开目录、查看 latest.log、分析最新崩溃
- 资源下载：Modrinth 资源包/光影搜索与一键下载
- 存档管理：列表、备份、恢复最近备份、删除、打开
- 联机：服务器列表、添加/编辑/删除、显示切换、一键直连、TCP 测速
- 关于：下载最新版（可自更新）、访问官网、BUG反馈、导出诊断信息
- 基础设施：单实例、`launcher.log` 文件日志、启动前 Java 版本检查、继承版本（Fabric/Forge）、natives 解压/扁平化

## 3. 验证记录

- 12 个导航页面逐一点击：**12/12 正常**，进程持续响应，无崩溃
- Fabric 1.20.1：出现 `java` 进程，窗口标题 `Minecraft* 1.20.1`
- Forge 1.20.1：日志进入 Render thread，完成 LWJGL/OpenAL/资源图集初始化
- 资源下载：`Faithful 64x` 19,411,638 字节，15 秒内完成
- 自更新：`v1.26.0` → `v1.26.1`，替换前后 SHA256 变化，新进程启动
- 拖拽导入：`.jar` → mods，`.zip` → shaderpacks（按名称判断），日志各 1 个
- 无效 Java 路径 `Z:\missing\java.exe` 自动修正为 Adoptium Java 17
- 存档恢复：当前存档改名 `.before_restore_*`，备份内容恢复成功
- 设置页/存档页/资源页/12 项侧边栏布局无重叠

## 4. 反馈写入 Win10

- 网站表单：`POST /feedback`（`application/x-www-form-urlencoded`）
- guest 保存：`C:\MCLServer\feedback\BUG反馈_<时间>.txt` / `BUG反馈.txt`
- 响应：返回 txt 供下载；同时 POST 到主机 `http://192.168.3.155:8098/feedback`
- 限流：同 IP 60 秒 1 条（429），请求体 > 20KB（413）
- 管理页：`/admin`（token）→ 查看、搜索、10 条/页、导出 CSV

## 5. 自动修复闭环

- DSH 定时任务 `schedule-b741b6d1-da18-4f72-ae33-ee50a2dea856`，每 15 分钟
- 步骤：读 `PENDING` → 改名 `PROCESSING` → 读反馈 → 复现/修复 → `deploy.ps1` → `gh.ps1` push/release → 写 `fix_result_<id>.md` → 清理标记
- 已处理：`20261122_000219`（v1.1.1），记录 `docs/fixes/20261122_000219.md`
- 队列当前：无 PENDING / PROCESSING

## 6. 发布与校验

- 站点 `/api/status`：版本、大小、更新时间、反馈数、服务器地址
- 每次发布校验：站点下载 = GitHub 资产 = 本地构建（SHA256）
- v1.27.1：`F5D4EC1927D0561D2598B14E284ADF01C37C6BEB5A01F8A70E71EB74CC4FD219`，1,620,133 字节
- GitHub Release：<https://github.com/Tiantian124446/MCL/releases>

## 7. 协作与限制

- **豆包**：完成云端评审，提出反馈认证/限流建议；其环境无法访问私有 LAN IP，网络结论有局限。
- **Marvis**：Edge 插件已安装并显示「已连接」；popup 仅有连接状态；桌面端无独立聊天窗口；本地服务（MarvisHost 39099/10123、Knowledgebase 5151 等）均为内部/鉴权接口，未找到可直接调用的聊天 API。评审未完成。
- **Microsoft 登录**：需要可用的 Azure 客户端 ID，本轮未接入；当前使用离线账户。
- **模组更新检查**：未实现（需要 Modrinth hash 接口与额外 UI）。
- **Modrinth 直连**：资源/模组下载使用 Modrinth API + CDN，网络慢时耗时较长；已通过 `version?limit=1` 避免大响应挂起。

## 8. 运维入口

- 站点：<http://192.168.3.173:7891>
- 管理页：<http://192.168.3.173:7891/admin>
- 状态：<http://192.168.3.173:7891/api/status>
- 部署：`E:\MCL\tools\deploy.ps1 -Version <新版本> -Notes <说明>`
- GitHub：`E:\MCL\tools\gh.ps1 -Action push|release|syncasset`
- 状态手册：`PROJECT_STATE.md`
