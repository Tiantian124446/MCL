$ErrorActionPreference = 'Continue'
$log = 'C:\fixnet.log'
"=== before ===" | Out-File -FilePath $log -Encoding utf8
Get-NetIPConfiguration | Out-File -FilePath $log -Append -Encoding utf8

foreach ($a in Get-NetAdapter | Where-Object { $_.Status -eq 'Up' -and $_.InterfaceDescription -notmatch 'Loopback' }) {
    "adapter: $($a.Name) | $($a.InterfaceDescription)" | Out-File -FilePath $log -Append -Encoding utf8
    Get-NetIPAddress -InterfaceIndex $a.ifIndex -AddressFamily IPv4 -ErrorAction SilentlyContinue |
        Where-Object { $_.PrefixOrigin -ne 'Dhcp' } |
        ForEach-Object {
            "  remove static: $($_.IPAddress)/$($_.PrefixLength)" | Out-File -FilePath $log -Append -Encoding utf8
            Remove-NetIPAddress -InterfaceIndex $a.ifIndex -IPAddress $_.IPAddress -Confirm:$false -ErrorAction SilentlyContinue
        }
    Get-NetRoute -InterfaceIndex $a.ifIndex -DestinationPrefix '0.0.0.0/0' -ErrorAction SilentlyContinue |
        Where-Object { $_.NextHop -ne '0.0.0.0' } |
        ForEach-Object {
            "  remove route: $($_.DestinationPrefix) -> $($_.NextHop)" | Out-File -FilePath $log -Append -Encoding utf8
            Remove-NetRoute -InterfaceIndex $a.ifIndex -DestinationPrefix $_.DestinationPrefix -NextHop $_.NextHop -Confirm:$false -ErrorAction SilentlyContinue
        }
    Set-NetIPInterface -InterfaceIndex $a.ifIndex -Dhcp Enabled -ErrorAction SilentlyContinue
    Set-DnsClientServerAddress -InterfaceIndex $a.ifIndex -ResetServerAddresses -ErrorAction SilentlyContinue
}

Start-Sleep -Seconds 2
ipconfig /release | Out-File -FilePath $log -Append -Encoding utf8
ipconfig /renew | Out-File -FilePath $log -Append -Encoding utf8

"=== after ===" | Out-File -FilePath $log -Append -Encoding utf8
Get-NetIPConfiguration | Out-File -FilePath $log -Append -Encoding utf8
ipconfig /all | Out-File -FilePath $log -Append -Encoding utf8
"done" | Out-File -FilePath $log -Append -Encoding utf8
