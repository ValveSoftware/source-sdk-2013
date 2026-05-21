@echo off
REM Build Frog Fortress 2 (mod_tf) - uses v143 if v145 toolset is not installed
set MSBUILD=%ProgramFiles%\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe
if not exist "%MSBUILD%" set MSBUILD=%ProgramFiles%\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe
if not exist "%MSBUILD%" set MSBUILD=%ProgramFiles%\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe

cd /d "%~dp0"
"%MSBUILD%" everything.sln /t:client_win64_tf;server_win64_tf;launcher_main_win64_tf /p:Configuration=Release /p:Platform=win64 /p:PlatformToolset=v143 /m /v:minimal
exit /b %ERRORLEVEL%
