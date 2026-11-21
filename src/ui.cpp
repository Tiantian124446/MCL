// ---------------------------------------------------------------------------
//  ui.cpp  -  自绘 UI (GDI+), 布局参考 PCL2 风格
// ---------------------------------------------------------------------------
#include <winsock2.h>
#include <ws2tcpip.h>

#include "ui.h"
#include "util.h"
#include "core.h"
#include "http.h"
#include "json.h"

#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>
#include <shlobj.h>
#include <cstdio>

#include <atomic>
#include <cmath>
#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>
#include <algorithm>
#include <shellapi.h>

using namespace Gdiplus;

// ------------------------------ 常量 ---------------------------------------
static const int SIDEBAR_W = 176;
static const int HEADER_H  = 56;
static const int MIN_W     = 900;
static const int MIN_H     = 620;

// 设计基准尺寸: 逻辑坐标系以 960x640 客户区为 1.0 倍, 窗口放大时整体等比放大
static const float DESIGN_W = 960.0f;
static const float DESIGN_H = 640.0f;
static const float SCALE_MIN = 0.60f;
static const float SCALE_MAX = 4.00f;

// 应用版本与发布服务器（Win10 服务器站点）
static const wchar_t* kAppVersion = L"1.27.1";
static const wchar_t* kServerBase = L"http://192.168.3.173:7891";

static inline Color C(int r, int g, int b, int a = 255) {
    return Color((BYTE)a, (BYTE)r, (BYTE)g, (BYTE)b);
}
static const Color kSideBg    = C(255, 255, 255);
static const Color kContentBg = C(245, 246, 248);
static const Color kCardBg    = C(255, 255, 255);
static const Color kInputBg   = C(242, 243, 245);
static const Color kAccent    = C(60, 133, 39);
static const Color kAccentHi  = C(78, 160, 54);
static const Color kText      = C(28, 30, 34);
static const Color kTextDim   = C(132, 136, 142);
static const Color kLine      = C(228, 230, 234);
static const Color kHover     = C(240, 242, 245);
static const Color kSelBg     = C(232, 244, 227);
static const Color kLogBg     = C(30, 31, 34);
static const Color kLogText   = C(198, 204, 210);
static const Color kWhite     = C(255, 255, 255);

// ------------------------------ 控件 ID ------------------------------------
enum {
    BTN_NONE = 0,
    BTN_NAV_HOME = 1,
    BTN_NAV_VERSIONS,
    BTN_NAV_SETTINGS,
    BTN_NAV_ABOUT,
    BTN_PICK_VERSION,
    BTN_START_GAME,
    BTN_INSTALL_SEL,
    BTN_REFRESH,
    BTN_LAUNCH_SEL,
    BTN_DELETE_SEL,
    BTN_BROWSE_DIR,
    BTN_AUTO_JAVA,
    BTN_BROWSE_JAVA,
    BTN_SAVE,
    BTN_MIRROR_BMCL,
    BTN_MIRROR_OFFICIAL,
    BTN_CANCEL,
    BTN_TOGGLE_INSTALLED,
    BTN_FULLSCREEN,
    BTN_CHECK_UPDATE,
    BTN_OPEN_SITE,
    BTN_OPEN_FEEDBACK,
    // ---- 模组下载页
    BTN_NAV_MODS,
    BTN_MODS_SEARCH,
    BTN_MODS_BACK,
    BTN_MODS_GET_VERSIONS,
    BTN_MODS_QUERY,
    BTN_MODS_DOWNLOAD,
    BTN_MODS_OPEN_DIR,
    BTN_MODS_OPEN_PAGE,      // 在浏览器打开百科页
    BTN_MODS_OPEN_MR,        // 在浏览器打开 Modrinth 项目页
    BTN_MODS_MR_SEARCH,      // 详情无 Modrinth 链接时, 用 Modrinth 搜索兜底解析
    // ---- 联机页
    BTN_NAV_SERVERS,
    BTN_NAV_RESOURCES,
    BTN_NAV_MODMGR,
    BTN_NAV_SAVES,
    BTN_NAV_RESDL,
    BTN_NAV_ACCOUNTS,
    BTN_SRV_ADD,             // 新增 / 保存服务器条目
    BTN_SRV_RESET,           // 取消编辑 / 清空表单
    BTN_SRV_IMPORT,          // 从公开清单导入服务器
    BTN_MODS_LOADER_BASE   = 300000,   // +0..4: 全部/Fabric/Forge/NeoForge/Quilt
    BTN_MOD_ROW_BASE       = 400000,   // +列表下标
    BTN_MODVER_ROW_BASE    = 500000,   // +列表下标
    BTN_MOD_LINK_BASE      = 600000,   // +相关链接下标
    BTN_SRV_ROW_BASE       = 700000,   // +列表下标: 选中该行
    BTN_SRV_CONNECT_BASE   = 710000,   // +列表下标: 一键直连
    BTN_SRV_EDIT_BASE      = 720000,   // +列表下标: 载入到表单编辑
    BTN_SRV_DEL_BASE       = 730000,   // +列表下标: 删除
    BTN_SRV_VIS_BASE       = 750000,   // +列表下标: 切换"显示服务器 / 不显示服务器"
    BTN_SRV_PING_BASE      = 760000,   // +列表下标: TCP 测速
    BTN_VERCAT_BASE        = 800000,
    BTN_RES_OPEN_BASE      = 900000,   // +0..7: 资源管理卡片
    BTN_OPEN_GAMEDIR       = 901000,
    BTN_ANALYZE_CRASH      = 901001,
    BTN_EXPORT_DIAG        = 901002,
    BTN_MODMGR_REFRESH     = 901003,
    BTN_MODMGR_OPEN        = 901004,
    BTN_MODMGR_ENABLE_ALL  = 901005,
    BTN_MODMGR_DISABLE_ALL = 901006,
    BTN_JAVA_SCAN          = 901007,
    BTN_ACCOUNT_ADD        = 901008,
    BTN_SAVE_RESTORE       = 901013,
    BTN_ISOLATION          = 901014,
    BTN_INSTALL_FABRIC     = 901015,
    BTN_INSTALL_FORGE      = 901016,
    BTN_DOWNLOAD_LATEST    = 901017,
    BTN_RES_WEB_BASE       = 908000,   // +资源卡片下标：Modrinth 搜索
    BTN_OPEN_LATEST_LOG    = 901022,
    BTN_RESDL_SEARCH       = 901018,
    BTN_RESDL_TYPE_PACK    = 901019,
    BTN_RESDL_TYPE_SHADER  = 901020,
    BTN_RESDL_OPEN         = 901021,
    BTN_RESDL_DL_BASE      = 909000,   // +结果下标：下载最新文件
    BTN_AUTO_BACKUP        = 901009,
    BTN_GOPT_SAVE          = 901010,
    BTN_NAV_GOPT,
    BTN_GOPT_DEC_BASE      = 905000,   // +行: 数值减
    BTN_GOPT_INC_BASE      = 906000,   // +行: 数值加
    BTN_GOPT_TOGGLE_BASE   = 907000,   // +行: 切换
    BTN_ACCOUNT_USE_BASE   = 903000,   // +列表下标: 使用账户
    BTN_ACCOUNT_DEL_BASE   = 904000,   // +列表下标: 删除账户
    BTN_JAVA_USE_BASE      = 902000,   // +列表下标: 使用该 Java
    BTN_MODMGR_TOGGLE_BASE = 910000,   // +列表下标: 启用/禁用
    BTN_MODMGR_DEL_BASE    = 920000,   // +列表下标: 删除
    BTN_SAVE_OPEN_BASE     = 930000,   // +列表下标: 打开存档目录
    BTN_SAVE_BACKUP_BASE   = 940000,   // +列表下标: 备份存档
    BTN_SAVE_DEL_BASE      = 950000,   // +列表下标: 删除存档
    // +0..4: 版本分类 全部/正式版/快照/远古版/愚人节版
    BTN_LIST_BASE = 2000
};

enum {
    CTL_NAME = 100,
    CTL_SEARCH,
    CTL_MOD_SEARCH,
    CTL_MOD_MCVER,
    CTL_SRV_NAME,
    CTL_SRV_HOST,
    CTL_SRV_PORT,
    CTL_ACCOUNT
};

struct HitRect {
    RECT rc;
    int  id;
};

// ------------------------------ 绘制辅助 -----------------------------------
static void fillRound(Graphics& g, const RECT& rc, int r, const Color& c) {
    if (rc.right <= rc.left || rc.bottom <= rc.top) return;
    int d = r * 2;
    if (d <= 0) { SolidBrush b(c); g.FillRectangle(&b, (INT)rc.left, (INT)rc.top, (INT)(rc.right - rc.left), (INT)(rc.bottom - rc.top)); return; }
    GraphicsPath p;
    p.AddArc(rc.left, rc.top, d, d, 180, 90);
    p.AddArc(rc.right - d, rc.top, d, d, 270, 90);
    p.AddArc(rc.right - d, rc.bottom - d, d, d, 0, 90);
    p.AddArc(rc.left, rc.bottom - d, d, d, 90, 90);
    p.CloseFigure();
    SolidBrush b(c);
    g.FillPath(&b, &p);
}

static void strokeRound(Graphics& g, const RECT& rc, int r, const Color& c, float w = 1.0f) {
    int d = r * 2;
    GraphicsPath p;
    p.AddArc(rc.left, rc.top, d, d, 180, 90);
    p.AddArc(rc.right - d, rc.top, d, d, 270, 90);
    p.AddArc(rc.right - d, rc.bottom - d, d, d, 0, 90);
    p.AddArc(rc.left, rc.bottom - d, d, d, 90, 90);
    p.CloseFigure();
    Pen pen(c, w);
    g.DrawPath(&pen, &p);
}

static void drawText(Graphics& g, const std::wstring& s, Font& f, const Color& c,
                     const RECT& rc, int align, bool ellipsis = true) {
    if (s.empty()) return;
    SolidBrush br(c);
    StringFormat sf;
    sf.SetAlignment(align == 0 ? StringAlignmentNear : (align == 1 ? StringAlignmentCenter : StringAlignmentFar));
    sf.SetLineAlignment(StringAlignmentCenter);
    sf.SetFormatFlags(StringFormatFlagsNoWrap);
    sf.SetTrimming(ellipsis ? StringTrimmingEllipsisCharacter : StringTrimmingNone);
    g.DrawString(s.c_str(), (INT)s.size(), &f,
                 RectF((REAL)rc.left, (REAL)rc.top,
                       (REAL)(rc.right - rc.left), (REAL)(rc.bottom - rc.top)),
                 &sf, &br);
}

static RECT mkRect(int l, int t, int r, int b) { RECT rc{ l, t, r, b }; return rc; }

// ------------------------- 内存分配 ----------------------------------------
// 可分配内存区间; 上限放大, 便于大内存机器给整合包分配更充裕
static const int kMemMinMB = 512;
static const int kMemMaxMB = 32768;

// 依据物理内存给出推荐分配值: 取总内存的 1/4, 约束在 [4096, 8192]
static int recommendMemoryMB() {
    MEMORYSTATUSEX ms{};
    ms.dwLength = sizeof(ms);
    if (GlobalMemoryStatusEx(&ms) && ms.ullTotalPhys > 0) {
        unsigned long long totalMB = ms.ullTotalPhys / (1024ULL * 1024ULL);
        int rec = (int)(totalMB / 4);
        rec = ((rec + 511) / 512) * 512;   // 对齐到 512MB, 数值更整齐
        if (rec > 8192) rec = 8192;
        if (rec < 4096) rec = 4096;
        return rec;
    }
    return 4096;
}

// ===========================================================================
//  模组下载页 (MC 百科检索 + Modrinth 下载)
// ===========================================================================

using Json = mjson::Value;

static const wchar_t* kBrowserUA =
    L"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 "
    L"(KHTML, like Gecko) Chrome/122.0.0.0 Safari/537.36";

// Modrinth 加载器 id 与显示名 (下标与 BTN_MODS_LOADER_BASE 偏移一致)
static const char* kLoaderIds[]      = { "fabric", "forge", "neoforge", "quilt" };
static const wchar_t* kLoaderNames[] = { L"Fabric", L"Forge", L"NeoForge", L"Quilt" };
static const int kLoaderCount = 4;

// MC 百科第三方接口 / Modrinth 官方接口
static const wchar_t* kMcmodSearchApi = L"https://mcmod-api.zkitefly.eu.org/s/key=";
static const wchar_t* kMcmodDetailApi = L"https://mcmod-api.zkitefly.eu.org/d/class/";
static const wchar_t* kModrinthApi    = L"https://api.modrinth.com/v2/";

// 从 https://modrinth.com/mod/{slug} 解析项目 slug
static std::string parseModrinthSlug(const std::string& url) {
    std::string low = url;
    for (size_t i = 0; i < low.size(); ++i) low[i] = (char)tolower((unsigned char)low[i]);
    const std::string key = "modrinth.com/mod/";
    size_t p = low.find(key);
    if (p == std::string::npos) return "";
    std::string slug = url.substr(p + key.size());
    size_t cut = slug.find_first_of("?#");
    if (cut != std::string::npos) slug = slug.substr(0, cut);
    while (!slug.empty() && slug[slug.size() - 1] == '/') slug.erase(slug.size() - 1);
    return slug;
}

// 单行化文本: 去掉换行与百科标记, 便于列表展示
static std::wstring oneLine(const std::wstring& s, size_t maxLen) {
    std::wstring out;
    for (wchar_t c : s) {
        if (c == L'\r' || c == L'\n' || c == L'\t') out += L' ';
        else out += c;
    }
    out = replaceAllW(out, L"  ", L" ");
    out = trimW(out);
    if (out.size() > maxLen) out = out.substr(0, maxLen) + L"…";
    return out;
}

// 每条网络请求都会创建一次会话, 这里做最小间隔节流, 遵守第三方接口的使用约定
static std::mutex g_netGate;
static unsigned long long g_lastReqMs = 0;
static void apiThrottle(int ms) {
    std::lock_guard<std::mutex> lk(g_netGate);
    for (;;) {
        unsigned long long now = GetTickCount64();
        unsigned long long wait = (g_lastReqMs + (unsigned long long)ms > now)
                                  ? (g_lastReqMs + (unsigned long long)ms - now) : 0;
        if (wait == 0) { g_lastReqMs = now; return; }
        Sleep((DWORD)(wait > 500 ? 500 : wait));
    }
}

// UTF-8 百分号编码 (用于检索关键词)
static std::string urlEncode(const std::string& s) {
    static const char* hex = "0123456789ABCDEF";
    std::string out;
    for (unsigned char c : s) {
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            out.push_back((char)c);
        } else {
            out.push_back('%');
            out.push_back(hex[c >> 4]);
            out.push_back(hex[c & 0x0F]);
        }
    }
    return out;
}

// 字符串拼接后再做百分号编码 (game_versions/loaders 的 JSON 需要整体编码)
static std::string urlEncodeAll(const std::string& s) {
    static const char* hex = "0123456789ABCDEF";
    std::string out;
    for (unsigned char c : s) {
        out.push_back('%');
        out.push_back(hex[c >> 4]);
        out.push_back(hex[c & 0x0F]);
    }
    return out;
}

static std::string getStr(const Json& j, const std::string& k, const std::string& def = "") {
    return j.has(k) ? j[k].asString(def) : def;
}

// MC 百科分类 ID -> 中文 (mcmod.cn 模组元素)
static std::wstring categoryNameW(const std::string& id) {
    static const std::map<std::string, const wchar_t*> kMap = {
        { "1",  L"科技" },   { "2",  L"魔法" },   { "3",  L"冒险" },
        { "4",  L"农业" },   { "5",  L"装饰" },   { "6",  L"实用" },
        { "7",  L"辅助" },   { "8",  L"资源" },   { "9",  L"世界" },
        { "10", L"生物" },   { "11", L"群系" },   { "12", L"结构" },
        { "13", L"道具" },   { "14", L"物流" },   { "15", L"存储" },
        { "16", L"能源" },   { "17", L"红石" },   { "18", L"食物" },
        { "19", L"模型" },   { "20", L"指南" },   { "21", L"破坏" },
        { "22", L"Meme" },   { "23", L"恐怖" },   { "24", L"建材" },
        { "25", L"生存" },   { "26", L"优化" },   { "27", L"指令" },
        { "28", L"国创" },   { "29", L"联机" },   { "30", L"安全" },
        { "31", L"关卡" },   { "32", L"中式" },   { "33", L"日式" },
        { "34", L"西式" },   { "35", L"多人" }
    };
    // 百科分类字段可能是逗号/竖线分隔的多值
    std::wstring out;
    std::string cur;
    auto flush = [&](const std::string& t) {
        auto it = kMap.find(t);
        std::wstring name = (it != kMap.end()) ? it->second : utf8ToWide(t);
        if (!name.empty()) { if (!out.empty()) out += L" / "; out += name; }
    };
    for (char c : id) {
        if (c == ',' || c == '|' || c == ';' || c == ' ') { if (!cur.empty()) { flush(cur); cur.clear(); } }
        else cur.push_back(c);
    }
    if (!cur.empty()) flush(cur);
    return out;
}

// ===========================================================================
//  联机页: 服务器条目
// ===========================================================================
struct ServerEntry {
    std::wstring name;      // 备注名
    std::wstring host;      // 地址 (IP / 域名)
    int          port = 25565;
    bool         visible = true;   // true=显示服务器(任何人可见可进入) false=仅本机可见
};

// 服务器列表持久化文件: <exe 目录>\mcl_servers.txt  (UTF-8, 每行 name|host|port|visible)
static std::wstring serversFilePath() {
    return joinPath(exeDir(), L"mcl_servers.txt");
}

// 公开清单: <exe 目录>\mcl_servers_public.txt  (UTF-8, 每行 name|host|port)
// 只有"显示服务器"的条目才会写进这里, 任何人都可以拿这份清单导入并进入这些房间;
// "不显示服务器"的条目只保留在本机 mcl_servers.txt 里。
static std::wstring publicServersFilePath() {
    return joinPath(exeDir(), L"mcl_servers_public.txt");
}

// 名称/地址中不允许出现分隔符与换行, 写入前统一净化
static std::wstring sanitizeServerField(const std::wstring& s) {
    std::wstring out;
    for (wchar_t c : s) {
        if (c == L'|' || c == L'\r' || c == L'\n' || c == L'\t') out += L' ';
        else out += c;
    }
    return trimW(out);
}

// 解析端口: 仅接受 1 - 65535 的纯数字
static bool parsePortW(const std::wstring& s, int& out) {
    std::wstring t = trimW(s);
    if (t.empty() || t.size() > 5) return false;
    for (wchar_t c : t) { if (c < L'0' || c > L'9') return false; }
    int v = 0;
    for (wchar_t c : t) v = v * 10 + (c - L'0');
    if (v < 1 || v > 65535) return false;
    out = v;
    return true;
}

struct McModItem {
    std::wstring name;      // 中文名
    std::wstring enName;    // 英文名 / 副标题
    std::wstring abbr;      // 简称
    std::wstring category;  // 分类(中文)
    std::wstring desc;      // 简介
    std::wstring cover;     // 封面 URL
    std::wstring address;   // 百科页面
    std::string  id;        // mcmod_id
};

struct McModDetail {
    std::string  id;
    std::wstring title;
    std::wstring subtitle;
    std::wstring shortName;
    std::wstring cover;
    std::wstring mcVersions;   // 支持的游戏版本
    std::wstring environment;  // 运行环境
    std::wstring authors;      // 作者
    std::wstring recorded;     // 收录时间
    std::wstring lastEdit;     // 最后编辑
    std::wstring address;      // 百科页面地址
    std::vector<std::pair<std::wstring, std::wstring>> links;  // 相关链接 (文本, URL)
    std::wstring slug;         // 从 related_links 解析出的 Modrinth slug
    std::string  raw;          // 原始 JSON 片段 (排查用)
};

struct ModFile {
    std::wstring ver;
    std::wstring loaders;
    std::wstring name;
    std::wstring url;
    unsigned long long size = 0;
};

// ---- 封面缓存: 数据下载在后台线程, 解码与绘制只在 UI 线程
struct CoverStore {
    std::map<std::wstring, std::string> bytes;      // 已下载的原始图片数据 (mtx_ 保护)
    std::set<std::wstring> pending;                 // 已发起、尚未返回的请求 (mtx_ 保护)
    std::set<std::wstring> failed;                  // 下载失败的 URL (mtx_ 保护)
    std::map<std::wstring, Gdiplus::Image*> imgs;   // UI 线程解码结果 (进程级缓存, 退出时随进程回收)
};

// 等比裁剪填充绘制
static void drawCoverImage(Gdiplus::Graphics& g, const RECT& rc, Gdiplus::Image* img) {
    if (!img || img->GetLastStatus() != Gdiplus::Ok) return;
    int iw = (int)img->GetWidth(), ih = (int)img->GetHeight();
    int w = rc.right - rc.left, h = rc.bottom - rc.top;
    if (iw <= 0 || ih <= 0 || w <= 0 || h <= 0) return;
    REAL sx = (REAL)w / (REAL)iw, sy = (REAL)h / (REAL)ih;
    REAL s = sx > sy ? sx : sy;
    REAL dw = iw * s, dh = ih * s;
    g.SetClip(Gdiplus::RectF((REAL)rc.left, (REAL)rc.top, (REAL)w, (REAL)h));
    g.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    g.DrawImage(img, Gdiplus::RectF(rc.left + (w - dw) / 2.0f, rc.top + (h - dh) / 2.0f, dw, dh));
    g.ResetClip();
}

// ------------------------------ 主窗口 -------------------------------------
class MainWindow {
public:
    static MainWindow& inst() { static MainWindow w; return w; }

    int  run(HINSTANCE hInst, int nCmdShow);

private:
    HWND  hwnd_ = nullptr;
    HFONT uiFont_ = nullptr;
    HBRUSH brushInput_ = nullptr;

    // ---- 界面状态
    int   page_ = 0;                 // 0 启动 1 版本 2 设置 3 关于
    int   hoverId_ = 0;
    int   listScroll_ = 0;
    int   selList_ = -1;             // 列表选中行 (过滤后的下标)
    bool  draggingMem_ = false;
    bool  fullscreen_ = false;
    RECT  savedRect_{};
    DWORD savedStyle_ = 0;
    DWORD savedExStyle_ = 0;

    // ---- 业务数据
    Launcher launcher_;
    std::vector<VersionInfo> manifest_;
    std::set<std::string> installedSet_;
    std::vector<int> filtered_;       // manifest_ 下标
    std::wstring verSearch_;          // 版本列表搜索词
    std::vector<std::string> installedOnly_;
    bool  showInstalledOnly_ = false;
    int   verCat_ = 0;                // 版本分类 0全部 1正式版 2快照 3远古版 4愚人节版

    std::wstring selVersion_ = L"";
    std::wstring playerName_ = L"Steve";
    std::vector<std::wstring> accounts_;
    int          memoryMB_ = 0;      // 0 = 未配置, 由 recommendMemoryMB() 决定
    std::wstring gameDir_;
    std::map<std::string, std::string> gopt_;
    std::vector<std::string> goptOrder_;
    std::wstring javaPath_;
    std::wstring javaInfo_;
    bool         useMirror_ = true;
    bool autoBackup_ = true;
    bool isolation_ = false;
    bool autoBackupChecked_ = false;

    // ---- 联机页状态 (servers_ 一律在 mtx_ 内读写)
    std::vector<ServerEntry> servers_;
    std::map<std::string, int> srvPingMs_;   // host:port -> ms; -1 超时, -2 测试中
    int  srvSel_ = -1;                 // 列表选中行
    int  srvScroll_ = 0;               // 列表滚动偏移
    int  srvEditIdx_ = -1;             // >=0 表示正在编辑该条目; -1 表示表单为"新增"
    RECT srvListArea_{};

    // ---- 异步状态
    std::mutex   mtx_;
    int          progress_ = -1;
    std::wstring statusText_;
    std::vector<std::wstring> logs_;
    std::atomic<bool> busy_{ false };
    std::atomic<bool> dirty_{ true };
    std::atomic<bool> manifestLoaded_{ false };
    std::wstring lastError_;

    // ---- 命中区
    std::vector<HitRect> hits_;
    RECT listRect_{};
    RECT memSlider_{};
    int  rowH_ = 44;
    CoverStore covers_;            // 封面缓存 (仅 UI 线程)

    // ---- 等比缩放 (最大化 / 拉伸时文字与控件同步放大)
    float uiScale_   = 1.0f;       // 逻辑坐标 -> 物理像素 的缩放系数
    float editScale_ = 0.0f;       // 原生 EDIT 控件当前字体对应的缩放系数
    void  updateScale(int physW, int physH);
    int   S(int v) const { return (int)std::lround(v * (double)uiScale_); }
    int   toLogicalX(int x) const { return (int)std::lround(x / (double)uiScale_); }
    int   toLogicalY(int y) const { return (int)std::lround(y / (double)uiScale_); }

    // ---- 模组下载页状态 (共享数据一律在 mtx_ 内读写)
    std::wstring modsKeyword_;
    std::string  modsQuery_;                  // 本次检索实际使用的关键词
    std::vector<McModItem> modsItems_;
    int          modsSel_ = -1;
    int          modsScroll_ = 0;
    bool         modsLoading_ = false;
    std::atomic<bool> modsDirty_{ false };    // 检索结果已更新, 需重新下载封面
    std::wstring modsErr_;

    McModDetail  detail_;
    int          modDetail_ = -1;             // 详情所属列表下标
    bool         detailLoading_ = false;
    bool         detailReady_ = false;        // true=详情已就绪 false=失败/等待
    bool         detailFromCache_ = false;
    std::wstring detailErr_;
    std::wstring detailMcver_;
    int          detailLoader_ = -1;          // 0 Fabric 1 Forge 2 NeoForge 3 Quilt

    std::vector<ModFile> modFiles_;
    struct InstalledMod { std::wstring file; std::wstring name; unsigned long long size; bool enabled; };
    std::vector<InstalledMod> installedMods_;
    int modMgrScroll_ = 0;
    std::wstring modMgrFilter_;      // 模组列表筛选词
    struct JavaEntry { std::wstring path; int major; };
    std::vector<JavaEntry> javaFound_;
    bool javaScanning_ = false;
    struct SaveEntry { std::wstring name; std::wstring path; unsigned long long size; FILETIME modified; };
    std::vector<SaveEntry> saves_;
    struct ResDlItem { std::string id; std::wstring title; std::wstring author; std::wstring desc; int downloads; };
    std::vector<ResDlItem> resDlItems_;
    int resDlType_ = 0;
    bool resDlLoading_ = false;
    std::wstring resDlErr_;
    bool         filesLoading_ = false;
    bool         filesLoaded_ = false;
    std::wstring filesErr_;

    // ---- 下载状态 (进度/开关跨线程, 用原子; 文本在 mtx_ 内读写)
    std::atomic<bool> dlActive_{ false };
    std::atomic<int>  dlPct_{ 0 };
    std::wstring dlName_;
    std::wstring dlResult_;

    bool listCoversRunning_ = false;          // 列表封面补全任务是否在跑
    int  listCoverFetched_ = 0;               // 已补全封面的条目数
    bool mrSearchRunning_ = false;            // Modrinth 搜索兜底解析是否在跑

    // ---- 布局缓存
    RECT detailLinkRects_[100]{};
    int  detailLinkIds_[100]{};
    int  detailLinkCount_ = 0;
    RECT listArea_{};
    RECT detailArea_{};

    MainWindow() = default;

    // 窗口过程
    static LRESULT CALLBACK wndProc(HWND h, UINT m, WPARAM w, LPARAM l);

    bool createWindow(HINSTANCE hInst, int nCmdShow);
    void onPaint(HDC hdc);
    void onLButtonDown(int x, int y);
    void onLButtonUp(int x, int y);
    void onMouseMove(int x, int y);
    void onMouseWheel(int delta);
    void onTimer();
    void onCommand(int id);
    void toggleFullscreen();
    void layoutControls();
    void syncEdits();
    HWND editName_ = nullptr;
    HWND editSearch_ = nullptr;
    HWND editDir_ = nullptr;
    HWND editJava_ = nullptr;
    HWND editMem_ = nullptr;
    HWND editModSearch_ = nullptr;
    HWND editModMcVer_ = nullptr;
    HWND editSrvName_ = nullptr;
    HWND editSrvHost_ = nullptr;
    HWND editSrvPort_ = nullptr;
    HWND editAccount_ = nullptr;

    // 绘制各区域
    void drawSidebar(Graphics& g, int W, int H);
    void drawHeader(Graphics& g, int W);
    void drawHome(Graphics& g, int W, int H);
    void drawVersions(Graphics& g, int W, int H);
    void drawSettings(Graphics& g, int W, int H);
    void drawAbout(Graphics& g, int W, int H);
    void drawMods(Graphics& g, int W, int H);
    void drawModDetail(Graphics& g, int W, int H);
    void drawButton(Graphics& g, const RECT& rc, const std::wstring& text, int id,
                    bool primary, bool enabled = true, int fontSize = 13);
    void drawCard(Graphics& g, const RECT& rc);
    void drawField(Graphics& g, const RECT& rc, const std::wstring& label,
                   const std::wstring& value, int labelW);

    // 业务动作
    void refreshManifestAsync();
    void installSelectedAsync();
    void launchSelectedAsync();
    void loadSettings();
    void saveSettings();
    void applySettingsToLauncher();
    void checkUpdateAsync();

    // ---- 模组页业务动作
    void openModPage();
    void searchModsAsync(const std::wstring& keyword);
    void ensureDetailAsync(int idx);            // 选中列表项后按需拉取详情
    void fetchModFilesAsync();
    void fetchCoversAsync();                    // 结果页列表封面补全 (低并发顺序拉取)
    void resolveModrinthBySearchAsync();        // 详情缺 Modrinth 链接时的搜索兜底
    void startModDownload(const ModFile& mf);
    void ensureCover(const std::wstring& url);
    Gdiplus::Image* coverImage(const std::wstring& url);
    std::wstring modsDir() const;
    std::wstring activeGameDir() const;   // 版本隔离后的实际数据目录
    int  collectGameVersions(std::vector<std::wstring>& out) const;   // 扫描本地 versions 目录

    // ---- 联机页业务动作
    void openServersPage();
    void drawServers(Graphics& g, int W, int H);
    void drawResources(Graphics& g, int W, int H);
    void drawModMgr(Graphics& g, int W, int H);
    void drawSaves(Graphics& g, int W, int H);
    void drawAccounts(Graphics& g, int W, int H);
    void drawGameOptions(Graphics& g, int W, int H);
    void loadServers();
    void saveServers();
    void fillServerForm(int idx);        // 条目载入表单 (idx < 0 表示清空)
    void commitServerForm();             // 表单 -> 新增 / 更新条目 (含校验)
    void deleteServer(int idx);
    void connectServer(int idx);         // 一键启动并直连该服务器
    void pingServerAsync(int idx);       // TCP 连接测速
    void openResourceFolder(int idx);    // 打开资源包/光影/存档等目录
    void handleDropFiles(HDROP h);       // 拖入 .jar / .zip 导入
    void drawResDl(Graphics& g, int W, int H);
    void searchResDlAsync();
    void downloadResLatest(int idx);
    void analyzeCrash();                 // 读取最新崩溃报告并给出建议
    void exportDiagnostics();            // 导出诊断信息
    void installFabricAsync();           // 为当前版本安装 Fabric
    void installForgeAsync();            // 为当前版本安装 Forge
    void downloadLatestAsync();          // 下载最新启动器到 downloads
    void loadInstalledMods();            // 扫描 mods 目录
    void toggleInstalledMod(int idx);    // 启用/禁用模组
    void deleteInstalledMod(int idx);    // 删除模组
    void setAllMods(bool enable);        // 全部启用 / 全部禁用
    void scanJavaVersions();             // 扫描常见 Java 安装
    void syncAccountList();              // 确保当前昵称在账户列表
    void maybeAutoBackup();              // 启动时自动备份存档(每天一次)
    void loadGameOptions();              // 读取 options.txt
    void saveGameOptions();              // 写回 options.txt
    void addAccount();                   // 添加离线账户
    void useAccount(int idx);            // 使用账户
    void deleteAccount(int idx);         // 删除账户
    void useJavaVersion(int idx);        // 选用扫描到的 Java
    void loadSaves();                    // 扫描 saves 目录
    void backupSave(int idx);            // 备份存档
    void deleteSave(int idx);            // 删除存档
    void restoreLatestBackup();          // 从最近备份恢复
    void toggleServerVisible(int idx);   // 切换"显示服务器 / 不显示服务器"
    void writePublicServers();           // 把"显示"的条目同步到公开清单
    void importPublicServers();          // 从公开清单导入本机没有的服务器

    std::wstring contentTitle() const;
    void pushLog(const std::wstring& s);
    void pushLogLocked(const std::wstring& s);   // 调用方已持有 mtx_ 时必须用这个
    void setStatus(int p, const std::wstring& t);
};

// ------------------------------ 窗口创建 -----------------------------------
LRESULT CALLBACK MainWindow::wndProc(HWND h, UINT m, WPARAM w, LPARAM l) {
    MainWindow& self = MainWindow::inst();
    switch (m) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(h, &ps);
        self.onPaint(hdc);
        EndPaint(h, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    case WM_SIZE:
        self.layoutControls();
        InvalidateRect(h, nullptr, FALSE);
        return 0;
    case WM_GETMINMAXINFO: {
        if (self.fullscreen_) return 0;
        MINMAXINFO* mmi = (MINMAXINFO*)l;
        mmi->ptMinTrackSize.x = MIN_W;
        mmi->ptMinTrackSize.y = MIN_H;
        return 0;
    }
    case WM_KEYDOWN:
        if (w == VK_F9) { self.analyzeCrash(); return 0; }
        // F11 / Alt+Enter 切换全屏, 其余按键交回系统处理
        if (w == VK_F11 ||
            (w == VK_RETURN && (GetKeyState(VK_MENU) & 0x8000))) {
            self.toggleFullscreen();
            return 0;
        }
        break;
    case WM_LBUTTONDOWN:
        SetFocus(h);
        self.onLButtonDown(self.toLogicalX(GET_X_LPARAM(l)), self.toLogicalY(GET_Y_LPARAM(l)));
        return 0;
    case WM_LBUTTONUP:
        self.onLButtonUp(self.toLogicalX(GET_X_LPARAM(l)), self.toLogicalY(GET_Y_LPARAM(l)));
        return 0;
    case WM_MOUSEMOVE:
        self.onMouseMove(self.toLogicalX(GET_X_LPARAM(l)), self.toLogicalY(GET_Y_LPARAM(l)));
        return 0;
    case WM_MOUSEWHEEL:
        self.onMouseWheel(GET_WHEEL_DELTA_WPARAM(w));
        return 0;
    case WM_TIMER:
        if (w == 1) self.onTimer();
        return 0;
    case WM_CTLCOLOREDIT: {
        HDC hdc = (HDC)w;
        SetTextColor(hdc, RGB(28, 30, 34));
        SetBkColor(hdc, RGB(242, 243, 245));
        return (LRESULT)self.brushInput_;
    }
    case WM_DROPFILES:
        self.handleDropFiles((HDROP)w);
        return 0;
    case WM_COMMAND:
        if (HIWORD(w) == EN_CHANGE) {
            if (LOWORD(w) == CTL_SEARCH) {
                wchar_t b[256] = { 0 };
                GetWindowTextW((HWND)l, b, 256);
                self.verSearch_ = trimW(std::wstring(b));
                self.listScroll_ = 0;
                self.selList_ = -1;
                self.dirty_ = true;
                InvalidateRect(h, nullptr, FALSE);
            } else if (LOWORD(w) == CTL_MOD_SEARCH && self.page_ == 7) {
                wchar_t b[256] = { 0 };
                GetWindowTextW((HWND)l, b, 256);
                self.modMgrFilter_ = trimW(std::wstring(b));
                self.modMgrScroll_ = 0;
                self.dirty_ = true;
                InvalidateRect(h, nullptr, FALSE);
            }
            // 输入内容变化 -> 稍后由按钮点击时统一读取
            return 0;
        }
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, m, w, l);
}

bool MainWindow::createWindow(HINSTANCE hInst, int nCmdShow) {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWindow::wndProc;
    wc.hInstance = hInst;
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"MCLMainWindow";
    wc.hIcon = LoadIcon(nullptr, IDI_APPLICATION);
    if (!RegisterClassExW(&wc)) return false;

    // 初始窗口尺寸按系统 DPI 换算: 设计逻辑尺寸为 1000x680
    // (高 DPI 显示器下自动放大, 避免窗口与字体偏小)
    float dpiScale = 1.0f;
    {
        typedef UINT(WINAPI* GetDpiForSystemFn)(void);
        HMODULE u32 = GetModuleHandleW(L"user32.dll");
        GetDpiForSystemFn gdf = u32 ? (GetDpiForSystemFn)GetProcAddress(u32, "GetDpiForSystem") : nullptr;
        if (gdf) {
            UINT d = gdf();
            if (d >= 96 && d <= 960) dpiScale = (float)d / 96.0f;
        }
    }
    hwnd_ = CreateWindowExW(0, L"MCLMainWindow", L"MCL 启动器  -  Minecraft Launcher",
                            WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
                            (int)std::lround(1000.0 * (double)dpiScale),
                            (int)std::lround(680.0 * (double)dpiScale),
                            nullptr, nullptr, hInst, nullptr);
    if (!hwnd_) return false;

    brushInput_ = CreateSolidBrush(RGB(242, 243, 245));
    uiFont_ = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                          DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                          CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");

    // 文本输入控件
    auto mkEdit = [&](int id) -> HWND {
        HWND e = CreateWindowExW(0, L"EDIT", L"", WS_CHILD | ES_AUTOHSCROLL,
                                0, 0, 100, 28, hwnd_, (HMENU)(INT_PTR)id, hInst, nullptr);
        SendMessageW(e, WM_SETFONT, (WPARAM)uiFont_, TRUE);
        return e;
    };
    editName_   = mkEdit(CTL_NAME);
    editSearch_ = mkEdit(CTL_SEARCH);
    editDir_    = mkEdit(200);
    editJava_   = mkEdit(201);
    editMem_    = mkEdit(202);
    editModSearch_ = mkEdit(CTL_MOD_SEARCH);
    editModMcVer_  = mkEdit(CTL_MOD_MCVER);
    editSrvName_   = mkEdit(CTL_SRV_NAME);
    editSrvHost_   = mkEdit(CTL_SRV_HOST);
    editSrvPort_   = mkEdit(CTL_SRV_PORT);
    editAccount_   = mkEdit(CTL_ACCOUNT);

    loadSettings();
    if (accounts_.empty()) accounts_.push_back(playerName_);
    syncAccountList();
    SetWindowTextW(editName_, playerName_.c_str());
    loadServers();
    applySettingsToLauncher();

    layoutControls();
    DragAcceptFiles(hwnd_, TRUE);
    ShowWindow(hwnd_, nCmdShow);
    UpdateWindow(hwnd_);

    SetTimer(hwnd_, 1, 120, nullptr);
    refreshManifestAsync();
    checkUpdateAsync();
    return true;
}

// ------------------------------ 设置持久化 ---------------------------------
static std::wstring settingsPath() {
    return joinPath(exeDir(), L"mcl_settings.txt");
}

void MainWindow::loadSettings() {
    gameDir_ = joinPath(exeDir(), L".minecraft");
    javaPath_ = Launcher::autoDetectJava();
    javaInfo_ = javaPath_.empty() ? L"未检测到 Java" : L"已检测到 Java";

    std::string txt;
    std::wstring err;
    // 简单读取 (不用 httpGetToString)
    FILE* fp = _wfopen(settingsPath().c_str(), L"rb");
    bool missing = (fp == nullptr);
    if (fp) {
        char buf[4096];
        size_t n = fread(buf, 1, sizeof(buf) - 1, fp);
        buf[n] = 0;
        txt.assign(buf, n);
        fclose(fp);
        // 逐行解析 key=value
        size_t pos = 0;
        while (pos < txt.size()) {
            size_t e = txt.find('\n', pos);
            if (e == std::string::npos) e = txt.size();
            std::string line = txt.substr(pos, e - pos);
            pos = e + 1;
            if (!line.empty() && line.back() == '\r') line.pop_back();
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string k = line.substr(0, eq);
            std::string v = line.substr(eq + 1);
            if (k == "gameDir")       gameDir_ = utf8ToWide(v);
            else if (k == "java")     javaPath_ = utf8ToWide(v);
            else if (k == "player")   playerName_ = utf8ToWide(v);
            else if (k == "memory")   { try { memoryMB_ = std::stoi(v); } catch (...) {} }
            else if (k == "mirror")   useMirror_ = (v == "1");
            else if (k == "autobackup") autoBackup_ = (v == "1");
            else if (k == "isolation") isolation_ = (v == "1");
            else if (k == "version")  selVersion_ = utf8ToWide(v);
            else if (k == "verCat")   { try { verCat_ = std::stoi(v); } catch (...) { verCat_ = 0; } }
            else if (k == "accounts") {
                accounts_.clear();
                for (auto& n : splitW(utf8ToWide(v), L',')) {
                    std::wstring t = trimW(n);
                    if (!t.empty()) accounts_.push_back(t);
                }
            }
        }
    }
    if (missing) {
        // 配置文件缺失: 用内置默认值在本地生成一份, 保证程序可以直接启动。
        // (后续若要改成从远程源下载配置, 只需把这一段替换为下载逻辑即可)
        gameDir_ = joinPath(exeDir(), L".minecraft");
        if (javaPath_.empty()) javaPath_ = Launcher::autoDetectJava();
        if (playerName_.empty()) playerName_ = L"Steve";
        if (memoryMB_ < kMemMinMB) memoryMB_ = recommendMemoryMB();
        useMirror_ = true;
        saveSettings();
        pushLog(L"未找到配置文件, 已按内置默认值生成: " + settingsPath());
    }
    if (verCat_ < 0 || verCat_ > 4) verCat_ = 0;   // 分类值越界回退到“全部”
    if (playerName_.empty()) playerName_ = L"Steve";
    // 未配置或配置低于 3GB 时, 按物理内存给出推荐值 (不小于 4096MB)
    if (memoryMB_ < 3072) memoryMB_ = recommendMemoryMB();
    if (gameDir_.empty()) gameDir_ = joinPath(exeDir(), L".minecraft");
    if (!pathExists(gameDir_)) {
        createDirs(gameDir_);
        if (!pathExists(gameDir_)) gameDir_ = joinPath(exeDir(), L".minecraft");
    }
    if (!javaPath_.empty() && !pathExists(javaPath_)) {
        std::wstring bad = javaPath_;
        std::wstring autoJ = Launcher::autoDetectJava();
        if (!autoJ.empty()) javaPath_ = autoJ;
        pushLog(L"Java 路径失效: " + bad + L" -> " + (autoJ.empty() ? std::wstring(L"(自动检测为空)") : autoJ));
    }
    // 没有任何版本信息时, 退回到本地已安装的第一个版本, 方便直接开玩
    if (selVersion_.empty()) {
        std::vector<std::string> local = launcher_.listInstalled();
        if (!local.empty()) selVersion_ = utf8ToWide(local[0]);
    }
}

void MainWindow::syncAccountList() {
    if (playerName_.empty()) playerName_ = L"Steve";
    for (auto& a : accounts_) if (a == playerName_) return;
    accounts_.push_back(playerName_);
}

std::wstring MainWindow::activeGameDir() const {
    std::wstring root = gameDir_.empty() ? joinPath(exeDir(), L".minecraft") : gameDir_;
    if (isolation_ && !selVersion_.empty()) {
        return joinPath(joinPath(root, L"versions"), selVersion_);
    }
    return root;
}

void MainWindow::saveSettings() {
    syncAccountList();
    FILE* fp = _wfopen(settingsPath().c_str(), L"wb");
    if (!fp) return;
    std::string out;
    out += "gameDir=" + wideToUtf8(gameDir_) + "\r\n";
    out += "java=" + wideToUtf8(javaPath_) + "\r\n";
    out += "player=" + wideToUtf8(playerName_) + "\r\n";
    out += "memory=" + std::to_string(memoryMB_) + "\r\n";
    out += "mirror=" + std::string(useMirror_ ? "1" : "0") + "\r\n";
    out += "autobackup=" + std::string(autoBackup_ ? "1" : "0") + "\r\n";
    out += "isolation=" + std::string(isolation_ ? "1" : "0") + "\r\n";
    out += "version=" + wideToUtf8(selVersion_) + "\r\n";
    out += "verCat=" + std::to_string(verCat_) + "\r\n";
    out += "accounts=";
    for (size_t i = 0; i < accounts_.size(); ++i) {
        if (i) out += ",";
        out += wideToUtf8(accounts_[i]);
    }
    out += "\r\n";
    fwrite(out.data(), 1, out.size(), fp);
    fclose(fp);
}

void MainWindow::applySettingsToLauncher() {
    launcher_.opt.gameDir = activeGameDir();
    if (isolation_) {
        std::wstring gd = activeGameDir();
        createDirs(gd);
        createDirs(joinPath(gd, L"mods"));
        createDirs(joinPath(gd, L"saves"));
        createDirs(joinPath(gd, L"resourcepacks"));
        createDirs(joinPath(gd, L"shaderpacks"));
    }
    launcher_.opt.javaPath = javaPath_;
    launcher_.opt.playerName = wideToUtf8(playerName_);
    launcher_.opt.maxMemoryMB = memoryMB_;
    launcher_.opt.useMirror = useMirror_;
    // 直连参数只在联机页"一键进服"这一条路径上注入, 其余入口必须保持为空
    launcher_.opt.serverAddress.clear();
    launcher_.opt.serverPort = 0;
    launcher_.onLog = [this](const std::wstring& s) { pushLog(s); };
    launcher_.onProgress = [this](int p, const std::wstring& t) -> bool {
        setStatus(p, t);
        return !launcher_.cancelled.load();
    };
}

// ------------------------------ 状态交互 -----------------------------------
static std::mutex g_logFileMutex;
static void appendLogFile(const std::wstring& s) {
    std::lock_guard<std::mutex> lk(g_logFileMutex);
    std::wstring path = joinPath(exeDir(), L"launcher.log");
    WIN32_FILE_ATTRIBUTE_DATA fa{};
    if (GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &fa)) {
        ULARGE_INTEGER sz;
        sz.LowPart = fa.nFileSizeLow;
        sz.HighPart = fa.nFileSizeHigh;
        if (sz.QuadPart > 4ULL * 1024 * 1024) {
            MoveFileExW(path.c_str(), (path + L".old").c_str(), MOVEFILE_REPLACE_EXISTING);
        }
    }
    FILE* fp = _wfopen(path.c_str(), L"ab");
    if (!fp) return;
    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t ts[32];
    swprintf(ts, 32, L"%04d-%02d-%02d %02d:%02d:%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    std::wstring line = L"[" + std::wstring(ts) + L"] " + s + L"\r\n";
    std::string u8 = wideToUtf8(line);
    fwrite(u8.data(), 1, u8.size(), fp);
    fclose(fp);
}

void MainWindow::pushLogLocked(const std::wstring& s) {
    // 前置条件: 调用方已持有 mtx_。持锁时禁止再调 pushLog(), 否则自锁死。
    appendLogFile(s);
    logs_.push_back(s);
    if (logs_.size() > 400) logs_.erase(logs_.begin(), logs_.begin() + 100);
    dirty_ = true;
}

void MainWindow::pushLog(const std::wstring& s) {
    std::lock_guard<std::mutex> lk(mtx_);
    pushLogLocked(s);
}

void MainWindow::setStatus(int p, const std::wstring& t) {
    std::lock_guard<std::mutex> lk(mtx_);
    progress_ = p;
    statusText_ = t;
    dirty_ = true;
}

// ------------------------------ 布局 ---------------------------------------
void MainWindow::layoutControls() {
    RECT rc;
    GetClientRect(hwnd_, &rc);
    updateScale(rc.right, rc.bottom);
    int W = toLogicalX(rc.right), H = toLogicalY(rc.bottom);

    // 原生 EDIT 控件不参与 GDI+ 变换, 需按当前缩放重建字体
    {
        float fs = editScale_;
        bool need = !(fs > uiScale_ - 0.001f && fs < uiScale_ + 0.001f);
        if (need && uiScale_ > 0.0f) {
            HFONT nf = CreateFontW(-(int)std::lround(14.0 * (double)uiScale_), 0, 0, 0,
                                   FW_NORMAL, FALSE, FALSE, FALSE,
                                   DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                   CLEARTYPE_QUALITY, DEFAULT_PITCH, L"Microsoft YaHei");
            HWND all[] = { editName_, editSearch_, editDir_, editJava_, editMem_,
                           editModSearch_, editModMcVer_, editSrvName_, editSrvHost_, editSrvPort_, editAccount_ };
            for (HWND e : all) if (e) SendMessageW(e, WM_SETFONT, (WPARAM)nf, TRUE);
            if (uiFont_) DeleteObject(uiFont_);
            uiFont_ = nf;
            editScale_ = uiScale_;
        }
    }

    auto hide = [](HWND h) { ShowWindow(h, SW_HIDE); };
    hide(editName_); hide(editSearch_); hide(editDir_); hide(editJava_); hide(editMem_);
    hide(editModSearch_); hide(editModMcVer_);
    hide(editSrvName_); hide(editSrvHost_); hide(editSrvPort_); hide(editAccount_);

    int cx = SIDEBAR_W + 24;
    int cw = W - SIDEBAR_W - 48;

    if (page_ == 0) {
        int y = HEADER_H + 16;
        // 卡片1: 当前版本
        int fieldY = y + 44;
        RECT r1 = mkRect(cx + cw - 220, fieldY, cx + cw - 8, fieldY + 30);
        (void)r1;
        // 昵称输入框 (第二张卡片左半)
        int card2Y = y + 64 + 12;
        int half = (cw - 16) / 2;
        SetWindowPos(editName_, nullptr, S(cx + 16), S(card2Y + 32), S(half - 32), S(30), SWP_NOZORDER);
        ShowWindow(editName_, SW_SHOW);
    } else if (page_ == 1) {
        int y = HEADER_H + 16;
        SetWindowPos(editSearch_, nullptr, S(cx + 16), S(y + 12), S(260), S(30), SWP_NOZORDER);
        ShowWindow(editSearch_, SW_SHOW);
    } else if (page_ == 2) {
        int y = HEADER_H + 16;
        int c2 = y + 64 + 12;
        int c3 = c2 + 64 + 12;
        SetWindowPos(editDir_, nullptr, S(cx + 100), S(y + 18), S(cw - 440), S(28), SWP_NOZORDER);
        ShowWindow(editDir_, SW_SHOW);
        SetWindowPos(editJava_, nullptr, S(cx + 100), S(c2 + 18), S(cw - 430), S(28), SWP_NOZORDER);
        ShowWindow(editJava_, SW_SHOW);
        SetWindowPos(editMem_, nullptr, S(cx + 100), S(c3 + 18), S(120), S(28), SWP_NOZORDER);
        ShowWindow(editMem_, SW_SHOW);
    } else if (page_ == 4) {
        int y = HEADER_H + 16;
        if (modDetail_ < 0) {
            int sw = cw - 416; if (sw < 160) sw = 160;
            SetWindowPos(editModSearch_, nullptr, S(cx + 16), S(y + 10), S(sw), S(30), SWP_NOZORDER);
            ShowWindow(editModSearch_, SW_SHOW);
        }
        SetWindowPos(editModMcVer_, nullptr, S(cx + cw - 210), S(y + 10), S(194), S(30), SWP_NOZORDER);
        ShowWindow(editModMcVer_, SW_SHOW);
    } else if (page_ == 7) {
        int y = HEADER_H + 16;
        SetWindowPos(editModSearch_, nullptr, S(cx + 64), S(y + 60), S(cw - 500), S(28), SWP_NOZORDER);
        ShowWindow(editModSearch_, SW_SHOW);
    } else if (page_ == 12) {
        int y = HEADER_H + 16;
        SetWindowPos(editModSearch_, nullptr, S(cx + 96), S(y + 12), S(cw - 520), S(30), SWP_NOZORDER);
        ShowWindow(editModSearch_, SW_SHOW);
    } else if (page_ == 10) {
        int addTop = H - 108;
        SetWindowPos(editAccount_, nullptr, S(cx + 120), S(addTop + 28), S(cw - 300), S(30), SWP_NOZORDER);
        ShowWindow(editAccount_, SW_SHOW);
    } else if (page_ == 5) {
        int y = HEADER_H + 16;
        int hostX = cx + 264;                 // 与 drawServers 的表单卡片保持一致
        int portLabelX = cx + cw - 274;
        int portX = cx + cw - 222;
        int hostW = portLabelX - 8 - hostX;
        if (hostW < 90) hostW = 90;
        SetWindowPos(editSrvName_, nullptr, S(cx + 60), S(y + 14), S(140), S(30), SWP_NOZORDER);
        ShowWindow(editSrvName_, SW_SHOW);
        SetWindowPos(editSrvHost_, nullptr, S(hostX), S(y + 14), S(hostW), S(30), SWP_NOZORDER);
        ShowWindow(editSrvHost_, SW_SHOW);
        SetWindowPos(editSrvPort_, nullptr, S(portX), S(y + 14), S(76), S(30), SWP_NOZORDER);
        ShowWindow(editSrvPort_, SW_SHOW);
    }
    // 窗口尺寸变化时文本控件的字体已在上方按缩放重建
}

// 依据客户区物理尺寸计算等比缩放系数 (取宽高比例较小者, 保证内容不被裁切)
void MainWindow::updateScale(int physW, int physH) {
    if (physW <= 0 || physH <= 0) return;
    float sx = (float)physW / DESIGN_W;
    float sy = (float)physH / DESIGN_H;
    float s = (sx < sy) ? sx : sy;
    if (s < SCALE_MIN) s = SCALE_MIN;
    if (s > SCALE_MAX) s = SCALE_MAX;
    uiScale_ = s;
}


// ------------------------------ 绘制 ---------------------------------------
void MainWindow::onPaint(HDC hdc) {
    RECT rc;
    GetClientRect(hwnd_, &rc);
    const int PW = rc.right, PH = rc.bottom;
    if (PW <= 0 || PH <= 0) return;
    updateScale(PW, PH);
    const float s = uiScale_;
    // 逻辑尺寸: 绘制统一在逻辑坐标系里进行, 由 ScaleTransform 放大到物理像素
    const int W = toLogicalX(PW), H = toLogicalY(PH);

    HDC mem = CreateCompatibleDC(hdc);
    HBITMAP bmp = CreateCompatibleBitmap(hdc, PW, PH);
    HBITMAP old = (HBITMAP)SelectObject(mem, bmp);

    {
        Graphics g(mem);
        g.SetSmoothingMode(SmoothingModeAntiAlias);
        g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);
        g.ScaleTransform(s, s);

        SolidBrush bg(kContentBg);
        g.FillRectangle(&bg, 0, 0, W, H);

        hits_.clear();
        drawSidebar(g, W, H);
        drawHeader(g, W);
        switch (page_) {
        case 0: drawHome(g, W, H); break;
        case 1: drawVersions(g, W, H); break;
        case 2: drawSettings(g, W, H); break;
        case 4: (modDetail_ >= 0) ? drawModDetail(g, W, H) : drawMods(g, W, H); break;
        case 5: drawServers(g, W, H); break;
        case 6: drawResources(g, W, H); break;
        case 7: drawModMgr(g, W, H); break;
        case 8: drawSaves(g, W, H); break;
        case 10: drawAccounts(g, W, H); break;
        case 11: drawGameOptions(g, W, H); break;
        case 12: drawResDl(g, W, H); break;
        default: drawAbout(g, W, H); break;
        }
    }

    BitBlt(hdc, 0, 0, PW, PH, mem, 0, 0, SRCCOPY);
    SelectObject(mem, old);
    DeleteObject(bmp);
    DeleteDC(mem);
}

void MainWindow::drawButton(Graphics& g, const RECT& rc, const std::wstring& text, int id,
                            bool primary, bool enabled, int fontSize) {
    bool hover = (hoverId_ == id) && enabled;
    Color bg, fg;
    if (!enabled) {
        bg = C(226, 228, 232); fg = C(160, 164, 170);
    } else if (primary) {
        bg = hover ? kAccentHi : kAccent; fg = kWhite;
    } else {
        bg = hover ? kHover : kCardBg; fg = kText;
    }
    if (enabled && !primary) strokeRound(g, rc, 6, hover ? C(200, 205, 212) : kLine);
    fillRound(g, rc, 6, bg);

    FontFamily ff(L"Microsoft YaHei");
    Font f(&ff, (REAL)fontSize, FontStyleRegular, UnitPixel);
    drawText(g, text, f, fg, rc, 1);

    if (id != BTN_NONE) hits_.push_back({ rc, id });
}

void MainWindow::drawCard(Graphics& g, const RECT& rc) {
    // 轻微投影: 让卡片在内容背景上浮起, 层次更清晰
    RECT s1 = rc; s1.left += 1; s1.right += 2; s1.top += 2; s1.bottom += 4;
    fillRound(g, s1, 9, C(100, 110, 125, 16));
    RECT s2 = rc; s2.left += 1; s2.right += 1; s2.top += 1; s2.bottom += 2;
    fillRound(g, s2, 9, C(100, 110, 125, 20));
    fillRound(g, rc, 8, kCardBg);
    strokeRound(g, rc, 8, kLine);
}

void MainWindow::drawSidebar(Graphics& g, int W, int H) {
    SolidBrush bg(kSideBg);
    g.FillRectangle(&bg, 0, 0, SIDEBAR_W, H);
    Pen line(kLine);
    g.DrawLine(&line, SIDEBAR_W - 1, 0, SIDEBAR_W - 1, H);

    // Logo 区
    {
        RECT lr = mkRect(20, 22, 52, 54);
        fillRound(g, lr, 8, kAccent);
        // 简易 "草方块" 图案
        SolidBrush light(C(122, 190, 92));
        g.FillRectangle(&light, lr.left + 6, lr.top + 6, 20, 8);
        SolidBrush dark(C(88, 152, 66));
        g.FillRectangle(&dark, lr.left + 6, lr.top + 14, 20, 12);

        FontFamily ff(L"Microsoft YaHei");
        Font f(&ff, 15.0f, FontStyleBold, UnitPixel);
        Font fs(&ff, 11.0f, FontStyleRegular, UnitPixel);
        RECT tr = mkRect(62, 20, SIDEBAR_W - 8, 42);
        drawText(g, L"MCL 启动器", f, kText, tr, 0);
        RECT sr = mkRect(62, 40, SIDEBAR_W - 8, 58);
        drawText(g, L"Minecraft C++ Launcher", fs, kTextDim, sr, 0);
    }

    // 导航
    struct NavItem { const wchar_t* text; int id; int page; };
    NavItem nav[] = {
        { L"启动游戏", BTN_NAV_HOME, 0 },
        { L"版本列表", BTN_NAV_VERSIONS, 1 },
        { L"游戏设置", BTN_NAV_SETTINGS, 2 },
        { L"模组下载", BTN_NAV_MODS, 4 },
        { L"联机",     BTN_NAV_SERVERS, 5 },
        { L"资源管理", BTN_NAV_RESOURCES, 6 },
        { L"模组管理", BTN_NAV_MODMGR, 7 },
        { L"存档管理", BTN_NAV_SAVES, 8 },
        { L"账户",     BTN_NAV_ACCOUNTS, 10 },
        { L"游戏选项", BTN_NAV_GOPT, 11 },
        { L"资源下载", BTN_NAV_RESDL, 12 },
        { L"关于",     BTN_NAV_ABOUT, 3 },
    };
    FontFamily ff(L"Microsoft YaHei");
    Font f(&ff, 13.5f, FontStyleRegular, UnitPixel);

    int y = 96;
    for (auto& n : nav) {
        RECT rc = mkRect(10, y, SIDEBAR_W - 10, y + 42);
        bool active = (page_ == n.page);
        bool hover = (hoverId_ == n.id);
        if (active)       fillRound(g, rc, 7, kSelBg);
        else if (hover)   fillRound(g, rc, 7, kHover);

        RECT rr = mkRect(rc.left + 18, rc.top, rc.right - 8, rc.bottom);
        drawText(g, n.text, f, active ? kAccent : kText, rr, 0);

        if (active) {
            SolidBrush b(kAccent);
            g.FillRectangle(&b, rc.left + 8, rc.top + 12, 3, 18);
        }
        hits_.push_back({ rc, n.id });
        y += 42;
    }

    // 底部版本信息
    Font fs(&ff, 10.5f, FontStyleRegular, UnitPixel);
    RECT br = mkRect(16, H - 34, SIDEBAR_W - 12, H - 14);
    drawText(g, std::wstring(L"v") + kAppVersion + L"  ·  BMCLAPI · Win10 服务器", fs, kTextDim, br, 0);
}

std::wstring MainWindow::contentTitle() const {
    switch (page_) {
    case 0: return L"启动游戏";
    case 1: return L"版本列表";
    case 2: return L"游戏设置";
    case 4: return (modDetail_ >= 0) ? L"模组下载  ·  模组详情" : L"模组下载";
    case 5: return L"联机  ·  服务器列表";
    case 6: return L"资源管理";
    case 7: return L"模组管理";
    case 8: return L"存档管理";
    case 10: return L"账户";
    case 11: return L"游戏选项";
    case 12: return L"资源下载";
    default: return L"关于";
    }
}

void MainWindow::drawHeader(Graphics& g, int W) {
    SolidBrush bg(kContentBg);
    g.FillRectangle(&bg, SIDEBAR_W, 0, W - SIDEBAR_W, HEADER_H);
    // 顶栏与内容区之间的细分隔线
    {
        SolidBrush lb(kLine);
        g.FillRectangle(&lb, SIDEBAR_W, HEADER_H - 1, W - SIDEBAR_W, 1);
    }
    FontFamily ff(L"Microsoft YaHei");
    Font f(&ff, 20.0f, FontStyleBold, UnitPixel);
    RECT tr = mkRect(SIDEBAR_W + 24, 8, W - 150, HEADER_H - 4);
    drawText(g, contentTitle(), f, kText, tr, 0);

    // 右上角: 全屏切换入口
    RECT fb = mkRect(W - 146, HEADER_H - 46, W - 24, HEADER_H - 14);
    drawButton(g, fb, fullscreen_ ? L"退出全屏" : L"全屏 F11", BTN_FULLSCREEN, false, true, 12);
}

// ------------------------------ 首页 ---------------------------------------
void MainWindow::drawHome(Graphics& g, int W, int H) {
    FontFamily ff(L"Microsoft YaHei");
    Font fL(&ff, 12.0f, FontStyleRegular, UnitPixel);
    Font fV(&ff, 14.0f, FontStyleRegular, UnitPixel);
    Font fB(&ff, 15.0f, FontStyleBold, UnitPixel);

    int cx = SIDEBAR_W + 24;
    int cw = W - SIDEBAR_W - 48;
    int y = HEADER_H + 16;

    // --- 卡片1: 当前版本
    RECT c1 = mkRect(cx, y, cx + cw, y + 64);
    drawCard(g, c1);
    RECT l1 = mkRect(c1.left + 16, c1.top, c1.left + 100, c1.bottom);
    drawText(g, L"游戏版本", fL, kTextDim, l1, 0);
    RECT v1 = mkRect(c1.left + 100, c1.top, c1.right - 130, c1.bottom);
    drawText(g, selVersion_.empty() ? L"未选择版本" : selVersion_, fB,
             selVersion_.empty() ? kTextDim : kText, v1, 0);
    RECT b1 = mkRect(c1.right - 128, c1.top + 17, c1.right - 16, c1.bottom - 17);
    drawButton(g, b1, L"选择版本", BTN_PICK_VERSION, false, true, 12);

    // --- 卡片2: 昵称 + 内存
    int y2 = y + 64 + 12;
    RECT c2 = mkRect(cx, y2, cx + cw, y2 + 64);
    drawCard(g, c2);
    int half = (cw - 16) / 2;
    RECT l2 = mkRect(c2.left + 16, c2.top, c2.left + 16 + half - 16, c2.bottom);
    drawText(g, L"玩家昵称", fL, kTextDim, l2, 0);
    // 昵称输入框位于下半部分

    RECT l3 = mkRect(c2.left + half + 8, c2.top, c2.right - 16, c2.bottom);
    drawText(g, L"最大内存", fL, kTextDim, l3, 0);
    RECT v3 = mkRect(c2.right - 140, c2.top, c2.right - 16, c2.bottom);
    drawText(g, std::to_wstring(memoryMB_) + L" MB", fV, kText, v3, 2);

    // 内存滑块
    RECT s = mkRect(c2.left + half + 8, y2 + 40, c2.right - 16, y2 + 52);
    memSlider_ = s;
    fillRound(g, s, 6, C(230, 232, 236));
    int minMB = kMemMinMB, maxMB = kMemMaxMB;
    int pct = (memoryMB_ - minMB) * 100 / (maxMB - minMB);
    if (pct < 0) pct = 0; if (pct > 100) pct = 100;
    int w = (s.right - s.left);
    int fw = w * pct / 100;
    if (fw < 12) fw = 12;
    RECT sf = mkRect(s.left, s.top, s.left + fw, s.bottom);
    fillRound(g, sf, 6, kAccent);
    int knobX = s.left + fw;
    SolidBrush kb(kWhite);
    Pen kp(kAccent, 2.0f);
    g.FillEllipse(&kb, knobX - 7, s.top - 3, 14, s.bottom - s.top + 6);
    g.DrawEllipse(&kp, knobX - 7, s.top - 3, 14, s.bottom - s.top + 6);

    // --- 按钮行
    int by = y2 + 64 + 16;
    RECT bs = mkRect(cx, by, cx + 160, by + 46);
    drawButton(g, bs, L"开始游戏", BTN_START_GAME, true, !busy_.load(), 15);

    RECT bi = mkRect(cx + 170, by, cx + 320, by + 46);
    std::wstring instText = L"安装 / 更新版本";
    drawButton(g, bi, instText, BTN_INSTALL_SEL, false,
               !busy_.load() && !selVersion_.empty(), 13);

    RECT bu = mkRect(cx + 330, by, cx + 445, by + 46);
    drawButton(g, bu, L"检查更新", BTN_CHECK_UPDATE, false, !busy_.load(), 13);

    RECT bweb = mkRect(cx + 455, by, cx + 585, by + 46);
    drawButton(g, bweb, L"官网 / 反馈", BTN_OPEN_SITE, false, true, 13);

    RECT bc = mkRect(cx + 595, by, cx + 680, by + 46);
    if (busy_.load()) {
        drawButton(g, bc, L"取消", BTN_CANCEL, false, true, 13);
    }

    // --- 进度条 + 状态
    int py = by + 46 + 14;
    std::wstring st;
    int prog = -1;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        st = statusText_;
        prog = progress_;
    }
    if (prog >= 0 && busy_.load()) {
        RECT pb = mkRect(cx, py, cx + cw, py + 8);
        fillRound(g, pb, 4, C(230, 232, 236));
        int pw = (pb.right - pb.left) * (prog > 100 ? 100 : prog) / 100;
        if (pw > 4) {
            RECT pf = mkRect(pb.left, pb.top, pb.left + pw, pb.bottom);
            fillRound(g, pf, 4, kAccent);
        }
        RECT stx = mkRect(cx, py + 12, cx + cw, py + 32);
        drawText(g, st, fL, kTextDim, stx, 0);
        py += 34;
    } else if (!st.empty()) {
        RECT stx = mkRect(cx, py, cx + cw, py + 20);
        drawText(g, st, fL, kTextDim, stx, 0);
        py += 24;
    } else {
        py += 4;
    }

    // --- 日志
    int logTop = py + 8;
    int logBottom = H - 16;
    if (logBottom - logTop > 60) {
        RECT lg = mkRect(cx, logTop, cx + cw, logBottom);
        fillRound(g, lg, 8, kLogBg);
        FontFamily cf(L"Consolas");
        Font cf1(&cf, 12.0f, FontStyleRegular, UnitPixel);

        std::vector<std::wstring> ls;
        { std::lock_guard<std::mutex> lk(mtx_); ls = logs_; }
        int lineH = 18;
        int maxLines = (lg.bottom - lg.top - 16) / lineH;
        int start = (int)ls.size() - maxLines;
        if (start < 0) start = 0;
        int ly = lg.top + 8;
        for (int i = start; i < (int)ls.size(); ++i) {
            RECT r = mkRect(lg.left + 12, ly, lg.right - 12, ly + lineH);
            drawText(g, ls[i], cf1, kLogText, r, 0);
            ly += lineH;
        }
        if (ls.empty()) {
            RECT r = mkRect(lg.left + 12, lg.top + 8, lg.right - 12, lg.top + 8 + lineH);
            drawText(g, L"就绪。点击「开始游戏」即可启动，或先在版本列表中选择并安装版本。",
                     cf1, C(120, 126, 134), r, 0);
        }
    }
}

// ------------------------------ 版本列表页 ---------------------------------
// ------------------------- 版本分类筛选 (参照 PCL2) -------------------------
// 0 全部 / 1 正式版(Release) / 2 快照(Snapshot) / 3 远古版(Old Alpha·Beta) / 4 愚人节版
static const wchar_t* kVerCatNames[5] = { L"全部", L"正式版", L"快照", L"远古版", L"愚人节版" };

// 愚人节版本识别: 已知愚人节版本 id 名单 + 每年 4 月 1 日发布的快照
static bool isAprilFoolsVersion(const std::string& type, const std::string& id,
                                const std::string& releaseTime) {
    if (type != "snapshot") return false;
    static const char* kKnown[] = {
        "15w14a", "1.RV-Pre1", "3D Shareware v1.34", "20w14infinite",
        "22w13oneblockatatime", "23w13a_or_b", "24w14potato", "25w14craftmine",
    };
    for (const char* k : kKnown) if (id == k) return true;
    // 发布时间落在 4 月 1 日 (清单形如 2020-04-01T12:47:08+00:00)
    return releaseTime.size() >= 10 && releaseTime.compare(5, 5, "-04-01") == 0;
}

static bool versionMatchesSearch(const std::wstring& query, const std::string& id) {
    if (query.empty()) return true;
    std::wstring w = utf8ToWide(id);
    std::wstring q = query;
    for (auto& c : w) c = (wchar_t)towlower(c);
    for (auto& c : q) c = (wchar_t)towlower(c);
    return w.find(q) != std::wstring::npos;
}

static bool versionInCategory(int cat, const std::string& type, const std::string& id,
                              const std::string& releaseTime) {
    switch (cat) {
    case 1: return type == "release";
    case 2: return type == "snapshot" && !isAprilFoolsVersion(type, id, releaseTime);
    case 3: return type == "old_alpha" || type == "old_beta";
    case 4: return isAprilFoolsVersion(type, id, releaseTime);
    default: return true;
    }
}

void MainWindow::drawVersions(Graphics& g, int W, int H) {
    FontFamily ff(L"Microsoft YaHei");
    Font fL(&ff, 12.0f, FontStyleRegular, UnitPixel);
    Font fN(&ff, 13.5f, FontStyleBold, UnitPixel);
    Font fS(&ff, 11.0f, FontStyleRegular, UnitPixel);

    int cx = SIDEBAR_W + 24;
    int cw = W - SIDEBAR_W - 48;
    int y = HEADER_H + 16;

    // 工具栏
    RECT tb = mkRect(cx, y, cx + cw, y + 94);
    drawCard(g, tb);
    RECT br = mkRect(tb.right - 104, y + 12, tb.right - 16, y + 42);
    drawButton(g, br, L"刷新列表", BTN_REFRESH, false, !busy_.load(), 12);

    RECT bc2 = mkRect(tb.right - 104 - 130, y + 12, tb.right - 112 - 12, y + 42);
    drawButton(g, bc2, showInstalledOnly_ ? L"显示全部版本" : L"仅看已安装",
               BTN_TOGGLE_INSTALLED, false, true, 12);
    RECT bfab = mkRect(tb.right - 400, y + 12, tb.right - 260, y + 42);
    drawButton(g, bfab, L"安装 Fabric", BTN_INSTALL_FABRIC, false, !busy_.load(), 12);
    RECT bforge = mkRect(tb.right - 548, y + 12, tb.right - 410, y + 42);
    drawButton(g, bforge, L"安装 Forge", BTN_INSTALL_FORGE, false, !busy_.load(), 12);

    // 分类标签: 全部 / 正式版 / 快照 / 远古版 / 愚人节版
    for (int i = 0; i < 5; ++i) {
        int bx = cx + 16 + i * 96;
        RECT bc = mkRect(bx, y + 52, bx + 88, y + 82);
        drawButton(g, bc, kVerCatNames[i], BTN_VERCAT_BASE + i, verCat_ == i, true, 12);
    }

    // 列表
    int ly = y + 94 + 12;
    int lh = H - ly - 76;
    listRect_ = mkRect(cx, ly, cx + cw, ly + lh);
    drawCard(g, listRect_);

    // 加锁取共享数据快照, 避免绘制线程与后台下载线程的数据竞争
    std::vector<VersionInfo> manSnap;
    std::vector<int> filtSnap;
    std::set<std::string> instSnap;
    std::vector<std::string> instOnlySnap;
    bool manLoaded = manifestLoaded_.load();
    {
        std::lock_guard<std::mutex> lk(mtx_);
        manSnap = manifest_;
        filtSnap = filtered_;
        instSnap = installedSet_;
        instOnlySnap = installedOnly_;
    }

    // 组装可见列表
    struct Row { std::wstring name; std::wstring type; std::wstring date; bool installed; std::string id; };
    std::vector<Row> rows;
    if (manLoaded && !manSnap.empty()) {
        for (int idx : filtSnap) {
            if (idx < 0 || idx >= (int)manSnap.size()) continue;
            const VersionInfo& v = manSnap[idx];
            if (!versionInCategory(verCat_, v.type, v.id, v.releaseTime)) continue;
            if (!versionMatchesSearch(verSearch_, v.id)) continue;
            Row r;
            r.name = utf8ToWide(v.id);
            r.id = v.id;
            if (isAprilFoolsVersion(v.type, v.id, v.releaseTime)) r.type = L"愚人节版";
            else if (v.type == "release")       r.type = L"正式版";
            else if (v.type == "snapshot")      r.type = L"快照";
            else if (v.type == "old_beta")      r.type = L"远古Beta";
            else if (v.type == "old_alpha")     r.type = L"远古Alpha";
            else if (v.type == "installed")     r.type = L"已安装";
            else                                r.type = utf8ToWide(v.type);
            r.date = v.releaseTime.size() >= 10 ? utf8ToWide(v.releaseTime.substr(0, 10)) : L"";
            r.installed = instSnap.count(v.id) > 0;
            if (showInstalledOnly_ && !r.installed) continue;
            rows.push_back(r);
        }
    } else {
        for (auto& s : instOnlySnap) {
            Row r;
            r.name = utf8ToWide(s);
            r.id = s;
            r.type = L"已安装";
            r.installed = true;
            rows.push_back(r);
        }
    }

    int rowH = 46;
    rowH_ = rowH;
    int viewRows = (listRect_.bottom - listRect_.top - 8) / rowH;
    int total = (int)rows.size();
    int maxScroll = total - viewRows;
    if (maxScroll < 0) maxScroll = 0;
    if (listScroll_ > maxScroll) listScroll_ = maxScroll;
    if (listScroll_ < 0) listScroll_ = 0;

    int ry = listRect_.top + 4;
    for (int i = 0; i < viewRows && listScroll_ + i < total; ++i) {
        int ri = listScroll_ + i;
        const Row& r = rows[ri];
        RECT rc = mkRect(listRect_.left + 6, ry, listRect_.right - 6, ry + rowH - 2);

        bool hover = (hoverId_ == BTN_LIST_BASE + ri);
        bool sel = (selList_ == ri);
        if (sel)        fillRound(g, rc, 6, kSelBg);
        else if (hover) fillRound(g, rc, 6, kHover);

        // 版本名
        RECT nr = mkRect(rc.left + 14, rc.top, rc.left + 300, rc.bottom);
        drawText(g, r.name, fN, r.installed ? kAccent : kText, nr, 0);

        // 类型标签
        RECT tr = mkRect(rc.left + 300, rc.top + 12, rc.left + 390, rc.bottom - 12);
        Color tagBg = C(238, 240, 244), tagFg = C(110, 116, 124);
        if (r.type == L"正式版")                { tagBg = C(230, 242, 226); tagFg = C(60, 133, 39); }
        else if (r.type == L"愚人节版")           { tagBg = C(255, 240, 224); tagFg = C(190, 100, 20); }
        else if (r.type == L"远古Beta" || r.type == L"远古Alpha") { tagBg = C(240, 236, 228); tagFg = C(140, 110, 70); }
        fillRound(g, tr, 4, tagBg);
        drawText(g, r.type, fS, tagFg, tr, 1);

        // 日期
        RECT dr = mkRect(rc.left + 410, rc.top, rc.left + 520, rc.bottom);
        drawText(g, r.date, fS, kTextDim, dr, 0);

        // 已安装标记
        if (r.installed) {
            RECT ir = mkRect(rc.right - 100, rc.top, rc.right - 14, rc.bottom);
            drawText(g, L"已安装", fS, kAccent, ir, 2);
        }
        hits_.push_back({ rc, BTN_LIST_BASE + ri });
        ry += rowH;
    }

    if (total == 0) {
        RECT er = mkRect(listRect_.left, listRect_.top, listRect_.right, listRect_.bottom);
        std::wstring msg;
        if (!manLoaded) msg = L"正在获取版本列表...";
        else if (verCat_ > 0 && verCat_ < 5)
            msg = std::wstring(L"当前分类（") + kVerCatNames[verCat_] + L"）下没有版本";
        else msg = L"没有匹配的版本";
        drawText(g, msg, fL, kTextDim, er, 1);
    }

    // 滚动条
    if (maxScroll > 0) {
        int trackH = listRect_.bottom - listRect_.top - 12;
        int thumbH = trackH * viewRows / total;
        if (thumbH < 30) thumbH = 30;
        int thumbY = listRect_.top + 6 + (trackH - thumbH) * listScroll_ / maxScroll;
        RECT th = mkRect(listRect_.right - 9, thumbY, listRect_.right - 4, thumbY + thumbH);
        fillRound(g, th, 3, C(200, 205, 212));
    }

    // 底部操作
    int by = listRect_.bottom + 12;
    RECT b1 = mkRect(cx, by, cx + 150, by + 44);
    drawButton(g, b1, L"安装 / 更新", BTN_INSTALL_SEL, true,
               !busy_.load() && selList_ >= 0 && selList_ < total, 13);

    RECT b2 = mkRect(cx + 162, by, cx + 292, by + 44);
    bool canLaunch = false;
    if (selList_ >= 0 && selList_ < total) canLaunch = instSnap.count(rows[selList_].id) > 0;
    drawButton(g, b2, L"启动游戏", BTN_LAUNCH_SEL, false, !busy_.load() && canLaunch, 13);

    RECT b3 = mkRect(cx + 304, by, cx + 404, by + 44);
    drawButton(g, b3, L"删除版本", BTN_DELETE_SEL, false, !busy_.load() && canLaunch, 13);

    RECT sr = mkRect(cx + cw - 260, by, cx + cw, by + 44);
    drawText(g, L"共 " + std::to_wstring(total) + L" 个版本", fL, kTextDim, sr, 2);
}

// ------------------------------ 设置页 -------------------------------------
void MainWindow::scanJavaVersions() {
    if (javaScanning_) return;
    javaScanning_ = true;
    dirty_ = true;
    std::thread([this]() {
        std::vector<JavaEntry> found;
        auto add = [&](const std::wstring& p) {
            if (p.empty()) return;
            for (auto& e : found) if (_wcsicmp(e.path.c_str(), p.c_str()) == 0) return;
            int major = Launcher::queryJavaMajor(p);
            if (major > 0) found.push_back({ p, major });
        };
        auto scanBase = [&](const std::wstring& base) {
            WIN32_FIND_DATAW fd;
            HANDLE h = FindFirstFileW((base + L"\\*").c_str(), &fd);
            if (h == INVALID_HANDLE_VALUE) return;
            do {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
                if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
                std::wstring p = base + L"\\" + fd.cFileName + L"\\bin\\java.exe";
                if (GetFileAttributesW(p.c_str()) != INVALID_FILE_ATTRIBUTES) add(p);
            } while (FindNextFileW(h, &fd));
            FindClose(h);
        };
        if (!javaPath_.empty()) add(javaPath_);
        scanBase(L"C:\\Program Files\\Eclipse Adoptium");
        scanBase(L"C:\\Program Files\\Java");
        scanBase(L"C:\\Program Files\\Microsoft");
        scanBase(L"C:\\Program Files\\Zulu");
        scanBase(L"C:\\Program Files\\BellSoft");
        scanBase(L"C:\\Program Files (x86)\\Java");
        scanBase(L"C:\\Program Files (x86)\\Eclipse Adoptium");
        wchar_t local[MAX_PATH] = { 0 };
        if (GetEnvironmentVariableW(L"LOCALAPPDATA", local, MAX_PATH) > 0) {
            scanBase(std::wstring(local) + L"\\Programs\\Eclipse Adoptium");
        }
        scanBase(L"D:\\jdk");
        scanBase(L"D:\\java");
        scanBase(L"E:\\jdk");
        std::sort(found.begin(), found.end(), [](const JavaEntry& a, const JavaEntry& b) {
            return a.major > b.major;
        });
        if (found.size() > 8) found.resize(8);
        {
            std::lock_guard<std::mutex> lk(mtx_);
            javaFound_.swap(found);
            javaScanning_ = false;
        }
        dirty_ = true;
        if (hwnd_) PostMessageW(hwnd_, WM_TIMER, 0, 0);
    }).detach();
}

void MainWindow::useJavaVersion(int idx) {
    std::vector<JavaEntry> jv;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        jv = javaFound_;
    }
    if (idx < 0 || idx >= (int)jv.size()) return;
    javaPath_ = jv[idx].path;
    if (editJava_) SetWindowTextW(editJava_, javaPath_.c_str());
    saveSettings();
    applySettingsToLauncher();
    pushLog(L"已切换 Java: " + javaPath_ + L" (Java " + std::to_wstring(jv[idx].major) + L")");
    dirty_ = true;
    InvalidateRect(hwnd_, nullptr, FALSE);
}
void MainWindow::addAccount() {
    wchar_t buf[128] = { 0 };
    if (editAccount_) GetWindowTextW(editAccount_, buf, 128);
    std::wstring name = trimW(std::wstring(buf));
    if (name.empty()) {
        MessageBoxW(hwnd_, L"请输入账户昵称。", L"账户", MB_ICONINFORMATION);
        return;
    }
    if (name.find(L',') != std::wstring::npos) {
        MessageBoxW(hwnd_, L"昵称不能包含英文逗号。", L"账户", MB_ICONINFORMATION);
        return;
    }
    if (name.size() > 16) name = name.substr(0, 16);
    for (auto& a : accounts_) {
        if (a == name) {
            useAccount((int)(&a - accounts_.data()));
            return;
        }
    }
    accounts_.push_back(name);
    saveSettings();
    if (editAccount_) SetWindowTextW(editAccount_, L"");
    pushLog(L"已添加账户: " + name);
    dirty_ = true;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::useAccount(int idx) {
    if (idx < 0 || idx >= (int)accounts_.size()) return;
    playerName_ = accounts_[idx];
    if (editName_) SetWindowTextW(editName_, playerName_.c_str());
    saveSettings();
    applySettingsToLauncher();
    pushLog(L"已切换到账户: " + playerName_);
    dirty_ = true;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::deleteAccount(int idx) {
    if (idx < 0 || idx >= (int)accounts_.size()) return;
    if ((int)accounts_.size() <= 1) {
        MessageBoxW(hwnd_, L"至少保留一个账户。", L"账户", MB_ICONINFORMATION);
        return;
    }
    if (accounts_[idx] == playerName_) {
        MessageBoxW(hwnd_, L"不能删除当前正在使用的账户，请先切换到其他账户。", L"账户", MB_ICONWARNING);
        return;
    }
    std::wstring name = accounts_[idx];
    if (MessageBoxW(hwnd_, (L"确定删除账户 " + name + L" 吗？").c_str(), L"删除账户",
                    MB_ICONWARNING | MB_YESNO) != IDYES) {
        return;
    }
    accounts_.erase(accounts_.begin() + idx);
    saveSettings();
    pushLog(L"已删除账户: " + name);
    dirty_ = true;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::drawAccounts(Graphics& g, int W, int H) {
    FontFamily ff(L"Microsoft YaHei");
    Font fL(&ff, 12.0f, FontStyleRegular, UnitPixel);
    Font fB(&ff, 14.0f, FontStyleBold, UnitPixel);
    Font fD(&ff, 11.5f, FontStyleRegular, UnitPixel);

    int cx = SIDEBAR_W + 24;
    int cw = W - SIDEBAR_W - 48;
    int y = HEADER_H + 16;

    int addTop = H - 108;
    RECT list = mkRect(cx, y, cx + cw, addTop - 12);
    drawCard(g, list);
    RECT hd = mkRect(list.left + 16, list.top + 8, list.right - 16, list.top + 34);
    drawText(g, L"离线账户 · 共 " + std::to_wstring(accounts_.size()) + L" 个", fL, kTextDim, hd, 0);
    RECT hint = mkRect(list.left + 16, list.top + 30, list.right - 16, list.top + 52);
    drawText(g, L"「使用」切换当前昵称；昵称用于离线登录，不包含正版验证。", fD, kTextDim, hint, 0);

    int rowH = 48;
    int top = list.top + 58;
    int maxRows = (list.bottom - top - 8) / rowH;
    if (maxRows < 1) maxRows = 1;
    int show = (int)accounts_.size();
    if (show > maxRows) show = maxRows;
    for (int i = 0; i < show; ++i) {
        RECT row = mkRect(list.left + 12, top + i * rowH, list.right - 12, top + i * rowH + rowH - 6);
        if (accounts_[i] == playerName_) fillRound(g, row, 7, kSelBg);
        RECT rn = mkRect(row.left + 12, row.top, row.right - 220, row.bottom);
        drawText(g, accounts_[i], fB, kText, rn, 0);
        if (accounts_[i] == playerName_) {
            RECT rb = mkRect(row.right - 330, row.top + 10, row.right - 270, row.bottom - 8);
            drawText(g, L"当前", fD, kAccent, rb, 0);
        }
        RECT bu = mkRect(row.right - 260, row.top + 8, row.right - 180, row.bottom - 8);
        drawButton(g, bu, L"使用", BTN_ACCOUNT_USE_BASE + i, false, true, 11);
        RECT bd = mkRect(row.right - 170, row.top + 8, row.right - 90, row.bottom - 8);
        drawButton(g, bd, L"删除", BTN_ACCOUNT_DEL_BASE + i, false, accounts_[i] != playerName_, 11);
    }

    RECT add = mkRect(cx, addTop, cx + cw, H - 16);
    drawCard(g, add);
    RECT al = mkRect(add.left + 20, add.top, add.left + 110, add.bottom);
    drawText(g, L"添加账户", fL, kTextDim, al, 0);
    RECT ai = mkRect(add.left + 120, add.top + 28, add.right - 140, add.top + 58);
    fillRound(g, ai, 7, kInputBg);
    RECT ab = mkRect(add.right - 120, add.top + 28, add.right - 16, add.top + 58);
    drawButton(g, ab, L"添加", BTN_ACCOUNT_ADD, true, true, 12);
}
void MainWindow::maybeAutoBackup() {
    if (!autoBackup_) return;
    std::wstring saves = joinPath(activeGameDir(), L"saves");
    if (!pathExists(saves)) return;
    std::wstring backups = joinPath(activeGameDir(), L"backups");
    unsigned long long newest = 0;
    {
        WIN32_FIND_DATAW fd;
        HANDLE h = FindFirstFileW((backups + L"\\auto_*").c_str(), &fd);
        if (h != INVALID_HANDLE_VALUE) {
            do {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
                ULARGE_INTEGER t;
                t.LowPart = fd.ftLastWriteTime.dwLowDateTime;
                t.HighPart = fd.ftLastWriteTime.dwHighDateTime;
                if (t.QuadPart > newest) newest = t.QuadPart;
            } while (FindNextFileW(h, &fd));
            FindClose(h);
        }
    }
    if (newest > 0) {
        FILETIME now{};
        GetSystemTimeAsFileTime(&now);
        ULARGE_INTEGER n;
        n.LowPart = now.dwLowDateTime;
        n.HighPart = now.dwHighDateTime;
        if (n.QuadPart > newest && n.QuadPart - newest < 24ULL * 60 * 60 * 10000000ULL) return;
    }
    std::thread([this, backups, saves]() {
        createDirs(backups);
        SYSTEMTIME st{};
        GetLocalTime(&st);
        wchar_t ts[32];
        swprintf(ts, 32, L"auto_%04d%02d%02d_%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
        std::wstring dest = joinPath(backups, ts);
        std::wstring from = saves;
        from.push_back(L'\0');
        from.push_back(L'\0');
        std::wstring to = dest;
        to.push_back(L'\0');
        to.push_back(L'\0');
        SHFILEOPSTRUCTW op{};
        op.wFunc = FO_COPY;
        op.pFrom = from.c_str();
        op.pTo = to.c_str();
        op.fFlags = FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR;
        int rc = SHFileOperationW(&op);
        if (rc == 0) pushLog(L"自动备份存档完成: " + dest);
        else pushLog(L"自动备份存档失败，代码 " + std::to_wstring(rc));
        dirty_ = true;
        if (hwnd_) PostMessageW(hwnd_, WM_TIMER, 0, 0);
    }).detach();
}
static int goptFovDegrees(const std::map<std::string, std::string>& m) {
    auto it = m.find("fov");
    if (it == m.end()) return 70;
    double v = 0.0;
    try { v = std::stod(it->second); } catch (...) {}
    int deg = 0;
    if (v < -1.5 || v > 1.5) deg = (int)std::lround(v);
    else deg = 70 + (int)std::lround(v * 40.0);
    if (deg < 30) deg = 30;
    if (deg > 110) deg = 110;
    return deg;
}

static bool goptNumeric(int row, const char*& key, int& mn, int& mx, int& step) {
    switch (row) {
    case 0: key = "renderDistance";     mn = 2;  mx = 32;  step = 1;  return true;
    case 1: key = "simulationDistance"; mn = 5;  mx = 32;  step = 1;  return true;
    case 2: key = "maxFps";             mn = 30; mx = 260; step = 10; return true;
    case 3: key = "fov";                mn = 30; mx = 110; step = 5;  return true;
    default: return false;
    }
}

static std::string goptInt(const std::map<std::string, std::string>& m, const char* key, int defv) {
    auto it = m.find(key);
    if (it == m.end()) return std::to_string(defv);
    return it->second;
}

void MainWindow::loadGameOptions() {
    gopt_.clear();
    goptOrder_.clear();
    std::wstring path = joinPath(gameDir_, L"options.txt");
    FILE* fp = _wfopen(path.c_str(), L"rb");
    if (fp) {
        std::string data;
        char buf[4096];
        size_t n = 0;
        while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) data.append(buf, n);
        fclose(fp);
        size_t pos = 0;
        while (pos < data.size()) {
            size_t e = data.find('\n', pos);
            if (e == std::string::npos) e = data.size();
            std::string line = data.substr(pos, e - pos);
            pos = e + 1;
            if (!line.empty() && line.back() == '\r') line.pop_back();
            size_t c = line.find(':');
            if (c == std::string::npos) continue;
            std::string k = line.substr(0, c);
            std::string v = line.substr(c + 1);
            if (k.empty()) continue;
            gopt_[k] = v;
            goptOrder_.push_back(k);
        }
    }
    const char* defs[][2] = {
        { "renderDistance", "12" }, { "simulationDistance", "12" },
        { "maxFps", "120" }, { "fov", "70" },
        { "graphicsMode", "0" }, { "particles", "0" },
        { "renderClouds", "true" }, { "fullscreen", "false" }
    };
    for (auto& d : defs) {
        if (gopt_.find(d[0]) == gopt_.end()) {
            gopt_[d[0]] = d[1];
            goptOrder_.push_back(d[0]);
        }
    }
    pushLog(L"已读取 options.txt（" + std::to_wstring(gopt_.size()) + L" 项）");
    dirty_ = true;
}

void MainWindow::saveGameOptions() {
    std::wstring path = joinPath(gameDir_, L"options.txt");
    std::string out;
    std::set<std::string> written;
    for (auto& k : goptOrder_) {
        auto it = gopt_.find(k);
        if (it == gopt_.end()) continue;
        out += k + ":" + it->second + "\n";
        written.insert(k);
    }
    for (auto& kv : gopt_) {
        if (written.count(kv.first)) continue;
        out += kv.first + ":" + kv.second + "\n";
    }
    FILE* fp = _wfopen(path.c_str(), L"wb");
    if (!fp) {
        MessageBoxW(hwnd_, L"无法写入 options.txt，请检查游戏目录。", L"游戏选项", MB_ICONWARNING);
        return;
    }
    fwrite(out.data(), 1, out.size(), fp);
    fclose(fp);
    pushLog(L"已保存游戏选项: " + path);
    MessageBoxW(hwnd_, L"游戏选项已保存。\n部分选项需要重启游戏生效。", L"游戏选项", MB_ICONINFORMATION);
}

void MainWindow::drawGameOptions(Graphics& g, int W, int H) {
    FontFamily ff(L"Microsoft YaHei");
    Font fL(&ff, 12.5f, FontStyleRegular, UnitPixel);
    Font fV(&ff, 12.0f, FontStyleRegular, UnitPixel);
    Font fB(&ff, 14.0f, FontStyleBold, UnitPixel);

    int cx = SIDEBAR_W + 24;
    int cw = W - SIDEBAR_W - 48;
    int y = HEADER_H + 16;

    RECT hint = mkRect(cx, y, cx + cw, y + 24);
    drawText(g, L"修改的是游戏目录下的 options.txt；建议关闭游戏后再改，部分选项需重启生效。", fL, kTextDim, hint, 0);

    RECT card = mkRect(cx, y + 30, cx + cw, H - 80);
    drawCard(g, card);

    const wchar_t* labels[8] = {
        L"渲染距离", L"模拟距离", L"最大帧率", L"视场角",
        L"图形模式", L"粒子效果", L"云", L"全屏"
    };
    int rowH = 44;
    int top = card.top + 10;
    for (int i = 0; i < 8; ++i) {
        int ry = top + i * rowH;
        RECT rl = mkRect(card.left + 20, ry, card.right - 280, ry + rowH);
        drawText(g, labels[i], fL, kText, rl, 0);
        std::wstring value;
        if (i == 0) value = utf8ToWide(goptInt(gopt_, "renderDistance", 12));
        else if (i == 1) value = utf8ToWide(goptInt(gopt_, "simulationDistance", 12));
        else if (i == 2) value = utf8ToWide(goptInt(gopt_, "maxFps", 120));
        else if (i == 3) value = std::to_wstring(goptFovDegrees(gopt_)) + L"°";
        else if (i == 4) value = (gopt_["graphicsMode"] == "1") ? L"高品质" : L"流畅";
        else if (i == 5) {
            int p = 0; try { p = std::stoi(gopt_["particles"]); } catch (...) {}
            value = (p == 0) ? L"全部" : (p == 1) ? L"减少" : L"最少";
        }
        else if (i == 6) value = (gopt_["renderClouds"] == "true") ? L"开" : L"关";
        else value = (gopt_["fullscreen"] == "true") ? L"开" : L"关";

        RECT rv = mkRect(card.right - 270, ry, card.right - 190, ry + rowH);
        drawText(g, value, fB, kAccent, rv, 2);
        if (i < 4) {
            RECT bd = mkRect(card.right - 180, ry + 7, card.right - 140, ry + 37);
            drawButton(g, bd, L"−", BTN_GOPT_DEC_BASE + i, false, true, 12);
            RECT bi = mkRect(card.right - 132, ry + 7, card.right - 92, ry + 37);
            drawButton(g, bi, L"+", BTN_GOPT_INC_BASE + i, false, true, 12);
        } else {
            RECT bt = mkRect(card.right - 180, ry + 7, card.right - 92, ry + 37);
            drawButton(g, bt, L"切换", BTN_GOPT_TOGGLE_BASE + i, false, true, 12);
        }
    }

    RECT bs = mkRect(cx + 16, H - 64, cx + 166, H - 20);
    drawButton(g, bs, L"保存游戏选项", BTN_GOPT_SAVE, true, true, 13);
    RECT bh = mkRect(cx + 180, H - 64, cx + cw, H - 20);
    drawText(g, L"提示：保存后会重写 options.txt，未知选项会原样保留。", fV, kTextDim, bh, 0);
}
void MainWindow::drawSettings(Graphics& g, int W, int H) {
    FontFamily ff(L"Microsoft YaHei");
    Font fL(&ff, 12.5f, FontStyleRegular, UnitPixel);
    Font fV(&ff, 12.0f, FontStyleRegular, UnitPixel);

    int cx = SIDEBAR_W + 24;
    int cw = W - SIDEBAR_W - 48;
    int y = HEADER_H + 16;

    // 卡片1: 目录
    RECT c1 = mkRect(cx, y, cx + cw, y + 64);
    drawCard(g, c1);
    RECT l1 = mkRect(c1.left + 20, c1.top, c1.left + 92, c1.bottom);
    drawText(g, L"游戏目录", fL, kTextDim, l1, 0);
    RECT b1 = mkRect(c1.right - 108, c1.top + 17, c1.right - 16, c1.bottom - 17);
    drawButton(g, b1, L"浏览", BTN_BROWSE_DIR, false, true, 12);
    {
        bool hover = (hoverId_ == BTN_ISOLATION);
        RECT sq = mkRect(c1.right - 292, c1.top + 23, c1.right - 274, c1.top + 41);
        fillRound(g, sq, 4, isolation_ ? kAccent : kWhite);
        strokeRound(g, sq, 4, isolation_ ? kAccent : (hover ? C(160, 165, 172) : C(190, 194, 200)));
        if (isolation_) {
            Pen p(kWhite, 2.0f);
            g.DrawLine(&p, (float)(sq.left + 4), (float)(sq.top + 9), (float)(sq.left + 8), (float)(sq.top + 13));
            g.DrawLine(&p, (float)(sq.left + 8), (float)(sq.top + 13), (float)(sq.left + 14), (float)(sq.top + 5));
        }
        RECT tr = mkRect(sq.right + 8, c1.top, c1.right - 120, c1.bottom);
        drawText(g, L"版本隔离", fL, kText, tr, 0);
        hits_.push_back({ mkRect(c1.right - 300, c1.top, c1.right - 110, c1.bottom), BTN_ISOLATION });
    }

    // 卡片2: Java
    int y2 = y + 64 + 12;
    RECT c2 = mkRect(cx, y2, cx + cw, y2 + 64);
    drawCard(g, c2);
    RECT l2 = mkRect(c2.left + 20, c2.top, c2.left + 92, c2.bottom);
    drawText(g, L"Java 路径", fL, kTextDim, l2, 0);
    RECT b2a = mkRect(c2.right - 108, c2.top + 17, c2.right - 16, c2.bottom - 17);
    drawButton(g, b2a, L"浏览", BTN_BROWSE_JAVA, false, true, 12);
    RECT b2b = mkRect(c2.right - 200, c2.top + 17, c2.right - 118, c2.bottom - 17);
    drawButton(g, b2b, L"扫描", BTN_JAVA_SCAN, false, !javaScanning_, 12);
    RECT b2c = mkRect(c2.right - 300, c2.top + 17, c2.right - 210, c2.bottom - 17);
    drawButton(g, b2c, L"自动检测", BTN_AUTO_JAVA, false, true, 12);

    // 卡片3: 内存 + 下载源
    int y3 = y2 + 64 + 12;
    RECT c3 = mkRect(cx, y3, cx + cw, y3 + 64);
    drawCard(g, c3);
    RECT l3 = mkRect(c3.left + 20, c3.top, c3.left + 92, c3.bottom);
    drawText(g, L"最大内存", fL, kTextDim, l3, 0);
    RECT u3 = mkRect(c3.left + 232, c3.top, c3.left + 292, c3.bottom);
    drawText(g, L"MB", fV, kTextDim, u3, 0);

    // 卡片4: 下载源
    int y4 = y3 + 64 + 12;
    RECT c4 = mkRect(cx, y4, cx + cw, y4 + 64);
    drawCard(g, c4);
    RECT l4 = mkRect(c4.left + 20, c4.top, c4.left + 92, c4.bottom);
    drawText(g, L"下载源", fL, kTextDim, l4, 0);

    auto radio = [&](const RECT& rc, const std::wstring& text, bool checked, int id) {
        bool hover = (hoverId_ == id);
        Color ring = checked ? kAccent : (hover ? C(160, 165, 172) : C(190, 194, 200));
        Pen p(ring, 1.6f);
        g.DrawEllipse(&p, rc.left, rc.top + 9, 14, 14);
        if (checked) {
            SolidBrush b(kAccent);
            g.FillEllipse(&b, rc.left + 4, rc.top + 13, 6, 6);
        }
        RECT tr = mkRect(rc.left + 22, rc.top, rc.right, rc.bottom);
        drawText(g, text, fV, kText, tr, 0);
        hits_.push_back({ rc, id });
    };
    radio(mkRect(c4.left + 100, c4.top + 4, c4.left + 300, c4.bottom), L"BMCLAPI 国内镜像（推荐）", useMirror_, BTN_MIRROR_BMCL);
    radio(mkRect(c4.left + 320, c4.top + 4, c4.left + 520, c4.bottom), L"Mojang 官方源", !useMirror_, BTN_MIRROR_OFFICIAL);
    {
        bool hover = (hoverId_ == BTN_AUTO_BACKUP);
        RECT sq = mkRect(c4.left + 540, c4.top + 14, c4.left + 558, c4.top + 32);
        fillRound(g, sq, 4, autoBackup_ ? kAccent : kWhite);
        strokeRound(g, sq, 4, autoBackup_ ? kAccent : (hover ? C(160, 165, 172) : C(190, 194, 200)));
        if (autoBackup_) {
            Pen p(kWhite, 2.0f);
            g.DrawLine(&p, (float)(sq.left + 4), (float)(sq.top + 10), (float)(sq.left + 8), (float)(sq.top + 14));
            g.DrawLine(&p, (float)(sq.left + 8), (float)(sq.top + 14), (float)(sq.left + 14), (float)(sq.top + 6));
        }
        RECT tr = mkRect(sq.right + 8, c4.top, c4.left + 750, c4.bottom);
        drawText(g, L"自动备份存档", fV, kText, tr, 0);
        hits_.push_back({ mkRect(c4.left + 540, c4.top + 4, c4.left + 750, c4.bottom), BTN_AUTO_BACKUP });
    }

    // 保存
    int by = y4 + 64 + 16;
    RECT bs = mkRect(cx, by, cx + 150, by + 44);
    drawButton(g, bs, L"保存设置", BTN_SAVE, true, true, 13);

    RECT hr = mkRect(cx + 170, by, cx + cw, by + 44);
    drawText(g, L"提示：修改后需点击「保存设置」生效。", fV, kTextDim, hr, 0);

    int jy = by + 44 + 12;
    RECT c5 = mkRect(cx, jy, cx + cw, H - 16);
    drawCard(g, c5);
    RECT jt = mkRect(c5.left + 20, c5.top + 8, c5.right - 130, c5.top + 30);
    drawText(g, javaScanning_ ? L"已检测到的 Java · 正在扫描…" : L"已检测到的 Java", fL, kTextDim, jt, 0);
    RECT jb = mkRect(c5.right - 116, c5.top + 8, c5.right - 16, c5.top + 36);
    drawButton(g, jb, L"扫描 Java", BTN_JAVA_SCAN, false, !javaScanning_, 12);

    std::vector<JavaEntry> jv;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        jv = javaFound_;
    }
    if (jv.empty()) {
        RECT je = mkRect(c5.left + 20, c5.top + 44, c5.right - 20, c5.top + 76);
        drawText(g, javaScanning_ ? L"正在查找常见 Java 安装位置…" : L"点「扫描 Java」查找本机已安装的 Java；也可以手动填写路径。",
                 fV, kTextDim, je, 0);
    } else {
        int rowH = 34;
        int maxRows = (c5.bottom - c5.top - 44) / rowH;
        if (maxRows < 1) maxRows = 1;
        int show = (int)jv.size();
        if (show > maxRows) show = maxRows;
        int ry = c5.top + 42;
        for (int i = 0; i < show; ++i) {
            const JavaEntry& e = jv[i];
            RECT rp = mkRect(c5.left + 20, ry, c5.right - 220, ry + rowH);
            drawText(g, e.path, fV, kText, rp, 0);
            RECT rm = mkRect(c5.right - 210, ry, c5.right - 90, ry + rowH);
            drawText(g, L"Java " + std::to_wstring(e.major), fV, kAccent, rm, 0);
            RECT rb = mkRect(c5.right - 80, ry + 3, c5.right - 16, ry + 29);
            drawButton(g, rb, L"使用", BTN_JAVA_USE_BASE + i, false, true, 11);
            ry += rowH;
        }
    }
}

// ------------------------------ 关于页 -------------------------------------
void MainWindow::downloadLatestAsync() {
    if (dlActive_.load()) { pushLog(L"已有下载任务在进行。"); return; }
    dlActive_ = true;
    dlPct_ = 0;
    dirty_ = true;
    std::thread([this]() {
        std::wstring err;
        std::wstring ver = L"latest";
        std::string body;
        if (httpGetToStringEx(L"http://192.168.3.173:7891/api/status", kBrowserUA, body, err)) {
            Json j;
            if (mjson::parse(body, j) && j.isObject()) {
                std::string v = getStr(j, "version");
                if (!v.empty()) ver = utf8ToWide(v);
            }
        }
        std::wstring dir = joinPath(exeDir(), L"downloads");
        createDirs(dir);
        std::wstring file = joinPath(dir, L"MCLauncher_v" + ver + L".exe");
        bool ok = httpDownloadToFileEx(L"http://192.168.3.173:7891/download/latest", file, kBrowserUA,
            [this](unsigned long long done, unsigned long long total) {
                if (total > 0) dlPct_ = (int)(done * 100 / total);
                return true;
            }, err);
        dlActive_ = false;
        if (ok) {
            pushLog(L"已下载最新版: " + file);
            int choice = MessageBoxW(hwnd_,
                (L"最新版已下载：\n" + file + L"\n\n是否立即用新版本替换当前启动器并重启？").c_str(),
                L"MCL 更新", MB_YESNO | MB_ICONQUESTION);
            if (choice == IDYES) {
                wchar_t exePath[MAX_PATH] = { 0 };
                GetModuleFileNameW(nullptr, exePath, MAX_PATH);
                wchar_t tmp[MAX_PATH] = { 0 };
                GetTempPathW(MAX_PATH, tmp);
                std::wstring bat = joinPath(tmp, L"mcl_self_update.bat");
                std::string script;
                script += "@echo off\r\n";
                script += "chcp 65001 >nul\r\n";
                script += "timeout /t 2 /nobreak >nul\r\n";
                script += "copy /y \"" + wideToUtf8(file) + "\" \"" + wideToUtf8(std::wstring(exePath)) + "\" >nul\r\n";
                script += "start \"\" \"" + wideToUtf8(std::wstring(exePath)) + "\"\r\n";
                script += "del \"%~f0\"\r\n";
                FILE* fp = _wfopen(bat.c_str(), L"wb");
                if (fp) { fwrite(script.data(), 1, script.size(), fp); fclose(fp); }
                if (fp && ShellExecuteW(nullptr, L"open", bat.c_str(), nullptr, nullptr, SW_HIDE) > (HINSTANCE)32) {
                    pushLog(L"正在替换启动器并重启…");
                    PostMessageW(hwnd_, WM_CLOSE, 0, 0);
                    return;
                }
                MessageBoxW(hwnd_, L"无法启动更新脚本，已保留下载文件。", L"MCL 更新", MB_ICONWARNING);
            }
            ShellExecuteW(nullptr, L"open", dir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        } else {
            pushLog(L"下载最新版失败: " + err);
            MessageBoxW(hwnd_, (L"下载失败：" + err).c_str(), L"MCL 下载", MB_ICONWARNING);
        }
        dirty_ = true;
        if (hwnd_) PostMessageW(hwnd_, WM_TIMER, 0, 0);
    }).detach();
}
void MainWindow::drawAbout(Graphics& g, int W, int H) {
    FontFamily ff(L"Microsoft YaHei");
    Font fL(&ff, 13.0f, FontStyleRegular, UnitPixel);
    Font fS(&ff, 12.0f, FontStyleRegular, UnitPixel);
    Font fB(&ff, 16.0f, FontStyleBold, UnitPixel);

    int cx = SIDEBAR_W + 24;
    int cw = W - SIDEBAR_W - 48;
    int y = HEADER_H + 16;

    RECT c = mkRect(cx, y, cx + cw, y + 400);
    drawCard(g, c);

    RECT r1 = mkRect(c.left + 24, c.top + 20, c.right - 24, c.top + 52);
    drawText(g, std::wstring(L"MCL 启动器  v") + kAppVersion, fB, kText, r1, 0);

    const wchar_t* lines[] = {
        L"一个用 C++ 编写的 Minecraft 启动器，界面参考 PCL2。",
        L"",
        L"核心能力：",
        L"  · 从 BMCLAPI 国内镜像 / Mojang 官方源获取版本清单",
        L"  · 一键下载客户端、依赖库、资源文件并解压 natives",
        L"  · 离线模式启动，自动组装 classpath 与 JVM 参数",
        L"  · 支持自定义游戏目录、Java 路径与最大内存",
        L"  · 多线程并行下载，实时进度显示",
        L"",
        L"说明：",
        L"  · MCBBS 已于 2024-01-22 永久关闭，故下载源改用 BMCLAPI 与官方镜像。",
        L"  · 离线模式仅用于本地测试，正版登录请使用官方启动器。",
        L"",
        L"作者名单：",
        L"  · 霍霍电脑的pocket4",
    };
    int ly = c.top + 60;
    for (auto s : lines) {
        RECT r = mkRect(c.left + 24, ly, c.right - 24, ly + 22);
        drawText(g, s, fS, (std::wstring(s).empty() ? kTextDim : kText), r, 0);
        ly += 19;
    }

    RECT b1 = mkRect(c.left + 24, c.top + 350, c.left + 144, c.top + 390);
    drawButton(g, b1, L"访问官网", BTN_OPEN_SITE, true, true, 13);
    RECT b2 = mkRect(c.left + 154, c.top + 350, c.left + 294, c.top + 390);
    drawButton(g, b2, L"提交 BUG 反馈", BTN_OPEN_FEEDBACK, false, true, 13);
    RECT b3 = mkRect(c.left + 304, c.top + 350, c.left + 454, c.top + 390);
    drawButton(g, b3, L"导出诊断信息", BTN_EXPORT_DIAG, false, true, 13);
    RECT b4 = mkRect(c.left + 464, c.top + 350, c.left + 604, c.top + 390);
    drawButton(g, b4, L"下载最新版", BTN_DOWNLOAD_LATEST, false, !dlActive_.load(), 13);
}

static bool newestFileInDir(const std::wstring& dir, const std::wstring& filter, std::wstring& out) {
    WIN32_FIND_DATAW fd;
    std::wstring pattern = dir + L"\\" + filter;
    HANDLE h = FindFirstFileW(pattern.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return false;
    bool found = false;
    FILETIME best{};
    do {
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
        if (!found || CompareFileTime(&fd.ftLastWriteTime, &best) > 0) {
            best = fd.ftLastWriteTime;
            out = dir + L"\\" + fd.cFileName;
            found = true;
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    return found;
}

void MainWindow::exportDiagnostics() {
    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t ts[64];
    swprintf(ts, 64, L"%04d%02d%02d_%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);

    OSVERSIONINFOW os{};
    os.dwOSVersionInfoSize = sizeof(os);
    {
        typedef LONG (WINAPI* RtlGetVersionFn)(OSVERSIONINFOW*);
        HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
        RtlGetVersionFn fn = ntdll ? (RtlGetVersionFn)GetProcAddress(ntdll, "RtlGetVersion") : nullptr;
        if (fn) fn(&os); else GetVersionExW(&os);
    }
    SYSTEM_INFO si{};
    GetNativeSystemInfo(&si);
    const wchar_t* arch = (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) ? L"x64" :
                          (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_ARM64) ? L"ARM64" : L"x86";

    int javaMajor = 0;
    if (!javaPath_.empty()) javaMajor = Launcher::queryJavaMajor(javaPath_);
    std::vector<std::string> installed = launcher_.listInstalled();

    auto countFiles = [](const std::wstring& dir, const wchar_t* filter) {
        int n = 0;
        WIN32_FIND_DATAW fd;
        HANDLE h = FindFirstFileW((dir + L"\\" + filter).c_str(), &fd);
        if (h != INVALID_HANDLE_VALUE) {
            do { if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) n++; } while (FindNextFileW(h, &fd));
            FindClose(h);
        }
        return n;
    };
    auto countDirs = [](const std::wstring& dir) {
        int n = 0;
        WIN32_FIND_DATAW fd;
        HANDLE h = FindFirstFileW((dir + L"\\*").c_str(), &fd);
        if (h != INVALID_HANDLE_VALUE) {
            do {
                if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
                if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
                n++;
            } while (FindNextFileW(h, &fd));
            FindClose(h);
        }
        return n;
    };

    std::wstring logsDir = joinPath(activeGameDir(), L"logs");
    std::wstring crashDir = joinPath(activeGameDir(), L"crash-reports");
    std::wstring latestLog, latestCrash;
    newestFileInDir(logsDir, L"latest.log", latestLog);
    if (!newestFileInDir(crashDir, L"crash-*.txt", latestCrash)) newestFileInDir(crashDir, L"*.txt", latestCrash);

    std::wstring t;
    t += L"MCL 启动器诊断信息\r\n";
    t += L"生成时间: " + std::wstring(ts) + L"\r\n";
    t += L"启动器版本: v" + std::wstring(kAppVersion) + L"\r\n";
    t += L"系统: Windows " + std::to_wstring(os.dwMajorVersion) + L"." + std::to_wstring(os.dwMinorVersion) +
         L" (build " + std::to_wstring(os.dwBuildNumber) + L", " + arch + L")\r\n";
    t += L"游戏目录: " + gameDir_ + L"\r\n";
    t += L"Java: " + (javaPath_.empty() ? L"(未检测到)" : javaPath_) +
         L" (主版本 " + std::to_wstring(javaMajor) + L")\r\n";
    t += L"玩家昵称: " + playerName_ + L"\r\n";
    t += L"最大内存: " + std::to_wstring(memoryMB_) + L" MB\r\n";
    t += L"当前选择版本: " + (selVersion_.empty() ? L"(未选择)" : selVersion_) + L"\r\n";
    t += L"已安装版本数: " + std::to_wstring(installed.size()) + L"\r\n";
    if (!installed.empty()) {
        std::wstring list;
        for (size_t i = 0; i < installed.size() && i < 30; ++i) {
            if (i) list += L", ";
            list += utf8ToWide(installed[i]);
        }
        t += L"已安装版本: " + list + L"\r\n";
    }
    t += L"模组数: " + std::to_wstring(countFiles(modsDir(), L"*")) + L"\r\n";
    t += L"资源包数: " + std::to_wstring(countFiles(joinPath(activeGameDir(), L"resourcepacks"), L"*")) + L"\r\n";
    t += L"光影包数: " + std::to_wstring(countFiles(joinPath(activeGameDir(), L"shaderpacks"), L"*")) + L"\r\n";
    t += L"存档数: " + std::to_wstring(countDirs(joinPath(activeGameDir(), L"saves"))) + L"\r\n";
    t += L"最新日志: " + (latestLog.empty() ? L"(无)" : latestLog) + L"\r\n";
    t += L"最新崩溃报告: " + (latestCrash.empty() ? L"(无)" : latestCrash) + L"\r\n";
    t += L"发布服务器: http://192.168.3.173:7891\r\n";
    t += L"\r\n提示：把本文件附在官网 BUG反馈 中，可帮助快速定位问题。\r\n";

    std::wstring path = joinPath(exeDir(), L"诊断信息_" + std::wstring(ts) + L".txt");
    FILE* fp = _wfopen(path.c_str(), L"wb");
    if (!fp) {
        MessageBoxW(hwnd_, L"无法写入诊断文件。", L"MCL 诊断", MB_ICONWARNING);
        return;
    }
    const unsigned char bom[3] = { 0xEF, 0xBB, 0xBF };
    fwrite(bom, 1, 3, fp);
    std::string u8 = wideToUtf8(t);
    fwrite(u8.data(), 1, u8.size(), fp);
    fclose(fp);
    pushLog(L"诊断信息已导出: " + path);
    if (MessageBoxW(hwnd_, (L"诊断信息已导出：\n" + path + L"\n\n是否打开所在文件夹？").c_str(),
                    L"MCL 诊断", MB_YESNO | MB_ICONINFORMATION) == IDYES) {
        ShellExecuteW(nullptr, L"open", exeDir().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
}

void MainWindow::analyzeCrash() {
    std::wstring crashDir = joinPath(activeGameDir(), L"crash-reports");
    std::wstring logsDir  = joinPath(activeGameDir(), L"logs");
    std::wstring file;
    bool fromCrash = newestFileInDir(crashDir, L"crash-*.txt", file);
    if (!fromCrash) fromCrash = newestFileInDir(crashDir, L"*.txt", file);
    if (!fromCrash) newestFileInDir(logsDir, L"latest.log", file);
    if (file.empty()) {
        MessageBoxW(hwnd_, L"没有找到崩溃报告（crash-reports）或 latest.log。\n\n先启动一次游戏，崩溃后再回来分析。",
                    L"MCL 崩溃分析", MB_ICONINFORMATION);
        return;
    }

    FILE* fp = _wfopen(file.c_str(), L"rb");
    if (!fp) {
        MessageBoxW(hwnd_, L"无法读取崩溃报告文件。", L"MCL 崩溃分析", MB_ICONWARNING);
        return;
    }
    fseek(fp, 0, SEEK_END);
    long sz = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (sz > 1024 * 1024) sz = 1024 * 1024;
    std::vector<char> buf((size_t)(sz > 0 ? sz : 0));
    size_t got = buf.empty() ? 0 : fread(buf.data(), 1, buf.size(), fp);
    fclose(fp);
    if (got == 0) {
        MessageBoxW(hwnd_, L"崩溃报告是空的。", L"MCL 崩溃分析", MB_ICONWARNING);
        return;
    }

    int wlen = MultiByteToWideChar(CP_UTF8, 0, buf.data(), (int)got, nullptr, 0);
    UINT cp = CP_UTF8;
    if (wlen <= 0) { cp = CP_ACP; wlen = MultiByteToWideChar(CP_ACP, 0, buf.data(), (int)got, nullptr, 0); }
    std::wstring text(wlen > 0 ? wlen : 0, L'\0');
    if (wlen > 0) MultiByteToWideChar(cp, 0, buf.data(), (int)got, &text[0], wlen);

    std::wstring low = text;
    for (auto& c : low) c = (wchar_t)towlower(c);

    std::wstring report;
    report += L"分析文件：\n" + file + L"\n\n";
    report += L"检测到的问题：\n";
    int hits = 0;
    auto has = [&](const wchar_t* p) { return low.find(p) != std::wstring::npos; };
    if (has(L"outofmemoryerror") || has(L"java heap space") || has(L"out of memory")) {
        report += L"· 内存不足（OutOfMemoryError）\n  建议：在首页把「最大内存」调高到 4096–8192 MB，并关闭占用内存的程序。\n";
        hits++;
    }
    if (has(L"unsupportedclassversionerror") || has(L"class file version") || has(L"unsupported major.minor")) {
        report += L"· Java 版本不匹配（UnsupportedClassVersionError）\n  建议：在「游戏设置」里换用与 MC 版本匹配的 Java（1.16 及以下用 Java 8，1.17+ 用 Java 17/21）。\n";
        hits++;
    }
    if (has(L"missing or unsupported mandatory dependencies") || has(L"requires") && has(L"mod file")) {
        report += L"· 模组缺少依赖或版本不匹配\n  建议：检查 mods 文件夹，补齐前置模组；或临时移出最近加入的模组再试。\n";
        hits++;
    }
    if (has(L"duplicate mods") || has(L"duplicate mod")) {
        report += L"· 模组重复\n  建议：删除 mods 中重复的 jar（常见于同时放了 Forge 与 Fabric 版本）。\n";
        hits++;
    }
    if (has(L"noclassdeffounderror") || has(L"nosuchmethoderror") || has(L"incompatible")) {
        report += L"· 模组/加载器版本不兼容（NoSuchMethod / NoClassDefFound）\n  建议：确认所有模组支持当前 MC 版本与加载器版本，优先移除最近新增的模组。\n";
        hits++;
    }
    if (has(L"glfw error") || has(L"opengl") || has(L"pixel format") || has(L"wglcreatecontext")) {
        report += L"· 显卡/OpenGL 初始化失败\n  建议：更新显卡驱动；关闭光影；在启动器里尝试关闭 3D 加速或降低渲染距离。\n";
        hits++;
    }
    if (has(L"the game crashed whilst") || has(L"exception in thread \"main\"")) {
        report += L"· 游戏主线程崩溃\n  建议：把本文件通过官网 BUG反馈 提交，附上完整崩溃报告以便定位。\n";
        hits++;
    }
    if (hits == 0) {
        report += L"· 未识别到常见错误特征\n  建议：把崩溃报告通过官网 BUG反馈 提交，或在「日志」文件夹查看 latest.log。\n";
    }
    report += L"\n是否打开官网提交 BUG 反馈？";

    pushLog(L"崩溃分析完成: " + fileNameOf(file));
    int r = MessageBoxW(hwnd_, report.c_str(), L"MCL 崩溃分析", MB_YESNO | MB_ICONINFORMATION);
    if (r == IDYES) {
        std::wstring u = std::wstring(kServerBase) + L"/?feedback=1";
        ShellExecuteW(nullptr, L"open", u.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    }
}
// ------------------------------ 模组管理页 ---------------------------------
static bool endsWithNoCase(const std::wstring& s, const std::wstring& tail) {
    if (s.size() < tail.size()) return false;
    return _wcsicmp(s.c_str() + (s.size() - tail.size()), tail.c_str()) == 0;
}

void MainWindow::loadInstalledMods() {
    installedMods_.clear();
    std::wstring dir = modsDir();
    if (!pathExists(dir)) createDirs(dir);
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((dir + L"\\*").c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;
            std::wstring fn = fd.cFileName;
            bool disabled = endsWithNoCase(fn, L".jar.disabled");
            bool enabled = !disabled && endsWithNoCase(fn, L".jar");
            if (!disabled && !enabled) continue;
            InstalledMod m;
            m.file = fn;
            m.name = disabled ? fn.substr(0, fn.size() - 9) : fn;
            m.size = ((unsigned long long)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
            m.enabled = enabled;
            installedMods_.push_back(m);
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    std::sort(installedMods_.begin(), installedMods_.end(),
              [](const InstalledMod& a, const InstalledMod& b) { return _wcsicmp(a.name.c_str(), b.name.c_str()) < 0; });
    if (modMgrScroll_ >= (int)installedMods_.size()) modMgrScroll_ = 0;
}

void MainWindow::toggleInstalledMod(int idx) {
    if (idx < 0 || idx >= (int)installedMods_.size()) return;
    const InstalledMod& m = installedMods_[idx];
    std::wstring dir = modsDir();
    std::wstring from = joinPath(dir, m.file);
    std::wstring to;
    if (m.enabled) {
        to = from + L".disabled";
    } else {
        if (!endsWithNoCase(from, L".disabled")) return;
        to = from.substr(0, from.size() - 9);
    }
    if (!MoveFileW(from.c_str(), to.c_str())) {
        MessageBoxW(hwnd_, L"无法重命名模组文件，可能被游戏占用。", L"MCL 模组管理", MB_ICONWARNING);
        return;
    }
    pushLog((m.enabled ? L"已禁用模组: " : L"已启用模组: ") + m.name);
    loadInstalledMods();
    dirty_ = true;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::deleteInstalledMod(int idx) {
    if (idx < 0 || idx >= (int)installedMods_.size()) return;
    InstalledMod m = installedMods_[idx];
    std::wstring msg = L"确定删除模组？\n\n" + m.name;
    if (MessageBoxW(hwnd_, msg.c_str(), L"MCL 模组管理", MB_YESNO | MB_ICONQUESTION) != IDYES) return;
    std::wstring path = joinPath(modsDir(), m.file);
    if (!DeleteFileW(path.c_str())) {
        MessageBoxW(hwnd_, L"删除失败，文件可能被占用。", L"MCL 模组管理", MB_ICONWARNING);
        return;
    }
    pushLog(L"已删除模组: " + m.name);
    loadInstalledMods();
    dirty_ = true;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::setAllMods(bool enable) {
    int changed = 0, failed = 0;
    std::wstring dir = modsDir();
    for (auto& m : installedMods_) {
        if (m.enabled == enable) continue;
        std::wstring from = joinPath(dir, m.file);
        std::wstring to;
        if (enable) {
            if (!endsWithNoCase(from, L".disabled")) continue;
            to = from.substr(0, from.size() - 9);
        } else {
            to = from + L".disabled";
        }
        if (MoveFileW(from.c_str(), to.c_str())) changed++;
        else failed++;
    }
    loadInstalledMods();
    std::wstring msg = (enable ? L"全部启用完成，变更 " : L"全部禁用完成，变更 ") + std::to_wstring(changed) + L" 个";
    if (failed) msg += L"，失败 " + std::to_wstring(failed) + L" 个（可能被占用）";
    pushLog(msg);
    dirty_ = true;
    InvalidateRect(hwnd_, nullptr, FALSE);
}
void MainWindow::drawModMgr(Graphics& g, int W, int H) {
    FontFamily ff(L"Microsoft YaHei");
    Font fL(&ff, 12.0f, FontStyleRegular, UnitPixel);
    Font fB(&ff, 14.0f, FontStyleBold, UnitPixel);
    Font fD(&ff, 11.5f, FontStyleRegular, UnitPixel);
    Font fH(&ff, 11.0f, FontStyleRegular, UnitPixel);

    int cx = SIDEBAR_W + 24;
    int cw = W - SIDEBAR_W - 48;
    int y = HEADER_H + 16;

    RECT info = mkRect(cx, y, cx + cw, y + 96);
    drawCard(g, info);
    RECT t1 = mkRect(info.left + 16, info.top + 8, info.right - 440, info.top + 30);
    drawText(g, L"模组目录", fL, kTextDim, t1, 0);
    RECT t2 = mkRect(info.left + 16, info.top + 28, info.right - 440, info.top + 50);
    drawText(g, modsDir(), fD, kText, t2, 0);
    int en = 0, dis = 0;
    for (auto& m : installedMods_) { if (m.enabled) en++; else dis++; }
    std::wstring cnt = L"共 " + std::to_wstring(installedMods_.size()) + L" 个 · 已启用 " +
                       std::to_wstring(en) + L" · 已禁用 " + std::to_wstring(dis);
    RECT t3 = mkRect(info.left + 16, info.top + 46, info.right - 440, info.top + 62);
    drawText(g, cnt, fD, kTextDim, t3, 0);
    RECT b1 = mkRect(info.right - 420, info.top + 16, info.right - 330, info.top + 46);
    drawButton(g, b1, L"刷新", BTN_MODMGR_REFRESH, false, true, 12);
    RECT b2 = mkRect(info.right - 322, info.top + 16, info.right - 222, info.top + 46);
    drawButton(g, b2, L"全部启用", BTN_MODMGR_ENABLE_ALL, false, true, 12);
    RECT b3 = mkRect(info.right - 214, info.top + 16, info.right - 114, info.top + 46);
    drawButton(g, b3, L"全部禁用", BTN_MODMGR_DISABLE_ALL, false, true, 12);
    RECT b4 = mkRect(info.right - 106, info.top + 16, info.right - 16, info.top + 46);
    drawButton(g, b4, L"打开目录", BTN_MODMGR_OPEN, false, true, 12);
    RECT fl = mkRect(info.left + 16, info.top + 64, info.left + 56, info.top + 92);
    drawText(g, L"筛选", fL, kTextDim, fl, 0);
    RECT fi = mkRect(info.left + 64, info.top + 64, info.right - 440, info.top + 92);
    fillRound(g, fi, 7, kInputBg);

    int listTop = y + 96 + 12;
    int listBottom = H - 16;
    int rowH = 46;
    int headerH = 28;
    int rows = (listBottom - listTop - headerH - 8) / rowH;
    if (rows < 1) rows = 1;
    std::vector<int> vis;
    {
        std::wstring q = modMgrFilter_;
        for (auto& c : q) c = (wchar_t)towlower(c);
        for (int i = 0; i < (int)installedMods_.size(); ++i) {
            std::wstring n = installedMods_[i].name;
            for (auto& c : n) c = (wchar_t)towlower(c);
            if (q.empty() || n.find(q) != std::wstring::npos) vis.push_back(i);
        }
    }
    int total = (int)vis.size();
    int maxScroll = total > rows ? total - rows : 0;
    if (modMgrScroll_ > maxScroll) modMgrScroll_ = maxScroll;
    if (modMgrScroll_ < 0) modMgrScroll_ = 0;

    RECT list = mkRect(cx, listTop, cx + cw, listBottom);
    drawCard(g, list);
    RECT hdr = mkRect(list.left + 12, list.top + 8, list.right - 12, list.top + 8 + headerH);
    drawText(g, L"模组名称", fH, kTextDim, mkRect(hdr.left, hdr.top, hdr.left + 360, hdr.bottom), 0);
    drawText(g, L"大小", fH, kTextDim, mkRect(hdr.right - 300, hdr.top, hdr.right - 220, hdr.bottom), 0);
    drawText(g, L"状态", fH, kTextDim, mkRect(hdr.right - 210, hdr.top, hdr.right - 140, hdr.bottom), 0);
    drawText(g, L"操作", fH, kTextDim, mkRect(hdr.right - 130, hdr.top, hdr.right - 12, hdr.bottom), 0);

    if (total == 0) {
        RECT e = mkRect(list.left + 12, list.top + headerH + 24, list.right - 12, list.top + headerH + 56);
        drawText(g, installedMods_.empty() ? L"mods 文件夹为空。去「模组下载」搜索并下载模组，或打开目录手动放入 .jar。" : L"没有匹配的模组，换个关键词试试。",
                 fL, kTextDim, e, 0);
        return;
    }

    int ry = list.top + 8 + headerH;
    for (int i = 0; i < rows; ++i) {
        int idx = modMgrScroll_ + i;
        if (idx >= total) break;
        int orig = vis[idx];
        const InstalledMod& m = installedMods_[orig];
        RECT row = mkRect(list.left + 8, ry, list.right - 8, ry + rowH - 4);
        if ((idx & 1) == 0) fillRound(g, row, 6, C(250, 251, 253));
        RECT rn = mkRect(row.left + 8, row.top, row.right - 330, row.bottom);
        std::wstring name = m.name;
        if (name.size() > 54) name = name.substr(0, 51) + L"...";
        drawText(g, name, fB, kText, rn, 0);
        RECT rs = mkRect(row.right - 298, row.top, row.right - 220, row.bottom);
        drawText(g, humanSize(m.size), fD, kTextDim, rs, 0);
        RECT rst = mkRect(row.right - 208, row.top, row.right - 136, row.bottom);
        drawText(g, m.enabled ? L"已启用" : L"已禁用", fD, m.enabled ? kAccent : kTextDim, rst, 0);
        RECT bt = mkRect(row.right - 128, row.top + 8, row.right - 66, row.bottom - 8);
        drawButton(g, bt, m.enabled ? L"禁用" : L"启用", BTN_MODMGR_TOGGLE_BASE + orig, false, true, 11);
        RECT bd = mkRect(row.right - 58, row.top + 8, row.right - 6, row.bottom - 8);
        drawButton(g, bd, L"删除", BTN_MODMGR_DEL_BASE + orig, false, true, 11);
        ry += rowH;
    }
    if (maxScroll > 0) {
        RECT barBg = mkRect(list.right - 6, list.top + headerH + 4, list.right - 2, list.bottom - 4);
        fillRound(g, barBg, 2, C(232, 234, 238));
        int trackH = barBg.bottom - barBg.top;
        int barH = trackH * rows / total;
        if (barH < 24) barH = 24;
        int barY = barBg.top + (trackH - barH) * modMgrScroll_ / maxScroll;
        fillRound(g, mkRect(barBg.left, barY, barBg.right, barY + barH), 2, C(198, 202, 210));
    }
}
// ------------------------------ 存档管理页 ---------------------------------
static void dirStats(const std::wstring& dir, unsigned long long& total, FILETIME& newest) {
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((dir + L"\\*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
        std::wstring p = dir + L"\\" + fd.cFileName;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            dirStats(p, total, newest);
        } else {
            total += ((unsigned long long)fd.nFileSizeHigh << 32) | fd.nFileSizeLow;
            if (CompareFileTime(&fd.ftLastWriteTime, &newest) > 0) newest = fd.ftLastWriteTime;
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
}

static std::wstring safeName(const std::wstring& s) {
    std::wstring out = s;
    for (auto& c : out) {
        if (c == L'\\' || c == L'/' || c == L':' || c == L'*' || c == L'?' || c == L'"' ||
            c == L'<' || c == L'>' || c == L'|') c = L'_';
    }
    return out;
}

static std::wstring fileTimeText(const FILETIME& ft) {
    SYSTEMTIME utc{}, local{};
    if (!FileTimeToSystemTime(&ft, &utc)) return L"-";
    SystemTimeToTzSpecificLocalTime(nullptr, &utc, &local);
    wchar_t buf[64];
    swprintf(buf, 64, L"%04d-%02d-%02d", local.wYear, local.wMonth, local.wDay);
    return buf;
}

void MainWindow::loadSaves() {
    saves_.clear();
    std::wstring dir = joinPath(activeGameDir(), L"saves");
    if (!pathExists(dir)) createDirs(dir);
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((dir + L"\\*").c_str(), &fd);
    if (h != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
            if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
            SaveEntry e;
            e.name = fd.cFileName;
            e.path = dir + L"\\" + fd.cFileName;
            e.size = 0;
            e.modified = fd.ftLastWriteTime;
            dirStats(e.path, e.size, e.modified);
            saves_.push_back(e);
        } while (FindNextFileW(h, &fd));
        FindClose(h);
    }
    std::sort(saves_.begin(), saves_.end(),
              [](const SaveEntry& a, const SaveEntry& b) { return _wcsicmp(a.name.c_str(), b.name.c_str()) < 0; });
    if (modMgrScroll_ >= (int)saves_.size()) modMgrScroll_ = 0;
}

void MainWindow::backupSave(int idx) {
    if (idx < 0 || idx >= (int)saves_.size()) return;
    const SaveEntry& e = saves_[idx];
    std::wstring backups = joinPath(activeGameDir(), L"backups");
    createDirs(backups);
    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t stamp[64];
    swprintf(stamp, 64, L"%04d%02d%02d_%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    std::wstring dst = joinPath(backups, L"saves_" + safeName(e.name) + L"_" + stamp);
    std::wstring from = e.path; from.push_back(L'\0'); from.push_back(L'\0');
    std::wstring to = dst; to.push_back(L'\0'); to.push_back(L'\0');
    SHFILEOPSTRUCTW op{};
    op.wFunc = FO_COPY;
    op.pFrom = from.c_str();
    op.pTo = to.c_str();
    op.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
    int rc = SHFileOperationW(&op);
    if (rc != 0 || op.fAnyOperationsAborted) {
        MessageBoxW(hwnd_, (L"备份失败（错误码 " + std::to_wstring(rc) + L"）").c_str(), L"MCL 存档管理", MB_ICONWARNING);
        return;
    }
    pushLog(L"已备份存档: " + e.name + L" -> " + dst);
    MessageBoxW(hwnd_, (L"备份完成：\n" + dst).c_str(), L"MCL 存档管理", MB_ICONINFORMATION);
}

void MainWindow::deleteSave(int idx) {
    if (idx < 0 || idx >= (int)saves_.size()) return;
    SaveEntry e = saves_[idx];
    std::wstring msg = L"确定删除存档？此操作不可撤销。\n\n" + e.name;
    if (MessageBoxW(hwnd_, msg.c_str(), L"MCL 存档管理", MB_YESNO | MB_ICONWARNING) != IDYES) return;
    std::wstring from = e.path; from.push_back(L'\0'); from.push_back(L'\0');
    SHFILEOPSTRUCTW op{};
    op.wFunc = FO_DELETE;
    op.pFrom = from.c_str();
    op.fFlags = FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
    int rc = SHFileOperationW(&op);
    if (rc != 0 || op.fAnyOperationsAborted) {
        MessageBoxW(hwnd_, (L"删除失败（错误码 " + std::to_wstring(rc) + L"）").c_str(), L"MCL 存档管理", MB_ICONWARNING);
        return;
    }
    pushLog(L"已删除存档: " + e.name);
    loadSaves();
    dirty_ = true;
    InvalidateRect(hwnd_, nullptr, FALSE);
}

static bool copyTreeOverwrite(const std::wstring& src, const std::wstring& dst) {
    CreateDirectoryW(dst.c_str(), nullptr);
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((src + L"\\*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return false;
    bool ok = true;
    do {
        if (wcscmp(fd.cFileName, L".") == 0 || wcscmp(fd.cFileName, L"..") == 0) continue;
        std::wstring s = src + L"\\" + fd.cFileName;
        std::wstring d = dst + L"\\" + fd.cFileName;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!copyTreeOverwrite(s, d)) ok = false;
        } else {
            if (!CopyFileW(s.c_str(), d.c_str(), FALSE)) ok = false;
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    return ok;
}

void MainWindow::restoreLatestBackup() {
    std::wstring backups = joinPath(activeGameDir(), L"backups");
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW((backups + L"\\saves_*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) {
        MessageBoxW(hwnd_, L"还没有存档备份。", L"MCL 存档管理", MB_ICONINFORMATION);
        return;
    }
    std::wstring newest;
    FILETIME best{};
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) continue;
        if (CompareFileTime(&fd.ftLastWriteTime, &best) > 0) {
            best = fd.ftLastWriteTime;
            newest = fd.cFileName;
        }
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    if (newest.empty()) {
        MessageBoxW(hwnd_, L"还没有存档备份。", L"MCL 存档管理", MB_ICONINFORMATION);
        return;
    }
    std::wstring base = newest.substr(6);
    if (base.size() > 16 && base[base.size() - 16] == L'_') base = base.substr(0, base.size() - 16);
    if (base.empty()) {
        MessageBoxW(hwnd_, L"备份目录名无法解析。", L"MCL 存档管理", MB_ICONWARNING);
        return;
    }
    std::wstring backupPath = joinPath(backups, newest);
    std::wstring savesDir = joinPath(activeGameDir(), L"saves");
    std::wstring dest = joinPath(savesDir, base);
    std::wstring msg = L"将用备份恢复存档：\n" + base + L"\n\n当前存档会先改名为 " + base + L".before_restore_<时间>。\n继续吗？";
    if (MessageBoxW(hwnd_, msg.c_str(), L"恢复存档", MB_ICONWARNING | MB_YESNO) != IDYES) return;

    SYSTEMTIME st{};
    GetLocalTime(&st);
    wchar_t stamp[64];
    swprintf(stamp, 64, L"%04d%02d%02d_%02d%02d%02d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    bool had = pathExists(dest);
    std::wstring old = dest + L".before_restore_" + std::wstring(stamp);
    if (had && !MoveFileW(dest.c_str(), old.c_str())) {
        MessageBoxW(hwnd_, L"当前存档正在被占用，无法重命名。", L"MCL 存档管理", MB_ICONWARNING);
        return;
    }
    createDirs(dest);
    if (!copyTreeOverwrite(backupPath, dest)) {
        if (had) MoveFileW(old.c_str(), dest.c_str());
        MessageBoxW(hwnd_, L"恢复失败，已尝试还原原存档。", L"MCL 存档管理", MB_ICONWARNING);
        return;
    }
    loadSaves();
    pushLog(L"已从备份恢复存档: " + base + L" <- " + backupPath);
    MessageBoxW(hwnd_, (L"恢复完成：\n" + base).c_str(), L"MCL 存档管理", MB_ICONINFORMATION);
}
void MainWindow::drawSaves(Graphics& g, int W, int H) {
    FontFamily ff(L"Microsoft YaHei");
    Font fL(&ff, 12.0f, FontStyleRegular, UnitPixel);
    Font fB(&ff, 14.0f, FontStyleBold, UnitPixel);
    Font fD(&ff, 11.5f, FontStyleRegular, UnitPixel);
    Font fH(&ff, 11.0f, FontStyleRegular, UnitPixel);

    int cx = SIDEBAR_W + 24;
    int cw = W - SIDEBAR_W - 48;
    int y = HEADER_H + 16;

    RECT info = mkRect(cx, y, cx + cw, y + 64);
    drawCard(g, info);
    RECT t1 = mkRect(info.left + 16, info.top + 8, info.right - 440, info.top + 30);
    drawText(g, L"存档目录", fL, kTextDim, t1, 0);
    RECT t2 = mkRect(info.left + 16, info.top + 28, info.right - 440, info.top + 50);
    drawText(g, joinPath(activeGameDir(), L"saves"), fD, kText, t2, 0);
    unsigned long long total = 0;
    for (auto& e : saves_) total += e.size;
    std::wstring cnt = L"共 " + std::to_wstring(saves_.size()) + L" 个存档 · 合计 " + humanSize(total);
    RECT t3 = mkRect(info.left + 16, info.top + 46, info.right - 440, info.top + 62);
    drawText(g, cnt, fD, kTextDim, t3, 0);
    RECT b1 = mkRect(info.right - 420, info.top + 16, info.right - 330, info.top + 46);
    drawButton(g, b1, L"刷新", BTN_MODMGR_REFRESH, false, true, 12);
    RECT b3 = mkRect(info.right - 322, info.top + 16, info.right - 172, info.top + 46);
    drawButton(g, b3, L"恢复最近备份", BTN_SAVE_RESTORE, false, true, 12);
    RECT b2 = mkRect(info.right - 164, info.top + 16, info.right - 16, info.top + 46);
    drawButton(g, b2, L"打开 backups", BTN_MODMGR_OPEN, false, true, 12);

    int listTop = y + 64 + 12;
    int listBottom = H - 16;
    int rowH = 46;
    int headerH = 28;
    int rows = (listBottom - listTop - headerH - 8) / rowH;
    if (rows < 1) rows = 1;
    int totalCount = (int)saves_.size();
    int maxScroll = totalCount > rows ? totalCount - rows : 0;
    if (modMgrScroll_ > maxScroll) modMgrScroll_ = maxScroll;
    if (modMgrScroll_ < 0) modMgrScroll_ = 0;

    RECT list = mkRect(cx, listTop, cx + cw, listBottom);
    drawCard(g, list);
    RECT hdr = mkRect(list.left + 12, list.top + 8, list.right - 12, list.top + 8 + headerH);
    drawText(g, L"存档名称", fH, kTextDim, mkRect(hdr.left, hdr.top, hdr.left + 300, hdr.bottom), 0);
    drawText(g, L"大小", fH, kTextDim, mkRect(hdr.right - 300, hdr.top, hdr.right - 220, hdr.bottom), 0);
    drawText(g, L"最后修改", fH, kTextDim, mkRect(hdr.right - 210, hdr.top, hdr.right - 136, hdr.bottom), 0);
    drawText(g, L"操作", fH, kTextDim, mkRect(hdr.right - 128, hdr.top, hdr.right - 12, hdr.bottom), 0);

    if (totalCount == 0) {
        RECT e = mkRect(list.left + 12, list.top + headerH + 24, list.right - 12, list.top + headerH + 56);
        drawText(g, L"saves 文件夹为空。进入游戏创建一个世界后，这里会出现存档。", fL, kTextDim, e, 0);
        return;
    }

    int ry = list.top + 8 + headerH;
    for (int i = 0; i < rows; ++i) {
        int idx = modMgrScroll_ + i;
        if (idx >= totalCount) break;
        const SaveEntry& e = saves_[idx];
        RECT row = mkRect(list.left + 8, ry, list.right - 8, ry + rowH - 4);
        if ((idx & 1) == 0) fillRound(g, row, 6, C(250, 251, 253));
        RECT rn = mkRect(row.left + 8, row.top, row.right - 350, row.bottom);
        std::wstring name = e.name;
        if (name.size() > 44) name = name.substr(0, 41) + L"...";
        drawText(g, name, fB, kText, rn, 0);
        RECT rs = mkRect(row.right - 298, row.top, row.right - 220, row.bottom);
        drawText(g, humanSize(e.size), fD, kTextDim, rs, 0);
        RECT rt = mkRect(row.right - 210, row.top, row.right - 136, row.bottom);
        drawText(g, fileTimeText(e.modified), fD, kTextDim, rt, 0);
        RECT bd = mkRect(row.right - 132, row.top + 8, row.right - 92, row.bottom - 8);
        drawButton(g, bd, L"删除", BTN_SAVE_DEL_BASE + idx, false, true, 11);
        RECT bb = mkRect(row.right - 88, row.top + 8, row.right - 48, row.bottom - 8);
        drawButton(g, bb, L"备份", BTN_SAVE_BACKUP_BASE + idx, false, true, 11);
        RECT bo = mkRect(row.right - 44, row.top + 8, row.right - 4, row.bottom - 8);
        drawButton(g, bo, L"打开", BTN_SAVE_OPEN_BASE + idx, false, true, 11);
        ry += rowH;
    }
}
// ------------------------------ 资源管理页 ---------------------------------
struct ResEntry {
    const wchar_t* dir;
    const wchar_t* title;
    const wchar_t* desc;
    const wchar_t* filter;
};

static const ResEntry kResEntries[] = {
    { L"resourcepacks", L"资源包",   L"材质、音效与界面资源",       L"*" },
    { L"shaderpacks",   L"光影包",   L"Shaders 光影与渲染配置",     L"*" },
    { L"saves",         L"存档",     L"单人世界与地图",             L"*" },
    { L"screenshots",   L"截图",     L"游戏内截图",                 L"*" },
    { L"mods",          L"模组",     L"Forge / Fabric 模组",        L"*" },
    { L"logs",          L"日志",     L"latest.log 与调试日志",      L"*" },
    { L"crash-reports", L"崩溃报告", L"crash-*.txt 崩溃记录",       L"*.txt" },
    { L"versions",      L"版本",     L"已安装的游戏版本",           L"*" },
};

static int countDirEntries(const std::wstring& dir, const wchar_t* filter) {
    int n = 0;
    WIN32_FIND_DATAW fd;
    std::wstring pattern = dir + L"\\" + (filter ? filter : L"*");
    HANDLE h = FindFirstFileW(pattern.c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    do {
        if (wcscmp(fd.cFileName, L".") != 0 && wcscmp(fd.cFileName, L"..") != 0) n++;
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    return n;
}

void MainWindow::searchResDlAsync() {
    if (resDlLoading_) return;
    wchar_t buf[256] = { 0 };
    if (editModSearch_) GetWindowTextW(editModSearch_, buf, 256);
    std::wstring kw = trimW(std::wstring(buf));
    if (kw.empty()) {
        MessageBoxW(hwnd_, L"请先输入关键词，例如 faithful、shader。", L"资源下载", MB_ICONINFORMATION);
        return;
    }
    int type = resDlType_;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        resDlLoading_ = true;
        resDlItems_.clear();
        resDlErr_.clear();
    }
    modMgrScroll_ = 0;
    dirty_ = true;
    std::string kw8 = wideToUtf8(kw);
    std::thread([this, kw8, type]() {
        apiThrottle(300);
        std::string facets = (type == 0) ? "[[\"project_type:resourcepack\"]]" : "[[\"project_type:shader\"]]";
        std::wstring url = std::wstring(kModrinthApi) + L"search?query=" + utf8ToWide(urlEncode(kw8)) +
                           L"&limit=20&index=downloads&facets=" + utf8ToWide(urlEncodeAll(facets));
        std::string body;
        std::wstring err;
        bool ok = httpGetToStringEx(url, kBrowserUA, body, err);
        std::vector<ResDlItem> items;
        std::wstring fail;
        if (!ok) {
            fail = L"Modrinth 搜索失败：" + err;
        } else {
            Json j;
            if (!mjson::parse(body, j) || !j.isObject()) {
                fail = L"Modrinth 返回内容解析失败";
            } else {
                const Json& hits = j.get("hits");
                if (!hits.isArray()) fail = L"Modrinth 返回结果格式异常";
                else {
                    for (size_t i = 0; i < hits.size() && items.size() < 20; ++i) {
                        const Json& h = hits.at(i);
                        ResDlItem it;
                        it.id = getStr(h, "project_id");
                        if (it.id.empty()) continue;
                        it.title = utf8ToWide(getStr(h, "title"));
                        it.author = utf8ToWide(getStr(h, "author"));
                        it.desc = oneLine(utf8ToWide(getStr(h, "description")), 140);
                        it.downloads = (int)h.get("downloads").asNumber(0);
                        items.push_back(it);
                    }
                    if (items.empty()) fail = L"没有找到匹配的资源";
                }
            }
        }
        {
            std::lock_guard<std::mutex> lk(mtx_);
            resDlItems_.swap(items);
            resDlErr_ = fail;
            resDlLoading_ = false;
        }
        dirty_ = true;
        if (hwnd_) PostMessageW(hwnd_, WM_TIMER, 0, 0);
    }).detach();
}

void MainWindow::downloadResLatest(int idx) {
    if (dlActive_.load()) { pushLog(L"已有下载任务在进行。"); return; }
    std::vector<ResDlItem> items;
    int type = resDlType_;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        items = resDlItems_;
    }
    if (idx < 0 || idx >= (int)items.size()) return;
    ResDlItem item = items[idx];
    std::wstring targetDir = joinPath(activeGameDir(), type == 0 ? L"resourcepacks" : L"shaderpacks");
    createDirs(targetDir);
    dlActive_ = true;
    dlPct_ = 0;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        dlName_ = item.title;
        dlResult_ = L"正在获取最新版本…";
    }
    dirty_ = true;
    std::string id = item.id;
    std::wstring title = item.title;
    std::thread([this, id, title, targetDir]() {
        apiThrottle(300);
        std::wstring url = std::wstring(kModrinthApi) + L"project/" + utf8ToWide(id) + L"/version?limit=1";
        std::string body;
        std::wstring err;
        bool ok = httpGetToStringEx(url, kBrowserUA, body, err);
        std::wstring fileUrl, fileName;
        unsigned long long fileSize = 0;
        if (ok) {
            Json j;
            if (mjson::parse(body, j) && j.isArray() && j.size() > 0) {
                const Json& fs = j.at(0).get("files");
                if (fs.isArray() && fs.size() > 0) {
                    int pick = 0;
                    for (size_t k = 0; k < fs.size(); ++k) {
                        if (fs.at(k).get("primary").asBool(false)) { pick = (int)k; break; }
                    }
                    fileName = utf8ToWide(getStr(fs.at(pick), "filename"));
                    fileUrl = utf8ToWide(getStr(fs.at(pick), "url"));
                    fileSize = (unsigned long long)fs.at(pick).get("size").asNumber(0);
                }
            }
        }
        if (fileUrl.empty()) {
            {
                std::lock_guard<std::mutex> lk(mtx_);
                dlResult_ = L"获取文件失败：" + (err.empty() ? L"没有可用文件" : err);
            }
            dlActive_ = false;
            dirty_ = true;
            if (hwnd_) PostMessageW(hwnd_, WM_TIMER, 0, 0);
            return;
        }
        std::wstring file = joinPath(targetDir, safeName(fileName.empty() ? L"download.bin" : fileName));
        {
            std::lock_guard<std::mutex> lk(mtx_);
            dlResult_ = L"正在下载 " + fileName + L"（" + humanSize(fileSize) + L"）…";
        }
        bool dlOk = httpDownloadToFileEx(fileUrl, file, kBrowserUA,
            [this](unsigned long long done, unsigned long long total) {
                if (total > 0) dlPct_ = (int)(done * 100 / total);
                return true;
            }, err);
        dlPct_ = dlOk ? 100 : dlPct_.load();
        {
            std::lock_guard<std::mutex> lk(mtx_);
            dlResult_ = dlOk ? (L"已下载到 " + targetDir + L"：" + fileName) : (L"下载失败：" + err);
        }
        if (dlOk) pushLog(L"资源下载完成: " + title + L" -> " + file);
        dlActive_ = false;
        dirty_ = true;
        if (hwnd_) PostMessageW(hwnd_, WM_TIMER, 0, 0);
    }).detach();
}

void MainWindow::drawResDl(Graphics& g, int W, int H) {
    FontFamily ff(L"Microsoft YaHei");
    Font fL(&ff, 12.0f, FontStyleRegular, UnitPixel);
    Font fB(&ff, 14.0f, FontStyleBold, UnitPixel);
    Font fD(&ff, 11.5f, FontStyleRegular, UnitPixel);
    Font fH(&ff, 11.0f, FontStyleRegular, UnitPixel);

    int cx = SIDEBAR_W + 24;
    int cw = W - SIDEBAR_W - 48;
    int y = HEADER_H + 16;

    RECT sc = mkRect(cx, y, cx + cw, y + 64);
    drawCard(g, sc);
    RECT lab = mkRect(sc.left + 16, sc.top + 12, sc.left + 90, sc.top + 42);
    drawText(g, L"关键词", fL, kTextDim, lab, 0);
    RECT ib = mkRect(sc.left + 96, sc.top + 12, sc.right - 520, sc.top + 42);
    fillRound(g, ib, 7, kInputBg);
    RECT bp = mkRect(sc.right - 400, sc.top + 16, sc.right - 310, sc.top + 46);
    drawButton(g, bp, L"资源包", BTN_RESDL_TYPE_PACK, resDlType_ == 0, true, 12);
    RECT bs = mkRect(sc.right - 302, sc.top + 16, sc.right - 212, sc.top + 46);
    drawButton(g, bs, L"光影", BTN_RESDL_TYPE_SHADER, resDlType_ == 1, true, 12);
    RECT bq = mkRect(sc.right - 204, sc.top + 16, sc.right - 114, sc.top + 46);
    drawButton(g, bq, L"搜索", BTN_RESDL_SEARCH, true, !resDlLoading_, 12);
    RECT bo = mkRect(sc.right - 106, sc.top + 16, sc.right - 16, sc.top + 46);
    drawButton(g, bo, L"打开文件夹", BTN_RESDL_OPEN, false, true, 12);

    std::vector<ResDlItem> items;
    std::wstring err;
    bool loading = false;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        items = resDlItems_;
        err = resDlErr_;
        loading = resDlLoading_;
    }
    int listTop = y + 76;
    int listBottom = H - 16;
    int rowH = 68, headerH = 30;
    int rows = (listBottom - listTop - headerH - 8) / rowH;
    if (rows < 1) rows = 1;
    int total = (int)items.size();
    int maxScroll = total > rows ? total - rows : 0;
    if (modMgrScroll_ > maxScroll) modMgrScroll_ = maxScroll;
    if (modMgrScroll_ < 0) modMgrScroll_ = 0;
    RECT list = mkRect(cx, listTop, cx + cw, listBottom);
    drawCard(g, list);
    std::wstring head = L"搜索结果";
    if (loading) head += L"  ·  正在搜索…"; else head += L"  ·  共 " + std::to_wstring(total) + L" 个";
    head += (resDlType_ == 0) ? L"  ·  资源包" : L"  ·  光影";
    RECT hdr = mkRect(list.left + 12, list.top + 6, list.right - 240, list.top + 36);
    drawText(g, head, fH, kTextDim, hdr, 0);
    if (dlActive_.load()) {
        std::wstring dr;
        { std::lock_guard<std::mutex> lk(mtx_); dr = dlResult_; }
        RECT st = mkRect(list.right - 300, list.top + 6, list.right - 12, list.top + 36);
        drawText(g, dr + L"  " + std::to_wstring(dlPct_.load()) + L"%", fH, kAccent, st, 2);
    }
    if (total == 0) {
        RECT e = mkRect(list.left + 12, list.top + headerH + 20, list.right - 12, list.top + headerH + 56);
        std::wstring msg = loading ? L"正在从 Modrinth 搜索…" : (err.empty() ? L"输入关键词后点「搜索」。" : err);
        drawText(g, msg, fL, kTextDim, e, 0);
        return;
    }
    int ry = list.top + 6 + headerH;
    for (int i = 0; i < rows; ++i) {
        int idx = modMgrScroll_ + i;
        if (idx >= total) break;
        const ResDlItem& it = items[idx];
        RECT row = mkRect(list.left + 8, ry, list.right - 8, ry + rowH - 6);
        if ((idx & 1) == 0) fillRound(g, row, 6, C(250, 251, 253));
        RECT rt = mkRect(row.left + 10, row.top + 4, row.right - 130, row.top + 26);
        drawText(g, it.title, fB, kText, rt, 0);
        std::wstring sub = it.author + L"  ·  下载 " + std::to_wstring(it.downloads);
        RECT rs = mkRect(row.left + 10, row.top + 26, row.right - 130, row.top + 44);
        drawText(g, sub, fD, kTextDim, rs, 0);
        RECT rd = mkRect(row.left + 10, row.top + 44, row.right - 130, row.top + 62);
        drawText(g, it.desc, fD, kTextDim, rd, 0);
        RECT bd = mkRect(row.right - 120, row.top + 18, row.right - 10, row.top + 50);
        drawButton(g, bd, L"下载最新", BTN_RESDL_DL_BASE + idx, true, !dlActive_.load(), 12);
        ry += rowH;
    }
}
void MainWindow::handleDropFiles(HDROP h) {
    UINT count = DragQueryFileW(h, 0xFFFFFFFF, nullptr, 0);
    int jars = 0, packs = 0, shaders = 0, failed = 0;
    for (UINT i = 0; i < count; ++i) {
        wchar_t path[MAX_PATH] = { 0 };
        if (DragQueryFileW(h, i, path, MAX_PATH) == 0) continue;
        std::wstring src = path;
        std::wstring name = src.substr(src.find_last_of(L"\\/") + 1);
        std::wstring lower = name;
        for (auto& c : lower) c = (wchar_t)towlower(c);
        std::wstring dir;
        if (lower.size() >= 4 && lower.substr(lower.size() - 4) == L".jar") dir = modsDir();
        else if (lower.size() >= 4 && lower.substr(lower.size() - 4) == L".zip") {
            bool shader = lower.find(L"shader") != std::wstring::npos || lower.find(L"光影") != std::wstring::npos ||
                          lower.find(L"bsl") != std::wstring::npos || lower.find(L"complementary") != std::wstring::npos ||
                          lower.find(L"sildur") != std::wstring::npos || lower.find(L"seus") != std::wstring::npos;
            dir = joinPath(activeGameDir(), shader ? L"shaderpacks" : L"resourcepacks");
        } else {
            continue;
        }
        createDirs(dir);
        std::wstring dest = joinPath(dir, name);
        if (pathExists(dest)) {
            std::wstring base = name;
            std::wstring ext;
            size_t dot = name.find_last_of(L'.');
            if (dot != std::wstring::npos) { base = name.substr(0, dot); ext = name.substr(dot); }
            for (int k = 1; k < 1000; ++k) {
                dest = joinPath(dir, base + L" (" + std::to_wstring(k) + L")" + ext);
                if (!pathExists(dest)) break;
            }
        }
        if (CopyFileW(src.c_str(), dest.c_str(), FALSE)) {
            if (dir == modsDir()) jars++; else if (dir.find(L"shaderpacks") != std::wstring::npos) shaders++; else packs++;
        } else failed++;
    }
    DragFinish(h);
    std::wstring msg = L"已导入：模组 " + std::to_wstring(jars) + L" 个，资源包 " + std::to_wstring(packs) +
                       L" 个，光影 " + std::to_wstring(shaders) + L" 个";
    if (failed) msg += L"，失败 " + std::to_wstring(failed) + L" 个";
    pushLog(msg);
    dirty_ = true;
    MessageBoxW(hwnd_, msg.c_str(), L"MCL 导入", MB_ICONINFORMATION);
}
void MainWindow::openResourceFolder(int idx) {
    int count = (int)(sizeof(kResEntries) / sizeof(kResEntries[0]));
    if (idx < 0 || idx >= count) return;
    std::wstring rbase = (wcscmp(kResEntries[idx].dir, L"versions") == 0) ? gameDir_ : activeGameDir();
    std::wstring dir = joinPath(rbase, kResEntries[idx].dir);
    if (!pathExists(dir)) createDirs(dir);
    ShellExecuteW(nullptr, L"open", dir.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

void MainWindow::drawResources(Graphics& g, int W, int H) {
    FontFamily ff(L"Microsoft YaHei");
    Font fT(&ff, 18.0f, FontStyleBold, UnitPixel);
    Font fL(&ff, 12.0f, FontStyleRegular, UnitPixel);
    Font fB(&ff, 16.0f, FontStyleBold, UnitPixel);
    Font fD(&ff, 12.0f, FontStyleRegular, UnitPixel);
    Font fC(&ff, 11.0f, FontStyleRegular, UnitPixel);

    int cx = SIDEBAR_W + 24;
    int cw = W - SIDEBAR_W - 48;
    int y = HEADER_H + 16;

    // 当前游戏目录
    RECT info = mkRect(cx, y, cx + cw, y + 58);
    drawCard(g, info);
    RECT t1 = mkRect(info.left + 16, info.top + 8, info.right - 140, info.top + 30);
    drawText(g, L"当前游戏目录", fL, kTextDim, t1, 0);
    RECT t2 = mkRect(info.left + 16, info.top + 28, info.right - 140, info.top + 50);
    drawText(g, activeGameDir(), fD, kText, t2, 0);
    RECT b0 = mkRect(info.right - 128, info.top + 14, info.right - 16, info.top + 44);
    drawButton(g, b0, L"打开 .minecraft", BTN_OPEN_GAMEDIR, false, true, 12);

    int gridTop = y + 58 + 14;
    int gap = 14;
    int cardW = (cw - gap) / 2;
    int cardH = 104;
    int total = (int)(sizeof(kResEntries) / sizeof(kResEntries[0]));

    for (int i = 0; i < total; ++i) {
        int row = i / 2;
        int col = i % 2;
        int x = cx + col * (cardW + gap);
        int yy = gridTop + row * (cardH + gap);
        if (yy + cardH > H - 16) break;
        RECT c = mkRect(x, yy, x + cardW, yy + cardH);
        drawCard(g, c);

        RECT rt = mkRect(c.left + 16, c.top + 12, c.right - 90, c.top + 36);
        drawText(g, kResEntries[i].title, fB, kText, rt, 0);

        std::wstring rbase = (wcscmp(kResEntries[i].dir, L"versions") == 0) ? gameDir_ : activeGameDir();
        std::wstring dir = joinPath(rbase, kResEntries[i].dir);
        int n = countDirEntries(dir, kResEntries[i].filter);
        std::wstring cnt = std::to_wstring(n) + L" 项";
        RECT rc = mkRect(c.right - 86, c.top + 14, c.right - 16, c.top + 36);
        drawText(g, cnt, fC, n > 0 ? kAccent : kTextDim, rc, 2);

        RECT rd = mkRect(c.left + 16, c.top + 40, c.right - 16, c.top + 62);
        drawText(g, kResEntries[i].desc, fD, kTextDim, rd, 0);

        RECT rb = mkRect(c.left + 16, c.top + 68, c.left + 126, c.top + 98);
        drawButton(g, rb, L"打开文件夹", BTN_RES_OPEN_BASE + i, false, true, 12);
        if (wcscmp(kResEntries[i].dir, L"resourcepacks") == 0 || wcscmp(kResEntries[i].dir, L"shaderpacks") == 0) {
            RECT rw = mkRect(c.left + 136, c.top + 68, c.left + 276, c.top + 98);
            drawButton(g, rw, L"Modrinth 搜索", BTN_RES_WEB_BASE + i, false, true, 11);
        }
        if (wcscmp(kResEntries[i].dir, L"crash-reports") == 0) {
            RECT ra = mkRect(c.left + 136, c.top + 68, c.left + 246, c.top + 98);
            drawButton(g, ra, L"分析最新崩溃", BTN_ANALYZE_CRASH, true, true, 12);
        }
        if (wcscmp(kResEntries[i].dir, L"logs") == 0) {
            RECT rl = mkRect(c.left + 136, c.top + 68, c.left + 276, c.top + 98);
            drawButton(g, rl, L"查看 latest.log", BTN_OPEN_LATEST_LOG, false, true, 11);
        }
    }

    RECT hint = mkRect(cx, H - 36, cx + cw, H - 16);
    drawText(g, L"提示：把资源包 / 光影放入对应文件夹后，在游戏内「选项 → 资源包 / 视频设置」中启用。",
             fD, kTextDim, hint, 0);
}
// ------------------------------ 鼠标 ---------------------------------------
void MainWindow::onMouseMove(int x, int y) {
    int old = hoverId_;
    hoverId_ = 0;
    if (draggingMem_) {
        RECT s = memSlider_;
        if (s.right > s.left) {
            int pct = (x - s.left) * 100 / (s.right - s.left);
            if (pct < 0) pct = 0; if (pct > 100) pct = 100;
            int v = kMemMinMB + (kMemMaxMB - kMemMinMB) * pct / 100;
            v = (v / 512) * 512;
            if (v < kMemMinMB) v = kMemMinMB;
            if (v > kMemMaxMB) v = kMemMaxMB;
            memoryMB_ = v;
            dirty_ = true;
            InvalidateRect(hwnd_, nullptr, FALSE);
        }
        return;
    }
    for (auto it = hits_.rbegin(); it != hits_.rend(); ++it) {
        if (x >= it->rc.left && x < it->rc.right && y >= it->rc.top && y < it->rc.bottom) {
            hoverId_ = it->id;
            break;
        }
    }
    if (hoverId_ != old) InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::onLButtonDown(int x, int y) {
    // 内存滑块拖动
    if (page_ == 0) {
        RECT hit = mkRect(memSlider_.left - 8, memSlider_.top - 6, memSlider_.right + 8, memSlider_.bottom + 6);
        if (x >= hit.left && x < hit.right && y >= hit.top && y < hit.bottom) {
            draggingMem_ = true;
            SetCapture(hwnd_);
            onMouseMove(x, y);
            return;
        }
    }
    int id = 0;
    for (auto it = hits_.rbegin(); it != hits_.rend(); ++it) {
        if (x >= it->rc.left && x < it->rc.right && y >= it->rc.top && y < it->rc.bottom) {
            id = it->id;
            break;
        }
    }
    if (id > 0) onCommand(id);
}

void MainWindow::onLButtonUp(int, int) {
    if (draggingMem_) {
        draggingMem_ = false;
        ReleaseCapture();
        // 拖动结束同步到设置页输入框
        if (editMem_) SetWindowTextW(editMem_, std::to_wstring(memoryMB_).c_str());
    }
}

void MainWindow::onMouseWheel(int delta) {
    if (page_ == 1) {
        listScroll_ -= (delta / WHEEL_DELTA) * 3;
        if (listScroll_ < 0) listScroll_ = 0;
    } else if (page_ == 4 && modDetail_ < 0) {
        modsScroll_ -= (delta / WHEEL_DELTA) * 3;
        if (modsScroll_ < 0) modsScroll_ = 0;
    } else if (page_ == 5) {
        srvScroll_ -= (delta / WHEEL_DELTA) * 2;
        if (srvScroll_ < 0) srvScroll_ = 0;
    } else if (page_ == 7 || page_ == 8) {
        modMgrScroll_ -= (delta / WHEEL_DELTA) * 2;
        if (modMgrScroll_ < 0) modMgrScroll_ = 0;
    } else {
        return;
    }
    InvalidateRect(hwnd_, nullptr, FALSE);
}

void MainWindow::onTimer() {
    if (!autoBackupChecked_) {
        autoBackupChecked_ = true;
        maybeAutoBackup();
    }
    if (dirty_.exchange(false)) {
        InvalidateRect(hwnd_, nullptr, FALSE);
    }
}

// 全屏切换: 去边框铺满当前显示器, 再切一次还原
void MainWindow::toggleFullscreen() {
    if (!fullscreen_) {
        GetWindowRect(hwnd_, &savedRect_);
        savedStyle_   = (DWORD)GetWindowLongPtrW(hwnd_, GWL_STYLE);
        savedExStyle_ = (DWORD)GetWindowLongPtrW(hwnd_, GWL_EXSTYLE);

        MONITORINFO mi{};
        mi.cbSize = sizeof(mi);
        HMONITOR mon = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
        if (mon && GetMonitorInfoW(mon, &mi)) {
            SetWindowLongPtrW(hwnd_, GWL_STYLE, WS_POPUP | WS_VISIBLE);
            SetWindowLongPtrW(hwnd_, GWL_EXSTYLE, savedExStyle_ & ~WS_EX_TOPMOST);
            SetWindowPos(hwnd_, HWND_TOP,
                         mi.rcMonitor.left, mi.rcMonitor.top,
                         mi.rcMonitor.right - mi.rcMonitor.left,
                         mi.rcMonitor.bottom - mi.rcMonitor.top,
                         SWP_FRAMECHANGED | SWP_NOZORDER);
            fullscreen_ = true;
            pushLog(L"已进入全屏, 按 F11 / Alt+Enter 退出。");
        }
    } else {
        SetWindowLongPtrW(hwnd_, GWL_STYLE, savedStyle_);
        SetWindowLongPtrW(hwnd_, GWL_EXSTYLE, savedExStyle_);
        SetWindowPos(hwnd_, nullptr,
                     savedRect_.left, savedRect_.top,
                     savedRect_.right - savedRect_.left,
                     savedRect_.bottom - savedRect_.top,
                     SWP_FRAMECHANGED | SWP_NOZORDER);
        fullscreen_ = false;
        pushLog(L"已退出全屏。");
    }
    layoutControls();
    InvalidateRect(hwnd_, nullptr, FALSE);
}

// ------------------------------ 命令处理 -----------------------------------
void MainWindow::onCommand(int id) {
    // 版本页分类标签: 枚举值大于其余 BTN_ 区间, 需在 switch 前优先截获
    if (id >= BTN_VERCAT_BASE && id < BTN_VERCAT_BASE + 5) {
        int cat = id - BTN_VERCAT_BASE;
        if (cat != verCat_) {
            verCat_ = cat;
            listScroll_ = 0;      // 切分类后滚动位置回顶
            selList_ = -1;        // 原选中行与新列表不再对应, 一并清空
            saveSettings();       // 分类状态持久化
            pushLog(std::wstring(L"版本分类切换为: ") + kVerCatNames[cat]);
        }
        dirty_ = true;
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    if (id >= BTN_RES_OPEN_BASE && id < BTN_RES_OPEN_BASE + 8) {
        openResourceFolder(id - BTN_RES_OPEN_BASE);
        return;
    }
    if (id >= BTN_MODMGR_TOGGLE_BASE && id < BTN_MODMGR_TOGGLE_BASE + 1000) {
        toggleInstalledMod(id - BTN_MODMGR_TOGGLE_BASE);
        return;
    }
    if (id >= BTN_SAVE_OPEN_BASE && id < BTN_SAVE_OPEN_BASE + 1000) {
        int i = id - BTN_SAVE_OPEN_BASE;
        if (i >= 0 && i < (int)saves_.size()) ShellExecuteW(nullptr, L"open", saves_[i].path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        return;
    }
    if (id >= BTN_SAVE_BACKUP_BASE && id < BTN_SAVE_BACKUP_BASE + 1000) {
        backupSave(id - BTN_SAVE_BACKUP_BASE);
        return;
    }
    if (id >= BTN_SRV_PING_BASE && id < BTN_SRV_PING_BASE + 1000) {
        pingServerAsync(id - BTN_SRV_PING_BASE);
        return;
    }
    if (id >= BTN_SAVE_DEL_BASE && id < BTN_SAVE_DEL_BASE + 1000) {
        deleteSave(id - BTN_SAVE_DEL_BASE);
        return;
    }
    if (id >= BTN_MODMGR_DEL_BASE && id < BTN_MODMGR_DEL_BASE + 1000) {
        deleteInstalledMod(id - BTN_MODMGR_DEL_BASE);
        return;
    }
    if (id == BTN_MODMGR_REFRESH) {
        if (page_ == 8) {
            loadSaves();
            pushLog(L"存档列表已刷新。");
        } else {
            loadInstalledMods();
            pushLog(L"模组列表已刷新。");
        }
        dirty_ = true;
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    if (id == BTN_MODMGR_OPEN) {
        std::wstring d = (page_ == 8) ? joinPath(activeGameDir(), L"backups") : modsDir();
        if (!pathExists(d)) createDirs(d);
        ShellExecuteW(nullptr, L"open", d.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        return;
    }
    if (id == BTN_EXPORT_DIAG) {
        exportDiagnostics();
        return;
    }
    if (id == BTN_NAV_GOPT) {
        page_ = 11; loadGameOptions(); layoutControls(); dirty_ = true; return;
    }
    if (id == BTN_GOPT_SAVE) { saveGameOptions(); return; }
    if (id == BTN_INSTALL_FABRIC) { installFabricAsync(); return; }
    if (id == BTN_INSTALL_FORGE) { installForgeAsync(); return; }
    if (id == BTN_DOWNLOAD_LATEST) { downloadLatestAsync(); return; }
    if (id == BTN_OPEN_LATEST_LOG) {
        std::wstring logPath = joinPath(activeGameDir(), L"logs\\latest.log");
        if (!pathExists(logPath)) {
            MessageBoxW(hwnd_, L"还没有 latest.log，请先启动一次游戏。", L"MCL 日志", MB_ICONINFORMATION);
        } else {
            ShellExecuteW(nullptr, L"open", logPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
        return;
    }
    if (id == BTN_RESDL_SEARCH) { searchResDlAsync(); return; }
    if (id == BTN_RESDL_TYPE_PACK || id == BTN_RESDL_TYPE_SHADER) {
        resDlType_ = (id == BTN_RESDL_TYPE_SHADER) ? 1 : 0;
        pushLog(resDlType_ == 0 ? L"资源类型：资源包" : L"资源类型：光影");
        dirty_ = true;
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    if (id == BTN_RESDL_OPEN) {
        std::wstring d = joinPath(activeGameDir(), resDlType_ == 0 ? L"resourcepacks" : L"shaderpacks");
        if (!pathExists(d)) createDirs(d);
        ShellExecuteW(nullptr, L"open", d.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        return;
    }
    if (id >= BTN_RESDL_DL_BASE && id < BTN_RESDL_DL_BASE + 1000) {
        downloadResLatest(id - BTN_RESDL_DL_BASE);
        return;
    }
    if (id >= BTN_RES_WEB_BASE && id < BTN_RES_WEB_BASE + 100) {
        int ri = id - BTN_RES_WEB_BASE;
        const wchar_t* url = nullptr;
        if (ri >= 0 && ri < (int)(sizeof(kResEntries) / sizeof(kResEntries[0]))) {
            if (wcscmp(kResEntries[ri].dir, L"resourcepacks") == 0) url = L"https://modrinth.com/resourcepacks";
            else if (wcscmp(kResEntries[ri].dir, L"shaderpacks") == 0) url = L"https://modrinth.com/shaders";
        }
        if (url) ShellExecuteW(nullptr, L"open", url, nullptr, nullptr, SW_SHOWNORMAL);
        return;
    }
    if (id >= BTN_GOPT_DEC_BASE && id < BTN_GOPT_INC_BASE) {
        int row = id - BTN_GOPT_DEC_BASE;
        const char* key = nullptr; int mn = 0, mx = 0, step = 1;
        if (goptNumeric(row, key, mn, mx, step)) {
            if (row == 3) {
                int deg = goptFovDegrees(gopt_) - 5;
                if (deg < 30) deg = 30;
                char fb[32];
                snprintf(fb, sizeof(fb), "%.2f", (deg - 70) / 40.0);
                gopt_["fov"] = fb;
                dirty_ = true;
                InvalidateRect(hwnd_, nullptr, FALSE);
                return;
            }
            int v = 0;
            auto it = gopt_.find(key);
            if (it != gopt_.end()) { try { v = std::stoi(it->second); } catch (...) {} }
            v -= step; if (v < mn) v = mn;
            gopt_[key] = std::to_string(v);
            dirty_ = true;
            InvalidateRect(hwnd_, nullptr, FALSE);
            return;
        }
    }
    if (id >= BTN_GOPT_INC_BASE && id < BTN_GOPT_TOGGLE_BASE) {
        int row = id - BTN_GOPT_INC_BASE;
        const char* key = nullptr; int mn = 0, mx = 0, step = 1;
        if (goptNumeric(row, key, mn, mx, step)) {
            if (row == 3) {
                int deg = goptFovDegrees(gopt_) + 5;
                if (deg > 110) deg = 110;
                char fb[32];
                snprintf(fb, sizeof(fb), "%.2f", (deg - 70) / 40.0);
                gopt_["fov"] = fb;
                dirty_ = true;
                InvalidateRect(hwnd_, nullptr, FALSE);
                return;
            }
            int v = 0;
            auto it = gopt_.find(key);
            if (it != gopt_.end()) { try { v = std::stoi(it->second); } catch (...) {} }
            v += step; if (v > mx) v = mx;
            gopt_[key] = std::to_string(v);
            dirty_ = true;
            InvalidateRect(hwnd_, nullptr, FALSE);
            return;
        }
    }
    if (id >= BTN_GOPT_TOGGLE_BASE && id < BTN_GOPT_TOGGLE_BASE + 100) {
        int row = id - BTN_GOPT_TOGGLE_BASE;
        if (row == 4) {
            gopt_["graphicsMode"] = (gopt_["graphicsMode"] == "1") ? "0" : "1";
        } else if (row == 5) {
            int v = 0; try { v = std::stoi(gopt_["particles"]); } catch (...) {}
            v = (v + 1) % 3;
            gopt_["particles"] = std::to_string(v);
        } else if (row == 6) {
            gopt_["renderClouds"] = (gopt_["renderClouds"] == "true") ? "false" : "true";
        } else if (row == 7) {
            gopt_["fullscreen"] = (gopt_["fullscreen"] == "true") ? "false" : "true";
        }
        dirty_ = true;
        InvalidateRect(hwnd_, nullptr, FALSE);
        return;
    }
    if (id == BTN_SAVE_RESTORE) { restoreLatestBackup(); return; }
    if (id == BTN_ACCOUNT_ADD) { addAccount(); return; }
    if (id >= BTN_ACCOUNT_USE_BASE && id < BTN_ACCOUNT_DEL_BASE) {
        useAccount(id - BTN_ACCOUNT_USE_BASE);
        return;
    }
    if (id >= BTN_ACCOUNT_DEL_BASE && id < BTN_ACCOUNT_DEL_BASE + 1000) {
        deleteAccount(id - BTN_ACCOUNT_DEL_BASE);
        return;
    }
    if (id == BTN_JAVA_SCAN) { scanJavaVersions(); return; }
    if (id >= BTN_JAVA_USE_BASE && id < BTN_JAVA_USE_BASE + 100) {
        useJavaVersion(id - BTN_JAVA_USE_BASE);
        return;
    }
    if (id == BTN_MODMGR_ENABLE_ALL) { setAllMods(true); return; }
    if (id == BTN_MODMGR_DISABLE_ALL) { setAllMods(false); return; }
    if (id == BTN_ANALYZE_CRASH) {
        analyzeCrash();
        return;
    }
    if (id == BTN_OPEN_GAMEDIR) {
        if (!pathExists(gameDir_)) createDirs(gameDir_);
        ShellExecuteW(nullptr, L"open", gameDir_.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        return;
    }
    switch (id) {
    case BTN_NAV_HOME:     page_ = 0; layoutControls(); dirty_ = true; break;
    case BTN_NAV_VERSIONS: page_ = 1; layoutControls(); dirty_ = true; break;
    case BTN_NAV_SETTINGS: page_ = 2; layoutControls();
        SetWindowTextW(editDir_, gameDir_.c_str());
        SetWindowTextW(editJava_, javaPath_.c_str());
        SetWindowTextW(editMem_, std::to_wstring(memoryMB_).c_str());
        dirty_ = true; break;
    case BTN_NAV_ABOUT:    page_ = 3; layoutControls(); dirty_ = true; break;

    // ---- 存档管理页
    case BTN_NAV_SAVES:
        page_ = 8; loadSaves(); layoutControls(); dirty_ = true; break;

    // ---- 游戏选项页
    case BTN_NAV_GOPT:
        page_ = 11; loadGameOptions(); layoutControls(); dirty_ = true; break;

    // ---- 资源下载页
    case BTN_NAV_RESDL:
        page_ = 12; layoutControls(); dirty_ = true; break;

    // ---- 账户页
    case BTN_NAV_ACCOUNTS:
        page_ = 10; layoutControls(); dirty_ = true; break;

    // ---- 模组管理页
    case BTN_NAV_MODMGR:
        page_ = 7;
        modMgrFilter_.clear();
        if (editModSearch_) SetWindowTextW(editModSearch_, L"");
        loadInstalledMods(); layoutControls(); dirty_ = true; break;

    // ---- 资源管理页
    case BTN_NAV_RESOURCES:
        page_ = 6; layoutControls(); dirty_ = true; break;

    // ---- 联机页
    case BTN_NAV_SERVERS:
        openServersPage();
        break;
    case BTN_SRV_ADD:
        commitServerForm();
        break;
    case BTN_SRV_RESET:
        fillServerForm(-1);
        dirty_ = true;
        break;
    case BTN_SRV_IMPORT:
        importPublicServers();
        break;

    // ---- 模组下载页
    case BTN_NAV_MODS:
        openModPage();
        break;

    case BTN_MODS_QUERY: {
        wchar_t buf[512] = { 0 };
        if (editModSearch_) GetWindowTextW(editModSearch_, buf, 512);
        std::wstring kw = trimW(buf);
        if (kw.empty()) {
            MessageBoxW(hwnd_, L"请先输入要检索的模组关键词。", L"提示", MB_ICONINFORMATION);
            break;
        }
        {
            std::lock_guard<std::mutex> lk(mtx_);
            modsKeyword_ = kw;
        }
        searchModsAsync(kw);
        break;
    }

    case BTN_MODS_BACK:
        modDetail_ = -1;
        detailLinkCount_ = 0;
        {
            std::lock_guard<std::mutex> lk(mtx_);
            modFiles_.clear();
            filesLoaded_ = false;
            filesErr_.clear();
        }
        layoutControls();
        dirty_ = true;
        break;

    case BTN_MODS_GET_VERSIONS:
        filesLoaded_ = false;
        fetchModFilesAsync();
        break;

    case BTN_MODS_OPEN_PAGE: {
        std::wstring addr;
        {
            std::lock_guard<std::mutex> lk(mtx_);
            addr = detail_.address;
        }
        if (!addr.empty()) ShellExecuteW(nullptr, L"open", addr.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        break;
    }

    case BTN_MODS_OPEN_MR: {
        std::wstring slug;
        {
            std::lock_guard<std::mutex> lk(mtx_);
            slug = detail_.slug;
        }
        if (!slug.empty()) {
            std::wstring u = L"https://modrinth.com/mod/" + slug;
            ShellExecuteW(nullptr, L"open", u.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        }
        break;
    }

    case BTN_MODS_MR_SEARCH:
        resolveModrinthBySearchAsync();
        break;

    case BTN_MODS_OPEN_DIR: {
        std::wstring d = modsDir();
        if (!pathExists(d)) createDirs(d);
        ShellExecuteW(nullptr, L"open", d.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        break;
    }

    case BTN_PICK_VERSION:
        page_ = 1; layoutControls(); dirty_ = true; break;

    case BTN_CHECK_UPDATE:
        checkUpdateAsync();
        break;

    case BTN_OPEN_SITE:
        ShellExecuteW(nullptr, L"open", kServerBase, nullptr, nullptr, SW_SHOWNORMAL);
        break;

    case BTN_OPEN_FEEDBACK: {
        std::wstring feedbackUrl = std::wstring(kServerBase) + L"/?feedback=1";
        ShellExecuteW(nullptr, L"open", feedbackUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        break;
    }

    case BTN_REFRESH:
        manifestLoaded_ = false;
        refreshManifestAsync();
        pushLog(L"正在刷新版本列表...");
        break;

    case BTN_INSTALL_SEL:
        installSelectedAsync();
        break;

    case BTN_START_GAME:
    case BTN_LAUNCH_SEL:
        launchSelectedAsync();
        break;

    case BTN_DELETE_SEL: {
        if (selList_ < 0) return;
        // 由当前列表获取版本名
        std::wstring target;
        if (manifestLoaded_.load() && !manifest_.empty()) {
            // 重新构造过滤后的可见行, 与绘制保持一致
            std::vector<std::string> ids;
            {
                std::lock_guard<std::mutex> lk(mtx_);
                for (int idx : filtered_) {
                    if (idx < 0 || idx >= (int)manifest_.size()) continue;
                    const VersionInfo& v = manifest_[idx];
                    if (!versionInCategory(verCat_, v.type, v.id, v.releaseTime)) continue;
                    if (!versionMatchesSearch(verSearch_, v.id)) continue;
                    bool inst = installedSet_.count(v.id) > 0;
                    if (showInstalledOnly_ && !inst) continue;
                    ids.push_back(v.id);
                }
            }
            if (selList_ < (int)ids.size()) target = utf8ToWide(ids[selList_]);
        } else {
            std::lock_guard<std::mutex> lk(mtx_);
            if (selList_ < (int)installedOnly_.size()) target = utf8ToWide(installedOnly_[selList_]);
        }
        if (target.empty()) return;
        std::wstring msg = L"确定要删除版本 " + target + L" 吗？\n\n该操作会删除该版本的 jar、json 与 natives 文件，不可撤销。";
        if (MessageBoxW(hwnd_, msg.c_str(), L"删除版本", MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2) != IDYES)
            return;
        // 执行删除 (后台)
        std::wstring vdir = launcher_.versionDir(wideToUtf8(target));
        busy_ = true;
        std::thread([this, vdir, target]() {
            // 递归删除目录
            std::wstring cmd = L"rmdir /s /q \"" + vdir + L"\"";
            std::vector<wchar_t> buf(cmd.begin(), cmd.end());
            buf.push_back(0);
            STARTUPINFOW si{}; si.cb = sizeof(si);
            PROCESS_INFORMATION pi{};
            if (CreateProcessW(nullptr, buf.data(), nullptr, nullptr, FALSE,
                               CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
                WaitForSingleObject(pi.hProcess, INFINITE);
                CloseHandle(pi.hProcess); CloseHandle(pi.hThread);
            }
            pushLog(L"已删除版本目录: " + target);
            std::lock_guard<std::mutex> lk(mtx_);
            installedOnly_ = launcher_.listInstalled();
            installedSet_.clear();
            for (auto& s : installedOnly_) installedSet_.insert(s);
            busy_ = false;
            dirty_ = true;
        }).detach();
        break;
    }

    case BTN_CANCEL:
        launcher_.cancelled = true;
        pushLog(L"已请求取消当前任务...");
        break;

    case BTN_BROWSE_DIR: {
        // 简易目录选择: 使用 SHBrowseForFolder
        BROWSEINFOW bi{};
        bi.hwndOwner = hwnd_;
        bi.lpszTitle = L"选择 .minecraft 游戏目录";
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
        if (pidl) {
            wchar_t path[MAX_PATH] = { 0 };
            if (SHGetPathFromIDListW(pidl, path)) {
                gameDir_ = path;
                SetWindowTextW(editDir_, gameDir_.c_str());
            }
            CoTaskMemFree(pidl);
        }
        break;
    }

    case BTN_BROWSE_JAVA: {
        OPENFILENAMEW ofn{};
        wchar_t file[MAX_PATH] = { 0 };
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hwnd_;
        ofn.lpstrFilter = L"Java 可执行文件\0java.exe;javaw.exe\0所有文件\0*.*\0";
        ofn.lpstrFile = file;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        if (GetOpenFileNameW(&ofn)) {
            javaPath_ = file;
            SetWindowTextW(editJava_, javaPath_.c_str());
        }
        break;
    }

    case BTN_AUTO_JAVA: {
        std::wstring j = Launcher::autoDetectJava();
        if (j.empty()) {
            MessageBoxW(hwnd_, L"未能自动检测到 Java，请手动指定 java.exe 路径。", L"提示", MB_ICONINFORMATION);
        } else {
            javaPath_ = j;
            SetWindowTextW(editJava_, javaPath_.c_str());
            int major = Launcher::queryJavaMajor(j);
            javaInfo_ = L"Java " + std::to_wstring(major);
            pushLog(L"已检测到 Java: " + j + L"  (主版本 " + std::to_wstring(major) + L")");
        }
        break;
    }

    case BTN_MIRROR_BMCL:     useMirror_ = true;  dirty_ = true; break;
    case BTN_MIRROR_OFFICIAL: useMirror_ = false; dirty_ = true; break;

    case BTN_SAVE: {
        wchar_t buf[4096];
        GetWindowTextW(editDir_, buf, 4096); gameDir_ = trimW(buf);
        if (gameDir_.empty()) {
            MessageBoxW(hwnd_, L"游戏目录不能为空。", L"设置", MB_ICONWARNING);
            return;
        }
        if (!pathExists(gameDir_)) {
            createDirs(gameDir_);
            if (!pathExists(gameDir_)) {
                MessageBoxW(hwnd_, L"游戏目录不存在且无法创建。", L"设置", MB_ICONWARNING);
                return;
            }
        }
        GetWindowTextW(editJava_, buf, 4096); javaPath_ = trimW(buf);
        if (!javaPath_.empty() && !pathExists(javaPath_)) {
            std::wstring autoJ = Launcher::autoDetectJava();
            if (autoJ.empty()) {
                MessageBoxW(hwnd_, L"Java 路径不存在，且没有自动检测到可用 Java。", L"设置", MB_ICONWARNING);
                return;
            }
            javaPath_ = autoJ;
            SetWindowTextW(editJava_, javaPath_.c_str());
        }
        GetWindowTextW(editMem_, buf, 64);
        try { memoryMB_ = std::stoi(buf); } catch (...) {}
        if (memoryMB_ < kMemMinMB) memoryMB_ = kMemMinMB;
        if (memoryMB_ > kMemMaxMB) memoryMB_ = kMemMaxMB;
        saveSettings();
        applySettingsToLauncher();
        pushLog(L"设置已保存。");
        MessageBoxW(hwnd_, L"设置已保存。", L"提示", MB_ICONINFORMATION);
        break;
    }

    case BTN_ISOLATION:
        isolation_ = !isolation_;
        saveSettings();
        if (page_ == 8) loadSaves();
        pushLog(isolation_ ? L"已开启版本隔离：mods/saves 等从 versions/<版本>/ 读取"
                           : L"已关闭版本隔离：回到全局 .minecraft");
        if (isolation_ && !selVersion_.empty()) {
            MessageBoxW(hwnd_, L"版本隔离已开启。\n\n当前版本的 mods、saves、resourcepacks 等将从 versions/<版本>/ 读取；现有的全局数据不会自动迁移。",
                        L"版本隔离", MB_ICONINFORMATION);
        }
        dirty_ = true;
        break;

    case BTN_AUTO_BACKUP:
        autoBackup_ = !autoBackup_;
        saveSettings();
        pushLog(autoBackup_ ? L"已开启启动时自动备份存档" : L"已关闭启动时自动备份存档");
        dirty_ = true;
        break;

    case BTN_FULLSCREEN:
        toggleFullscreen();
        break;

    case BTN_TOGGLE_INSTALLED:
        showInstalledOnly_ = !showInstalledOnly_;
        listScroll_ = 0;
        dirty_ = true;
        break;

    default:
        // ---- 联机页列表热点 (基准值最大, 必须先判断, 否则会被模组页的区间吞掉)
        if (id >= BTN_SRV_ROW_BASE && id < BTN_SRV_CONNECT_BASE) {
            int idx = id - BTN_SRV_ROW_BASE;
            {
                std::lock_guard<std::mutex> lk(mtx_);
                if (idx >= 0 && idx < (int)servers_.size()) srvSel_ = idx;
            }
        } else if (id >= BTN_SRV_CONNECT_BASE && id < BTN_SRV_EDIT_BASE) {
            connectServer(id - BTN_SRV_CONNECT_BASE);
        } else if (id >= BTN_SRV_EDIT_BASE && id < BTN_SRV_DEL_BASE) {
            fillServerForm(id - BTN_SRV_EDIT_BASE);
        } else if (id >= BTN_SRV_DEL_BASE && id < BTN_SRV_DEL_BASE + 10000) {
            deleteServer(id - BTN_SRV_DEL_BASE);
        } else if (id >= BTN_SRV_VIS_BASE && id < BTN_SRV_VIS_BASE + 10000) {
            toggleServerVisible(id - BTN_SRV_VIS_BASE);
        } else if (id >= BTN_MOD_LINK_BASE) {
            int idx = id - BTN_MOD_LINK_BASE;
            std::wstring url;
            {
                std::lock_guard<std::mutex> lk(mtx_);
                if (idx >= 0 && idx < (int)detail_.links.size()) url = detail_.links[idx].second;
            }
            if (!url.empty()) ShellExecuteW(nullptr, L"open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        } else if (id >= BTN_MODVER_ROW_BASE) {
            int idx = id - BTN_MODVER_ROW_BASE;
            ModFile mf;
            bool found = false;
            {
                std::lock_guard<std::mutex> lk(mtx_);
                if (idx >= 0 && idx < (int)modFiles_.size()) { mf = modFiles_[idx]; found = true; }
            }
            if (found) startModDownload(mf);
        } else if (id >= BTN_MOD_ROW_BASE) {
            int idx = id - BTN_MOD_ROW_BASE;
            {
                std::lock_guard<std::mutex> lk(mtx_);
                if (idx >= 0 && idx < (int)modsItems_.size()) modsSel_ = idx;
            }
            ensureDetailAsync(idx);
        } else if (id >= BTN_MODS_LOADER_BASE && id < BTN_MODS_LOADER_BASE + 4) {
            detailLoader_ = id - BTN_MODS_LOADER_BASE;
            filesLoaded_ = false;
            fetchModFilesAsync();
        } else if (id >= BTN_LIST_BASE) {
            selList_ = id - BTN_LIST_BASE;
            dirty_ = true;
        }
        break;
    }
    dirty_ = true;
}

// ===========================================================================
//  联机页: 服务器列表 + 一键直连
// ===========================================================================

void MainWindow::openServersPage() {
    page_ = 5;
    srvScroll_ = 0;
    srvSel_ = -1;
    fillServerForm(-1);
    layoutControls();
    dirty_ = true;
}

// 读取 <exe 目录>\mcl_servers.txt (UTF-8, 每行: 备注名|地址|端口)
void MainWindow::loadServers() {
    std::string data;
    FILE* fp = _wfopen(serversFilePath().c_str(), L"rb");
    if (fp) {
        char buf[4096];
        size_t n = 0;
        while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) data.append(buf, n);
        fclose(fp);
    }
    std::wstring text = utf8ToWide(data);
    if (!text.empty() && text[0] == (wchar_t)0xFEFF) text.erase(0, 1);

    std::vector<ServerEntry> out;
    std::vector<std::wstring> lines = splitW(text, L'\n');
    for (size_t li = 0; li < lines.size(); ++li) {
        std::wstring line = trimW(lines[li]);
        if (line.empty() || line[0] == L'#') continue;
        std::vector<std::wstring> f = splitW(line, L'|');
        if (f.size() < 2) continue;
        ServerEntry e;
        e.name = trimW(f[0]);
        e.host = trimW(f[1]);
        e.port = 25565;
        if (f.size() >= 3) {
            int p = 0;
            if (parsePortW(f[2], p)) e.port = p;
        }
        // 第 4 列: 显示状态 (缺省视为"显示服务器")
        if (f.size() >= 4) {
            std::wstring v = trimW(f[3]);
            e.visible = !(v == L"0" || v == L"false" || v == L"否" || v == L"隐藏");
        }
        if (e.host.empty()) continue;
        if (e.name.empty()) e.name = e.host;
        out.push_back(e);
    }
    {
        std::lock_guard<std::mutex> lk(mtx_);
        servers_ = out;
    }
    // 启动时同步一次公开清单, 保证"显示服务器"的条目对外可见
    writePublicServers();
}

void MainWindow::saveServers() {
    std::vector<ServerEntry> items;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        items = servers_;
    }
    std::string out;
    for (size_t i = 0; i < items.size(); ++i) {
        const ServerEntry& e = items[i];
        std::wstring host = sanitizeServerField(e.host);
        if (host.empty()) continue;
        std::wstring name = sanitizeServerField(e.name);
        int port = (e.port > 0 && e.port <= 65535) ? e.port : 25565;
        out += wideToUtf8(name + L"|" + host + L"|" + std::to_wstring(port) +
                          L"|" + (e.visible ? L"1" : L"0") + L"\n");
    }
    FILE* fp = _wfopen(serversFilePath().c_str(), L"wb");
    if (!fp) {
        pushLog(L"服务器列表保存失败: 无法写入 " + serversFilePath());
        return;
    }
    if (!out.empty()) fwrite(out.data(), 1, out.size(), fp);
    fclose(fp);
    // 本地列表变化后同步公开清单 (只包含"显示服务器"的条目)
    writePublicServers();
}

// 把"显示服务器"的条目写入公开清单, 供他人导入; "不显示服务器"的条目不会出现在这里
void MainWindow::writePublicServers() {
    std::vector<ServerEntry> items;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        items = servers_;
    }
    std::string out = "# MCL 公开服务器清单 (由启动器自动生成)\n"
                      "# 格式: 名称|地址|端口   —— 只有勾选\"显示服务器\"的条目才会出现在这里\n";
    for (size_t i = 0; i < items.size(); ++i) {
        const ServerEntry& e = items[i];
        if (!e.visible) continue;
        std::wstring host = sanitizeServerField(e.host);
        if (host.empty()) continue;
        std::wstring name = sanitizeServerField(e.name);
        if (name.empty()) name = host;
        int port = (e.port > 0 && e.port <= 65535) ? e.port : 25565;
        out += wideToUtf8(name + L"|" + host + L"|" + std::to_wstring(port) + L"\n");
    }
    FILE* fp = _wfopen(publicServersFilePath().c_str(), L"wb");
    if (!fp) return;
    fwrite(out.data(), 1, out.size(), fp);
    fclose(fp);
}

// 把公开清单里本机还没有的服务器导入到本地列表 (导入的条目同样是"显示服务器")
void MainWindow::importPublicServers() {
    std::string data;
    FILE* fp = _wfopen(publicServersFilePath().c_str(), L"rb");
    if (!fp) {
        MessageBoxW(hwnd_, L"还没有公开清单文件:\n把某个服务器切换为\"显示服务器\"后才会生成。",
                    L"联机", MB_ICONINFORMATION);
        return;
    }
    char buf[4096];
    size_t n = 0;
    while ((n = fread(buf, 1, sizeof(buf), fp)) > 0) data.append(buf, n);
    fclose(fp);

    std::wstring text = utf8ToWide(data);
    if (!text.empty() && text[0] == (wchar_t)0xFEFF) text.erase(0, 1);

    int added = 0;
    std::vector<std::wstring> lines = splitW(text, L'\n');
    {
        std::lock_guard<std::mutex> lk(mtx_);
        for (size_t li = 0; li < lines.size(); ++li) {
            std::wstring line = trimW(lines[li]);
            if (line.empty() || line[0] == L'#') continue;
            std::vector<std::wstring> f = splitW(line, L'|');
            if (f.size() < 2) continue;
            ServerEntry e;
            e.name = trimW(f[0]);
            e.host = trimW(f[1]);
            e.port = 25565;
            if (f.size() >= 3) { int p = 0; if (parsePortW(f[2], p)) e.port = p; }
            if (e.host.empty()) continue;
            if (e.name.empty()) e.name = e.host;
            e.visible = true;
            bool dup = false;
            for (size_t i = 0; i < servers_.size(); ++i) {
                if (servers_[i].host == e.host && servers_[i].port == e.port) { dup = true; break; }
            }
            if (dup) continue;
            servers_.push_back(e);
            ++added;
        }
    }
    if (added > 0) {
        saveServers();
        pushLog(L"已从公开清单导入 " + std::to_wstring(added) + L" 个服务器。");
    } else {
        pushLog(L"公开清单里没有可导入的新服务器。");
    }
    dirty_ = true;
}

// 切换某个条目是否对外显示
void MainWindow::toggleServerVisible(int idx) {
    bool nowVisible = false;
    std::wstring label;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (idx < 0 || idx >= (int)servers_.size()) return;
        servers_[idx].visible = !servers_[idx].visible;
        nowVisible = servers_[idx].visible;
        label = servers_[idx].name + L"  (" + servers_[idx].host + L":" +
                std::to_wstring(servers_[idx].port) + L")";
    }
    saveServers();
    pushLog(nowVisible ? (L"已设为显示服务器 (任何人都能看到并进入): " + label)
                       : (L"已设为不显示服务器 (仅本机可见): " + label));
    dirty_ = true;
}

void MainWindow::fillServerForm(int idx) {
    ServerEntry e;
    bool found = false;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (idx >= 0 && idx < (int)servers_.size()) { e = servers_[idx]; found = true; }
    }
    if (found) {
        SetWindowTextW(editSrvName_, e.name.c_str());
        SetWindowTextW(editSrvHost_, e.host.c_str());
        SetWindowTextW(editSrvPort_, std::to_wstring(e.port).c_str());
        srvEditIdx_ = idx;
        srvSel_ = idx;
    } else {
        SetWindowTextW(editSrvName_, L"");
        SetWindowTextW(editSrvHost_, L"");
        SetWindowTextW(editSrvPort_, L"25565");
        srvEditIdx_ = -1;
    }
}

void MainWindow::commitServerForm() {
    wchar_t buf[256] = { 0 };
    GetWindowTextW(editSrvName_, buf, 256);
    std::wstring name = trimW(buf);
    buf[0] = 0;
    GetWindowTextW(editSrvHost_, buf, 256);
    std::wstring host = trimW(buf);
    buf[0] = 0;
    GetWindowTextW(editSrvPort_, buf, 256);
    std::wstring portS = trimW(buf);

    if (host.empty()) {
        MessageBoxW(hwnd_, L"请先填写服务器地址 (IP 或域名)。", L"联机", MB_ICONWARNING);
        return;
    }
    int port = 25565;
    if (!portS.empty() && !parsePortW(portS, port)) {
        MessageBoxW(hwnd_, L"端口需要是 1 - 65535 之间的整数。", L"联机", MB_ICONWARNING);
        return;
    }
    // 允许直接粘贴 "地址:端口" 形式, 此时以地址里的端口为准
    size_t colon = host.rfind(L':');
    if (colon != std::wstring::npos && host.find(L':') == colon) {
        int p2 = 0;
        if (parsePortW(host.substr(colon + 1), p2)) {
            host = trimW(host.substr(0, colon));
            port = p2;
        }
    }
    if (host.empty()) {
        MessageBoxW(hwnd_, L"服务器地址无效。", L"联机", MB_ICONWARNING);
        return;
    }
    if (name.empty()) name = host;

    bool updated = false;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (srvEditIdx_ >= 0 && srvEditIdx_ < (int)servers_.size()) {
            servers_[srvEditIdx_].name = name;
            servers_[srvEditIdx_].host = host;
            servers_[srvEditIdx_].port = port;
            srvSel_ = srvEditIdx_;
            updated = true;
        } else {
            ServerEntry e;
            e.name = name;
            e.host = host;
            e.port = port;
            servers_.push_back(e);
            srvSel_ = (int)servers_.size() - 1;
        }
    }
    srvEditIdx_ = -1;
    saveServers();
    SetWindowTextW(editSrvName_, L"");
    SetWindowTextW(editSrvHost_, L"");
    SetWindowTextW(editSrvPort_, L"25565");
    pushLog((updated ? L"已更新服务器: " : L"已添加服务器: ") + host + L":" + std::to_wstring(port));
    dirty_ = true;
}

void MainWindow::deleteServer(int idx) {
    std::wstring label;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (idx < 0 || idx >= (int)servers_.size()) return;
        label = servers_[idx].name + L"  (" + servers_[idx].host + L":" +
                std::to_wstring(servers_[idx].port) + L")";
    }
    std::wstring msg = L"确定删除这条服务器记录吗?\n\n" + label;
    if (MessageBoxW(hwnd_, msg.c_str(), L"删除服务器",
                    MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) != IDYES) {
        return;
    }
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (idx < 0 || idx >= (int)servers_.size()) return;
        servers_.erase(servers_.begin() + idx);
        if (srvEditIdx_ == idx)     srvEditIdx_ = -1;
        else if (srvEditIdx_ > idx) --srvEditIdx_;
        if (srvSel_ >= (int)servers_.size()) srvSel_ = (int)servers_.size() - 1;
    }
    saveServers();
    pushLog(L"已删除服务器记录。");
    dirty_ = true;
}

void MainWindow::connectServer(int idx) {
    if (busy_.load()) {
        pushLog(L"当前有任务在执行, 请稍候再连接。");
        dirty_ = true;
        return;
    }
    std::wstring host;
    int port = 25565;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (idx < 0 || idx >= (int)servers_.size()) return;
        host = servers_[idx].host;
        port = servers_[idx].port;
        srvSel_ = idx;
    }
    if (host.empty()) return;

    // 目标版本: 用当前选中的版本; 若本地未安装则回退到第一个已安装版本
    std::vector<std::string> local = launcher_.listInstalled();
    std::string id = wideToUtf8(selVersion_);
    bool installed = false;
    for (size_t i = 0; i < local.size(); ++i) { if (local[i] == id) { installed = true; break; } }
    if (!installed) {
        if (local.empty()) {
            MessageBoxW(hwnd_,
                        L"本地还没有已安装的游戏版本。\n请先在「版本列表」里下载一个版本, 再回来连接服务器。",
                        L"联机", MB_ICONINFORMATION);
            return;
        }
        id = local[0];
    }

    wchar_t buf[256] = { 0 };
    GetWindowTextW(editName_, buf, 256);
    playerName_ = trimW(buf);
    if (playerName_.empty()) playerName_ = L"Steve";

    selVersion_ = utf8ToWide(id);
    applySettingsToLauncher();
    launcher_.opt.serverAddress = wideToUtf8(host);   // 直连参数只在这一条路径上注入
    launcher_.opt.serverPort = port;
    saveSettings();

    busy_ = true;
    launcher_.cancelled = false;
    const std::wstring target = host + L":" + std::to_wstring(port);
    std::thread([this, id, target]() {
        std::wstring err;
        pushLog(L"================ 一键进服 " + target + L" ================");
        bool okFlag = launcher_.launch(id, err);
        if (okFlag) {
            setStatus(-1, L"游戏已启动, 正在进入 " + target);
            pushLog(L"游戏进程已创建, 直连目标 " + target + L"。");
        } else {
            setStatus(-1, L"启动失败: " + err);
            pushLog(L"启动失败: " + err);
        }
        busy_ = false;
        dirty_ = true;
    }).detach();
    dirty_ = true;
}

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
void MainWindow::drawServers(Graphics& g, int W, int H) {
    FontFamily ff(L"Microsoft YaHei");
    Font fT(&ff, 14.0f, FontStyleBold, UnitPixel);
    Font fS(&ff, 12.0f, FontStyleRegular, UnitPixel);
    Font fL(&ff, 12.0f, FontStyleRegular, UnitPixel);

    int cx = SIDEBAR_W + 24;
    int cw = W - SIDEBAR_W - 48;
    int y  = HEADER_H + 16;

    // ---- 顶部表单卡片 (三个输入框是原生 EDIT, 这里只画卡片 / 标签 / 按钮)
    drawCard(g, mkRect(cx, y, cx + cw, y + 106));
    drawText(g, L"名称", fL, kTextDim, mkRect(cx + 16, y + 14, cx + 56, y + 44), 0);
    drawText(g, L"地址", fL, kTextDim, mkRect(cx + 216, y + 14, cx + 256, y + 44), 0);
    int portLabelX = cx + cw - 274;
    drawText(g, L"端口", fL, kTextDim, mkRect(portLabelX, y + 14, portLabelX + 44, y + 44), 0);

    drawButton(g, mkRect(cx + cw - 136, y + 14, cx + cw - 16, y + 44),
               srvEditIdx_ >= 0 ? L"保存修改" : L"添加服务器",
               BTN_SRV_ADD, true, !busy_.load(), 13);

    std::wstring tip = (srvEditIdx_ >= 0)
        ? L"正在编辑已有条目, 改完点「保存修改」生效。"
        : L"填名称与地址后点「添加服务器」; 「显示服务器」的条目任何人都能进入。";
    drawText(g, tip, fS, kTextDim, mkRect(cx + 16, y + 62, cx + cw - 296, y + 92), 0);
    drawButton(g, mkRect(cx + cw - 276, y + 62, cx + cw - 148, y + 92), L"导入公开清单",
               BTN_SRV_IMPORT, false, true, 12);
    drawButton(g, mkRect(cx + cw - 140, y + 62, cx + cw - 16, y + 92), L"清空表单",
               BTN_SRV_RESET, false, true, 12);

    // ---- 数据快照
    std::vector<ServerEntry> items;
    std::map<std::string, int> pings;
    int sel = -1;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        items = servers_;
        pings = srvPingMs_;
        sel = srvSel_;
    }
    int total = (int)items.size();

    int ly = y + 118;
    RECT area = mkRect(cx, ly, cx + cw, H - 16);
    srvListArea_ = area;

    if (total == 0) {
        drawText(g, L"还没有服务器, 先在上方添加一个吧", fT, kTextDim,
                 mkRect(area.left, area.top, area.right, area.top + 100), 1, false);
        return;
    }

    const int rowH = 58;
    int viewRows = (area.bottom - area.top) / rowH;
    if (viewRows < 1) viewRows = 1;
    int maxScroll = total - viewRows;
    if (maxScroll < 0) maxScroll = 0;
    if (srvScroll_ > maxScroll) srvScroll_ = maxScroll;
    if (srvScroll_ < 0) srvScroll_ = 0;

    const int bh = 30;
    for (int i = 0; i < viewRows; ++i) {
        int ri = srvScroll_ + i;
        if (ri >= total) break;
        const ServerEntry& it = items[ri];
        int ry = area.top + i * rowH;
        RECT rr = mkRect(area.left, ry, area.right, ry + rowH - 6);

        bool hover = (hoverId_ == BTN_SRV_ROW_BASE + ri ||
                      hoverId_ == BTN_SRV_CONNECT_BASE + ri ||
                      hoverId_ == BTN_SRV_EDIT_BASE + ri ||
                      hoverId_ == BTN_SRV_DEL_BASE + ri ||
                      hoverId_ == BTN_SRV_PING_BASE + ri ||
                      hoverId_ == BTN_SRV_VIS_BASE + ri);
        bool selected = (sel == ri);
        fillRound(g, rr, 8, selected ? kSelBg : (hover ? kHover : kCardBg));
        strokeRound(g, rr, 8, selected ? kAccent : kLine);
        hits_.push_back({ rr, BTN_SRV_ROW_BASE + ri });

        int barW = 420;   // 右侧按钮区宽度 (5 个按钮)
        drawText(g, it.name.empty() ? L"(未命名)" : it.name, fT, kText,
                 mkRect(rr.left + 16, rr.top + 7, rr.right - barW, rr.top + 30), 0);
        std::wstring sub = it.host + L":" + std::to_wstring(it.port) +
                           (it.visible ? L"  ·  显示服务器 (任何人都能进入)"
                                       : L"  ·  不显示服务器 (仅本机可见)");
        drawText(g, sub, fS, it.visible ? kAccent : kTextDim,
                 mkRect(rr.left + 16, rr.top + 29, rr.right - barW, rr.top + 49), 0);

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
                   it.visible ? L"不显示服务器" : L"显示服务器",
                   BTN_SRV_VIS_BASE + ri, false, true, 11);
        drawButton(g, mkRect(rr.right - 216, by, rr.right - 160, by + bh), L"删除",
                   BTN_SRV_DEL_BASE + ri, false, true, 12);
        drawButton(g, mkRect(rr.right - 152, by, rr.right - 96, by + bh), L"编辑",
                   BTN_SRV_EDIT_BASE + ri, false, true, 12);
        drawButton(g, mkRect(rr.right - 88,  by, rr.right - 16,  by + bh), L"连接",
                   BTN_SRV_CONNECT_BASE + ri, true, !busy_.load(), 12);
    }

    // ---- 滚动条
    if (maxScroll > 0) {
        int trackH = area.bottom - area.top;
        int barH = trackH * viewRows / total;
        if (barH < 30) barH = 30;
        int barY = area.top + (trackH - barH) * srvScroll_ / maxScroll;
        fillRound(g, mkRect(area.right - 6, area.top, area.right - 2, area.bottom), 2, C(232, 234, 238));
        fillRound(g, mkRect(area.right - 6, barY, area.right - 2, barY + barH), 2, C(198, 202, 210));
    }
}

// ------------------------------ 业务异步 -----------------------------------
void MainWindow::checkUpdateAsync() {
    pushLog(L"正在检查服务器上的最新版本...");
    std::thread([this]() {
        std::string body;
        std::wstring err;
        bool ok = httpGetToString(std::wstring(kServerBase) + L"/api/status", body, err);
        if (!ok) {
            pushLog(L"检查更新失败: " + err);
            setStatus(-1, L"服务器未连接，可稍后重试");
            return;
        }
        mjson::Value v;
        if (!mjson::parse(body, v) || !v.isObject() || !v.has("version")) {
            pushLog(L"检查更新失败: 服务器返回内容无法解析");
            return;
        }
        std::string sv = v["version"].asString();
        if (sv.empty()) {
            pushLog(L"检查更新失败: 服务器未返回版本号");
            return;
        }
        std::wstring remote = utf8ToWide(sv);
        std::wstring current = kAppVersion;
        if (remote == current) {
            pushLog(L"服务器在线，已是最新版本 v" + current + L"。");
            setStatus(-1, L"Win10 服务器在线 · 已是最新版本 v" + current);
        } else {
            pushLog(L"发现新版本 v" + remote + L"（当前 v" + current + L"），点击「官网 / 反馈」下载。");
            setStatus(-1, L"发现新版本 v" + remote + L"，点击「官网 / 反馈」下载");
        }
        dirty_ = true;
        if (hwnd_) InvalidateRect(hwnd_, nullptr, FALSE);
    }).detach();
}

void MainWindow::refreshManifestAsync() {
    if (busy_.load()) return;
    busy_ = true;
    std::thread([this]() {
        std::vector<VersionInfo> out;
        std::wstring err;
        pushLog(L"正在从下载源获取版本清单...");
        bool okFlag = launcher_.fetchManifest(out, err);
        // 磁盘枚举移到锁外, 避免长时间占用主线程绘制所需的锁
        std::vector<std::string> local = launcher_.listInstalled();
        {
            std::lock_guard<std::mutex> lk(mtx_);
            if (okFlag) {
                manifest_ = out;
                for (auto& s : local) {
                    bool found = false;
                    for (auto& v : manifest_) { if (v.id == s) { found = true; break; } }
                    if (!found) { VersionInfo nv; nv.id = s; nv.type = "installed"; manifest_.push_back(nv); }
                }
                filtered_.clear();
                for (int i = 0; i < (int)manifest_.size(); ++i) filtered_.push_back(i);
                pushLogLocked(L"版本清单获取成功，共 " + std::to_wstring(manifest_.size()) + L" 个版本。");
            } else {
                pushLogLocked(L"版本清单获取失败: " + err);
            }
            installedOnly_ = local;
            installedSet_.clear();
            for (auto& s : installedOnly_) installedSet_.insert(s);
            pushLogLocked(L"本地已安装 " + std::to_wstring(installedOnly_.size()) + L" 个版本。");
        }
        manifestLoaded_ = true;
        busy_ = false;
        dirty_ = true;
    }).detach();
}

void MainWindow::installSelectedAsync() {
    std::string id;
    if (page_ == 1) {
        if (!manifestLoaded_.load() || manifest_.empty()) {
            // 用已安装列表无意义
            MessageBoxW(hwnd_, L"版本清单尚未加载完成，请稍候。", L"提示", MB_ICONINFORMATION);
            return;
        }
        std::vector<std::string> ids;
        {
            std::lock_guard<std::mutex> lk(mtx_);
            for (int idx : filtered_) {
                if (idx < 0 || idx >= (int)manifest_.size()) continue;
                const VersionInfo& v = manifest_[idx];
                if (!versionInCategory(verCat_, v.type, v.id, v.releaseTime)) continue;
            if (!versionMatchesSearch(verSearch_, v.id)) continue;
                bool inst = installedSet_.count(v.id) > 0;
                if (showInstalledOnly_ && !inst) continue;
                ids.push_back(v.id);
            }
        }
        if (selList_ < 0 || selList_ >= (int)ids.size()) {
            MessageBoxW(hwnd_, L"请先在列表中选择一个版本。", L"提示", MB_ICONINFORMATION);
            return;
        }
        id = ids[selList_];
    } else {
        id = wideToUtf8(selVersion_);
        if (id.empty()) {
            MessageBoxW(hwnd_, L"请先选择游戏版本。", L"提示", MB_ICONINFORMATION);
            return;
        }
    }

    // 找到 VersionInfo
    VersionInfo vi;
    bool found = false;
    for (auto& v : manifest_) if (v.id == id) { vi = v; found = true; break; }
    if (!found) {
        vi.id = id;
        vi.type = "release";
        vi.url = launcher_.opt.useMirror
            ? "https://bmclapi2.bangbang93.com/version/" + id + "/json"
            : "https://launchermeta.mojang.com/version/" + id + "/json";
    }

    saveSettings();
    selVersion_ = utf8ToWide(id);
    busy_ = true;
    launcher_.cancelled = false;
    std::thread([this, vi]() {
        std::wstring err;
        pushLog(L"================ 开始安装 " + utf8ToWide(vi.id) + L" ================");
        bool okFlag = launcher_.install(vi, err);
        if (okFlag) {
            setStatus(-1, L"安装完成");
            pushLog(L"安装完成: " + utf8ToWide(vi.id));
            std::lock_guard<std::mutex> lk(mtx_);
            installedOnly_ = launcher_.listInstalled();
            installedSet_.clear();
            for (auto& s : installedOnly_) installedSet_.insert(s);
        } else {
            setStatus(-1, L"安装失败: " + err);
            pushLog(L"安装失败: " + err);
        }
        busy_ = false;
        dirty_ = true;
    }).detach();
}

static std::string baseMcVersionFromId(const std::string& id) {
    if (id.rfind("fabric-loader-", 0) == 0 || id.rfind("quilt-loader-", 0) == 0) {
        size_t p = id.rfind('-');
        if (p != std::string::npos && p + 1 < id.size()) return id.substr(p + 1);
    }
    size_t f = id.find("-forge-");
    if (f != std::string::npos) return id.substr(0, f);
    if (id.rfind("neoforge-", 0) == 0) {
        int major = 0, minor = 0;
        sscanf(id.c_str() + 9, "%d.%d", &major, &minor);
        if (major > 0) return "1." + std::to_string(major) + "." + std::to_string(minor);
    }
    return id;
}

static int requiredJavaForVersion(const std::string& id) {
    std::string base = baseMcVersionFromId(id);
    int a = 0, b = 0, c = 0;
    sscanf(base.c_str(), "%d.%d.%d", &a, &b, &c);
    if (a <= 1) {
        if (b > 20 || (b == 20 && c >= 5)) return 21;
        if (b >= 18) return 17;
        if (b == 17) return 16;
        return 8;
    }
    return 21;
}
void MainWindow::installForgeAsync() {
    if (busy_.load()) { pushLog(L"当前有任务在执行，请稍候。"); return; }
    std::string mc = wideToUtf8(selVersion_);
    if (mc.empty()) {
        MessageBoxW(hwnd_, L"请先在首页选择一个游戏版本，再安装 Forge。", L"安装 Forge", MB_ICONINFORMATION);
        return;
    }
    if (mc.find("-forge-") != std::string::npos || mc.rfind("fabric-loader-", 0) == 0) {
        MessageBoxW(hwnd_, L"当前选中的已经是模组加载器版本，请先选择原版版本。", L"安装 Forge", MB_ICONINFORMATION);
        return;
    }
    std::wstring mcW = utf8ToWide(mc);
    std::wstring baseDir = joinPath(joinPath(gameDir_, L"versions"), mcW);
    if (!pathExists(joinPath(baseDir, mcW + L".jar"))) {
        MessageBoxW(hwnd_, L"请先安装并完成这个原版版本，再安装 Forge。", L"安装 Forge", MB_ICONWARNING);
        return;
    }
    std::wstring java = javaPath_.empty() ? Launcher::autoDetectJava() : javaPath_;
    if (java.empty()) {
        MessageBoxW(hwnd_, L"没有可用的 Java，请先在游戏设置里选择。", L"安装 Forge", MB_ICONWARNING);
        return;
    }
    busy_ = true;
    pushLog(L"开始安装 Forge: MC " + mcW);
    setStatus(0, L"正在获取 Forge 版本…");
    dirty_ = true;

    std::thread([this, mc, mcW, java]() {
        std::wstring err, fail;
        std::string body;
        std::wstring forgeW;
        if (!httpGetToStringEx(L"https://files.minecraftforge.net/net/minecraftforge/forge/promotions_slim.json",
                               kBrowserUA, body, err)) {
            fail = L"获取 Forge 版本列表失败：" + err;
        } else {
            Json j;
            if (!mjson::parse(body, j) || !j.isObject()) {
                fail = L"Forge promotions 解析失败";
            } else {
                const Json& promos = j.get("promos");
                std::string key = mc + "-recommended";
                forgeW = utf8ToWide(getStr(promos, key.c_str()));
                if (forgeW.empty()) {
                    key = mc + "-latest";
                    forgeW = utf8ToWide(getStr(promos, key.c_str()));
                }
                if (forgeW.empty()) fail = L"没有找到该 MC 版本的 Forge 推荐版本";
            }
        }
        std::wstring jar;
        if (fail.empty()) {
            wchar_t tmp[MAX_PATH] = { 0 };
            GetTempPathW(MAX_PATH, tmp);
            std::wstring name = mcW + L"-forge-" + forgeW;
            std::wstring file = L"forge-" + mcW + L"-" + forgeW + L"-installer.jar";
            jar = joinPath(tmp, file);
            std::wstring url = L"https://maven.minecraftforge.net/net/minecraftforge/forge/" +
                               mcW + L"-" + forgeW + L"/forge-" + mcW + L"-" + forgeW + L"-installer.jar";
            setStatus(20, L"正在下载 Forge 安装器…");
            if (!httpDownloadToFileEx(url, jar, kBrowserUA, [](unsigned long long, unsigned long long) { return true; }, err)) {
                fail = L"下载 Forge 安装器失败：" + err;
            }
        }
        if (fail.empty()) {
            std::wstring lp = joinPath(gameDir_, L"launcher_profiles.json");
            if (!pathExists(lp)) {
                FILE* fp = _wfopen(lp.c_str(), L"wb");
                if (fp) {
                    const char* js = "{\"profiles\":{},\"settings\":{\"language\":\"zh_cn\"},\"version\":3}";
                    fwrite(js, 1, strlen(js), fp);
                    fclose(fp);
                }
            }
            setStatus(40, L"正在运行 Forge 安装器（可能需要 1-2 分钟）…");
            std::wstring cmd = L"\"" + java + L"\" -jar \"" + jar + L"\" --installClient \"" + gameDir_ + L"\"";
            std::vector<wchar_t> cbuf(cmd.begin(), cmd.end());
            cbuf.push_back(0);
            STARTUPINFOW si{};
            si.cb = sizeof(si);
            PROCESS_INFORMATION pi{};
            if (!CreateProcessW(nullptr, cbuf.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                                nullptr, nullptr, &si, &pi)) {
                fail = L"无法启动 Forge 安装器";
            } else {
                DWORD wait = WaitForSingleObject(pi.hProcess, 600000);
                DWORD code = 1;
                GetExitCodeProcess(pi.hProcess, &code);
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
                if (wait == WAIT_TIMEOUT) fail = L"Forge 安装器超时";
                else if (code != 0) fail = L"Forge 安装器退出码 " + std::to_wstring(code);
            }
        }
        if (fail.empty()) {
            std::wstring name = mcW + L"-forge-" + forgeW;
            std::string nid = wideToUtf8(name);
            {
                std::lock_guard<std::mutex> lk(mtx_);
                bool found = false;
                for (auto& v : manifest_) { if (v.id == nid) { found = true; break; } }
                if (!found) {
                    VersionInfo nv;
                    nv.id = nid;
                    nv.type = "installed";
                    manifest_.push_back(nv);
                    filtered_.push_back((int)manifest_.size() - 1);
                }
                installedSet_.insert(nid);
                bool inLocal = false;
                for (auto& s : installedOnly_) { if (s == nid) { inLocal = true; break; } }
                if (!inLocal) installedOnly_.push_back(nid);
            }
            pushLog(L"Forge 安装完成: " + name + L"，请在版本列表选择并启动");
            setStatus(-1, L"Forge 安装完成");
            busy_ = false;
            dirty_ = true;
            if (hwnd_) PostMessageW(hwnd_, WM_TIMER, 0, 0);
            return;
        }
        pushLog(L"Forge 安装失败: " + fail);
        setStatus(-1, L"Forge 安装失败");
        busy_ = false;
        dirty_ = true;
        if (hwnd_) PostMessageW(hwnd_, WM_TIMER, 0, 0);
        MessageBoxW(hwnd_, fail.c_str(), L"安装 Forge 失败", MB_ICONWARNING);
    }).detach();
}
void MainWindow::installFabricAsync() {
    if (busy_.load()) { pushLog(L"当前有任务在执行，请稍候。"); return; }
    std::string mc = wideToUtf8(selVersion_);
    if (mc.empty()) {
        MessageBoxW(hwnd_, L"请先在首页选择一个游戏版本，再安装 Fabric。", L"安装 Fabric", MB_ICONINFORMATION);
        return;
    }
    if (mc.rfind("fabric-loader-", 0) == 0) {
        MessageBoxW(hwnd_, L"当前选中的已经是 Fabric 版本。", L"安装 Fabric", MB_ICONINFORMATION);
        return;
    }
    std::wstring mcW = utf8ToWide(mc);
    std::wstring baseDir = joinPath(joinPath(gameDir_, L"versions"), mcW);
    if (!pathExists(joinPath(baseDir, mcW + L".jar"))) {
        MessageBoxW(hwnd_, L"请先安装并完成这个原版版本，再安装 Fabric。", L"安装 Fabric", MB_ICONWARNING);
        return;
    }
    std::wstring java = javaPath_.empty() ? Launcher::autoDetectJava() : javaPath_;
    if (java.empty()) {
        MessageBoxW(hwnd_, L"没有可用的 Java，请先在游戏设置里选择。", L"安装 Fabric", MB_ICONWARNING);
        return;
    }
    busy_ = true;
    pushLog(L"开始安装 Fabric: MC " + mcW);
    setStatus(0, L"正在获取 Fabric 版本信息…");
    dirty_ = true;

    std::thread([this, mc, mcW, java]() {
        std::wstring err;
        std::string body;
        std::wstring metaUrl = L"https://meta.fabricmc.net/v2/versions/loader/" + mcW;
        std::wstring fail;
        std::wstring loaderW;
        if (!httpGetToStringEx(metaUrl, kBrowserUA, body, err)) {
            fail = L"获取 Fabric loader 列表失败：" + err;
        } else {
            Json j;
            if (!mjson::parse(body, j) || !j.isArray() || j.size() == 0) {
                fail = L"Fabric loader 列表解析失败";
            } else {
                loaderW = utf8ToWide(getStr(j.at(0).get("loader"), "version"));
                if (loaderW.empty()) fail = L"Fabric loader 版本为空";
            }
        }
        std::wstring installerW;
        if (fail.empty()) {
            setStatus(20, L"正在获取 Fabric 安装器版本…");
            std::string xml;
            std::wstring merr;
            if (!httpGetToStringEx(L"https://maven.fabricmc.net/net/fabricmc/fabric-installer/maven-metadata.xml",
                                   kBrowserUA, xml, merr)) {
                fail = L"获取 Fabric 安装器版本失败：" + merr;
            } else {
                size_t a = xml.find("<release>");
                size_t b = xml.find("</release>", a == std::string::npos ? 0 : a);
                if (a == std::string::npos || b == std::string::npos) fail = L"Fabric 安装器版本解析失败";
                else installerW = utf8ToWide(xml.substr(a + 9, b - a - 9));
            }
        }
        std::wstring name = L"fabric-loader-" + loaderW + L"-" + mcW;
        std::wstring jar;
        if (fail.empty()) {
            wchar_t tmp[MAX_PATH] = { 0 };
            GetTempPathW(MAX_PATH, tmp);
            jar = joinPath(tmp, L"fabric-installer-" + installerW + L".jar");
            std::wstring instUrl = L"https://maven.fabricmc.net/net/fabricmc/fabric-installer/" + installerW +
                                   L"/fabric-installer-" + installerW + L".jar";
            setStatus(35, L"正在下载 Fabric 安装器…");
            if (!httpDownloadToFileEx(instUrl, jar, kBrowserUA, [](unsigned long long, unsigned long long) { return true; }, err)) {
                fail = L"下载 Fabric 安装器失败：" + err;
            }
        }
        if (fail.empty()) {
            std::wstring lp = joinPath(gameDir_, L"launcher_profiles.json");
            if (!pathExists(lp)) {
                FILE* fp = _wfopen(lp.c_str(), L"wb");
                if (fp) {
                    const char* js = "{\"profiles\":{},\"settings\":{\"language\":\"zh_cn\"},\"version\":3}";
                    fwrite(js, 1, strlen(js), fp);
                    fclose(fp);
                }
            }
            setStatus(45, L"正在运行 Fabric 安装器…");
            std::wstring cmd = L"\"" + java + L"\" -jar \"" + jar + L"\" client -dir \"" + gameDir_ +
                               L"\" -mcversion \"" + mcW + L"\" -loader \"" + loaderW + L"\" -n \"" + name + L"\"";
            std::vector<wchar_t> cbuf(cmd.begin(), cmd.end());
            cbuf.push_back(0);
            STARTUPINFOW si{};
            si.cb = sizeof(si);
            PROCESS_INFORMATION pi{};
            if (!CreateProcessW(nullptr, cbuf.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW,
                                nullptr, nullptr, &si, &pi)) {
                fail = L"无法启动 Fabric 安装器";
            } else {
                DWORD wait = WaitForSingleObject(pi.hProcess, 300000);
                DWORD code = 1;
                GetExitCodeProcess(pi.hProcess, &code);
                CloseHandle(pi.hThread);
                CloseHandle(pi.hProcess);
                if (wait == WAIT_TIMEOUT) fail = L"Fabric 安装器超时";
                else if (code != 0) fail = L"Fabric 安装器退出码 " + std::to_wstring(code);
            }
        }
        if (fail.empty()) {
            std::string nid = wideToUtf8(name);
            {
                std::lock_guard<std::mutex> lk(mtx_);
                bool found = false;
                for (auto& v : manifest_) { if (v.id == nid) { found = true; break; } }
                if (!found) {
                    VersionInfo nv;
                    nv.id = nid;
                    nv.type = "installed";
                    manifest_.push_back(nv);
                    filtered_.push_back((int)manifest_.size() - 1);
                }
                installedSet_.insert(nid);
                bool inLocal = false;
                for (auto& s : installedOnly_) { if (s == nid) { inLocal = true; break; } }
                if (!inLocal) installedOnly_.push_back(nid);
            }
            pushLog(L"Fabric 安装完成: " + name + L"，请在版本列表选择并启动");
            busy_ = false;
            dirty_ = true;
            if (hwnd_) PostMessageW(hwnd_, WM_TIMER, 0, 0);
            return;
        }
        pushLog(L"Fabric 安装失败: " + fail);
        busy_ = false;
        dirty_ = true;
        if (hwnd_) PostMessageW(hwnd_, WM_TIMER, 0, 0);
        MessageBoxW(hwnd_, fail.c_str(), L"安装 Fabric 失败", MB_ICONWARNING);
    }).detach();
}
void MainWindow::launchSelectedAsync() {
    std::string id;
    if (page_ == 1) {
        std::vector<std::string> ids;
        {
            std::lock_guard<std::mutex> lk(mtx_);
            if (manifestLoaded_.load() && !manifest_.empty()) {
                for (int idx : filtered_) {
                    if (idx < 0 || idx >= (int)manifest_.size()) continue;
                    const VersionInfo& v = manifest_[idx];
                    if (!versionInCategory(verCat_, v.type, v.id, v.releaseTime)) continue;
                    if (!versionMatchesSearch(verSearch_, v.id)) continue;
                    bool inst = installedSet_.count(v.id) > 0;
                    if (showInstalledOnly_ && !inst) continue;
                    ids.push_back(v.id);
                }
            } else {
                ids = installedOnly_;
            }
        }
        if (selList_ >= 0 && selList_ < (int)ids.size()) id = ids[selList_];
    } else {
        id = wideToUtf8(selVersion_);
    }
    if (id.empty()) {
        MessageBoxW(hwnd_, L"请先选择要启动的版本。", L"提示", MB_ICONINFORMATION);
        return;
    }
    {
        int req = requiredJavaForVersion(id);
        int have = javaPath_.empty() ? 0 : Launcher::queryJavaMajor(javaPath_);
        if (have == 0) {
            MessageBoxW(hwnd_, L"未检测到可用的 Java。\n\n请到「游戏设置」点「扫描」，或手动选择 java.exe。",
                        L"MCL 启动", MB_ICONWARNING);
            return;
        }
        if (have < req) {
            std::wstring msg = L"版本 " + utf8ToWide(id) + L" 需要 Java " + std::to_wstring(req) +
                               L"，当前选择的是 Java " + std::to_wstring(have) +
                               L"。\n\n请到「游戏设置」点「扫描」并选用匹配的 Java。";
            MessageBoxW(hwnd_, msg.c_str(), L"MCL 启动", MB_ICONWARNING);
            return;
        }
    }
    // 读昵称
    wchar_t buf[256] = { 0 };
    GetWindowTextW(editName_, buf, 256);
    playerName_ = trimW(buf);
    if (playerName_.empty()) playerName_ = L"Steve";

    selVersion_ = utf8ToWide(id);
    applySettingsToLauncher();
    saveSettings();

    busy_ = true;
    launcher_.cancelled = false;
    std::thread([this, id]() {
        std::wstring err;
        pushLog(L"================ 启动 " + utf8ToWide(id) + L" ================");
        bool okFlag = launcher_.launch(id, err);
        if (okFlag) {
            setStatus(-1, L"游戏已启动");
            pushLog(L"游戏进程已创建。");
        } else {
            setStatus(-1, L"启动失败: " + err);
            pushLog(L"启动失败: " + err);
        }
        busy_ = false;
        dirty_ = true;
    }).detach();
}

// ===========================================================================
//  模组下载页: 页面入口 / 路径辅助
// ===========================================================================

void MainWindow::openModPage() {
    page_ = 4;
    modDetail_ = -1;
    layoutControls();
    dirty_ = true;
}

// <游戏目录>\.minecraft\mods
std::wstring MainWindow::modsDir() const {
    return joinPath(activeGameDir(), L"mods");
}

// 扫描 <游戏目录>\.minecraft\versions 下的本地版本, 用于填充"目标 MC 版本"
int MainWindow::collectGameVersions(std::vector<std::wstring>& out) const {
    out.clear();
    std::wstring gd = gameDir_;
    if (gd.empty()) gd = joinPath(exeDir(), L".minecraft");
    std::wstring dir = joinPath(gd, L"versions");
    WIN32_FIND_DATAW fd{};
    HANDLE h = FindFirstFileW((dir + L"\\*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return 0;
    do {
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) continue;
        if (fd.cFileName[0] == L'.') continue;
        out.push_back(fd.cFileName);
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    std::sort(out.begin(), out.end(), std::greater<std::wstring>());
    return (int)out.size();
}

static std::wstring sanitizeFileName(const std::wstring& s) {
    std::wstring out;
    for (wchar_t c : s) {
        if (c == L'\\' || c == L'/' || c == L':' || c == L'*' || c == L'?' ||
            c == L'"' || c == L'<' || c == L'>' || c == L'|') out += L'_';
        else out += c;
    }
    if (out.empty()) out = L"mod.jar";
    return out;
}

// ===========================================================================
//  封面缓存 (后台下载字节, UI 线程解码)
// ===========================================================================

void MainWindow::ensureCover(const std::wstring& url) {
    if (url.empty()) return;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (covers_.bytes.count(url) || covers_.pending.count(url) || covers_.failed.count(url)) return;
        covers_.pending.insert(url);
    }
    std::thread([this, url]() {
        apiThrottle(150);
        std::string bytes; std::wstring err;
        bool ok = httpGetToStringEx(url, kBrowserUA, bytes, err);
        {
            std::lock_guard<std::mutex> lk(mtx_);
            covers_.pending.erase(url);
            if (ok && !bytes.empty()) covers_.bytes[url] = bytes;
            else covers_.failed.insert(url);
        }
        dirty_ = true;
    }).detach();
}

Gdiplus::Image* MainWindow::coverImage(const std::wstring& url) {
    if (url.empty()) return nullptr;
    auto it = covers_.imgs.find(url);
    if (it != covers_.imgs.end()) return it->second;

    std::string bytes;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        auto b = covers_.bytes.find(url);
        if (b == covers_.bytes.end()) { bytes.clear(); }
        else bytes = b->second;
    }
    if (bytes.empty()) { ensureCover(url); return nullptr; }

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes.size());
    if (!hMem) return nullptr;
    void* p = GlobalLock(hMem);
    if (!p) { GlobalFree(hMem); return nullptr; }
    memcpy(p, bytes.data(), bytes.size());
    GlobalUnlock(hMem);

    IStream* st = nullptr;
    if (CreateStreamOnHGlobal(hMem, TRUE, &st) != S_OK || !st) { GlobalFree(hMem); return nullptr; }
    Gdiplus::Image* img = Gdiplus::Image::FromStream(st);
    st->Release();
    if (!img || img->GetLastStatus() != Gdiplus::Ok) {
        delete img;
        covers_.imgs[url] = nullptr;
        return nullptr;
    }
    covers_.imgs[url] = img;
    return img;
}

// ===========================================================================
//  后台任务: MC 百科检索
// ===========================================================================

void MainWindow::searchModsAsync(const std::wstring& keyword) {
    if (modsLoading_) return;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        modsLoading_ = true;
        modsItems_.clear();
        modsSel_ = -1;
        modsScroll_ = 0;
        modsErr_.clear();
        covers_.imgs.clear();          // 列表换了一批, 释放解码缓存 (原始字节保留)
    }
    listCoverFetched_ = 0;
    modsDirty_ = true;
    dirty_ = true;

    std::string kw8 = wideToUtf8(keyword);
    std::thread([this, kw8]() {
        apiThrottle(400);
        std::wstring url = std::wstring(kMcmodSearchApi) + utf8ToWide(urlEncode(kw8));
        std::string body; std::wstring err;
        bool ok = httpGetToStringEx(url, kBrowserUA, body, err);

        std::vector<McModItem> items;
        std::wstring fail;
        if (!ok) {
            fail = L"检索失败：" + err;
        } else {
            Json j;
            if (!mjson::parse(body, j) || !j.isArray()) {
                fail = L"检索结果解析失败 (接口返回异常)";
            } else {
                for (size_t i = 0; i < j.size() && items.size() < 60; ++i) {
                    const Json& e = j.at(i);
                    const Json& d = e.get("data");
                    McModItem it;
                    it.id = getStr(d, "mcmod_id");
                    if (it.id.empty()) continue;
                    it.name = utf8ToWide(getStr(d, "chinese_name"));
                    it.enName = utf8ToWide(getStr(d, "sub_name"));
                    it.abbr = utf8ToWide(getStr(d, "abbr"));
                    if (it.name.empty()) it.name = utf8ToWide(getStr(e, "title"));
                    it.category = categoryNameW(getStr(d, "category"));
                    it.desc = oneLine(utf8ToWide(getStr(e, "description")), 150);
                    it.address = utf8ToWide(getStr(e, "address"));
                    items.push_back(it);
                }
                if (items.empty()) fail = L"没有找到匹配的模组，换个关键词试试";
            }
        }
        {
            std::lock_guard<std::mutex> lk(mtx_);
            modsItems_.swap(items);
            modsErr_ = fail;
            modsLoading_ = false;
        }
        modsDirty_ = true;
        dirty_ = true;
    }).detach();
}

// 结果列表封面补全 (前 12 条, 顺序 + 节流)
void MainWindow::fetchCoversAsync() {
    if (listCoversRunning_) return;
    std::vector<std::pair<int, std::wstring>> todo;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        int n = (int)modsItems_.size();
        if (n > 12) n = 12;
        int from = listCoverFetched_ < 0 ? 0 : listCoverFetched_;
        for (int i = from; i < n; ++i) {
            if (modsItems_[i].cover.empty()) todo.push_back(std::make_pair(i, utf8ToWide(modsItems_[i].id)));
        }
        if (n > listCoverFetched_) listCoverFetched_ = n;
    }
    if (todo.empty()) return;

    listCoversRunning_ = true;
    std::thread([this, todo]() {
        for (size_t k = 0; k < todo.size(); ++k) {
            int idx = todo[k].first;
            std::wstring id = todo[k].second;
            std::wstring coverUrl;
            {
                std::lock_guard<std::mutex> lk(mtx_);
                if (idx < (int)modsItems_.size()) coverUrl = modsItems_[idx].cover;
            }
            if (coverUrl.empty()) {
                apiThrottle(300);
                std::string body; std::wstring err;
                if (httpGetToStringEx(std::wstring(kMcmodDetailApi) + id, kBrowserUA, body, err)) {
                    Json j;
                    if (mjson::parse(body, j) && j.isObject())
                        coverUrl = utf8ToWide(getStr(j, "cover_image"));
                }
            }
            if (coverUrl.empty()) continue;
            {
                std::lock_guard<std::mutex> lk(mtx_);
                if (idx < (int)modsItems_.size()) modsItems_[idx].cover = coverUrl;
            }
            ensureCover(coverUrl);
            dirty_ = true;
        }
        listCoversRunning_ = false;
        dirty_ = true;
    }).detach();
}

// ===========================================================================
//  后台任务: MC 百科详情
// ===========================================================================

void MainWindow::ensureDetailAsync(int idx) {
    McModItem it;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (idx < 0 || idx >= (int)modsItems_.size()) return;
        it = modsItems_[idx];
    }

    modDetail_ = idx;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        detail_ = McModDetail();
        detail_.id = it.id;
        detail_.title = it.name;
        detail_.subtitle = it.enName;
        detail_.address = it.address;
        detail_.cover = it.cover;
        detailLoading_ = true;
        detailReady_ = false;
        detailErr_.clear();
        detailMcver_.clear();
        detailLoader_ = -1;
        modFiles_.clear();
        filesLoading_ = false;
        filesLoaded_ = false;
        filesErr_.clear();
        detailLinkCount_ = 0;
    }
    layoutControls();
    dirty_ = true;

    std::string id = it.id;
    std::wstring url = std::wstring(kMcmodDetailApi) + utf8ToWide(id);
    std::thread([this, url, idx, it]() {
        apiThrottle(300);
        std::string body; std::wstring err;
        bool ok = httpGetToStringEx(url, kBrowserUA, body, err);

        McModDetail d;
        std::wstring fail;
        if (!ok) {
            fail = L"详情加载失败：" + err;
        } else {
            Json j;
            if (!mjson::parse(body, j) || !j.isObject()) {
                fail = L"详情解析失败 (接口返回异常)";
            } else {
                d.id = it.id;
                d.title = utf8ToWide(getStr(j, "title"));
                d.subtitle = utf8ToWide(getStr(j, "subtitle"));
                d.shortName = utf8ToWide(getStr(j, "short_name"));
                d.cover = utf8ToWide(getStr(j, "cover_image"));
                d.environment = utf8ToWide(getStr(j, "operating_environment"));
                d.recorded = utf8ToWide(getStr(j, "recorded_time"));
                d.lastEdit = utf8ToWide(getStr(j, "last_edit_time"));
                d.address = it.address;
                if (d.title.empty()) d.title = it.name;
                if (d.subtitle.empty()) d.subtitle = it.enName;

                // supported_versions: { "1.20.1": [...], ... }
                const Json& sv = j.get("supported_versions");
                if (sv.isObject()) {
                    std::wstring vs;
                    for (size_t i = 0; i < sv.size(); ++i) {
                        if (!vs.empty()) vs += L", ";
                        vs += utf8ToWide(sv.obj[i].first);
                    }
                    d.mcVersions = vs;
                }
                // authors: [{name, position}]
                const Json& au = j.get("authors");
                if (au.isArray()) {
                    std::wstring as;
                    for (size_t i = 0; i < au.size() && i < 8; ++i) {
                        std::wstring nm = utf8ToWide(getStr(au.at(i), "name"));
                        if (nm.empty()) continue;
                        if (!as.empty()) as += L"、";
                        as += nm;
                    }
                    d.authors = as;
                }
                // related_links: [{text, url}]
                const Json& rl = j.get("related_links");
                if (rl.isArray()) {
                    for (size_t i = 0; i < rl.size(); ++i) {
                        std::wstring text = utf8ToWide(getStr(rl.at(i), "text"));
                        std::wstring u = utf8ToWide(getStr(rl.at(i), "url"));
                        if (u.empty()) continue;
                        d.links.push_back(std::make_pair(text, u));
                        if (d.slug.empty()) {
                            std::string slug = parseModrinthSlug(getStr(rl.at(i), "url"));
                            if (!slug.empty()) d.slug = utf8ToWide(slug);
                        }
                    }
                }
            }
        }

        {
            std::lock_guard<std::mutex> lk(mtx_);
            if (modDetail_ != idx) { dirty_ = true; return; }
            detail_ = d;
            detailReady_ = fail.empty();
            detailErr_ = fail;
            detailLoading_ = false;
        }
        dirty_ = true;
    }).detach();
}

// 详情缺少 Modrinth 关联链接时, 用 Modrinth 搜索兜底解析 slug
void MainWindow::resolveModrinthBySearchAsync() {
    if (mrSearchRunning_) return;
    std::wstring kw;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (detail_.slug.empty() == false) return;
        kw = detail_.subtitle.empty() ? detail_.title : detail_.subtitle;
    }
    if (kw.empty()) return;
    mrSearchRunning_ = true;
    dirty_ = true;

    std::string q = wideToUtf8(kw);
    std::thread([this, q]() {
        apiThrottle(300);
        std::wstring url = std::wstring(kModrinthApi) + L"search?query=" + utf8ToWide(urlEncode(q)) +
                           L"&limit=1&facets=" + utf8ToWide(urlEncodeAll("[[\"project_type:mod\"]]"));
        std::string body; std::wstring err;
        std::wstring slug;
        if (httpGetToStringEx(url, kBrowserUA, body, err)) {
            Json j;
            if (mjson::parse(body, j) && j.isObject()) {
                const Json& hits = j.get("hits");
                if (hits.isArray() && hits.size() > 0)
                    slug = utf8ToWide(getStr(hits.at(0), "slug"));
            }
        }
        {
            std::lock_guard<std::mutex> lk(mtx_);
            if (!slug.empty()) {
                detail_.slug = slug;
                filesLoaded_ = false;
            }
        }
        mrSearchRunning_ = false;
        dirty_ = true;
    }).detach();
}

// ===========================================================================
//  后台任务: Modrinth 可用版本文件
// ===========================================================================

void MainWindow::fetchModFilesAsync() {
    std::string slug;
    int loader = 0;
    std::wstring mcver;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        slug = wideToUtf8(detail_.slug);
        loader = detailLoader_;
        if (loader < 0 || loader >= kLoaderCount) loader = 0;
        detailLoader_ = loader;
        mcver = detailMcver_;
    }
    if (slug.empty()) {
        std::lock_guard<std::mutex> lk(mtx_);
        modFiles_.clear();
        filesLoaded_ = false;
        filesErr_ = L"当前模组未解析到 Modrinth 项目，无法列出可下载版本。\n可在详情页点击「用 Modrinth 搜索兜底解析」，或前往相关链接手动下载。";
        dirty_ = true;
        return;
    }
    if (mcver.empty() && editModMcVer_) {
        wchar_t buf[64] = { 0 };
        GetWindowTextW(editModMcVer_, buf, 64);
        mcver = trimW(std::wstring(buf));
    }
    if (mcver.empty()) {
        std::vector<std::wstring> vs;
        if (collectGameVersions(vs) > 0) mcver = vs[0];
    }
    if (mcver.empty()) mcver = L"1.20.1";

    {
        std::lock_guard<std::mutex> lk(mtx_);
        filesLoading_ = true;
        filesLoaded_ = false;
        filesErr_.clear();
        modFiles_.clear();
        detailMcver_ = mcver;
    }
    dirty_ = true;

    std::string ldr = kLoaderIds[loader];
    std::thread([this, slug, ldr, mcver]() {
        apiThrottle(300);
        std::string q = "game_versions=" + urlEncodeAll("[\"" + wideToUtf8(mcver) + "\"]") +
                        "&loaders=" + urlEncodeAll("[\"" + ldr + "\"]");
        std::wstring url = std::wstring(kModrinthApi) + L"project/" + utf8ToWide(slug) +
                           L"/version?" + utf8ToWide(q);
        std::string body; std::wstring err;
        bool ok = httpGetToStringEx(url, kBrowserUA, body, err);

        std::vector<ModFile> files;
        std::wstring fail;
        if (!ok) {
            fail = L"Modrinth 版本获取失败：" + err;
        } else {
            Json j;
            if (!mjson::parse(body, j) || !j.isArray()) {
                fail = L"Modrinth 版本解析失败 (接口返回异常)";
            } else if (j.size() == 0) {
                fail = L"该 MC 版本 / 加载器组合下没有可用文件，换个版本或加载器试试";
            } else {
                for (size_t i = 0; i < j.size() && files.size() < 50; ++i) {
                    const Json& v = j.at(i);
                    ModFile f;
                    f.ver = utf8ToWide(getStr(v, "version_number"));
                    const Json& ld = v.get("loaders");
                    if (ld.isArray()) {
                        std::wstring ls;
                        for (size_t k = 0; k < ld.size(); ++k) {
                            std::wstring t = utf8ToWide(ld.at(k).asString());
                            if (t.empty()) continue;
                            if (!ls.empty()) ls += L"/";
                            ls += t;
                        }
                        f.loaders = ls;
                    }
                    const Json& fs = v.get("files");
                    if (fs.isArray() && fs.size() > 0) {
                        int pick = 0;
                        for (size_t k = 0; k < fs.size(); ++k) {
                            if (fs.at(k).get("primary").asBool(false)) { pick = (int)k; break; }
                        }
                        f.name = utf8ToWide(getStr(fs.at(pick), "filename"));
                        f.url = utf8ToWide(getStr(fs.at(pick), "url"));
                        f.size = (unsigned long long)fs.at(pick).get("size").asNumber(0);
                    }
                    if (f.url.empty()) continue;
                    files.push_back(f);
                }
                if (files.empty()) fail = L"没有解析到可下载的 jar 文件";
            }
        }
        {
            std::lock_guard<std::mutex> lk(mtx_);
            modFiles_.swap(files);
            filesErr_ = fail;
            filesLoading_ = false;
            filesLoaded_ = fail.empty();
        }
        dirty_ = true;
    }).detach();
}

// ===========================================================================
//  后台任务: 下载 jar 到 <游戏目录>\.minecraft\mods
// ===========================================================================

void MainWindow::startModDownload(const ModFile& mf) {
    if (dlActive_.load()) return;
    if (mf.url.empty()) return;

    std::wstring dir = modsDir();
    createDirs(dir);
    std::wstring file = joinPath(dir, sanitizeFileName(mf.name.empty() ? L"mod.jar" : mf.name));

    dlActive_ = true;
    dlPct_ = 0;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        dlName_ = mf.name;
        dlResult_ = L"正在下载…";
    }
    dirty_ = true;

    std::wstring url = mf.url;
    std::wstring label = mf.name;
    std::thread([this, url, file, label]() {
        std::wstring err;
        bool ok = httpDownloadToFileEx(url, file, kBrowserUA,
            [this](unsigned long long done, unsigned long long total) {
                if (total > 0) dlPct_ = (int)(done * 100 / total);
                return true;
            }, err);
        dlPct_ = ok ? 100 : dlPct_.load();
        {
            std::lock_guard<std::mutex> lk(mtx_);
            dlResult_ = ok ? (L"已下载到 mods：" + label) : (L"下载失败：" + err);
        }
        dlActive_ = false;
        dirty_ = true;
        PostMessageW(hwnd_, WM_TIMER, 0, 0);
    }).detach();
}

// ===========================================================================
//  绘制: 模组检索列表页
// ===========================================================================

void MainWindow::drawMods(Graphics& g, int W, int H) {
    FontFamily ff(L"Microsoft YaHei");
    Font fL(&ff, 12.0f, FontStyleRegular, UnitPixel);
    Font fT(&ff, 14.0f, FontStyleBold, UnitPixel);
    Font fS(&ff, 12.0f, FontStyleRegular, UnitPixel);

    int cx = SIDEBAR_W + 24;
    int cw = W - SIDEBAR_W - 48;
    int y = HEADER_H + 16;

    // --- 检索条 (输入框为原生 EDIT, 绘制其右侧按钮与标签)
    RECT card = mkRect(cx, y, cx + cw, y + 50);
    drawCard(g, card);
    RECT bq = mkRect(card.right - 390, y + 10, card.right - 310, y + 40);
    drawButton(g, bq, L"检索", BTN_MODS_QUERY, true, !modsLoading_, 13);
    RECT lb = mkRect(card.right - 306, y + 10, card.right - 218, y + 40);
    drawText(g, L"MC 版本", fL, kTextDim, lb, 2);

    // --- 快照
    bool loading = false;
    std::wstring err;
    std::vector<McModItem> items;
    int sel = -1;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        loading = modsLoading_;
        err = modsErr_;
        items = modsItems_;
        sel = modsSel_;
    }
    int total = (int)items.size();

    // 新一轮检索完成后, 在 UI 线程顺序补全列表封面
    if (modsDirty_.exchange(false)) fetchCoversAsync();

    // --- 结果列表区
    int ly = y + 62;
    RECT area = mkRect(cx, ly, cx + cw, H - 16);
    listArea_ = area;

    if (loading || total == 0) {
        std::wstring msg;
        if (loading) msg = L"正在从 MC 百科检索…";
        else if (!err.empty()) msg = err;
        else msg = L"输入关键词后点击「检索」，可从 MC 百科查找模组";
        RECT mr = mkRect(area.left, area.top, area.right, area.bottom);
        drawText(g, msg, fT, kTextDim, mr, 1, false);
        return;
    }

    // 结果数量 / 加载中提示
    std::wstring tip = L"共 " + std::to_wstring(total) + L" 条结果" +
                       (loading ? L"  ·  更新中…" : L"") +
                       (err.empty() ? L"" : (L"  ·  " + err));
    RECT tr = mkRect(area.left, area.top, area.right, area.top + 22);
    drawText(g, tip, fL, kTextDim, tr, 0);

    int listTop = area.top + 26;
    const int rowH = 66;
    int viewRows = (area.bottom - listTop) / rowH;
    if (viewRows < 1) viewRows = 1;
    int maxScroll = total - viewRows;
    if (maxScroll < 0) maxScroll = 0;
    if (modsScroll_ > maxScroll) modsScroll_ = maxScroll;
    if (modsScroll_ < 0) modsScroll_ = 0;

    for (int i = 0; i < viewRows; ++i) {
        int ri = modsScroll_ + i;
        if (ri >= total) break;
        const McModItem& it = items[ri];
        int ry = listTop + i * rowH;
        RECT rr = mkRect(area.left, ry, area.right, ry + rowH - 6);

        bool hover = (hoverId_ == BTN_MOD_ROW_BASE + ri);
        bool selected = (sel == ri);
        fillRound(g, rr, 8, selected ? kSelBg : (hover ? kHover : kCardBg));
        strokeRound(g, rr, 8, selected ? kAccent : kLine);
        hits_.push_back({ rr, BTN_MOD_ROW_BASE + ri });

        // 封面
        RECT cov = mkRect(rr.left + 10, rr.top + 10, rr.left + 56, rr.top + 56);
        fillRound(g, cov, 6, C(238, 240, 243));
        Image* img = coverImage(it.cover);
        if (img) {
            GraphicsPath cp;
            int dd = 12;
            cp.AddArc(cov.left, cov.top, dd, dd, 180, 90);
            cp.AddArc(cov.right - dd, cov.top, dd, dd, 270, 90);
            cp.AddArc(cov.right - dd, cov.bottom - dd, dd, dd, 0, 90);
            cp.AddArc(cov.left, cov.bottom - dd, dd, dd, 90, 90);
            cp.CloseFigure();
            g.SetClip(&cp);
            drawCoverImage(g, cov, img);
            g.ResetClip();
        } else {
            drawText(g, L"图", fS, C(180, 186, 194), cov, 1, false);
        }

        // 名称行
        std::wstring nm = it.name;
        if (!it.enName.empty()) nm += L"  ·  " + it.enName;
        if (!it.abbr.empty()) nm = L"[" + it.abbr + L"] " + nm;
        RECT nr = mkRect(rr.left + 66, rr.top + 6, rr.right - 16, rr.top + 32);
        drawText(g, nm, fT, kText, nr, 0);

        // 分类标签
        if (!it.category.empty()) {
            RECT cr = mkRect(rr.left + 66, rr.top + 34, rr.right - 16, rr.top + 52);
            Font fs(&ff, 11.0f, FontStyleRegular, UnitPixel);
            int tw = (int)it.category.size() * 12 + 18;
            if (tw > nr.right - nr.left) tw = nr.right - nr.left;
            RECT chip = mkRect(cr.left, cr.top, cr.left + tw, cr.bottom);
            fillRound(g, chip, 5, C(232, 244, 227));
            drawText(g, it.category, fs, kAccent, chip, 1, false);
            RECT dr = mkRect(chip.right + 8, cr.top, rr.right - 16, cr.bottom);
            drawText(g, it.desc, fS, kTextDim, dr, 0);
        } else {
            RECT dr = mkRect(rr.left + 66, rr.top + 34, rr.right - 16, rr.top + 56);
            drawText(g, it.desc, fS, kTextDim, dr, 0);
        }
    }

    // 滚动条
    if (maxScroll > 0) {
        int trackTop = listTop + 4;
        int trackH = viewRows * rowH - 12;
        int thumbH = trackH * viewRows / total;
        if (thumbH < 24) thumbH = 24;
        int thumbY = trackTop + (trackH - thumbH) * modsScroll_ / maxScroll;
        RECT tb = mkRect(area.right - 6, thumbY, area.right - 2, thumbY + thumbH);
        fillRound(g, tb, 2, C(200, 205, 212));
    }

    // 详情页还在加载时, 列表页顶部不显示其它内容
    (void)fS;
}

// ===========================================================================
//  绘制: 模组详情页
// ===========================================================================

void MainWindow::drawModDetail(Graphics& g, int W, int H) {
    FontFamily ff(L"Microsoft YaHei");
    Font fL(&ff, 12.0f, FontStyleRegular, UnitPixel);
    Font fT(&ff, 18.0f, FontStyleBold, UnitPixel);
    Font fB(&ff, 13.0f, FontStyleRegular, UnitPixel);
    Font fS(&ff, 12.0f, FontStyleRegular, UnitPixel);

    int cx = SIDEBAR_W + 24;
    int cw = W - SIDEBAR_W - 48;
    int y = HEADER_H + 16;

    // --- 返回按钮
    RECT back = mkRect(cx, y, cx + 96, y + 34);
    drawButton(g, back, L"← 返回列表", BTN_MODS_BACK, false, true, 12);

    // --- 快照
    McModDetail d;
    bool loading = false, ready = false;
    std::wstring derr;
    std::vector<ModFile> files;
    bool fLoading = false, fLoaded = false;
    std::wstring fErr;
    int loader = -1;
    std::wstring mcver, dlResult;
    bool dlActive = false;
    int dlPct = 0;
    bool mrSearching = false;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        d = detail_;
        loading = detailLoading_;
        ready = detailReady_;
        derr = detailErr_;
        files = modFiles_;
        fLoading = filesLoading_;
        fLoaded = filesLoaded_;
        fErr = filesErr_;
        loader = detailLoader_;
        mcver = detailMcver_;
        dlResult = dlResult_;
        dlActive = dlActive_.load();
        dlPct = dlPct_.load();
        mrSearching = mrSearchRunning_;
    }

    int y0 = y + 34 + 12;

    // --- 标题卡 (封面 + 名称)
    RECT head = mkRect(cx, y0, cx + cw, y0 + 96);
    drawCard(g, head);
    RECT cov = mkRect(head.left + 14, head.top + 14, head.left + 82, head.top + 82);
    fillRound(g, cov, 8, C(238, 240, 243));
    Image* img = coverImage(d.cover);
    if (img) {
        GraphicsPath cp;
        int dd = 16;
        cp.AddArc(cov.left, cov.top, dd, dd, 180, 90);
        cp.AddArc(cov.right - dd, cov.top, dd, dd, 270, 90);
        cp.AddArc(cov.right - dd, cov.bottom - dd, dd, dd, 0, 90);
        cp.AddArc(cov.left, cov.bottom - dd, dd, dd, 90, 90);
        cp.CloseFigure();
        g.SetClip(&cp);
        drawCoverImage(g, cov, img);
        g.ResetClip();
    }
    std::wstring title = d.title.empty() ? L"模组详情" : d.title;
    if (!d.shortName.empty() && d.shortName != d.title) title = L"[" + d.shortName + L"] " + title;
    RECT t1 = mkRect(head.left + 94, head.top + 10, head.right - 16, head.top + 40);
    drawText(g, title, fT, kText, t1, 0);
    std::wstring sub = d.subtitle;
    if (!d.authors.empty()) sub += (sub.empty() ? L"" : L"   |   ") + std::wstring(L"作者：") + d.authors;
    RECT t2 = mkRect(head.left + 94, head.top + 40, head.right - 16, head.top + 62);
    drawText(g, sub, fB, kTextDim, t2, 0);
    std::wstring env = d.environment;
    if (!d.mcVersions.empty()) env += (env.empty() ? L"" : L"   |   ") + std::wstring(L"支持版本：") + oneLine(d.mcVersions, 90);
    RECT t3 = mkRect(head.left + 94, head.top + 62, head.right - 16, head.top + 86);
    drawText(g, env, fS, kTextDim, t3, 0);

    RECT bpage = mkRect(head.right - 236, y0 + 104, head.right - 128, y0 + 134);
    drawButton(g, bpage, L"打开百科页", BTN_MODS_OPEN_PAGE, false, !d.address.empty(), 12);
    RECT bmr = mkRect(head.right - 120, y0 + 104, head.right - 12, y0 + 134);
    drawButton(g, bmr, L"打开 Modrinth", BTN_MODS_OPEN_MR, false, !d.slug.empty(), 12);

    int y1 = y0 + 96 + 12;

    // --- 状态: 加载中 / 失败
    if (loading || !ready) {
        std::wstring msg = loading ? std::wstring(L"正在加载模组详情…") : derr;
        if (msg.empty()) msg = L"详情未能加载";
        RECT mr = mkRect(cx, y1, cx + cw, y1 + 60);
        drawText(g, msg, fB, kTextDim, mr, 1, false);
        return;
    }

    // --- 相关链接
    int y2 = y1;
    detailLinkCount_ = 0;
    if (!d.links.empty()) {
        RECT lc = mkRect(cx, y2, cx + cw, y2 + 30);
        drawText(g, L"相关链接", fT, kText, lc, 0);
        y2 += 30;
        int bx = cx;
        int shown = 0;
        for (size_t i = 0; i < d.links.size() && shown < 40; ++i) {
            std::wstring t = d.links[i].first;
            if (t.empty()) t = d.links[i].second;
            Font fs(&ff, 12.0f, FontStyleRegular, UnitPixel);
            int tw = 0;
            for (wchar_t ch : t) tw += ((unsigned)ch > 127 ? 13 : 7);
            tw += 24;
            if (tw > 320) tw = 320;
            if (bx + tw > cx + cw) { bx = cx; y2 += 38; }
            RECT br = mkRect(bx, y2, bx + tw, y2 + 30);
            bool hover = (hoverId_ == BTN_MOD_LINK_BASE + (int)i);
            fillRound(g, br, 6, hover ? kHover : kCardBg);
            strokeRound(g, br, 6, kLine);
            drawText(g, t, fs, kText, br, 1);
            hits_.push_back({ br, BTN_MOD_LINK_BASE + (int)i });
            if (detailLinkCount_ < 100) {
                detailLinkRects_[detailLinkCount_] = br;
                detailLinkIds_[detailLinkCount_] = (int)i;
                ++detailLinkCount_;
            }
            bx += tw + 8;
            ++shown;
        }
        y2 += 38 + 4;
    }

    // --- 下载区
    RECT dc = mkRect(cx, y2, cx + cw, H - 16);
    drawCard(g, dc);

    int iy = dc.top + 12;
    drawText(g, L"从 Modrinth 下载", fT, kText, mkRect(dc.left + 16, iy, dc.right - 16, iy + 26), 0);
    iy += 32;

    if (d.slug.empty()) {
        std::wstring msg1 = mrSearching ? L"正在通过 Modrinth 搜索兜底解析项目…"
                                        : L"未在该模组的百科相关链接中找到 Modrinth 项目。";
        drawText(g, msg1, fB, kTextDim, mkRect(dc.left + 16, iy, dc.right - 16, iy + 24), 0);
        RECT b = mkRect(dc.left + 16, iy + 30, dc.left + 236, iy + 60);
        drawButton(g, b, mrSearching ? L"正在解析…" : L"用 Modrinth 搜索兜底解析", BTN_MODS_MR_SEARCH, true, !mrSearching, 12);
        return;
    }

    // 加载器选择
    for (int i = 0; i < kLoaderCount; ++i) {
        RECT b = mkRect(dc.left + 16 + i * 96, iy, dc.left + 16 + i * 96 + 88, iy + 30);
        bool active = (loader == i);
        Color bg = active ? kAccent : kCardBg;
        Color fg = active ? kWhite : kText;
        bool hover = (hoverId_ == BTN_MODS_LOADER_BASE + i);
        if (hover && !active) bg = kHover;
        fillRound(g, b, 6, bg);
        if (!active) strokeRound(g, b, 6, kLine);
        Font fb(&ff, 12.0f, FontStyleRegular, UnitPixel);
        drawText(g, kLoaderNames[i], fb, fg, b, 1);
        hits_.push_back({ b, BTN_MODS_LOADER_BASE + i });
    }

    // 当前版本提示 + 获取按钮
    RECT vin = mkRect(dc.left + 16 + 4 * 96, iy, dc.right - 176, iy + 30);
    std::wstring vtxt = mcver.empty() ? L"目标版本见上方输入框" : (L"目标 MC 版本：" + mcver);
    drawText(g, vtxt, fB, kTextDim, vin, 0);

    RECT bget = mkRect(dc.right - 166, iy, dc.right - 16, iy + 30);
    drawButton(g, bget, fLoading ? L"获取中…" : L"获取可用版本", BTN_MODS_GET_VERSIONS, true, !fLoading, 12);
    iy += 40;

    // 打开 mods 目录
    RECT bdir = mkRect(dc.left + 16, iy, dc.left + 176, iy + 30);
    drawButton(g, bdir, L"打开 mods 目录", BTN_MODS_OPEN_DIR, false, true, 12);
    RECT dtip = mkRect(dc.left + 186, iy, dc.right - 16, iy + 30);
    drawText(g, L"下载的 jar 将直接保存到 <游戏目录>\\.minecraft\\mods", fS, kTextDim, dtip, 0);
    iy += 42;

    // 下载状态
    if (!dlResult.empty() || dlActive) {
        RECT sr = mkRect(dc.left + 16, iy, dc.right - 16, iy + 22);
        drawText(g, dlActive ? (L"下载中 " + std::to_wstring(dlPct) + L"%  ·  " + dlResult) : dlResult,
                 fB, dlActive ? kAccent : kTextDim, sr, 0);
        RECT pb = mkRect(dc.left + 16, iy + 24, dc.right - 16, iy + 32);
        fillRound(g, pb, 4, C(230, 232, 236));
        if (dlPct > 0) {
            RECT pf = mkRect(pb.left, pb.top, pb.left + (pb.right - pb.left) * (dlPct > 100 ? 100 : dlPct) / 100, pb.bottom);
            fillRound(g, pf, 4, kAccent);
        }
        iy += 44;
    }

    // 文件列表
    if (fLoading) {
        drawText(g, L"正在查询 Modrinth 可用文件…", fB, kTextDim, mkRect(dc.left + 16, iy, dc.right - 16, iy + 40), 0);
        return;
    }
    if (!fErr.empty()) {
        RECT er = mkRect(dc.left + 16, iy, dc.right - 16, iy + 40);
        drawText(g, fErr, fB, kTextDim, er, 0, false);
        return;
    }
    if (!fLoaded || files.empty()) {
        drawText(g, L"点击「获取可用版本」按当前 MC 版本与加载器列出可下载的 jar 文件。",
                 fB, kTextDim, mkRect(dc.left + 16, iy, dc.right - 16, iy + 40), 0);
        return;
    }

    int listTop = iy;
    int rowH = 52;
    int viewRows = (dc.bottom - 12 - listTop) / rowH;
    if (viewRows < 1) viewRows = 1;
    int start = (int)files.size() > viewRows ? (int)files.size() - viewRows : 0;  // 仅显示最后若干条(最新在前)
    int shownRows = 0;
    for (int i = start; i < (int)files.size(); ++i) {
        const ModFile& mf = files[i];
        int ry = listTop + shownRows * rowH;
        RECT rr = mkRect(dc.left + 16, ry, dc.right - 16, ry + rowH - 8);
        bool hover = (hoverId_ == BTN_MODVER_ROW_BASE + i);
        fillRound(g, rr, 6, hover ? kHover : C(248, 249, 250));
        strokeRound(g, rr, 6, kLine);
        hits_.push_back({ rr, BTN_MODVER_ROW_BASE + i });

        std::wstring vn = mf.ver.empty() ? L"(未知版本)" : mf.ver;
        drawText(g, vn, fT, kText, mkRect(rr.left + 12, rr.top + 4, rr.right - 200, rr.top + 26), 0);
        std::wstring sub2 = mf.loaders;
        if (mf.size > 0) sub2 += (sub2.empty() ? L"" : L"  ·  ") + humanSize(mf.size);
        RECT s2 = mkRect(rr.left + 12, rr.top + 24, rr.right - 200, rr.top + 44);
        drawText(g, sub2, fS, kTextDim, s2, 0);

        RECT bdl = mkRect(rr.right - 176, rr.top + 8, rr.right - 96, rr.top + 40);
        drawButton(g, bdl, L"下载", BTN_MODVER_ROW_BASE + i, true, !dlActive, 12);
        RECT bf = mkRect(rr.right - 92, rr.top + 8, rr.right - 12, rr.top + 40);
        drawText(g, oneLine(mf.name, 28), fS, kTextDim, bf, 2);
        ++shownRows;
    }
    (void)fS;
}

// ------------------------------ 入口 ---------------------------------------
int MainWindow::run(HINSTANCE hInst, int nCmdShow) {
    if (!createWindow(hInst, nCmdShow)) return 1;
    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return 0;
}

int runApp(HINSTANCE hInst, int nCmdShow) {
    GdiplusStartupInput gsi;
    ULONG_PTR token = 0;
    if (GdiplusStartup(&token, &gsi, nullptr) != Ok) return 1;
    int r = MainWindow::inst().run(hInst, nCmdShow);
    GdiplusShutdown(token);
    return r;
}
