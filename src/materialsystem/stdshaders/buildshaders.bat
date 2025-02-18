@echo off

set TTEXE=..\..\devtools\bin\timeprecise.exe
if not exist %TTEXE% goto no_ttexe
goto no_ttexe_end

:no_ttexe
set TTEXE=time /t
:no_ttexe_end

echo.
echo ==================== buildshaders %* ==================
%TTEXE% -cur-Q
set tt_start=%ERRORLEVEL%
set tt_chkpt=%tt_start%


REM ****************
REM usage: buildshaders <shaderProjectName>
REM ****************

setlocal
set arg_filename=%1
set shadercompilecommand=ShaderCompile2.exe
set targetdir=shaders
set SrcDirBase=..\..
set shaderDir=shaders
set SDKArgs=-local

if "%1" == "" goto usage
set inputbase=%1

REM ignore -dx9_30
if /i "%6" == "-dx9_30" shift /6

if /i "%6" == "-force30" goto set_force30_arg
goto set_force_end
:set_force30_arg
			set IS30=1
			goto set_force_end
:set_force_end

if /i "%2" == "-game" goto set_mod_args
goto build_shaders

REM ****************
REM USAGE
REM ****************
:usage
echo.
echo "usage: buildshaders <shaderProjectName> [-game] [gameDir if -game was specified] [-source sourceDir]"
echo "       gameDir is where gameinfo.txt is (where it will store the compiled shaders)."
echo "       sourceDir is where the source code is (where it will find scripts and compilers)."
echo "ex   : buildshaders myshaders"
echo "ex   : buildshaders myshaders -game c:\steam\steamapps\sourcemods\mymod -source c:\mymod\src"
goto :end

REM ****************
REM MOD ARGS - look for -game or the vproject environment variable
REM ****************
:set_mod_args

if not exist "..\..\devtools\bin\ShaderCompile2.exe" goto NoShaderCompile
set ChangeToDir=%SrcDirBase%\devtools\bin\

if /i "%4" NEQ "-source" goto NoSourceDirSpecified
set SrcDirBase=%~5

REM ** use the -game parameter to tell us where to put the files
set targetdir=%~3\shaders

if not exist "%~3\gameinfo.txt" goto InvalidGameDirectory

if not exist "%inputbase%.txt" goto InvalidInputFile

goto build_shaders

REM ****************
REM ERRORS
REM ****************
:InvalidGameDirectory
echo Error: "%~3" is not a valid game directory.
echo (The -game directory must have a gameinfo.txt file)
goto end

:InvalidInputFile
echo Error: "%inputbase%.txt" is not a valid file.
goto end

:NoSourceDirSpecified
echo ERROR: If you specify -game on the command line, you must specify -source.
goto usage
goto end

:NoShaderCompile
echo - ERROR: ShaderCompile2.exe doesn't exist in devtools\bin
goto end

REM ****************
REM BUILD SHADERS
REM ****************
:build_shaders

rem echo --------------------------------
rem echo %inputbase%
rem echo --------------------------------
REM make sure that target dirs exist
REM files will be built in these targets and copied to their final destination
if not exist include mkdir include
if not exist %shaderDir% mkdir %shaderDir%
if not exist %shaderDir%\fxc mkdir %shaderDir%\fxc
REM Nuke some files that we will add to later.

set SHVER=20b
if defined IS30 (
	set SHVER=30
)

title %1 %SHVER%

echo Building inc files and worklist for %inputbase%...

echo Building for %SHVER%...

set DYNAMIC=
if "%dynamic_shaders%" == "1" set DYNAMIC=-Dynamic
powershell -NoLogo -ExecutionPolicy Bypass -Command "%SrcDirBase%\devtools\bin\process_shaders.ps1 %DYNAMIC% -Version %SHVER% '%inputbase%.txt'"

REM ****************
REM PC Shader copy
REM Publish the generated files to the output dir using XCOPY
REM This batch file may have been invoked standalone or slaved (master does final smart mirror copy)
REM ****************
:DoXCopy
if not "%dynamic_shaders%" == "1" (
if not exist "%targetdir%" md "%targetdir%"
if not "%targetdir%"=="%shaderDir%" xcopy %shaderDir%\*.* "%targetdir%" /e /y
)
goto end

REM ****************
REM END
REM ****************
:end


%TTEXE% -diff %tt_start%
echo.
