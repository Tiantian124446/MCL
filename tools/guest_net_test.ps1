$ProgressPreference = 'SilentlyContinue'
$out = 'C:\net_test.txt'
Remove-Item $out -Force -ErrorAction SilentlyContinue
$sw = [Diagnostics.Stopwatch]::StartNew()
try {
  $r = Invoke-WebRequest 'https://api.modrinth.com/v2/search?query=faithful&limit=1' -UseBasicParsing -TimeoutSec 20
  Add-Content $out ("api ok status=" + $r.StatusCode + " ms=" + $sw.ElapsedMilliseconds)
} catch {
  Add-Content $out ("api err " + $_.Exception.Message)
}
$sw.Restart()
try {
  $r = Invoke-WebRequest 'https://cdn.modrinth.com/data/r4GILswZ/versions/6QfM0mZq/Faithful%2064x%20-%20Release%2015.zip' -Method Head -UseBasicParsing -TimeoutSec 20
  Add-Content $out ("cdn head status=" + $r.StatusCode + " ms=" + $sw.ElapsedMilliseconds + " len=" + $r.Headers['Content-Length'])
} catch {
  Add-Content $out ("cdn err " + $_.Exception.Message)
}
Add-Content $out 'done'
