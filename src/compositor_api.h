#ifndef THREE_FINGER_COMPOSITOR_API_H
#define THREE_FINGER_COMPOSITOR_API_H
/* Small C ABI for the Windows system DirectComposition interfaces.
   Slots match dcomp.h (MSVC overloads are reversed in the binary ABI) and
   dcompanimation.h. No SDK, Direct3D device or bundled UI runtime is required.
   References: microsoft/win32metadata generation/WinSDK/RecompiledIdlHeaders/um
   and mingw-w64-headers/include/dcomp.h, both checked during implementation. */
typedef struct {void **v;} MAIN_DC;
#define DC_METHOD(object,slot,type) ((type)((MAIN_DC*)(object))->v[slot])
static HRESULT dc_call(void *o,int slot){return DC_METHOD(o,slot,HRESULT(WINAPI*)(void*))(o);}
static HRESULT dc_new(void *o,int slot,void **out){return DC_METHOD(o,slot,HRESULT(WINAPI*)(void*,void**))(o,out);}
static HRESULT dc_ptr(void *o,int slot,void *value){return DC_METHOD(o,slot,HRESULT(WINAPI*)(void*,void*))(o,value);}
static HRESULT dc_float(void *o,int slot,float value){return DC_METHOD(o,slot,HRESULT(WINAPI*)(void*,float))(o,value);}
static HRESULT dc_enum(void *o,int slot,int value){return DC_METHOD(o,slot,HRESULT(WINAPI*)(void*,int))(o,value);}
static HRESULT dc_target(void *o,HWND hwnd,void **out){return DC_METHOD(o,6,HRESULT(WINAPI*)(void*,HWND,BOOL,void**))(o,hwnd,TRUE,out);}
static HRESULT dc_hwnd(void *o,HWND hwnd,void **out){return DC_METHOD(o,11,HRESULT(WINAPI*)(void*,HWND,void**))(o,hwnd,out);}
/* With a NULL reference, FALSE puts the new child in front of all siblings. */
static HRESULT dc_add(void *o,void *child){return DC_METHOD(o,16,HRESULT(WINAPI*)(void*,void*,BOOL,void*))(o,child,FALSE,0);}
static HRESULT dc_cubic(void *o,float start,float delta,double seconds){
    float linear=0,quadratic=(float)(3*delta/(seconds*seconds)),cubic=(float)(-2*delta/(seconds*seconds*seconds));
    return DC_METHOD(o,5,HRESULT(WINAPI*)(void*,double,float,float,float,float))(o,0.0,start,linear,quadratic,cubic);
}
static HRESULT dc_end(void *o,double seconds,float value){return DC_METHOD(o,8,HRESULT(WINAPI*)(void*,double,float))(o,seconds,value);}
static HRESULT dc_absolute(void *o,LONGLONG time){LARGE_INTEGER begin;begin.QuadPart=time;return DC_METHOD(o,4,HRESULT(WINAPI*)(void*,LARGE_INTEGER))(o,begin);}
static void dc_release(void **o){if(*o)DC_METHOD(*o,2,ULONG(WINAPI*)(void*))(*o);*o=0;}
static const GUID main_dc_device_iid={0xc37ea93a,0xe7aa,0x450d,{0xb1,0x6f,0x97,0x46,0xcb,0x04,0x07,0xf3}};
#endif
