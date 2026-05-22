# Compile bm_arena.vmf -> game/mod_tf/maps/bm_arena.bsp

$ErrorActionPreference = 'Stop'

$Map = 'bm_arena'

$ModDir = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path

$Vmf = Join-Path $PSScriptRoot "$Map.vmf"

$SdkBin = Join-Path ${env:ProgramFiles(x86)} 'Steam\steamapps\common\Source SDK Base 2013 Multiplayer\bin'

$Vbsp = Join-Path $SdkBin 'vbsp.exe'



if (-not (Test-Path $Vmf)) { throw "Missing VMF: $Vmf" }

if (-not (Test-Path $Vbsp)) {

    throw "vbsp not found at $Vbsp. Install Source SDK Base 2013 Multiplayer (Steam Tools)."

}



$TempDir = Join-Path $env:TEMP "bm_arena_compile"

New-Item -ItemType Directory -Force -Path $TempDir | Out-Null

Copy-Item $Vmf (Join-Path $TempDir "$Map.vmf") -Force



Push-Location $TempDir

& $Vbsp -game $ModDir "$Map.vmf"

if ($LASTEXITCODE -ne 0) { throw 'vbsp failed' }

& (Join-Path $SdkBin 'vvis.exe') -game $ModDir "$Map.vmf"

if ($LASTEXITCODE -ne 0) { throw 'vvis failed' }

& (Join-Path $SdkBin 'vrad.exe') -game $ModDir -both "$Map.vmf"

if ($LASTEXITCODE -ne 0) { throw 'vrad failed' }

Pop-Location



$BspSrc = Join-Path $TempDir "$Map.bsp"

if (-not (Test-Path $BspSrc)) { throw 'No BSP output' }



$MapsDir = Join-Path $ModDir 'maps'

New-Item -ItemType Directory -Force -Path $MapsDir | Out-Null

$BspDst = Join-Path $MapsDir "$Map.bsp"

Copy-Item $BspSrc $BspDst -Force

Write-Host "OK: $BspDst"

Write-Host "In-game: ff_play bomber"

