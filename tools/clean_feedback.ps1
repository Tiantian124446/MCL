Get-ChildItem 'C:\MCLServer\feedback' -Filter 'BUG??*.txt' -ErrorAction SilentlyContinue | Remove-Item -Force -ErrorAction SilentlyContinue
Get-ChildItem 'C:\MCLServer\feedback' -Filter 'BUG*.txt' -ErrorAction SilentlyContinue | Remove-Item -Force -ErrorAction SilentlyContinue
Write-Host 'guest feedback cleaned'
