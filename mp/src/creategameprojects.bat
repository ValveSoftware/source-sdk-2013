devtools\bin\vpc.exe /hl2 +game /mksln momentum.sln
copy momentum.sln+sln_fix.txt momentum.sln
Powershell.exe -ExecutionPolicy Bypass -File creategameprojects.ps1