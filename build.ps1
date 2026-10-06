param(
    [string]$CompilerPath,
    [switch]$RenderPreview
)
$ErrorActionPreference = 'Stop'
if ($env:OS -ne 'Windows_NT') { throw 'Build on Windows with the x64 Windows TCC compiler.' }
$taskNative = $PSScriptRoot
$taskSource = Join-Path $taskNative 'src'
$taskOutput = Join-Path $taskNative 'build'
[IO.Directory]::CreateDirectory($taskOutput) | Out-Null
if (-not (Test-Path -LiteralPath $taskOutput -PathType Container)) { throw 'Build output directory was not created.' }
if (-not $CompilerPath) {
    $taskBundledCompiler = Join-Path $taskNative 'compiler\tcc\tcc.exe'
    if (Test-Path -LiteralPath $taskBundledCompiler -PathType Leaf) { $CompilerPath = $taskBundledCompiler }
    else { $CompilerPath = (Get-Command tcc.exe -ErrorAction Stop).Source }
}
$taskCompiler = (Resolve-Path -LiteralPath $CompilerPath -ErrorAction Stop).ProviderPath
foreach ($taskName in @('GestureHost', 'GestureSettings')) {
    $taskExpectedExe = Join-Path $taskOutput ($taskName + '.exe')
    if (@(Get-Process -Name $taskName -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $taskExpectedExe }).Count) {
        throw ('Exit the program built in this directory before rebuilding: ' + $taskName)
    }
}
& (Join-Path $taskNative 'scripts/generate-hand.ps1')
& (Join-Path $taskNative 'scripts/generate-language.ps1')
& $taskCompiler -m64 '-Wl,-subsystem=windows' -I $taskSource -o (Join-Path $taskOutput 'GestureHost.exe') (Join-Path $taskSource 'gesture_host.c') (Join-Path $taskSource 'user32-extra.def') -luser32 -lkernel32 -ladvapi32
if ($LASTEXITCODE -ne 0) { throw 'GestureHost build failed.' }
& $taskCompiler -m64 '-Wl,-subsystem=windows' -I $taskSource -o (Join-Path $taskOutput 'GestureSettings.exe') (Join-Path $taskSource 'settings_ui.c') -luser32 -lgdi32 -lkernel32 -ladvapi32
if ($LASTEXITCODE -ne 0) { throw 'GestureSettings build failed.' }
# Reuse the accepted icon asset instead of generating a second design.
Copy-Item -LiteralPath (Join-Path $taskNative 'assets/Gesture.ico') -Destination (Join-Path $taskOutput 'Gesture.ico') -ErrorAction Stop
& (Join-Path $taskNative 'scripts/Set-SettingsBranding.ps1') -Path (Join-Path $taskOutput 'GestureSettings.exe')
& (Join-Path $taskNative 'test.ps1') -CompilerPath $taskCompiler
if ($RenderPreview) {
    $taskRender = Start-Process -FilePath (Join-Path $taskOutput 'GestureSettings.exe') -ArgumentList '--render-preview' -WorkingDirectory $taskOutput -WindowStyle Hidden -PassThru -Wait -ErrorAction Stop
    if ($taskRender.ExitCode -ne 0) { throw 'Preview rendering failed.' }
    Add-Type -AssemblyName System.Drawing
    $taskImage = [Drawing.Image]::FromFile((Join-Path $taskOutput 'ui-preview.bmp'))
    try { $taskImage.Save((Join-Path $taskOutput 'ui-preview.png'), [Drawing.Imaging.ImageFormat]::Png) }
    finally { $taskImage.Dispose() }
}
Get-Item -LiteralPath (Join-Path $taskOutput 'GestureHost.exe'), (Join-Path $taskOutput 'GestureSettings.exe') | Select-Object Name, Length
