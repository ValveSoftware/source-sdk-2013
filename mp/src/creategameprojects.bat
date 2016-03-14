@echo off
devtools\bin\vpc.exe /momentum +game /mksln momentum.sln /f
copy momentum.sln+sln_fix.txt momentum.sln
Powershell.exe -ExecutionPolicy Bypass -File creategameprojects.ps1
PAUSE