param([ValidateRange(1,7)][int]$Task = 1, [switch]$NoBuild, [switch]$Benchmark)
$ErrorActionPreference = 'Stop'
if (-not $NoBuild) { & "$PSScriptRoot/build.ps1" -Task $Task }
$executable = Join-Path $PSScriptRoot "build/task_$Task.exe"
if (-not (Test-Path $executable)) { throw "Build first: $executable" }
if ($Benchmark -and $Task -ne 5) { throw 'Benchmark is available only for Task 5.' }
if ($Benchmark) { & $executable --benchmark 1000000 }
else { & $executable }
exit $LASTEXITCODE
