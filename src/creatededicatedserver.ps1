# Use when powershell doesn't let you execute script
# Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass

devtools\bin\vpc.exe /hl2mp /tf /define:SOURCESDK +dedicated /dedicated /mksln dedicated.sln

# Use DEDICATED define within tf sdk project
$vcxprojPath = Resolve-Path ".\game\server\server_win64_srv_tf.vcxproj"
$updated = $false
# Check vcxproj file
$vcxproj = [xml] (Get-Content -Path $vcxprojPath)
foreach ($group in $vcxproj.Project.ItemDefinitionGroup){
	$preprocessor = $group.ClCompile.PreprocessorDefinitions
	# Look for missing DEDICATED preprocessor definition
	if ($preprocessor -notmatch "DEDICATED;") {
		# Append to beginning DEDICATED definition
		$group.ClCompile.PreprocessorDefinitions = "DEDICATED;$($preprocessor)"
		$updated = $true
	}
}

if ($updated){
	# Update vcxproj with added definition for DEDICATED build
	$vcxproj.Save($vcxprojPath)
}