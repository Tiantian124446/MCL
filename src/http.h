// ---------------------------------------------------------------------------
//  http.h  -  基于 WinHTTP 的下载器 (支持连接复用)
// ---------------------------------------------------------------------------
#pragma once

#include <string>
#include <functional>
#include <windows.h>
#include <winhttp.h>

// 进度回调: (已下载字节, 总字节)  返回 false 表示请求取消
typedef std::function<bool(unsigned long long, unsigned long long)> ProgressFn;

// 绑定到单一 host 的 HTTP 客户端, session/connection 复用 (适合批量下载小文件)
class HttpClient {
public:
    HttpClient(const std::wstring& host, bool https = true, INTERNET_PORT port = 0);
    ~HttpClient();

    HttpClient(const HttpClient&) = delete;
    HttpClient& operator=(const HttpClient&) = delete;

    bool ok() const { return conn_ != nullptr; }
    const std::wstring& host() const { return host_; }

    // 自定义 User-Agent (必须在首次请求前调用)
    void setUserAgent(const std::wstring& ua) { ua_ = ua; }

    // path 形如 "/mc/game/version_manifest.json"
    bool get(const std::wstring& path, std::string& out, std::wstring& err);
    bool download(const std::wstring& path, const std::wstring& filePath,
                  ProgressFn progress, std::wstring& err);

private:
    HINTERNET session_ = nullptr;
    HINTERNET conn_ = nullptr;
    std::wstring host_;
    bool https_ = true;
    INTERNET_PORT port_ = 0;
    std::wstring ua_ = L"MCL/1.0 (Windows NT; C++ Minecraft Launcher)";
};

// 解析 URL
bool urlSplit(const std::wstring& url, std::wstring& host, std::wstring& path,
              bool& https, INTERNET_PORT& port);

// 一次性便捷接口
bool httpGetToString(const std::wstring& url, std::string& out, std::wstring& err);
bool httpDownloadToFile(const std::wstring& url, const std::wstring& filePath,
                        ProgressFn progress, std::wstring& err);

// 同上, 但可自定义 User-Agent (MC 百科 / Modrinth 需要浏览器 UA)
bool httpGetToStringEx(const std::wstring& url, const std::wstring& userAgent,
                       std::string& out, std::wstring& err);
bool httpDownloadToFileEx(const std::wstring& url, const std::wstring& filePath,
                          const std::wstring& userAgent, ProgressFn progress,
                          std::wstring& err);

// 网络是否可用
bool netIsOnline();
