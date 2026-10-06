#ifndef THREE_FINGER_MAIN_SURFACE_H
#define THREE_FINGER_MAIN_SURFACE_H
/* Reuse the existing DIB for one premultiplied-alpha window surface.
   Curves, content and shadow are presented together, as in the tray panel. */
static int main_surface_radius;
static BYTE *main_curve_coverage;
static int main_curve_capacity,main_curve_radius;
static float main_shadow_falloff=9.0f,main_shadow_offset=2.5f;
static float main_round_distance(int x,int y,int width,int height,int radius){
    /* Integer coordinates avoid TCC x64 aliasing of adjacent float-expression arguments. */
    float qx=x+0.5f-width/2.0f,qy=y-main_shadow_offset-height/2.0f,ax,ay,m;
    if(qx<0)qx=-qx;if(qy<0)qy=-qy;qx-=width/2.0f-radius;qy-=height/2.0f-radius;
    ax=qx>0?qx:0;ay=qy>0?qy:0;m=qx>qy?qx:qy;
    return (ax==0?ay:ay==0?ax:(float)sqrt(ax*ax+ay*ay))+(m<0?m:0)-radius;
}
static int main_prepare_curve(int radius){
    int x,y,count=radius*radius;BYTE *bytes;
    if(radius==main_curve_radius)return 1;
    if(count>main_curve_capacity){bytes=HeapAlloc(GetProcessHeap(),0,count);if(!bytes)return 0;if(main_curve_coverage)HeapFree(GetProcessHeap(),0,main_curve_coverage);main_curve_coverage=bytes;main_curve_capacity=count;}
    for(y=0;y<radius;y++)for(x=0;x<radius;x++){
        float dx=radius-x-0.5f,dy=radius-y-0.5f,a=0.5f-((float)sqrt(dx*dx+dy*dy)-radius);
        if(a<0)a=0;if(a>1)a=1;main_curve_coverage[y*radius+x]=(BYTE)(a*255+0.5f);
    }
    main_curve_radius=radius;return 1;
}
static void main_surface_free(void){if(main_curve_coverage)HeapFree(GetProcessHeap(),0,main_curve_coverage);main_curve_coverage=0;main_curve_capacity=main_curve_radius=0;main_surface_ready=0;}
static void main_surface_opaque(DWORD *p,int count){
    DWORD *end=p+count;const ULONGLONG mask=0xFF000000FF000000ULL;
    while(end-p>=8){((ULONGLONG*)p)[0]|=mask;((ULONGLONG*)p)[1]|=mask;((ULONGLONG*)p)[2]|=mask;((ULONGLONG*)p)[3]|=mask;p+=8;}
    while(end-p>=2){*(ULONGLONG*)p|=mask;p+=2;}if(p<end)*p|=0xFF000000;
}
static void main_surface_shadow(DWORD *pixel,int x,int y,int width,int height,int radius){
    unsigned a=*pixel>>24;float distance,fade;int shadow;if(a==255)return;
    distance=main_round_distance(x,y,width,height,radius);if(distance<0)distance=0;
    fade=1-distance/main_shadow_falloff;if(fade<0)fade=0;shadow=(int)(24*fade*fade);a+=shadow*(255-a)/255;*pixel=(*pixel&0x00FFFFFF)|(a<<24);
}
static void main_surface_caption(HDC dc,int close,int unit,int ox,int oy){
    RECT rect={ox+MulDiv(close?MAIN_CLOSE_X:MAIN_MIN_X,unit,10000),oy+MulDiv(MAIN_CAPTION_Y,unit,10000),0,0};int save;
    rect.right=rect.left+MulDiv(MAIN_CAPTION_WIDTH,unit,10000);rect.bottom=rect.top+MulDiv(MAIN_CAPTION_HEIGHT,unit,10000);
    if(!main_caption_visual[close])return;save=SaveDC(dc);IntersectClipRect(dc,rect.left,rect.top,rect.right,rect.bottom);
    if(graphics_begin(dc,unit/10000.0f,ox,oy))graphics_check(main_fill_rect(graphics,main_background,UI_ORIGIN_X,UI_ORIGIN_Y,UI_WIDTH/(float)UI_LAYOUT_SCALE,UI_HEIGHT/(float)UI_LAYOUT_SCALE));graphics_end();
    main_caption_glyph(dc,close,main_caption_visual[close],unit,ox,oy);RestoreDC(dc,save);
}
static int main_surface_present(HWND hwnd){
    RECT outer,client;POINT offset={0,0},source={0,0};SIZE size;BLENDFUNCTION blend={AC_SRC_OVER,0,255,AC_SRC_ALPHA};int unit,ox,oy,x,y,cw,ch,radius;
    DWORD *pixels;BOOL (WINAPI *update)(HWND,HDC,const POINT *,const SIZE *,HDC,const POINT *,COLORREF,const BLENDFUNCTION *,DWORD);
    if(IsIconic(hwnd))return 1;if(!frame.bits||!GetWindowRect(hwnd,&outer)||!GetClientRect(hwnd,&client)||!ClientToScreen(hwnd,&offset))return 0;
    size.cx=outer.right-outer.left;size.cy=outer.bottom-outer.top;offset.x-=outer.left;offset.y-=outer.top;canvas(client.right,client.bottom,&unit,&ox,&oy);
    radius=MulDiv(MAIN_CARD_RADIUS,unit,10000);if(radius<1)radius=1;
    main_compositor_prepare(hwnd,unit,ox,oy);
    if(!main_surface_ready||main_surface_dirty||frame_dirty||size.cx!=main_surface_width||size.cy!=main_surface_height||unit!=frame.unit){
        int inset=offset.x,bottom=size.cy-offset.y-client.bottom,right=size.cx-offset.x-client.right;
        if(bottom<inset)inset=bottom;if(right<inset)inset=right;if(inset<1)inset=1;
        main_shadow_falloff=(float)(inset-3);if(main_shadow_falloff<1)main_shadow_falloff=1;if(main_shadow_falloff>9)main_shadow_falloff=9;
        main_shadow_offset=inset/4.0f;if(main_shadow_offset>2.5f)main_shadow_offset=2.5f;
        if(!render_frame(unit)||frame.capacity_width<size.cx||frame.capacity_height<size.cy||!main_prepare_curve(radius))return 0;
        cw=frame.width;ch=frame.height;ox+=MulDiv(UI_ORIGIN_X,unit,10000);oy+=MulDiv(UI_ORIGIN_Y,unit,10000);pixels=frame.bits;
        /* Move the current opaque artwork in-place, preserving the reserve and avoiding a second canvas. */
        for(y=ch-1;y>=0;y--)memmove(pixels+(y+offset.y+oy)*frame.capacity_width+offset.x+ox,pixels+y*frame.capacity_width,cw*4);
        for(y=0;y<size.cy;y++){
            DWORD *row=pixels+y*frame.capacity_width;
            if(y<offset.y||y>=offset.y+client.bottom)memset(row,0,size.cx*4);
            else{if(offset.x>0)memset(row,0,offset.x*4);if(size.cx>offset.x+client.right)memset(row+offset.x+client.right,0,(size.cx-offset.x-client.right)*4);}
        }
        /* Only deliberate letterboxing can remain outside the fixed-aspect artwork. */
        fill_background(frame.dc,offset.x,offset.y,client.right,oy);fill_background(frame.dc,offset.x,offset.y+oy+ch,client.right,client.bottom-oy-ch);
        fill_background(frame.dc,offset.x,offset.y+oy,ox,ch);fill_background(frame.dc,offset.x+ox+cw,offset.y+oy,client.right-ox-cw,ch);
        main_surface_caption(frame.dc,0,unit,offset.x+ox-MulDiv(UI_ORIGIN_X,unit,10000),offset.y+oy-MulDiv(UI_ORIGIN_Y,unit,10000));
        main_surface_caption(frame.dc,1,unit,offset.x+ox-MulDiv(UI_ORIGIN_X,unit,10000),offset.y+oy-MulDiv(UI_ORIGIN_Y,unit,10000));GdiFlush();
        for(y=0;y<client.bottom;y++)main_surface_opaque(pixels+(offset.y+y)*frame.capacity_width+offset.x,client.right);
        for(y=0;y<radius;y++)for(x=0;x<radius;x++){
            unsigned a=main_curve_coverage[y*radius+x];int corner;
            for(corner=0;corner<4;corner++){
                int px=corner&1?client.right-1-x:x,py=corner&2?client.bottom-1-y:y;DWORD *p=pixels+(offset.y+py)*frame.capacity_width+offset.x+px,v=*p;
                *p=(a<<24)|((((v>>16)&255)*a/255)<<16)|((((v>>8)&255)*a/255)<<8)|((v&255)*a/255);
                if(a<255)main_surface_shadow(p,px,py,client.right,client.bottom,radius);
            }
        }
        /* A small soft shadow fits the existing invisible side/bottom frame; its pixels are black premultiplied alpha. */
        for(y=0;y<size.cy;y++){
            DWORD *row=pixels+y*frame.capacity_width;
            if(y<offset.y||y>=offset.y+client.bottom){for(x=0;x<size.cx;x++)main_surface_shadow(row+x,x-offset.x,y-offset.y,client.right,client.bottom,radius);}
            else{for(x=0;x<offset.x;x++)main_surface_shadow(row+x,x-offset.x,y-offset.y,client.right,client.bottom,radius);for(x=offset.x+client.right;x<size.cx;x++)main_surface_shadow(row+x,x-offset.x,y-offset.y,client.right,client.bottom,radius);}
        }
        main_surface_width=size.cx;main_surface_height=size.cy;main_surface_x=offset.x;main_surface_y=offset.y;main_surface_radius=radius;main_surface_ready=1;main_surface_dirty=0;
        main_feedback_pending=0;
    }else if(main_feedback_pending){
        RECT damage=main_feedback_damage,device;int save,paint_x=offset.x+ox,paint_y=offset.y+oy;
        if(!main_set_clip_rect||!main_save_graphics||!main_restore_graphics){frame_dirty=1;return main_surface_present(hwnd);}
        device.left=paint_x+MulDiv(damage.left,unit,10000);device.top=paint_y+MulDiv(damage.top,unit,10000);
        device.right=paint_x+MulDiv(damage.right,unit,10000);device.bottom=paint_y+MulDiv(damage.bottom,unit,10000);
        if(device.left<offset.x)device.left=offset.x;if(device.top<offset.y)device.top=offset.y;
        if(device.right>offset.x+client.right)device.right=offset.x+client.right;if(device.bottom>offset.y+client.bottom)device.bottom=offset.y+client.bottom;
        /* Repaint the affected controls in-place. The original corner alpha,
           shadow and all other pixels remain in the same single DIB. */
        save=SaveDC(frame.dc);IntersectClipRect(frame.dc,device.left,device.top,device.right,device.bottom);
        if(main_feedback_knob_mask){
            int item;for(item=1;item<=2;item++)if(main_feedback_knob_mask&(1<<(item-1))){
                RECT clear;HBRUSH fill;int top=item==1?122:188;COLORREF color=main_feedback_down(item)?RGB(239,246,255):main_feedback_highlight(item)?RGB(247,250,255):RGB(255,255,255);
                clear.left=paint_x+MulDiv(412,unit,10000);clear.right=paint_x+MulDiv(466,unit,10000);clear.top=paint_y+MulDiv(top,unit,10000);clear.bottom=paint_y+MulDiv(top+32,unit,10000);
                fill=CreateSolidBrush(color);if(fill){FillRect(frame.dc,&clear,fill);DeleteObject(fill);}
            }GdiFlush();
            if(graphics_begin(frame.dc,unit/10000.0f,paint_x,paint_y))for(item=1;item<=2;item++)if(main_feedback_knob_mask&(1<<(item-1)))main_toggle(414,item==1?124:190,item);graphics_end();
        }else{main_feedback_paint_clip=damage;main_feedback_painting=1;paint_content(frame.dc,unit,paint_x,paint_y);main_feedback_painting=0;}
        GdiFlush();RestoreDC(frame.dc,save);
        if(graphics_error)return 0;
        pixels=frame.bits;for(y=device.top;y<device.bottom;y++)main_surface_opaque(pixels+y*frame.capacity_width+device.left,device.right-device.left);
        main_feedback_pending=0;main_feedback_knob_mask=0;main_feedback_partial_frames++;SetPropW(hwnd,L"ThreeFingerDrag-PartialFeedbackFrames",(HANDLE)(ULONG_PTR)main_feedback_partial_frames);
    }
    update=(void*)GetProcAddress(GetModuleHandleW(L"user32.dll"),"UpdateLayeredWindow");if(!update||!update(hwnd,0,0,&size,frame.dc,&source,0,&blend,ULW_ALPHA))return 0;
    SetPropW(hwnd,L"ThreeFingerDrag-AlphaSurface",(HANDLE)1);SetPropW(hwnd,L"ThreeFingerDrag-CornerRadius",(HANDLE)(ULONG_PTR)radius);return 1;
}
static int main_surface_snapshot(HWND hwnd){
    WCHAR path[MAX_PATH];HANDLE file;DWORD written;int header[9],y,ok=1;RECT client;
    if(!main_surface_present(hwnd)||!GetClientRect(hwnd,&client))return 0;
    header[0]=1;header[1]=main_surface_width;header[2]=main_surface_height;header[3]=main_surface_x;header[4]=main_surface_y;header[5]=client.right;header[6]=client.bottom;header[7]=main_surface_radius;header[8]=MAIN_CARD_RADIUS;
    _snwprintf(path,MAX_PATH,L"%s\\main-window-surface.bin",data_folder);file=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);if(file==INVALID_HANDLE_VALUE)return 0;
    if(!WriteFile(file,header,sizeof(header),&written,0)||written!=sizeof(header))ok=0;
    for(y=0;ok&&y<main_surface_height;y++)if(!WriteFile(file,(DWORD*)frame.bits+y*frame.capacity_width,main_surface_width*4,&written,0)||written!=main_surface_width*4)ok=0;
    CloseHandle(file);return ok;
}
#endif
