:: Get game dir by going into the directory with the gameinfo.txt and capturing %CD%
cd ..
Set GAMEDIR="%CD%"

echo %* >> %GAMEDIR%\bin\args.txt

:: Get steam dir
Set Reg.Key=HKEY_CURRENT_USER\Software\Valve\Steam
Set Reg.Val=SteamPath

For /F "Tokens=2*" %%A In ('Reg Query "%Reg.Key%" /v "%Reg.Val%" ^| Find /I "%Reg.Val%"' ) Do Call Set STEAMDIR=%%B

:: Change working directory to Source SDK Base 2013 Multiplayer
cd %STEAMDIR%\SteamApps\common\Source SDK Base 2013 Multiplayer\bin
echo %GAMEDIR%
:: Set game executable
SET GAMEEXE="%STEAMDIR%\steamapps\common\Source SDK Base 2013 Multiplayer\hl2.exe"

:: Launch in text mode with the -interpreter option (must be at the back)
%GAMEEXE% -dev -textmode -game %GAMEDIR% -interpreter %*
