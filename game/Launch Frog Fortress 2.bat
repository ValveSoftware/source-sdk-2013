@echo off
title Rainbow Is Magic TF (Source SDK Mod)
cd /d "%~dp0"
echo.
echo  Rainbow Is Magic TF - run mod_tf_win64.exe (Bop the Teddy mode).
echo  Solo test: map ctf_2fort, then tf_bot_add 8  (see mod_tf\SOLO_TEST.txt)
echo.
echo  Starting... (Steam should be running)
echo.
start "" "%~dp0mod_tf_win64.exe" -game mod_tf -console -insecure +sv_lan 1 +con_enable 1 +map koth_badlands +exec ow_quickstart
timeout /t 3 >nul
