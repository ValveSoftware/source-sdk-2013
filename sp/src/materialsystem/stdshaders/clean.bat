@echo off
setlocal

if /i "%1" == "-game" goto CleanGameDir

rem Clean out hl2
if exist ..\..\..\game\hl2\shaders rd /s /q ..\..\..\game\hl2\shaders
goto CleanOtherStuff

:CleanGameDir
set __GameDir=%~2
if not exist "%__GameDir%\gameinfo.txt" goto MissingGameInfo
if exist "%__GameDir%\shaders" rd /s /q "%2\shaders"
goto CleanOtherStuff

:CleanOtherStuff
if exist debug_dx9 rd /s /q debug_dx9

if exist fxctmp9 rd /s /q fxctmp9
if exist vshtmp9 rd /s /q vshtmp9
if exist pshtmp9 rd /s /q pshtmp9
if exist fxctmp9_tmp rd /s /q fxctmp9_tmp
if exist vshtmp9_tmp rd /s /q vshtmp9_tmp
if exist pshtmp9_tmp rd /s /q pshtmp9_tmp
if exist shaders rd /s /q shaders
goto end

:MissingGameInfo
echo Invalid -game parameter specified (no "%__GameDir%\gameinfo.txt" exists).
goto end


:end