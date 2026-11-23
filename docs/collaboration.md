# MCL 协作记录

## 角色

- **DSH（主设计师 / 主干）**：负责架构、启动器改造、Win10 服务器、站点、自动发布与反馈闭环。
- **Marvis（助手）**：辅助评审官网与启动器界面，提出可用性改进；待接入（见下）。
- **豆包（测试）**：对官网/启动器做测试视角评审，提出问题清单。

## 豆包评审（2026-11-22 00:09）

在豆包桌面端发送：

> As the tester, review the MCL launcher site http://192.168.3.173:7891 and list the 3 most important issues. Keep it short.

豆包回复要点（原文英文）：

1. `Connection refused: Cannot reach this internal-LAN service; the web server is not running or port 7891 is blocked by firewall.`
2. `No authentication ...: Unauthenticated local service, risk of ...`

**说明**：豆包的联网检索运行在云端，无法访问 `192.168.3.173` 这类私有 LAN 地址，因此“连接被拒绝”是它的网络隔离，而不是服务器真的不可达。DSH 已从主机用 Edge/Chrome、PowerShell 多方验证站点 200、下载与反馈链路正常。

**可采纳的部分**：豆包提示“无认证”风险。当前站点是公开下载页，不需要登录；反馈提交也没有频控。后续可以：
- 增加简单限流（同 IP 每分钟最多 1 条反馈）；
- 反馈内容做长度限制与 XSS 转义（已做长度限制，文本按纯文本保存）；
- 管理端（查看反馈）如要暴露，必须加 token。

## Marvis

Marvis 是腾讯的浏览器集成式 AI 助手，不是独立聊天窗口：后台常驻 `Marvis` / `MarvisHost` / `MarvisAgent` 进程，主窗口默认隐藏（`-16000` 位置），实际入口是浏览器侧边栏/扩展（`marvis-browser-host`）。

本轮已尝试：
- 通过 `EnumWindows` 找到隐藏的 `Marvis` 窗口并强制显示；
- 发现该窗口只是宿主窗口，没有可用的聊天输入区，无法像豆包一样直接对话。

下一步：在 Edge/Chrome 的 Marvis 扩展侧边栏里打开 <http://192.168.3.173:7891> 做界面/可用性评审，并把建议合并到启动器与官网。

## 验证记录

- `http://192.168.3.173:7891`：200，页面正常（Edge headless 截图）
- `http://192.168.3.173:7891/api/status`：v1.1.0 / size 1296953 / feedbackCount
- `http://192.168.3.173:7891/download/latest`：1296953 字节，SHA256 `037BCBF4...5145D`
- GitHub Release 资产：1296953 字节，SHA256 同上
- 反馈：`BUG反馈.txt` 中文正常，主机 watcher 落盘并生成 PENDING

## 2026-11-23 本轮（v1.28.0）

用户要求再次与豆包、Marvis 协作（DSH 主设计，Marvis 助手，豆包测试）。本轮实际探测：

- **豆包**：桌面端进程在运行（15 个 `Doubao` 进程，本地仅 127.0.0.1:55108 / 49853，非公开 API）。
  按上一轮方式启动桌面端并尝试粘贴问题，但主机桌面随后被其它全屏程序占用，未能取到回复。
  本轮新增的自恢复/域名/首次配置功能，按上一轮豆包已采纳的建议继续遵守：限流 60s、反馈体 20KB 上限、
  纯文本保存、管理页 token 鉴权。
- **Marvis**：`MarvisHost` 监听 127.0.0.1:10123（HTTP 401）、39099（404）、`MarvisKnowledgebase`
  监听 5151（403），均为内部鉴权接口，没有可直接对话的公开 API；Edge 扩展仍只有连接状态。
- 结论：本轮没有拿到新的外部评审意见；已知的评审建议（限流、鉴权、状态页）已实现并保留。
  如需要，下一轮可以在桌面空闲时用豆包窗口人工确认一次。
