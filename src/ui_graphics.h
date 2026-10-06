#ifndef THREE_FINGER_UI_GRAPHICS_H
#define THREE_FINGER_UI_GRAPHICS_H
/* GDI+ belongs only to the settings process; the input host stays unchanged. */
typedef struct {
    UINT version; void *debug_callback; BOOL suppress_thread, suppress_codecs;
} UI_GRAPHICS_STARTUP;
static HMODULE graphics_library;
static ULONG_PTR graphics_token;
static void *graphics;
static int graphics_error;
static int (WINAPI *graphics_startup)(ULONG_PTR *,const UI_GRAPHICS_STARTUP *,void *);
static void (WINAPI *graphics_shutdown)(ULONG_PTR);
#define UI_GP(name,args) static int (WINAPI *name) args
UI_GP(GdipCreateFromHDC,(HDC,void **));
UI_GP(GdipDeleteGraphics,(void *));
UI_GP(GdipSetSmoothingMode,(void *,int));
UI_GP(GdipSetPixelOffsetMode,(void *,int));
UI_GP(GdipSetPageUnit,(void *,int));
UI_GP(GdipCreateMatrix2,(float,float,float,float,float,float,void **));
UI_GP(GdipSetWorldTransform,(void *,void *));
UI_GP(GdipDeleteMatrix,(void *));
UI_GP(GdipCreatePath,(int,void **));
UI_GP(GdipAddPathArc,(void *,float,float,float,float,float,float));
UI_GP(GdipClosePathFigure,(void *));
UI_GP(GdipFillPath,(void *,void *,void *));
UI_GP(GdipDrawPath,(void *,void *,void *));
UI_GP(GdipDeletePath,(void *));
UI_GP(GdipCreateSolidFill,(DWORD,void **));
UI_GP(GdipDeleteBrush,(void *));
UI_GP(GdipCreatePen1,(DWORD,float,int,void **));
UI_GP(GdipSetPenStartCap,(void *,int));
UI_GP(GdipSetPenEndCap,(void *,int));
UI_GP(GdipDeletePen,(void *));
UI_GP(GdipFillEllipse,(void *,void *,float,float,float,float));
UI_GP(GdipDrawLine,(void *,void *,float,float,float,float));
UI_GP(GdipCreateBitmapFromScan0,(int,int,int,int,BYTE *,void **));
UI_GP(GdipDrawImageRectI,(void *,void *,int,int,int,int));
UI_GP(GdipDisposeImage,(void *));
UI_GP(GdipSetInterpolationMode,(void *,int));
typedef struct {float x,y,width,height;} UI_RECTF;
UI_GP(GdipCreateFontFamilyFromName,(const WCHAR *,void *,void **));
UI_GP(GdipDeleteFontFamily,(void *));
UI_GP(GdipCreateStringFormat,(int,int,void **));
UI_GP(GdipSetStringFormatAlign,(void *,int));
UI_GP(GdipSetStringFormatLineAlign,(void *,int));
UI_GP(GdipDeleteStringFormat,(void *));
UI_GP(GdipAddPathString,(void *,const WCHAR *,int,const void *,int,float,const UI_RECTF *,const void *));
UI_GP(GdipGetPathWorldBounds,(void *,UI_RECTF *,const void *,const void *));
UI_GP(GdipTransformPath,(void *,void *));
#undef UI_GP
static void graphics_check(int status){if(status)graphics_error=status;}
static int graphics_init(void){
    UI_GRAPHICS_STARTUP input;
    graphics_library=LoadLibraryExW(L"gdiplus.dll",0,0x00000800);
    if(!graphics_library)return 0;
    graphics_startup=(void*)GetProcAddress(graphics_library,"GdiplusStartup");
    graphics_shutdown=(void*)GetProcAddress(graphics_library,"GdiplusShutdown");
    if(!graphics_startup||!graphics_shutdown)return 0;
#define UI_LOAD(name) name=(void*)GetProcAddress(graphics_library,#name);if(!name)return 0
    UI_LOAD(GdipCreateFromHDC);UI_LOAD(GdipDeleteGraphics);
    UI_LOAD(GdipSetSmoothingMode);UI_LOAD(GdipSetPixelOffsetMode);UI_LOAD(GdipSetPageUnit);
    UI_LOAD(GdipCreateMatrix2);UI_LOAD(GdipSetWorldTransform);UI_LOAD(GdipDeleteMatrix);
    UI_LOAD(GdipCreatePath);UI_LOAD(GdipAddPathArc);UI_LOAD(GdipClosePathFigure);
    UI_LOAD(GdipFillPath);UI_LOAD(GdipDrawPath);UI_LOAD(GdipDeletePath);
    UI_LOAD(GdipCreateSolidFill);UI_LOAD(GdipDeleteBrush);
    UI_LOAD(GdipCreatePen1);UI_LOAD(GdipSetPenStartCap);UI_LOAD(GdipSetPenEndCap);UI_LOAD(GdipDeletePen);
    UI_LOAD(GdipFillEllipse);UI_LOAD(GdipDrawLine);
    UI_LOAD(GdipCreateBitmapFromScan0);UI_LOAD(GdipDrawImageRectI);UI_LOAD(GdipDisposeImage);UI_LOAD(GdipSetInterpolationMode);
    UI_LOAD(GdipCreateFontFamilyFromName);UI_LOAD(GdipDeleteFontFamily);
    UI_LOAD(GdipCreateStringFormat);UI_LOAD(GdipSetStringFormatAlign);UI_LOAD(GdipSetStringFormatLineAlign);UI_LOAD(GdipDeleteStringFormat);UI_LOAD(GdipAddPathString);UI_LOAD(GdipGetPathWorldBounds);UI_LOAD(GdipTransformPath);
#undef UI_LOAD
    memset(&input,0,sizeof(input));input.version=1;
    return graphics_startup(&graphics_token,&input,0)==0;
}
static void graphics_cleanup(void){
    if(graphics_token){graphics_shutdown(graphics_token);graphics_token=0;}
    if(graphics_library){FreeLibrary(graphics_library);graphics_library=0;}
}
static DWORD graphics_color(COLORREF color){
    return 0xFF000000|((DWORD)GetRValue(color)<<16)|((DWORD)GetGValue(color)<<8)|GetBValue(color);
}
static int graphics_begin(HDC dc,float scale,int x,int y){
    void *matrix=0;
    graphics_error=0;graphics=0;
    graphics_check(GdipCreateFromHDC(dc,&graphics));if(!graphics)return 0;
    graphics_check(GdipSetSmoothingMode(graphics,4)); /* AntiAlias */
    graphics_check(GdipSetPixelOffsetMode(graphics,4)); /* Half-pixel sampling */
    graphics_check(GdipSetPageUnit(graphics,2)); /* Pixel */
    graphics_check(GdipSetInterpolationMode(graphics,7)); /* HighQualityBicubic for the shared app icon. */
    graphics_check(GdipCreateMatrix2(scale,0,0,scale,(float)x,(float)y,&matrix));
    if(matrix){graphics_check(GdipSetWorldTransform(graphics,matrix));GdipDeleteMatrix(matrix);}
    return graphics_error==0;
}
static void graphics_end(void){if(graphics){graphics_check(GdipDeleteGraphics(graphics));graphics=0;}}
static void box(int x,int y,int w,int h,int radius,COLORREF color,COLORREF border){
    void *path=0,*brush=0,*pen=0;float r=(float)radius,d;
    if(r>w/2.0f)r=w/2.0f;if(r>h/2.0f)r=h/2.0f;d=2*r;
    graphics_check(GdipCreatePath(0,&path));if(!path)return;
    graphics_check(GdipAddPathArc(path,(float)x,(float)y,d,d,180,90));
    graphics_check(GdipAddPathArc(path,x+w-d,(float)y,d,d,270,90));
    graphics_check(GdipAddPathArc(path,x+w-d,y+h-d,d,d,0,90));
    graphics_check(GdipAddPathArc(path,(float)x,y+h-d,d,d,90,90));
    graphics_check(GdipClosePathFigure(path));
    graphics_check(GdipCreateSolidFill(graphics_color(color),&brush));
    if(brush){graphics_check(GdipFillPath(graphics,brush,path));GdipDeleteBrush(brush);}
    if(border!=color){
        graphics_check(GdipCreatePen1(graphics_color(border),1,2,&pen));
        if(pen){graphics_check(GdipDrawPath(graphics,pen,path));GdipDeletePen(pen);}
    }
    GdipDeletePath(path);
}
static void circle(int x,int y,int r,COLORREF color){
    void *brush=0;graphics_check(GdipCreateSolidFill(graphics_color(color),&brush));
    if(brush){graphics_check(GdipFillEllipse(graphics,brush,(float)(x-r),(float)(y-r),(float)(2*r),(float)(2*r)));GdipDeleteBrush(brush);}
}
static void line(int x,int y,int xx,int yy,COLORREF color,int width){
    void *pen=0;graphics_check(GdipCreatePen1(graphics_color(color),(float)width,2,&pen));
    if(pen){
        graphics_check(GdipSetPenStartCap(pen,2));graphics_check(GdipSetPenEndCap(pen,2));
        graphics_check(GdipDrawLine(graphics,pen,(float)x,(float)y,(float)xx,(float)yy));GdipDeletePen(pen);
    }
}
#endif
