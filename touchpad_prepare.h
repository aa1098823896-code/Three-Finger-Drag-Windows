#ifndef THREE_FINGER_TOUCHPAD_PREPARE_H
#define THREE_FINGER_TOUCHPAD_PREPARE_H
/* Runs only in the settings process, on open/check or an explicit preparation action. */
typedef struct {DWORD version,contacts,legacy,status,user,sensitivity,speed,feedback,force,right_width,right_height;} TP_PARAMS;
typedef struct TP_CACHE TP_CACHE;
typedef struct {HRESULT (WINAPI *query)(TP_CACHE *,const GUID *,void **);ULONG (WINAPI *add)(TP_CACHE *);ULONG (WINAPI *release)(TP_CACHE *);HRESULT (WINAPI *notify)(TP_CACHE *,DWORD);HRESULT (WINAPI *get_bool)(TP_CACHE *,DWORD,BOOL *);HRESULT (WINAPI *get_dword)(TP_CACHE *,DWORD,DWORD *);} TP_CACHE_VTABLE;
struct TP_CACHE {TP_CACHE_VTABLE *vtable;};
typedef struct TP_PROVIDER TP_PROVIDER;
typedef struct {HRESULT (WINAPI *query)(TP_PROVIDER *,const GUID *,void **);ULONG (WINAPI *add)(TP_PROVIDER *);ULONG (WINAPI *release)(TP_PROVIDER *);HRESULT (WINAPI *service)(TP_PROVIDER *,const GUID *,const GUID *,void **);} TP_PROVIDER_VTABLE;
struct TP_PROVIDER {TP_PROVIDER_VTABLE *vtable;};
typedef struct {HMODULE library;void (WINAPI *uninit)(void);TP_CACHE *cache;int initialized;} TP_SESSION;
typedef struct {int native_ok,cache_ok,registry_ok,present,enabled,ready,tap_drag;DWORD tap,swipe,registry_tap,registry_swipe,four_tap,four_swipe,error,build;TP_PARAMS native;} TP_STATE;
typedef struct {int present;DWORD value,type;} TP_REG_VALUE;
static int tp_reg_read(HKEY,int,TP_REG_VALUE *);
static const WCHAR tp_key_name[]=L"Software\\Microsoft\\Windows\\CurrentVersion\\PrecisionTouchPad";
static const WCHAR *tp_names[2]={L"ThreeFingerTapEnabled",L"ThreeFingerSlideEnabled"};
static TP_STATE tp_state;
/* Keep the action result separate from fresh readback, which resets tp_state. */
static DWORD tp_prepare_error;
static int tp_prepare_stage,tp_rollback_ok=1;
static int tp_build(void){
    typedef struct {DWORD size,major,minor,build,platform;WCHAR service[128];} TP_VERSION;
    LONG (WINAPI *version)(TP_VERSION *);TP_VERSION data;HMODULE library=GetModuleHandleW(L"ntdll.dll");
    version=(void*)GetProcAddress(library,"RtlGetVersion");memset(&data,0,sizeof(data));data.size=sizeof(data);
    return version&&version(&data)==0?(int)data.build:0;
}
static void tp_close(TP_SESSION *session){
    if(session->cache)session->cache->vtable->release(session->cache);
    if(session->initialized&&session->uninit)session->uninit();
    if(session->library)FreeLibrary(session->library);memset(session,0,sizeof(*session));
}
static int tp_open(TP_SESSION *session){
    HRESULT (WINAPI *initialize)(void *,DWORD);HRESULT (WINAPI *create)(const GUID *,void *,DWORD,const GUID *,void **);
    TP_PROVIDER *provider=0;HRESULT hr;
    const GUID shell={0xC2F03A33,0x21F5,0x47FA,{0xB4,0xBB,0x15,0x63,0x62,0xA2,0xF2,0x39}};
    const GUID provider_id={0x6D5140C1,0x7436,0x11CE,{0x80,0x34,0x00,0xAA,0x00,0x60,0x09,0xFA}};
    const GUID service_id={0x53660488,0x8855,0x460B,{0xA9,0xAB,0x5C,0xFC,0x6B,0x50,0x12,0xCA}};
    const GUID cache_id={0x4214F6FA,0xEB36,0x4E2F,{0x9C,0xA2,0x23,0xFD,0xC1,0x83,0x2D,0xF7}};
    memset(session,0,sizeof(*session));
    /* This private shell interface is verified on these builds only. Fail closed elsewhere. */
    if(tp_build()<22621||tp_build()>26200)return 0;
    session->library=LoadLibraryExW(L"ole32.dll",0,0x00000800);if(!session->library)return 0;
    initialize=(void*)GetProcAddress(session->library,"CoInitializeEx");session->uninit=(void*)GetProcAddress(session->library,"CoUninitialize");create=(void*)GetProcAddress(session->library,"CoCreateInstance");
    if(!initialize||!session->uninit||!create)goto fail;
    hr=initialize(0,2);if(hr<0&&hr!=(HRESULT)0x80010106){tp_state.error=(DWORD)hr;goto fail;}session->initialized=hr>=0;
    hr=create(&shell,0,23,&provider_id,(void**)&provider);if(hr<0||!provider){tp_state.error=(DWORD)hr;goto fail;}
    hr=provider->vtable->service(provider,&service_id,&cache_id,(void**)&session->cache);provider->vtable->release(provider);
    if(hr<0||!session->cache){tp_state.error=(DWORD)hr;goto fail;}return 1;
fail:tp_close(session);return 0;
}
static int tp_cache_read(TP_SESSION *session,DWORD *tap,DWORD *swipe,DWORD *four_tap,DWORD *four_swipe){
    return session->cache->vtable->get_dword(session->cache,15,tap)==0&&session->cache->vtable->get_dword(session->cache,17,swipe)==0&&session->cache->vtable->get_dword(session->cache,16,four_tap)==0&&session->cache->vtable->get_dword(session->cache,18,four_swipe)==0;
}
static int tp_native_read(TP_PARAMS *data){memset(data,0,sizeof(*data));data->version=1;return SystemParametersInfoW(0xAE,sizeof(*data),data,0)!=0;}
static int tp_ready(const TP_STATE *state){return state->present&&state->enabled&&state->cache_ok&&state->registry_ok&&!state->tap&&!state->swipe&&!state->registry_tap&&!state->registry_swipe;}
static void tp_check(void){
    TP_SESSION session;HKEY key;TP_REG_VALUE values[2];memset(&tp_state,0,sizeof(tp_state));tp_state.build=tp_build();
    tp_state.native_ok=tp_native_read(&tp_state.native);if(!tp_state.native_ok){tp_state.error=GetLastError();return;}
    tp_state.present=(tp_state.native.status&1)!=0&&tp_state.native.contacts>=3;tp_state.enabled=(tp_state.native.status&8)!=0;tp_state.tap_drag=(tp_state.native.user&8)!=0;
    if(tp_open(&session)){tp_state.cache_ok=tp_cache_read(&session,&tp_state.tap,&tp_state.swipe,&tp_state.four_tap,&tp_state.four_swipe);tp_close(&session);}
    if(RegOpenKeyExW(HKEY_CURRENT_USER,tp_key_name,0,KEY_QUERY_VALUE,&key)==ERROR_SUCCESS){tp_state.registry_ok=tp_reg_read(key,0,&values[0])&&tp_reg_read(key,1,&values[1]);if(tp_state.registry_ok){tp_state.registry_tap=values[0].value;tp_state.registry_swipe=values[1].value;}RegCloseKey(key);}
    /* Tap-and-drag is independent; the user verified that leaving it enabled works. */
    tp_state.ready=tp_ready(&tp_state);
}
static int tp_reg_read(HKEY key,int item,TP_REG_VALUE *value){
    DWORD bytes=sizeof(DWORD);LONG result;memset(value,0,sizeof(*value));
    result=RegQueryValueExW(key,tp_names[item],0,&value->type,(BYTE*)&value->value,&bytes);
    if(result==ERROR_FILE_NOT_FOUND)return 1;
    if(result!=ERROR_SUCCESS||value->type!=REG_DWORD||bytes!=sizeof(DWORD))return 0;value->present=1;return 1;
}
static int tp_native_preserved(const TP_PARAMS *before,const TP_PARAMS *after){
    return after->version==before->version&&after->contacts==before->contacts&&after->legacy==before->legacy&&after->user==before->user&&after->sensitivity==before->sensitivity&&after->speed==before->speed&&after->feedback==before->feedback&&after->force==before->force&&after->right_width==before->right_width&&after->right_height==before->right_height;
}
static int tp_write_number(const WCHAR *file,const WCHAR *key,DWORD value){WCHAR number[32];_snwprintf(number,32,L"%lu",value);return WritePrivateProfileStringW(L"Original",key,number,file)!=0;}
static int tp_backup(const TP_STATE *before,const TP_REG_VALUE *values){
    WCHAR file[MAX_PATH];_snwprintf(file,MAX_PATH,L"%s\\touchpad-setup-backup.ini",data_folder);file[MAX_PATH-1]=0;
    /* Retain the user's first original setup across repeated repairs. */
    if(GetFileAttributesW(file)!=INVALID_FILE_ATTRIBUTES)return GetPrivateProfileIntW(L"Original",L"Version",0,file)==1;
    if(!tp_write_number(file,L"TapPresent",values[0].present)||!tp_write_number(file,L"TapValue",values[0].value)||!tp_write_number(file,L"SwipePresent",values[1].present)||!tp_write_number(file,L"SwipeValue",values[1].value)||!tp_write_number(file,L"UserFlags",before->native.user)||!tp_write_number(file,L"ThreeTap",before->tap)||!tp_write_number(file,L"ThreeSwipe",before->swipe)||!tp_write_number(file,L"FourTap",before->four_tap)||!tp_write_number(file,L"FourSwipe",before->four_swipe)||!tp_write_number(file,L"CursorSpeed",before->native.speed)||!tp_write_number(file,L"Sensitivity",before->native.sensitivity)||!tp_write_number(file,L"Version",1))return 0;
    return GetPrivateProfileIntW(L"Original",L"Version",0,file)==1&&GetPrivateProfileIntW(L"Original",L"UserFlags",0,file)==before->native.user;
}
static int tp_prepare(void){
    TP_SESSION session;TP_STATE before;TP_PARAMS after;TP_REG_VALUE values[2];HKEY key=0;DWORD zero=0,tap,swipe,four_tap,four_swipe;int wrote_tap=0,wrote_swipe=0,ok=0;LONG registry_result;HRESULT result;
    tp_prepare_error=0;tp_prepare_stage=0;tp_rollback_ok=1;
    tp_check();if(tp_state.ready)return 1;before=tp_state;
    tp_prepare_stage=1;if(!before.native_ok||!before.present||!before.enabled||!before.cache_ok||!tp_open(&session)){tp_prepare_error=tp_state.error?tp_state.error:ERROR_NOT_SUPPORTED;return 0;}
    tp_prepare_stage=2;registry_result=RegOpenKeyExW(HKEY_CURRENT_USER,tp_key_name,0,KEY_QUERY_VALUE|KEY_SET_VALUE,&key);if(registry_result!=ERROR_SUCCESS){tp_prepare_error=registry_result;goto finish;}
    if(!tp_reg_read(key,0,&values[0])||!tp_reg_read(key,1,&values[1])||!tp_backup(&before,values)){tp_prepare_error=ERROR_INVALID_DATA;goto finish;}
    tp_prepare_stage=3;registry_result=RegSetValueExW(key,tp_names[0],0,REG_DWORD,(BYTE*)&zero,sizeof(zero));if(registry_result!=ERROR_SUCCESS){tp_prepare_error=registry_result;goto finish;}wrote_tap=1;
    registry_result=RegSetValueExW(key,tp_names[1],0,REG_DWORD,(BYTE*)&zero,sizeof(zero));if(registry_result!=ERROR_SUCCESS){tp_prepare_error=registry_result;goto rollback;}wrote_swipe=1;
    tp_prepare_stage=4;result=session.cache->vtable->notify(session.cache,15);if(result>=0)result=session.cache->vtable->notify(session.cache,17);if(result<0){tp_prepare_error=(DWORD)result;goto rollback;}
    tp_prepare_stage=6;
    if(!tp_native_read(&after)||!tp_native_preserved(&before.native,&after)||!tp_cache_read(&session,&tap,&swipe,&four_tap,&four_swipe)||tap||swipe||four_tap!=before.four_tap||four_swipe!=before.four_swipe){tp_prepare_error=ERROR_RETRY;goto rollback;}
    {TP_REG_VALUE readback[2];if(!tp_reg_read(key,0,&readback[0])||!tp_reg_read(key,1,&readback[1])||!readback[0].present||!readback[1].present||readback[0].value||readback[1].value)goto rollback;}
    ok=1;goto finish;
rollback:
    if(!tp_prepare_error)tp_prepare_error=ERROR_INVALID_DATA;
    if(wrote_tap){registry_result=values[0].present?RegSetValueExW(key,tp_names[0],0,REG_DWORD,(BYTE*)&values[0].value,sizeof(DWORD)):RegDeleteValueW(key,tp_names[0]);if(registry_result!=ERROR_SUCCESS)tp_rollback_ok=0;}
    if(wrote_swipe){registry_result=values[1].present?RegSetValueExW(key,tp_names[1],0,REG_DWORD,(BYTE*)&values[1].value,sizeof(DWORD)):RegDeleteValueW(key,tp_names[1]);if(registry_result!=ERROR_SUCCESS)tp_rollback_ok=0;}
    if(session.cache->vtable->notify(session.cache,15)<0||session.cache->vtable->notify(session.cache,17)<0)tp_rollback_ok=0;
    {TP_REG_VALUE restored[2];if(!tp_reg_read(key,0,&restored[0])||!tp_reg_read(key,1,&restored[1])||restored[0].present!=values[0].present||restored[1].present!=values[1].present||restored[0].value!=values[0].value||restored[1].value!=values[1].value)tp_rollback_ok=0;}
finish:
    if(key)RegCloseKey(key);tp_close(&session);tp_check();
    if(!ok&&(wrote_tap||wrote_swipe)&&(!tp_state.native_ok||!tp_native_preserved(&before.native,&tp_state.native)||!tp_state.cache_ok||tp_state.tap!=before.tap||tp_state.swipe!=before.swipe||tp_state.four_tap!=before.four_tap||tp_state.four_swipe!=before.four_swipe))tp_rollback_ok=0;
    if(ok&&tp_state.ready)tp_prepare_stage=0;return ok&&tp_state.ready;
}
static int tp_report(int applied){
    WCHAR path[MAX_PATH];char data[1536];DWORD count,written;HANDLE file;
    _snwprintf(path,MAX_PATH,L"%s\\touchpad-setup-status.json",data_folder);path[MAX_PATH-1]=0;
    _snprintf(data,sizeof(data),"{\"ProcessId\":%lu,\"Build\":%lu,\"Ready\":%s,\"NativeApiAvailable\":%s,\"CacheVerified\":%s,\"PrecisionTouchpadPresent\":%s,\"TouchpadEnabled\":%s,\"ThreeFingerTap\":%lu,\"ThreeFingerSwipe\":%lu,\"RegistryTap\":%lu,\"RegistrySwipe\":%lu,\"TapAndDrag\":%s,\"FourFingerTap\":%lu,\"FourFingerSwipe\":%lu,\"CursorSpeed\":%lu,\"Sensitivity\":%lu,\"UserFlags\":%lu,\"Applied\":%s,\"Error\":%lu,\"PreparationStage\":%d,\"PreparationError\":%lu,\"RollbackVerified\":%s}",GetCurrentProcessId(),tp_state.build,tp_state.ready?"true":"false",tp_state.native_ok?"true":"false",tp_state.cache_ok?"true":"false",tp_state.present?"true":"false",tp_state.enabled?"true":"false",tp_state.tap,tp_state.swipe,tp_state.registry_tap,tp_state.registry_swipe,tp_state.tap_drag?"true":"false",tp_state.four_tap,tp_state.four_swipe,tp_state.native.speed,tp_state.native.sensitivity,tp_state.native.user,applied?"true":"false",tp_state.error,tp_prepare_stage,tp_prepare_error,tp_rollback_ok?"true":"false");data[sizeof(data)-1]=0;count=(DWORD)strlen(data);
    file=CreateFileW(path,GENERIC_WRITE,FILE_SHARE_READ,0,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,0);if(file==INVALID_HANDLE_VALUE)return 0;
    written=0;WriteFile(file,data,count,&written,0);CloseHandle(file);return written==count;
}
static int tp_result_number(const char *data,const char *name,DWORD *value){
    char pattern[80];const char *position;_snprintf(pattern,sizeof(pattern),"\"%s\":",name);pattern[sizeof(pattern)-1]=0;
    position=strstr(data,pattern);return position&&sscanf(position+strlen(pattern),"%lu",value)==1;
}
static int tp_previous_failure(void){
    WCHAR path[MAX_PATH];char data[1536];HANDLE file;DWORD count,build,tap,swipe,registry_tap,registry_swipe,user,stage,error;
    if(tp_state.ready||!tp_state.native_ok||!tp_state.present||!tp_state.enabled)return 0;
    _snwprintf(path,MAX_PATH,L"%s\\touchpad-setup-status.json",data_folder);path[MAX_PATH-1]=0;
    file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,0,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);if(file==INVALID_HANDLE_VALUE)return 0;
    count=0;if(!ReadFile(file,data,sizeof(data)-1,&count,0)){CloseHandle(file);return 0;}CloseHandle(file);data[count]=0;
    if(!strstr(data,"\"Ready\":false")||!tp_result_number(data,"Build",&build)||!tp_result_number(data,"ThreeFingerTap",&tap)||!tp_result_number(data,"ThreeFingerSwipe",&swipe)||!tp_result_number(data,"RegistryTap",&registry_tap)||!tp_result_number(data,"RegistrySwipe",&registry_swipe)||!tp_result_number(data,"UserFlags",&user)||!tp_result_number(data,"PreparationStage",&stage)||!tp_result_number(data,"PreparationError",&error))return 0;
    /* Reuse the last explicit action result only while its input settings still match. */
    if(!stage||!error||build!=tp_state.build||tap!=tp_state.tap||swipe!=tp_state.swipe||registry_tap!=tp_state.registry_tap||registry_swipe!=tp_state.registry_swipe||user!=tp_state.native.user)return 0;
    tp_prepare_stage=(int)stage;tp_prepare_error=error;tp_rollback_ok=strstr(data,"\"RollbackVerified\":true")!=0;return 1;
}
#endif
