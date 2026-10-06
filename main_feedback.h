#ifndef THREE_FINGER_MAIN_FEEDBACK_H
#define THREE_FINGER_MAIN_FEEDBACK_H
/* Main-window feedback only. No host/tray changes and no idle animation timer. */
#define MAIN_FEEDBACK_TIMER 5
#define MAIN_FEEDBACK_DURATION 120
static int main_hit_item(int,int);
static int main_hot_item,main_pressed_item,main_press_source,main_pressed_key,main_mouse_tracking;
static int main_feedback_animate=1,main_feedback_timer,main_feedback_pending,main_feedback_painting;
static int main_feedback_knob_mask;
static int main_compositor_ready,main_compositor_disabled;
static int main_compositor_prepare(HWND,int,int,int);
static int main_compositor_switches(void);
static int main_compositor_health(void);
static void main_compositor_free(void);
static RECT main_feedback_damage,main_feedback_paint_clip;
static UINT main_feedback_ticks,main_feedback_partial_frames;
static struct {int from,position,target,active;LONGLONG started;} main_slides[2];
static struct {int result;DWORD until;} main_action_feedback[2];
static HANDLE main_feedback_clock;
static LONGLONG main_feedback_frequency,main_feedback_step,main_feedback_due;
static int main_feedback_clock_unavailable;
static LONGLONG main_feedback_now(void){LARGE_INTEGER time;QueryPerformanceCounter(&time);return time.QuadPart;}
static void main_feedback_clock_stop(void){if(main_feedback_clock){CancelWaitableTimer(main_feedback_clock);CloseHandle(main_feedback_clock);main_feedback_clock=0;}}
static int main_feedback_clock_arm(void){
    LARGE_INTEGER relative;LONGLONG now=main_feedback_now();
    while(main_feedback_due<=now)main_feedback_due+=main_feedback_step;
    relative.QuadPart=-((main_feedback_due-now)*10000000/main_feedback_frequency);if(!relative.QuadPart)relative.QuadPart=-1;
    return SetWaitableTimer(main_feedback_clock,&relative,0,0,0,FALSE)!=0;
}
static int main_feedback_clock_start(void){
    HANDLE (WINAPI *create)(LPSECURITY_ATTRIBUTES,LPCWSTR,DWORD,DWORD);MONITORINFOEXW monitor;DEVMODEW mode;int hz=60;
    if(main_feedback_clock)return 1;if(main_feedback_clock_unavailable)return 0;
    create=(void*)GetProcAddress(GetModuleHandleW(L"kernel32.dll"),"CreateWaitableTimerExW");if(create)main_feedback_clock=create(0,0,2,0x001F0003);
    if(!main_feedback_clock){main_feedback_clock_unavailable=1;return 0;}
    memset(&monitor,0,sizeof(monitor));((MONITORINFO*)&monitor)->cbSize=sizeof(monitor);memset(&mode,0,sizeof(mode));mode.dmSize=sizeof(mode);
    if(GetMonitorInfoW(MonitorFromWindow(window,MONITOR_DEFAULTTONEAREST),(MONITORINFO*)&monitor)&&EnumDisplaySettingsW(monitor.szDevice,ENUM_CURRENT_SETTINGS,&mode)&&mode.dmDisplayFrequency>=30&&mode.dmDisplayFrequency<=1000)hz=(int)mode.dmDisplayFrequency;
    main_feedback_step=main_feedback_frequency/hz;if(main_feedback_step<1)main_feedback_step=1;main_feedback_due=main_feedback_now()+main_feedback_step;
    if(main_feedback_clock_arm())return 1;main_feedback_clock_stop();main_feedback_clock_unavailable=1;return 0;
}
static void main_feedback_properties(void){
    if(!window)return;
    SetPropW(window,L"ThreeFingerDrag-HotItem",(HANDLE)(ULONG_PTR)main_hot_item);
    SetPropW(window,L"ThreeFingerDrag-PressedItem",(HANDLE)(ULONG_PTR)main_pressed_item);
    SetPropW(window,L"ThreeFingerDrag-FeedbackTimer",(HANDLE)(ULONG_PTR)(main_feedback_timer||main_feedback_clock));
    SetPropW(window,L"ThreeFingerDrag-FeedbackTicks",(HANDLE)(ULONG_PTR)main_feedback_ticks);
    SetPropW(window,L"ThreeFingerDrag-SwitchPosition",(HANDLE)(ULONG_PTR)(main_slides[0].position+1));
    SetPropW(window,L"ThreeFingerDrag-StartupPosition",(HANDLE)(ULONG_PTR)(main_slides[1].position+1));
    SetPropW(window,L"ThreeFingerDrag-SettingsFeedback",(HANDLE)(ULONG_PTR)main_action_feedback[0].result);
    SetPropW(window,L"ThreeFingerDrag-CopyFeedback",(HANDLE)(ULONG_PTR)main_action_feedback[1].result);
}
static void main_feedback_region(int item,RECT *rect){
    rect->left=item==4?254:18;rect->right=item==3?248:482;
    rect->top=item==1?102:item==2?168:436;rect->bottom=item==1?172:item==2?238:500;
}
static void main_feedback_redraw(int item){
    RECT rect;if(!window||item<1||item>4)return;main_feedback_region(item,&rect);
    if(main_feedback_pending){RECT combined;UnionRect(&combined,&main_feedback_damage,&rect);main_feedback_damage=combined;}
    else main_feedback_damage=rect;
    main_feedback_pending=1;main_feedback_knob_mask=0;InvalidateRect(window,0,FALSE);
}
static void main_feedback_redraw_knob(int item){
    RECT rect;int mask=1<<(item-1);rect.left=412;rect.right=466;rect.top=item==1?122:188;rect.bottom=rect.top+32;
    if(main_feedback_pending){RECT combined;UnionRect(&combined,&main_feedback_damage,&rect);main_feedback_damage=combined;if(main_feedback_knob_mask)main_feedback_knob_mask|=mask;}
    else{main_feedback_damage=rect;main_feedback_knob_mask=mask;}main_feedback_pending=1;InvalidateRect(window,0,FALSE);
}
static int main_feedback_active(void){return main_slides[0].active||main_slides[1].active;}
static void main_feedback_schedule(void){
    DWORD now=GetTickCount(),wait=0;int i;
    if(!window)return;
    if(main_feedback_timer)KillTimer(window,MAIN_FEEDBACK_TIMER);main_feedback_timer=0;
    if(!IsIconic(window)){
        if(main_feedback_active()){
            if(main_compositor_ready){
                LONGLONG precise=main_feedback_now();main_feedback_clock_stop();
                for(i=0;i<2;i++)if(main_slides[i].active){double remaining=MAIN_FEEDBACK_DURATION-(precise-main_slides[i].started)*1000.0/main_feedback_frequency;DWORD next=remaining>0?(DWORD)remaining+1:1;if(!wait||next<wait)wait=next;}
            }else if(!main_feedback_clock_start())wait=16;
        }else main_feedback_clock_stop();
        for(i=0;i<2;i++)if(main_action_feedback[i].until){LONG remaining=(LONG)(main_action_feedback[i].until-now);DWORD next=remaining>0?(DWORD)remaining:1;if(!wait||next<wait)wait=next;}
        if(wait)main_feedback_timer=SetTimer(window,MAIN_FEEDBACK_TIMER,wait,0)!=0;
    }else main_feedback_clock_stop();
    /* A failed timer must not leave a knob or a success label stuck halfway. */
    if(wait&&!main_feedback_timer){for(i=0;i<2;i++){main_slides[i].position=main_slides[i].target;main_slides[i].active=0;main_action_feedback[i].result=0;main_action_feedback[i].until=0;main_feedback_redraw(i+1);main_feedback_redraw(i+3);}}
    main_feedback_properties();
}
static void main_feedback_motion(void){
    int i;BOOL enabled=TRUE;if(SystemParametersInfoW(0x1042,0,&enabled,0))main_feedback_animate=enabled!=0;
    if(!main_feedback_animate){for(i=0;i<2;i++)if(main_slides[i].active){main_slides[i].position=main_slides[i].target;main_slides[i].active=0;main_feedback_redraw(i+1);}if(main_compositor_ready)main_compositor_switches();main_feedback_schedule();}
}
static void main_feedback_init(void){
    LARGE_INTEGER frequency;QueryPerformanceFrequency(&frequency);main_feedback_frequency=frequency.QuadPart;
    main_feedback_motion();main_slides[0].position=main_slides[0].target=on?1000:0;main_slides[1].position=main_slides[1].target=startup?1000:0;main_feedback_properties();
}
static void main_feedback_evaluate(void){
    int i;LONGLONG now=main_feedback_now();for(i=0;i<2;i++)if(main_slides[i].active){double p=(now-main_slides[i].started)*1000.0/main_feedback_frequency/MAIN_FEEDBACK_DURATION;
        if(p>=1){main_slides[i].position=main_slides[i].target;main_slides[i].active=0;}
        else{if(p<0)p=0;p=p*p*(3-2*p);main_slides[i].position=main_slides[i].from+(int)((main_slides[i].target-main_slides[i].from)*p);}
    }
}
static void main_feedback_sync_switches(int animate){
    int i,changed=0;LONGLONG now;main_feedback_evaluate();now=main_feedback_now();for(i=0;i<2;i++){
        int target=(i?startup:on)?1000:0;if(main_slides[i].target==target)continue;
        main_slides[i].from=main_slides[i].position;main_slides[i].target=target;main_slides[i].started=now;
        main_slides[i].active=animate&&main_feedback_animate&&!IsIconic(window)&&main_slides[i].position!=target;
        if(!main_slides[i].active)main_slides[i].position=target;main_feedback_redraw(i+1);changed=1;
    }
    if(changed&&main_compositor_ready)main_compositor_switches();
    main_feedback_schedule();
}
static void main_feedback_notice(int item,int result){
    int i=item-3;if(i<0||i>1)return;main_action_feedback[i].result=result;main_action_feedback[i].until=GetTickCount()+(result==1?2200:3600);
    main_feedback_redraw(item);main_feedback_schedule();
}
static int main_feedback_tick(void){
    DWORD now=GetTickCount();LONGLONG precise=main_feedback_now();int i,changed=0;main_feedback_ticks++;
    if(main_compositor_ready)main_compositor_health();
    for(i=0;i<2;i++){
        if(main_slides[i].active){double elapsed=(precise-main_slides[i].started)*1000.0/main_feedback_frequency;float p;
            if(elapsed>=MAIN_FEEDBACK_DURATION){main_slides[i].position=main_slides[i].target;main_slides[i].active=0;}
            else{p=elapsed/(float)MAIN_FEEDBACK_DURATION;p=p*p*(3-2*p);main_slides[i].position=main_slides[i].from+(int)((main_slides[i].target-main_slides[i].from)*p);}
            if(!main_compositor_ready)main_feedback_redraw_knob(i+1);changed=1;
        }
        if(main_action_feedback[i].until&&(LONG)(main_action_feedback[i].until-now)<=0){main_action_feedback[i].until=0;main_action_feedback[i].result=0;main_feedback_redraw(i+3);changed=1;}
    }
    if(!main_feedback_active()||main_compositor_ready)main_feedback_schedule();else main_feedback_properties();return changed;
}
/* The UI thread waits only during a slide: no worker thread, global
   timer-resolution change, or idle wake-up. */
static int main_feedback_message(MSG *message){
    for(;;){DWORD ready;if(!main_feedback_clock)return GetMessageW(message,0,0,0);
        ready=MsgWaitForMultipleObjectsEx(1,&main_feedback_clock,INFINITE,QS_ALLINPUT,0x0004);
        if(ready==WAIT_OBJECT_0){
            SendMessageW(window,WM_TIMER,MAIN_FEEDBACK_TIMER,0);UpdateWindow(window);
            if(main_feedback_clock){main_feedback_due+=main_feedback_step;if(!main_feedback_clock_arm()){main_feedback_clock_stop();main_feedback_clock_unavailable=1;main_feedback_schedule();}}
        }else if(ready==WAIT_OBJECT_0+1){if(PeekMessageW(message,0,0,0,PM_REMOVE))return message->message==WM_QUIT?0:1;}
        else{main_feedback_clock_stop();main_feedback_clock_unavailable=1;main_feedback_schedule();}
    }
}
static int main_feedback_point(HWND hwnd,LPARAM point){
    RECT rect;int unit,ox,oy,x=(short)LOWORD(point),y=(short)HIWORD(point);GetClientRect(hwnd,&rect);
    if(x<0||y<0||x>=rect.right||y>=rect.bottom)return 0;canvas(rect.right,rect.bottom,&unit,&ox,&oy);
    return main_hit_item(MulDiv(x-ox,10000,unit),MulDiv(y-oy,10000,unit));
}
static void main_feedback_hot(int item){
    int previous=main_hot_item;if(previous==item)return;main_hot_item=item;main_feedback_redraw(previous);main_feedback_redraw(item);main_feedback_properties();
}
static void main_feedback_track(HWND hwnd){
    TRACKMOUSEEVENT track;if(main_mouse_tracking)return;memset(&track,0,sizeof(track));track.cbSize=sizeof(track);track.dwFlags=TME_LEAVE;track.hwndTrack=hwnd;
    main_mouse_tracking=TrackMouseEvent(&track)!=0;
}
static void main_feedback_cancel(HWND hwnd,int clear_hot){
    int item=main_pressed_item;main_pressed_item=main_press_source=main_pressed_key=0;
    if(GetCapture()==hwnd)ReleaseCapture();main_feedback_redraw(item);if(clear_hot)main_feedback_hot(0);main_feedback_properties();
}
static void main_feedback_suspend(HWND hwnd){
    int i;main_feedback_cancel(hwnd,1);for(i=0;i<2;i++){main_slides[i].position=main_slides[i].target;main_slides[i].active=0;}
    if(main_compositor_ready)main_compositor_switches();if(main_feedback_timer)KillTimer(hwnd,MAIN_FEEDBACK_TIMER);main_feedback_timer=0;main_feedback_clock_stop();main_feedback_properties();
}
static int main_feedback_down(int item){return main_pressed_item==item&&(main_press_source==2||main_hot_item==item);}
static int main_feedback_highlight(int item){return main_hot_item==item||focus_item==item;}
static COLORREF main_feedback_mix(COLORREF a,COLORREF b,int p){
    return RGB((GetRValue(a)*(1000-p)+GetRValue(b)*p)/1000,(GetGValue(a)*(1000-p)+GetGValue(b)*p)/1000,(GetBValue(a)*(1000-p)+GetBValue(b)*p)/1000);
}
#endif
