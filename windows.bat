@echo off
setlocal enabledelayedexpansion

:input
echo Enter 'sp' or 'mp' to move the corresponding folder to 'C:\vortexmod':
set /p folderType=

if /i "%folderType%"=="sp" (
    set folderName=sp
) else if /i "%folderType%"=="mp" (
    set folderName=mp
) else (
    echo Invalid input. Please enter either 'sp' or 'mp'.
    goto input
)

if exist "%folderName%" (
    if not exist "C:\vortexmod" (
        mkdir "C:\vortexmod"
    )
    move "%folderName%" "C:\vortexmod"
    echo %folderName% has been moved to C:\vortexmod.
) else (
    echo The folder %folderName% does not exist.
)

pause
