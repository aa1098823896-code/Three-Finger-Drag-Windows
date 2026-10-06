/* Exercises the shipping window procedure on an invisible private desktop.
   Cursor coordinates are fixtures; no real input or user settings are changed. */
#define UNICODE
#define _UNICODE
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app_shared.h"
#undef APP_UI_CLASS
#define APP_UI_CLASS L"ThreeFingerDrag-GeometryProbe"
static POINT geometry_cursor;
static int geometry_result=1,geometry_worker;
static WNDPROC geometry_settings_proc;
static LRESULT CALLBACK geometry_proc(HWND,UINT,WPARAM,LPARAM);
static BOOL WINAPI geometry_get_cursor(LPPOINT point){*point=geometry_cursor;return TRUE;}
static int geometry_paths(int unused){return app_paths(1);}
static ATOM WINAPI geometry_register(const WNDCLASSW *input){
    WNDCLASSW wc=*input;geometry_settings_proc=wc.lpfnWndProc;wc.lpfnWndProc=geometry_proc;return RegisterClassW(&wc);
}
#define GetCursorPos geometry_get_cursor
#define app_paths geometry_paths
#define RegisterClassW geometry_register
#define WinMain geometry_settings_main
#include "settings_ui.c"
#undef WinMain
#undef RegisterClassW
#undef app_paths
#undef GetCursorPos
static HMONITOR geometry_monitors[8];
static MONITORINFO geometry_info[8];
static int geometry_monitor_count;
static BOOL CALLBACK geometry_monitor(HMONITOR monitor,HDC dc,LPRECT rect,LPARAM data){
    int i=geometry_monitor_count;if(i>=8)return FALSE;
    geometry_monitors[i]=monitor;geometry_info[i].cbSize=sizeof(MONITORINFO);
    if(!GetMonitorInfoW(monitor,&geometry_info[i]))return FALSE;geometry_monitor_count++;return TRUE;
}
static int geometry_apply(HWND hwnd,const RECT *rect){
    return SetWindowPos(hwnd,0,rect->left,rect->top,rect->right-rect->left,rect->bottom-rect->top,SWP_NOZORDER|SWP_NOACTIVATE);
}
static int geometry_reset(HWND hwnd,int screen,RECT *rect){
    RECT *work=&geometry_info[screen].rcWork;
    if(!SetWindowPos(hwnd,0,(work->left+work->right)/2,
        (work->top+work->bottom)/2,128,128,SWP_NOZORDER|SWP_NOACTIVATE))return 0;
    main_default_monitor_rect(hwnd,geometry_monitors[screen],rect);return geometry_apply(hwnd,rect);
}
static RECT geometry_raw(RECT origin,UINT edge,int dx,int dy){
    if(edge==WMSZ_LEFT||edge==WMSZ_TOPLEFT||edge==WMSZ_BOTTOMLEFT)origin.left-=dx;
    if(edge==WMSZ_RIGHT||edge==WMSZ_TOPRIGHT||edge==WMSZ_BOTTOMRIGHT)origin.right+=dx;
    if(edge==WMSZ_TOP||edge==WMSZ_TOPLEFT||edge==WMSZ_TOPRIGHT)origin.top-=dy;
    if(edge==WMSZ_BOTTOM||edge==WMSZ_BOTTOMLEFT||edge==WMSZ_BOTTOMRIGHT)origin.bottom+=dy;return origin;
}
static int geometry_fixed(const RECT *a,const RECT *b,UINT edge){
    if((edge==WMSZ_TOPLEFT||edge==WMSZ_BOTTOMLEFT||edge==WMSZ_LEFT)&&a->right!=b->right)return 0;
    if((edge==WMSZ_TOPRIGHT||edge==WMSZ_BOTTOMRIGHT||edge==WMSZ_RIGHT)&&a->left!=b->left)return 0;
    if((edge==WMSZ_TOPLEFT||edge==WMSZ_TOPRIGHT||edge==WMSZ_TOP)&&a->bottom!=b->bottom)return 0;
    if((edge==WMSZ_BOTTOMLEFT||edge==WMSZ_BOTTOMRIGHT||edge==WMSZ_BOTTOM)&&a->top!=b->top)return 0;return 1;
}
static void geometry_check(HWND hwnd){
    int screen,edge,round,step,stationary=1,smooth=1,anchor=1,ratio=1,fixed=1,minimum=1;
    int stationary_count=0,resize_count=0,trip_count=0,max_drift=0;
    UINT dpis[8]={0};RECT origin,raw,adjusted,first,previous,outer,client;FILE *report;
    EnumDisplayMonitors(0,0,geometry_monitor,0);
    for(screen=0;screen<geometry_monitor_count;screen++){
        if(!geometry_reset(hwnd,screen,&origin)){geometry_result=2;return;}
        dpis[screen]=main_window_dpi(hwnd);
        for(edge=WMSZ_LEFT;edge<=WMSZ_BOTTOMRIGHT;edge++){
            int ex,ey,minw,minh;MONITORINFO monitor;
            geometry_apply(hwnd,&origin);geometry_cursor.x=origin.left+20;geometry_cursor.y=origin.top+20;
            SendMessageW(hwnd,WM_ENTERSIZEMOVE,0,0);main_window_limits(hwnd,&ex,&ey,&minw,&minh,&monitor);
            raw=geometry_raw(origin,edge,MulDiv(30,dpis[screen],96),MulDiv(60,dpis[screen],96));
            for(round=0;round<6;round++){
                adjusted=raw;SendMessageW(hwnd,WM_SIZING,edge,(LPARAM)&adjusted);geometry_apply(hwnd,&adjusted);
                if(round==0)first=adjusted;else if(memcmp(&first,&adjusted,sizeof(RECT)))stationary=0;
                fixed&=geometry_fixed(&origin,&adjusted,edge);stationary_count++;
            }
            previous=origin;
            for(step=-160;step<=160;step+=8){
                adjusted=geometry_raw(origin,edge,MulDiv(step,dpis[screen],96),MulDiv(step*2,dpis[screen],96));
                SendMessageW(hwnd,WM_SIZING,edge,(LPARAM)&adjusted);geometry_apply(hwnd,&adjusted);
                GetWindowRect(hwnd,&outer);GetClientRect(hwnd,&client);
                if(abs(client.right*UI_HEIGHT-client.bottom*UI_WIDTH)>UI_HEIGHT)ratio=0;
                if(client.right<minw-1||client.bottom<minh-1)minimum=0;
                fixed&=geometry_fixed(&origin,&adjusted,edge);
                if(step>-160&&(adjusted.right-adjusted.left<previous.right-previous.left||adjusted.bottom-adjusted.top<previous.bottom-previous.top))smooth=0;
                previous=adjusted;resize_count++;
            }
            SendMessageW(hwnd,WM_EXITSIZEMOVE,0,0);
        }
    }
    if(geometry_monitor_count>1&&geometry_reset(hwnd,0,&origin)){
        POINT grab;int initial_width=origin.right-origin.left,initial_height=origin.bottom-origin.top;
        grab.x=origin.left+initial_width/3;grab.y=origin.top+MulDiv(30,dpis[0],96);geometry_cursor=grab;
        SendMessageW(hwnd,WM_ENTERSIZEMOVE,0,0);
        for(round=0;round<6;round++)for(screen=geometry_monitor_count-1;screen>=0;screen--){
            RECT *work=&geometry_info[screen].rcWork;int w,h,dx,dy;
            geometry_cursor.x=(work->left+work->right)/2;geometry_cursor.y=work->top+MulDiv(80,dpis[screen],96);
            GetWindowRect(hwnd,&outer);w=outer.right-outer.left;h=outer.bottom-outer.top;
            raw.left=geometry_cursor.x-(grab.x-origin.left);raw.top=geometry_cursor.y-(grab.y-origin.top);raw.right=raw.left+w;raw.bottom=raw.top+h;
            SendMessageW(hwnd,WM_MOVING,0,(LPARAM)&raw);geometry_apply(hwnd,&raw);GetWindowRect(hwnd,&outer);
            dx=abs(geometry_cursor.x-outer.left-MulDiv(grab.x-origin.left,outer.right-outer.left,initial_width));
            dy=abs(geometry_cursor.y-outer.top-MulDiv(grab.y-origin.top,outer.bottom-outer.top,initial_height));
            if(dx>max_drift)max_drift=dx;if(dy>max_drift)max_drift=dy;
            if(dx>1||dy>1||main_window_dpi(hwnd)!=dpis[screen])anchor=0;trip_count++;
        }
        SendMessageW(hwnd,WM_EXITSIZEMOVE,0,0);
    }
    geometry_result=stationary&&smooth&&anchor&&ratio&&fixed&&minimum&&geometry_monitor_count>0?0:1;
    report=fopen("window-geometry-regression.json","wb");if(!report){geometry_result=3;return;}
    fprintf(report,"{\"Passed\":%s,\"StationaryCornerStable\":%s,\"MonotonicResize\":%s,\"OppositeCornerFixed\":%s,\"AspectRatioPreserved\":%s,\"MinimumSizePreserved\":%s,\"CrossMonitorAnchorPreserved\":%s,\"MaxAnchorDriftPixels\":%d,\"StationaryChecks\":%d,\"ResizeChecks\":%d,\"CrossMonitorMoves\":%d,\"MonitorDpi\":[",geometry_result==0?"true":"false",stationary?"true":"false",smooth?"true":"false",fixed?"true":"false",ratio?"true":"false",minimum?"true":"false",anchor?"true":"false",max_drift,stationary_count,resize_count,trip_count);
    for(screen=0;screen<geometry_monitor_count;screen++)fprintf(report,"%s%u",screen?",":"",dpis[screen]);
    fprintf(report,"],\"PrivateDesktop\":true,\"SimulatedCursor\":true,\"RealInputCalls\":0}");fclose(report);
}
static LRESULT CALLBACK geometry_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    LRESULT result;
    if(msg==WM_APP+40){geometry_check(hwnd);DestroyWindow(hwnd);return 0;}
    result=geometry_settings_proc(hwnd,msg,wp,lp);
    if(msg==WM_CREATE&&geometry_worker)PostMessageW(hwnd,WM_APP+40,0,0);return result;
}
int WINAPI WinMain(HINSTANCE instance,HINSTANCE previous,LPSTR command,int show){
    if(strstr(command,"--worker")){
        int result;geometry_worker=1;result=geometry_settings_main(instance,previous,"--hidden-check --no-host-launch --software-animation",SW_HIDE);
        if(result){FILE *f=fopen("window-geometry-startup.json","wb");if(f){fprintf(f,"{\"Result\":%d,\"LastError\":%lu,\"GraphicsError\":%d,\"WindowCreated\":%s,\"Monitors\":%d}",result,GetLastError(),graphics_error,window?"true":"false",geometry_monitor_count);fclose(f);}}return geometry_result;
    }else{
        WCHAR executable[MAX_PATH],args[1024],name[80],desktop_name[96];HDESK desktop;STARTUPINFOW startup;PROCESS_INFORMATION pi;DWORD code=2;
        _snwprintf(name,80,L"ThreeFingerGeometry-%lu",GetCurrentProcessId());_snwprintf(desktop_name,96,L"winsta0\\%s",name);
        desktop=CreateDesktopW(name,0,0,0,GENERIC_ALL,0);if(!desktop)return 4;
        GetModuleFileNameW(0,executable,MAX_PATH);_snwprintf(args,1024,L"\"%s\" --worker",executable);
        memset(&startup,0,sizeof(startup));startup.cb=sizeof(startup);startup.lpDesktop=desktop_name;startup.dwFlags=STARTF_USESHOWWINDOW;startup.wShowWindow=SW_HIDE;
        if(CreateProcessW(executable,args,0,0,FALSE,0,0,0,&startup,&pi)){
            CloseHandle(pi.hThread);if(WaitForSingleObject(pi.hProcess,30000)==WAIT_OBJECT_0)GetExitCodeProcess(pi.hProcess,&code);else{TerminateProcess(pi.hProcess,5);WaitForSingleObject(pi.hProcess,3000);}CloseHandle(pi.hProcess);
        }CloseDesktop(desktop);return code;
    }
}
