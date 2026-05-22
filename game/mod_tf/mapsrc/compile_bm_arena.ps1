# Compile bm_arena.vmf -> game/mod_tf/maps/bm_arena.bsp
# Requires: Source SDK Base 2013 Multiplayer (Steam -> Library -> Tools)

$ErrorActionPreference = 'Stop'

$Map = 'bm_arena'
$ModDir = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$Vmf = Join-Path $PSScriptRoot "$Map.vmf"
$OutBsp = Join-Path $ModDir "maps\$Map.bsp"

if (-not (Test-Path $Vmf)) { throw "Missing VMF: $Vmf" }

$Prog86 = ${env:ProgramFiles(x86)}
$Candidates = @(
    (Join-Path $Prog86 'Steam\steamapps\common\Source SDK Base 2013 Multiplayer\bin'),
    (Join-Path $Prog86 'Steam\steamapps\common\Source SDK Base 2013 Singleplayer\bin')
)

$SdkBin = $Candidates | Where-Object { Test-Path (Join-Path $_ 'vbsp.exe') } | Select-Object -First 1
if (-not $SdkBin) {
    Write-Host ''
    Write-Host 'ERROR: vbsp.exe not found.'
    Write-Host 'Install Steam tool: Source SDK Base 2013 Multiplayer'
    Write-Host 'See mapsrc/COMPILE_MAP_SETUP.txt'
    Write-Host ''
    foreach ($c in $Candidates) { Write-Host "  checked: $c" }
    exit 1
}

$Vbsp = Join-Path $SdkBin 'vbsp.exe'
$Vvis = Join-Path $SdkBin 'vvis.exe'
$Vrad = Join-Path $SdkBin 'vrad.exe'

$WorkDir = Join-Path $env:TEMP "bm_arena_compile_$([Guid]::NewGuid().ToString('N').Substring(0,8))"
New-Item -ItemType Directory -Force -Path $WorkDir | Out-Null
Copy-Item $Vmf (Join-Path $WorkDir "$Map.vmf") -Force

Write-Host "Compiling $Map"
Write-Host "  SDK:  $SdkBin"
Write-Host "  game: $ModDir"

Push-Location $WorkDir
try {
    Write-Host '[1/3] vbsp...'
    & $Vbsp -game $ModDir "$Map.vmf"
    if ($LASTEXITCODE -ne 0) { throw 'vbsp failed' }

    Write-Host '[2/3] vvis...'
    & $Vvis -game $ModDir $Map
    if ($LASTEXITCODE -ne 0) { throw 'vvis failed' }

    Write-Host '[3/3] vrad...'
    & $Vrad -game $ModDir -both $Map
    if ($LASTEXITCODE -ne 0) { throw 'vrad failed' }
}
finally {
    Pop-Location
}

$BspSrc = Join-Path $WorkDir "$Map.bsp"
if (-not (Test-Path $BspSrc)) { throw "No BSP output in $WorkDir" }

New-Item -ItemType Directory -Force -Path (Join-Path $ModDir 'maps') | Out-Null
Copy-Item $BspSrc $OutBsp -Force
Remove-Item -Recurse -Force $WorkDir -ErrorAction SilentlyContinue

Write-Host "OK: $OutBsp"
Write-Host 'In-game: ff_play bomber'
