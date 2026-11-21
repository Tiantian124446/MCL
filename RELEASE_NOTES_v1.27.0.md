# MCL 启动器 v1.27.0

## 新增：资源下载页

侧边栏新增 **资源下载**：

- 输入关键词，选择 **资源包 / 光影**
- 使用 Modrinth 搜索（按下载量排序，20 条）
- 列表显示标题、作者、下载量、简介
- 点「下载最新」：
  - 请求 `/project/<id>/version?limit=1`
  - 选择 primary 文件
  - 下载到 `resourcepacks` 或 `shaderpacks`（版本隔离时用 `versions/<版本>/`）
  - 进度显示在列表右上角，完成写日志
- 「打开文件夹」直达目标目录
- 侧边栏调整为 12 项（行高 42），640 高度下不重叠

## 验证

- 构建成功：1619109 字节
- 搜索 `faithful` → 20 条，首项 `Faithful 64x`（作者 Faithful-Resource-Pack，下载 7781151）
- 点「下载最新」→ 15 秒内完成
- 文件：`resourcepacks/Faithful 64x - Release 15.zip`，19,411,638 字节
- 日志：`资源下载完成: Faithful 64x -> …`
- 测试文件已清理
- `deploy.ps1` 已发布到 Win10，`/api/status` 返回 `v1.27.0`
- 站点下载与 GitHub Release 资产 SHA256 一致

## 兼容

- 版本号统一为 v1.27.0。
- 自更新、拖拽导入、Modrinth 搜索入口、继承版本（Fabric/Forge）、natives 扁平化、加载器 Java 检查、设置校验、单实例、文件日志、版本隔离、存档备份/恢复、游戏选项、自动备份、服务器测速、账户、模组筛选、版本搜索、诊断、崩溃分析、反馈闭环均保留。
