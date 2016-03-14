if (-NOT ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator"))
{
	Write-Warning "You do not have Administrator rights to run this script!`nPlease re-run this script as an Administrator!"
	pause
	exit
}
$ErrorActionPreference= 'silentlycontinue'
$path = (Get-ItemProperty "HKLM:\SOFTWARE\Wow6432Node\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 243750" -Name InstallLocation).InstallLocation

if (!$path)
{
	Write-Warning "You should install Source SDK Base 2013 Multiplayer.`nRequesting Steam to install the app..."
    try {
        $cmd ="cmd.exe"
        &$cmd "/c start steam://install/243750/"
    }
    catch {
        Write-Warning "Steam is not running. Can not launch installation pop-up"
    }
	pause
	exit
}

$hl2exe = Join-Path $path hl2.exe
$hl2args = "-game momentum -novid +developer 2 +sv_cheats 1 -console"

$momentum_sym = Join-Path $path momentum
$momentum = [System.IO.Path]::GetFullPath("..\game\momentum")
cmd /c mklink /d $momentum_sym $momentum

"`nSymbolic link created in ..\Source SDK Base 2013 Multiplayer\momentum to ..\game\momentum\"

$data = @"
<?xml version="1.0" encoding="utf-8"?>
<Project ToolsVersion="12.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup Condition="'`$(Configuration)|`$(Platform)'=='Debug|Win32'">
    <LocalDebuggerCommand>$hl2exe</LocalDebuggerCommand>
    <LocalDebuggerCommandArguments>$hl2args</LocalDebuggerCommandArguments>
    <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
  </PropertyGroup>
  <PropertyGroup Condition="'`$(Configuration)|`$(Platform)'=='Release|Win32'">
    <LocalDebuggerCommand>$hl2exe</LocalDebuggerCommand>
    <LocalDebuggerCommandArguments>$hl2args</LocalDebuggerCommandArguments>
    <DebuggerFlavor>WindowsLocalDebugger</DebuggerFlavor>
  </PropertyGroup>
</Project>
"@

$data | Out-File game\client\client_momentum.vcxproj.user utf8

"`nDebug and launch added to client_momentum.vcxproj`n"
