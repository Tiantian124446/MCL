# MCL 启动器 v1.23.2

## 修复：加载器版本的 Java 检查

启动前 Java 检查之前直接对版本 ID 做 `sscanf("%d.%d.%d")`：

- `fabric-loader-0.20.1-1.21.1` 解析成 0.0.0 → 误判只需 Java 8
- `1.20.1-forge-47.4.28` 可解析，但不稳
- NeoForge/Quilt 同类问题

v1.23.2 新增 `baseMcVersionFromId()`：

- `fabric-loader-…-<mc>` / `quilt-loader-…-<mc>` 取最后一段 MC 版本
- `<mc>-forge-…` 取 `-forge-` 前
- `neoforge-<major>.<minor>.*` 映射为 `1.<major>.<minor>`

## 验证

- 构建成功：1576299 字节
- 设置 `fabric-loader-0.20.1-1.21.1`（本机未安装该版本）+ Java 17，点击开始：
  - 提示「版本 fabric-loader-0.20.1-1.21.1 需要 Java 21，当前选择的是 Java 17」
  - 不再误判为 Java 8
- 测试后 `mcl_settings.txt` 已恢复
- `deploy.ps1` 已发布到 Win10，`/api/status` 返回 `v1.23.2`
- 站点下载与 GitHub Release 资产 SHA256 一致

## 兼容

- 版本号统一为 v1.23.2。
- 继承版本（Fabric/Forge）、natives 扁平化、Forge/Fabric 安装、设置校验、下载最新版、单实例、文件日志、版本隔离、存档备份/恢复、游戏选项、自动备份、服务器测速、账户、模组筛选、版本搜索、诊断、崩溃分析、资源管理、反馈闭环均保留。
