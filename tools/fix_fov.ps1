$ErrorActionPreference = 'Stop'
$p = 'E:\MCL\src\ui.cpp'
$u = ([IO.File]::ReadAllText($p, [Text.Encoding]::UTF8)) -replace "`r`n", "`n"
function Rep([string]$old, [string]$new) {
    $old = ($old -replace "`r`n", "`n")
    $new = ($new -replace "`r`n", "`n")
    $count = ([regex]::Matches($script:u, [regex]::Escape($old))).Count
    if ($count -ne 1) { throw "count $count for: $($old.Substring(0, [Math]::Min(60, $old.Length)))" }
    $script:u = $script:u.Replace($old, $new)
}

Rep 'static bool goptNumeric(int row, const char*& key, int& mn, int& mx, int& step) {' @'
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
'@

Rep @'
        if (goptNumeric(row, key, mn, mx, step)) {
            int v = 0;
            auto it = gopt_.find(key);
            if (it != gopt_.end()) { try { v = std::stoi(it->second); } catch (...) {} }
            v -= step; if (v < mn) v = mn;
'@ @'
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
'@

Rep @'
        if (goptNumeric(row, key, mn, mx, step)) {
            int v = 0;
            auto it = gopt_.find(key);
            if (it != gopt_.end()) { try { v = std::stoi(it->second); } catch (...) {} }
            v += step; if (v > mx) v = mx;
'@ @'
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
'@

Rep '        else if (i == 3) value = utf8ToWide(goptInt(gopt_, "fov", 70));' '        else if (i == 3) value = std::to_wstring(goptFovDegrees(gopt_)) + L"°";'

[IO.File]::WriteAllText($p, $u, (New-Object Text.UTF8Encoding($false)))
"fov fixed, length=$($u.Length)"
