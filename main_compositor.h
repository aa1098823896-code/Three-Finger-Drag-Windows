#ifndef THREE_FINGER_MAIN_COMPOSITOR_H
#define THREE_FINGER_MAIN_COMPOSITOR_H
#include "compositor_api.h"
/* Reuse the approved GDI+ vectors once in five tiny layered bitmaps: blue/gray
   tracks in normal/pressed states and one shared white knob. Windows
   composes the two switch tracks and white knobs; no per-frame application work,
   extra thread, Direct3D device, framework or full-window texture is created. */
static HMODULE main_compositor_library;
#define MAIN_DC_ASSETS 5
static void *main_dc_device,*main_dc_target,*main_dc_root,*main_dc_clip,*main_dc_bases[2],*main_dc_tracks[2],*main_dc_knobs[2],*main_dc_opacity[2],*main_dc_surfaces[MAIN_DC_ASSETS];
static HWND main_dc_bitmaps[MAIN_DC_ASSETS];
static int main_dc_unit,main_dc_ox,main_dc_oy,main_dc_down[2]={-1,-1};
static UINT main_dc_commits,main_dc_assets;
static HRESULT main_dc_error;
static int main_dc_source_x,main_dc_source_y;
#ifdef MAIN_COMPOSITOR_DIAGNOSTICS
static struct {BYTE *bits;int width,height;} main_dc_diagnostics[MAIN_DC_ASSETS];
#endif
static void main_compositor_properties(void){
    if(!window)return;SetPropW(window,L"ThreeFingerDrag-Compositor",(HANDLE)(ULONG_PTR)main_compositor_ready);
    SetPropW(window,L"ThreeFingerDrag-CompositorCommits",(HANDLE)(ULONG_PTR)main_dc_commits);SetPropW(window,L"ThreeFingerDrag-CompositorAssets",(HANDLE)(ULONG_PTR)main_dc_assets);
    SetPropW(window,L"ThreeFingerDrag-CompositorError",(HANDLE)(ULONG_PTR)(DWORD)main_dc_error);
}
static int main_compositor_ok(HRESULT hr){if(FAILED(hr)){main_dc_error=hr;return 0;}return 1;}
static LRESULT CALLBACK main_compositor_bitmap_proc(HWND hwnd,UINT message,WPARAM wp,LPARAM lp){
    if(message==WM_PAINT){PAINTSTRUCT paint;BeginPaint(hwnd,&paint);EndPaint(hwnd,&paint);return 0;}
    if(message==WM_ERASEBKGND)return 1;if(message==WM_NCHITTEST)return HTTRANSPARENT;if(message==WM_MOUSEACTIVATE)return MA_NOACTIVATE;
    return DefWindowProcW(hwnd,message,wp,lp);
}
static int main_compositor_position_sources(HWND hwnd){
    MONITORINFO monitor;int i,x,y;memset(&monitor,0,sizeof(monitor));monitor.cbSize=sizeof(monitor);
    if(!GetMonitorInfoW(MonitorFromWindow(hwnd,MONITOR_DEFAULTTONEAREST),&monitor))return 0;x=monitor.rcWork.left+8;y=monitor.rcWork.top+8;
    if(x!=main_dc_source_x||y!=main_dc_source_y){for(i=0;i<MAIN_DC_ASSETS;i++)if(main_dc_bitmaps[i]&&!SetWindowPos(main_dc_bitmaps[i],0,x,y,0,0,SWP_NOZORDER|SWP_NOACTIVATE|SWP_NOSIZE|SWP_NOOWNERZORDER))return 0;main_dc_source_x=x;main_dc_source_y=y;}return 1;
}
static int main_compositor_commit(void){if(!main_compositor_ok(dc_call(main_dc_device,3)))return 0;main_dc_commits++;main_compositor_properties();return 1;}
static void main_compositor_free(void){
    int i;main_compositor_ready=0;if(main_dc_target)dc_ptr(main_dc_target,3,0);if(main_dc_device)dc_call(main_dc_device,3);
    dc_release(&main_dc_target);dc_release(&main_dc_root);dc_release(&main_dc_clip);for(i=0;i<2;i++){dc_release(&main_dc_bases[i]);dc_release(&main_dc_tracks[i]);dc_release(&main_dc_knobs[i]);dc_release(&main_dc_opacity[i]);}
    for(i=0;i<MAIN_DC_ASSETS;i++){dc_release(&main_dc_surfaces[i]);if(main_dc_bitmaps[i])DestroyWindow(main_dc_bitmaps[i]);main_dc_bitmaps[i]=0;
#ifdef MAIN_COMPOSITOR_DIAGNOSTICS
        if(main_dc_diagnostics[i].bits)HeapFree(GetProcessHeap(),0,main_dc_diagnostics[i].bits);memset(&main_dc_diagnostics[i],0,sizeof(main_dc_diagnostics[i]));
#endif
    }
    dc_release(&main_dc_device);if(main_compositor_library)FreeLibrary(main_compositor_library);main_compositor_library=0;
    main_dc_unit=0;main_dc_source_x=main_dc_source_y=0;main_dc_down[0]=main_dc_down[1]=-1;main_compositor_properties();
}
static int main_compositor_fallback(void){main_compositor_free();main_compositor_disabled=1;frame_dirty=main_surface_dirty=1;main_feedback_schedule();return 0;}
static int main_compositor_health(void){
    BOOL valid=FALSE;
    if(!main_compositor_ok(DC_METHOD(main_dc_device,26,HRESULT(WINAPI*)(void*,BOOL*))(main_dc_device,&valid)))return main_compositor_fallback();
    if(!valid){main_dc_error=(HRESULT)0x887A0005;return main_compositor_fallback();}return 1;
}
static int main_compositor_asset(HWND hwnd,int unit,int knob,int down){
    BITMAPINFO info;void *bits=0,*image=0,*brush=0,*matrix=0;HDC dc=0;HBITMAP bitmap=0,old=0;POINT origin={0,0};SIZE size;BLENDFUNCTION blend={AC_SRC_OVER,0,255,AC_SRC_ALPHA};float scale=unit/10000.0f;int ok=0;
    int(WINAPI *image_graphics)(void*,void**)=(void*)GetProcAddress(graphics_library,"GdipGetImageGraphicsContext");
    BOOL(WINAPI *update)(HWND,HDC,const POINT*,const SIZE*,HDC,const POINT*,COLORREF,const BLENDFUNCTION*,DWORD)=(void*)GetProcAddress(GetModuleHandleW(L"user32.dll"),"UpdateLayeredWindow");
    size.cx=(int)((knob==1?26:50)*scale+0.999f);size.cy=(int)((knob==1?26:28)*scale+0.999f);memset(&info,0,sizeof(info));info.bmiHeader.biSize=40;info.bmiHeader.biWidth=size.cx;info.bmiHeader.biHeight=-size.cy;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;
    dc=CreateCompatibleDC(0);if(!dc)goto done;bitmap=CreateDIBSection(dc,&info,0,&bits,0,0);if(!bitmap)goto done;old=SelectObject(dc,bitmap);memset(bits,0,size.cx*size.cy*4);
    if(!image_graphics||!update||GdipCreateBitmapFromScan0(size.cx,size.cy,size.cx*4,0x000E200B,bits,&image)||image_graphics(image,&graphics))goto done;
    GdipSetSmoothingMode(graphics,4);GdipSetPixelOffsetMode(graphics,4);GdipSetPageUnit(graphics,2);
    if(GdipCreateMatrix2(scale,0,0,scale,0,0,&matrix)||GdipSetWorldTransform(graphics,matrix))goto done;
    if(knob==1){if(GdipCreateSolidFill(0xFFFFFFFF,&brush)||GdipFillEllipse(graphics,brush,2.0f,2.0f,22.0f,22.0f))goto done;}
    else{COLORREF color=knob==2?RGB(216,218,223):blue;if(down)color=main_feedback_mix(color,RGB(0,0,0),60);
        if(GdipCreateSolidFill(graphics_color(color),&brush)||main_fill_rect(graphics,brush,0,0,(float)(size.cx/scale),(float)(size.cy/scale)))goto done;
    }
    graphics_end();GdiFlush();
#ifdef MAIN_COMPOSITOR_DIAGNOSTICS
    {int i;for(i=0;i<MAIN_DC_ASSETS;i++)if(main_dc_bitmaps[i]==hwnd){BYTE *copy=HeapAlloc(GetProcessHeap(),0,size.cx*size.cy*4);if(!copy)goto done;memcpy(copy,bits,size.cx*size.cy*4);if(main_dc_diagnostics[i].bits)HeapFree(GetProcessHeap(),0,main_dc_diagnostics[i].bits);main_dc_diagnostics[i].bits=copy;main_dc_diagnostics[i].width=size.cx;main_dc_diagnostics[i].height=size.cy;}}
#endif
    ok=update(hwnd,0,0,&size,dc,&origin,0,&blend,ULW_ALPHA)!=0;if(ok)main_dc_assets++;
done:
    if(graphics)graphics_end();if(matrix)GdipDeleteMatrix(matrix);if(brush)GdipDeleteBrush(brush);if(image)GdipDisposeImage(image);if(old)SelectObject(dc,old);if(bitmap)DeleteObject(bitmap);if(dc)DeleteDC(dc);return ok;
}
static int main_compositor_create(HWND hwnd){
    HRESULT(WINAPI *create)(void*,const GUID*,void**);int i;BOOL cloak=TRUE;WNDCLASSW cls;
    main_compositor_library=LoadLibraryExW(L"dcomp.dll",0,0x00000800);if(!main_compositor_library)return 0;
    create=(void*)GetProcAddress(main_compositor_library,"DCompositionCreateDevice");if(!create||!main_compositor_ok(create(0,&main_dc_device_iid,&main_dc_device))||!main_compositor_ok(dc_target(main_dc_device,hwnd,&main_dc_target))||!main_compositor_ok(dc_new(main_dc_device,7,&main_dc_root))||!main_compositor_ok(dc_new(main_dc_device,24,&main_dc_clip)))return 0;
    memset(&cls,0,sizeof(cls));cls.hInstance=(HINSTANCE)GetWindowLongPtrW(hwnd,GWLP_HINSTANCE);cls.lpfnWndProc=main_compositor_bitmap_proc;cls.lpszClassName=L"ThreeFingerDrag-CompositionBitmap";
    if(!main_dwm_attribute||(!RegisterClassW(&cls)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS))return 0;
    for(i=0;i<MAIN_DC_ASSETS;i++){
        main_dc_bitmaps[i]=CreateWindowExW(WS_EX_LAYERED|WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE|WS_EX_TRANSPARENT,cls.lpszClassName,L"",WS_POPUP,0,0,1,1,hwnd,0,cls.hInstance,0);
        /* CreateSurfaceFromHwnd stops composing off-screen/zero-size windows.
           Cloaking keeps the source available without showing it or taking input. */
        if(!main_dc_bitmaps[i]||!main_compositor_ok(main_dwm_attribute(main_dc_bitmaps[i],13,&cloak,sizeof(cloak))))return 0;
        ShowWindow(main_dc_bitmaps[i],SW_SHOWNOACTIVATE);
    }
    if(!main_compositor_position_sources(hwnd))return 0;
    for(i=0;i<2;i++)if(!main_compositor_ok(dc_new(main_dc_device,7,&main_dc_bases[i]))||!main_compositor_ok(dc_new(main_dc_device,7,&main_dc_tracks[i]))||!main_compositor_ok(dc_new(main_dc_device,7,&main_dc_knobs[i]))||!main_compositor_ok(dc_new(main_dc_device,23,&main_dc_opacity[i]))||!main_compositor_ok(dc_ptr(main_dc_tracks[i],10,main_dc_opacity[i]))||!main_compositor_ok(dc_ptr(main_dc_bases[i],13,main_dc_clip))||!main_compositor_ok(dc_add(main_dc_bases[i],main_dc_tracks[i]))||!main_compositor_ok(dc_add(main_dc_bases[i],main_dc_knobs[i]))||!main_compositor_ok(dc_add(main_dc_root,main_dc_bases[i])))return 0;
    /* Rounded clips otherwise inherit the system's aliased default. */
    return main_compositor_ok(dc_enum(main_dc_root,12,0))&&main_compositor_ok(dc_enum(main_dc_root,11,1))&&main_compositor_ok(dc_ptr(main_dc_target,3,main_dc_root));
}
static int main_compositor_curve(void *object,int animated,int scalar,float from,float target,LONGLONG started,int active){
    void *animation=0;int ok=0;if(!active)return main_compositor_ok(dc_float(object,scalar,target));
    if(!main_compositor_ok(dc_new(main_dc_device,25,&animation)))return 0;
    if(main_compositor_ok(dc_absolute(animation,started))&&main_compositor_ok(dc_cubic(animation,from,target-from,MAIN_FEEDBACK_DURATION/1000.0))&&main_compositor_ok(dc_end(animation,MAIN_FEEDBACK_DURATION/1000.0,target))&&main_compositor_ok(dc_ptr(object,animated,animation)))ok=1;
    dc_release(&animation);return ok;
}
static int main_compositor_switches(void){
    int i;float scale=main_dc_unit/10000.0f,x;
    if(!main_compositor_ready)return 0;
    for(i=0;i<2;i++){
        float from=main_slides[i].from/1000.0f,target=main_slides[i].target/1000.0f;
        x=scale;
        if(!main_compositor_curve(main_dc_knobs[i],3,4,x+22*scale*from,x+22*scale*target,main_slides[i].started,main_slides[i].active)||!main_compositor_curve(main_dc_opacity[i],3,4,from,target,main_slides[i].started,main_slides[i].active))return main_compositor_fallback();
    }
    if(!main_compositor_commit())return main_compositor_fallback();return 1;
}
static int main_compositor_prepare(HWND hwnd,int unit,int ox,int oy){
    int i,resized,changed=0;float scale=unit/10000.0f;
    if(main_compositor_disabled||(!main_feedback_animate&&!main_dc_device))return main_compositor_ready;
    if(!main_dc_device&&!main_compositor_create(hwnd))return main_compositor_fallback();
    if(!main_compositor_health())return 0;
    if(!main_compositor_position_sources(hwnd))return main_compositor_fallback();
    resized=unit!=main_dc_unit;changed=main_feedback_down(1)!=main_dc_down[0]||main_feedback_down(2)!=main_dc_down[1];
    for(i=0;i<MAIN_DC_ASSETS;i++){
        if(resized){if(!main_compositor_asset(main_dc_bitmaps[i],unit,i==2?1:i>2?2:0,i==1||i==4))return main_compositor_fallback();}
        if(!main_dc_surfaces[i]&&!main_compositor_ok(dc_hwnd(main_dc_device,main_dc_bitmaps[i],&main_dc_surfaces[i])))return main_compositor_fallback();
    }
    if(resized||changed)for(i=0;i<2;i++){int down=main_feedback_down(i+1);
        if(!main_compositor_ok(dc_ptr(main_dc_bases[i],15,main_dc_surfaces[down?4:3]))||!main_compositor_ok(dc_ptr(main_dc_tracks[i],15,main_dc_surfaces[down?1:0])))return main_compositor_fallback();
    }
    if(resized||ox!=main_dc_ox||oy!=main_dc_oy||!main_compositor_ready){
        main_dc_unit=unit;main_dc_ox=ox;main_dc_oy=oy;
        if(!main_compositor_ok(dc_float(main_dc_clip,4,0))||!main_compositor_ok(dc_float(main_dc_clip,6,0))||!main_compositor_ok(dc_float(main_dc_clip,8,50*scale))||!main_compositor_ok(dc_float(main_dc_clip,10,28*scale)))return main_compositor_fallback();
        for(i=12;i<=26;i+=2)if(!main_compositor_ok(dc_float(main_dc_clip,i,14*scale)))return main_compositor_fallback();
        for(i=0;i<2;i++){
            float y=oy+(i?190:124)*scale,x=ox+414*scale;
            if(!main_compositor_ok(dc_ptr(main_dc_knobs[i],15,main_dc_surfaces[2]))||!main_compositor_ok(dc_float(main_dc_bases[i],4,x))||!main_compositor_ok(dc_float(main_dc_bases[i],6,y))||!main_compositor_ok(dc_float(main_dc_knobs[i],6,scale)))return main_compositor_fallback();
        }
        main_compositor_ready=1;main_feedback_evaluate();if(!main_compositor_switches())return 0;main_feedback_schedule();frame_dirty=main_surface_dirty=1;
    }else if(changed&&!main_compositor_commit())return main_compositor_fallback();
    for(i=0;i<2;i++)main_dc_down[i]=main_feedback_down(i+1);main_compositor_properties();return main_compositor_ready;
}
/* WM_PRINT receives a memory DC, outside the screen compositor. Replay the
   same approved vectors there once without changing the live background DIB. */
static void main_compositor_snapshot(HDC dc,int x,int y){
    if(!main_compositor_ready)return;main_feedback_evaluate();
    if(graphics_begin(dc,main_dc_unit/10000.0f,main_dc_ox+x,main_dc_oy+y)){
        main_compositor_ready=0;main_toggle(414,124,1);main_toggle(414,190,2);main_compositor_ready=1;graphics_end();
    }
}
#endif
