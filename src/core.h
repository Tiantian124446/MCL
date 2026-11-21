// ---------------------------------------------------------------------------
//  core.h  -  Minecraft 启动器核心逻辑
// ---------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <atomic>

struct VersionInfo {
    std::string id;
    std::string type;         // release / snapshot / old_beta / old_alpha
    std::string url;
    std::string releaseTime;
    std::string sha1;
};

struct LaunchOptions {
    std::wstring gameDir;                       // .minecraft 目录
    std::wstring javaPath;                      // java.exe 绝对路径
    std::string  playerName = "Player";
    int          maxMemoryMB = 2048;
    bool         useMirror = true;
    std::string  mirrorHost = "https://bmclapi2.bangbang93.com";
    // 联机直连: 非空时启动后直接进入该服务器
    std::string  serverAddress;                 // 服务器地址 (IP 或域名)
    int          serverPort = 0;                // 服务器端口 (0 = 使用默认 25565)
};

class Launcher {
public:
    LaunchOptions opt;

    // UI 回调
    std::function<void(const std::wstring&)>      onLog;
    std::function<bool(int, const std::wstring&)> onProgress;  // 返回 false 取消
    std::atomic<bool> cancelled{ false };

    void log(const std::wstring& s);
    bool tick(int percent, const std::wstring& text);

    // 版本清单
    bool fetchManifest(std::vector<VersionInfo>& out, std::wstring& err);

    // 本地版本
    bool isInstalled(const std::string& id) const;
    std::vector<std::string> listInstalled() const;

    // 下载安装完整版本(客户端 + 依赖库 + 资源 + 解压 natives)
    bool install(const VersionInfo& vi, std::wstring& err);

    // 启动已安装版本
    bool launch(const std::string& id, std::wstring& err);

    // Java 探测
    static std::wstring autoDetectJava();
    static int          queryJavaMajor(const std::wstring& javaExe);

    std::wstring mirrorUrl(const std::string& url) const;
    std::wstring versionDir(const std::string& id) const;
};
