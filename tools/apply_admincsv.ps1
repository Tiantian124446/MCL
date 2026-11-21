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

Rep "if (`$path -eq '/admin' -or `$path -eq '/admin/feedback') {" @"
if (`$path -eq '/admin' -or `$path -eq '/admin/feedback' -or `$path -eq '/admin/feedback.csv') {
"@

Rep @'
      $files = @(Get-ChildItem $FbDir -Filter 'BUG反馈_*.txt' -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending | Select-Object -First 200)
      $html = '<!doctype html><meta charset="utf-8"><title>MCL 反馈记录</title><style>body{font-family:Microsoft YaHei;background:#0b1020;color:#e8eefc;padding:32px;max-width:1000px;margin:auto}h1{color:#8fb6ff}h2{font-size:15px;color:#b9c8ea;margin-top:26px;border-top:1px solid #27345c;padding-top:14px}pre{white-space:pre-wrap;background:#121a33;border:1px solid #27345c;border-radius:10px;padding:14px;line-height:1.6}a{color:#4f8cff}</style><h1>MCL 反馈记录</h1><p>共 ' + $files.Count + ' 条 · <a href="/">返回官网</a></p>'
'@ @'
      $files = @(Get-ChildItem $FbDir -Filter 'BUG反馈_*.txt' -ErrorAction SilentlyContinue | Sort-Object LastWriteTime -Descending | Select-Object -First 200)
      if ($path -eq '/admin/feedback.csv') {
        $sb = New-Object System.Text.StringBuilder
        [void]$sb.AppendLine('文件名,时间,内容')
        foreach ($f in $files) {
          $content = ''
          try { $content = [IO.File]::ReadAllText($f.FullName, [Text.Encoding]::UTF8) } catch {}
          $name = $f.Name.Replace('"', '""')
          $time = $f.LastWriteTime.ToString('yyyy-MM-dd HH:mm:ss')
          $body = $content.Replace('"', '""')
          [void]$sb.AppendLine('"' + $name + '","' + $time + '","' + $body + '"')
        }
        $csv = [char]0xFEFF + $sb.ToString()
        Send-Text $stream 200 $csv 'text/csv; charset=utf-8' @{ 'Content-Disposition' = 'attachment; filename="BUG反馈.csv"' }
        Log ('admin exported csv, files=' + $files.Count)
        return
      }
      $html = '<!doctype html><meta charset="utf-8"><title>MCL 反馈记录</title><style>body{font-family:Microsoft YaHei;background:#0b1020;color:#e8eefc;padding:32px;max-width:1000px;margin:auto}h1{color:#8fb6ff}h2{font-size:15px;color:#b9c8ea;margin-top:26px;border-top:1px solid #27345c;padding-top:14px}pre{white-space:pre-wrap;background:#121a33;border:1px solid #27345c;border-radius:10px;padding:14px;line-height:1.6}a{color:#4f8cff}</style><h1>MCL 反馈记录</h1><p>共 ' + $files.Count + ' 条 · <a href="/">返回官网</a> · <a href="/admin/feedback.csv?token=' + $token + '">导出 CSV</a></p>'
'@

[IO.File]::WriteAllText($p, $u, (New-Object Text.UTF8Encoding($true)))
'admin csv patched'
