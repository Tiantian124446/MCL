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

Marvis 是腾讯的 AI 浏览器助手，可访问本机 LAN 站点。下一轮将让 Marvis 打开 <http://192.168.3.173:7891> 做一次界面/可用性评审，并把建议合并到启动器与官网。

## 验证记录

- `http://192.168.3.173:7891`：200，页面正常（Edge headless 截图）
- `http://192.168.3.173:7891/api/status`：v1.1.0 / size 1296953 / feedbackCount
- `http://192.168.3.173:7891/download/latest`：1296953 字节，SHA256 `037BCBF4...5145D`
- GitHub Release 资产：1296953 字节，SHA256 同上
- 反馈：`BUG反馈.txt` 中文正常，主机 watcher 落盘并生成 PENDING
