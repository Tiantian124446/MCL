// ---------------------------------------------------------------------------
//  util.h  -  基础工具函数
// ---------------------------------------------------------------------------
#pragma once

#include <string>
#include <vector>
#include <windows.h>

std::wstring utf8ToWide(const std::string& s);
std::string  wideToUtf8(const std::wstring& s);

bool pathExists(const std::wstring& path);
bool isDirectory(const std::wstring& path);
bool createDirs(const std::wstring& path);
bool ensureParentDir(const std::wstring& filePath);

std::wstring joinPath(const std::wstring& a, const std::wstring& b);
std::wstring fileNameOf(const std::wstring& path);
std::wstring dirNameOf(const std::wstring& path);

std::wstring trimW(const std::wstring& s);
std::vector<std::wstring> splitW(const std::wstring& s, wchar_t delim);

// 把字符串中的 from 全部替换为 to
std::string replaceAll(std::string s, const std::string& from, const std::string& to);
std::wstring replaceAllW(std::wstring s, const std::wstring& from, const std::wstring& to);

// 人类可读的容量
std::wstring humanSize(unsigned long long bytes);

// 简单日志（写文件 + 便于排查）
void appLog(const std::wstring& msg);
std::wstring exeDir();

// ---------------------------------------------------------------------------
// 自恢复 / 防删除（课堂管理软件会在开关机时清理应用，这里做一份备份并在登录时还原）
// ---------------------------------------------------------------------------
std::wstring selfGuardDir();                 // %LOCALAPPDATA%\MCL\guard
bool selfGuardInstall();                     // 备份自身 + 写登录自启（可重复调用）
bool selfGuardRestore(std::wstring& target); // --guard 模式: 还原被删除的主程序
bool selfGuardInstalled();                   // 是否已建立保护
void selfGuardRemove();                      // 用户主动“彻底删除”：移除备份与自启
std::wstring selfExePath();                  // 当前 exe 的完整路径
