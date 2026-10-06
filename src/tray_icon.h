#ifndef THREE_FINGER_TRAY_ICON_H
#define THREE_FINGER_TRAY_ICON_H
/* SDK-compatible Unicode NOTIFYICONDATA layout, including Windows 7 fields. */
typedef struct {
    DWORD size; HWND window; UINT id,flags,callback; HICON icon;
    WCHAR tip[128]; DWORD state,state_mask; WCHAR info[256]; UINT version;
    WCHAR title[64]; DWORD info_flags; GUID guid; HICON balloon_icon;
} APP_NOTIFY_ICON;
typedef BOOL (WINAPI *APP_SHELL_NOTIFY)(DWORD,APP_NOTIFY_ICON *);
static HMODULE tray_library;
static APP_SHELL_NOTIFY tray_notify;
static APP_NOTIFY_ICON tray_data;
static int tray_registered,tray_state=-1;
static UINT tray_taskbar_created;
static void tray_tooltip(void){
    const WCHAR *value=last_error?L"三指拖拽 · 已安全停止":(!enabled?L"三指拖拽 · 已暂停":(!device_n?L"三指拖拽 · 等待触控板":L"三指拖拽 · 正在运行"));
    wcscpy(tray_data.tip,value);
}
static int tray_ensure(void){
    if(!production||!tray_notify||!tray_data.icon)return 0;
    if(tray_registered)return 1;
    tray_tooltip();tray_data.version=4;
    tray_registered=tray_notify(0,&tray_data)!=0;
    if(tray_registered){tray_notify(4,&tray_data);tray_state=-1;}
    return tray_registered;
}
static int tray_init(void){
    WCHAR path[MAX_PATH];static const GUID icon_guid={0x3df9c6a7,0x8bcc,0x4c72,{0x9d,0x70,0x66,0x25,0x8d,0xfb,0x30,0xec}};
    tray_library=LoadLibraryExW(L"shell32.dll",0,0x00000800);if(!tray_library)return 0;
    tray_notify=(void*)GetProcAddress(tray_library,"Shell_NotifyIconW");if(!tray_notify)return 0;
    memset(&tray_data,0,sizeof(tray_data));tray_data.size=sizeof(tray_data);tray_data.window=host;tray_data.id=1;
    tray_data.flags=1|2|4|32|128; /* MESSAGE | ICON | TIP | GUID | SHOWTIP */
    tray_data.callback=APP_TRAY_CALLBACK;tray_data.guid=icon_guid;
    _snwprintf(path,MAX_PATH,L"%s\\Gesture.ico",app_folder);path[MAX_PATH-1]=0;
    tray_data.icon=LoadImageW(0,path,IMAGE_ICON,32,32,LR_LOADFROMFILE);
    tray_taskbar_created=RegisterWindowMessageW(L"TaskbarCreated");return tray_ensure();
}
static void tray_update(void){
    int state=last_error?3:(!enabled?1:(!device_n?2:0));
    if(!production)return;if(!tray_registered&&!tray_ensure())return;
    if(state!=tray_state){tray_tooltip();tray_notify(1,&tray_data);tray_state=state;}
}
static void tray_action(UINT id){
    if(id==APP_TRAY_OPEN){
        HWND settings=FindWindowW(APP_UI_CLASS,0);
        if(settings){ShowWindow(settings,SW_RESTORE);SetForegroundWindow(settings);}else app_launch(L"GestureSettings.exe",L"");
    }else if(id==APP_TRAY_EXIT)PostMessageW(host,APP_QUIT,0,0);
}
static void tray_context_menu(WPARAM anchor){
    /* Native menu/theme caches die with this short-lived settings entry. */
    app_launch(L"GestureSettings.exe",L"--tray-menu");
}
static void tray_cleanup(void){
    if(tray_registered&&tray_notify)tray_notify(2,&tray_data);tray_registered=0;
    if(tray_data.icon){DestroyIcon(tray_data.icon);tray_data.icon=0;}
    if(tray_library){FreeLibrary(tray_library);tray_library=0;tray_notify=0;}
}
#endif
