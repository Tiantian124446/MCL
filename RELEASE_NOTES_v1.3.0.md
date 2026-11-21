# MCL 启动器 v1.3.0

## 新增：崩溃分析

- 「资源管理 → 崩溃报告」新增 **分析最新崩溃** 按钮；任意页面按 **F9** 也可触发。
- 自动读取最新的 `crash-reports/crash-*.txt`，没有崩溃报告时回退到 `logs/latest.log`。
- 识别常见问题并给出可执行建议：
  - 内存不足 `OutOfMemoryError` → 提高最大内存
  - Java 版本不匹配 `UnsupportedClassVersionError` → 切换 Java 8/17/21
  - 模组缺少依赖 / 版本不匹配
  - 模组重复
  - `NoSuchMethodError` / `NoClassDefFoundError` 模组不兼容
  - 显卡 / OpenGL / GLFW 初始化失败
  - 游戏主线程崩溃
- 识别不到时提示把崩溃报告通过官网 BUG反馈 提交。
- 弹窗内可直接选择「是」打开官网反馈页（`/?feedback=1`），完成“崩溃 → 反馈”的闭环。

## 验证

- 构建成功：1312432 字节
- 运行时用合成崩溃报告 `java.lang.OutOfMemoryError: Java heap space` 验证：弹窗正确显示“内存不足”和“游戏主线程崩溃”，并给出建议与反馈入口。
- `deploy.ps1` 已发布到 Win10，`/api/status` 返回 `v1.3.0`。
- 站点下载与 GitHub Release 资产 SHA256 一致。

## 兼容

- 版本号统一为 v1.3.0（关于页 / 页脚 / 服务器 version.json）。
- 资源管理页（v1.2.0）与反馈闭环（v1.1.1）保留。
