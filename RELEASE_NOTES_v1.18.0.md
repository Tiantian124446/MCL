# MCL 启动器 v1.18.0

## 新增：一键安装 Fabric

「版本列表」工具栏新增 **安装 Fabric**：

- 以当前选中版本（首页「游戏版本」）作为 MC 版本
- 自动执行：
  1. 读取 `meta.fabricmc.net` 最新 loader 版本
  2. 读取 `maven.fabricmc.net` 最新安装器版本并下载
  3. 自动创建无 BOM 的 `launcher_profiles.json`（Fabric 安装器要求）
  4. 运行 `fabric-installer client -dir … -mcversion … -loader …`
- 安装过程后台执行，日志/状态栏显示进度；失败弹窗提示
- 安装完成后提示到版本列表选择启动

## 修复：已安装版本不可见

之前版本列表只展示 Mojang 清单，手动安装的 Forge / Fabric 版本在列表中不可见。现在获取清单后会把本地已安装但不在清单中的版本合并为「已安装」，方便选择、启动、删除。

## 验证

- 构建成功：1512694 字节
- 实际安装 `fabric-loader-0.20.1-1.20.1` 成功（安装器退出码 0，版本 JSON 生成）
- 「仅看已安装」显示 4 个：26.4-snapshot-2、1.20.1、1.21.1-Forge、fabric-loader-0.20.1-1.20.1
- 日志显示「Fabric 安装完成」，状态栏不再停留在“正在运行安装器”
- `deploy.ps1` 已发布到 Win10，`/api/status` 返回 `v1.18.0`
- 站点下载与 GitHub Release 资产 SHA256 一致

## 兼容

- 版本号统一为 v1.18.0。
- 版本隔离、启动前 Java 检查、存档恢复、游戏选项、自动备份、服务器测速、账户、Java 扫描、模组筛选、版本搜索、诊断信息、崩溃分析、资源管理、反馈闭环均保留。
