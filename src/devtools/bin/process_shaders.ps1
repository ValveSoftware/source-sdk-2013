[CmdletBinding()]
param (
    [Parameter(Mandatory=$true, ValueFromPipeline=$true)][System.IO.FileInfo]$File,
    [Parameter(Mandatory=$true)][string]$Version,
    [Parameter(Mandatory=$false)][switch]$Dynamic,
    [Parameter(Mandatory=$false)][System.UInt32]$Threads
)

if ($Version -notin @("20b", "30", "40", "41", "50", "51")) {
	return
}

$fileList = $File.OpenText()
while ($null -ne ($line = $fileList.ReadLine())) {
	if ($line -match '^\s*$' -or $line -match '^\s*//') {
		continue
	}

	if ($Dynamic) {
		& "$PSScriptRoot\ShaderCompile2" "-dynamic" "-ver" $Version "-shaderpath" $File.DirectoryName $line
		continue
	}

	if ($Threads -ne 0) {
		& "$PSScriptRoot\ShaderCompile2" "-threads" $Threads "-ver" $Version "-shaderpath" $File.DirectoryName $line
		continue
	}

	& "$PSScriptRoot\ShaderCompile2" "-ver" $Version "-shaderpath" $File.DirectoryName $line
}
$fileList.Close()
