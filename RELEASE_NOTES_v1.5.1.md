# MCL 启动器 v1.5.1

## 体积优化

- 存档备份 / 删除由 `std::filesystem` 改为 Win32 `SHFileOperationW`。
- 可执行文件：**3.6 MB → 1.36 MB**（1,358,447 字节）。
- 发布包：**883 KB → 423 KB**。
- 功能与 v1.5.0 完全一致：存档列表、备份、删除、打开、刷新。

## 验证

- 构建成功：1358447 字节
- 备份实测：`z_test_save` → `backups/saves_z_test_save_20261122_005232`，内容递归复制
- 删除实测：确认后 `z_test_save` 目录被移除
- `deploy.ps1` 已发布到 Win10，`/api/status` 返回 `v1.5.1`
- 站点下载与 GitHub Release 资产 SHA256 一致

## 兼容

- 版本号统一为 v1.5.1（关于页 / 页脚 / 服务器 version.json）。
- 模组管理、崩溃分析、资源管理、存档管理、反馈限流均保留。
