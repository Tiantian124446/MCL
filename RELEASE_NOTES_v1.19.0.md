# MCL 启动器 v1.19.0

## 新增：一键安装 Forge

「版本列表」工具栏新增 **安装 Forge**：

- 以当前选中版本作为 MC 版本
- 从 `files.minecraftforge.net/…/promotions_slim.json` 读取推荐版本（无推荐则用 latest）
- 下载 `maven.minecraftforge.net` 安装器（约 9MB）
- 自动确保无 BOM 的 `launcher_profiles.json`
- 后台运行 `java -jar forge-installer.jar --installClient <游戏目录>`（约 1–2 分钟）
- 安装完成后合并 `<mc>-forge-<forge>` 到版本列表，日志/状态栏显示结果
- 失败时弹窗并恢复按钮状态

## 验证

- 构建成功：1532187 字节
- 实测 `1.20.1` → Forge **47.4.28**，安装器退出码 0
- 版本目录 `1.20.1-forge-47.4.28` 生成，JSON 16.6KB
- 日志：「Forge 安装完成: 1.20.1-forge-47.4.28，请在版本列表选择并启动」
- 本地已安装 5 个版本，状态栏显示「Forge 安装完成」
- `deploy.ps1` 已发布到 Win10，`/api/status` 返回 `v1.19.0`
- 站点下载与 GitHub Release 资产 SHA256 一致

## 兼容

- 版本号统一为 v1.19.0。
- Fabric 安装、已安装版本合并、版本隔离、启动前 Java 检查、存档恢复、游戏选项、自动备份、服务器测速、账户、模组筛选、版本搜索、诊断信息、崩溃分析、资源管理、反馈闭环均保留。
