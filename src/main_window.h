#ifndef THREE_FINGER_MAIN_WINDOW_H
#define THREE_FINGER_MAIN_WINDOW_H
/* Native vector artwork belongs to the settings window only. */
#include "main_caption.h"
static int (WINAPI *main_add_line)(void *,float,float,float,float);
static int (WINAPI *main_add_bezier)(void *,float,float,float,float,float,float,float,float);
static int (WINAPI *main_start_figure)(void *);
static int (WINAPI *main_add_ellipse)(void *,float,float,float,float);
static int (WINAPI *main_line_brush)(const UI_RECTF *,DWORD,DWORD,int,int,void **);
static int (WINAPI *main_fill_rect)(void *,void *,float,float,float,float);
static BOOL (WINAPI *main_native_gradient)(HDC,TRIVERTEX *,ULONG,void *,ULONG,ULONG);
static int main_background_prefilled;
static int main_cards_prefilled;
static HBRUSH main_card_native_white,main_card_native_tip;
static int (WINAPI *main_set_clip_rect)(void *,float,float,float,float,int);
static int (WINAPI *main_reset_clip)(void *);
static int (WINAPI *main_save_graphics)(void *,UINT *);
static int (WINAPI *main_restore_graphics)(void *,UINT);
static void *main_card_paths[3],*main_card_border_pen,*main_tip_fill;
static const int main_card_boxes[3][4]={{20,104,460,132},{20,248,460,178},{20,508,460,88}};
static void *main_hand,*main_hand_outline,*main_brand_shape,*main_cursor,*main_spark,*main_gear,*main_power,*main_bulb,*main_arrow,*main_ai_card;
static void *main_white,*main_blue,*main_background,*main_ai_fill,*main_arrow_fill,*main_ai_hot_fill,*main_ai_down_fill,*main_outline,*main_cursor_outline,*main_icon_pen;
static HWND main_min_button,main_close_button;
static int main_was_minimized;
#define MAIN_CARD_RADIUS 14
/* User-approved client width: 1368 physical pixels at 200% Windows scaling.
   Artwork coordinates stay unchanged; every monitor renders this same DIP size. */
#define MAIN_DEFAULT_CLIENT_DIP_WIDTH 684
static const COLORREF main_border=RGB(227,234,244),main_soft=RGB(234,243,255),main_muted=RGB(110,122,145);
static void main_art_free(void){
    void **paths[]={&main_hand,&main_hand_outline,&main_brand_shape,&main_cursor,&main_spark,&main_gear,&main_power,&main_bulb,&main_arrow,&main_ai_card};
    void **brushes[]={&main_white,&main_blue,&main_background,&main_ai_fill,&main_arrow_fill,&main_ai_hot_fill,&main_ai_down_fill};
    void **pens[]={&main_outline,&main_cursor_outline,&main_icon_pen};int i;
    for(i=0;i<10;i++){if(*paths[i])GdipDeletePath(*paths[i]);*paths[i]=0;}
    for(i=0;i<7;i++){if(*brushes[i])GdipDeleteBrush(*brushes[i]);*brushes[i]=0;}
    for(i=0;i<3;i++){if(*pens[i])GdipDeletePen(*pens[i]);*pens[i]=0;}
    for(i=0;i<3;i++){if(main_card_paths[i])GdipDeletePath(main_card_paths[i]);main_card_paths[i]=0;}
    if(main_card_border_pen)GdipDeletePen(main_card_border_pen);main_card_border_pen=0;if(main_tip_fill)GdipDeleteBrush(main_tip_fill);main_tip_fill=0;
    if(main_card_native_white)DeleteObject(main_card_native_white);main_card_native_white=0;if(main_card_native_tip)DeleteObject(main_card_native_tip);main_card_native_tip=0;
    main_caption_theme_free();
}
static void main_segment(void *path,float x,float y,float xx,float yy){graphics_check(main_add_line(path,x,y,xx,yy));}
static void main_curve(void *path,float x,float y,float a,float b,float c,float d,float xx,float yy){graphics_check(main_add_bezier(path,x,y,a,b,c,d,xx,yy));}
#include "generated/tutorial_hand.h"
static void main_polygon(void *path,const float *points,int count){
    int i;for(i=0;i<count;i++)main_segment(path,points[2*i],points[2*i+1],points[2*((i+1)%count)],points[2*((i+1)%count)+1]);
    graphics_check(GdipClosePathFigure(path));
}
static void main_round_path(void *path,float x,float y,float w,float h,float r){
    float d=2*r;graphics_check(GdipAddPathArc(path,x,y,d,d,180,90));graphics_check(GdipAddPathArc(path,x+w-d,y,d,d,270,90));graphics_check(GdipAddPathArc(path,x+w-d,y+h-d,d,d,0,90));graphics_check(GdipAddPathArc(path,x,y+h-d,d,d,90,90));graphics_check(GdipClosePathFigure(path));
}
static int main_art_init(void){
    UI_RECTF area={UI_ORIGIN_X,UI_ORIGIN_Y,UI_WIDTH/(float)UI_LAYOUT_SCALE,UI_HEIGHT/(float)UI_LAYOUT_SCALE},action={256,438,224,60},arrow_area={0,0,50,24};
    const float cursor[]={0,0,0,32,8,25,15,39,22,35,15,22,27,22};
    const float arrow[]={0,8,35,8,35,1,50,12,35,23,35,16,0,16};
    const float cog[]={29,12,27,9,28,6,25,3,22,4,19,3,18,0,14,0,13,3,10,4,7,3,4,6,5,9,3,12,0,13,0,17,3,18,5,21,4,24,7,27,10,26,13,28,14,31,18,31,19,28,22,26,25,27,28,24,27,21,29,18,32,17,32,13};
    void **paths[]={&main_hand,&main_brand_shape,&main_cursor,&main_spark,&main_gear,&main_power,&main_bulb,&main_arrow,&main_ai_card};int i;int (WINAPI *clone_path)(void *,void **);int (WINAPI *set_line_join)(void *,int);
    main_native_gradient=(void*)GetProcAddress(GetModuleHandleW(L"gdi32.dll"),"GdiGradientFill");
    main_set_clip_rect=(void*)GetProcAddress(graphics_library,"GdipSetClipRect");main_reset_clip=(void*)GetProcAddress(graphics_library,"GdipResetClip");
    main_save_graphics=(void*)GetProcAddress(graphics_library,"GdipSaveGraphics");main_restore_graphics=(void*)GetProcAddress(graphics_library,"GdipRestoreGraphics");
    main_card_native_white=CreateSolidBrush(RGB(255,255,255));main_card_native_tip=CreateSolidBrush(RGB(232,243,255));
    main_add_line=(void*)GetProcAddress(graphics_library,"GdipAddPathLine");main_add_bezier=(void*)GetProcAddress(graphics_library,"GdipAddPathBezier");
    main_start_figure=(void*)GetProcAddress(graphics_library,"GdipStartPathFigure");main_add_ellipse=(void*)GetProcAddress(graphics_library,"GdipAddPathEllipse");
    main_line_brush=(void*)GetProcAddress(graphics_library,"GdipCreateLineBrushFromRect");main_fill_rect=(void*)GetProcAddress(graphics_library,"GdipFillRectangle");
    clone_path=(void*)GetProcAddress(graphics_library,"GdipClonePath");set_line_join=(void*)GetProcAddress(graphics_library,"GdipSetPenLineJoin");
    if(!main_add_line||!main_add_bezier||!main_start_figure||!main_add_ellipse||!main_line_brush||!main_fill_rect||!clone_path||!set_line_join)return 0;
    graphics_error=0;
    graphics_check(GdipCreateSolidFill(graphics_color(RGB(255,255,255)),&main_white));graphics_check(GdipCreateSolidFill(graphics_color(blue),&main_blue));
    if(!main_caption_theme_init())return 0;
    graphics_check(GdipCreatePen1(graphics_color(RGB(99,109,133)),1.8f,2,&main_outline));graphics_check(GdipCreatePen1(graphics_color(RGB(255,255,255)),4,2,&main_cursor_outline));
    if(main_outline)graphics_check(set_line_join(main_outline,2));
    graphics_check(GdipCreatePen1(graphics_color(blue),2.4f,2,&main_icon_pen));if(main_icon_pen){GdipSetPenStartCap(main_icon_pen,2);GdipSetPenEndCap(main_icon_pen,2);}
    graphics_check(main_line_brush(&area,graphics_color(RGB(250,252,255)),graphics_color(RGB(243,248,255)),1,0,&main_background));
    graphics_check(main_line_brush(&action,graphics_color(RGB(56,145,255)),graphics_color(RGB(20,126,255)),1,0,&main_ai_fill));
    graphics_check(main_line_brush(&action,graphics_color(RGB(44,137,247)),graphics_color(RGB(0,112,243)),1,0,&main_ai_hot_fill));
    graphics_check(main_line_brush(&action,graphics_color(RGB(13,115,236)),graphics_color(RGB(0,97,222)),1,0,&main_ai_down_fill));
    graphics_check(main_line_brush(&arrow_area,graphics_color(RGB(169,208,255)),graphics_color(blue),0,0,&main_arrow_fill));
    for(i=0;i<9;i++){graphics_check(GdipCreatePath(0,paths[i]));if(!*paths[i])return 0;}
    for(i=0;i<3;i++){graphics_check(GdipCreatePath(0,&main_card_paths[i]));if(!main_card_paths[i])return 0;main_round_path(main_card_paths[i],main_card_boxes[i][0],main_card_boxes[i][1],main_card_boxes[i][2],main_card_boxes[i][3],MAIN_CARD_RADIUS);}
    graphics_check(GdipCreatePen1(graphics_color(main_border),1,2,&main_card_border_pen));graphics_check(GdipCreateSolidFill(graphics_color(RGB(232,243,255)),&main_tip_fill));
    main_round_path(main_brand_shape,22,33,11,34,5.5f);main_round_path(main_brand_shape,42,25,11,42,5.5f);main_round_path(main_brand_shape,62,33,11,34,5.5f);
    /* Both tutorial tiles reuse the cached contour generated from the shared SVG. */
    main_hand_art_path(main_hand);graphics_check(clone_path(main_hand,&main_hand_outline));GdipClosePathFigure(main_hand);
    main_polygon(main_cursor,cursor,7);main_polygon(main_arrow,arrow,7);main_polygon(main_gear,cog,32);graphics_check(main_add_ellipse(main_gear,11,10.5f,10,10));
    main_curve(main_spark,12,0,14,8,16,10,24,12);main_curve(main_spark,24,12,16,14,14,16,12,24);main_curve(main_spark,12,24,10,16,8,14,0,12);main_curve(main_spark,0,12,8,10,10,8,12,0);GdipClosePathFigure(main_spark);
    graphics_check(GdipAddPathArc(main_power,0,4,24,24,-50,280));graphics_check(main_start_figure(main_power));main_segment(main_power,12,0,12,14);
    main_curve(main_bulb,6,20,6,15,1,14,1,8);main_curve(main_bulb,1,8,1,-3,19,-3,19,8);main_curve(main_bulb,19,8,19,14,14,15,14,20);main_segment(main_bulb,14,20,6,20);
    graphics_check(main_start_figure(main_bulb));main_segment(main_bulb,7,23,13,23);graphics_check(main_start_figure(main_bulb));main_segment(main_bulb,8,26,12,26);
    graphics_check(main_start_figure(main_bulb));main_segment(main_bulb,10,-7,10,-4);graphics_check(main_start_figure(main_bulb));main_segment(main_bulb,-6,2,-3,4);graphics_check(main_start_figure(main_bulb));main_segment(main_bulb,23,4,26,2);
    graphics_check(main_start_figure(main_bulb));main_segment(main_bulb,-7,12,-4,12);graphics_check(main_start_figure(main_bulb));main_segment(main_bulb,24,12,27,12);
    main_round_path(main_ai_card,256,438,224,60,11);
    return graphics_error==0;
}
static void main_transform_xy(int unit,int ox,int oy,float x,float y,float sx,float sy){
    void *matrix=0;float scale=unit/10000.0f;
    graphics_check(GdipCreateMatrix2(scale*sx,0,0,scale*sy,ox+x*scale,oy+y*scale,&matrix));
    if(matrix){graphics_check(GdipSetWorldTransform(graphics,matrix));GdipDeleteMatrix(matrix);}
}
static void main_transform_at(int unit,int ox,int oy,float x,float y,float factor){main_transform_xy(unit,ox,oy,x,y,factor,factor);}
static void main_psd_transform(const float *m,int unit,int ox,int oy){main_transform_xy(unit,ox,oy,m[2],m[3],m[0],m[1]);}
static void main_transform(int unit,int ox,int oy,float x,float y,float factor){main_transform_at(unit,ox,oy,x,y,factor);}
static void main_path(void *path,void *brush,void *pen,float x,float y,float factor,int unit,int ox,int oy){
    main_transform(unit,ox,oy,x,y,factor);if(brush)graphics_check(GdipFillPath(graphics,brush,path));if(pen)graphics_check(GdipDrawPath(graphics,pen,path));main_transform(unit,ox,oy,0,0,1);
}
static void main_toggle(int x,int y,int item){
    int p=main_slides[item-1].position;void *knob=0;COLORREF color=main_feedback_mix(RGB(216,218,223),blue,p);
    if(main_compositor_ready)return;
    if(main_feedback_down(item))color=main_feedback_mix(color,RGB(0,0,0),60);
    box(x,y,50,28,14,color,color);graphics_check(GdipCreateSolidFill(graphics_color(RGB(255,255,255)),&knob));
    if(knob){graphics_check(GdipFillEllipse(graphics,knob,x+3+22*p/1000.0f,(float)(y+3),22,22));GdipDeleteBrush(knob);}
}
static void main_brand(int x,int y,int size,int badge,int unit,int ox,int oy){
    if(badge)box(x,y,size,size,size*18/96,blue,blue);
    main_path(main_brand_shape,badge?main_white:main_blue,0,(float)x,(float)y,size/96.0f,unit,ox,oy);
}
static void main_gesture_art(int item,int x,int y,int unit,int ox,int oy){
    if(item<2){
        circle(x+60,y+27,24,RGB(224,238,255));circle(x+60,y+27,19,RGB(199,224,255));circle(x+60,y+27,14,RGB(171,207,253));
        if(item==1)main_path(main_arrow,main_arrow_fill,0,x+66,y+15,0.9f,unit,ox,oy);
        main_path(main_hand,main_white,0,x+27,y+3,0.66f,unit,ox,oy);main_path(main_hand_outline,0,main_outline,x+27,y+3,0.66f,unit,ox,oy);
    }else{
        box(x+31,y+18,64,44,6,RGB(255,255,255),RGB(159,198,252));box(x+31,y+18,64,12,6,blue,blue);
        main_transform(unit,ox,oy,0,0,1);graphics_check(main_fill_rect(graphics,main_blue,(float)(x+31),(float)(y+24),64,6));
        circle(x+38,y+24,1,RGB(255,255,255));circle(x+45,y+24,1,RGB(255,255,255));circle(x+52,y+24,1,RGB(255,255,255));
        main_path(main_cursor,main_blue,main_cursor_outline,x+63,y+38,0.65f,unit,ox,oy);
    }
}
static void main_chevron(int x,int y,COLORREF color){line(x,y,x+5,y+5,color,2);line(x+5,y+5,x,y+10,color,2);}
static void main_tip_check(int x,int y){circle(x,y,6,blue);line(x-2,y,x,y+2,RGB(255,255,255),1);line(x,y+2,x+3,y-2,RGB(255,255,255),1);}
static int main_hit_item(int x,int y){
    if(x>=20&&x<=480&&y>=104&&y<170)return 1;
    if(x>=20&&x<=480&&y>=170&&y<=236)return 2;
    if(y>=438&&y<=498){if(x>=20&&x<=246)return 3;if(x>=256&&x<=480)return 4;}
    return 0;
}
static void main_caption_draw(HWND parent,HWND child,HDC dc,int close,int state){
    RECT rect;POINT origin={0,0};int unit,ox,oy;
    if(GetWindowLongPtrW(parent,GWL_EXSTYLE)&WS_EX_LAYERED)return;
    if(main_caption_hot==child)state|=ODS_HOTLIGHT;
    if(main_caption_visual[close]!=(state&(ODS_HOTLIGHT|ODS_SELECTED|ODS_DISABLED))){main_caption_visual[close]=state&(ODS_HOTLIGHT|ODS_SELECTED|ODS_DISABLED);main_surface_touch(parent);}
    GetClientRect(parent,&rect);canvas(rect.right,rect.bottom,&unit,&ox,&oy);MapWindowPoints(child,parent,&origin,1);
    ox-=origin.x;oy-=origin.y;
    if(graphics_begin(dc,unit/10000.0f,ox,oy)){
        graphics_check(main_fill_rect(graphics,main_background,UI_ORIGIN_X,UI_ORIGIN_Y,UI_WIDTH/(float)UI_LAYOUT_SCALE,UI_HEIGHT/(float)UI_LAYOUT_SCALE));
    }graphics_end();
    if(main_caption_hot==child)state|=ODS_HOTLIGHT;
    main_caption_glyph(dc,close,(UINT)state,unit,ox,oy);
}
static void main_place_caption(HWND hwnd){
    RECT rect;int unit,ox,oy,x,y,width,height,right;if(!GetClientRect(hwnd,&rect))return;canvas(rect.right,rect.bottom,&unit,&ox,&oy);
    y=oy+MulDiv(MAIN_CAPTION_Y,unit,10000);height=MulDiv(MAIN_CAPTION_HEIGHT,unit,10000);
    if(main_min_button)SetWindowPos(main_min_button,0,ox+MulDiv(MAIN_MIN_X,unit,10000),y,MulDiv(MAIN_CAPTION_WIDTH,unit,10000),height,SWP_NOZORDER|SWP_NOACTIVATE);
    right=ox+MulDiv(UI_ORIGIN_X*UI_LAYOUT_SCALE+UI_WIDTH,unit,10000*UI_LAYOUT_SCALE);
    x=ox+MulDiv(MAIN_CLOSE_X,unit,10000);width=MulDiv(MAIN_CAPTION_WIDTH,unit,10000);if(x+width>right)width=right-x;
    if(main_close_button)SetWindowPos(main_close_button,0,x,y,width,height,SWP_NOZORDER|SWP_NOACTIVATE);
}
static int main_create_caption(HWND hwnd){
    HINSTANCE instance=(HINSTANCE)GetWindowLongPtrW(hwnd,GWLP_HINSTANCE);
    main_min_button=CreateWindowExW(WS_EX_TRANSPARENT,L"BUTTON",app_text(APP_TEXT_MINIMIZE),WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,0,0,1,1,hwnd,(HMENU)6101,instance,0);
    main_close_button=CreateWindowExW(WS_EX_TRANSPARENT,L"BUTTON",app_text(APP_TEXT_CLOSE),WS_CHILD|WS_VISIBLE|BS_OWNERDRAW,0,0,1,1,hwnd,(HMENU)6102,instance,0);
    if(!main_min_button||!main_close_button)return 0;
    main_caption_base=(WNDPROC)SetWindowLongPtrW(main_min_button,GWLP_WNDPROC,(LONG_PTR)main_caption_button_proc);
    if(!main_caption_base||!SetWindowLongPtrW(main_close_button,GWLP_WNDPROC,(LONG_PTR)main_caption_button_proc))return 0;
    main_caption_theme_refresh(hwnd);main_place_caption(hwnd);return 1;
}
static LRESULT main_caption_hit(HWND hwnd,LPARAM position){
    LRESULT hit=DefWindowProcW(hwnd,WM_NCHITTEST,0,position);POINT point,origin={0,0};RECT rect;int unit,ox,oy,x,y,border=1;
    point.x=(short)LOWORD(position);point.y=(short)HIWORD(position);
    /* The top resize strip is painted client content, not a second system frame. */
    if(!IsIconic(hwnd)&&GetWindowRect(hwnd,&rect)&&ClientToScreen(hwnd,&origin)){
        border=origin.x-rect.left;if(border<1)border=1;
        if(GetWindowLongPtrW(hwnd,GWL_EXSTYLE)&WS_EX_LAYERED){
            RECT client;POINT local={point.x-origin.x,point.y-origin.y};GetClientRect(hwnd,&client);canvas(client.right,client.bottom,&unit,&ox,&oy);
            if(local.x>=0&&local.y>=0&&local.x<client.right&&local.y<client.bottom){
                int radius=MulDiv(MAIN_CARD_RADIUS,unit,10000),corner=radius/2;
                int left=local.x<corner,right=local.x>=client.right-corner,top=local.y<corner,bottom=local.y>=client.bottom-corner;
                if(top&&left)return HTTOPLEFT;if(top&&right)return HTTOPRIGHT;if(bottom&&left)return HTBOTTOMLEFT;if(bottom&&right)return HTBOTTOMRIGHT;
                if(local.x<border)return HTLEFT;if(local.x>=client.right-border)return HTRIGHT;if(local.y<border)return HTTOP;if(local.y>=client.bottom-border)return HTBOTTOM;
            }
        }
        if(point.x>=rect.left&&point.x<rect.right&&point.y>=rect.top&&point.y<rect.top+border){
            if(point.x<rect.left+border)return HTTOPLEFT;
            if(point.x>=rect.right-border)return HTTOPRIGHT;
            return HTTOP;
        }
        if(point.x>=rect.left&&point.x<rect.right&&point.y>=rect.top&&point.y<rect.bottom){
            if(point.y>=rect.bottom-border){if(point.x<rect.left+border)return HTBOTTOMLEFT;if(point.x>=rect.right-border)return HTBOTTOMRIGHT;return HTBOTTOM;}
            if(point.x<rect.left+border)return HTLEFT;if(point.x>=rect.right-border)return HTRIGHT;
        }
    }
    if(hit!=HTCLIENT)return hit;ScreenToClient(hwnd,&point);GetClientRect(hwnd,&rect);canvas(rect.right,rect.bottom,&unit,&ox,&oy);
    x=MulDiv(point.x-ox,10000,unit);y=MulDiv(point.y-oy,10000,unit);
    if(x>=MAIN_MIN_X&&y>=MAIN_CAPTION_Y&&y<MAIN_CAPTION_Y+MAIN_CAPTION_HEIGHT)return HTCLIENT;
    return x>=UI_ORIGIN_X&&x<UI_ORIGIN_X+UI_WIDTH/UI_LAYOUT_SCALE&&y>=UI_ORIGIN_Y&&y<100?HTCAPTION:HTCLIENT;
}
static LRESULT main_client_bounds(HWND hwnd,WPARAM valid,LPARAM position){
    RECT *client=valid?&((NCCALCSIZE_PARAMS*)position)->rgrc[0]:(RECT*)position;
    UINT dpi=96;int margin;UINT (WINAPI *window_dpi)(HWND)=(void*)GetProcAddress(GetModuleHandleW(L"user32.dll"),"GetDpiForWindow");
    if(window_dpi)dpi=window_dpi(hwnd);if(!dpi)dpi=96;margin=MulDiv(6,dpi,96);
    /* Transparent side/bottom gutters belong to our alpha surface, not a second native frame. */
    client->left+=margin;client->right-=margin;client->bottom-=margin;
    return valid?WVR_REDRAW:0;
}

static UINT main_window_dpi(HWND hwnd){
    UINT dpi=96;UINT (WINAPI *window_dpi)(HWND)=(void*)GetProcAddress(GetModuleHandleW(L"user32.dll"),"GetDpiForWindow");
    if(window_dpi)dpi=window_dpi(hwnd);return dpi?dpi:96;
}
/* Retain the original grab point for the whole native move loop. Repeated DPI
   changes must never turn an already-rounded offset into the next baseline. */
static int main_move_active;
static UINT main_move_edge;
static RECT main_move_origin;
static POINT main_move_grab;
static void main_move_begin(HWND hwnd){
    main_move_active=GetWindowRect(hwnd,&main_move_origin)&&GetCursorPos(&main_move_grab);
    main_move_edge=0;
}
static void main_move_anchor(RECT *rect,const RECT *origin,POINT grab,POINT cursor){
    int width=rect->right-rect->left,height=rect->bottom-rect->top;
    int old_width=origin->right-origin->left,old_height=origin->bottom-origin->top;
    if(old_width<1||old_height<1)return;
    rect->left=cursor.x-MulDiv(grab.x-origin->left,width,old_width);
    rect->top=cursor.y-MulDiv(grab.y-origin->top,height,old_height);
    rect->right=rect->left+width;rect->bottom=rect->top+height;
}
static void main_moving(RECT *rect){
    POINT cursor;if(main_move_active&&!main_move_edge&&GetCursorPos(&cursor))main_move_anchor(rect,&main_move_origin,main_move_grab,cursor);
}
static void main_fit_monitor_rect(const RECT *work,UINT dpi,RECT *rect,int centered){
    int gutter=MulDiv(6,dpi,96),padding=MulDiv(16,dpi,96),ex=2*gutter,ey=gutter;
    int workw=work->right-work->left,workh=work->bottom-work->top,w=rect->right-rect->left-ex,h;
    int maxw=workw-ex-2*padding,maxh=workh-ey-2*padding,left,top;
    if(maxw<1){padding=0;maxw=workw-ex;}if(maxh<1){padding=0;maxh=workh-ey;}
    if(maxw<1)maxw=1;if(maxh<1)maxh=1;if(w<1)w=1;if(w>maxw)w=maxw;
    h=MulDiv(w,UI_HEIGHT,UI_WIDTH);if(h>maxh){h=maxh;w=MulDiv(h,UI_WIDTH,UI_HEIGHT);}if(w<1)w=1;if(h<1)h=1;
    w+=ex;h+=ey;left=centered?work->left+(workw-w)/2:rect->left;top=centered?work->top+(workh-h)/2:rect->top;
    if(left<work->left+padding)left=work->left+padding;if(left+w>work->right-padding)left=work->right-padding-w;
    if(top<work->top+padding)top=work->top+padding;if(top+h>work->bottom-padding)top=work->bottom-padding-h;
    rect->left=left;rect->top=top;rect->right=left+w;rect->bottom=top+h;
}
static void main_default_monitor_rect(HWND hwnd,HMONITOR target,RECT *rect){
    MONITORINFO monitor;UINT dpi=main_window_dpi(hwnd);int w=MulDiv(MAIN_DEFAULT_CLIENT_DIP_WIDTH,dpi,96),gutter=MulDiv(6,dpi,96);
    memset(&monitor,0,sizeof(monitor));monitor.cbSize=sizeof(monitor);GetMonitorInfoW(target,&monitor);
    rect->left=monitor.rcWork.left;rect->top=monitor.rcWork.top;rect->right=rect->left+w+2*gutter;rect->bottom=rect->top+MulDiv(w,UI_HEIGHT,UI_WIDTH)+gutter;
    main_fit_monitor_rect(&monitor.rcWork,dpi,rect,1);
}
static void main_refit_window(HWND hwnd){
    RECT rect,original;MONITORINFO monitor;if(IsIconic(hwnd)||!GetWindowRect(hwnd,&rect))return;original=rect;
    memset(&monitor,0,sizeof(monitor));monitor.cbSize=sizeof(monitor);if(!GetMonitorInfoW(MonitorFromWindow(hwnd,MONITOR_DEFAULTTONEAREST),&monitor))return;
    main_fit_monitor_rect(&monitor.rcWork,main_window_dpi(hwnd),&rect,0);
    if(memcmp(&original,&rect,sizeof(rect)))SetWindowPos(hwnd,0,rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top,SWP_NOZORDER|SWP_NOACTIVATE);
}
static void main_dpi_changed(HWND hwnd,UINT dpi,const RECT *suggested){
    RECT rect=*suggested;MONITORINFO monitor;memset(&monitor,0,sizeof(monitor));monitor.cbSize=sizeof(monitor);
    if(GetMonitorInfoW(MonitorFromRect(&rect,MONITOR_DEFAULTTONEAREST),&monitor))main_fit_monitor_rect(&monitor.rcWork,dpi,&rect,0);
    /* Work-area position clamping during a drag pulls the window away from the
       cursor, then the native move loop pulls it back. Keep the cursor anchor
       instead; the size still fits the destination monitor. */
    if(main_move_active&&!main_move_edge)main_moving(&rect);
    SetWindowPos(hwnd,0,rect.left,rect.top,rect.right-rect.left,rect.bottom-rect.top,SWP_FRAMECHANGED|SWP_NOZORDER|SWP_NOACTIVATE);
}
static void main_window_limits(HWND hwnd,int *extra_x,int *extra_y,int *min_width,int *min_height,MONITORINFO *monitor){
    RECT outer,client;UINT dpi=96;int unit,available,padding;
    UINT (WINAPI *window_dpi)(HWND)=(void*)GetProcAddress(GetModuleHandleW(L"user32.dll"),"GetDpiForWindow");
    if(window_dpi)dpi=window_dpi(hwnd);if(!dpi)dpi=96;
    *extra_x=*extra_y=0;if(GetWindowRect(hwnd,&outer)&&GetClientRect(hwnd,&client)){*extra_x=outer.right-outer.left-client.right;*extra_y=outer.bottom-outer.top-client.bottom;}
    memset(monitor,0,sizeof(*monitor));monitor->cbSize=sizeof(*monitor);GetMonitorInfoW(MonitorFromWindow(hwnd,MONITOR_DEFAULTTONEAREST),monitor);
    unit=MulDiv((int)dpi,10000,96);padding=2*MulDiv(16,dpi,96);
    if(monitor->rcWork.right>monitor->rcWork.left){
        available=MulDiv(monitor->rcWork.right-monitor->rcWork.left-*extra_x-padding,10000*UI_LAYOUT_SCALE,UI_WIDTH);if(available<unit)unit=available;
        available=MulDiv(monitor->rcWork.bottom-monitor->rcWork.top-*extra_y-padding,10000*UI_LAYOUT_SCALE,UI_HEIGHT);if(available<unit)unit=available;
    }
    if(unit<1)unit=1;*min_width=MulDiv(UI_WIDTH,unit,10000*UI_LAYOUT_SCALE);*min_height=MulDiv(UI_HEIGHT,unit,10000*UI_LAYOUT_SCALE);
}
static void main_minmax(HWND hwnd,MINMAXINFO *info){
    MONITORINFO monitor;int ex,ey,minw,minh,maxw,maxh;
    main_window_limits(hwnd,&ex,&ey,&minw,&minh,&monitor);info->ptMinTrackSize.x=minw+ex;info->ptMinTrackSize.y=minh+ey;
    if(monitor.rcWork.right>monitor.rcWork.left){
        maxw=monitor.rcWork.right-monitor.rcWork.left-ex;maxh=MulDiv(maxw,UI_HEIGHT,UI_WIDTH);
        if(maxh+ey>monitor.rcWork.bottom-monitor.rcWork.top){maxh=monitor.rcWork.bottom-monitor.rcWork.top-ey;maxw=MulDiv(maxh,UI_WIDTH,UI_HEIGHT);}
        info->ptMaxSize.x=maxw+ex;info->ptMaxSize.y=maxh+ey;
        info->ptMaxPosition.x=monitor.rcWork.left-monitor.rcMonitor.left+(monitor.rcWork.right-monitor.rcWork.left-info->ptMaxSize.x)/2;
        info->ptMaxPosition.y=monitor.rcWork.top-monitor.rcMonitor.top+(monitor.rcWork.bottom-monitor.rcWork.top-info->ptMaxSize.y)/2;
    }
    SetPropW(hwnd,L"ThreeFingerDrag-MinClientWidth",(HANDLE)(ULONG_PTR)minw);SetPropW(hwnd,L"ThreeFingerDrag-MinClientHeight",(HANDLE)(ULONG_PTR)minh);
}
static void main_constrain_size(HWND hwnd){
    static int adjusting;RECT client,outer;MONITORINFO monitor;LONGLONG difference;int ex,ey,minw,minh,w,h;
    if(adjusting||!GetClientRect(hwnd,&client)||client.right<1||client.bottom<1)return;
    difference=(LONGLONG)client.right*UI_HEIGHT-(LONGLONG)client.bottom*UI_WIDTH;if(difference<0)difference=-difference;
    if(difference<=UI_HEIGHT)return;
    if(!GetWindowRect(hwnd,&outer))return;main_window_limits(hwnd,&ex,&ey,&minw,&minh,&monitor);
    h=client.bottom;if(h<minh)h=minh;w=MulDiv(h,UI_WIDTH,UI_HEIGHT);if(w<minw){w=minw;h=MulDiv(w,UI_HEIGHT,UI_WIDTH);}
    adjusting=1;SetWindowPos(hwnd,0,0,0,w+ex,h+ey,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);adjusting=0;
}
static void main_sizing(HWND hwnd,UINT edge,RECT *rect){
    MONITORINFO monitor;int ex,ey,minw,minh,w,h,center;
    main_move_edge=edge;main_window_limits(hwnd,&ex,&ey,&minw,&minh,&monitor);
    w=rect->right-rect->left-ex;h=rect->bottom-rect->top-ey;
    if(edge==WMSZ_TOP||edge==WMSZ_BOTTOM){if(h<minh)h=minh;w=MulDiv(h,UI_WIDTH,UI_HEIGHT);}
    else {
        if(edge!=WMSZ_LEFT&&edge!=WMSZ_RIGHT){
            /* Project both cursor dimensions onto the aspect-ratio line. The
               old per-message choice of a dominant axis oscillated because
               each corrected rectangle changed the next comparison. */
            LONGLONG denominator=(LONGLONG)UI_WIDTH*UI_WIDTH+(LONGLONG)UI_HEIGHT*UI_HEIGHT;
            LONGLONG numerator=(LONGLONG)w*UI_WIDTH*UI_WIDTH+(LONGLONG)h*UI_WIDTH*UI_HEIGHT;
            w=(int)((numerator+denominator/2)/denominator);
        }
        if(w<minw)w=minw;h=MulDiv(w,UI_HEIGHT,UI_WIDTH);
    }
    w+=ex;h+=ey;
    if(edge==WMSZ_LEFT||edge==WMSZ_TOPLEFT||edge==WMSZ_BOTTOMLEFT)rect->left=rect->right-w;
    else if(edge==WMSZ_TOP||edge==WMSZ_BOTTOM){center=(rect->left+rect->right)/2;rect->left=center-w/2;rect->right=rect->left+w;}
    else rect->right=rect->left+w;
    if(edge==WMSZ_TOP||edge==WMSZ_TOPLEFT||edge==WMSZ_TOPRIGHT)rect->top=rect->bottom-h;
    else if(edge==WMSZ_LEFT||edge==WMSZ_RIGHT){center=(rect->top+rect->bottom)/2;rect->top=center-h/2;rect->bottom=rect->top+h;}
    else rect->bottom=rect->top+h;
}
static int main_paint_background(HDC dc,int unit,int ox,int oy){
    TRIVERTEX vertices[2];GRADIENT_RECT mesh={0,1};int x=ox+MulDiv(UI_ORIGIN_X,unit,10000),y=oy+MulDiv(UI_ORIGIN_Y,unit,10000);
    if(!main_native_gradient)return 0;
    memset(vertices,0,sizeof(vertices));vertices[0].x=x;vertices[0].y=y;vertices[0].Red=250<<8;vertices[0].Green=252<<8;vertices[0].Blue=255<<8;
    vertices[1].x=x+MulDiv(UI_WIDTH,unit,10000*UI_LAYOUT_SCALE);vertices[1].y=y+MulDiv(UI_HEIGHT,unit,10000*UI_LAYOUT_SCALE);vertices[1].Red=243<<8;vertices[1].Green=248<<8;vertices[1].Blue=255<<8;
    /* Only the rectangular background uses fast native interpolation. Curves,
       artwork and glyphs retain the original GDI+ antialiasing and paths. */
    if(!main_native_gradient(dc,vertices,2,&mesh,1,GRADIENT_FILL_RECT_V))return 0;GdiFlush();return 1;
}
static int main_paint_card_interiors(HDC dc,int unit,int ox,int oy){
    int i;if(!main_set_clip_rect||!main_reset_clip||!main_card_native_white||!main_card_native_tip)return 0;
    for(i=0;i<3;i++){
        const int *b=main_card_boxes[i];RECT rect;
        /* Overlap the vector clip boundary by one device pixel, entirely inside
           the rounded card, so fractional scales cannot expose a seam. */
        rect.left=ox+MulDiv(b[0]+MAIN_CARD_RADIUS+2,unit,10000)-1;rect.top=oy+MulDiv(b[1]+2,unit,10000)-1;
        rect.right=ox+MulDiv(b[0]+b[2]-MAIN_CARD_RADIUS-2,unit,10000)+1;rect.bottom=oy+MulDiv(b[1]+b[3]-2,unit,10000)+1;
        if(!FillRect(dc,&rect,i==2?main_card_native_tip:main_card_native_white))return 0;
    }
    GdiFlush();return 1;
}
static void main_card(int index){
    void *brush=index==2?main_tip_fill:main_white;
    const int *b=main_card_boxes[index];UINT state=0;int saved=0;
    UI_RECTF interior={(float)(b[0]+MAIN_CARD_RADIUS+2),(float)(b[1]+2),(float)(b[2]-2*MAIN_CARD_RADIUS-4),(float)(b[3]-4)};
    /* The native interior avoids rasterizing large solid paths; the original
       vector path still owns all edges, corners and borders. */
    if(main_cards_prefilled){if(main_save_graphics&&main_restore_graphics){graphics_check(main_save_graphics(graphics,&state));saved=1;}graphics_check(main_set_clip_rect(graphics,interior.x,interior.y,interior.width,interior.height,4));}
    graphics_check(GdipFillPath(graphics,brush,main_card_paths[index]));
    if(main_cards_prefilled){if(saved)graphics_check(main_restore_graphics(graphics,state));else graphics_check(main_reset_clip(graphics));}
    if(index!=2)graphics_check(GdipDrawPath(graphics,main_card_border_pen,main_card_paths[index]));
}
static void main_paint(HDC dc,int unit,int ox,int oy){
    int i;const int tiles[3]={38,191,342};const WCHAR *captions[3]={app_text(APP_TEXT_STEP_TOUCH),app_text(APP_TEXT_STEP_MOVE),app_text(APP_TEXT_STEP_RELEASE)};
    if(!main_background_prefilled)graphics_check(main_fill_rect(graphics,main_background,UI_ORIGIN_X,UI_ORIGIN_Y,UI_WIDTH/(float)UI_LAYOUT_SCALE,UI_HEIGHT/(float)UI_LAYOUT_SCALE));
    main_brand(UI_BRAND_X,UI_BRAND_Y,UI_BRAND_SIZE,1,unit,ox,oy);
    main_psd_transform(main_psd_status_pill,unit,ox,oy);box(408,43,73,26,13,status_fill,status_fill);
    main_psd_transform(main_psd_status_dot,unit,ox,oy);circle(424,56,4,status_color);main_transform(unit,ox,oy,0,0,1);
    main_card(0);
    for(i=1;i<=2;i++)if(main_feedback_highlight(i)||main_feedback_down(i)){COLORREF tint=main_feedback_down(i)?RGB(239,246,255):RGB(247,250,255);box(22,i==1?106:172,456,62,10,tint,tint);}
    line(38,170,464,170,RGB(233,238,246),1);
    box(40,117,40,40,10,main_soft,main_soft);main_brand(42,119,36,0,unit,ox,oy);
    box(40,182,40,40,10,main_soft,main_soft);main_path(main_power,0,main_icon_pen,51,191,0.75f,unit,ox,oy);
    main_toggle(414,124,1);main_toggle(414,190,2);
    main_card(1);
    for(i=0;i<3;i++){int x=tiles[i];box(x,312,122,75,10,RGB(241,246,252),RGB(241,246,252));main_gesture_art(i,x,312,unit,ox,oy);circle(x+32,403,10,blue);}
    main_chevron(175,347,RGB(126,141,163));main_chevron(326,347,RGB(126,141,163));
    box(20,438,226,60,11,main_feedback_down(3)?RGB(206,230,255):main_feedback_highlight(3)?RGB(218,237,255):RGB(230,242,255),main_feedback_highlight(3)||main_feedback_down(3)?RGB(162,202,252):RGB(204,225,254));circle(49,468,17,RGB(218,235,255));main_path(main_gear,main_blue,0,37,456,0.75f,unit,ox,oy);main_chevron(226,464,blue);
    graphics_check(GdipFillPath(graphics,main_feedback_down(4)?main_ai_down_fill:main_feedback_highlight(4)?main_ai_hot_fill:main_ai_fill,main_ai_card));main_path(main_spark,main_white,0,273,457,0.9f,unit,ox,oy);main_path(main_spark,main_white,0,290,473,0.35f,unit,ox,oy);main_chevron(464,464,RGB(255,255,255));
    main_card(2);main_path(main_bulb,0,main_icon_pen,36,529,0.75f,unit,ox,oy);
    box(420,520,46,23,12,RGB(215,233,255),RGB(215,233,255));main_tip_check(70,554);main_tip_check(70,575);
    text_sequence=0;
    main_psd_transform(main_psd_title,unit,ox,oy);
    text(dc,6,103,27,300,42,app_text(APP_TEXT_APP_NAME),ink,DT_LEFT);
    main_psd_transform(main_psd_subtitle,unit,ox,oy);
    text(dc,0,103,68,372,24,app_text(APP_TEXT_TAGLINE),main_muted,DT_LEFT);
    main_psd_transform(main_psd_status_text,unit,ox,oy);
    text(dc,0,436,43,42,26,status_label,status_color,DT_CENTER);
    main_transform(unit,ox,oy,0,0,1);
    text(dc,5,99,110,290,28,app_text(APP_TEXT_DRAG),ink,DT_LEFT);text(dc,0,99,136,294,22,app_text(APP_TEXT_DRAG_DESCRIPTION),main_muted,DT_LEFT);
    text(dc,5,99,176,290,28,app_text(APP_TEXT_STARTUP),ink,DT_LEFT);text(dc,0,99,202,294,22,app_text(APP_TEXT_STARTUP_DESCRIPTION),main_muted,DT_LEFT);
    text(dc,5,42,253,420,28,app_text(APP_TEXT_GUIDE_TITLE),ink,DT_LEFT);
    text(dc,0,42,279,422,24,app_text(APP_TEXT_GUIDE_DESCRIPTION),main_muted,DT_LEFT);
    for(i=0;i<3;i++){const WCHAR *number=i==0?L"1":i==1?L"2":L"3";text(dc,0,tiles[i]+22,393,20,20,number,RGB(255,255,255),DT_CENTER);text(dc,0,tiles[i]+52,392,83,22,captions[i],ink,DT_LEFT);}
    text(dc,7,80,447,142,26,main_action_feedback[0].result==1?app_text(APP_TEXT_SETTINGS_OPENED):main_action_feedback[0].result==2?app_text(APP_TEXT_SETTINGS_FAILED):app_text(APP_TEXT_SETTINGS),blue,DT_LEFT);text(dc,8,80,473,146,17,main_action_feedback[0].result==1?app_text(APP_TEXT_SETTINGS_OPENED_HINT):main_action_feedback[0].result==2?app_text(APP_TEXT_RETRY_HINT):app_text(APP_TEXT_SETTINGS_DESCRIPTION),RGB(110,133,164),DT_LEFT);
    text(dc,7,316,447,134,26,main_action_feedback[1].result==1?app_text(APP_TEXT_AI_COPIED):main_action_feedback[1].result==2?app_text(APP_TEXT_AI_FAILED):app_text(APP_TEXT_AI),RGB(255,255,255),DT_LEFT);text(dc,8,316,473,141,17,main_action_feedback[1].result==1?app_text(APP_TEXT_AI_COPIED_HINT):main_action_feedback[1].result==2?app_text(APP_TEXT_RETRY_HINT):app_text(APP_TEXT_AI_DESCRIPTION),RGB(223,239,255),DT_LEFT);
    text(dc,7,66,517,264,28,app_text(APP_TEXT_FIRST_TIPS),RGB(20,76,147),DT_LEFT);text(dc,8,420,520,46,23,app_text(APP_TEXT_TIPS),RGB(61,112,176),DT_CENTER);
    text(dc,8,82,544,384,22,setup_steps_visible()||tp_state.ready?app_text(APP_TEXT_TIP_SETUP):footer,RGB(92,119,155),DT_LEFT);
    text(dc,8,82,565,384,22,setup_notice_until?footer:app_text(APP_TEXT_TIP_AI),RGB(92,119,155),DT_LEFT);
    if(window){SetPropW(window,L"ThreeFingerDrag-LogicalWidth",(HANDLE)(ULONG_PTR)UI_WIDTH);SetPropW(window,L"ThreeFingerDrag-LogicalHeight",(HANDLE)(ULONG_PTR)UI_HEIGHT);SetPropW(window,L"ThreeFingerDrag-GuideVisible",(HANDLE)(ULONG_PTR)2);}
}
#endif
