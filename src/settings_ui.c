#define UNICODE
#define _UNICODE
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include "app_shared.h"
#include "ui_graphics.h"
#include "touchpad_prepare.h"

/* Settings persist independently of the small input process. */
#include "main_psd_layout.h"
#define UI_WINDOW_STYLE (WS_POPUP|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX)
static HWND window;static WCHAR folder[MAX_PATH],footer[180],status_label[24];static int on=1,startup=0,probe=0,isolated=0,focus_item=0,test_no_host=0,close_pending=0;static DWORD close_deadline;
static HBRUSH background_brush;static const COLORREF ink=RGB(29,29,31),muted=RGB(112,112,117),blue=RGB(0,122,255),background_color=RGB(247,247,249);
static COLORREF status_color=RGB(31,142,65),status_fill=RGB(233,247,237);
static const int font_sizes[10]={11,12,12,18,10,14,28,13,10,11};
#define TEXT_CACHE_SIZE 40
static struct {void *path,*brush;WCHAR value[180];int font,x,y,w,h;UINT align;COLORREF color;} text_cache[TEXT_CACHE_SIZE];
static void *text_family,*text_formats[3];static int text_sequence;static UINT text_paths_created,canvas_allocations;
/* Reuse font outlines at every scale; only changed text rebuilds its outline. */
static struct {HDC dc;HBITMAP bitmap,original;void *bits;BITMAPINFO info;int width,height,capacity_width,capacity_height,unit;} frame;
static int frame_dirty=1;
static int main_surface_dirty=1,main_surface_ready,main_surface_width,main_surface_height,main_surface_x,main_surface_y;
static int main_surface_present(HWND);
static int main_surface_snapshot(HWND);
static void main_surface_free(void);
static void main_surface_touch(HWND hwnd){main_surface_dirty=1;if(hwnd)InvalidateRect(hwnd,0,FALSE);}
static void redraw(void){frame_dirty=1;InvalidateRect(window,0,FALSE);}
typedef HRESULT (WINAPI *DWM_ATTRIBUTE)(HWND,DWORD,LPCVOID,DWORD);
static DWM_ATTRIBUTE main_dwm_attribute;
static void main_frame_border(HWND hwnd){
    COLORREF none=0xFFFFFFFE;int system_corners=1;
    /* Our alpha surface already owns both corners and shadow; DWM rounding adds a second base. */
    if(main_dwm_attribute){main_dwm_attribute(hwnd,33,&system_corners,sizeof(system_corners));main_dwm_attribute(hwnd,34,&none,sizeof(none));}
}
static int init_text(void);
static void main_art_free(void);
static int setup_validation;
static int setup_failed,setup_copy_result;
static DWORD setup_notice_until;
static WCHAR setup_notice[180];
static void free_text(void);
static void text(HDC,int,int,int,int,int,const WCHAR *,COLORREF,UINT);
static void *app_icon_image;
static BYTE *app_icon_pixels;
static void free_app_icon(void){
    if(app_icon_image)GdipDisposeImage(app_icon_image);app_icon_image=0;
    if(app_icon_pixels)HeapFree(GetProcessHeap(),0,app_icon_pixels);app_icon_pixels=0;
}
static int init_app_icon(void){
    WCHAR path[MAX_PATH];HICON icon=0;ICONINFO source;BITMAP bitmap;BITMAPINFO info;HDC dc=0;int ok=0;
    memset(&source,0,sizeof(source));_snwprintf(path,MAX_PATH,L"%s\\Gesture.ico",app_folder);path[MAX_PATH-1]=0;
    icon=LoadImageW(0,path,IMAGE_ICON,256,256,LR_LOADFROMFILE);if(!icon||!GetIconInfo(icon,&source)||!source.hbmColor)goto cleanup;
    if(!GetObjectW(source.hbmColor,sizeof(bitmap),&bitmap)||bitmap.bmWidth!=256||bitmap.bmHeight!=256||bitmap.bmBitsPixel!=32)goto cleanup;
    app_icon_pixels=HeapAlloc(GetProcessHeap(),0,256*256*4);dc=CreateCompatibleDC(0);if(!app_icon_pixels||!dc)goto cleanup;
    memset(&info,0,sizeof(info));info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=256;info.bmiHeader.biHeight=-256;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
    if(GetDIBits(dc,source.hbmColor,0,256,app_icon_pixels,&info,DIB_RGB_COLORS)!=256)goto cleanup;
    /* Keep the icon's original straight-alpha pixels alive until GDI+ releases the image. */
    ok=GdipCreateBitmapFromScan0(256,256,256*4,0x0026200A,app_icon_pixels,&app_icon_image)==0;
cleanup:
    if(dc)DeleteDC(dc);if(source.hbmColor)DeleteObject(source.hbmColor);if(source.hbmMask)DeleteObject(source.hbmMask);if(icon)DestroyIcon(icon);
    if(!ok)free_app_icon();return ok;
}
static void app_icon(int x,int y,int size){
    if(app_icon_image)graphics_check(GdipDrawImageRectI(graphics,app_icon_image,x,y,size,size));
}
static int save_toggle(int item,int value){
    HWND core;
    if(item)return app_save_startup(value);
    if(!app_save_enabled(value))return 0;
    core=FindWindowW(APP_HOST_CLASS,0);
    if(core)return PostMessageW(core,APP_RELOAD,0,0)!=0;
    return app_launch(L"GestureHost.exe",L"--background");
}
#include "tray_panel.h"
static void canvas(int width,int height,int *unit,int *ox,int *oy){
    int sx=MulDiv(width,10000*UI_LAYOUT_SCALE,UI_WIDTH),sy=MulDiv(height,10000*UI_LAYOUT_SCALE,UI_HEIGHT);
    *unit=sx<sy?sx:sy;if(*unit<1)*unit=1;
    *ox=(width-MulDiv(UI_WIDTH,*unit,10000*UI_LAYOUT_SCALE))/2-MulDiv(UI_ORIGIN_X,*unit,10000);
    *oy=(height-MulDiv(UI_HEIGHT,*unit,10000*UI_LAYOUT_SCALE))/2-MulDiv(UI_ORIGIN_Y,*unit,10000);
}
#include "main_feedback.h"
static int init_text(void){
    int i;
    if(GdipCreateFontFamilyFromName(L"Microsoft YaHei UI",0,&text_family))return 0;
    for(i=0;i<3;i++)if(GdipCreateStringFormat(0x1000,0,&text_formats[i])||GdipSetStringFormatAlign(text_formats[i],i)||GdipSetStringFormatLineAlign(text_formats[i],1))return 0;
    return init_app_icon();
}
static void free_text(void){
    int i;main_art_free();free_app_icon();for(i=0;i<TEXT_CACHE_SIZE;i++){if(text_cache[i].path)GdipDeletePath(text_cache[i].path);if(text_cache[i].brush)GdipDeleteBrush(text_cache[i].brush);}
    memset(text_cache,0,sizeof(text_cache));for(i=0;i<3;i++){if(text_formats[i])GdipDeleteStringFormat(text_formats[i]);text_formats[i]=0;}
    if(text_family)GdipDeleteFontFamily(text_family);text_family=0;
}
static void text(HDC dc,int font,int x,int y,int w,int h,const WCHAR *value,COLORREF color,UINT align){
    int slot=text_sequence++,format=align==DT_CENTER?1:align==DT_RIGHT?2:0;void *path=0;UI_RECTF rect={(float)x,(float)y,(float)w,(float)h};
    if(slot>=TEXT_CACHE_SIZE){graphics_error=2;return;}
    if(!text_cache[slot].path||text_cache[slot].font!=font||text_cache[slot].x!=x||text_cache[slot].y!=y||text_cache[slot].w!=w||text_cache[slot].h!=h||text_cache[slot].align!=align||wcscmp(text_cache[slot].value,value)){
        graphics_check(GdipCreatePath(0,&path));if(!path)return;
        graphics_check(GdipAddPathString(path,value,-1,text_family,font==2||font==3||font==5||font==6||font==7?1:0,(float)font_sizes[font],&rect,text_formats[format]));
        if(graphics_error){GdipDeletePath(path);return;}
        if(text_cache[slot].path)GdipDeletePath(text_cache[slot].path);text_cache[slot].path=path;text_paths_created++;
        text_cache[slot].font=font;text_cache[slot].x=x;text_cache[slot].y=y;text_cache[slot].w=w;text_cache[slot].h=h;text_cache[slot].align=align;
        wcsncpy(text_cache[slot].value,value,179);text_cache[slot].value[179]=0;
    }
    if(!text_cache[slot].brush||text_cache[slot].color!=color){
        if(text_cache[slot].brush)GdipDeleteBrush(text_cache[slot].brush);text_cache[slot].brush=0;
        graphics_check(GdipCreateSolidFill(graphics_color(color),&text_cache[slot].brush));text_cache[slot].color=color;
    }
    if(text_cache[slot].brush)graphics_check(GdipFillPath(graphics,text_cache[slot].brush,text_cache[slot].path));
}
static void toggle(int x,int y,int value){
    box(x,y,46,27,14,value?RGB(52,199,89):RGB(216,218,223),value?RGB(52,199,89):RGB(216,218,223));
    circle(x+(value?33:13),y+14,10,RGB(255,255,255));
}
static int setup_actionable(void){return !tp_state.ready;}
static int setup_steps_visible(void){return setup_actionable()&&tp_state.native_ok&&tp_state.present&&tp_state.enabled;}
static const WCHAR setup_guidance[]=L"只需关闭系统的三指轻扫和点击。";
static int setup_open_settings(void){
    HMODULE library=LoadLibraryExW(L"shell32.dll",0,0x00000800);HINSTANCE (WINAPI *open)(HWND,const WCHAR *,const WCHAR *,const WCHAR *,const WCHAR *,int);
    HINSTANCE result=0;if(!library)return 0;open=(void*)GetProcAddress(library,"ShellExecuteW");if(open)result=open(window,L"open",L"ms-settings:devices-touchpad",0,0,SW_SHOWNORMAL);FreeLibrary(library);return (INT_PTR)result>32;
}
static void refresh(void){
    WCHAR state[32];HWND core;
    status_color=RGB(31,142,65);status_fill=RGB(233,247,237);
    if(isolated){wcscpy(status_label,on?L"运行中":L"已暂停");wcscpy(footer,L"关闭窗口后留在托盘，右键图标可退出。");if(!on){status_color=muted;status_fill=RGB(232,233,236);}return;}
    on=app_enabled();startup=app_startup();core=FindWindowW(APP_HOST_CLASS,0);
    GetPrivateProfileStringW(L"Status",L"State",L"",state,32,config_path);
    if(!on){wcscpy(status_label,L"已暂停");wcscpy(footer,L"三指拖拽已暂停，可从托盘重新开启。");status_color=muted;status_fill=RGB(232,233,236);}
    else if(!core){wcscpy(status_label,L"未运行");wcscpy(footer,L"后台未运行，关闭再开启「三指拖拽」重试。");status_color=RGB(155,92,10);status_fill=RGB(255,245,224);}
    else if(!wcscmp(state,L"error")){wcscpy(status_label,L"需重试");wcscpy(footer,L"触控板暂不可用，关闭再开启「三指拖拽」重试。");status_color=RGB(155,92,10);status_fill=RGB(255,245,224);}
    else if(!wcscmp(state,L"disconnected")){wcscpy(status_label,L"待连接");wcscpy(footer,L"等待触控板连接。");status_color=muted;status_fill=RGB(232,233,236);}
    else if(!tp_state.ready){
        wcscpy(status_label,L"需设置");status_color=RGB(155,92,10);status_fill=RGB(255,245,224);
        if(!tp_state.native_ok)wcscpy(footer,L"需要 Windows 11 24H2 及以上的精确触控板。");
        else if(!tp_state.present)wcscpy(footer,L"未检测到支持三指的精确触控板。");
        else if(!tp_state.enabled)wcscpy(footer,L"触控板已关闭，请在系统设置中开启。");
        else wcscpy(footer,setup_guidance);
    }
    else {wcscpy(status_label,L"运行中");wcscpy(footer,L"关闭窗口后留在托盘，右键图标可退出。");}
    if(setup_notice_until&&(LONG)(setup_notice_until-GetTickCount())>0)wcscpy(footer,setup_notice);else setup_notice_until=0;
    if(tp_state.ready)setup_failed=0;
    if(window){SetPropW(window,L"ThreeFingerDrag-SetupReady",(HANDLE)(ULONG_PTR)(tp_state.ready+1));SetPropW(window,L"ThreeFingerDrag-SetupCapabilities",(HANDLE)(ULONG_PTR)(tp_state.native_ok|(tp_state.present<<1)|(tp_state.enabled<<2)|(tp_state.cache_ok<<3)|(tp_state.registry_ok<<4)));SetPropW(window,L"ThreeFingerDrag-SetupError",(HANDLE)(ULONG_PTR)(tp_prepare_error?tp_prepare_error:tp_state.error));SetPropW(window,L"ThreeFingerDrag-SetupFailed",(HANDLE)(ULONG_PTR)setup_failed);SetPropW(window,L"ThreeFingerDrag-SetupCopyResult",(HANDLE)(ULONG_PTR)setup_copy_result);}
    if(window&&(main_slides[0].target!=(on?1000:0)||main_slides[1].target!=(startup?1000:0)))main_feedback_sync_switches(1);
}
static void setup_action(void){
    if(isolated||(test_no_host&&!setup_validation))return;
    main_feedback_notice(3,setup_open_settings()?1:2);
}
static void setup_copy_ai(void){
    WCHAR prompt[4096],details[128];int count;HGLOBAL memory=0;WCHAR *target;
    if(isolated||(test_no_host&&!setup_validation))return;
    tp_check();
    if(!tp_state.native_ok||!tp_state.cache_ok)wcscpy(details,L"软件暂时无法确认这两项设置，请以系统设置里看到的结果为准。");
    else if(!tp_state.present)wcscpy(details,L"软件没有检测到支持三指操作的触控板，请先确认电脑是否支持。");
    else if(!tp_state.enabled)wcscpy(details,L"软件看到触控板目前已关闭，请先检查这一点。");
    else _snwprintf(details,128,L"软件看到：三指轻扫%s，三指点击%s。",tp_state.swipe?L"还没设为「无」":L"已设为「无」",tp_state.tap?L"还没设为「无」":L"已设为「无」");
    count=_snwprintf(prompt,4096,
        L"请帮我设置好三指拖拽：在系统触控板设置里，把三指轻扫和三指点击都设为「无」。其他设置保持原样，尤其不要关闭双击拖动，也不要改系统指针速度。\r\n\r\n"
        L"先检查当前设置，已经设置好的就不重复修改。修改前记下原来的值；改完后再看一次系统设置，并重新打开软件确认状态。不要安装额外软件、重启电脑或关闭其他程序。\r\n\r\n"
        L"如果你不能操作电脑，请直接告诉我，并用最简单的步骤教我自己设置。\r\n\r\n%s",
        details);
    setup_copy_result=2;
    if(count>0&&count<4096){
        memory=GlobalAlloc(GMEM_MOVEABLE,((SIZE_T)count+1)*sizeof(WCHAR));
        if(memory&&(target=GlobalLock(memory))){memcpy(target,prompt,((SIZE_T)count+1)*sizeof(WCHAR));GlobalUnlock(memory);
            if(OpenClipboard(window)){if(EmptyClipboard()&&SetClipboardData(CF_UNICODETEXT,memory)){memory=0;setup_copy_result=1;}CloseClipboard();}
        }
    }
    if(memory)GlobalFree(memory);
    wcscpy(setup_notice,setup_copy_result==1?L"已复制，粘贴给能操作电脑的 AI。":L"复制失败，请稍后再点一次。");setup_notice_until=GetTickCount()+12000;main_feedback_notice(4,setup_copy_result);tp_report(0);refresh();redraw();
}
static void change_toggle(int item){
    int success;
    if(isolated){if(item==0)on=!on;else startup=!startup;refresh();main_feedback_sync_switches(1);redraw();return;}
    success=save_toggle(item,item?!startup:!on);
    if(!success){MessageBoxW(window,L"设置没有保存成功，请重新打开程序再试。",L"三指拖拽",MB_OK|MB_ICONERROR);return;}
    refresh();main_feedback_sync_switches(1);redraw();
}
static int tray_ready(void){
    HWND core=FindWindowW(APP_HOST_CLASS,0);DWORD_PTR result=0;
    return core&&SendMessageTimeoutW(core,APP_TRAY_READY,0,0,SMTO_ABORTIFHUNG,500,&result)&&result!=0;
}
static void request_close(HWND hwnd){
    if(isolated||test_no_host||tray_ready()){DestroyWindow(hwnd);return;}
    if(!FindWindowW(APP_HOST_CLASS,0)&&!app_launch(L"GestureHost.exe",L"--background")){
        MessageBoxW(hwnd,L"托盘后台未能启动。设置窗口会保留，请稍后重试。",L"三指拖拽",MB_OK|MB_ICONERROR);return;
    }
    close_pending=1;close_deadline=GetTickCount()+3000;SetTimer(hwnd,3,100,0);
}
#include "main_window.h"
#include "main_compositor.h"
static void main_activate_item(int item){
    if(item==1||item==2)change_toggle(item-1);else if(item==3||item==4)PostMessageW(window,item==4?WM_APP+10:WM_APP+9,0,0);
}
static void paint_content(HDC dc,int unit,int ox,int oy){
    /* Shapes and cached glyph outlines share the same continuous vector transform. */
    main_background_prefilled=main_paint_background(dc,unit,ox,oy);
    main_cards_prefilled=main_background_prefilled&&main_paint_card_interiors(dc,unit,ox,oy);
    if(graphics_begin(dc,unit/10000.0f,ox,oy)){
        if(main_feedback_painting)graphics_check(main_set_clip_rect(graphics,(float)main_feedback_paint_clip.left,(float)main_feedback_paint_clip.top,(float)(main_feedback_paint_clip.right-main_feedback_paint_clip.left),(float)(main_feedback_paint_clip.bottom-main_feedback_paint_clip.top),1));
        main_paint(dc,unit,ox,oy);
    }
    graphics_end();
    main_caption_glyph(dc,0,0,unit,ox,oy);main_caption_glyph(dc,1,0,unit,ox,oy);
}
static void fill_background(HDC dc,int x,int y,int width,int height){
    RECT rect={x,y,x+width,y+height};
    if(width>0&&height>0)FillRect(dc,&rect,background_brush);
}
static void paint(HDC dc,int width,int height){
    int unit,ox,oy;fill_background(dc,0,0,width,height);canvas(width,height,&unit,&ox,&oy);paint_content(dc,unit,ox,oy);
}
static void free_frame(void){
    if(frame.dc){SelectObject(frame.dc,frame.original);if(frame.bitmap)DeleteObject(frame.bitmap);DeleteDC(frame.dc);}
    memset(&frame,0,sizeof(frame));
}
static int render_frame(int unit){
    int width=MulDiv(UI_WIDTH,unit,10000*UI_LAYOUT_SCALE),height=MulDiv(UI_HEIGHT,unit,10000*UI_LAYOUT_SCALE);
    int required_width=width,required_height=height;RECT outer;
    if(window&&GetWindowRect(window,&outer)){if(outer.right-outer.left>required_width)required_width=outer.right-outer.left;if(outer.bottom-outer.top>required_height)required_height=outer.bottom-outer.top;}
    if(width<1||height<1)return 0;
    if(!frame.dc){frame.dc=CreateCompatibleDC(0);if(!frame.dc)return 0;}
    /* Keep one reusable canvas while settings is open: a 200% -> 100% DPI move
       must not shrink and reallocate it on every round trip. free_frame releases it. */
    if(frame.capacity_width<required_width||frame.capacity_height<required_height){
        BITMAPINFO info;HBITMAP bitmap,old;void *bits=0;int capacity_width=(required_width+63)&~63,capacity_height=(required_height+63)&~63;
        memset(&info,0,sizeof(info));info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
        info.bmiHeader.biWidth=capacity_width;info.bmiHeader.biHeight=-capacity_height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
        bitmap=CreateDIBSection(frame.dc,&info,DIB_RGB_COLORS,&bits,0,0);if(!bitmap)return 0;
        old=(HBITMAP)SelectObject(frame.dc,bitmap);
        if(!old||old==(HBITMAP)HGDI_ERROR){DeleteObject(bitmap);return 0;}
        if(frame.bitmap)DeleteObject(frame.bitmap);else frame.original=old;
        frame.bitmap=bitmap;frame.bits=bits;frame.info=info;frame.capacity_width=capacity_width;frame.capacity_height=capacity_height;canvas_allocations++;
    }
    fill_background(frame.dc,0,0,width,height);paint_content(frame.dc,unit,-MulDiv(UI_ORIGIN_X,unit,10000),-MulDiv(UI_ORIGIN_Y,unit,10000));
    GdiFlush();frame.width=width;frame.height=height;frame.unit=unit;frame_dirty=0;main_surface_ready=0;return graphics_error==0;
}
static int prepare_frame(int unit){
    int max_unit=unit+2500;
    /* Allocate a small reserve and construct all glyph outlines before showing the window. */
    if(!render_frame(max_unit))return 0;
    return render_frame(unit);
}
static void draw_frame(HDC dc,int width,int height){
    int unit,ox,oy,cw,ch,save;
    if(width<1||height<1)return;
    if(main_surface_ready){BitBlt(dc,0,0,width,height,frame.dc,main_surface_x,main_surface_y,SRCCOPY);return;}
    canvas(width,height,&unit,&ox,&oy);cw=MulDiv(UI_WIDTH,unit,10000*UI_LAYOUT_SCALE);ch=MulDiv(UI_HEIGHT,unit,10000*UI_LAYOUT_SCALE);
    ox+=MulDiv(UI_ORIGIN_X,unit,10000);oy+=MulDiv(UI_ORIGIN_Y,unit,10000);
    if(!frame.bits||frame_dirty||frame.unit!=unit){if(!render_frame(unit)){paint(dc,width,height);return;}}
    save=SaveDC(dc);
    fill_background(dc,0,0,width,oy);fill_background(dc,0,oy+ch,width,height-oy-ch);
    fill_background(dc,0,oy,ox,ch);fill_background(dc,ox+cw,oy,width-ox-cw,ch);
    /* Copy the current top-left canvas 1:1; the reserved DIB may be taller than it. */
    if(!StretchDIBits(dc,ox,oy,cw,ch,0,0,cw,ch,frame.bits,&frame.info,DIB_RGB_COLORS,SRCCOPY))paint(dc,width,height);
    RestoreDC(dc,save);
}
#include "main_surface.h"
static int render_bmp(const WCHAR *name){
    int w=UI_WIDTH,h=UI_HEIGHT,ok=1;BITMAPINFO info;BITMAPFILEHEADER file_header;void *bits=0;HDC dc;HBITMAP bitmap,old;HANDLE file;DWORD written;WCHAR path[MAX_PATH];
    memset(&info,0,sizeof(info));info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=w;info.bmiHeader.biHeight=-h;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
    dc=CreateCompatibleDC(0);bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,&bits,0,0);if(!bitmap){DeleteDC(dc);return 0;}
    old=(HBITMAP)SelectObject(dc,bitmap);{int composed=main_compositor_ready;main_compositor_ready=0;paint(dc,w,h);main_compositor_ready=composed;}GdiFlush();if(graphics_error)ok=0;
    memset(&file_header,0,sizeof(file_header));file_header.bfType=0x4D42;file_header.bfOffBits=sizeof(file_header)+sizeof(BITMAPINFOHEADER);file_header.bfSize=file_header.bfOffBits+w*h*4;
    _snwprintf(path,MAX_PATH,L"%s\\%s",folder,name);path[MAX_PATH-1]=0;
    file=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
    if(file!=INVALID_HANDLE_VALUE){if(!WriteFile(file,&file_header,sizeof(file_header),&written,0)||written!=sizeof(file_header))ok=0;if(!WriteFile(file,&info.bmiHeader,sizeof(BITMAPINFOHEADER),&written,0)||written!=sizeof(BITMAPINFOHEADER))ok=0;if(!WriteFile(file,bits,w*h*4,&written,0)||written!=w*h*4)ok=0;CloseHandle(file);}else ok=0;
    SelectObject(dc,old);DeleteObject(bitmap);DeleteDC(dc);return ok;
}
static LRESULT CALLBACK proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    if(msg==WM_CREATE){if(!main_create_caption(hwnd))return -1;return 0;}
    if(msg==WM_NCACTIVATE){LRESULT result;main_frame_border(hwnd);result=DefWindowProcW(hwnd,msg,wp,-1);InvalidateRect(hwnd,0,FALSE);return result;}
    if(msg==WM_THEMECHANGED){main_caption_theme_refresh(hwnd);main_frame_border(hwnd);redraw();InvalidateRect(main_min_button,0,FALSE);InvalidateRect(main_close_button,0,FALSE);return 0;}
    if(msg==WM_NCHITTEST)return main_caption_hit(hwnd,lp);
    if(msg==WM_NCLBUTTONDBLCLK&&wp==HTCAPTION)return 0;
    if(msg==WM_SYSCOMMAND&&(wp&0xFFF0)==SC_MAXIMIZE)return 0;
    if(msg==WM_DRAWITEM){DRAWITEMSTRUCT *item=(DRAWITEMSTRUCT*)lp;if(item->CtlID==6101||item->CtlID==6102){main_caption_draw(hwnd,item->hwndItem,item->hDC,item->CtlID==6102,item->itemState);return TRUE;}}
    if(msg==WM_COMMAND){if(LOWORD(wp)==6101){SetFocus(hwnd);ShowWindow(hwnd,SW_MINIMIZE);return 0;}if(LOWORD(wp)==6102){PostMessageW(hwnd,WM_CLOSE,0,0);return 0;}}
    if(msg==WM_GETMINMAXINFO){main_minmax(hwnd,(MINMAXINFO*)lp);return 0;}
    if(msg==WM_ENTERSIZEMOVE){main_move_begin(hwnd);return 0;}
    if(msg==WM_MOVING){main_moving((RECT*)lp);return TRUE;}
    if(msg==WM_SIZING){main_sizing(hwnd,(UINT)wp,(RECT*)lp);return TRUE;}
    if(msg==WM_PRINT||msg==WM_PRINTCLIENT){RECT rect;main_surface_present(hwnd);if(msg==WM_PRINT&&(lp&PRF_NONCLIENT)){BitBlt((HDC)wp,0,0,main_surface_width,main_surface_height,frame.dc,0,0,SRCCOPY);main_compositor_snapshot((HDC)wp,main_surface_x,main_surface_y);}else{GetClientRect(hwnd,&rect);BitBlt((HDC)wp,0,0,rect.right,rect.bottom,frame.dc,main_surface_x,main_surface_y,SRCCOPY);main_compositor_snapshot((HDC)wp,0,0);}return 0;}
    if(msg==WM_PAINT){PAINTSTRUCT ps;BeginPaint(hwnd,&ps);EndPaint(hwnd,&ps);main_surface_present(hwnd);return 0;}
    if(msg==WM_APP+24)return main_surface_snapshot(hwnd);
    if(msg==WM_ERASEBKGND)return 1;
    if(msg==WM_NCPAINT)return 0;
    if(msg==WM_NCCALCSIZE)return main_client_bounds(hwnd,wp,lp);
    if(msg==WM_MOUSEMOVE){main_feedback_track(hwnd);main_feedback_hot(main_feedback_point(hwnd,lp));return 0;}
    if(msg==WM_MOUSELEAVE){main_mouse_tracking=0;main_feedback_hot(0);return 0;}
    if(msg==WM_SETCURSOR&&LOWORD(lp)==HTCLIENT){SetCursor(LoadCursorW(0,main_hot_item?IDC_HAND:IDC_ARROW));return TRUE;}
    if(msg==WM_LBUTTONDOWN){int item=main_feedback_point(hwnd,lp);if(!item)return 0;
        main_feedback_cancel(hwnd,0);SetFocus(hwnd);if(focus_item){focus_item=0;redraw();}main_feedback_hot(item);main_pressed_item=item;main_press_source=1;SetCapture(hwnd);main_feedback_track(hwnd);main_feedback_redraw(item);main_feedback_properties();return 0;
    }
    if(msg==WM_LBUTTONUP){int item=main_feedback_point(hwnd,lp),pressed=main_press_source==1?main_pressed_item:0;
        main_feedback_cancel(hwnd,0);main_feedback_hot(item);if(pressed&&pressed==item)main_activate_item(item);return 0;
    }
    if(msg==WM_CAPTURECHANGED&&(HWND)lp!=hwnd&&main_press_source==1){main_feedback_cancel(hwnd,1);return 0;}
    if(msg==WM_CANCELMODE||msg==WM_KILLFOCUS){main_feedback_cancel(hwnd,1);}
    if(msg==WM_CLOSE){request_close(hwnd);return 0;}
    if(msg==WM_EXITSIZEMOVE){main_move_active=0;main_move_edge=0;InvalidateRect(hwnd,0,FALSE);UpdateWindow(hwnd);return 0;}
    if(msg==WM_SIZE){if(wp==SIZE_MINIMIZED){main_was_minimized=1;main_feedback_suspend(hwnd);}else{if(main_was_minimized){main_was_minimized=0;main_constrain_size(hwnd);main_refit_window(hwnd);main_feedback_tick();}main_place_caption(hwnd);main_surface_touch(hwnd);}return 0;}
    if(msg==0x02E0){main_dpi_changed(hwnd,LOWORD(wp),(RECT*)lp);main_constrain_size(hwnd);main_place_caption(hwnd);main_surface_touch(hwnd);return 0;}
    if(msg==WM_DISPLAYCHANGE||(msg==WM_SETTINGCHANGE&&wp==SPI_SETWORKAREA)){main_refit_window(hwnd);main_place_caption(hwnd);main_surface_touch(hwnd);if(msg==WM_DISPLAYCHANGE)return 0;}
    if(msg==WM_KEYDOWN&&wp==VK_ESCAPE){PostMessageW(hwnd,WM_CLOSE,0,0);return 0;}
    if(msg==WM_KEYDOWN&&wp==VK_TAB){int count=4;main_feedback_cancel(hwnd,0);focus_item=(GetKeyState(VK_SHIFT)&0x8000)?(focus_item<=1?count:focus_item-1):(focus_item%count+1);redraw();return 0;}
    if(msg==WM_KEYDOWN&&(wp==VK_SPACE||wp==VK_RETURN)&&focus_item){
        if(!(lp&(1L<<30))&&!main_pressed_item){main_pressed_item=focus_item;main_press_source=2;main_pressed_key=(int)wp;main_feedback_redraw(focus_item);main_feedback_properties();}return 0;
    }
    if(msg==WM_KEYUP&&(wp==VK_SPACE||wp==VK_RETURN)){
        int item=main_press_source==2&&main_pressed_key==(int)wp?main_pressed_item:0;if(item){main_feedback_cancel(hwnd,0);if(item==focus_item)main_activate_item(item);}return 0;
    }
    if(msg==WM_APP+9){setup_action();return 0;} /* Shell COM calls cannot run inside a synchronous input message. */
    if(msg==WM_APP+10){setup_copy_ai();return 0;}
    if(msg==WM_ACTIVATE&&LOWORD(wp)==WA_INACTIVE)main_feedback_cancel(hwnd,1);
    if(msg==WM_ACTIVATE&&LOWORD(wp)!=WA_INACTIVE&&!isolated&&(!test_no_host||setup_validation)){SetTimer(hwnd,4,100,0);return 0;}
    if(msg==WM_SETTINGCHANGE)main_feedback_motion();
    if(msg==WM_SETTINGCHANGE&&!isolated&&(!test_no_host||setup_validation)){SetTimer(hwnd,4,100,0);return 0;}
    if(msg==WM_TIMER){WCHAR previous_footer[180];int previous_on=on,previous_startup=startup;
        if(wp==MAIN_FEEDBACK_TIMER){main_feedback_tick();return 0;}
        if(wp==4){if(main_feedback_active()){SetTimer(hwnd,4,MAIN_FEEDBACK_DURATION,0);return 0;}KillTimer(hwnd,4);tp_check();refresh();redraw();return 0;}
        if(wp==3&&close_pending){if(tray_ready()){DestroyWindow(hwnd);return 0;}if((LONG)(GetTickCount()-close_deadline)>=0){close_pending=0;KillTimer(hwnd,3);ShowWindow(hwnd,SW_RESTORE);MessageBoxW(hwnd,L"系统托盘暂时不可用，设置窗口已保留。请稍后再次关闭。",L"三指拖拽",MB_OK|MB_ICONERROR);}return 0;}
        if(probe&&wp==2){DestroyWindow(hwnd);return 0;}wcscpy(previous_footer,footer);refresh();
        if(previous_on!=on||previous_startup!=startup||wcscmp(previous_footer,footer))redraw();return 0;}
    if(msg==WM_APP+3){DestroyWindow(hwnd);return 0;}
    if(msg==WM_DESTROY){main_feedback_suspend(hwnd);main_compositor_free();free_frame();PostQuitMessage(0);return 0;}return DefWindowProcW(hwnd,msg,wp,lp);
}
int WINAPI WinMain(HINSTANCE instance,HINSTANCE previous,LPSTR command,int show){
    WNDCLASSW wc;MSG message;WCHAR icon_path[MAX_PATH];RECT window_rect,work_area;MONITORINFO initial_info;POINT initial_pointer={0,0};HMONITOR initial_monitor;int result,width,height,unit,ox,oy;HMODULE user,dwm=0;HANDLE instance_mutex=0;
    BOOL (WINAPI *dpi_context)(HANDLE);DWM_ATTRIBUTE attr;
    user=GetModuleHandleW(L"user32.dll");dpi_context=(void*)GetProcAddress(user,"SetProcessDpiAwarenessContext");if(dpi_context)dpi_context((HANDLE)-4);
    isolated=strstr(command,"--render-preview")!=0||strstr(command,"--render-prepare")!=0||strstr(command,"--ui-probe")!=0;
    test_no_host=strstr(command,"--no-host-launch")!=0;
    main_compositor_disabled=strstr(command,"--software-animation")!=0;
    setup_validation=strstr(command,"--setup-validation")!=0;
    if(!app_paths(isolated))return 1;
    if(strstr(command,"--setup-validation"))wcscpy(data_folder,app_folder); /* Diagnostic backups stay out of the user's first real setup backup. */
    if(strstr(command,"--prepare-check")||strstr(command,"--prepare-system")){int applied=0;tp_check();if(strstr(command,"--prepare-system"))applied=tp_prepare();if(!tp_report(applied))return 2;return tp_state.ready?0:3;}
    if(strstr(command,"--tray-menu"))return run_tray_menu(instance);
    if(strstr(command,"--quit")){HWND existing=FindWindowW(APP_UI_CLASS,0);if(existing)PostMessageW(existing,WM_APP+3,0,0);return 0;}
    if(!isolated&&!test_no_host&&!strstr(command,"--render-current")){
        instance_mutex=CreateMutexW(0,TRUE,L"Local\\ThreeFingerDrag-Native-Settings");if(!instance_mutex)return 1;
        if(GetLastError()==ERROR_ALREADY_EXISTS){HWND existing=FindWindowW(APP_UI_CLASS,0);if(existing){ShowWindow(existing,SW_RESTORE);SetForegroundWindow(existing);}CloseHandle(instance_mutex);return 0;}
    }
    if(isolated||(test_no_host&&!setup_validation))tp_state.ready=1;else tp_check();
    if(!isolated&&!test_no_host)setup_failed=tp_previous_failure();
    if(strstr(command,"--render-preview")||strstr(command,"--render-prepare"))startup=1;
    if(strstr(command,"--render-prepare")||strstr(command,"--layout-setup")){tp_state.ready=0;tp_state.native_ok=tp_state.cache_ok=tp_state.present=tp_state.enabled=1;}
    _snwprintf(folder,MAX_PATH,L"%s",app_folder);refresh();
    if(strstr(command,"--render-prepare")){wcscpy(status_label,L"需设置");wcscpy(footer,setup_guidance);status_color=RGB(155,92,10);status_fill=RGB(255,245,224);}
    if(!isolated&&!strstr(command,"--render-current")){
        if(!WritePrivateProfileStringW(L"Window",L"CloseToMinimize",0,config_path))return 1;
    }
    if(!isolated&&!strstr(command,"--render-current")){HWND existing=FindWindowW(APP_UI_CLASS,0);if(existing){ShowWindow(existing,SW_RESTORE);SetForegroundWindow(existing);return 0;}
        if(!test_no_host&&!FindWindowW(APP_HOST_CLASS,0))app_launch(L"GestureHost.exe",L"--background");}
    if(!graphics_init()){graphics_cleanup();MessageBoxW(0,L"Windows 图形组件未能初始化。三指拖拽后台不受影响。",L"三指拖拽",MB_OK|MB_ICONERROR);return 1;}
    background_brush=CreateSolidBrush(background_color);if(!background_brush){graphics_cleanup();return 1;}
    if(!init_text()){free_text();DeleteObject(background_brush);graphics_cleanup();return 1;}
    if(!main_art_init()){free_text();DeleteObject(background_brush);graphics_cleanup();return 1;}
    main_feedback_init();
    if(strstr(command,"--render-current"))wcscpy(folder,data_folder);
    if(strstr(command,"--render-preview")||strstr(command,"--render-prepare")||strstr(command,"--render-current")){result=render_bmp(L"ui-preview.bmp")?0:1;free_text();DeleteObject(background_brush);graphics_cleanup();return result;}
    memset(&wc,0,sizeof(wc));wc.lpfnWndProc=proc;wc.hInstance=instance;wc.lpszClassName=APP_UI_CLASS;wc.hCursor=LoadCursorW(0,IDC_ARROW);
    _snwprintf(icon_path,MAX_PATH,L"%s\\Gesture.ico",app_folder);wc.hIcon=LoadImageW(instance,MAKEINTRESOURCEW(1),IMAGE_ICON,128,128,0);if(!wc.hIcon)wc.hIcon=LoadImageW(0,icon_path,IMAGE_ICON,128,128,LR_LOADFROMFILE);
    if(!RegisterClassW(&wc))return 1;
    /* Create a small hidden window on the opening monitor first, so Windows gives
       us that monitor's real DPI rather than the primary monitor's system DPI. */
    GetCursorPos(&initial_pointer);initial_monitor=MonitorFromPoint(initial_pointer,MONITOR_DEFAULTTOPRIMARY);
    memset(&initial_info,0,sizeof(initial_info));initial_info.cbSize=sizeof(initial_info);
    if(GetMonitorInfoW(initial_monitor,&initial_info))work_area=initial_info.rcWork;
    else if(!SystemParametersInfoW(SPI_GETWORKAREA,0,&work_area,0)){work_area.left=work_area.top=0;work_area.right=GetSystemMetrics(SM_CXSCREEN);work_area.bottom=GetSystemMetrics(SM_CYSCREEN);}
    window=CreateWindowExW(WS_EX_APPWINDOW|WS_EX_LAYERED,wc.lpszClassName,L"三指拖拽",UI_WINDOW_STYLE,work_area.left+(work_area.right-work_area.left-128)/2,work_area.top+(work_area.bottom-work_area.top-128)/2,128,128,0,0,instance,0);if(!window)return 1;
    SendMessageW(window,WM_SETICON,ICON_BIG,(LPARAM)wc.hIcon);SendMessageW(window,WM_SETICON,ICON_SMALL,(LPARAM)wc.hIcon);
    /* Keep a caption-free layered popup, as in the existing tray panel. */
    SetWindowLongPtrW(window,GWL_STYLE,GetWindowLongPtrW(window,GWL_STYLE)&~(WS_CAPTION|WS_MAXIMIZEBOX));
    main_default_monitor_rect(window,MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),&window_rect);width=window_rect.right-window_rect.left;height=window_rect.bottom-window_rect.top;
    SetWindowPos(window,0,window_rect.left,window_rect.top,width,height,SWP_FRAMECHANGED|SWP_NOZORDER|SWP_NOACTIVATE);
    SetPropW(window,L"ThreeFingerDrag-DefaultWidthDip",(HANDLE)(ULONG_PTR)MAIN_DEFAULT_CLIENT_DIP_WIDTH);
    main_constrain_size(window);
    refresh();
    dwm=LoadLibraryW(L"dwmapi.dll");if(dwm){attr=(void*)GetProcAddress(dwm,"DwmSetWindowAttribute");main_dwm_attribute=attr;if(attr)main_frame_border(window);}
    GetClientRect(window,&window_rect);canvas(window_rect.right,window_rect.bottom,&unit,&ox,&oy);
    if(!prepare_frame(unit)||!main_surface_present(window)){DestroyWindow(window);free_text();DeleteObject(background_brush);if(wc.hIcon)DestroyIcon(wc.hIcon);if(dwm)FreeLibrary(dwm);graphics_cleanup();return 1;}
    probe=strstr(command,"--ui-probe")!=0;
    if(probe){render_bmp(L"ui-preview.bmp");SetTimer(window,2,10000,0);}else if(!strstr(command,"--hidden-check")){ShowWindow(window,SW_SHOW);UpdateWindow(window);}
    SetTimer(window,1,1000,0);
    while((result=main_feedback_message(&message))>0){TranslateMessage(&message);DispatchMessageW(&message);}
    main_surface_free();free_text();DeleteObject(background_brush);if(wc.hIcon)DestroyIcon(wc.hIcon);if(dwm)FreeLibrary(dwm);graphics_cleanup();if(instance_mutex){ReleaseMutex(instance_mutex);CloseHandle(instance_mutex);}return result<0?1:0;
}
