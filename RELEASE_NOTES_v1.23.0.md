# MCL 启动器 v1.23.0

## 核心修复：继承版本（Fabric / Forge）

之前 Fabric/Forge 版本只能安装、无法启动，核心启动器只读取子版本 JSON，缺少：

- 父版本 `libraries`（LWJGL、authlib 等）
- 父版本 `arguments`（jvm/game 参数）
- 父版本 `assetIndex` / `assets` / `logging` / `downloads`
- 父版本主 jar

v1.23.0 在 `Launcher::launch` 中：

1. 解析到 `inheritsFrom` 时读取父版本 JSON
2. 合并父版本库、参数、资源索引、日志配置等
3. 子版本没有 jar 时使用父版本 `versions/<parent>/<parent>.jar`
4. 启动时若 `natives/lwjgl.dll` 缺失，扫描 `:natives-windows` 分类库并 `tar` 解压到 `versions/<id>/natives`
5. 安装流程同样支持 `:natives-windows` 分类库（后续安装不再缺 natives）

## 验证

- 构建成功：1573992 字节
- **Fabric 1.20.1**：启动后出现 `java` 进程，窗口标题 `Minecraft* 1.20.1`
- **Forge 1.20.1**：日志进入 Render thread，完成 LWJGL/OpenAL/资源图集初始化（随后在本机环境退出，无 launcher 错误）
- 启动前不再报「未找到游戏主程序」
- 测试后设置恢复 `version=1.20.1`
- `deploy.ps1` 已发布到 Win10，`/api/status` 返回 `v1.23.0`
- 站点下载与 GitHub Release 资产 SHA256 一致

## 兼容

- 版本号统一为 v1.23.0。
- 设置校验、下载最新版、单实例、文件日志、Forge/Fabric 安装、已安装版本合并、版本隔离、启动前 Java 检查、存档恢复、游戏选项、自动备份、服务器测速、账户、模组筛选、版本搜索、诊断信息、崩溃分析、资源管理、反馈闭环均保留。
