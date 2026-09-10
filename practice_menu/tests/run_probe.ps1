param(
    [Parameter(Mandatory=$true)][string]$Script,
    [string]$Rom = 'practice_menu/build/TMC_Practice_Menu_USA_COMFORT_TEST.gba',
    [string]$Name = 'probe',
    [string]$Save = 'work/mgba_test/tmc_final_test.sav',
    [int]$TimeoutSeconds = 120,
    [switch]$Fast
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'hash_file.ps1')
$workspace = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$results = Join-Path $workspace "practice_menu/test_results/$Name"
New-Item -ItemType Directory -Force -Path $results | Out-Null
$romSource = (Resolve-Path -LiteralPath (Join-Path $workspace $Rom)).Path
$testRom = Join-Path $results 'test.gba'
Copy-Item -LiteralPath $romSource -Destination $testRom
$layoutSource = [IO.Path]::ChangeExtension($romSource, '.layout.lua')
if (Test-Path -LiteralPath $layoutSource) { Copy-Item -LiteralPath $layoutSource -Destination (Join-Path $results 'layout.lua') }
Copy-Item -LiteralPath (Join-Path $workspace $Save) -Destination (Join-Path $results 'test.sav')
$scriptPath = (Resolve-Path -LiteralPath (Join-Path $workspace $Script)).Path
$mgba = Join-Path $workspace 'work/mgba_dev/mGBA-build-2026-08-27-win64-9128-c65e8a3d4666b0ea68a01578232452f31b185332/mGBA.exe'
$done = Join-Path $results 'done.txt'
if (Test-Path -LiteralPath $done) { Remove-Item -LiteralPath $done }
$env:TMC_TEST_OUTPUT = $results.Replace('\','/') + '/'
$arguments = @('--script', ('"{0}"' -f $scriptPath), ('"{0}"' -f $testRom))
if ($Fast) { $arguments = @('-1','-C','audioSync=0','-C','videoSync=0','-C','fpsTarget=240','-s','0') + $arguments }
$process = Start-Process -FilePath $mgba -ArgumentList $arguments -PassThru -WindowStyle Hidden
try {
    $deadline = (Get-Date).AddSeconds($TimeoutSeconds)
    while (-not (Test-Path -LiteralPath $done) -and (Get-Date) -lt $deadline) {
        $process.Refresh()
        if ($process.HasExited) { throw 'mGBA exited' }
        Start-Sleep -Milliseconds 250
    }
    if (-not (Test-Path -LiteralPath $done)) { throw "Probe timed out: $results" }
    Get-Content -LiteralPath $done
} finally {
    $process.Refresh()
    if (-not $process.HasExited) { Stop-Process -Id $process.Id; $process.WaitForExit() }
    Remove-Item Env:TMC_TEST_OUTPUT
}
Get-TestFileHash -LiteralPath $testRom,(Join-Path $results 'test.sav') -Algorithm SHA256
