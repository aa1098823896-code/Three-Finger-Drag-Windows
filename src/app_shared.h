#ifndef THREE_FINGER_APP_SHARED_H
#define THREE_FINGER_APP_SHARED_H
#include "app_language.h"
#define APP_HOST_CLASS L"ThreeFingerDrag-Native-Host"
#define APP_UI_CLASS L"ThreeFingerDrag-Native-Settings"
#define APP_TRAY_MENU_CLASS L"ThreeFingerDrag-Native-TrayMenu"
#define APP_MUTEX L"Local\\ThreeFingerDrag-Native-Host"
#define APP_RELOAD (WM_APP+2)
#define APP_QUIT (WM_APP+1)
#define APP_TRAY_CALLBACK (WM_APP+5)
#define APP_TRAY_READY (WM_APP+6)
#define APP_TRAY_OPEN 5001
#define APP_TRAY_EXIT 5002
#define APP_TRAY_GESTURE 5003
#define APP_TRAY_STARTUP 5004
#define APP_RUN_KEY L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"
#define APP_RUN_VALUE L"ThreeFingerDragOnWindowsZH"
static WCHAR app_folder[MAX_PATH],data_folder[MAX_PATH],config_path[MAX_PATH];
static int app_paths(int isolated){
    WCHAR *slash;DWORD n;
    app_language_init();
    n=GetModuleFileNameW(0,app_folder,MAX_PATH);if(!n||n>=MAX_PATH)return 0;
    slash=wcsrchr(app_folder,L'\\');if(!slash)return 0;*slash=0;
    if(isolated)_snwprintf(data_folder,MAX_PATH,L"%s",app_folder);
    else{
        n=GetEnvironmentVariableW(L"LOCALAPPDATA",data_folder,MAX_PATH);
        if(!n||n>MAX_PATH-30)return 0;
        wcscat(data_folder,L"\\ThreeFingerDragNative");
        if(!CreateDirectoryW(data_folder,0)&&GetLastError()!=ERROR_ALREADY_EXISTS)return 0;
    }
    _snwprintf(config_path,MAX_PATH,L"%s\\settings.ini",data_folder);config_path[MAX_PATH-1]=0;
    return 1;
}
static int app_enabled(void){return GetPrivateProfileIntW(L"Gesture",L"Enabled",1,config_path)!=0;}
static int app_save_enabled(int value){return WritePrivateProfileStringW(L"Gesture",L"Enabled",value?L"1":L"0",config_path);}
static int app_launch(const WCHAR *name,const WCHAR *args){
    WCHAR path[MAX_PATH],command[1024];STARTUPINFOW si;PROCESS_INFORMATION pi;
    _snwprintf(path,MAX_PATH,L"%s\\%s",app_folder,name);path[MAX_PATH-1]=0;
    _snwprintf(command,1024,L"\"%s\" %s",path,args?args:L"");command[1023]=0;
    memset(&si,0,sizeof(si));si.cb=sizeof(si);si.dwFlags=STARTF_USESHOWWINDOW;si.wShowWindow=SW_SHOWNORMAL;
    if(!CreateProcessW(path,command,0,0,FALSE,0,0,app_folder,&si,&pi))return 0;
    CloseHandle(pi.hThread);CloseHandle(pi.hProcess);return 1;
}
static int app_startup(void){
    HKEY key;WCHAR current[1024],expected[1024];DWORD bytes=sizeof(current),type;LONG result;
    _snwprintf(expected,1024,L"\"%s\\GestureHost.exe\" --background",app_folder);expected[1023]=0;
    if(RegOpenKeyExW(HKEY_CURRENT_USER,APP_RUN_KEY,0,KEY_QUERY_VALUE,&key)!=ERROR_SUCCESS)return 0;
    result=RegQueryValueExW(key,APP_RUN_VALUE,0,&type,(BYTE*)current,&bytes);RegCloseKey(key);
    if(result!=ERROR_SUCCESS||type!=REG_SZ||bytes>=sizeof(current))return 0;
    current[bytes/sizeof(WCHAR)]=0;return _wcsicmp(current,expected)==0;
}
static int app_save_startup(int value){
    HKEY key;WCHAR command[1024];LONG result;
    if(RegCreateKeyExW(HKEY_CURRENT_USER,APP_RUN_KEY,0,0,0,KEY_SET_VALUE,0,&key,0)!=ERROR_SUCCESS)return 0;
    if(value){
        _snwprintf(command,1024,L"\"%s\\GestureHost.exe\" --background",app_folder);command[1023]=0;
        result=RegSetValueExW(key,APP_RUN_VALUE,0,REG_SZ,(BYTE*)command,(DWORD)((wcslen(command)+1)*sizeof(WCHAR)));
    }else result=RegDeleteValueW(key,APP_RUN_VALUE);
    RegCloseKey(key);return result==ERROR_SUCCESS||(!value&&result==ERROR_FILE_NOT_FOUND);
}
#endif
