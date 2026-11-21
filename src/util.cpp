// ---------------------------------------------------------------------------
//  util.cpp
// ---------------------------------------------------------------------------
#include "util.h"

#include <cstdio>
#include <cstdarg>
#include <algorithm>

std::wstring utf8ToWide(const std::string& s) {
    if (s.empty()) return std::wstring();
    int n = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0);
    if (n <= 0) return std::wstring();
    std::wstring w((size_t)n, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), (int)s.size(), &w[0], n);
    return w;
}

std::string wideToUtf8(const std::wstring& s) {
    if (s.empty()) return std::string();
    int n = WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), nullptr, 0, nullptr, nullptr);
    if (n <= 0) return std::string();
    std::string a((size_t)n, '\0');
    WideCharToMultiByte(CP_UTF8, 0, s.c_str(), (int)s.size(), &a[0], n, nullptr, nullptr);
    return a;
}

bool pathExists(const std::wstring& path) {
    DWORD a = GetFileAttributesW(path.c_str());
    return a != INVALID_FILE_ATTRIBUTES;
}

bool isDirectory(const std::wstring& path) {
    DWORD a = GetFileAttributesW(path.c_str());
    return a != INVALID_FILE_ATTRIBUTES && (a & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool createDirs(const std::wstring& path) {
    if (path.empty()) return false;
    if (isDirectory(path)) return true;
    std::wstring p = path;
    while (!p.empty() && (p.back() == L'\\' || p.back() == L'/')) p.pop_back();
    if (p.empty()) return false;

    std::wstring cur;
    size_t i = 0;
    if (p.size() >= 2 && p[1] == L':') { cur = p.substr(0, 2); i = 2; }
    else if (p.size() >= 1 && p[0] == L'\\') { cur = L"\\"; i = 1; }

    while (i < p.size()) {
        size_t j = p.find_first_of(L"\\/", i);
        std::wstring seg = (j == std::wstring::npos) ? p.substr(i) : p.substr(i, j - i);
        if (!seg.empty()) {
            if (!cur.empty() && cur.back() != L'\\') cur += L'\\';
            cur += seg;
            CreateDirectoryW(cur.c_str(), nullptr);
        }
        if (j == std::wstring::npos) break;
        i = j + 1;
    }
    return isDirectory(path);
}

bool ensureParentDir(const std::wstring& filePath) {
    std::wstring d = dirNameOf(filePath);
    if (d.empty()) return true;
    return createDirs(d);
}

std::wstring joinPath(const std::wstring& a, const std::wstring& b) {
    if (a.empty()) return b;
    if (b.empty()) return a;
    std::wstring r = a;
    if (r.back() != L'\\' && r.back() != L'/') r += L'\\';
    if (b.front() == L'\\' || b.front() == L'/') r += b.substr(1);
    else r += b;
    return r;
}

std::wstring fileNameOf(const std::wstring& path) {
    size_t p = path.find_last_of(L"\\/");
    return (p == std::wstring::npos) ? path : path.substr(p + 1);
}

std::wstring dirNameOf(const std::wstring& path) {
    size_t p = path.find_last_of(L"\\/");
    return (p == std::wstring::npos) ? std::wstring() : path.substr(0, p);
}

std::wstring trimW(const std::wstring& s) {
    size_t b = 0, e = s.size();
    while (b < e && (s[b] == L' ' || s[b] == L'\t' || s[b] == L'\r' || s[b] == L'\n')) ++b;
    while (e > b && (s[e - 1] == L' ' || s[e - 1] == L'\t' || s[e - 1] == L'\r' || s[e - 1] == L'\n')) --e;
    return s.substr(b, e - b);
}

std::vector<std::wstring> splitW(const std::wstring& s, wchar_t delim) {
    std::vector<std::wstring> out;
    std::wstring cur;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == delim) { out.push_back(cur); cur.clear(); }
        else cur += s[i];
    }
    out.push_back(cur);
    return out;
}

std::string replaceAll(std::string s, const std::string& from, const std::string& to) {
    if (from.empty()) return s;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::string::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

std::wstring replaceAllW(std::wstring s, const std::wstring& from, const std::wstring& to) {
    if (from.empty()) return s;
    size_t pos = 0;
    while ((pos = s.find(from, pos)) != std::wstring::npos) {
        s.replace(pos, from.size(), to);
        pos += to.size();
    }
    return s;
}

std::wstring humanSize(unsigned long long bytes) {
    const wchar_t* units[] = { L"B", L"KB", L"MB", L"GB", L"TB" };
    double v = (double)bytes;
    int u = 0;
    while (v >= 1024.0 && u < 4) { v /= 1024.0; ++u; }
    wchar_t buf[64];
    swprintf(buf, 64, (u == 0) ? L"%.0f %s" : L"%.1f %s", v, units[u]);
    return buf;
}

static std::wstring g_logPath;

std::wstring exeDir() {
    wchar_t buf[MAX_PATH] = { 0 };
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    std::wstring s = buf;
    size_t p = s.find_last_of(L"\\/");
    return (p == std::wstring::npos) ? s : s.substr(0, p);
}

void appLog(const std::wstring& msg) {
    if (g_logPath.empty()) {
        std::wstring d = exeDir();
        g_logPath = joinPath(d, L"MCL.log");
        // 启动时截断
        HANDLE h = CreateFileW(g_logPath.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                               nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (h != INVALID_HANDLE_VALUE) CloseHandle(h);
    }
    FILE* f = _wfopen(g_logPath.c_str(), L"a, ccs=UTF-8");
    if (!f) return;
    SYSTEMTIME st;
    GetLocalTime(&st);
    fwprintf(f, L"[%02d:%02d:%02d] %s\n", st.wHour, st.wMinute, st.wSecond, msg.c_str());
    fclose(f);
}
