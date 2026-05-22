@echo off
setlocal EnableExtensions
set "MAP=bm_arena"
set "MODDIR=%~dp0.."
set "VMF=%~dp0%MAP%.vmf"
set "OUTBSP=%MODDIR%\maps\%MAP%.bsp"

if not exist "%VMF%" (
	echo ERROR: missing %VMF%
	exit /b 1
)

set "SDKBIN="
set "PROG86=%ProgramFiles(x86)%"

if exist "%PROG86%\Steam\steamapps\common\Source SDK Base 2013 Multiplayer\bin\vbsp.exe" (
	set "SDKBIN=%PROG86%\Steam\steamapps\common\Source SDK Base 2013 Multiplayer\bin"
)
if not defined SDKBIN (
	if exist "%PROG86%\Steam\steamapps\common\Source SDK Base 2013 Singleplayer\bin\vbsp.exe" (
		set "SDKBIN=%PROG86%\Steam\steamapps\common\Source SDK Base 2013 Singleplayer\bin"
	)
)

if not defined SDKBIN (
	echo.
	echo ERROR: vbsp.exe not found.
	echo Install Steam tool: Source SDK Base 2013 Multiplayer
	echo See mapsrc\COMPILE_MAP_SETUP.txt
	exit /b 1
)

set "VBSP=%SDKBIN%\vbsp.exe"
set "VVIS=%SDKBIN%\vvis.exe"
set "VRAD=%SDKBIN%\vrad.exe"
set "GAME=%MODDIR%"

echo Compiling %MAP% for mod_tf...
echo   SDK:  %SDKBIN%
echo   game: %GAME%

if not exist "%MODDIR%\maps" mkdir "%MODDIR%\maps"

set "WORKDIR=%TEMP%\bm_arena_%RANDOM%"
mkdir "%WORKDIR%" 2>nul
copy /Y "%VMF%" "%WORKDIR%\%MAP%.vmf" >nul
pushd "%WORKDIR%"

echo [1/3] vbsp...
"%VBSP%" -game "%GAME%" %MAP%.vmf
if errorlevel 1 goto :fail

echo [2/3] vvis...
"%VVIS%" -game "%GAME%" %MAP%
if errorlevel 1 goto :fail

echo [3/3] vrad...
"%VRAD%" -game "%GAME%" -both %MAP%
if errorlevel 1 goto :fail

popd

if not exist "%WORKDIR%\%MAP%.bsp" (
	echo ERROR: no BSP in %WORKDIR%
	rmdir /s /q "%WORKDIR%" 2>nul
	exit /b 1
)

copy /Y "%WORKDIR%\%MAP%.bsp" "%OUTBSP%"
rmdir /s /q "%WORKDIR%" 2>nul

echo.
echo OK: %OUTBSP%
echo In-game: ff_play bomber
exit /b 0

:fail
popd
echo Compile FAILED - see COMPILE_MAP_SETUP.txt
exit /b 1
