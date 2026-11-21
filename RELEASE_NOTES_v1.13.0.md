# MCL 启动器 v1.13.0

## 新增：启动时自动备份存档

- 「游戏设置」下载源卡片新增 **自动备份存档** 复选框，默认开启
- 启动器启动后立即检查：若 `backups` 下 24 小时内没有 `auto_*` 备份，则后台复制整个 `saves`
- 备份目录：`<游戏目录>/backups/auto_YYYYMMDD_HHMMSS/`
- 复制使用 `SHFileOperationW`，静默执行，不弹确认框
- 结果写入启动日志；设置持久化到 `mcl_settings.txt`（`autobackup=1/0`）

## 验证

- 构建成功：1437528 字节
- 首次启动生成 `backups/auto_20261122_014826/新的世界`，内容完整
- 重启启动器后未重复生成（24 小时节流生效）
- 复选框切换：`autobackup=0` ↔ `autobackup=1`，持久化正常
- `deploy.ps1` 已发布到 Win10，`/api/status` 返回 `v1.13.0`
- 站点下载与 GitHub Release 资产 SHA256 一致

## 兼容

- 版本号统一为 v1.13.0。
- 服务器测速、账户、Java 扫描、模组筛选、版本搜索、诊断信息、存档管理、崩溃分析、资源管理、反馈闭环均保留。
