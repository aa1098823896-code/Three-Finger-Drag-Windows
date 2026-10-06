param(
    [string]$InstallDirectory,
    [switch]$NoLaunch
)
$ErrorActionPreference='Stop'
if($env:OS -ne 'Windows_NT' -or -not [Environment]::Is64BitOperatingSystem){throw 'Windows 11 x64 is required.'}
$taskArchitecture=$env:PROCESSOR_ARCHITEW6432
if(-not $taskArchitecture){$taskArchitecture=$env:PROCESSOR_ARCHITECTURE}
if($taskArchitecture -ne 'AMD64'){throw 'This package is for x64 Windows, not ARM64.'}
$taskBuild=[int](Get-ItemProperty -LiteralPath 'HKLM:\SOFTWARE\Microsoft\Windows NT\CurrentVersion' -Name CurrentBuildNumber -ErrorAction Stop).CurrentBuildNumber
if($taskBuild -lt 26100){throw 'Windows 11 24H2 or later is required.'}
if(-not $InstallDirectory){$InstallDirectory=Join-Path $env:LOCALAPPDATA 'Programs\ThreeFingerDragNative'}
$taskTarget=[IO.Path]::GetFullPath($InstallDirectory)
if([IO.Path]::GetPathRoot($taskTarget).TrimEnd('\') -eq $taskTarget.TrimEnd('\')){throw 'Do not install at a drive root.'}
$taskPackage=[IO.Path]::GetFullPath($PSScriptRoot)
if($taskPackage -eq $taskTarget){throw 'Extract the package outside the installed directory before updating.'}
$taskManifest=Get-Content -LiteralPath (Join-Path $taskPackage 'release-manifest.json') -Raw -ErrorAction Stop | ConvertFrom-Json
if($taskManifest.version -ne 'v1.0.0' -or $taskManifest.platform -ne 'windows-x64'){throw 'Unexpected release manifest.'}
$taskNames=@('GestureHost.exe','GestureSettings.exe','Gesture.ico')
foreach($taskName in $taskNames){
 $taskEntry=@($taskManifest.files|Where-Object {$_.name -eq $taskName})
 if($taskEntry.Count -ne 1){throw ('Missing or duplicate manifest entry: '+$taskName)}
 $taskSource=Join-Path $taskPackage $taskName
 if((Get-Item -LiteralPath $taskSource -ErrorAction Stop).Length -ne $taskEntry[0].bytes -or (Get-FileHash -LiteralPath $taskSource -Algorithm SHA256 -ErrorAction Stop).Hash -ne $taskEntry[0].sha256){throw ('Package verification failed: '+$taskName)}
}
Add-Type -TypeDefinition @'
using System;using System.Text;using System.Runtime.InteropServices;
public static class ThreeFingerDragInstall {
 [StructLayout(LayoutKind.Sequential)]public struct Rect{public int Left,Top,Right,Bottom;}
 [DllImport("user32.dll",CharSet=CharSet.Unicode)]static extern IntPtr FindWindow(string name,string title);
 public static IntPtr Window(bool host){return FindWindow(host?"ThreeFingerDrag-Native-Host":"ThreeFingerDrag-Native-Settings",null);}
 [DllImport("user32.dll")]public static extern uint GetWindowThreadProcessId(IntPtr hwnd,out uint process);
 [DllImport("user32.dll")]public static extern bool PostMessage(IntPtr hwnd,uint message,IntPtr a,IntPtr b);
 [DllImport("user32.dll")]public static extern bool GetWindowRect(IntPtr hwnd,out Rect rect);
 [DllImport("user32.dll")]public static extern bool IsWindowVisible(IntPtr hwnd);
 [DllImport("user32.dll")]public static extern bool IsIconic(IntPtr hwnd);
 [DllImport("user32.dll")]public static extern bool SetWindowPos(IntPtr hwnd,IntPtr after,int x,int y,int width,int height,uint flags);
 [DllImport("user32.dll")]public static extern bool ShowWindow(IntPtr hwnd,int command);
 [DllImport("user32.dll")]public static extern IntPtr GetForegroundWindow();
 [DllImport("user32.dll")]public static extern bool SetForegroundWindow(IntPtr hwnd);
 [DllImport("user32.dll")]public static extern IntPtr GetShellWindow();
 [DllImport("user32.dll",CharSet=CharSet.Unicode)]public static extern IntPtr GetProp(IntPtr hwnd,string key);
 [DllImport("user32.dll")]public static extern IntPtr SendMessageTimeout(IntPtr hwnd,uint message,IntPtr a,IntPtr b,uint flags,uint timeout,out IntPtr result);
 [DllImport("kernel32.dll",CharSet=CharSet.Unicode)]static extern uint GetPrivateProfileString(string section,string key,string fallback,StringBuilder result,uint size,string file);
 public static string Enabled(string file){var result=new StringBuilder(32);GetPrivateProfileString("Gesture","Enabled","1",result,32,file);return result.ToString();}
}
'@ -ErrorAction Stop
function Get-TaskWindow([bool]$taskHost){
 $taskWindow=[ThreeFingerDragInstall]::Window($taskHost)
 if($taskWindow -ne [IntPtr]::Zero){
  [uint32]$taskOwner=0;[void][ThreeFingerDragInstall]::GetWindowThreadProcessId($taskWindow,[ref]$taskOwner)
  $taskExpected=Join-Path $taskTarget $(if($taskHost){'GestureHost.exe'}else{'GestureSettings.exe'})
  if((Get-Process -Id $taskOwner -ErrorAction Stop).Path -ne $taskExpected){throw 'Another copy is running. Quit that copy from its tray menu before installing.'}
 }
 return $taskWindow
}
function Stop-TaskApps {
 foreach($taskHost in @($false,$true)){
  $taskWindow=Get-TaskWindow $taskHost
  if($taskWindow -ne [IntPtr]::Zero){
   [uint32]$taskOwner=0;[void][ThreeFingerDragInstall]::GetWindowThreadProcessId($taskWindow,[ref]$taskOwner)
   $taskProcess=Get-Process -Id $taskOwner -ErrorAction Stop
   [void][ThreeFingerDragInstall]::PostMessage($taskWindow,$(if($taskHost){0x8001}else{0x8003}),[IntPtr]::Zero,[IntPtr]::Zero)
   if(-not $taskProcess.WaitForExit(5000)){throw 'The running app did not exit. No process will be force-terminated.'}
  }
 }
}
function Start-TaskUI {
 # Ask the real Explorer desktop to launch; an Agent terminal may own a short-lived job.
 $taskShell=New-Object -ComObject Shell.Application
 $taskDesktopHandle=0
 $taskDesktop=$taskShell.Windows().FindWindowSW(0,0,8,[ref]$taskDesktopHandle,1)
 if(-not $taskDesktop){throw 'The Explorer desktop is unavailable. Open GestureSettings.exe yourself after installation.'}
 $taskDesktop.Document.Application.ShellExecute((Join-Path $taskTarget 'GestureSettings.exe'),'--hidden-check',$taskTarget,'open',0)
 $taskDeadline=[DateTime]::UtcNow.AddSeconds(10)
 do{
  $taskWindow=Get-TaskWindow $false
  $taskHost=Get-TaskWindow $true
  [IntPtr]$taskTrayReady=[IntPtr]::Zero
  if($taskWindow -ne [IntPtr]::Zero -and [ThreeFingerDragInstall]::GetProp($taskWindow,'ThreeFingerDrag-AlphaSurface') -ne [IntPtr]::Zero -and $taskHost -ne [IntPtr]::Zero){
   [void][ThreeFingerDragInstall]::SendMessageTimeout($taskHost,0x8006,[IntPtr]::Zero,[IntPtr]::Zero,2,250,[ref]$taskTrayReady)
   if($taskTrayReady -ne [IntPtr]::Zero){return $taskWindow}
  }
  Start-Sleep -Milliseconds 50
 }while([DateTime]::UtcNow -lt $taskDeadline)
 throw 'The installed window and tray did not become ready.'
}
$taskOldWindow=if($NoLaunch){[IntPtr]::Zero}else{Get-TaskWindow $false}
$taskOldHost=if($NoLaunch){[IntPtr]::Zero}else{Get-TaskWindow $true}
$taskVisible=[ThreeFingerDragInstall]::IsWindowVisible($taskOldWindow)
$taskMinimized=[ThreeFingerDragInstall]::IsIconic($taskOldWindow)
$taskForeground=[ThreeFingerDragInstall]::GetForegroundWindow()
$taskRect=New-Object ThreeFingerDragInstall+Rect
if($taskOldWindow -ne [IntPtr]::Zero -and -not [ThreeFingerDragInstall]::GetWindowRect($taskOldWindow,[ref]$taskRect)){throw 'Current window placement could not be read.'}
$taskConfig=Join-Path $env:LOCALAPPDATA 'ThreeFingerDragNative\settings.ini'
$taskEnabled=[ThreeFingerDragInstall]::Enabled($taskConfig)
$taskRunKey='HKCU:\Software\Microsoft\Windows\CurrentVersion\Run'
$taskRunName='ThreeFingerDragOnWindowsZH'
$taskRunObject=Get-ItemProperty -LiteralPath $taskRunKey -ErrorAction Stop
$taskRunBefore=$taskRunObject.PSObject.Properties[$taskRunName]
$taskHadStartup=$null -ne $taskRunBefore
$taskRunValue=if($taskHadStartup){[string]$taskRunBefore.Value}else{$null}
$taskExisting=Test-Path -LiteralPath (Join-Path $taskTarget 'GestureHost.exe') -PathType Leaf
if(Test-Path -LiteralPath $taskTarget -PathType Leaf){throw 'The installation path is a file.'}
[IO.Directory]::CreateDirectory($taskTarget)|Out-Null
if(-not(Test-Path -LiteralPath $taskTarget -PathType Container)){throw 'Installation directory was not created.'}
$taskBackup=Join-Path $taskTarget ('.previous\'+[DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss')+'-'+[guid]::NewGuid().ToString('N').Substring(0,8))
[IO.Directory]::CreateDirectory($taskBackup)|Out-Null
$taskFiles=@('GestureHost.exe','GestureSettings.exe','Gesture.ico','Install.ps1','release-manifest.json','LICENSE','LICENSE.upstream.txt')
foreach($taskName in $taskFiles){$taskOld=Join-Path $taskTarget $taskName;if(Test-Path -LiteralPath $taskOld -PathType Leaf){Copy-Item -LiteralPath $taskOld -Destination (Join-Path $taskBackup $taskName) -ErrorAction Stop}}
if(Test-Path -LiteralPath $taskConfig -PathType Leaf){Copy-Item -LiteralPath $taskConfig -Destination (Join-Path $taskBackup 'settings.ini') -ErrorAction Stop}
$taskStarted=$false;$taskStopped=$false;$taskStartupChanged=$false
try{
 if(-not $NoLaunch){Stop-TaskApps;$taskStopped=$true}
 foreach($taskName in $taskFiles){$taskSource=Join-Path $taskPackage $taskName;if(Test-Path -LiteralPath $taskSource -PathType Leaf){Copy-Item -LiteralPath $taskSource -Destination (Join-Path $taskTarget $taskName) -Force -ErrorAction Stop}}
 foreach($taskName in $taskNames){$taskExpected=($taskManifest.files|Where-Object {$_.name -eq $taskName}).sha256;if((Get-FileHash -LiteralPath (Join-Path $taskTarget $taskName) -Algorithm SHA256 -ErrorAction Stop).Hash -ne $taskExpected){throw 'Installed file verification failed.'}}
 if(-not $NoLaunch){
  if(-not $taskExisting -and -not $taskHadStartup){New-ItemProperty -LiteralPath $taskRunKey -Name $taskRunName -PropertyType String -Value ('"'+(Join-Path $taskTarget 'GestureHost.exe')+'" --background') -ErrorAction Stop|Out-Null;$taskStartupChanged=$true}
  $taskStarted=$true;$taskNewWindow=Start-TaskUI
  if($taskVisible -and -not $taskMinimized){
   for($taskStep=0;$taskStep -lt 2;$taskStep++){[void][ThreeFingerDragInstall]::SetWindowPos($taskNewWindow,[IntPtr]::Zero,$taskRect.Left,$taskRect.Top,$taskRect.Right-$taskRect.Left,$taskRect.Bottom-$taskRect.Top,0x14)}
   [void][ThreeFingerDragInstall]::SetWindowPos($taskNewWindow,[IntPtr]::Zero,0,0,0,0,0x57)
  }elseif($taskMinimized){[void][ThreeFingerDragInstall]::ShowWindow($taskNewWindow,7)}
  elseif(-not $taskExisting){[void][ThreeFingerDragInstall]::ShowWindow($taskNewWindow,5);[void][ThreeFingerDragInstall]::SetForegroundWindow($taskNewWindow)}
  if($taskForeground -eq $taskOldWindow -and $taskOldWindow -ne [IntPtr]::Zero){[void][ThreeFingerDragInstall]::SetForegroundWindow($taskNewWindow)}
  if([ThreeFingerDragInstall]::Enabled($taskConfig) -ne $taskEnabled){throw 'The previous drag setting was not preserved.'}
  if($taskHadStartup -and (Get-ItemProperty -LiteralPath $taskRunKey -Name $taskRunName -ErrorAction Stop).$taskRunName -ne $taskRunValue){throw 'The previous startup setting was not preserved.'}
 }
 [pscustomobject]@{Installed=$true;Version=$taskManifest.version;FilesVerified=$true;Launched=(-not $NoLaunch);SettingsPreserved=$true;InstallDirectory=$taskTarget;BackupDirectory=$taskBackup}
}catch{
 $taskFailure=$_
 if($taskStarted){Stop-TaskApps}
 foreach($taskName in $taskFiles){$taskOld=Join-Path $taskBackup $taskName;if(Test-Path -LiteralPath $taskOld -PathType Leaf){Copy-Item -LiteralPath $taskOld -Destination (Join-Path $taskTarget $taskName) -Force -ErrorAction Stop}}
 if($taskStartupChanged){Remove-ItemProperty -LiteralPath $taskRunKey -Name $taskRunName -ErrorAction Stop}
 if($taskStopped -and (Test-Path -LiteralPath (Join-Path $taskBackup 'settings.ini') -PathType Leaf)){Copy-Item -LiteralPath (Join-Path $taskBackup 'settings.ini') -Destination $taskConfig -Force -ErrorAction Stop}
 if($taskStopped -and $taskOldHost -ne [IntPtr]::Zero -and $taskExisting){[void](Start-TaskUI)}
 throw $taskFailure
}
