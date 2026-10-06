$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
$taskCodes=@('zh-CN','zh-TW','en','ja','ko','de','fr','es','pt')
$taskCatalogs=@{}
foreach($taskCode in $taskCodes){$taskCatalogs[$taskCode]=Get-Content -LiteralPath (Join-Path $taskRoot ('locales/'+$taskCode+'.json')) -Raw -ErrorAction Stop | ConvertFrom-Json -AsHashtable -ErrorAction Stop}
$taskKeys=@($taskCatalogs['zh-CN'].Keys)
$taskHostKeys=@('HOST_WINDOW_NAME','TOOLTIP_RUNNING','TOOLTIP_PAUSED','TOOLTIP_WAITING','TOOLTIP_STOPPED')
foreach($taskCode in $taskCodes){
 $taskCatalog=$taskCatalogs[$taskCode]
 if(@(Compare-Object $taskKeys @($taskCatalog.Keys)).Count){throw ('Translation keys differ: '+$taskCode)}
 foreach($taskKey in $taskKeys){
  if(-not ($taskCatalog[$taskKey] -is [string]) -or -not $taskCatalog[$taskKey].Length){throw ('Empty translation: '+$taskCode+'/'+$taskKey)}
  $taskExpected=[regex]::Matches($taskCatalogs['zh-CN'][$taskKey],'%[sd]').Value -join ','
  $taskActual=[regex]::Matches($taskCatalog[$taskKey],'%[sd]').Value -join ','
  if($taskExpected -ne $taskActual){throw ('Format placeholders differ: '+$taskCode+'/'+$taskKey)}
  if($taskKey -like 'STATUS_*' -and $taskCatalog[$taskKey].Length -gt 23){throw ('Status too long: '+$taskCode+'/'+$taskKey)}
  if(($taskKey -like 'FOOTER_*' -or $taskKey -like 'COPY_*') -and $taskCatalog[$taskKey].Length -gt 179){throw ('Notice too long: '+$taskCode+'/'+$taskKey)}
 }
}
function Convert-CString([string]$taskValue){'L"'+$taskValue.Replace('\','\\').Replace('"','\"').Replace("`r",'\r').Replace("`n",'\n')+'"'}
$taskLines=New-Object 'System.Collections.Generic.List[string]'
$taskLines.Add('/* Generated from locales/*.json. Do not edit this table directly. */')
$taskLines.Add('#ifndef THREE_FINGER_APP_STRINGS_H')
$taskLines.Add('#define THREE_FINGER_APP_STRINGS_H')
foreach($taskMode in @('HOST','UI')){
 $taskLines.Add($(if($taskMode -eq 'HOST'){'#ifdef APP_LANGUAGE_HOST'}else{'#else'}))
 $taskSelected=if($taskMode -eq 'HOST'){$taskHostKeys}else{$taskKeys}
 $taskLines.Add('enum APP_TEXT_ID { '+(($taskSelected | ForEach-Object {'APP_TEXT_'+$_}) -join ',')+', APP_TEXT_COUNT };')
 $taskLines.Add('static const WCHAR *const app_strings[9][APP_TEXT_COUNT]={')
 foreach($taskCode in $taskCodes){$taskLines.Add(' {'+(($taskSelected | ForEach-Object {Convert-CString $taskCatalogs[$taskCode][$_]}) -join ',')+'},')}
 $taskLines.Add('};')
}
$taskLines.Add('#endif')
$taskLines.Add('#endif')
[IO.File]::WriteAllText((Join-Path $taskRoot 'src/generated/app_strings.h'),($taskLines -join "`r`n")+"`r`n",(New-Object Text.UTF8Encoding $false))
