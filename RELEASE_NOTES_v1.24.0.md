# MCL 启动器 v1.24.0

## 新增：拖拽导入

把文件拖进启动器窗口即可导入：

- `.jar` → `<游戏目录>/mods`（版本隔离时使用 `versions/<版本>/mods`）
- `.zip` → 资源包或光影包：
  - 文件名含 `shader` / `光影` / `bsl` / `complementary` / `sildur` / `seus` → `shaderpacks`
  - 其他 → `resourcepacks`
- 重名自动生成 `名称 (1).zip` / `名称 (1).jar`
- 完成后弹窗并写日志：模组 / 资源包 / 光影 各导入多少

## 验证

- 构建成功：1582642 字节
- 合成 `WM_DROPFILES`（`testdrop.jar` + `testdrop_shader.zip`）：
  - `mods/testdrop.jar` → 存在
  - `shaderpacks/testdrop_shader.zip` → 存在
  - 日志：`已导入：模组 1 个，资源包 0 个，光影 1 个`
- 测试文件已清理
- `deploy.ps1` 已发布到 Win10，`/api/status` 返回 `v1.24.0`
- 站点下载与 GitHub Release 资产 SHA256 一致

## 兼容

- 版本号统一为 v1.24.0。
- 继承版本（Fabric/Forge）、natives 扁平化、加载器 Java 检查、设置校验、下载最新版、单实例、文件日志、版本隔离、存档备份/恢复、游戏选项、自动备份、服务器测速、账户、模组筛选、版本搜索、诊断、崩溃分析、资源管理、反馈闭环均保留。
