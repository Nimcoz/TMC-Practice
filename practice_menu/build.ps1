param(
    [string]$Rom = 'C:\Users\user\Downloads\Legend of Zelda, The - The Minish Cap (USA).gba',
    [string]$Save = '',
    [switch]$Autotest
)

$ErrorActionPreference = 'Stop'
$Project = Split-Path -Parent $MyInvocation.MyCommand.Path
$ToolBin = Join-Path $Project 'toolchain\bin'
$Gcc = Join-Path $ToolBin 'arm-none-eabi-gcc.exe'
$Objcopy = Join-Path $ToolBin 'arm-none-eabi-objcopy.exe'
$Nm = Join-Path $ToolBin 'arm-none-eabi-nm.exe'
if (-not (Test-Path -LiteralPath $Gcc)) {
    $Gcc = (Get-Command 'arm-none-eabi-gcc' -ErrorAction Stop).Source
    $Objcopy = (Get-Command 'arm-none-eabi-objcopy' -ErrorAction Stop).Source
    $Nm = (Get-Command 'arm-none-eabi-nm' -ErrorAction Stop).Source
}
$Build = Join-Path $Project 'build'
$OutputName = if ($Autotest) { 'TMC_Practice_USA_SCENES_autotest.gba' } else { 'TMC_Practice_USA_SCENES_TEST.gba' }
$Output = Join-Path $Build $OutputName
$Exploration = Join-Path (Split-Path -Parent $Project) 'KNOWN_GOOD_FINAL_EXPLORATION\outputs\FINAL_verified_respawn_guard_payload.bin'
if ([string]::IsNullOrWhiteSpace($Save)) {
    $WorkspaceSave = Join-Path (Split-Path -Parent $Project) 'work\mgba_test\tmc_final_test.sav'
    if (Test-Path -LiteralPath $WorkspaceSave) { $Save = $WorkspaceSave }
} elseif (-not (Test-Path -LiteralPath $Save)) {
    throw "Save file not found: $Save"
}

New-Item -ItemType Directory -Force -Path $Build | Out-Null
python (Join-Path $Project 'tools/generate_actor_names.py') --source (Join-Path (Split-Path -Parent $Project) 'work/tmc') --output (Join-Path $Build 'actor_names.h')
if ($LASTEXITCODE -ne 0) { throw 'actor name generation failed' }
python (Join-Path $Project 'tools/generate_warp_metadata.py') --source (Join-Path (Split-Path -Parent $Project) 'work/tmc') --output (Join-Path $Build 'warp_metadata.h')
if ($LASTEXITCODE -ne 0) { throw 'warp metadata generation failed' }
python (Join-Path $Project 'tools/generate_completion_flags.py') --source (Join-Path (Split-Path -Parent $Project) 'work/tmc') --gcc $Gcc --output (Join-Path $Build 'completion_flags.h')
if ($LASTEXITCODE -ne 0) { throw 'completion flag generation failed' }
$Objects = @()
$Common = @(
    '-mthumb', '-mcpu=arm7tdmi', '-Os', '-std=c11', '-ffreestanding', '-fno-builtin',
    '-ffunction-sections', '-fdata-sections', '-fomit-frame-pointer', '-Wall', '-Wextra',
    '-Wno-int-to-pointer-cast', '-Wno-pointer-to-int-cast', '-I', (Join-Path $Project 'include'), '-I', $Build
)

Get-ChildItem -LiteralPath (Join-Path $Project 'src') -Recurse -Filter '*.c' | Sort-Object FullName | ForEach-Object {
    $Relative = $_.FullName.Substring((Join-Path $Project 'src').Length + 1)
    $ObjectName = ($Relative -replace '[\\/]', '_') -replace '\.c$', '.o'
    $Object = Join-Path $Build $ObjectName
    & $Gcc @Common -c $_.FullName -o $Object
    if ($LASTEXITCODE -ne 0) { throw "compile failed: $Relative" }
    $Objects += $Object
}

$Elf = Join-Path $Build 'practice_menu.elf'
$Map = Join-Path $Build 'practice_menu.map'
& $Gcc '-mthumb' '-mcpu=arm7tdmi' '-nostdlib' "-Wl,-T,$(Join-Path $Project 'linker.ld'),-Map,$Map,--gc-sections" @Objects '-lgcc' '-o' $Elf
if ($LASTEXITCODE -ne 0) { throw 'link failed' }

$Module = Join-Path $Build 'practice_menu.bin'
$Symbols = Join-Path $Build 'practice_menu.sym'
& $Objcopy '-O' 'binary' $Elf $Module
if ($LASTEXITCODE -ne 0) { throw 'objcopy failed' }
& $Nm '-n' $Elf | Set-Content -LiteralPath $Symbols -Encoding ascii

$BuildArguments = @('--rom', $Rom, '--module', $Module, '--symbols', $Symbols, '--exploration', $Exploration, '--output', $Output)
if (-not [string]::IsNullOrWhiteSpace($Save)) { $BuildArguments += @('--save', $Save) }
if ($Autotest) { $BuildArguments += '--autotest' }
python (Join-Path $Project 'tools\build_rom.py') @BuildArguments
if ($LASTEXITCODE -ne 0) { throw 'ROM patch build failed' }

Write-Host $Output
