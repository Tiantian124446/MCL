@echo off
setlocal
title MCL Campus Setup
set "SRV=%~1"
if "%SRV%"=="" set /p SRV=Server IP (e.g. 192.168.3.173): 
if "%SRV%"=="" ( echo No IP entered & pause & exit /b 1 )
echo Downloading setup script from %SRV% ...
powershell -NoProfile -ExecutionPolicy Bypass -Command "$ProgressPreference='SilentlyContinue'; try { Invoke-WebRequest ('http://%SRV%:8080/mcl-client-setup.ps1') -OutFile ($env:TEMP+'\mcl-cs.ps1') -UseBasicParsing } catch { Invoke-WebRequest ('http://%SRV%:7891/mcl-client-setup.ps1') -OutFile ($env:TEMP+'\mcl-cs.ps1') -UseBasicParsing }"
if not exist "%TEMP%\mcl-cs.ps1" ( echo Download failed - check server IP / server running & pause & exit /b 1 )
powershell -NoProfile -ExecutionPolicy Bypass -File "%TEMP%\mcl-cs.ps1" -ServerIp %SRV%
