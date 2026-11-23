@echo off
netsh interface ipv4 set address name="Ethernet0" static 192.168.201.128 255.255.255.0 192.168.201.2
netsh interface ipv4 set dns name="Ethernet0" static 192.168.201.2
ipconfig > C:\nat_config.txt
ping -n 2 192.168.201.1 > C:\nat_ping.txt
