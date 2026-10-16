@echo off
setlocal

rem This is for if you are using dynamic shader compilation, and just want the .incs to build.
set DYNAMIC_SHADERS=1

call buildsdkshaders.bat
