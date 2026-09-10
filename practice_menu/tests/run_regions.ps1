param([ValidateSet('EU','JP','Both')][string]$Region='Both',
      [ValidateRange(1,3)][int]$Parallel=3,[string[]]$Only=@(),[switch]$Resume)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot 'hash_file.ps1')
$workspace=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$regions=if($Region -eq 'Both'){@('EU','JP')}else{@($Region)}
$cases=@();$hashes=@{}
foreach($r in $regions) {
 foreach($c in (Get-Content -LiteralPath (Join-Path $workspace "practice_menu/build/regions/$r/test_cases.json") -Raw|ConvertFrom-Json)) {
  if($Only.Count -and $c.name -notin $Only){continue}
  $c|Add-Member -NotePropertyName region -NotePropertyValue $r
  $c|Add-Member -NotePropertyName resultName -NotePropertyValue ('region_'+$r.ToLower()+'_verified_'+$c.name)
  if(-not $hashes.ContainsKey($c.rom)){$hashes[$c.rom]=(Get-TestFileHash -LiteralPath (Join-Path $workspace $c.rom)).Hash}
  $dir=Join-Path $workspace ('practice_menu/test_results/'+$c.resultName)
  if($Resume -and (Test-Path -LiteralPath (Join-Path $dir 'done.txt')) -and (Test-Path -LiteralPath (Join-Path $dir 'test.gba'))) {
   $text=(Get-Content -LiteralPath (Join-Path $dir 'done.txt') -Raw).Trim()
   if(($text -eq 'GAMEPLAY_CHECKS failures=0' -or ($c.name -eq 'internal' -and $text -match '^PASS ')) -and
      (Get-TestFileHash -LiteralPath (Join-Path $dir 'test.gba')).Hash -eq $hashes[$c.rom]) {Write-Output ('REUSE PASS '+$c.resultName);continue}
  }
  $cases+=$c
 }
}
$next=0;$active=@();$failed=@();$results=@()
try {
 while($next -lt $cases.Count -or $active.Count) {
  while($next -lt $cases.Count -and $active.Count -lt $Parallel) {
   $c=$cases[$next];$next++
   $job=Start-Job -Name $c.resultName -ArgumentList $workspace,$c -ScriptBlock {
    param($root,$case)
    Set-Location -LiteralPath $root
    & (Join-Path $root 'practice_menu/tests/run_probe.ps1') -Script $case.script -Rom $case.rom -Save $case.save -Name $case.resultName -TimeoutSeconds 900 -Fast
   }
   $active+=@{job=$job;case=$c};Write-Output ('START '+$c.resultName)
  }
  $null=Wait-Job -Job @($active|ForEach-Object {$_.job}) -Any -Timeout 1
  foreach($entry in @($active|Where-Object {$_.job.State -in @('Completed','Failed','Stopped')})) {
   $job=$entry.job;$c=$entry.case
   Receive-Job $job -ErrorAction Continue | Out-Null
   $dir=Join-Path $workspace ('practice_menu/test_results/'+$c.resultName)
   $text=if(Test-Path -LiteralPath (Join-Path $dir 'done.txt')){(Get-Content -LiteralPath (Join-Path $dir 'done.txt') -Raw).Trim()}else{'MISSING'}
   $pass=($text -eq 'GAMEPLAY_CHECKS failures=0' -or ($c.name -eq 'internal' -and $text -match '^PASS ')) -and
         (Get-TestFileHash -LiteralPath (Join-Path $dir 'test.gba')).Hash -eq $hashes[$c.rom]
   $label=if($pass){'PASS '}else{'FAIL '}
   Write-Output ($label+$c.resultName+' '+$text)
   $results+=@{name=$c.resultName;passed=$pass;result=$text;rom_sha256=$hashes[$c.rom]}
   if(-not $pass){$failed+=$c.resultName}
   $active=@($active|Where-Object {$_.job.Id -ne $job.Id});Remove-Job $job
  }
 }
} finally {foreach($entry in $active){Stop-Job $entry.job;Remove-Job $entry.job}}
$results|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $workspace "practice_menu/build/regions/last_run_$Region.json") -Encoding utf8
if($failed.Count){throw ('Failed suites: '+($failed -join ', '))}
Write-Output 'REGIONAL REGRESSION PASS'
