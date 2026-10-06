/* Regression reproduction: simulated touch reports only, no real input. */
#define WinMain release_probe_original_entry
#include "gesture_host.c"
#undef WinMain
static CONTACT probe_contacts[3];
static DWORD probe_time;
static DEVICE *probe_device;
static void probe_reset(void){
    int j;
    memset(devices,0,sizeof(devices));memset(&drag,0,sizeof(drag));
    device_n=1;simulate=enabled=1;production=0;drag_source=0;
    starts=ends=native_starts=native_ends=native_frames=real_input_calls=0;last_error=forced_error=0;
    probe_device=&devices[0];probe_device->hid=(HANDLE)42;probe_device->width=13200;
    probe_device->height=7690;probe_device->x_max=probe_device->y_max=32767;
    probe_device->tracked=-1;probe_device->frame.type=5;probe_device->frame.touch.pointer.type=5;probe_device->first_time=1000;
    probe_time=1000;
    for(j=0;j<3;j++){probe_contacts[j].id=j+1;probe_contacts[j].tip=1;probe_contacts[j].x=400+j*220;probe_contacts[j].y=500+j*100;}
}
static void probe_frame(int n,int move){
    int j;for(j=0;j<n;j++)probe_contacts[j].x+=move;
    process_contacts(probe_device,probe_contacts,n,probe_time);probe_time+=10;
}
static void probe_begin(void){int i;for(i=0;i<15;i++)probe_frame(3,12);}
int WINAPI WinMain(HINSTANCE a,HINSTANCE b,LPSTR command,int show){
    FILE *out;int i,j,timer_duplicate,partial_duplicate,stationary_duplicate,normal_ok=1,fresh_recovery,early_restart;
    probe_reset();probe_begin();if(!drag.held||starts!=1)return 2;
    probe_time=drag.due;release_drag();probe_frame(3,0);timer_duplicate=starts>1;
    probe_reset();probe_begin();probe_frame(2,0);for(i=0;i<12;i++)probe_frame(3,12);partial_duplicate=starts>1;
    probe_reset();probe_begin();release_drag();for(i=0;i<10;i++)probe_frame(3,0);stationary_duplicate=starts>1;
    probe_reset();probe_begin();release_drag();
    for(j=0;j<3;j++)probe_contacts[j].id+=10;
    for(i=0;i<5;i++)probe_frame(3,0);early_restart=starts>1;
    probe_begin();fresh_recovery=drag.held&&starts==2;
    probe_reset();
    for(i=0;i<100;i++){
        unsigned long before=starts;probe_begin();
        if(!drag.held||starts!=before+1)normal_ok=0;
        probe_frame(0,0);if(drag.held||ends!=starts)normal_ok=0;
    }
    out=fopen(strstr(command,"--before")?"release-rearm-before.json":"release-rearm-after.json","wb");if(!out)return 3;
    fprintf(out,"{\"SimulatedOnly\":true,\"RealInputCalls\":%lu,\"DuplicateAfterDeadlineRelease\":%s,\"DuplicateAfterPartialLift\":%s,\"DuplicateOnStationaryTail\":%s,\"FreshContactRecovery\":%s,\"FreshStationaryContactsRestarted\":%s,\"Repeated100FreshGestures\":%s,\"DragStarts\":%lu,\"DragEnds\":%lu}",real_input_calls,timer_duplicate?"true":"false",partial_duplicate?"true":"false",stationary_duplicate?"true":"false",fresh_recovery?"true":"false",early_restart?"true":"false",normal_ok?"true":"false",starts,ends);fclose(out);cleanup();
    return normal_ok&&(strstr(command,"--before")||(!timer_duplicate&&!partial_duplicate&&!stationary_duplicate&&fresh_recovery&&!early_restart))?0:4;
}
