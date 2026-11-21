# MCL 启动器 v1.23.1

## 修复：natives DLL 扁平化

v1.23.0 已支持继承版本与 `:natives-windows` 解压，但 `tar` 保留了 `windows/x64/org/lwjgl/...` 目录结构，`java.library.path` 根目录下没有 `lwjgl.dll`，存在再次启动失败的风险。

v1.23.1 在解压后递归扫描 natives 目录，把所有 `.dll` 复制到 `natives` 根目录（覆盖同名旧文件），保证 LWJGL/GLFW/JNA 能按 `java.library.path` 找到。

## 验证

- 构建成功：1575787 字节
- 删除 Fabric 版本 `natives/lwjgl.dll` 后启动：
  - `natives/lwjgl.dll` 重新出现（根目录）
  - 出现 `java` 进程，窗口标题 `Minecraft* 1.20.1`
- 测试后设置恢复 `version=1.20.1`
- `deploy.ps1` 已发布到 Win10，`/api/status` 返回 `v1.23.1`
- 站点下载与 GitHub Release 资产 SHA256 一致

## 兼容

- 版本号统一为 v1.23.1。
- 继承版本（Fabric/Forge）父 JSON/父 jar/参数/库合并、Forge/Fabric 安装、设置校验、下载最新版、单实例、文件日志、版本隔离、Java 检查、存档备份/恢复、游戏选项、自动备份、服务器测速、账户、模组筛选、版本搜索、诊断、崩溃分析、资源管理、反馈闭环均保留。
