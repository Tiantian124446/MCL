@echo off
netsh interface ipv4 set address name="Ethernet0" static 192.168.3.173 255.255.255.0 192.168.3.1
netsh interface ipv4 set dns name="Ethernet0" static 192.168.3.1
ipconfig > C:\ip_config.txt
ping -n 2 192.168.3.155 > C:\ip_ping.txt
