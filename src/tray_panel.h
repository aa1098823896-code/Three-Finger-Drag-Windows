#ifndef THREE_FINGER_TRAY_PANEL_H
#define THREE_FINGER_TRAY_PANEL_H
extern double __cdecl sqrt(double);

/* One short-lived native popup. Its pixels, corners and shadow share one surface. */
#define PANEL_WIDTH 132
#define PANEL_HEIGHT 144
#define PANEL_MARGIN 12
#ifndef WM_DPICHANGED
#define WM_DPICHANGED 0x02E0
#endif
static HWND panel_window,panel_buttons[4];
static WNDPROC panel_button_base;
static HDC panel_dc;
static HBITMAP panel_bitmap,panel_original_bitmap;
static DWORD *panel_pixels;
static WORD *panel_coverage;
static int panel_pixel_width,panel_pixel_height,panel_hover=-1,panel_closing,panel_ready;
static UINT panel_dpi=96;
static int panel_prepare_surface(void);
static void panel_free_surface(void);
static BOOL (WINAPI *panel_update)(HWND,HDC,const POINT *,const SIZE *,HDC,const POINT *,COLORREF,const BLENDFUNCTION *,DWORD);
static const UINT panel_commands[4]={APP_TRAY_GESTURE,APP_TRAY_STARTUP,APP_TRAY_OPEN,APP_TRAY_EXIT};
static const WCHAR *panel_labels[4]={L"三指拖拽",L"开机启动",L"打开设置",L"退出"};
static const int panel_rows[4]={5,37,77,107};

static float panel_distance(float x,float y){
    float qx=x-PANEL_WIDTH/2.0f,qy=y-PANEL_HEIGHT/2.0f;
    if(qx<0)qx=-qx;if(qy<0)qy=-qy;qx-=PANEL_WIDTH/2.0f-12;qy-=PANEL_HEIGHT/2.0f-12;
    float ox=qx>0?qx:0,oy=qy>0?qy:0,m=qx>qy?qx:qy;
    return (float)sqrt(ox*ox+oy*oy)+(m<0?m:0)-12;
}
static int panel_row_at(int px,int py){
    float scale=panel_dpi/96.0f,x=px/scale-PANEL_MARGIN,y=py/scale-PANEL_MARGIN;int i;
    if(panel_distance(x,y)>0||x<5||x>PANEL_WIDTH-5)return -1;
    for(i=0;i<4;i++)if(y>=panel_rows[i]&&y<panel_rows[i]+(i<2?32:30))return i;
    return -1;
}
static int panel_inside(int px,int py){float scale=panel_dpi/96.0f;return panel_distance(px/scale-PANEL_MARGIN,py/scale-PANEL_MARGIN)<=0;}
static void panel_icon(int item,int y,COLORREF color){
    if(item==0)app_icon(9,y+9,15);
    else if(item==1){
        void *path=0,*pen=0;graphics_check(GdipCreatePath(0,&path));
        if(path){graphics_check(GdipAddPathArc(path,10,y+10,12,12,-45,270));graphics_check(GdipCreatePen1(graphics_color(color),1.3f,2,&pen));if(pen){graphics_check(GdipSetPenStartCap(pen,2));graphics_check(GdipSetPenEndCap(pen,2));graphics_check(GdipDrawPath(graphics,pen,path));GdipDeletePen(pen);}GdipDeletePath(path);}line(16,y+8,16,y+15,color,1);
    }else if(item==2){line(9,y+10,23,y+10,color,1);line(9,y+15,23,y+15,color,1);line(9,y+20,23,y+20,color,1);circle(13,y+10,2,color);circle(19,y+15,2,color);circle(12,y+20,2,color);}
    else{line(10,y+9,15,y+9,color,1);line(10,y+9,10,y+21,color,1);line(10,y+21,15,y+21,color,1);line(15,y+15,23,y+15,color,1);line(19,y+11,23,y+15,color,1);line(19,y+19,23,y+15,color,1);}
}
static void panel_switch(int y,int value){
    COLORREF fill=value?RGB(52,199,89):RGB(216,218,223);
    box(90,y+8,28,16,8,fill,fill);circle(value?110:98,y+16,6,RGB(255,255,255));
}
static int panel_render(void){
    RECT rect={0,0,panel_pixel_width,panel_pixel_height};int i;DWORD count;POINT origin={0,0};SIZE size={panel_pixel_width,panel_pixel_height};BLENDFUNCTION blend={AC_SRC_OVER,0,255,AC_SRC_ALPHA};
    if(!panel_ready)return 0;
    FillRect(panel_dc,&rect,background_brush);
    if(graphics_begin(panel_dc,panel_dpi/96.0f,MulDiv(PANEL_MARGIN,panel_dpi,96),MulDiv(PANEL_MARGIN,panel_dpi,96))){
        box(0,0,PANEL_WIDTH,PANEL_HEIGHT,12,background_color,RGB(225,226,231));
        box(5,5,PANEL_WIDTH-10,64,9,RGB(255,255,255),RGB(231,231,235));
        box(5,77,PANEL_WIDTH-10,60,9,RGB(255,255,255),RGB(231,231,235));
        line(12,37,PANEL_WIDTH-12,37,RGB(235,235,239),1);line(12,107,PANEL_WIDTH-12,107,RGB(235,235,239),1);
        for(i=0;i<4;i++){
            COLORREF color=ink;
            if(panel_hover==i){COLORREF tint=i==3?RGB(255,239,238):RGB(235,242,253);box(7,panel_rows[i]+2,PANEL_WIDTH-14,(i<2?32:30)-4,7,tint,tint);if(i==3)color=RGB(192,53,43);}
            panel_icon(i,panel_rows[i],color);text_sequence=i;text(panel_dc,0,32,panel_rows[i],i<2?54:85,i<2?32:30,panel_labels[i],color,DT_LEFT);
            if(i<2)panel_switch(panel_rows[i],i==0?app_enabled():app_startup());
        }
    }
    graphics_end();if(graphics_error)return 0;GdiFlush();
    count=(DWORD)panel_pixel_width*panel_pixel_height;
    for(i=0;i<count;i++){
        unsigned a=panel_coverage[i]&255,shadow=panel_coverage[i]>>8,alpha=a+shadow*(255-a)/255,pixel=panel_pixels[i];
        panel_pixels[i]=(alpha<<24)|((((pixel>>16)&255)*a/255)<<16)|((((pixel>>8)&255)*a/255)<<8)|((pixel&255)*a/255);
    }
    return panel_update(panel_window,0,0,&size,panel_dc,&origin,0,&blend,ULW_ALPHA)!=0;
}
static void panel_set_hover(int row){if(panel_hover!=row){panel_hover=row;panel_render();}}
static LRESULT CALLBACK panel_button_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    if(msg==WM_MOUSEMOVE){int i;TRACKMOUSEEVENT track;for(i=0;i<4;i++)if(panel_buttons[i]==hwnd){panel_set_hover(i);break;}memset(&track,0,sizeof(track));track.cbSize=sizeof(track);track.dwFlags=TME_LEAVE;track.hwndTrack=hwnd;TrackMouseEvent(&track);}
    if(msg==WM_MOUSELEAVE)panel_set_hover(-1);
    return CallWindowProcW(panel_button_base,hwnd,msg,wp,lp);
}
static void panel_close(void){if(!panel_closing){panel_closing=1;DestroyWindow(panel_window);}}
static void panel_command(UINT command){
    HWND settings,core;DWORD process=0;int item,success;
    if(command==APP_TRAY_GESTURE||command==APP_TRAY_STARTUP){
        item=command==APP_TRAY_STARTUP;success=save_toggle(item,item?!app_startup():!app_enabled());
        settings=FindWindowW(APP_UI_CLASS,0);if(settings)PostMessageW(settings,WM_TIMER,1,0);
        if(!success)MessageBoxW(panel_window,L"设置没有保存成功，请重新打开程序再试。",L"三指拖拽",MB_OK|MB_ICONERROR);
    }else if(command==APP_TRAY_OPEN||command==APP_TRAY_EXIT){
        core=FindWindowW(APP_HOST_CLASS,0);if(core){GetWindowThreadProcessId(core,&process);AllowSetForegroundWindow(process);PostMessageW(core,WM_COMMAND,command,0);}
    }else return;
    panel_close();
}
static int panel_snapshot(void){
    WCHAR path[MAX_PATH];HANDLE file;DWORD written,header[5]={0x50474654,0,0,0,0},bytes;int ok;
    if(!panel_ready)return 0;header[1]=GetCurrentProcessId();header[2]=panel_dpi;header[3]=panel_pixel_width;header[4]=panel_pixel_height;
    _snwprintf(path,MAX_PATH,L"%s\\tray-panel-surface.bin",data_folder);path[MAX_PATH-1]=0;
    file=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);if(file==INVALID_HANDLE_VALUE)return 0;
    GdiFlush();bytes=panel_pixel_width*panel_pixel_height*4;
    ok=WriteFile(file,header,sizeof(header),&written,0)&&written==sizeof(header)&&WriteFile(file,panel_pixels,bytes,&written,0)&&written==bytes;CloseHandle(file);return ok;
}
static LRESULT CALLBACK panel_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    if(msg==WM_APP+7)return panel_snapshot(); /* Diagnostic readback of the exact UpdateLayeredWindow surface. */
    if(msg==WM_DPICHANGED&&panel_ready){
        RECT *rect=(RECT*)lp;int i;panel_ready=0;panel_free_surface();panel_dpi=LOWORD(wp);
        if(!panel_prepare_surface()){panel_close();return 0;}
        SetWindowPos(hwnd,0,rect->left,rect->top,panel_pixel_width,panel_pixel_height,SWP_NOZORDER|SWP_NOACTIVATE);
        for(i=0;i<4;i++)SetWindowPos(panel_buttons[i],0,MulDiv(PANEL_MARGIN+5,panel_dpi,96),MulDiv(PANEL_MARGIN+panel_rows[i],panel_dpi,96),MulDiv(PANEL_WIDTH-10,panel_dpi,96),MulDiv(i<2?32:30,panel_dpi,96),SWP_NOZORDER|SWP_NOACTIVATE);
        panel_ready=1;SetPropW(hwnd,L"ThreeFingerDrag-PanelDpi",(HANDLE)(ULONG_PTR)panel_dpi);panel_render();return 0;
    }
    if(msg==WM_DRAWITEM)return TRUE; /* Named native buttons expose actions; the layered surface paints them. */
    if(msg==WM_COMMAND){panel_command(LOWORD(wp));return 0;}
    if(msg==WM_MOUSEMOVE){panel_set_hover(panel_row_at((short)LOWORD(lp),(short)HIWORD(lp)));return 0;}
    if(msg==WM_LBUTTONDOWN||msg==WM_RBUTTONDOWN){if(!panel_inside((short)LOWORD(lp),(short)HIWORD(lp)))panel_close();return 0;}
    if(msg==WM_LBUTTONUP){int row=panel_row_at((short)LOWORD(lp),(short)HIWORD(lp));if(row>=0)panel_command(panel_commands[row]);else if(!panel_inside((short)LOWORD(lp),(short)HIWORD(lp)))panel_close();return 0;}
    if(msg==WM_KEYDOWN){
        if(wp==VK_ESCAPE){panel_close();return 0;}
        if(wp==VK_DOWN||wp==VK_TAB){panel_set_hover((panel_hover+1)%4);return 0;}
        if(wp==VK_UP){panel_set_hover(panel_hover<0?3:(panel_hover+3)%4);return 0;}
        if(wp==VK_HOME||wp==VK_END){panel_set_hover(wp==VK_HOME?0:3);return 0;}
        if(wp==VK_RETURN||wp==VK_SPACE){panel_command(panel_commands[panel_hover<0?2:panel_hover]);return 0;}
    }
    if(msg==WM_ACTIVATE&&LOWORD(wp)==WA_INACTIVE&&panel_ready){panel_close();return 0;}
    if(msg==WM_CANCELMODE||msg==WM_CLOSE){panel_close();return 0;}
    if(msg==WM_PAINT){PAINTSTRUCT paint;BeginPaint(hwnd,&paint);EndPaint(hwnd,&paint);panel_render();return 0;}
    if(msg==WM_ERASEBKGND)return 1;
    if(msg==WM_DESTROY){if(GetCapture()==hwnd)ReleaseCapture();PostQuitMessage(0);return 0;}
    return DefWindowProcW(hwnd,msg,wp,lp);
}
static void panel_free_surface(void){
    if(panel_dc&&panel_original_bitmap)SelectObject(panel_dc,panel_original_bitmap);
    if(panel_bitmap)DeleteObject(panel_bitmap);if(panel_dc)DeleteDC(panel_dc);
    if(panel_coverage)HeapFree(GetProcessHeap(),0,panel_coverage);
    panel_pixels=0;panel_coverage=0;panel_dc=0;panel_bitmap=0;panel_original_bitmap=0;
}
static int panel_prepare_surface(void){
    BITMAPINFO info;int x,y;float scale=panel_dpi/96.0f;
    panel_pixel_width=MulDiv(PANEL_WIDTH+2*PANEL_MARGIN,panel_dpi,96);panel_pixel_height=MulDiv(PANEL_HEIGHT+2*PANEL_MARGIN,panel_dpi,96);
    memset(&info,0,sizeof(info));info.bmiHeader.biSize=sizeof(info.bmiHeader);info.bmiHeader.biWidth=panel_pixel_width;info.bmiHeader.biHeight=-panel_pixel_height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
    panel_dc=CreateCompatibleDC(0);if(!panel_dc)return 0;
    panel_bitmap=CreateDIBSection(panel_dc,&info,DIB_RGB_COLORS,(void**)&panel_pixels,0,0);if(!panel_bitmap)return 0;
    panel_original_bitmap=SelectObject(panel_dc,panel_bitmap);
    panel_coverage=HeapAlloc(GetProcessHeap(),0,(SIZE_T)panel_pixel_width*panel_pixel_height*sizeof(WORD));if(!panel_coverage)return 0;
    for(y=0;y<panel_pixel_height;y++)for(x=0;x<panel_pixel_width;x++){
        float px=(x+0.5f)/scale-PANEL_MARGIN,py=(y+0.5f)/scale-PANEL_MARGIN;
        float coverage=0.5f-panel_distance(px,py)*scale,distance=panel_distance(px,py-3),fade;int a,shadow;
        if(coverage<0)coverage=0;if(coverage>1)coverage=1;a=(int)(coverage*255+0.5f);
        if(distance<0)distance=0;fade=1-distance/10;if(fade<0)fade=0;shadow=(int)(32*fade*fade);
        panel_coverage[y*panel_pixel_width+x]=(WORD)(a|(shadow<<8));
    }
    return 1;
}
static int run_tray_menu(HINSTANCE instance){
    WNDCLASSW wc;HANDLE mutex;POINT cursor;MONITORINFO monitor;UINT (WINAPI *window_dpi)(HWND);MSG message;int i,x,y,result=1;WCHAR accessible[48];
    mutex=CreateMutexW(0,TRUE,L"Local\\ThreeFingerDrag-Native-TrayMenu");if(!mutex)return 1;if(GetLastError()==ERROR_ALREADY_EXISTS){CloseHandle(mutex);return 0;}
    memset(&wc,0,sizeof(wc));wc.lpfnWndProc=panel_proc;wc.hInstance=instance;wc.lpszClassName=APP_TRAY_MENU_CLASS;wc.hCursor=LoadCursorW(0,IDC_ARROW);
    if(!RegisterClassW(&wc))goto cleanup;
    GetCursorPos(&cursor);
    panel_window=CreateWindowExW(WS_EX_LAYERED|WS_EX_TOOLWINDOW|WS_EX_TOPMOST,wc.lpszClassName,L"三指拖拽快捷菜单",WS_POPUP,cursor.x,cursor.y,1,1,0,0,instance,0);if(!panel_window)goto cleanup;
    window_dpi=(void*)GetProcAddress(GetModuleHandleW(L"user32.dll"),"GetDpiForWindow");if(window_dpi)panel_dpi=window_dpi(panel_window);if(!panel_dpi)panel_dpi=96;
    panel_update=(void*)GetProcAddress(GetModuleHandleW(L"user32.dll"),"UpdateLayeredWindow");if(!panel_update)goto cleanup;
    if(!graphics_init()||!init_text())goto cleanup;
    background_brush=CreateSolidBrush(background_color);if(!background_brush)goto cleanup;
    memset(&monitor,0,sizeof(monitor));monitor.cbSize=sizeof(monitor);if(!GetMonitorInfoW(MonitorFromPoint(cursor,MONITOR_DEFAULTTONEAREST),&monitor))goto cleanup;
    for(i=0;i<3;i++){
        UINT actual;panel_pixel_width=MulDiv(PANEL_WIDTH+2*PANEL_MARGIN,panel_dpi,96);panel_pixel_height=MulDiv(PANEL_HEIGHT+2*PANEL_MARGIN,panel_dpi,96);
        x=cursor.x-MulDiv(PANEL_MARGIN,panel_dpi,96);y=cursor.y-MulDiv(PANEL_MARGIN,panel_dpi,96);
        if(x+panel_pixel_width>monitor.rcWork.right)x=monitor.rcWork.right-panel_pixel_width;
        if(y+panel_pixel_height>monitor.rcWork.bottom)y=cursor.y-panel_pixel_height+MulDiv(PANEL_MARGIN,panel_dpi,96);
        if(x<monitor.rcWork.left)x=monitor.rcWork.left;if(y<monitor.rcWork.top)y=monitor.rcWork.top;
        if(!SetWindowPos(panel_window,HWND_TOPMOST,x,y,panel_pixel_width,panel_pixel_height,SWP_NOACTIVATE))goto cleanup;
        actual=window_dpi?window_dpi(panel_window):panel_dpi;if(!actual||actual==panel_dpi)break;panel_dpi=actual;
    }
    if(i==3||!panel_prepare_surface())goto cleanup;
    for(i=0;i<4;i++){
        if(i<2)_snwprintf(accessible,48,L"%s，%s",panel_labels[i],(i==0?app_enabled():app_startup())?L"已开启":L"已关闭");else wcscpy(accessible,panel_labels[i]);
        panel_buttons[i]=CreateWindowExW(WS_EX_TRANSPARENT,L"BUTTON",accessible,WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,MulDiv(PANEL_MARGIN+5,panel_dpi,96),MulDiv(PANEL_MARGIN+panel_rows[i],panel_dpi,96),MulDiv(PANEL_WIDTH-10,panel_dpi,96),MulDiv(i<2?32:30,panel_dpi,96),panel_window,(HMENU)(ULONG_PTR)panel_commands[i],instance,0);
        if(!panel_buttons[i])goto cleanup;
        {WNDPROC base=(WNDPROC)SetWindowLongPtrW(panel_buttons[i],GWLP_WNDPROC,(LONG_PTR)panel_button_proc);if(!base)goto cleanup;if(!panel_button_base)panel_button_base=base;}
    }
    panel_ready=1;if(!panel_render())goto cleanup;
    SetPropW(panel_window,L"ThreeFingerDrag-MenuStyle",(HANDLE)2);
    SetPropW(panel_window,L"ThreeFingerDrag-PanelDpi",(HANDLE)(ULONG_PTR)panel_dpi);
    ShowWindow(panel_window,SW_SHOW);SetForegroundWindow(panel_window);SetFocus(panel_window);
    while((result=GetMessageW(&message,0,0,0))>0){TranslateMessage(&message);DispatchMessageW(&message);}
    result=result<0?1:0;
cleanup:
    panel_closing=1;if(panel_window&&IsWindow(panel_window))DestroyWindow(panel_window);panel_ready=0;panel_free_surface();free_text();graphics_cleanup();if(background_brush){DeleteObject(background_brush);background_brush=0;}CloseHandle(mutex);return result;
}
#endif
