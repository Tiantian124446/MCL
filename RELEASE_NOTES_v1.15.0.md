# MCL 启动器 v1.15.0

## 新增：恢复最近备份

「存档管理」信息卡新增 **恢复最近备份**：

- 自动查找 `backups/saves_*` 中最后修改时间最新的备份
- 恢复前把当前存档改名为 `<存档>.before_restore_<时间>`，不会直接覆盖
- 递归复制备份内容回 `saves/<存档>`，文件逐个覆盖
- 复制失败时尝试把原存档改回，避免丢失
- 恢复完成后刷新存档列表并写日志

## 验证

- 构建成功：1464520 字节
- 测试存档 `restore_test`：当前 `a.txt=current`，备份 `a.txt=backup`
- 点「恢复最近备份」并确认后：
  - `saves/restore_test/a.txt` → **backup**
  - `saves/restore_test.before_restore_20261122_020026/a.txt` → **current**
- 测试存档与备份已清理
- `deploy.ps1` 已发布到 Win10，`/api/status` 返回 `v1.15.0`
- 站点下载与 GitHub Release 资产 SHA256 一致

## 兼容

- 版本号统一为 v1.15.0。
- 游戏选项、自动备份、服务器测速、账户、Java 扫描、模组筛选、版本搜索、诊断信息、崩溃分析、资源管理、反馈闭环均保留。
