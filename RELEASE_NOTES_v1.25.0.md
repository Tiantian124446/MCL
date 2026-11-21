# MCL 启动器 v1.25.0

## 新增：资源卡片「Modrinth 搜索]

「资源管理」页：

- **资源包**卡片：新增「Modrinth 搜索」→ 打开 `https://modrinth.com/resourcepacks`
- **光影包**卡片：新增「Modrinth 搜索」→ 打开 `https://modrinth.com/shaders`
- 配合 v1.24.0 的拖拽导入，形成：
  1. 浏览器搜索/下载 `.zip`
  2. 拖进启动器窗口
  3. 自动放入 `resourcepacks` 或 `shaderpacks`

这样绕开应用内 Modrinth API/CDN 的网络问题，仍能完成资源安装。

## 验证

- 构建成功：1583154 字节
- 资源管理页两张卡片显示「Modrinth 搜索」按钮，布局不重叠
- 点击后 Edge 打开 Modrinth 页面（窗口标题 `Modrinth - Microsoft Edge`）
- `deploy.ps1` 已发布到 Win10，`/api/status` 返回 `v1.25.0`
- 站点下载与 GitHub Release 资产 SHA256 一致

## 兼容

- 版本号统一为 v1.25.0。
- 拖拽导入、继承版本（Fabric/Forge）、natives 扁平化、加载器 Java 检查、设置校验、下载最新版、单实例、文件日志、版本隔离、存档备份/恢复、游戏选项、自动备份、服务器测速、账户、模组筛选、版本搜索、诊断、崩溃分析、反馈闭环均保留。
