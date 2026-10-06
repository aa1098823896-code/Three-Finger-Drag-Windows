#ifndef THREE_FINGER_MAIN_CAPTION_H
#define THREE_FINGER_MAIN_CAPTION_H
/* Two caption controls use Windows stock glyphs and UxTheme text rendering.
   The OS-owned caption group cannot omit only the disabled maximize button. */
#define MAIN_CAPTION_WIDTH 46
#define MAIN_CAPTION_HEIGHT 32
#define MAIN_CAPTION_Y UI_ORIGIN_Y
#define MAIN_CLOSE_X (UI_ORIGIN_X+UI_WIDTH/UI_LAYOUT_SCALE-MAIN_CAPTION_WIDTH)
#define MAIN_MIN_X (MAIN_CLOSE_X-MAIN_CAPTION_WIDTH)
typedef struct {
    DWORD size,flags;COLORREF text,border,shadow;int shadow_type;POINT shadow_offset;
    int border_size,font_property,color_property,state;BOOL overlay;int glow_size;
    void *callback;LPARAM callback_data;
} MAIN_THEME_TEXT_OPTIONS;
static HMODULE main_theme_library;
static HANDLE main_theme;
static HFONT main_caption_font;
static HBRUSH main_caption_close_hot,main_caption_close_pressed,main_caption_min_pressed;
static HWND main_caption_theme_window,main_caption_hot;
static UINT main_caption_visual[2];
static WNDPROC main_caption_base;
static HANDLE (WINAPI *main_open_theme)(HWND,LPCWSTR);
static HRESULT (WINAPI *main_close_theme)(HANDLE);
static HRESULT (WINAPI *main_theme_text)(HANDLE,HDC,int,int,LPCWSTR,int,DWORD,LPRECT,const MAIN_THEME_TEXT_OPTIONS *);
static COLORREF (WINAPI *main_theme_color)(HANDLE,int);
static void main_caption_theme_free(void){
    if(main_theme&&main_close_theme)main_close_theme(main_theme);main_theme=0;
    if(main_caption_font)DeleteObject(main_caption_font);main_caption_font=0;
    if(main_caption_close_hot)DeleteObject(main_caption_close_hot);main_caption_close_hot=0;
    if(main_caption_close_pressed)DeleteObject(main_caption_close_pressed);main_caption_close_pressed=0;
    if(main_caption_min_pressed)DeleteObject(main_caption_min_pressed);main_caption_min_pressed=0;
    if(main_theme_library)FreeLibrary(main_theme_library);main_theme_library=0;
    main_caption_theme_window=main_caption_hot=0;
}
static void main_caption_theme_refresh(HWND hwnd){
    COLORREF face=GetSysColor(COLOR_BTNFACE),shadow=GetSysColor(COLOR_3DSHADOW);
    if(main_theme&&main_close_theme)main_close_theme(main_theme);
    main_theme=main_open_theme?main_open_theme(hwnd,L"WINDOW"):0;
    main_caption_theme_window=hwnd;
    if(main_caption_min_pressed)DeleteObject(main_caption_min_pressed);
    main_caption_min_pressed=CreateSolidBrush(RGB((2*GetRValue(face)+GetRValue(shadow))/3,(2*GetGValue(face)+GetGValue(shadow))/3,(2*GetBValue(face)+GetBValue(shadow))/3));
}
static int main_caption_theme_init(void){
    main_theme_library=LoadLibraryExW(L"uxtheme.dll",0,0x00000800);
    if(main_theme_library){
        main_open_theme=(void*)GetProcAddress(main_theme_library,"OpenThemeData");
        main_close_theme=(void*)GetProcAddress(main_theme_library,"CloseThemeData");
        main_theme_text=(void*)GetProcAddress(main_theme_library,"DrawThemeTextEx");
        main_theme_color=(void*)GetProcAddress(main_theme_library,"GetThemeSysColor");
    }
    main_caption_font=CreateFontW(-10,0,0,0,FW_NORMAL,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,ANTIALIASED_QUALITY,DEFAULT_PITCH,L"Segoe MDL2 Assets");
    main_caption_close_hot=CreateSolidBrush(RGB(196,43,28));main_caption_close_pressed=CreateSolidBrush(RGB(161,35,24));
    main_caption_theme_refresh(0);return main_caption_font&&main_caption_close_hot&&main_caption_close_pressed&&main_caption_min_pressed;
}
static void main_caption_hover(HWND hwnd){
    HWND previous=main_caption_hot;
    if(previous==hwnd)return;main_caption_hot=hwnd;
    if(previous){main_caption_visual[GetDlgCtrlID(previous)==6102]&=~ODS_HOTLIGHT;InvalidateRect(previous,0,FALSE);main_surface_touch(GetParent(previous));}
    if(hwnd){main_caption_visual[GetDlgCtrlID(hwnd)==6102]|=ODS_HOTLIGHT;InvalidateRect(hwnd,0,FALSE);main_surface_touch(GetParent(hwnd));}
}
static LRESULT CALLBACK main_caption_button_proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp){
    LRESULT result;
    if(msg==WM_NCHITTEST){LRESULT hit=SendMessageW(GetParent(hwnd),WM_NCHITTEST,wp,lp);
        if(hit>=HTLEFT&&hit<=HTBOTTOMRIGHT)return HTTRANSPARENT;
    }
    if(msg==WM_MOUSEMOVE){TRACKMOUSEEVENT track;main_caption_hover(hwnd);memset(&track,0,sizeof(track));track.cbSize=sizeof(track);track.dwFlags=TME_LEAVE;track.hwndTrack=hwnd;TrackMouseEvent(&track);}
    if(msg==WM_MOUSELEAVE&&main_caption_hot==hwnd)main_caption_hover(0);
    if(msg==WM_DESTROY&&main_caption_hot==hwnd)main_caption_hot=0;
    result=CallWindowProcW(main_caption_base,hwnd,msg,wp,lp);
    if(msg==WM_MOUSEMOVE||msg==WM_MOUSELEAVE||msg==BM_SETSTATE||msg==WM_LBUTTONDOWN||msg==WM_LBUTTONUP||msg==WM_CAPTURECHANGED||msg==WM_CANCELMODE||msg==WM_ENABLE){
        int close=GetDlgCtrlID(hwnd)==6102;UINT flags=(main_caption_hot==hwnd?ODS_HOTLIGHT:0)|((SendMessageW(hwnd,BM_GETSTATE,0,0)&BST_PUSHED)?ODS_SELECTED:0)|(IsWindowEnabled(hwnd)?0:ODS_DISABLED);
        if(flags!=main_caption_visual[close]){main_caption_visual[close]=flags;main_surface_touch(GetParent(hwnd));}
    }
    return result;
}
static void main_caption_glyph(HDC dc,int close,UINT state,int unit,int ox,int oy){
    RECT rect={close?MAIN_CLOSE_X:MAIN_MIN_X,MAIN_CAPTION_Y,0,MAIN_CAPTION_Y+MAIN_CAPTION_HEIGHT};
    XFORM transform;MAIN_THEME_TEXT_OPTIONS options;WCHAR glyph[2]={close?0xE8BB:0xE921,0};
    int saved=SaveDC(dc),hot=(state&ODS_HOTLIGHT)!=0,pressed=(state&ODS_SELECTED)!=0;
    COLORREF color=main_theme&&main_theme_color?main_theme_color(main_theme,COLOR_BTNTEXT):GetSysColor(COLOR_BTNTEXT);
    rect.right=rect.left+MAIN_CAPTION_WIDTH;
    SetGraphicsMode(dc,GM_ADVANCED);memset(&transform,0,sizeof(transform));transform.eM11=transform.eM22=unit/10000.0f;transform.eDx=(float)ox;transform.eDy=(float)oy;SetWorldTransform(dc,&transform);
    if(hot||pressed){
        if(close){FillRect(dc,&rect,pressed?main_caption_close_pressed:main_caption_close_hot);color=RGB(255,255,255);}
        else FillRect(dc,&rect,pressed?main_caption_min_pressed:GetSysColorBrush(COLOR_BTNFACE));
    }
    if(state&ODS_DISABLED)color=GetSysColor(COLOR_GRAYTEXT);
    SelectObject(dc,main_caption_font);SetBkMode(dc,TRANSPARENT);
    memset(&options,0,sizeof(options));options.size=sizeof(options);options.flags=1;options.text=color;
    if(main_theme&&main_theme_text&&SUCCEEDED(main_theme_text(main_theme,dc,0,1,glyph,1,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX,&rect,&options))){
        if(main_caption_theme_window)SetPropW(main_caption_theme_window,L"ThreeFingerDrag-CaptionThemeRendering",(HANDLE)1);
    }else{SetTextColor(dc,color);DrawTextW(dc,glyph,1,&rect,DT_CENTER|DT_VCENTER|DT_SINGLELINE|DT_NOPREFIX);}
    RestoreDC(dc,saved);
}
#endif
