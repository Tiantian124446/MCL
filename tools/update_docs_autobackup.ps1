$ErrorActionPreference = 'Stop'
$r = 'E:\MCL\README.md'
$t = ([IO.File]::ReadAllText($r, [Text.Encoding]::UTF8)) -replace "`r`n", "`n"
$old = '- 存档管理：存档列表、备份到 `backups/saves_*`、删除、打开目录'
$new = $old + "`n" + '- 自动备份：启动时 24 小时内无 `backups/auto_*` 则后台复制整个 `saves`'
if ($t.Contains($old)) {
    $t = $t.Replace($old, $new)
    [IO.File]::WriteAllText($r, $t, (New-Object Text.UTF8Encoding($false)))
    'readme updated'
} else {
    'readme anchor not found'
}
