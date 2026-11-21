$ErrorActionPreference = 'Stop'
$p = 'E:\MCL\src\ui.cpp'
$u = ([IO.File]::ReadAllText($p, [Text.Encoding]::UTF8)) -replace "`r`n", "`n"
function Rep([string]$old, [string]$new) {
    $old = ($old -replace "`r`n", "`n")
    $new = ($new -replace "`r`n", "`n")
    $count = ([regex]::Matches($script:u, [regex]::Escape($old))).Count
    if ($count -ne 1) { throw "count $count for: $($old.Substring(0, [Math]::Min(70, $old.Length)))" }
    $script:u = $script:u.Replace($old, $new)
}

Rep '#include "ui.h"' @'
#include <winsock2.h>
#include <ws2tcpip.h>

#include "ui.h"
'@

Rep '    BTN_SRV_VIS_BASE       = 750000,   // +列表下标: 切换"显示服务器 / 不显示服务器"' @'
    BTN_SRV_VIS_BASE       = 750000,   // +列表下标: 切换"显示服务器 / 不显示服务器"
    BTN_SRV_PING_BASE      = 760000,   // +列表下标: TCP 测速
'@

Rep '    std::vector<ServerEntry> servers_;' @'
    std::vector<ServerEntry> servers_;
    std::map<std::string, int> srvPingMs_;   // host:port -> ms; -1 超时, -2 测试中
'@

Rep '    void connectServer(int idx);         // 一键启动并直连该服务器' @'
    void connectServer(int idx);         // 一键启动并直连该服务器
    void pingServerAsync(int idx);       // TCP 连接测速
'@

Rep '    if (id >= BTN_SAVE_DEL_BASE && id < BTN_SAVE_DEL_BASE + 1000) {' @'
    if (id >= BTN_SRV_PING_BASE && id < BTN_SRV_PING_BASE + 1000) {
        pingServerAsync(id - BTN_SRV_PING_BASE);
        return;
    }
    if (id >= BTN_SAVE_DEL_BASE && id < BTN_SAVE_DEL_BASE + 1000) {
'@

Rep @'
    std::vector<ServerEntry> items;
    int sel = -1;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        items = servers_;
        sel = srvSel_;
    }
'@ @'
    std::vector<ServerEntry> items;
    std::map<std::string, int> pings;
    int sel = -1;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        items = servers_;
        pings = srvPingMs_;
        sel = srvSel_;
    }
'@

Rep @'
        bool hover = (hoverId_ == BTN_SRV_ROW_BASE + ri ||
                      hoverId_ == BTN_SRV_CONNECT_BASE + ri ||
                      hoverId_ == BTN_SRV_EDIT_BASE + ri ||
                      hoverId_ == BTN_SRV_DEL_BASE + ri ||
                      hoverId_ == BTN_SRV_VIS_BASE + ri);
'@ @'
        bool hover = (hoverId_ == BTN_SRV_ROW_BASE + ri ||
                      hoverId_ == BTN_SRV_CONNECT_BASE + ri ||
                      hoverId_ == BTN_SRV_EDIT_BASE + ri ||
                      hoverId_ == BTN_SRV_DEL_BASE + ri ||
                      hoverId_ == BTN_SRV_PING_BASE + ri ||
                      hoverId_ == BTN_SRV_VIS_BASE + ri);
'@

Rep '        int barW = 352;   // 右侧按钮区宽度 (4 个按钮)' '        int barW = 420;   // 右侧按钮区宽度 (5 个按钮)'

Rep @'
        int by = ry + (rowH - 6 - bh) / 2;
        drawButton(g, mkRect(rr.right - 344, by, rr.right - 224, by + bh),
'@ @'
        int by = ry + (rowH - 6 - bh) / 2;
        std::string pkey = wideToUtf8(it.host) + ":" + std::to_string(it.port);
        auto pit = pings.find(pkey);
        int ping = (pit == pings.end()) ? 0 : pit->second;
        std::wstring pingText = L"测速";
        if (ping == -2) pingText = L"…";
        else if (ping == -1) pingText = L"超时";
        else if (ping > 0) pingText = std::to_wstring(ping) + L"ms";
        drawButton(g, mkRect(rr.right - 412, by, rr.right - 352, by + bh), pingText,
                   BTN_SRV_PING_BASE + ri, false, ping != -2, 11);
        drawButton(g, mkRect(rr.right - 344, by, rr.right - 224, by + bh),
'@

$fn = @'
static int tcpPingMs(const std::wstring& host, int port, int timeoutMs) {
    static std::once_flag once;
    std::call_once(once, []() {
        WSADATA wd{};
        WSAStartup(MAKEWORD(2, 2), &wd);
    });
    std::string h = wideToUtf8(host);
    std::string ps = std::to_string(port);
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    addrinfo* res = nullptr;
    if (getaddrinfo(h.c_str(), ps.c_str(), &hints, &res) != 0) return -1;
    int best = -1;
    for (addrinfo* ai = res; ai; ai = ai->ai_next) {
        SOCKET s = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
        if (s == INVALID_SOCKET) continue;
        u_long nb = 1;
        ioctlsocket(s, FIONBIO, &nb);
        unsigned long long t0 = GetTickCount64();
        int rc = connect(s, ai->ai_addr, (int)ai->ai_addrlen);
        if (rc == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK) {
            fd_set wf;
            FD_ZERO(&wf);
            FD_SET(s, &wf);
            timeval tv{ timeoutMs / 1000, (timeoutMs % 1000) * 1000 };
            rc = select(0, nullptr, &wf, nullptr, &tv);
            if (rc > 0) {
                int err = 0, len = sizeof(err);
                getsockopt(s, SOL_SOCKET, SO_ERROR, (char*)&err, &len);
                rc = (err == 0) ? 0 : SOCKET_ERROR;
            } else {
                rc = SOCKET_ERROR;
            }
        }
        if (rc == 0) {
            int ms = (int)(GetTickCount64() - t0);
            if (ms < 0) ms = 0;
            if (best < 0 || ms < best) best = ms;
        }
        closesocket(s);
    }
    freeaddrinfo(res);
    return best;
}

void MainWindow::pingServerAsync(int idx) {
    std::wstring host;
    int port = 0;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (idx < 0 || idx >= (int)servers_.size()) return;
        host = servers_[idx].host;
        port = servers_[idx].port;
    }
    std::string key = wideToUtf8(host) + ":" + std::to_string(port);
    {
        std::lock_guard<std::mutex> lk(mtx_);
        srvPingMs_[key] = -2;
    }
    dirty_ = true;
    std::thread([this, host, port, key]() {
        int ms = tcpPingMs(host, port, 1500);
        {
            std::lock_guard<std::mutex> lk(mtx_);
            srvPingMs_[key] = ms;
        }
        dirty_ = true;
        if (hwnd_) PostMessageW(hwnd_, WM_TIMER, 0, 0);
    }).detach();
}

'@
Rep 'void MainWindow::drawServers(Graphics& g, int W, int H) {' ($fn + 'void MainWindow::drawServers(Graphics& g, int W, int H) {')

Rep 'static const wchar_t* kAppVersion = L"1.11.0";' 'static const wchar_t* kAppVersion = L"1.12.0";'

[IO.File]::WriteAllText($p, $u, (New-Object Text.UTF8Encoding($false)))
"server ping patched, length=$($u.Length)"
