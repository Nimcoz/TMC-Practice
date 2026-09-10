param([int]$Parallel=3,
    [string]$Rom='practice_menu/build/TMC_Practice_Menu_USA_EXPANSION_TEST.gba',
    [string]$Prefix='expansion_verified_')
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'hash_file.ps1')
$workspace=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$rom=$Rom
$romHash=(Get-TestFileHash -LiteralPath (Join-Path $workspace $rom)).Hash
$cases=@(
    @('features','polish_gameplay.lua'), @('collection_viewer','collection_viewer_gameplay.lua'),
    @('camera','camera_interiors_gameplay.lua'), @('native_save','native_save_gameplay.lua'),
    @('regression','feature_gameplay.lua'), @('movement','movement_exploration_gameplay.lua'),
    @('hud','hud_dialog_gameplay.lua'), @('resources','resource_cheats_gameplay.lua'),
    @('combat','combat_inventory_gameplay.lua'), @('physics','protected_physics_gameplay.lua'),
    @('held','held_items_gameplay.lua'), @('fight','fight_freeze_gameplay.lua'),
    @('ui','ui_pages_gameplay.lua'), @('beta','expansion_beta.lua'),
    @('scene','expansion_scene_skip.lua'), @('scene_baseline','expansion_scene_skip.lua'),
    @('skip_guards_final','expansion_skip_guards.lua'),
    @('settings_bad_primary','expansion_settings_fallback.lua','practice_menu/test_results/expansion_settings_fixtures/bad_primary.sav'),
    @('settings_bad_both','expansion_settings_fallback.lua','practice_menu/test_results/expansion_settings_fixtures/bad_both.sav')
)
$active=@();$next=0;$failed=@()
try {
    while ($next -lt $cases.Count -or $active.Count) {
        while ($next -lt $cases.Count -and $active.Count -lt $Parallel) {
            $case=$cases[$next];$next++
            $save=if($case.Count -gt 2){$case[2]}else{'work/mgba_test/tmc_final_test.sav'}
            $job=Start-Job -Name $case[0] -ArgumentList $workspace,$case[0],$case[1],$rom,$save,$Prefix -ScriptBlock {
                param($root,$name,$script,$rom,$save,$prefix)
                Set-Location -LiteralPath $root
                & (Join-Path $root 'practice_menu/tests/run_probe.ps1') -Script ('practice_menu/tests/'+$script) -Name ($prefix+$name) -Rom $rom -Save $save -TimeoutSeconds 600
            }
            $active+= $job
            Write-Output ('START '+$case[0])
        }
        $null=Wait-Job -Job $active -Any -Timeout 1
        foreach($job in @($active | Where-Object {$_.State -in @('Completed','Failed','Stopped')})) {
            Receive-Job $job -ErrorAction Continue | Out-Null
            $dir=Join-Path $workspace ('practice_menu/test_results/'+$Prefix+$job.Name)
            $passed=$false
            if(Test-Path -LiteralPath (Join-Path $dir 'done.txt')) {
                $result=(Get-Content -LiteralPath (Join-Path $dir 'done.txt') -Raw).Trim()
                $passed=$result -eq 'GAMEPLAY_CHECKS failures=0' -and (Get-TestFileHash -LiteralPath (Join-Path $dir 'test.gba')).Hash -eq $romHash
            }
            Write-Output ((@{True='PASS ';False='FAIL '}[$passed.ToString()])+$job.Name)
            if(-not $passed){$failed+=$job.Name}
            $active=@($active | Where-Object {$_.Id -ne $job.Id})
            Remove-Job $job
        }
    }
} finally {
    foreach($job in $active){Stop-Job $job;Remove-Job $job}
}
if($failed.Count){throw ('Failed suites: '+($failed -join ', '))}
Write-Output ('REGRESSION PASS '+$cases.Count+' suites on '+$romHash)
