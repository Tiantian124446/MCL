# MCL 启动器 v1.2.0

## 新增：资源管理页

- 侧边栏新增「资源管理」，对齐 PCL2 的本地资源入口。
- 按当前游戏目录统计并展示：
  - 资源包 `resourcepacks`
  - 光影包 `shaderpacks`
  - 存档 `saves`
  - 截图 `screenshots`
  - 模组 `mods`
  - 日志 `logs`
  - 崩溃报告 `crash-reports`
  - 版本 `versions`
- 每类显示条目数量，一键「打开文件夹」；目录不存在时自动创建。
- 顶部显示当前 `.minecraft` 路径，并提供「打开 .minecraft」按钮。

## 验证

- 构建成功：1303133 字节
- `deploy.ps1` 已发布到 Win10，`/api/status` 返回 `v1.2.0`
- 站点下载与 GitHub Release 资产 SHA256 一致
- 运行时已验证：侧边栏「资源管理」可打开，资源包目录按钮可创建并打开 `resourcepacks`

## 兼容

- 启动器版本号统一为 v1.2.0（关于页 / 页脚 / 服务器 version.json）。
- 旧版本 v1.1.1 仍可从历史 Release 下载。
