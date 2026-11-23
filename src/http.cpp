// ---------------------------------------------------------------------------
//  http.cpp  -  WinHTTP 下载实现 (连接复用版)
// ---------------------------------------------------------------------------
#include "http.h"
#include "util.h"

#include <vector>
#include <cstdio>

#ifndef WINHTTP_OPTION_SECURE_PROTOCOLS
#define WINHTTP_OPTION_SECURE_PROTOCOLS 84
#endif
#ifndef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2
#define WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 0x00000800
#endif
#ifndef WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3
#define WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3 0x00002000
#endif
#ifndef WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS
#define WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS 0
#endif

bool urlSplit(const std::wstring& url, std::wstring& host, std::wstring& path,
              bool& https, INTERNET_PORT& port) {
    wchar_t scheme[32] = { 0 }, h[512] = { 0 }, p[16384] = { 0 }, extra[4096] = { 0 };

    URL_COMPONENTS uc;
    ZeroMemory(&uc, sizeof(uc));
    uc.dwStructSize     = sizeof(uc);
    uc.lpszScheme       = scheme; uc.dwSchemeLength = 31;
    uc.lpszHostName     = h;      uc.dwHostNameLength = 511;
    uc.lpszUrlPath      = p;      uc.dwUrlPathLength = 16383;
    uc.lpszExtraInfo    = extra;  uc.dwExtraInfoLength = 4095;

    if (!WinHttpCrackUrl(url.c_str(), (DWORD)url.size(), 0, &uc)) return false;

    https = (_wcsicmp(scheme, L"https") == 0);
    host  = h;
    path  = p;
    if (extra[0]) path += extra;
    if (path.empty()) path = L"/";
    port  = https ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;
    if (uc.nPort != 0) port = uc.nPort;
    return true;
}

// ---------------------------------------------------------------------------

HttpClient::HttpClient(const std::wstring& host, bool https, INTERNET_PORT port)
    : host_(host), https_(https), port_(port) {
    if (port_ == 0) port_ = https_ ? INTERNET_DEFAULT_HTTPS_PORT : INTERNET_DEFAULT_HTTP_PORT;

    session_ = WinHttpOpen(L"MCL/1.0 (Windows NT; C++ Minecraft Launcher)",
                           WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                           WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session_) return;

    // 解析/连接/发送/接收 超时(毫秒): 接收超时收紧, 避免单文件长时间挂起
    WinHttpSetTimeouts(session_, 8000, 10000, 15000, 20000);

    conn_ = WinHttpConnect(session_, host_.c_str(), port_, 0);
}

void HttpClient::setTimeoutMs(int connectMs, int sendMs, int recvMs) {
    if (session_) WinHttpSetTimeouts(session_, connectMs, sendMs, recvMs, recvMs);
}

HttpClient::~HttpClient() {
    if (conn_) WinHttpCloseHandle(conn_);
    if (session_) WinHttpCloseHandle(session_);
}

namespace {

// 在已建立的连接上发起 GET, 返回请求句柄(调用方关闭)
HINTERNET sendGet(HINTERNET conn, const std::wstring& path, bool https,
                  const std::wstring& ua, bool insecure, std::wstring& err) {
    DWORD flags = https ? WINHTTP_FLAG_SECURE : 0;
    HINTERNET req = WinHttpOpenRequest(conn, L"GET", path.c_str(), nullptr,
                                       WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, flags);
    if (!req) { err = L"创建请求失败"; return nullptr; }

    DWORD policy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(req, WINHTTP_OPTION_REDIRECT_POLICY, &policy, sizeof(policy));

    DWORD protocols = WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_2 | WINHTTP_FLAG_SECURE_PROTOCOL_TLS1_3;
    WinHttpSetOption(req, WINHTTP_OPTION_SECURE_PROTOCOLS, &protocols, sizeof(protocols));

    // 自建服务器使用自签证书: 仅对显式标记 insecure 的请求跳过证书校验
    if (https && insecure) {
        DWORD secFlags = SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_CN_INVALID |
                         SECURITY_FLAG_IGNORE_CERT_DATE_INVALID | SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE;
        WinHttpSetOption(req, WINHTTP_OPTION_SECURITY_FLAGS, &secFlags, sizeof(secFlags));
    }

    std::wstring headers = L"User-Agent: " + ua + L"\r\n";
    if (!WinHttpSendRequest(req, headers.c_str(), (DWORD)-1L,
                            WINHTTP_NO_REQUEST_DATA, 0, 0, 0)) {
        err = L"请求发送失败 (错误码 " + std::to_wstring(GetLastError()) + L")";
        WinHttpCloseHandle(req);
        return nullptr;
    }
    if (!WinHttpReceiveResponse(req, nullptr)) {
        err = L"等待响应失败 (错误码 " + std::to_wstring(GetLastError()) + L")";
        WinHttpCloseHandle(req);
        return nullptr;
    }

    DWORD status = 0, sz = sizeof(status);
    WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &status, &sz, WINHTTP_NO_HEADER_INDEX);
    if (status >= 400) {
        err = L"HTTP " + std::to_wstring(status);
        WinHttpCloseHandle(req);
        return nullptr;
    }
    return req;
}

} // namespace

bool HttpClient::get(const std::wstring& path, std::string& out, std::wstring& err) {
    if (!conn_) { err = L"未连接 " + host_; return false; }

    HINTERNET req = sendGet(conn_, path, https_, ua_, insecure_, err);
    if (!req) return false;

    out.clear();
    std::vector<char> buf;
    DWORD avail = 0;
    bool bad = false;

    while (WinHttpQueryDataAvailable(req, &avail) && avail > 0) {
        DWORD toRead = avail > 65536 ? 65536 : avail;
        buf.resize(toRead);
        DWORD read = 0;
        if (!WinHttpReadData(req, buf.data(), toRead, &read) || read == 0) { bad = true; break; }
        out.append(buf.data(), read);
    }
    WinHttpCloseHandle(req);

    if (bad) { err = L"读取响应失败"; return false; }
    if (out.empty()) { err = L"服务器返回空内容"; return false; }
    return true;
}

bool HttpClient::download(const std::wstring& path, const std::wstring& filePath,
                          ProgressFn progress, std::wstring& err) {
    if (!conn_) { err = L"未连接 " + host_; return false; }

    HINTERNET req = sendGet(conn_, path, https_, ua_, insecure_, err);
    if (!req) return false;

    DWORD total = 0, sz = sizeof(total);
    WinHttpQueryHeaders(req, WINHTTP_QUERY_CONTENT_LENGTH | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &total, &sz, WINHTTP_NO_HEADER_INDEX);

    if (!ensureParentDir(filePath)) {
        err = L"无法创建目录: " + dirNameOf(filePath);
        WinHttpCloseHandle(req);
        return false;
    }

    std::wstring tmpPath = filePath + L".part";
    HANDLE hFile = CreateFileW(tmpPath.c_str(), GENERIC_WRITE, 0, nullptr,
                               CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) {
        err = L"无法写入: " + tmpPath;
        WinHttpCloseHandle(req);
        return false;
    }

    std::vector<char> buf;
    DWORD avail = 0;
    unsigned long long done = 0;
    bool good = true, cancelled = false;

    while (WinHttpQueryDataAvailable(req, &avail) && avail > 0) {
        DWORD toRead = avail > 65536 ? 65536 : avail;
        buf.resize(toRead);
        DWORD read = 0;
        if (!WinHttpReadData(req, buf.data(), toRead, &read) || read == 0) { good = false; break; }
        DWORD written = 0;
        if (!WriteFile(hFile, buf.data(), read, &written, nullptr) || written != read) {
            good = false; err = L"磁盘写入失败"; break;
        }
        done += read;
        if (progress && !progress(done, (unsigned long long)total)) { cancelled = true; good = false; break; }
    }

    CloseHandle(hFile);
    WinHttpCloseHandle(req);

    if (good) {
        if (total > 0 && done != (unsigned long long)total) {
            err = L"下载不完整";
            DeleteFileW(tmpPath.c_str());
            return false;
        }
        if (!MoveFileExW(tmpPath.c_str(), filePath.c_str(),
                         MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            err = L"重命名临时文件失败";
            return false;
        }
        return true;
    }

    DeleteFileW(tmpPath.c_str());
    if (cancelled) err = L"已取消";
    else if (err.empty()) err = L"下载中断";
    return false;
}

// ---------------------------------------------------------------------------

bool httpGetToString(const std::wstring& url, std::string& out, std::wstring& err,
                     bool insecureTls, int timeoutMs) {
    std::wstring host, path;
    bool https = false;
    INTERNET_PORT port = 0;
    if (!urlSplit(url, host, path, https, port)) { err = L"URL 解析失败"; return false; }

    HttpClient c(host, https, port);
    if (timeoutMs > 0) c.setTimeoutMs(timeoutMs, timeoutMs, timeoutMs);
    if (!c.ok()) { err = L"无法连接 " + host; return false; }
    c.setInsecureTls(insecureTls);
    return c.get(path, out, err);
}

bool httpDownloadToFile(const std::wstring& url, const std::wstring& filePath,
                        ProgressFn progress, std::wstring& err,
                        bool insecureTls, int timeoutMs) {
    std::wstring host, path;
    bool https = false;
    INTERNET_PORT port = 0;
    if (!urlSplit(url, host, path, https, port)) { err = L"URL 解析失败"; return false; }

    HttpClient c(host, https, port);
    if (timeoutMs > 0) c.setTimeoutMs(timeoutMs, timeoutMs, timeoutMs);
    if (!c.ok()) { err = L"无法连接 " + host; return false; }
    c.setInsecureTls(insecureTls);
    return c.download(path, filePath, progress, err);
}

// ---- 带自定义 User-Agent 的便捷接口 (MC 百科 / Modrinth 需要浏览器 UA) ----
bool httpGetToStringEx(const std::wstring& url, const std::wstring& userAgent,
                       std::string& out, std::wstring& err,
                       bool insecureTls, int timeoutMs) {
    std::wstring host, path;
    bool https = false;
    INTERNET_PORT port = 0;
    if (!urlSplit(url, host, path, https, port)) { err = L"URL 解析失败"; return false; }

    HttpClient c(host, https, port);
    if (timeoutMs > 0) c.setTimeoutMs(timeoutMs, timeoutMs, timeoutMs);
    if (!c.ok()) { err = L"无法连接 " + host; return false; }
    c.setUserAgent(userAgent);
    c.setInsecureTls(insecureTls);
    return c.get(path, out, err);
}

bool httpDownloadToFileEx(const std::wstring& url, const std::wstring& filePath,
                          const std::wstring& userAgent, ProgressFn progress,
                          std::wstring& err, bool insecureTls, int timeoutMs) {
    std::wstring host, path;
    bool https = false;
    INTERNET_PORT port = 0;
    if (!urlSplit(url, host, path, https, port)) { err = L"URL 解析失败"; return false; }

    HttpClient c(host, https, port);
    if (timeoutMs > 0) c.setTimeoutMs(timeoutMs, timeoutMs, timeoutMs);
    if (!c.ok()) { err = L"无法连接 " + host; return false; }
    c.setUserAgent(userAgent);
    c.setInsecureTls(insecureTls);
    return c.download(path, filePath, progress, err);
}

// 用 WinHTTP 做一次轻量探测, 避免引入 wininet.h (与 winhttp.h 类型冲突)
bool netIsOnline() {
    HINTERNET s = WinHttpOpen(L"MCL/probe", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                              WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!s) return false;
    WinHttpSetTimeouts(s, 3000, 3000, 3000, 3000);
    HINTERNET c = WinHttpConnect(s, L"www.baidu.com", INTERNET_DEFAULT_HTTP_PORT, 0);
    bool okFlag = (c != nullptr);
    if (c) WinHttpCloseHandle(c);
    WinHttpCloseHandle(s);
    return okFlag;
}
