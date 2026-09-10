param([string]$RomDirectory='C:/Users/user/Downloads',[string]$ToolchainBin='')
$ErrorActionPreference='Stop'
$project=$PSScriptRoot
python (Join-Path $project 'tools/map_regions.py') --downloads $RomDirectory
if($LASTEXITCODE -ne 0){throw 'Region mapping failed'}
foreach($region in 'EU','JP') {
 $argsForBuild=@((Join-Path $project 'tools/build_region.py'),'--region',$region,'--downloads',$RomDirectory)
 if($ToolchainBin){$argsForBuild+=@('--toolchain',$ToolchainBin)}
 python @argsForBuild
 if($LASTEXITCODE -ne 0){throw "Build failed: $region"}
}
python (Join-Path $project 'tools/verify_regions.py') --downloads $RomDirectory
if($LASTEXITCODE -ne 0){throw 'Independent verification failed'}
Write-Output 'EU AND JP BUILDS: INDEPENDENT STATIC VERIFICATION PASS'
