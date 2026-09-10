param([int]$Parallel=3,
    [string]$Rom='practice_menu/build/TMC_Practice_Menu_USA_SPAWNER_TEST.gba',
    [string]$Prefix='spawner_verified_',
    [string]$AutotestRom='practice_menu/build/TMC_Practice_Menu_USA_autotest.gba',
    [switch]$Expert,
    [switch]$Named,
    [switch]$Travel,
    [switch]$Access,
    [switch]$Scenes,
    [switch]$Resume,
    [switch]$CatalogOnly)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'hash_file.ps1')
$workspace=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$rom=$Rom
$cases=@(
 @('actors','actors_gameplay.lua'),@('actor_guards','actors_guards.lua'),@('actor_interiors','actors_interiors.lua'),
 @('actor_navigation','actors_navigation.lua'),
 @('catalog_overworld','spawner_catalog.lua'),@('catalog_house','spawner_catalog.lua'),
 @('catalog_temple','spawner_catalog.lua'),@('catalog_castle','spawner_catalog.lua'),
 @('completion','expansion_completion.lua'),@('settings_figures','expansion_settings_figures.lua'),
 @('comfort','comfort_gameplay.lua'),@('undo','comfort_undo.lua'),@('save_undo_final','comfort_save_roundtrip.lua'),
 @('settings_bad_crc','expansion_settings_fallback.lua','practice_menu/test_results/expansion_settings_fixtures/bad_crc.sav'),
 @('v2_bad_primary','comfort_settings_fallback.lua','practice_menu/test_results/comfort_settings_fixtures/bad_primary.sav'),
 @('v2_bad_both','comfort_settings_fallback.lua','practice_menu/test_results/comfort_settings_fixtures/bad_both.sav'),
 @('v2_bad_crc','comfort_settings_fallback.lua','practice_menu/test_results/comfort_settings_fixtures/bad_crc.sav'),
 @('v2_bad_keys','comfort_settings_fallback.lua','practice_menu/test_results/comfort_settings_fixtures/bad_keys.sav'),
 @('v2_bad_room','comfort_settings_fallback.lua','practice_menu/test_results/comfort_settings_fixtures/bad_room.sav'),
 @('internal','internal_assertions.lua','work/mgba_test/tmc_final_test.sav',$AutotestRom)
)
if($Expert) {
 $cases=@($cases | Where-Object { $_[0] -ne 'actor_guards' })
 $cases+=@(@('expert_actors','expert_actors.lua'),@('expert_break_free','expert_break_free.lua'),
           @('expert_minigame','expert_minigame.lua'),@('expert_bosses','expert_bosses.lua'),
           @('expert_guards','expert_guards.lua'))
}
if($CatalogOnly) { $cases=@($cases | Where-Object { $_[0] -like 'catalog_*' }) }
if($Named) { $cases+=@(@('named_names','named_names.lua'),@('named_timers','named_timers.lua')) }
if($Travel) { $cases+=@(@('actor_travel','actor_travel.lua'),@('actor_travel_live','actor_travel_live.lua'),@('element_recognition','element_recognition.lua')) }
if($Access) { $cases+=@(@('access_modal','access_modal.lua'),@('access_sequences','access_sequences.lua'),@('access_views','access_views.lua')) }
if($Scenes) { $cases=@(@('scene_replays','scene_replays.lua'),@('scene_full','scene_full.lua'))+$cases }
if($Resume) {
 $cases=@($cases | Where-Object {
  $dir=Join-Path $workspace ('practice_menu/test_results/'+$Prefix+$_[0])
  $target=if($_.Count -gt 3){$_[3]}else{$rom}
  $donePath=Join-Path $dir 'done.txt';$testPath=Join-Path $dir 'test.gba'
  $passed=$false
  if((Test-Path -LiteralPath $donePath) -and (Test-Path -LiteralPath $testPath)) {
   $result=(Get-Content -LiteralPath $donePath -Raw).Trim()
   $passed=($result -eq 'GAMEPLAY_CHECKS failures=0' -or ($_[0] -eq 'internal' -and $result -match '^PASS .*failed=00000000')) -and
    (Get-TestFileHash -LiteralPath $testPath).Hash -eq (Get-TestFileHash -LiteralPath (Join-Path $workspace $target)).Hash
  }
  -not $passed
 })
}
$active=@();$next=0;$failed=@()
try {
 while($next -lt $cases.Count -or $active.Count) {
  while($next -lt $cases.Count -and $active.Count -lt $Parallel) {
   $case=$cases[$next];$next++
   $save=if($case.Count -gt 2){$case[2]}else{'work/mgba_test/tmc_final_test.sav'}
   $targetRom=if($case.Count -gt 3){$case[3]}else{$rom}
   $job=Start-Job -Name $case[0] -ArgumentList $workspace,$case[0],$case[1],$targetRom,$save,$Prefix -ScriptBlock {
    param($root,$name,$script,$rom,$save,$prefix)
    Set-Location -LiteralPath $root
    & (Join-Path $root 'practice_menu/tests/run_probe.ps1') -Script ('practice_menu/tests/'+$script) -Name ($prefix+$name) -Rom $rom -Save $save -TimeoutSeconds 600
   }
   $active+=$job;Write-Output ('START '+$case[0])
  }
  $null=Wait-Job -Job $active -Any -Timeout 1
  foreach($job in @($active|Where-Object {$_.State -in @('Completed','Failed','Stopped')})) {
   Receive-Job $job -ErrorAction Continue | Out-Null
   $dir=Join-Path $workspace ('practice_menu/test_results/'+$Prefix+$job.Name)
   $result=if(Test-Path -LiteralPath (Join-Path $dir 'done.txt')){(Get-Content -LiteralPath (Join-Path $dir 'done.txt') -Raw).Trim()}else{'MISSING'}
   $expectedRom=if($job.Name -eq 'internal'){$AutotestRom}else{$rom}
   $passed=($result -eq 'GAMEPLAY_CHECKS failures=0' -or ($job.Name -eq 'internal' -and $result -match '^PASS .*failed=00000000')) -and (Get-TestFileHash -LiteralPath (Join-Path $dir 'test.gba')).Hash -eq (Get-TestFileHash -LiteralPath (Join-Path $workspace $expectedRom)).Hash
   Write-Output ((@{True='PASS ';False='FAIL '}[$passed.ToString()])+$job.Name)
   if(-not $passed){$failed+=$job.Name}
   $active=@($active|Where-Object {$_.Id -ne $job.Id});Remove-Job $job
  }
 }
} finally {foreach($job in $active){Stop-Job $job;Remove-Job $job}}
if($failed.Count){throw ('Failed suites: '+($failed -join ', '))}
Write-Output 'ACTORS EXTRA PASS'
