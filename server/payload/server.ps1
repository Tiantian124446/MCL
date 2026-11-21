# MCL Server for Windows 10 (PowerShell 5.x, user or admin)
$ErrorActionPreference = 'Continue'
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
$WebDir = Join-Path $Root 'web'
$DlDir = Join-Path $Root 'downloads'
$FbDir = Join-Path $Root 'feedback'
$LogDir = Join-Path $Root 'logs'
$Port = 7891
$HostWatcher = 'http://192.168.3.155:8098/feedback'
foreach ($d in @($WebDir, $DlDir, $FbDir, $LogDir)) { if (-not (Test-Path $d)) { New-Item -ItemType Directory -Path $d -Force | Out-Null } }
$LogFile = Join-Path $LogDir 'server.log'
$AdminTokenFile = Join-Path $Root 'admin_token.txt'
if (-not (Test-Path $AdminTokenFile)) {
  try {
    $rng = [System.Security.Cryptography.RandomNumberGenerator]::Create()
    $bytes = New-Object byte[] 16
    $rng.GetBytes($bytes)
    $hex = ($bytes | ForEach-Object { $_.ToString('x2') }) -join ''
    [IO.File]::WriteAllText($AdminTokenFile, $hex, (New-Object System.Text.UTF8Encoding($false)))
  } catch { }
}
$AdminToken = ''
if (Test-Path $AdminTokenFile) { try { $AdminToken = ([IO.File]::ReadAllText($AdminTokenFile, [Text.Encoding]::UTF8)).Trim() } catch {} }
$script:LastFb = @{}

function Log([string]$msg) {
  $line = ('[{0}] {1}' -f (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'), $msg)
  try { Add-Content -Path $LogFile -Value $line -Encoding UTF8 } catch {}
}

function Read-Version {
  $v = @{ version = '1.0.0'; updated = ''; file = 'MCLauncher.exe'; notes = '' }
  $path = Join-Path $Root 'version.json'
  if (Test-Path $path) {
    try {
      $raw = [System.IO.File]::ReadAllText($path, [System.Text.Encoding]::UTF8)
      $o = $raw | ConvertFrom-Json
      if ($o.version) { $v.version = [string]$o.version }
      if ($o.updated) { $v.updated = [string]$o.updated }
      if ($o.file) { $v.file = [string]$o.file }
      if ($o.notes) { $v.notes = [string]$o.notes }
    } catch { Log ('version.json parse error: ' + $_.Exception.Message) }
  }
  return $v
}

function Get-Address {
  try {
    $ips = [System.Net.Dns]::GetHostAddresses([System.Net.Dns]::GetHostName()) | Where-Object { $_.AddressFamily -eq 'InterNetwork' -and -not $_.IPAddressToString.StartsWith('127.') }
    if ($ips) { return ($ips[0].IPAddressToString + ':' + $Port) }
  } catch {}
  return ('127.0.0.1:' + $Port)
}

function Send-Bytes($stream, [byte[]]$bytes) {
  $stream.Write($bytes, 0, $bytes.Length)
  $stream.Flush()
}

function Send-Response($stream, [int]$code, [string]$ctype, [byte[]]$body, [hashtable]$extra) {
  $reason = switch ($code) { 200 {'OK'} 204 {'No Content'} 400 {'Bad Request'} 404 {'Not Found'} 405 {'Method Not Allowed'} 500 {'Internal Server Error'} default {'OK'} }
  $head = "HTTP/1.1 $code $reason`r`n"
  $head += "Server: MCL-Server/1.0`r`n"
  $head += "Content-Type: $ctype`r`n"
  $head += "Content-Length: $($body.Length)`r`n"
  $head += "Cache-Control: no-store`r`n"
  $head += "Connection: close`r`n"
  if ($extra) { foreach ($k in $extra.Keys) { $head += "$k`: $($extra[$k])`r`n" } }
  $head += "`r`n"
  Send-Bytes $stream ([System.Text.Encoding]::ASCII.GetBytes($head))
  if ($body.Length -gt 0) { Send-Bytes $stream $body }
}

function Send-Text($stream, [int]$code, [string]$text, [string]$ctype, [hashtable]$extra) {
  Send-Response $stream $code $ctype ([System.Text.Encoding]::UTF8.GetBytes($text)) $extra
}

function Read-Request($stream) {
  $ms = New-Object System.IO.MemoryStream
  $buf = New-Object byte[] 8192
  $headerEnd = -1
  while ($headerEnd -lt 0) {
    $n = $stream.Read($buf, 0, $buf.Length)
    if ($n -le 0) { break }
    $ms.Write($buf, 0, $n)
    $arr = $ms.ToArray()
    for ($i = 0; $i -le $arr.Length - 4; $i++) {
      if ($arr[$i] -eq 13 -and $arr[$i+1] -eq 10 -and $arr[$i+2] -eq 13 -and $arr[$i+3] -eq 10) { $headerEnd = $i + 4; break }
    }
    if ($ms.Length -gt 65536) { break }
  }
  if ($headerEnd -lt 0) { return $null }
  $all = $ms.ToArray()
  $headerText = [System.Text.Encoding]::ASCII.GetString($all, 0, $headerEnd)
  $lines = $headerText -split "`r`n"
  $requestLine = $lines[0]
  $headers = @{}
  for ($i = 1; $i -lt $lines.Length; $i++) {
    $line = $lines[$i]
    $idx = $line.IndexOf(':')
    if ($idx -gt 0) { $headers[$line.Substring(0, $idx).Trim().ToLowerInvariant()] = $line.Substring($idx + 1).Trim() }
  }
  $clen = 0
  if ($headers.ContainsKey('content-length')) { [int]::TryParse($headers['content-length'], [ref]$clen) | Out-Null }
  $body = New-Object System.IO.MemoryStream
  if ($all.Length -gt $headerEnd) { $body.Write($all, $headerEnd, $all.Length - $headerEnd) }
  while ($body.Length -lt $clen) {
    $n = $stream.Read($buf, 0, $buf.Length)
    if ($n -le 0) { break }
    $body.Write($buf, 0, $n)
  }
  $bodyBytes = $body.ToArray()
  if ($bodyBytes.Length -gt $clen) { $bodyBytes = $bodyBytes[0..([Math]::Max(0,$clen-1))] }
  $parts = $requestLine -split ' '
  $method = if ($parts.Length -gt 0) { $parts[0].ToUpperInvariant() } else { 'GET' }
  $url = if ($parts.Length -gt 1) { $parts[1] } else { '/' }
  $path = $url.Split('?')[0]
  return @{ Method = $method; Path = $path; Url = $url; Headers = $headers; Body = $bodyBytes; Raw = $requestLine }
}

function Handle-Request($client) {
  $stream = $client.GetStream()
  $stream.ReadTimeout = 15000
  $stream.WriteTimeout = 15000
  try {
    $req = Read-Request $stream
    if ($null -eq $req) { return }
    Log ($req.Raw)
    $path = $req.Path
    if ($path -eq '/' -or $path -eq '/index.html') {
      $file = Join-Path $WebDir 'index.html'
      if (Test-Path $file) {
        Send-Response $stream 200 'text/html; charset=utf-8' ([System.IO.File]::ReadAllBytes($file)) @{}
      } else { Send-Text $stream 500 'text/plain; charset=utf-8' 'missing index.html' @{} }
      return
    }
    if ($path -eq '/api/status') {
      $v = Read-Version
      $exe = Join-Path $DlDir $v.file
      $size = 0; if (Test-Path $exe) { $size = (Get-Item $exe).Length }
      $fbCount = 0
      try { $fbCount = @(Get-ChildItem $FbDir -Filter 'BUG反馈_*.txt' -ErrorAction SilentlyContinue).Count } catch {}
      $json = [pscustomobject]@{
        version = $v.version; updated = $v.updated; size = $size;
        feedbackCount = $fbCount; address = (Get-Address); notes = $v.notes
      } | ConvertTo-Json -Compress
      Send-Text $stream 200 $json 'application/json; charset=utf-8' @{}
      return
    }
    if ($path -eq '/download/latest' -or $path -eq '/download') {
      $v = Read-Version
      $exe = Join-Path $DlDir $v.file
      if (-not (Test-Path $exe)) { Send-Text $stream 404 'text/plain; charset=utf-8' 'launcher not found' @{}; return }
      $bytes = [System.IO.File]::ReadAllBytes($exe)
      $safeName = 'MCLauncher_v' + $v.version + '.exe'
      $disp = 'attachment; filename="' + $safeName + '"; filename*=UTF-8''' + [Uri]::EscapeDataString('MCLauncher_v' + $v.version + '.exe')
      Send-Response $stream 200 'application/octet-stream' $bytes @{ 'Content-Disposition' = $disp }
      Log ('download v' + $v.version + ' size=' + $bytes.Length)
      return
    }
    if ($path -eq '/download/source') {
      $src = Join-Path $DlDir 'mcl-source.zip'
      if (Test-Path $src) {
        Send-Response $stream 200 'application/zip' ([System.IO.File]::ReadAllBytes($src)) @{ 'Content-Disposition' = 'attachment; filename="mcl-source.zip"' }
      } else { Send-Text $stream 404 'text/plain; charset=utf-8' 'source package not found' @{} }
      return
    }
    if ($path -eq '/feedback') {
      if ($req.Method -eq 'OPTIONS') { Send-Response $stream 204 'text/plain' ([byte[]]@()) @{ 'Access-Control-Allow-Origin' = '*' }; return }
      if ($req.Method -ne 'POST') { Send-Text $stream 405 'text/plain; charset=utf-8' 'method not allowed' @{}; return }
      $bodyText = [System.Text.Encoding]::UTF8.GetString($req.Body)
      $parsed = @{}
      foreach ($pair in $bodyText.Split('&')) {
        if ([string]::IsNullOrWhiteSpace($pair)) { continue }
        $kv = $pair.Split('=', 2)
        $k = [Uri]::UnescapeDataString($kv[0].Replace('+',' '))
        $v = ''
        if ($kv.Length -gt 1) { $v = [Uri]::UnescapeDataString($kv[1].Replace('+',' ')) }
        $parsed[$k] = $v
      }
      $feedback = ''
      if ($parsed.ContainsKey('feedback')) { $feedback = $parsed['feedback'] }
      $contact = ''
      if ($parsed.ContainsKey('contact')) { $contact = $parsed['contact'] }
      $ip = $client.Client.RemoteEndPoint.Address.ToString()
      $now = Get-Date
      if ($script:LastFb.ContainsKey($ip)) {
        $age = ($now - $script:LastFb[$ip]).TotalSeconds
        if ($age -lt 60) {
          Send-Text $stream 429 ('提交太频繁，请 ' + [int](60 - $age) + ' 秒后再试。') 'text/plain; charset=utf-8' @{ 'Retry-After' = '60' }
          return
        }
      }
      $script:LastFb[$ip] = $now
      if ($script:LastFb.Count -gt 500) {
        $cut = $now.AddMinutes(-10)
        foreach ($k in @($script:LastFb.Keys)) { if ($script:LastFb[$k] -lt $cut) { $script:LastFb.Remove($k) } }
      }
      if ($bodyText.Length -gt 20000) {
        Send-Text $stream 413 '反馈内容过长' 'text/plain; charset=utf-8' @{}
        return
      }
      if ([string]::IsNullOrWhiteSpace($feedback)) { Send-Text $stream 400 '反馈内容不能为空' 'text/plain; charset=utf-8' @{}; return }
      $stamp = Get-Date -Format 'yyyyMMdd_HHmmss'
      $title = 'BUG反馈'
      $content = "MCL 启动器 BUG 反馈`r`n"
      $content += "时间: " + (Get-Date -Format 'yyyy-MM-dd HH:mm:ss') + "`r`n"
      $content += "版本: " + (Read-Version).version + "`r`n"
      $content += "联系方式: " + $contact + "`r`n"
      $content += "来源: " + $client.Client.RemoteEndPoint.ToString() + "`r`n"
      $content += ("-" * 48) + "`r`n"
      $content += $feedback + "`r`n"
      $enc = New-Object System.Text.UTF8Encoding($true)
      $file = Join-Path $FbDir ("BUG反馈_" + $stamp + ".txt")
      [System.IO.File]::WriteAllText($file, $content, $enc)
      [System.IO.File]::WriteAllText((Join-Path $FbDir 'BUG反馈.txt'), $content, $enc)
      Log ('feedback saved: ' + $file)
      # notify host watcher (best effort)
      try {
        $payload = [pscustomobject]@{ id = $stamp; time = (Get-Date -Format 'yyyy-MM-dd HH:mm:ss'); version = (Read-Version).version; contact = $contact; text = $feedback } | ConvertTo-Json -Compress
        $wc = New-Object System.Net.WebClient
        $wc.Encoding = [System.Text.Encoding]::UTF8
        $wc.Headers['Content-Type'] = 'application/json; charset=utf-8'
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($payload)
        $wc.UploadData($HostWatcher, 'POST', $bytes) | Out-Null
      } catch { Log ('watcher notify failed: ' + $_.Exception.Message) }
      $respBytes = $enc.GetBytes($content)
      $disp = 'attachment; filename="BUG_fankui.txt"; filename*=UTF-8''' + [Uri]::EscapeDataString('BUG反馈.txt')
      Send-Response $stream 200 'text/plain; charset=utf-8' $respBytes @{ 'Content-Disposition' = $disp; 'X-Feedback-Id' = $stamp }
      return
    }
    if ($path -eq '/admin' -or $path -eq '/admin/feedback' -or $path -eq '/admin/feedback.csv') {
      $query = ''
      if ($req.ContainsKey('Url')) {
        $qp = $req.Url.Split('?', 2)
        if ($qp.Length -gt 1) { $query = $qp[1] }
      }
      $token = ''
      foreach ($pair in $query.Split('&')) {
        if ([string]::IsNullOrWhiteSpace($pair)) { continue }
        $kv = $pair.Split('=', 2)
        if ([Uri]::UnescapeDataString($kv[0]) -eq 'token' -and $kv.Length -gt 1) {
          $token = [Uri]::UnescapeDataString($kv[1].Replace('+', ' '))
        }
      }
      if ($path -eq '/admin' -and [string]::IsNullOrEmpty($token)) {
        $form = '<!doctype html><meta charset="utf-8"><title>MCL 反馈管理</title><style>body{font-family:Microsoft YaHei;background:#0b1020;color:#e8eefc;padding:40px}input{padding:10px;width:360px;border-radius:8px;border:1px solid #31406a;background:#0d1429;color:#e8eefc}button{padding:10px 18px;border-radius:8px;border:0;background:#4f8cff;color:#fff;font-weight:700}</style><h1>MCL 反馈管理</h1><p>输入 admin token 查看 Win10 上的 BUG反馈 记录。</p><form method="get" action="/admin/feedback"><input name="token" placeholder="admin token"><button>查看反馈</button></form>'
        Send-Text $stream 200 $form 'text/html; charset=utf-8' @{}
        return
      }
      if ([string]::IsNullOrEmpty($AdminToken) -or $token -ne $AdminToken) {
        Send-Text $stream 403 'forbidden: invalid admin token' 'text/plain; charset=utf-8' @{}
        Log ('admin denied from ' + $client.Client.RemoteEndPoint.ToString())
        return
      }
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
      if ($files.Count -eq 0) { $html += '<p>暂无反馈。</p>' }
      foreach ($f in $files) {
        $content = ''
        try { $content = [IO.File]::ReadAllText($f.FullName, [Text.Encoding]::UTF8) } catch {}
        $html += '<h2>' + [System.Net.WebUtility]::HtmlEncode($f.Name) + '</h2><pre>' + [System.Net.WebUtility]::HtmlEncode($content) + '</pre>'
      }
      Send-Text $stream 200 $html 'text/html; charset=utf-8' @{}
      Log ('admin viewed ' + $files.Count + ' feedback files')
      return
    }
    if ($path -eq '/favicon.ico') { Send-Response $stream 204 'image/x-icon' ([byte[]]@()) @{}; return }
    Send-Text $stream 404 'text/plain; charset=utf-8' 'not found' @{}
  } catch {
    Log ('handler error: ' + $_.Exception.Message)
    try { Send-Text $stream 500 'text/plain; charset=utf-8' 'server error' @{} } catch {}
  } finally {
    try { $stream.Close() } catch {}
    try { $client.Close() } catch {}
  }
}

# single instance guard
$listener = $null
try {
  $listener = New-Object System.Net.Sockets.TcpListener([System.Net.IPAddress]::Any, $Port)
  $listener.Start(50)
} catch {
  Log ('cannot bind port ' + $Port + ': ' + $_.Exception.Message)
  exit 1
}
Log ('MCL server started on port ' + $Port + ' root=' + $Root)
try {
  while ($true) {
    $client = $listener.AcceptTcpClient()
    try { Handle-Request $client } catch { Log ('accept error: ' + $_.Exception.Message) }
  }
} finally {
  try { $listener.Stop() } catch {}
  Log 'server stopped'
}
