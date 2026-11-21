# MCL 启动器 v1.10.0

## 新增：Java 扫描与切换

「游戏设置」页新增 **已检测到的 Java** 卡片：

- 点「扫描」/「扫描 Java」查找常见安装位置：
  - Eclipse Adoptium、Oracle Java、Microsoft JDK、Zulu、BellSoft
  - `D:\jdk`、`D:\java`、`E:\jdk`
  - 当前已填写的 Java 路径
- 显示路径与主版本（Java 8/17/21/25…），按主版本降序
- 点「使用」立即切换 Java 并保存设置
- 扫描在后台线程执行，不阻塞界面；最多展示 8 个
- 未扫描时显示操作提示

## 验证

- 构建成功：1404447 字节
- 本机扫描到 `C:\Program Files\Eclipse Adoptium\jre-17.0.20.8-hotspot\bin\java.exe`（Java 17）
- 点「使用」后进程正常响应，路径写入设置并保存
- `deploy.ps1` 已发布到 Win10，`/api/status` 返回 `v1.10.0`
- 站点下载与 GitHub Release 资产 SHA256 一致

## 兼容

- 版本号统一为 v1.10.0。
- 模组筛选/批量启用禁用、版本搜索、诊断信息、存档管理、崩溃分析、资源管理、反馈闭环均保留。
