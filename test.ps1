param([string]$CompilerPath)
$ErrorActionPreference = 'Stop'
if ($env:OS -ne 'Windows_NT') { throw 'Run these simulated regressions on Windows.' }
if (-not $CompilerPath) {
    $taskBundledCompiler = Join-Path $PSScriptRoot 'compiler\tcc\tcc.exe'
    if (Test-Path -LiteralPath $taskBundledCompiler -PathType Leaf) { $CompilerPath = $taskBundledCompiler }
    else { $CompilerPath = (Get-Command tcc.exe -ErrorAction Stop).Source }
}
$taskCompiler = (Resolve-Path -LiteralPath $CompilerPath -ErrorAction Stop).ProviderPath
$taskSource = Join-Path $PSScriptRoot 'src'
$taskTests = Join-Path $PSScriptRoot 'tests'
$taskResults = Join-Path $PSScriptRoot 'build/tests'
[IO.Directory]::CreateDirectory($taskResults) | Out-Null
if (-not (Test-Path -LiteralPath $taskResults -PathType Container)) { throw 'Test output directory was not created.' }
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'assets/Gesture.ico') -Destination (Join-Path $taskResults 'Gesture.ico') -ErrorAction Stop
$taskProbes = @(
    @{ Source = 'gesture_release_probe.c'; Report = 'release-rearm-after.json' },
    @{ Source = 'normal_lift_probe.c'; Report = 'normal-lift-regression.json' },
    @{ Source = 'ai_prompt_probe.c'; Report = 'ai-prompt-privacy.json' },
    @{ Source = 'window_geometry_probe.c'; Report = 'window-geometry-regression.json' }
)
foreach ($taskProbe in $taskProbes) {
    $taskExe = Join-Path $taskResults ([IO.Path]::GetFileNameWithoutExtension($taskProbe.Source) + '.exe')
    & $taskCompiler -m64 '-Wl,-subsystem=windows' -I $taskSource -o $taskExe (Join-Path $taskTests $taskProbe.Source) (Join-Path $taskSource 'user32-extra.def') -luser32 -lgdi32 -lkernel32 -ladvapi32
    if ($LASTEXITCODE -ne 0) { throw ('Probe build failed: ' + $taskProbe.Source) }
    $taskRun = Start-Process -FilePath $taskExe -WorkingDirectory $taskResults -WindowStyle Hidden -PassThru -Wait -ErrorAction Stop
    if ($taskRun.ExitCode -ne 0) { throw ('Regression failed: ' + $taskProbe.Source) }
    $taskReport = Get-Content -LiteralPath (Join-Path $taskResults $taskProbe.Report) -Raw -ErrorAction Stop | ConvertFrom-Json
    if ($taskReport.RealInputCalls -ne 0) { throw 'Regression must not inject real input.' }
    if ($taskProbe.Source -eq 'normal_lift_probe.c' -and -not $taskReport.Passed) { throw 'Lift regression did not pass.' }
    if ($taskProbe.Source -eq 'window_geometry_probe.c' -and (-not $taskReport.Passed -or -not $taskReport.PrivateDesktop -or -not $taskReport.StationaryCornerStable -or -not $taskReport.MonotonicResize -or -not $taskReport.CrossMonitorAnchorPreserved -or -not $taskReport.AspectRatioPreserved -or -not $taskReport.MinimumSizePreserved -or $taskReport.MaxAnchorDriftPixels -gt 1)) { throw 'Window geometry regression did not pass.' }
    if ($taskProbe.Source -eq 'ai_prompt_probe.c' -and (-not $taskReport.Passed -or -not $taskReport.UserClipboardUntouched -or -not $taskReport.NoPersonalPaths -or -not $taskReport.NoPathNeeded -or -not $taskReport.DifferentLocalPathsProduceSamePrompt -or -not $taskReport.UnavailableDetectionShownAsUnknown -or $taskReport.PromptCharacters -gt 300)) { throw 'Prompt privacy regression did not pass.' }
    if ($taskProbe.Source -eq 'gesture_release_probe.c' -and (-not $taskReport.SimulatedOnly -or $taskReport.DuplicateAfterDeadlineRelease -or $taskReport.DuplicateAfterPartialLift -or $taskReport.DuplicateOnStationaryTail -or -not $taskReport.FreshContactRecovery -or $taskReport.FreshStationaryContactsRestarted -or -not $taskReport.Repeated100FreshGestures)) { throw 'Release regression did not pass.' }
    Write-Output ('PASS: ' + $taskProbe.Source + ' (no real input, user clipboard unchanged)')
}
