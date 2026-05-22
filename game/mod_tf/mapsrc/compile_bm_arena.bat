@echo off
setlocal EnableExtensions
set "MAP=bm_arena"
set "MODDIR=%~dp0.."
set "VMF=%~dp0%MAP%.vmf"

if not exist "%VMF%" goto :no_vmf

set "PROG86=%ProgramFiles(x86)%"
set "SDKBIN=%PROG86%\Steam\steamapps\common\Source SDK Base 2013 Multiplayer\bin"
set "VBSP=%SDKBIN%\vbsp.exe"
set "VVIS=%SDKBIN%\vvis.exe"
set "VRAD=%SDKBIN%\vrad.exe"

dir /b "%VBSP%" >nul 2>&1
if errorlevel 1 goto :no_vbsp

set "GAME=%MODDIR%"
echo Compiling %MAP% for mod_tf...
echo   game: %GAME%
echo   vmf:  %VMF%

copy /Y "%VMF%" "%TEMP%\%MAP%.vmf" >nul
pushd "%TEMP%"
call "%VBSP%" -game "%GAME%" %MAP%.vmf
if errorlevel 1 goto :fail
call "%VVIS%" -game "%GAME%" %MAP%.vmf
if errorlevel 1 goto :fail
call "%VRAD%" -game "%GAME%" -both %MAP%.vmf
if errorlevel 1 goto :fail
popd

dir /b "%TEMP%\%MAP%.bsp" >nul 2>&1
if errorlevel 1 goto :no_bsp

if not exist "%MODDIR%\maps" mkdir "%MODDIR%\maps"
copy /Y "%TEMP%\%MAP%.bsp" "%MODDIR%\maps\%MAP%.bsp"
echo.
echo OK: %MODDIR%\maps\%MAP%.bsp
echo Launch mod_tf and run: ff_play bomber
exit /b 0

:no_vmf
echo ERROR: missing %VMF%
exit /b 1

:no_vbsp
echo ERROR: vbsp not found at:
echo   %VBSP%
echo Install Source SDK Base 2013 Multiplayer from Steam Library Tools
exit /b 1

:no_bsp
echo ERROR: compile produced no BSP
exit /b 1

:fail
popd
echo Compile failed.
exit /b 1
