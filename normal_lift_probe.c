/* Exercises the physical lift format observed on the installed touchpad.
   All input is simulated; the compiled application parser is reused. */
#define WinMain lift_probe_original_entry
#include "gesture_host.c"
#undef WinMain
static DWORD probe_time=1000;static CONTACT samples[5];static DEVICE *pd;
static void reset_probe(void){int j;memset(devices,0,sizeof(devices));memset(&drag,0,sizeof(drag));memset(samples,0,sizeof(samples));device_n=1;simulate=enabled=1;production=0;drag_source=0;starts=ends=native_starts=native_ends=native_frames=real_input_calls=0;last_error=forced_error=0;probe_time=1000;pd=&devices[0];pd->hid=(HANDLE)42;pd->width=13200;pd->height=7690;pd->x_max=pd->y_max=32767;pd->tracked=-1;pd->frame.type=5;pd->frame.touch.pointer.type=5;pd->first_time=1000;for(j=0;j<3;j++){samples[j].id=j;samples[j].tip=1;samples[j].x=400+j*220;samples[j].y=500+j*100;}}
static void report(int count){int j;for(j=0;j<3;j++)samples[j].x+=12;submit(pd,samples,5,count,probe_time,probe_time);probe_time+=10;}
static void begin_drag(void){int i;for(i=0;i<15;i++)report(3);}
int WINAPI WinMain(HINSTANCE a,HINSTANCE b,LPSTR command,int show){int i,j,ended,lift_reset,normal_flags,hybrid,unused,repeated=1;FILE *f;
reset_probe();begin_drag();if(!drag.held)return 2;
samples[2].tip=0;report(3);ended=!drag.held&&ends==1;normal_flags=pd->frame.touch.pointer.flags==0x4000;
samples[1].tip=samples[0].tip=0;report(3);lift_reset=!drag.wait_lift&&drag.old_n==0;
for(j=0;j<3;j++)samples[j].tip=1;begin_drag();lift_reset=lift_reset&&drag.held&&starts==2;
reset_probe();submit(pd,samples,1,3,77,1000);submit(pd,samples+1,2,0,77,1000);hybrid=drag.old_n==3;
samples[2].tip=0;submit(pd,samples,1,3,78,1010);submit(pd,samples+1,2,0,78,1010);hybrid=hybrid&&drag.old_n==2;
reset_probe();samples[0].id=9;report(1);unused=drag.old_n==1&&drag.old[0].id==9;
reset_probe();for(i=0;i<100;i++){unsigned long prior=starts;for(j=0;j<3;j++)samples[j].tip=1;begin_drag();if(!drag.held||starts!=prior+1)repeated=0;for(j=0;j<3;j++)samples[j].tip=0;report(3);if(drag.held||drag.wait_lift||ends!=starts)repeated=0;}
f=fopen("normal-lift-regression.json","wb");if(!f)return 3;fprintf(f,"{\"Passed\":%s,\"TipOffReleasesImmediately\":%s,\"NormalLiftNotCanceled\":%s,\"TipOffClearsRearmLock\":%s,\"HybridIncludesLiftRecords\":%s,\"UnusedSlotsIgnored\":%s,\"Repeated100ReusedIds\":%s,\"DragStarts\":%lu,\"DragEnds\":%lu,\"RealInputCalls\":%lu}",(ended&&normal_flags&&lift_reset&&hybrid&&unused&&repeated)?"true":"false",ended?"true":"false",normal_flags?"true":"false",lift_reset?"true":"false",hybrid?"true":"false",unused?"true":"false",repeated?"true":"false",starts,ends,real_input_calls);fclose(f);cleanup();return ended&&normal_flags&&lift_reset&&hybrid&&unused&&repeated?0:4;}
