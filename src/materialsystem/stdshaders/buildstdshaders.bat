@echo off
setlocal

rem This script builds all the shaders... like you're a real game or something.

set BUILD_SHADER=call buildshaders.bat

set SOURCE_DIR="..\..\"
set GAME_DIR="..\..\..\game\platform"

%BUILD_SHADER% stdshader_dx9_20b       -game %GAME_DIR%
%BUILD_SHADER% stdshader_dx9_20b_new   -game %GAME_DIR%
%BUILD_SHADER% stdshader_dx9_30        -game %GAME_DIR% -force30
