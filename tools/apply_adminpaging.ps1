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
      $q = ''
'@ @'
      $token = ''
      $q = ''
      $page = 1
'@

Rep @'
        } elseif ([Uri]::UnescapeDataString($kv[0]) -eq 'q' -and $kv.Length -gt 1) {
          $q = [Uri]::UnescapeDataString($kv[1].Replace('+', ' '))
        }
'@ @'
        } elseif ([Uri]::UnescapeDataString($kv[0]) -eq 'q' -and $kv.Length -gt 1) {
          $q = [Uri]::UnescapeDataString($kv[1].Replace('+', ' '))
        } elseif ([Uri]::UnescapeDataString($kv[0]) -eq 'page' -and $kv.Length -gt 1) {
          try { $page = [int][Uri]::UnescapeDataString($kv[1]) } catch { $page = 1 }
        }
'@

Rep @'
      $tokHtml = [System.Net.WebUtility]::HtmlEncode($token)
      $qHtml = [System.Net.WebUtility]::HtmlEncode($q)
'@ @'
      $tokHtml = [System.Net.WebUtility]::HtmlEncode($token)
      $qHtml = [System.Net.WebUtility]::HtmlEncode($q)
      $totalCount = $files.Count
      $pageSize = 10
      $pageCount = [Math]::Max(1, [Math]::Ceiling($totalCount / $pageSize))
      if ($page -lt 1) { $page = 1 }
      if ($page -gt $pageCount) { $page = $pageCount }
      $files = @($files | Select-Object -Skip (($page - 1) * $pageSize) -First $pageSize)
      $baseUrl = '/admin/feedback?token=' + $tokHtml + '&q=' + [Uri]::EscapeDataString($q)
      $pager = '<p>'
      if ($page -gt 1) { $pager += '<a href="' + $baseUrl + '&page=' + ($page - 1) + '">上一页</a> · ' }
      $pager += '第 ' + $page + ' / ' + $pageCount + ' 页'
      if ($page -lt $pageCount) { $pager += ' · <a href="' + $baseUrl + '&page=' + ($page + 1) + '">下一页</a>' }
      $pager += '</p>'
'@

Rep @'
<button>搜索</button></form><p>共 ' + $files.Count + ' 条' + $(if ($q) { '（搜索：' + $qHtml + '）' } else { '' }) + ' · <a href="/">返回官网</a> · <a href="/admin/feedback.csv?token=' + $tokHtml + '">导出 CSV</a></p>'
'@ @'
<button>搜索</button></form><p>共 ' + $totalCount + ' 条' + $(if ($q) { '（搜索：' + $qHtml + '）' } else { '' }) + ' · <a href="/">返回官网</a> · <a href="/admin/feedback.csv?token=' + $tokHtml + '">导出 CSV</a></p>' + $pager
'@

[IO.File]::WriteAllText($p, $u, (New-Object Text.UTF8Encoding($true)))
'admin paging patched'
