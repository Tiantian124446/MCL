$ErrorActionPreference = 'Stop'
$p = 'E:\MCL\PROJECT_STATE.md'
$t = ([IO.File]::ReadAllText($p, [Text.Encoding]::UTF8)) -replace "`r`n", "`n"
$old = @'
- 反馈限流：同 IP 60 秒 1 条（429 + `Retry-After`），请求体 > 20KB 返回 413
'@
$new = @'
- 反馈限流：同 IP 60 秒 1 条（429 + `Retry-After`），请求体 > 20KB 返回 413
- 管理页：`/admin` 输入 token → `/admin/feedback?token=…` 查看 Win10 上的 `BUG反馈_*.txt`；token 在 `C:\MCLServer\admin_token.txt`
'@
$old = $old -replace "`r`n", "`n"
$new = $new -replace "`r`n", "`n"
if (-not $t.Contains($old)) { throw 'project anchor missing' }
$t = $t.Replace($old, $new)
$t = $t.Replace('3. 站点反馈查看页（加 token）；限流已完成。', '3. 站点反馈查看页已完成（`/admin` + token）；可增加导出/分类。')
[IO.File]::WriteAllText($p, $t, (New-Object Text.UTF8Encoding($false)))

$r = 'E:\MCL\README.md'
$rt = ([IO.File]::ReadAllText($r, [Text.Encoding]::UTF8)) -replace "`r`n", "`n"
$ro = @'
- 反馈保存为 `C:\MCLServer\feedback\BUG反馈_*.txt`，并 POST 到主机 `:8098/feedback`
'@
$rn = @'
- 反馈保存为 `C:\MCLServer\feedback\BUG反馈_*.txt`，并 POST 到主机 `:8098/feedback`
- 管理页：`/admin` → 输入 `C:\MCLServer\admin_token.txt` 中的 token 查看反馈记录
'@
$ro = $ro -replace "`r`n", "`n"
$rn = $rn -replace "`r`n", "`n"
if ($rt.Contains($ro)) { $rt = $rt.Replace($ro, $rn); [IO.File]::WriteAllText($r, $rt, (New-Object Text.UTF8Encoding($false))) }
'docs updated'
