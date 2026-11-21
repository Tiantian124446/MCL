$ErrorActionPreference = 'Stop'
$p = 'E:\MCL\server\payload\server.ps1'
$u = ([IO.File]::ReadAllText($p, [Text.Encoding]::UTF8)) -replace "`r`n", "`n"
function Rep([string]$old, [string]$new) {
    $old = ($old -replace "`r`n", "`n")
    $new = ($new -replace "`r`n", "`n")
    $count = ([regex]::Matches($script:u, [regex]::Escape($old))).Count
    if ($count -ne 1) { throw "count $count for: $($old.Substring(0, [Math]::Min(70, $old.Length)))" }
    $script:u = $script:u.Replace($old, $new)
}

Rep @'
      $token = ''
'@ @'
      $token = ''
      $q = ''
'@

Rep @'
        if ([Uri]::UnescapeDataString($kv[0]) -eq 'token' -and $kv.Length -gt 1) {
          $token = [Uri]::UnescapeDataString($kv[1].Replace('+', ' '))
        }
'@ @'
        if ([Uri]::UnescapeDataString($kv[0]) -eq 'token' -and $kv.Length -gt 1) {
          $token = [Uri]::UnescapeDataString($kv[1].Replace('+', ' '))
        } elseif ([Uri]::UnescapeDataString($kv[0]) -eq 'q' -and $kv.Length -gt 1) {
          $q = [Uri]::UnescapeDataString($kv[1].Replace('+', ' '))
        }
'@

Rep @'
      $files = @(Get-ChildItem $FbDir -Filter 'BUG反馈_*.txt' -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending | Select-Object -First 200)
'@ @'
      $files = @(Get-ChildItem $FbDir -Filter 'BUG反馈_*.txt' -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending | Select-Object -First 200)
      if (-not [string]::IsNullOrWhiteSpace($q)) {
        $files = @($files | Where-Object {
          $hit = ($_.Name.IndexOf($q, [StringComparison]::OrdinalIgnoreCase) -ge 0)
          if (-not $hit) {
            try {
              $c = [IO.File]::ReadAllText($_.FullName, [Text.Encoding]::UTF8)
              $hit = ($c.IndexOf($q, [StringComparison]::OrdinalIgnoreCase) -ge 0)
            } catch { $hit = $false }
          }
          $hit
        })
      }
'@

Rep @'
      $html = '<!doctype html><meta charset="utf-8"><title>MCL 反馈记录</title><style>body{font-family:Microsoft YaHei;background:#0b1020;color:#e8eefc;padding:32px;max-width:1000px;margin:auto}h1{color:#8fb6ff}h2{font-size:15px;color:#b9c8ea;margin-top:26px;border-top:1px solid #27345c;padding-top:14px}pre{white-space:pre-wrap;background:#121a33;border:1px solid #27345c;border-radius:10px;padding:14px;line-height:1.6}a{color:#4f8cff}</style><h1>MCL 反馈记录</h1><p>共 ' + $files.Count + ' 条 · <a href="/">返回官网</a> · <a href="/admin/feedback.csv?token=' + $token + '">导出 CSV</a></p>'
'@ @'
      $tokHtml = [System.Net.WebUtility]::HtmlEncode($token)
      $qHtml = [System.Net.WebUtility]::HtmlEncode($q)
      $html = '<!doctype html><meta charset="utf-8"><title>MCL 反馈记录</title><style>body{font-family:Microsoft YaHei;background:#0b1020;color:#e8eefc;padding:32px;max-width:1000px;margin:auto}h1{color:#8fb6ff}h2{font-size:15px;color:#b9c8ea;margin-top:26px;border-top:1px solid #27345c;padding-top:14px}pre{white-space:pre-wrap;background:#121a33;border:1px solid #27345c;border-radius:10px;padding:14px;line-height:1.6}a{color:#4f8cff}form{margin:18px 0}input{padding:8px 10px;border-radius:8px;border:1px solid #31406a;background:#0d1429;color:#e8eefc}button{padding:8px 14px;border-radius:8px;border:0;background:#4f8cff;color:#fff;font-weight:700}</style><h1>MCL 反馈记录</h1><form method="get" action="/admin/feedback"><input type="hidden" name="token" value="' + $tokHtml + '"><input name="q" value="' + $qHtml + '" placeholder="搜索文件名或内容"><button>搜索</button></form><p>共 ' + $files.Count + ' 条' + $(if ($q) { '（搜索：' + $qHtml + '）' } else { '' }) + ' · <a href="/">返回官网</a> · <a href="/admin/feedback.csv?token=' + $tokHtml + '">导出 CSV</a></p>'
'@

[IO.File]::WriteAllText($p, $u, (New-Object Text.UTF8Encoding($true)))
'admin search patched'
