@echo off
title Deploy + Launch Frog Fortress 2 (RIM)
cd /d "%~dp0"

echo Copying latest server.dll...
copy /Y "%~dp0..\src\game\server\Release_mod_tf\server.dll" "%~dp0mod_tf\bin\x64\server.dll" >nul
if errorlevel 1 (
  echo WARN: server.dll copy failed - quit the game if it is running, then run this again.
) else (
  echo server.dll deployed.
)

if exist "%~dp0..\src\game\client\Release_mod_tf\client.dll" (
  copy /Y "%~dp0..\src\game\client\Release_mod_tf\client.dll" "%~dp0mod_tf\bin\x64\client.dll" >nul 2>&1
  if not errorlevel 1 echo client.dll deployed.
)

echo.
echo Launching Frog Fortress 2...
echo   Map: plr_hightower  |  Console: F1 or `
echo   Steam: set Working Directory to this game folder if you use Non-Steam shortcut.
echo.
start "" "%~dp0mod_tf_win64.exe" -game mod_tf -console -insecure +sv_lan 1 +con_enable 1 +map koth_badlands +exec ow_quickstart
