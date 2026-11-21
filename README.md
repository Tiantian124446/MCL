# MCL 启动器 · Minecraft Java Edition

一个用 C++ / Win32 / GDI+ 编写的 Minecraft Java 版启动器，界面参考 PCL2，由 DSH 主设计、Marvis 协助、豆包测试。

- 官网 / 下载 / BUG 反馈：<http://192.168.3.173:7891>（Windows 10 服务器）
- 最新版本：[GitHub Releases](https://github.com/Tiantian124446/MCL/releases)
- 反馈文件会在服务器上保存为 `BUG反馈.txt`，并自动进入修复与重新发布流程。

## 功能

- 版本清单：BMCLAPI 国内镜像 / Mojang 官方源，支持正式版、快照、远古版、愚人节版分类
- 版本搜索：版本列表输入即过滤，与分类 / 仅看已安装叠加
- 版本安装：客户端、依赖库、资源文件、natives 解压，多线程下载与进度显示；一键安装 Fabric / Forge，已安装的加载器版本会合并进列表
- 启动：离线模式、自动 Java 探测、启动前 Java 版本匹配检查、内存推荐、自定义游戏目录与 JVM 路径
- 版本隔离：开启后每个版本使用独立的 mods/saves/资源包/日志/备份目录
- 模组下载：MC 百科搜索、Modrinth 兜底、版本/加载器筛选、一键下载
- 联机：服务器列表、公开清单导入、一键启动并直连
- 资源管理：资源包 / 光影包 / 存档 / 截图 / 模组 / 日志 / 崩溃报告 / 版本，计数与一键打开文件夹
- 模组管理：已安装模组列表、名称筛选、单项/全部启用禁用（`.jar` ↔ `.jar.disabled`）、删除、刷新、打开目录
- 存档管理：存档列表、备份到 backups、恢复最近备份（先改名保护）、删除、打开/刷新
- 自动备份：启动时 24 小时内无 backups/auto_* 则后台复制整个 saves
- 崩溃分析：读取 `crash-reports` 或 `latest.log`，识别内存 / Java / 模组 / OpenGL 等常见问题（F9）
- 诊断信息：关于页一键导出系统 / Java / 目录 / 版本 / 模组 / 存档 / 日志 / 崩溃路径
- 更新：启动时自动检查 Win10 发布服务器版本，官网 / 反馈入口
- 界面：自绘 UI、等比缩放、全屏（F11）、深色日志区
- 单实例保护：重复启动会激活旧窗口并退出
- 文件日志：launcher.log 带时间戳，超过 4MB 自动转 .old

## 构建

需要 MinGW-w64 g++（支持 C++17）：

```powershell
g++ -std=c++17 -O2 -municode -mwindows `
  -finput-charset=UTF-8 -fexec-charset=UTF-8 -fwide-exec-charset=UTF-16LE `
  -o build\MCLauncher.exe `
  src\main.cpp src\ui.cpp src\core.cpp src\http.cpp src\util.cpp `
  -lgdiplus -lwinhttp -lcrypt32 -lole32 -loleaut32 -luuid -lshlwapi -ldwmapi -lcomctl32 -lws2_32 -static
```

或运行 `build.ps1`。

## 发布服务器（Win10）

`server/payload/` 是部署到 Windows 10 虚拟机的完整包：

- `server.ps1`：基于 TcpListener 的轻量 HTTP 服务（端口 7891）
- `web/index.html`：深色落地页，含「下载最新版本」「BUG反馈」
- `bootstrap.ps1`：在 guest 内提权运行，下载包、复制文件、加防火墙、建计划任务
- `watchdog.ps1`：每 5 分钟检查服务并重启
- 反馈保存为 `C:\MCLServer\feedback\BUG反馈_*.txt`，并 POST 到主机 `:8098/feedback`
- 管理页：`/admin` → 输入 `C:\MCLServer\admin_token.txt` 中的 token 查看反馈记录

## 反馈 → 修复 → 重新发布

1. 用户在网站提交 BUG 反馈，guest 保存 `BUG反馈.txt` 并通知主机 watcher。
2. 主机 `host/feedback_server.js` 落盘到 `E:\MCL\feedback`，写入 `PENDING`。
3. DSH 检查反馈，定位并修复启动器/服务器问题。
4. 重新构建、更新 `version.json`、打包部署到 Win10，并发布到 GitHub Releases。

## 角色

- DSH：主设计师 / 主干实现 / 发布
- Marvis：助手 / 辅助评审
- 豆包：测试 / 反馈验证
