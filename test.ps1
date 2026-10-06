param([string]$CompilerPath)
$ErrorActionPreference = 'Stop'
if ($env:OS -ne 'Windows_NT') { throw 'Run these simulated regressions on Windows.' }
if (-not $CompilerPath) {
    $taskBundledCompiler = Join-Path $PSScriptRoot 'compiler\tcc\tcc.exe'
    if (Test-Path -LiteralPath $taskBundledCompiler -PathType Leaf) { $CompilerPath = $taskBundledCompiler }
    else { $CompilerPath = (Get-Command tcc.exe -ErrorAction Stop).Source }
}
$taskCompiler = (Resolve-Path -LiteralPath $CompilerPath -ErrorAction Stop).ProviderPath
$taskResults = Join-Path $PSScriptRoot '.test-output'
[IO.Directory]::CreateDirectory($taskResults) | Out-Null
if (-not (Test-Path -LiteralPath $taskResults -PathType Container)) { throw 'Test output directory was not created.' }
$taskProbes = @(
    @{ Source = 'gesture_release_probe.c'; Report = 'release-rearm-after.json' },
    @{ Source = 'normal_lift_probe.c'; Report = 'normal-lift-regression.json' }
)
foreach ($taskProbe in $taskProbes) {
    $taskExe = Join-Path $taskResults ([IO.Path]::GetFileNameWithoutExtension($taskProbe.Source) + '.exe')
    & $taskCompiler -m64 '-Wl,-subsystem=windows' -o $taskExe (Join-Path $PSScriptRoot $taskProbe.Source) (Join-Path $PSScriptRoot 'user32-extra.def') -luser32 -lkernel32 -ladvapi32
    if ($LASTEXITCODE -ne 0) { throw ('Probe build failed: ' + $taskProbe.Source) }
    $taskRun = Start-Process -FilePath $taskExe -WorkingDirectory $taskResults -WindowStyle Hidden -PassThru -Wait -ErrorAction Stop
    if ($taskRun.ExitCode -ne 0) { throw ('Regression failed: ' + $taskProbe.Source) }
    $taskReport = Get-Content -LiteralPath (Join-Path $taskResults $taskProbe.Report) -Raw -ErrorAction Stop | ConvertFrom-Json
    if ($taskReport.RealInputCalls -ne 0) { throw 'Regression must not inject real input.' }
    if ($taskProbe.Source -eq 'normal_lift_probe.c' -and -not $taskReport.Passed) { throw 'Lift regression did not pass.' }
    if ($taskProbe.Source -eq 'gesture_release_probe.c' -and (-not $taskReport.SimulatedOnly -or $taskReport.DuplicateAfterDeadlineRelease -or $taskReport.DuplicateAfterPartialLift -or $taskReport.DuplicateOnStationaryTail -or -not $taskReport.FreshContactRecovery -or $taskReport.FreshStationaryContactsRestarted -or -not $taskReport.Repeated100FreshGestures)) { throw 'Release regression did not pass.' }
    Write-Output ('PASS: ' + $taskProbe.Source + ' (simulated input only)')
}
