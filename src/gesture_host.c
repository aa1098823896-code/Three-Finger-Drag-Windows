#define UNICODE
#define _UNICODE
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <math.h>
#include "app_shared.h"

/* The background entry is native; read-only diagnostics never inject input. */
typedef struct { USHORT usage,page,input,output,feature,reserved[17];
    USHORT links,in_buttons,in_values,in_indices,out_buttons,out_values,out_indices,feat_buttons,feat_values,feat_indices; } HID_CAPS;
typedef struct { USHORT page; BYTE report_id,alias; USHORT bitfield,link,link_usage,link_page;
    BYTE range,string_range,designator_range,absolute,has_null,reserved; USHORT bits,count,reserved2[5];
    ULONG exponent,units; LONG logical_min,logical_max,physical_min,physical_max;
    USHORT usage,usage_max,string_min,string_max,designator_min,designator_max,index_min,index_max; } VALUE_CAP;
typedef LONG (WINAPI *GET_CAPS)(void*,HID_CAPS*);
typedef LONG (WINAPI *GET_VALUES)(int,VALUE_CAP*,USHORT*,void*);
typedef LONG (WINAPI *GET_VALUE)(int,USHORT,USHORT,USHORT,ULONG*,void*,void*,ULONG);
typedef LONG (WINAPI *GET_USAGES)(int,USHORT,USHORT,USHORT*,ULONG*,void*,void*,ULONG);
typedef struct { UINT type,id,frame,flags; HANDLE device; HWND target; POINT pixel,himetric,pixel_raw,himetric_raw;
    DWORD time,history; int input_data; DWORD keys; ULONGLONG performance; UINT button; } PTR_INFO;
typedef struct { PTR_INFO pointer; UINT flags,mask; RECT contact,contact_raw; UINT orientation,pressure; } TOUCH_INFO;
typedef struct { UINT type; TOUCH_INFO touch; } TYPE_INFO;
typedef struct { UINT type,count,feedback; HMONITOR monitor; UINT width,height,options; } CREATE_INFO;
typedef HANDLE (WINAPI *CREATE_POINTER)(CREATE_INFO*);
typedef BOOL (WINAPI *INJECT_POINTER)(HANDLE,TYPE_INFO*,UINT);
typedef void (WINAPI *DESTROY_POINTER)(HANDLE);
typedef struct { int id,x,y,valid,tip; } CONTACT;
typedef struct {
    HANDLE hid; void *preparsed; VALUE_CAP caps[128]; USHORT cap_count;
    DWORD vendor,product;
    int x_min,x_max,y_min,y_max; UINT width,height;
    CONTACT pending[10]; int pending_n,target; ULONG scan;
    HANDLE synthetic; int active,tracked; TYPE_INFO frame; DWORD first_time,last_time;
} DEVICE;
typedef struct { CONTACT old[10]; int old_n,trust_id[10],trust_n; DWORD born[10],last;
    int original,short_count,long_count,held,wait_lift; double short_move,long_move; DWORD due;
} DRAG_STATE;
static DEVICE devices[4]; static int device_n,registered,enabled,simulate,forced_error,device_probe,sources_frozen,production;
static GET_CAPS hid_caps; static GET_VALUES hid_values; static GET_VALUE hid_value; static GET_USAGES hid_usages;
static CREATE_POINTER create_pointer; static INJECT_POINTER inject_pointer; static DESTROY_POINTER destroy_pointer;
static HWND host; static DRAG_STATE drag;
static HANDLE drag_source;
static unsigned long frames,starts,ends,native_frames,native_starts,native_ends;
static unsigned long real_input_calls;
static int last_error,max_fingers,shutdown_requested; static DWORD stop_at; static WCHAR folder[MAX_PATH];
static const WCHAR class_name[]=APP_HOST_CLASS;
#include "tray_icon.h"
static int clamp_int(int v,int min,int max){return v<min?min:(v>max?max:v);}
static void write_file(const WCHAR *name,const char *text){
    WCHAR path[MAX_PATH]; DWORD done; HANDLE file;
    _snwprintf(path,MAX_PATH,L"%s\\%s",folder,name); path[MAX_PATH-1]=0;
    file=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);
    if(file!=INVALID_HANDLE_VALUE){WriteFile(file,text,(DWORD)strlen(text),&done,0);CloseHandle(file);}
}
static int load_hid(void){
    HMODULE hid=LoadLibraryW(L"hid.dll"); if(!hid)return 0;
    hid_caps=(GET_CAPS)GetProcAddress(hid,"HidP_GetCaps");hid_values=(GET_VALUES)GetProcAddress(hid,"HidP_GetValueCaps");
    hid_value=(GET_VALUE)GetProcAddress(hid,"HidP_GetUsageValue");hid_usages=(GET_USAGES)GetProcAddress(hid,"HidP_GetUsages");
    return hid_caps&&hid_values&&hid_value&&hid_usages;
}
static UINT physical_size(VALUE_CAP *axis){
    int exponent=(int)(axis->exponent&15); double size,unit;
    if(exponent>=8)exponent-=16;
    unit=(axis->units&15)==1?1000.0:((axis->units&15)==3?2540.0:0);
    size=(axis->physical_max-axis->physical_min)*pow(10.0,exponent)*unit;
    if(size<500||size>40000||axis->logical_max<=axis->logical_min)return 0;
    return (UINT)(size+.5);
}
static int add_device(HANDLE handle){
    DEVICE *d; RID_DEVICE_INFO info; UINT size=sizeof(info); HID_CAPS caps; VALUE_CAP *x=0,*y=0; int i;
    for(i=0;i<device_n;i++)if(devices[i].hid==handle)return i;
    if(device_n>=4)return -1;
    memset(&info,0,sizeof(info));info.cbSize=sizeof(info);
    if(GetRawInputDeviceInfoW(handle,RIDI_DEVICEINFO,&info,&size)==(UINT)-1||info.dwType!=RIM_TYPEHID||info.hid.usUsagePage!=13||info.hid.usUsage!=5)return -1;
    if(sources_frozen&&(!info.hid.dwVendorId||!info.hid.dwProductId))return -1;
    d=&devices[device_n];memset(d,0,sizeof(*d));d->hid=handle;d->tracked=-1;
    d->vendor=info.hid.dwVendorId;d->product=info.hid.dwProductId;
    size=0;GetRawInputDeviceInfoW(handle,RIDI_PREPARSEDDATA,0,&size);if(!size||size>65536)return -1;
    d->preparsed=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,size);if(!d->preparsed)return -1;
    if(GetRawInputDeviceInfoW(handle,RIDI_PREPARSEDDATA,d->preparsed,&size)==(UINT)-1||hid_caps(d->preparsed,&caps)!=0x110000||caps.in_values>128)goto fail;
    d->cap_count=caps.in_values;
    if(hid_values(0,d->caps,&d->cap_count,d->preparsed)!=0x110000)goto fail;
    for(i=0;i<d->cap_count;i++){VALUE_CAP *v=&d->caps[i];if(v->page==1&&v->link&&v->usage==0x30&&!x)x=v;}
    if(!x)goto fail;
    for(i=0;i<d->cap_count;i++){VALUE_CAP *v=&d->caps[i];if(v->page==1&&v->link==x->link&&v->usage==0x31){y=v;break;}}
    if(!y)goto fail;
    d->x_min=x->logical_min;d->x_max=x->logical_max;d->y_min=y->logical_min;d->y_max=y->logical_max;
    d->width=physical_size(x);d->height=physical_size(y);if(!d->width||!d->height)goto fail;
    d->frame.type=5;d->frame.touch.pointer.type=5;d->frame.touch.pointer.id=1;d->first_time=GetTickCount();
    return device_n++;
fail: HeapFree(GetProcessHeap(),0,d->preparsed);memset(d,0,sizeof(*d));return -1;
}
static void enumerate_devices(void){
    RAWINPUTDEVICELIST list[128];UINT n=128;int i;
    if(GetRawInputDeviceList(list,&n,sizeof(list[0]))==(UINT)-1)return;
    for(i=0;i<(int)n;i++)if(list[i].dwType==RIM_TYPEHID)add_device(list[i].hDevice);
}
static int native_device(DEVICE *d){
    CREATE_INFO info;HMODULE user=GetModuleHandleW(L"user32.dll");
    if(d->synthetic)return 1;
    if(simulate){d->synthetic=(HANDLE)1;return 1;}
    if(sizeof(TYPE_INFO)!=152||sizeof(CREATE_INFO)!=40){last_error=ERROR_INVALID_DATA;return 0;}
    create_pointer=(CREATE_POINTER)GetProcAddress(user,"CreateSyntheticPointerDevice2");
    inject_pointer=(INJECT_POINTER)GetProcAddress(user,"InjectSyntheticPointerInput");
    destroy_pointer=(DESTROY_POINTER)GetProcAddress(user,"DestroySyntheticPointerDevice");
    if(!create_pointer||!inject_pointer||!destroy_pointer){last_error=ERROR_PROC_NOT_FOUND;return 0;}
    memset(&info,0,sizeof(info));info.type=5;info.count=1;info.feedback=3;info.width=d->width;info.height=d->height;info.options=1;
    d->synthetic=create_pointer(&info);if(!d->synthetic){last_error=GetLastError();return 0;}return 1;
}
static int native_inject(DEVICE *d,DWORD now){
    DWORD time=now-d->first_time+1;
    d->frame.touch.pointer.time=time>d->last_time?time:d->last_time+1;d->last_time=d->frame.touch.pointer.time;
    if(simulate&&forced_error){last_error=forced_error;return 0;}
    if(!simulate&&!inject_pointer(d->synthetic,&d->frame,1)){last_error=GetLastError();return 0;}
    if(!simulate)real_input_calls++;
    native_frames++;return 1;
}
static void end_native(void){int i;for(i=0;i<device_n;i++){DEVICE *d=&devices[i];if(!d->active)continue;
    /* A normal lift completes the interaction. CANCELED asks pointer-aware
       receivers to undo it, including a completed snipping selection. */
    d->frame.touch.pointer.flags=0x4000;
    if(!native_inject(d,GetTickCount())&&!simulate&&d->synthetic){destroy_pointer(d->synthetic);d->synthetic=0;}
    d->active=0;d->tracked=-1;native_ends++;
}}
static int mouse_button(DWORD flags){
    INPUT input; if(simulate)return 1;
    memset(&input,0,sizeof(input));input.type=INPUT_MOUSE;input.mi.dwFlags=flags;
    if(SendInput(1,&input,sizeof(input))!=1){last_error=GetLastError();return 0;}real_input_calls++;return 1;
}
static void release_drag(void){
    /* A release ends this contact sequence. Its trailing reports must never
       inject another LEFTDOWN, even after the missing-report deadline. */
    if(drag.held)drag.wait_lift=1;
    end_native();if(drag.held){
        if(!mouse_button(MOUSEEVENTF_LEFTUP)){drag.due=GetTickCount()+10;enabled=0;return;}
        drag.held=0;ends++;
    }drag.due=0;if(host)KillTimer(host,1);
}
static void native_follow(DEVICE *d,CONTACT *c,int n,DWORD now){
    int i,chosen=-1;CONTACT sorted[10],tmp;
    if(!native_device(d)){release_drag();enabled=0;return;}
    for(i=0;i<n;i++)if(c[i].id==d->tracked)chosen=i;
    if(d->active&&chosen<0)end_native();
    if(!d->active){int j;memcpy(sorted,c,n*sizeof(CONTACT));for(i=0;i<n;i++)for(j=i+1;j<n;j++)if(sorted[j].x<sorted[i].x){tmp=sorted[i];sorted[i]=sorted[j];sorted[j]=tmp;}
        d->tracked=sorted[n/2].id;for(i=0;i<n;i++)if(c[i].id==d->tracked)chosen=i;
    }
    if(chosen<0)return;
    d->frame.touch.pointer.himetric.x=clamp_int((int)((c[chosen].x-d->x_min)*(double)d->width/(d->x_max-d->x_min)+.5),0,d->width);
    d->frame.touch.pointer.himetric.y=clamp_int((int)((c[chosen].y-d->y_min)*(double)d->height/(d->y_max-d->y_min)+.5),0,d->height);
    d->frame.touch.pointer.flags=0x4006;
    if(!native_inject(d,now)){release_drag();enabled=0;return;}
    if(!d->active){d->active=1;native_starts++;}
}
/* Same contact identity, 40ms trust delay, and moving-finger gating as the
   accepted engine. Geometry conversion never applies a pointer speed curve. */
static void process_contacts(DEVICE *d,CONTACT *c,int n,DWORD now){
    int i,j,common,gap;double longest=0;int short_count,long_count;
    if(drag_source&&drag_source!=d->hid){release_drag();memset(&drag,0,sizeof(drag));}
    drag_source=d->hid;
    if(n>max_fingers)max_fingers=n;
    if(drag.held&&n!=3)release_drag();
    if(drag.wait_lift){
        int overlap=0;
        for(i=0;i<n&&!overlap;i++)for(j=0;j<drag.old_n;j++)if(c[i].id==drag.old[j].id){overlap=1;break;}
        if(n&&overlap){drag.last=now;return;}
        /* Zero contacts confirms lift. Entirely fresh identities also recover
           from a dropped terminal report, through the usual movement gate. */
        memset(&drag,0,sizeof(drag));
        if(!n){drag.last=now;return;}
    }
    common=n==drag.old_n;gap=(DWORD)(now-drag.last)>40;
    if(gap){drag.trust_n=0;}
    for(i=0;i<drag.trust_n;){int present=0;for(j=0;j<n;j++)if(c[j].id==drag.trust_id[i])present=1;
        if(present)i++;else{drag.trust_n--;drag.trust_id[i]=drag.trust_id[drag.trust_n];drag.born[i]=drag.born[drag.trust_n];}}
    for(i=0;i<n;i++){
        int found=-1;for(j=0;j<drag.old_n;j++)if(c[i].id==drag.old[j].id){found=j;break;}
        if(found<0)common=0;
        int trust=-1;for(j=0;j<drag.trust_n;j++)if(drag.trust_id[j]==c[i].id){trust=j;break;}
        if(trust<0&&drag.trust_n<10){trust=drag.trust_n++;drag.trust_id[trust]=c[i].id;drag.born[trust]=now;}
        if(!gap&&found>=0&&trust>=0&&(DWORD)(now-drag.born[trust])>40){
            double dx=c[i].x-drag.old[found].x,dy=c[i].y-drag.old[found].y,dist=sqrt(dx*dx+dy*dy);
            if(dist>longest)longest=dist;
        }
    }
    if(!common&&(n<=1||gap))drag.original=0;
    if(!common||gap){drag.short_move=drag.long_move=0;}
    else{
        /* Gating in millimetres; this does not scale injected motion. */
        double gate_mm=longest*(double)d->width/(100.0*(d->x_max-d->x_min));
        if(gate_mm>=.025){drag.short_move+=gate_mm;drag.long_move+=gate_mm;}
        if(drag.short_move>=.025){drag.short_count=n;drag.short_move=0;}
        if(drag.long_move>.1){drag.long_count=n;drag.long_move=0;if(drag.original<=1)drag.original=n;}
    }
    short_count=drag.short_count;long_count=drag.long_count;
    if(enabled&&n==3&&common&&long_count==3&&drag.original==3&&!drag.held){
        end_native();if(mouse_button(MOUSEEVENTF_LEFTDOWN)){drag.held=1;starts++;drag.due=now+40;if(host)SetTimer(host,1,10,0);}
    }else if(drag.held&&(short_count<2||(drag.original!=3&&drag.original>=2)))release_drag();
    if(drag.held&&enabled&&n==3){drag.due=now+40;native_follow(d,c,n,now);}
    memcpy(drag.old,c,n*sizeof(CONTACT));drag.old_n=n;drag.last=now;
}
static void submit(DEVICE *d,CONTACT *c,int n,ULONG count,ULONG scan,DWORD now){
    int i,j,active_n=0;CONTACT active[10];
    if(count>10)return;
    if(!n){if(!count){d->pending_n=d->target=0;process_contacts(d,c,0,now);}return;}
    if(count>0){d->pending_n=0;d->target=(int)count;d->scan=scan;}
    else if(!d->target){
        for(i=0;i<n;i++)if(c[i].tip)return;
        process_contacts(d,c,0,now);return;
    }else if(scan!=d->scan)return;
    /* Contact Count includes lift records. Assemble those records first;
       excluding Tip=0 before assembly loses the terminal frame. Stop at the
       declared count so unused slots cannot become phantom contacts. */
    for(i=0;i<n&&d->pending_n<d->target;i++){
        for(j=0;j<d->pending_n;j++)if(d->pending[j].id==c[i].id)break;
        if(j==d->pending_n&&j<10)d->pending[d->pending_n++]=c[i];
    }
    if(d->pending_n>=d->target){
        for(i=0;i<d->target;i++)if(d->pending[i].tip)active[active_n++]=d->pending[i];
        process_contacts(d,active,active_n,now);d->pending_n=d->target=0;
    }
}
static void receive_raw(HRAWINPUT handle){
    static BYTE buffer[65536];UINT size=sizeof(buffer);RAWINPUT *raw;DEVICE *d;int device=-1,r,i;
    if(production&&!enabled)return;
    if(GetRawInputData(handle,RID_INPUT,buffer,&size,sizeof(RAWINPUTHEADER))==(UINT)-1||size<sizeof(RAWINPUTHEADER)+8)return;
    raw=(RAWINPUT*)buffer;if(raw->header.dwType!=RIM_TYPEHID)return;
    for(i=0;i<device_n;i++)if(devices[i].hid==raw->header.hDevice){device=i;break;}if(device<0)return;
    d=&devices[device];if(!raw->data.hid.dwSizeHid||raw->data.hid.dwCount>64||raw->data.hid.dwSizeHid*raw->data.hid.dwCount>size-sizeof(RAWINPUTHEADER)-8)return;
    for(r=0;r<(int)raw->data.hid.dwCount;r++){
        BYTE *report=raw->data.hid.bRawData+r*raw->data.hid.dwSizeHid;CONTACT contacts[16];USHORT links[16];int slots=0,n=0;ULONG scan=0,count=0;
        memset(contacts,0,sizeof(contacts));memset(links,0,sizeof(links));
        for(i=0;i<d->cap_count;i++){VALUE_CAP *v=&d->caps[i];ULONG value=0;int slot;
            if(hid_value(0,v->page,v->link,v->usage,&value,d->preparsed,report,raw->data.hid.dwSizeHid)!=0x110000)continue;
            if(!v->link){if(v->page==13&&v->usage==0x54)count=value;else if(v->page==13&&v->usage==0x56)scan=value;continue;}
            for(slot=0;slot<slots;slot++)if(links[slot]==v->link)break;if(slot==slots){if(slots>=16)continue;links[slots++]=v->link;}
            if(v->page==13&&v->usage==0x51){contacts[slot].id=(int)value;contacts[slot].valid|=1;}
            else if(v->page==1&&v->usage==0x30){contacts[slot].x=(int)value;contacts[slot].valid|=2;}
            else if(v->page==1&&v->usage==0x31){contacts[slot].y=(int)value;contacts[slot].valid|=4;}
        }
        for(i=0;i<slots;i++)if(contacts[i].valid==7){USHORT usages[16];ULONG usage_n=16;int tip=1,j;
            if(hid_usages(0,13,links[i],usages,&usage_n,d->preparsed,report,raw->data.hid.dwSizeHid)==0x110000){tip=0;for(j=0;j<(int)usage_n;j++)if(usages[j]==0x42)tip=1;}
            if(n<10){contacts[i].tip=tip;contacts[n++]=contacts[i];}
        }
        frames++;submit(d,contacts,n,count,scan,GetTickCount());
    }
}
static void status(void){
    char text[1200];WCHAR number[32];
    tray_update();
    _snprintf(text,sizeof(text),"{\"ProcessId\":%lu,\"Mode\":\"%s\",\"DotNetRuntime\":false,\"UIProcessSeparate\":true,\"TrayIconRegistered\":%s,\"Touchpads\":%d,\"Enabled\":%s,\"InputReceiverRegistered\":%s,\"Frames\":%lu,\"MaxFingers\":%d,\"DragStarts\":%lu,\"DragEnds\":%lu,\"NativeFrames\":%lu,\"AppPointerMovementCalls\":0,\"Held\":%s,\"LastError\":%d,\"InjectedRealInput\":%s,\"WidthMm\":%.2f,\"HeightMm\":%.2f}",
        GetCurrentProcessId(),device_probe?"NativeDeviceProbe":(production?"NativeBackground":(enabled&&!simulate?"NativeTrial":"ReadOnlyMonitor")),tray_registered?"true":"false",device_n,enabled?"true":"false",registered?"true":"false",frames,max_fingers,starts,ends,native_frames,drag.held?"true":"false",last_error,real_input_calls?"true":"false",device_n?devices[0].width/100.0:0,device_n?devices[0].height/100.0:0);
    text[sizeof(text)-1]=0;write_file(L"host-status.json",text);
    if(production){static int prior_state=-1,prior_error=-1;int state=last_error?3:(!device_n?2:(!enabled?1:0));
      if(state!=prior_state||last_error!=prior_error){
        WritePrivateProfileStringW(L"Status",L"State",last_error?L"error":(!device_n?L"disconnected":(!enabled?L"disabled":L"running")),config_path);
        _snwprintf(number,32,L"%d",last_error);WritePrivateProfileStringW(L"Status",L"Error",number,config_path);
        _snwprintf(number,32,L"%lu",GetCurrentProcessId());WritePrivateProfileStringW(L"Status",L"ProcessId",number,config_path);
        prior_state=state;prior_error=last_error;
      }
    }
}
static void reset_sources(void){
    int i;release_drag();if(drag.held)return;
    for(i=0;i<device_n;i++){
        if(!simulate&&devices[i].synthetic&&destroy_pointer)destroy_pointer(devices[i].synthetic);
        if(devices[i].preparsed)HeapFree(GetProcessHeap(),0,devices[i].preparsed);
    }
    device_n=0;drag_source=0;memset(&drag,0,sizeof(drag));enumerate_devices();
    if(!simulate){for(i=0;i<device_n;i++)if(!native_device(&devices[i]))enabled=0;}
}
static LRESULT CALLBACK wndproc(HWND hwnd,UINT message,WPARAM wp,LPARAM lp){
    if(production&&tray_taskbar_created&&message==tray_taskbar_created){tray_registered=0;tray_ensure();status();return 0;}
    if(message==APP_TRAY_READY)return tray_ensure();
    if(message==APP_TRAY_CALLBACK&&production){UINT event=LOWORD(lp);
        if(event==WM_CONTEXTMENU)tray_context_menu(wp);
        else if(event==0x0400||event==0x0401||event==WM_LBUTTONDBLCLK)tray_action(APP_TRAY_OPEN);return 0;}
    if(message==WM_COMMAND&&production&&(LOWORD(wp)==APP_TRAY_OPEN||LOWORD(wp)==APP_TRAY_EXIT)){tray_action(LOWORD(wp));return 0;}
    if(message==WM_INPUT){receive_raw((HRAWINPUT)lp);return DefWindowProcW(hwnd,message,wp,lp);}
    if(message==0x00FE){int i;
        if(wp==2){for(i=0;i<device_n;i++)if(devices[i].hid==(HANDLE)lp){reset_sources();status();break;}}
        else if(wp==1&&!drag.held){int added=add_device((HANDLE)lp);if(added>=0&&!simulate)native_device(&devices[added]);status();}
        return 0;
    }
    if(message==WM_POWERBROADCAST){if(wp==4)release_drag();else if(wp==7||wp==18){reset_sources();status();}return TRUE;}
    if(message==WM_TIMER){DWORD now=GetTickCount();if(drag.held&&(LONG)(now-drag.due)>=0)release_drag();
        if(shutdown_requested&&!drag.held){DestroyWindow(hwnd);return 0;}
        if(wp==2)status();if(stop_at&&(LONG)(now-stop_at)>=0)DestroyWindow(hwnd);return 0;}
    if(message==APP_RELOAD){int requested=app_enabled();release_drag();enabled=0;
        if(!drag.held){memset(&drag,0,sizeof(drag));drag_source=0;if(requested&&last_error){last_error=0;reset_sources();}enabled=requested&&last_error==0;}status();return 0;}
    if(message==APP_QUIT){HWND settings=production?FindWindowW(APP_UI_CLASS,0):0,menu=production?FindWindowW(APP_TRAY_MENU_CLASS,0):0;
        if(settings)PostMessageW(settings,WM_APP+3,0,0);if(menu)PostMessageW(menu,WM_CANCELMODE,0,0);
        shutdown_requested=1;enabled=0;release_drag();if(!drag.held)DestroyWindow(hwnd);return 0;}
    if(message==WM_HOTKEY&&wp==1){app_save_enabled(0);PostMessageW(hwnd,APP_RELOAD,0,0);return 0;}
    if(message==WM_DESTROY){UnregisterHotKey(hwnd,1);release_drag();tray_cleanup();status();PostQuitMessage(0);return 0;}
    return DefWindowProcW(hwnd,message,wp,lp);
}
static void cleanup(void){int i;release_drag();for(i=0;i<device_n;i++){
    if(!simulate&&devices[i].synthetic&&destroy_pointer)destroy_pointer(devices[i].synthetic);
    if(devices[i].preparsed)HeapFree(GetProcessHeap(),0,devices[i].preparsed);
}}
static int self_test(void){
    CONTACT c[3],one;int i,j,cycle,continuous,quick,transition,hybrid,fault,repeated,passed,timer_pass=0;DWORD now=1000,timer_elapsed=0;char receipt[1500];DEVICE *d;
    simulate=enabled=1;enumerate_devices();if(!device_n)return 2;d=&devices[0];d->first_time=1000;
    for(i=0;i<30;i++){for(j=0;j<3;j++){c[j].id=j+1;c[j].tip=1;c[j].x=400+j*220+i*12;c[j].y=400+j*100;}process_contacts(d,c,3,now);now+=10;}
    continuous=drag.held&&starts==1&&native_frames>10;release_drag();continuous=continuous&&ends==1;
    memset(&drag,0,sizeof(drag));for(i=0;i<30&&!drag.held;i++){for(j=0;j<3;j++)c[j].x+=12;process_contacts(d,c,3,now);now+=10;}
    quick=drag.held;now=drag.due;if(drag.held&&(LONG)(now-drag.due)>=0)release_drag();quick=quick&&!drag.held;
    memset(&drag,0,sizeof(drag));for(i=0;i<30&&!drag.held;i++){for(j=0;j<3;j++)c[j].x+=12;process_contacts(d,c,3,now);now+=10;}
    one=c[0];transition=drag.held;process_contacts(d,&one,1,now);transition=transition&&!drag.held;
    memset(&drag,0,sizeof(drag));submit(d,c,1,3,77,now+10);submit(d,c+1,2,0,77,now+10);hybrid=drag.old_n==3;
    for(i=0;i<30&&!drag.held;i++){for(j=0;j<3;j++)c[j].x+=12;process_contacts(d,c,3,now+20+i*10);}
    fault=drag.held;forced_error=5;process_contacts(d,c,3,now+330);forced_error=0;fault=fault&&!drag.held&&!enabled&&last_error==5;
    enabled=1;last_error=0;now+=1000;repeated=1;
    for(cycle=0;cycle<100;cycle++){process_contacts(d,c,0,now);now+=10;
        for(i=0;i<15;i++){for(j=0;j<3;j++){c[j].id=cycle*3+j+20;c[j].x=400+j*220+i*12;c[j].y=400+j*100;}process_contacts(d,c,3,now);now+=10;}
        if(!drag.held)repeated=0;process_contacts(d,c,0,now);now+=10;if(drag.held)repeated=0;
    }
    {WNDCLASSW wc;MSG msg;DWORD began;memset(&wc,0,sizeof(wc));wc.lpfnWndProc=wndproc;wc.hInstance=GetModuleHandleW(0);wc.lpszClassName=L"ThreeFingerDrag-Release-Timer-Check";
        if(RegisterClassW(&wc)){host=CreateWindowExW(0,wc.lpszClassName,L"",0,0,0,0,0,0,0,wc.hInstance,0);
            if(host){began=GetTickCount();drag.held=1;starts++;drag.due=began+40;SetTimer(host,1,10,0);
                while(drag.held&&(DWORD)(GetTickCount()-began)<120){while(PeekMessageW(&msg,0,0,0,PM_REMOVE)){TranslateMessage(&msg);DispatchMessageW(&msg);}Sleep(1);}
                timer_elapsed=GetTickCount()-began;timer_pass=!drag.held&&timer_elapsed>=40&&timer_elapsed<=90;DestroyWindow(host);host=0;
            }
        }
    }
    passed=continuous&&quick&&transition&&hybrid&&fault&&repeated&&timer_pass&&starts==ends&&sizeof(TYPE_INFO)==152&&sizeof(CREATE_INFO)==40;
    _snprintf(receipt,sizeof(receipt),"{\"Passed\":%s,\"ContinuousDrag\":%s,\"ImmediateReleaseOnOneFinger\":%s,\"ReleaseDeadline40ms\":%s,\"Win32ReleaseTimerPassed\":%s,\"TimerObservedMs\":%lu,\"HybridReportAssembly\":%s,\"NativeErrorReleasesButton\":%s,\"Repeated100Gestures\":%s,\"DragStarts\":%lu,\"DragEnds\":%lu,\"NativeContactsStarted\":%lu,\"NativeContactsEnded\":%lu,\"AppPointerMovementCalls\":0,\"TypeInfoBytes\":%u,\"CreationInfoBytes\":%u,\"InjectedRealInput\":false}",passed?"true":"false",continuous?"true":"false",transition?"true":"false",quick?"true":"false",timer_pass?"true":"false",timer_elapsed,hybrid?"true":"false",fault?"true":"false",repeated?"true":"false",starts,ends,native_starts,native_ends,(UINT)sizeof(TYPE_INFO),(UINT)sizeof(CREATE_INFO));
    write_file(L"host-self-test.json",receipt);cleanup();return passed?0:3;
}
int WINAPI WinMain(HINSTANCE instance,HINSTANCE previous,LPSTR command,int show){
    WNDCLASSW wc;RAWINPUTDEVICE raw_device;MSG message;HANDLE mutex;int seconds=60,result=0;
    production=strstr(command,"--background")!=0;
    if(!app_paths(!production))return 1;
    _snwprintf(folder,MAX_PATH,L"%s",data_folder);folder[MAX_PATH-1]=0;
    if(strstr(command,"--quit")){HWND existing=FindWindowW(class_name,0);if(existing)PostMessageW(existing,WM_APP+1,0,0);return 0;}
    if(!*command){app_launch(L"GestureSettings.exe",L"");return 0;}
    if(!load_hid())return 1;if(strstr(command,"--self-test"))return self_test();
    simulate=!production;enabled=production?app_enabled():1;
    if(strstr(command,"--enable-native")){char *trial=strstr(command,"--trial-seconds ");if(!trial)return 4;seconds=atoi(trial+16);if(seconds<10||seconds>120)return 4;simulate=0;}
    else{char *monitor=strstr(command,"--monitor-seconds ");if(monitor){seconds=atoi(monitor+18);if(seconds<5||seconds>120)return 4;}}
    if(strstr(command,"--device-probe")){device_probe=1;simulate=0;enabled=0;}
    mutex=CreateMutexW(0,TRUE,APP_MUTEX);if(GetLastError()==ERROR_ALREADY_EXISTS){CloseHandle(mutex);return 0;}
    memset(&wc,0,sizeof(wc));wc.lpfnWndProc=wndproc;wc.hInstance=instance;wc.lpszClassName=class_name;
    if(!RegisterClassW(&wc)){CloseHandle(mutex);return 1;}
    host=CreateWindowExW(0,class_name,L"三指拖拽 · 原生内存验证",0,0,0,0,0,0,0,instance,0);if(!host){CloseHandle(mutex);return 1;}
    sources_frozen=!simulate;enumerate_devices();
    if(!simulate){int i;for(i=0;i<device_n;i++)if(!native_device(&devices[i])){cleanup();DestroyWindow(host);CloseHandle(mutex);return 1;}}
    raw_device.usUsagePage=13;raw_device.usUsage=5;raw_device.dwFlags=0x2100;raw_device.hwndTarget=host;
    registered=RegisterRawInputDevices(&raw_device,1,sizeof(raw_device));
    if(!registered){last_error=GetLastError();enabled=0;status();cleanup();DestroyWindow(host);CloseHandle(mutex);return 1;}
    stop_at=production?0:GetTickCount()+seconds*1000;
    if(production){tray_init();RegisterHotKey(host,1,MOD_CONTROL|MOD_ALT,VK_PAUSE);}
    SetTimer(host,2,1000,0);status();
    while((result=GetMessageW(&message,0,0,0))>0){TranslateMessage(&message);DispatchMessageW(&message);}
    cleanup();CloseHandle(mutex);return result<0?1:0;
}
