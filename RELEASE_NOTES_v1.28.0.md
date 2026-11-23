# MCL 启动器 v1.28.0

## 新图标

- 应用图标换成用户提供的绿色草方块（`res/mcl.ico`，16/24/32/48/64/128/256 多尺寸）
- `src/mcl.rc` + `resource.h` 编入 exe：资源管理器、任务栏、窗口标题栏统一使用
- 构建脚本新增 `windres` 步骤

## 服务器地址

- 发布服务器从 `http://192.168.3.173:7891` 改为 **https://pocket4-MCL.com**
- 域名不可达时自动回退 `http://192.168.3.173:7891`，并在日志中记录实际使用的地址
- 更新检查、下载最新版、官网/反馈、崩溃分析入口、诊断信息统一走同一套基地址

## 首次运行自动下载配置

- 本机没有任何 `mcl_settings.txt` 时：先内置默认值初始化，再请求
  `https://pocket4-MCL.com/api/default-config`，成功则应用并保存，日志记录来源
- 服务器端新增 `/api/default-config`（读取 `C:\MCLServer\default_config.txt`，缺失时返回内置默认值）
- 服务器不可达时仍按内置默认值生成配置，不阻塞启动

## 自恢复保护（防课堂管理软件清理）

- 启动时把自己备份到 `%LOCALAPPDATA%\MCL\guard\MCLauncher.exe`，并记录主程序路径
- 注册 `HKCU\...\Run\MCLGuard = "…guard\MCLauncher.exe" --guard`
- 登录时若主程序被删除：备份拷贝回原路径并用 `CreateProcess` 启动（避免安全警告弹窗）
- 「关于」页新增 **删除自恢复** 按钮：用户主动删除保护（移除备份与自启项）
- 日志落盘 `MCL.log`（本轮同时修复了日志中文乱码：`%s` → `%ls`）

## UI 修复（上一轮遗留）

- 首页「玩家昵称」标签不再被输入框遮挡；卡片加高，输入框与滑块底部留白
- 侧边栏副标题与底部版本信息不再被省略号截断（两行显示）
- 版本列表工具栏「搜索」标签右对齐，紧贴搜索框

## 服务器与域名（Win10 虚拟机）

- **未改动主机任何网卡**：WLAN 保持 DHCP（192.168.3.173 由路由器分配）
- 虚拟机此前残留静态 IP `192.168.3.173`（在 NAT 网段里不可达）——本轮在 guest 内恢复 DHCP，
  现在为 `192.168.201.129`，网关/DNS `192.168.201.2`
- 主机 `netsh portproxy`：`192.168.3.173:7891 → 192.168.201.129:7891`（局域网回退地址）
- 主机 Node 反向代理 `host/https_proxy.js`：`https://pocket4-MCL.com:443 → guest:7891`
  - 绑定时只监听主机 WLAN 地址（`0.0.0.0:443` 被 Steam++ 占用）
  - 自签证书 `host/cert/mcl.pfx`（SAN: pocket4-MCL.com / www.pocket4-MCL.com），已加入本机
    受信任根；计划任务 `MCLProxy` 开机自启
- hosts：`192.168.3.173 pocket4-MCL.com / www.pocket4-MCL.com`
- 主机 IP 或 VM IP 变化后运行 `tools/update_domain.ps1` 一键对齐

## 已知限制

- `pocket4-MCL.com` **没有注册**（DNS 查询为 NXDOMAIN），因此目前只在本机通过 hosts 生效；
  校园网/其它电脑要用这个域名，必须先注册域名并把 A 记录指向公网入口（或每台机器加 hosts + 信任证书）
- 自签证书只能在本机受信任；启动器对自建服务器跳过证书校验，浏览器在其它机器会提示不安全
- 课堂管理软件的删除策略无法被程序完全对抗：自恢复保护是“被删后自动还原”，不是“无法删除”

## 验证

- 构建：`build\MCLauncher.exe` 1,658,263 字节（含图标资源）
- 部署：`tools/deploy.ps1 -Version 1.28.0` → 服务器 `/api/status` 返回 v1.28.0
- 首次运行测试：干净目录启动 → 请求 `/api/default-config` → 生成 `mcl_settings.txt`（服务器日志可见）
- 自恢复测试：删除主程序 → `--guard` → 主程序恢复并启动，无安全警告
- 域名测试：`https://pocket4-MCL.com/api/status` 200；`http://192.168.3.173:7891/api/status` 200
