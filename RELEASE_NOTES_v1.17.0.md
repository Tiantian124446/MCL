# MCL 启动器 v1.17.0

## 新增：版本隔离

「游戏设置」游戏目录卡片新增 **版本隔离** 复选框：

- 开启后，模组、存档、资源包、光影包、截图、日志、崩溃报告、备份等数据目录切换到 `versions/<当前版本>/`
- 「版本」资源卡片仍指向全局 `versions`，不会自我嵌套
- 启动时自动创建 `mods`、`saves`、`resourcepacks`、`shaderpacks`
- 现有全局数据**不会自动迁移**，关闭开关即可回到全局 `.minecraft`
- 设置持久化到 `mcl_settings.txt`（`isolation=1/0`）
- 启动、一键进服、诊断、崩溃分析、自动备份、存档恢复都使用实际数据目录

## 验证

- 构建成功：1496508 字节
- 开启隔离后，模组管理路径显示 `E:\MCL\.minecraft\versions\1.20.1\mods`，并列出测试 `iso_test.jar`
- 关闭后 `mcl_settings.txt` 恢复 `isolation=0`，测试文件已清理
- `deploy.ps1` 已发布到 Win10，`/api/status` 返回 `v1.17.0`
- 站点下载与 GitHub Release 资产 SHA256 一致

## 兼容

- 版本号统一为 v1.17.0。
- 启动前 Java 检查、存档恢复、游戏选项、自动备份、服务器测速、账户、Java 扫描、模组筛选、版本搜索、诊断信息、崩溃分析、资源管理、反馈闭环均保留。
