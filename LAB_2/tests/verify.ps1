param([switch]$SkipBuild)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
if (-not $SkipBuild) { & "$root/build.ps1" }
$compiler = $env:CXX
if (-not $compiler) {
    $found = Get-Command g++.exe -ErrorAction SilentlyContinue
    if ($found) { $compiler = $found.Source } else { $compiler = "$env:USERPROFILE/gcc/bin/g++.exe" }
}
& $compiler -std=c++17 -O2 -Wall -Wextra -Wpedantic -pthread -static "$PSScriptRoot/common_tests.cpp" -o "$root/build/common_tests.exe"
if ($LASTEXITCODE -ne 0) { throw 'Test compilation failed' }
$script:log = [System.Collections.Generic.List[string]]::new()
$script:checks = 0
function Check([bool]$condition, [string]$message) {
    if (-not $condition) { throw "FAIL: $message" }
    $script:checks++
    $script:log.Add("PASS: $message")
}
function Run([string]$name, [string]$inputText = '', [string]$arguments = '', [int]$expectedExit = 0) {
    $start = [Diagnostics.ProcessStartInfo]::new()
    $start.FileName = "$root/build/$name.exe"
    $start.Arguments = $arguments
    $start.UseShellExecute = $false
    $start.CreateNoWindow = $true
    $start.RedirectStandardInput = $true
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    $start.StandardOutputEncoding = [Text.Encoding]::UTF8
    $start.StandardErrorEncoding = [Text.Encoding]::UTF8
    $process = [Diagnostics.Process]::new()
    $process.StartInfo = $start
    $watch = [Diagnostics.Stopwatch]::StartNew()
    [void]$process.Start()
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    $process.StandardInput.Write($inputText)
    $process.StandardInput.Close()
    if (-not $process.WaitForExit(30000)) {
        $process.Kill()
        $process.WaitForExit()
        throw "TIMEOUT: $name"
    }
    $watch.Stop()
    $output = $stdout.Result
    $errors = $stderr.Result
    Check ($process.ExitCode -eq $expectedExit) "$name exit code $expectedExit"
    $script:log.Add("`n--- $name $arguments | input: $($inputText.Replace("`n",' ')) | elapsed_ms=$($watch.ElapsedMilliseconds) ---`n$output$errors")
    $process.Dispose()
    return @{ Text = $output; Milliseconds = $watch.ElapsedMilliseconds }
}
try {
    $null = Run 'common_tests'
    foreach ($number in @(1,4)) {
        $r = Run "task_$number" "-5 0 1 2 3 4 17 25 97 q`n"
        Check ($r.Text -match ': 2 3 17 97\r?\n') "task_$number prime filtering"
        $r = Run "task_$number" "q`n"
        Check ($r.Text -notmatch ': [0-9]') "task_$number empty list"
        $r = Run "task_$number" "abc 2 12abc 7 9223372036854775808 q`n"
        Check ($r.Text -match ': 2 7\r?\n') "task_$number invalid tokens skipped"
        $r = Run "task_$number" '2 11'
        Check ($r.Text -match ': 2 11\r?\n') "task_$number EOF publishes data"
        $null = Run "task_$number"
    }
    foreach ($iteration in 1..10) {
        $r = Run 'task_2'
        Check (([regex]::Matches($r.Text, 'i=1')).Count -eq 4) "task_2 run $iteration three completions"
        $second = $r.Text.IndexOf('Awake:', $r.Text.IndexOf('Awake:') + 1)
        Check ($second -ge 0 -and $r.Text.Substring(0,$second) -notmatch 'i=1') "task_2 run $iteration no early completion"
        $r = Run 'task_3'
        # Unicode escapes keep this script compatible with Windows PowerShell 5.1 without BOM.
        $message = [regex]::Unescape('\u041f\u043e\u0432\u0456\u0434\u043e\u043c\u043b\u0435\u043d\u043d\u044f \u0437 \u043f\u043e\u0442\u043e\u043a\u0443')
        Check (([regex]::Matches($r.Text, "$message [123]")).Count -eq 1) "task_3 run $iteration exactly one message"
    }
    $r = Run 'task_5' "10`n1`n3`n"
    Check (([regex]::Matches($r.Text,'prime\(10\)=29')).Count -eq 2) 'task_5 both policies'
    Check ($r.Text -match 'sqrt\(n\)=' -and $r.Text -match 'ln\(n\)=') 'task_5 function choices'
    $r = Run 'task_5' "0 -2 abc 10000001 1`n2`n0`n"
    Check (([regex]::Matches($r.Text,'prime\(1\)=2')).Count -eq 2) 'task_5 invalid n recovery and sin'
    $null = Run 'task_5' '' '--benchmark 0' 1
    $null = Run 'task_5' "q`n"
    foreach ($iteration in 1..3) {
        $r = Run 'task_5' '' '--benchmark 1000000'
        Check (([regex]::Matches($r.Text,'prime=15485863')).Count -eq 2) "benchmark $iteration millionth prime"
    }
    $r = Run 'task_6' "1 2 10 100 q`n"
    foreach ($expected in @('prime(1)=2','prime(2)=3','prime(10)=29','prime(100)=541')) {
        Check ($r.Text.Contains($expected)) "task_6 $expected"
    }
    $null = Run 'task_6' "q`n"
    $null = Run 'task_6'
    $r = Run 'task_6' '0 abc -3 10000001 10'
    Check ($r.Text.Contains('prime(10)=29')) 'task_6 invalid n and EOF'
    $r = Run 'task_6' ((1..50 -join ' ') + ' q')
    Check (([regex]::Matches($r.Text,'prime\(')).Count -eq 50 -and $r.Text.Contains('prime(50)=229')) 'task_6 drains 50 queued jobs'
    $r = Run 'task_7' "10`n"
    Check ($r.Text.Contains('prime(10)=29') -and $r.Text.Contains('prime(100)=541') -and $r.Text.Contains('sqrt(10)=3.16228')) 'task_7 all three results'
    Check ($r.Milliseconds -ge 1900) 'task_7 waits for two-second worker'
    $r = Run 'task_7' "0 abc 1000001 1`n"
    Check ($r.Text.Contains('prime(1)=2') -and $r.Text.Contains('prime(10)=29')) 'task_7 range validation'
    $null = Run 'task_7' "q`n"
    $null = Run 'task_7'
    $script:log.Add("`nALL CHECKS PASSED: $script:checks")
    $script:log | Set-Content -Encoding UTF8 "$root/docs/verification.txt"
    Write-Host "ALL CHECKS PASSED: $script:checks; docs/verification.txt"
} catch {
    $script:log.Add($_.ToString())
    $script:log | Set-Content -Encoding UTF8 "$root/docs/verification.txt"
    throw
}
