@echo off
setlocal

set TTEXE=..\..\devtools\bin\timeprecise.exe
if not exist %TTEXE% goto no_ttexe
goto no_ttexe_end

:no_ttexe
set TTEXE=time /t
:no_ttexe_end


rem echo.
rem echo ~~~~~~ buildallshaders %* ~~~~~~
%TTEXE% -cur-Q
set tt_all_start=%ERRORLEVEL%
set tt_all_chkpt=%tt_start%



set sourcedir="shaders"
set targetdir="..\..\..\game\hl2\shaders"

set BUILD_SHADER=call buildshaders.bat

set ARG_X360=-x360
set ARG_EXTRA=



REM ****************
REM usage: buildallshaders [-pc | -x360]
REM ****************
set ALLSHADERS_CONFIG=pc


REM ****************
REM PC SHADERS
REM ****************

set SOURCE_DIR="..\..\"
set GAME_DIR="..\..\..\game\hl2"

%BUILD_SHADER% misylshaders_dx9_20b -game %GAME_DIR% -source %SOURCE_DIR%
%BUILD_SHADER% misylshaders_dx9_30 -game %GAME_DIR% -source %SOURCE_DIR% -force30

REM ****************
REM END
REM ****************
:end



rem echo.
if not "%dynamic_shaders%" == "1" (
  rem echo Finished full buildallshaders %*
) else (
  rem echo Finished dynamic buildallshaders %*
)

rem %TTEXE% -diff %tt_all_start% -cur
rem echo.
