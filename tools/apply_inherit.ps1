$ErrorActionPreference = 'Stop'
$p = 'E:\MCL\src\core.cpp'
$u = ([IO.File]::ReadAllText($p, [Text.Encoding]::UTF8)) -replace "`r`n", "`n"

$fn = @'
void jsonSetField(mjson::Value& obj, const std::string& key, const mjson::Value& val) {
    for (auto& kv : obj.obj) {
        if (kv.first == key) { kv.second = val; return; }
    }
    obj.obj.push_back({ key, val });
}

void mergeInheritedVersion(const mjson::Value& parent, mjson::Value& child) {
    const char* scalars[] = { "mainClass", "assetIndex", "assets", "downloads", "logging", "javaVersion", "complianceLevel" };
    for (const char* k : scalars) {
        const mjson::Value& cv = child.get(k);
        bool empty = !child.has(k) || cv.isNull() ||
                     (cv.isObject() && cv.size() == 0) ||
                     (cv.isString() && cv.asString().empty());
        if (empty && parent.has(k)) jsonSetField(child, k, parent.get(k));
    }
    if (child.get("type").asString().empty() && parent.has("type")) jsonSetField(child, "type", parent.get("type"));
    if (child.get("releaseTime").asString().empty() && parent.has("releaseTime"))
        jsonSetField(child, "releaseTime", parent.get("releaseTime"));

    mjson::Value libs;
    libs.type = mjson::Value::Type::Array;
    const mjson::Value& pl = parent.get("libraries");
    for (size_t i = 0; i < pl.size(); ++i) libs.arr.push_back(pl.at(i));
    const mjson::Value& cl = child.get("libraries");
    for (size_t i = 0; i < cl.size(); ++i) libs.arr.push_back(cl.at(i));
    jsonSetField(child, "libraries", libs);

    if (parent.has("arguments") || child.has("arguments")) {
        mjson::Value args;
        args.type = mjson::Value::Type::Object;
        const char* keys[] = { "jvm", "game" };
        for (const char* k : keys) {
            mjson::Value arr;
            arr.type = mjson::Value::Type::Array;
            if (parent.has("arguments")) {
                const mjson::Value& pa = parent.get("arguments").get(k);
                for (size_t i = 0; i < pa.size(); ++i) arr.arr.push_back(pa.at(i));
            }
            if (child.has("arguments")) {
                const mjson::Value& ca = child.get("arguments").get(k);
                for (size_t i = 0; i < ca.size(); ++i) arr.arr.push_back(ca.at(i));
            }
            if (!arr.arr.empty()) args.obj.push_back({ k, arr });
        }
        if (!args.obj.empty()) jsonSetField(child, "arguments", args);
    }
    if (child.get("minecraftArguments").asString().empty() && parent.has("minecraftArguments"))
        jsonSetField(child, "minecraftArguments", parent.get("minecraftArguments"));
}

'@
$fn = $fn -replace "`r`n", "`n"
$anchor = 'bool Launcher::launch(const std::string& id, std::wstring& err) {'
if (-not $u.Contains($anchor)) { throw 'launch anchor missing' }
$u = $u.Replace($anchor, $fn + $anchor)

$old = @'
    std::wstring jsonPath = joinPath(vdir, vidW + L".json");
    std::wstring jarPath  = joinPath(vdir, vidW + L".jar");

    std::string text;
    if (!readAll(jsonPath, text)) { err = L"未找到版本文件, 请先下载该版本"; return false; }
    if (!pathExists(jarPath))     { err = L"未找到游戏主程序, 请先下载该版本"; return false; }

    mjson::Value root;
    if (!mjson::parse(text, root)) { err = L"版本 JSON 解析失败"; return false; }
'@
$new = @'
    std::wstring jsonPath = joinPath(vdir, vidW + L".json");
    std::wstring jarPath  = joinPath(vdir, vidW + L".jar");

    std::string text;
    if (!readAll(jsonPath, text)) { err = L"未找到版本文件, 请先下载该版本"; return false; }

    mjson::Value root;
    if (!mjson::parse(text, root)) { err = L"版本 JSON 解析失败"; return false; }

    // 继承版本 (Fabric/Forge 等): 合并父版本 JSON, 子版本没有 jar 时使用父版本 jar
    if (root.has("inheritsFrom")) {
        std::string parentId = root.get("inheritsFrom").asString();
        if (!parentId.empty()) {
            std::wstring parentW = utf8ToWide(parentId);
            std::wstring parentDir = joinPath(joinPath(opt.gameDir, L"versions"), parentW);
            std::wstring parentJson = joinPath(parentDir, parentW + L".json");
            std::string parentText;
            if (!readAll(parentJson, parentText)) {
                err = L"父版本 " + parentW + L" 的 JSON 未找到, 请先安装该版本";
                return false;
            }
            mjson::Value parent;
            if (!mjson::parse(parentText, parent)) { err = L"父版本 JSON 解析失败"; return false; }
            mergeInheritedVersion(parent, root);
            std::wstring parentJar = joinPath(parentDir, parentW + L".jar");
            if (!pathExists(jarPath) && pathExists(parentJar)) jarPath = parentJar;
        }
    }
    if (!pathExists(jarPath)) { err = L"未找到游戏主程序, 请先下载该版本"; return false; }
'@
$old = $old -replace "`r`n", "`n"
$new = $new -replace "`r`n", "`n"
if (-not $u.Contains($old)) { throw 'launch code anchor missing' }
$u = $u.Replace($old, $new)

[IO.File]::WriteAllText($p, $u, (New-Object Text.UTF8Encoding($false)))
"inherit merge patched, length=$($u.Length)"
