param([ValidateRange(0,7)][int]$Task = 0)
$ErrorActionPreference = 'Stop'
$compiler = $env:CXX
if (-not $compiler) {
    $found = Get-Command g++.exe -ErrorAction SilentlyContinue
    if ($found) { $compiler = $found.Source }
    elseif (Test-Path "$env:USERPROFILE/gcc/bin/g++.exe") { $compiler = "$env:USERPROFILE/gcc/bin/g++.exe" }
    else { throw 'g++ not found. Add MinGW-w64 bin to PATH or set CXX to g++.exe.' }
}
$buildDir = Join-Path $PSScriptRoot 'build'
New-Item -ItemType Directory -Force $buildDir | Out-Null
$numbers = if ($Task -eq 0) { 1..7 } else { @($Task) }
foreach ($number in $numbers) {
    $source = Join-Path $PSScriptRoot "src/task_$number.cpp"
    $target = Join-Path $buildDir "task_$number.exe"
    & $compiler -std=c++17 -O2 -g -Wall -Wextra -Wpedantic -pthread -static -finput-charset=UTF-8 -fexec-charset=UTF-8 $source -o $target
    if ($LASTEXITCODE -ne 0) { throw "Compilation failed: task_$number" }
    Write-Host "Built task_$number.exe"
}
