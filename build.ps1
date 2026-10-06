param(
    [string]$CompilerPath,
    [switch]$RenderPreview
)
$ErrorActionPreference = 'Stop'
if ($env:OS -ne 'Windows_NT') { throw 'Build on Windows with the x64 Windows TCC compiler.' }
$taskNative = $PSScriptRoot
if (-not $CompilerPath) {
    $taskBundledCompiler = Join-Path $taskNative 'compiler\tcc\tcc.exe'
    if (Test-Path -LiteralPath $taskBundledCompiler -PathType Leaf) { $CompilerPath = $taskBundledCompiler }
    else { $CompilerPath = (Get-Command tcc.exe -ErrorAction Stop).Source }
}
$taskCompiler = (Resolve-Path -LiteralPath $CompilerPath -ErrorAction Stop).ProviderPath
foreach ($taskName in @('GestureHost', 'GestureSettings')) {
    $taskExpectedExe = Join-Path $taskNative ($taskName + '.exe')
    if (@(Get-Process -Name $taskName -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $taskExpectedExe }).Count) {
        throw ('Exit the program built in this directory before rebuilding: ' + $taskName)
    }
}
& (Join-Path $taskNative 'generate-hand.ps1')
& $taskCompiler -m64 '-Wl,-subsystem=windows' -o (Join-Path $taskNative 'GestureHost.exe') (Join-Path $taskNative 'gesture_host.c') (Join-Path $taskNative 'user32-extra.def') -luser32 -lkernel32 -ladvapi32
if ($LASTEXITCODE -ne 0) { throw 'GestureHost build failed.' }
& $taskCompiler -m64 '-Wl,-subsystem=windows' -o (Join-Path $taskNative 'GestureSettings.exe') (Join-Path $taskNative 'settings_ui.c') -luser32 -lgdi32 -lkernel32 -ladvapi32
if ($LASTEXITCODE -ne 0) { throw 'GestureSettings build failed.' }
# Reuse the accepted icon asset instead of generating a second design.
& (Join-Path $taskNative 'Set-SettingsBranding.ps1') -Path (Join-Path $taskNative 'GestureSettings.exe')
& (Join-Path $taskNative 'test.ps1') -CompilerPath $taskCompiler
if ($RenderPreview) {
    $taskRender = Start-Process -FilePath (Join-Path $taskNative 'GestureSettings.exe') -ArgumentList '--render-preview' -WorkingDirectory $taskNative -WindowStyle Hidden -PassThru -Wait -ErrorAction Stop
    if ($taskRender.ExitCode -ne 0) { throw 'Preview rendering failed.' }
    Add-Type -AssemblyName System.Drawing
    $taskImage = [Drawing.Image]::FromFile((Join-Path $taskNative 'ui-preview.bmp'))
    try { $taskImage.Save((Join-Path $taskNative 'ui-preview.png'), [Drawing.Imaging.ImageFormat]::Png) }
    finally { $taskImage.Dispose() }
}
Get-Item -LiteralPath (Join-Path $taskNative 'GestureHost.exe'), (Join-Path $taskNative 'GestureSettings.exe') | Select-Object Name, Length
