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
