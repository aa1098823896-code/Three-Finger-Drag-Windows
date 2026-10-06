/* Privacy regression: fictitious identity fixtures and a captured clipboard.
   Reuses the application copy function; the user's clipboard is untouched. */
#define UNICODE
#define _UNICODE
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include "app_shared.h"
#include "ui_graphics.h"
#include "touchpad_prepare.h"
static WCHAR captured_prompt[4096];static HANDLE captured_memory;static int clipboard_locked;
static int prompt_native_readable=1,prompt_cache_readable=1;
static BOOL WINAPI prompt_open(HWND hwnd){return !clipboard_locked;}
static BOOL WINAPI prompt_empty(void){return TRUE;}
static BOOL WINAPI prompt_close(void){return TRUE;}
static HANDLE WINAPI prompt_set(UINT format,HANDLE memory){
    WCHAR *value=GlobalLock(memory);if(!value)return 0;
    wcsncpy(captured_prompt,value,4095);captured_prompt[4095]=0;
    GlobalUnlock(memory);captured_memory=memory;return memory;
}
static BOOL WINAPI prompt_invalidate(HWND hwnd,const RECT *rect,BOOL erase){return TRUE;}
static void prompt_check(void){
    memset(&tp_state,0,sizeof(tp_state));
    tp_state.native_ok=tp_state.present=tp_state.enabled=tp_state.cache_ok=tp_state.registry_ok=1;
    tp_state.build=26200;tp_state.native.contacts=5;tp_state.swipe=1;
    tp_state.tap_drag=1;tp_state.native.speed=16;tp_state.native.sensitivity=2;
    tp_state.native_ok=prompt_native_readable;tp_state.cache_ok=prompt_cache_readable;
}
static int prompt_report(int applied){return 1;}
#define OpenClipboard prompt_open
#define EmptyClipboard prompt_empty
#define CloseClipboard prompt_close
#define SetClipboardData prompt_set
#define InvalidateRect prompt_invalidate
#define tp_check prompt_check
#define tp_report prompt_report
#define WinMain settings_main
#include "settings_ui.c"
#undef WinMain
int WINAPI WinMain(HINSTANCE instance,HINSTANCE previous,LPSTR command,int show){
    WCHAR first[4096];int passed,privacy,generic,scope,independent,failure,unknown,locale,all_languages=1,max_translated=0;FILE *f;
    app_language=APP_LANG_ZH_CN;app_language_initialized=1;
    /* Paths are test fixtures, not a real person's account or installation. */
    wcscpy(data_folder,L"C:\\Users\\PrivacyProbeLocal\\AppData\\Local\\ThreeFingerDragNative");
    wcscpy(app_folder,L"D:\\PrivateInstallProbe\\GestureTools");
    wcscpy(config_path,L"C:\\PrivacyProbeMissing\\settings.ini");
    isolated=0;test_no_host=setup_validation=1;setup_copy_ai();
    if(setup_copy_result!=1)return 2;wcscpy(first,captured_prompt);
    privacy=!wcsstr(first,L"PrivacyProbeLocal")&&!wcsstr(first,L"PrivateInstallProbe")
        &&!wcsstr(first,L":\\")&&!wcsstr(first,data_folder)&&!wcsstr(first,app_folder);
    generic=!wcsstr(first,L"LOCALAPPDATA")&&!wcsstr(first,L"GestureSettings.exe")&&!wcsstr(first,L"build")
        &&!wcsstr(first,L"注册表")&&wcslen(first)<=300;
    scope=wcsstr(first,L"把三指轻扫和三指点击都设为「无」")!=0&&wcsstr(first,L"不要关闭双击拖动")!=0
        &&wcsstr(first,L"不要改系统指针速度")!=0&&wcsstr(first,L"已经设置好的就不重复修改")!=0
        &&wcsstr(first,L"修改前记下原来的值")!=0&&wcsstr(first,L"再看一次系统设置")!=0
        &&wcsstr(first,L"如果你不能操作电脑，请直接告诉我")!=0
        &&wcsstr(first,L"三指轻扫还没设为「无」，三指点击已设为「无」")!=0;
    if(captured_memory){GlobalFree(captured_memory);captured_memory=0;}
    wcscpy(data_folder,L"C:\\Users\\DifferentPrivacyProbe\\AppData\\Local\\ThreeFingerDragNative");
    wcscpy(app_folder,L"E:\\AnotherPrivateInstallProbe");setup_copy_ai();
    independent=setup_copy_result==1&&wcscmp(first,captured_prompt)==0;
    if(captured_memory){GlobalFree(captured_memory);captured_memory=0;}
    clipboard_locked=1;setup_copy_ai();
    failure=setup_copy_result==2&&!wcsstr(setup_notice,L"已复制")&&wcscmp(first,captured_prompt)==0;
    clipboard_locked=0;prompt_native_readable=prompt_cache_readable=0;setup_copy_ai();
    unknown=setup_copy_result==1&&wcsstr(captured_prompt,L"软件暂时无法确认这两项设置")!=0
        &&!wcsstr(captured_prompt,L"三指轻扫已设为「无」")&&wcslen(captured_prompt)<=300;
    if(captured_memory){GlobalFree(captured_memory);captured_memory=0;}
    passed=privacy&&generic&&scope&&independent&&failure&&unknown;
    prompt_native_readable=prompt_cache_readable=1;
    for(locale=0;locale<9;locale++){
        app_language=locale;app_language_initialized=1;setup_copy_ai();
        if(setup_copy_result!=1||!wcsstr(captured_prompt,app_text(APP_TEXT_PROMPT_NOT_READY))||wcsstr(captured_prompt,L":\\")||wcsstr(captured_prompt,L"PrivacyProbe")||wcsstr(captured_prompt,L"PrivateInstallProbe"))all_languages=0;
        if(wcslen(captured_prompt)>max_translated)max_translated=(int)wcslen(captured_prompt);
        if(captured_memory){GlobalFree(captured_memory);captured_memory=0;}
    }passed&=all_languages;
    f=fopen("ai-prompt-privacy.json","wb");if(!f)return 3;
    fprintf(f,"{\"Passed\":%s,\"CapturedClipboardOnly\":true,\"UserClipboardUntouched\":true,\"NoPersonalPaths\":%s,\"NoPathNeeded\":%s,\"TwoSettingsScopePreserved\":%s,\"DifferentLocalPathsProduceSamePrompt\":%s,\"ClipboardFailureDoesNotClaimSuccess\":%s,\"UnavailableDetectionShownAsUnknown\":%s,\"PromptCharacters\":%lu,\"LanguagesChecked\":9,\"AllLanguagePromptsPrivate\":%s,\"MaximumTranslatedPromptCharacters\":%d,\"RealInputCalls\":0}",passed?"true":"false",privacy?"true":"false",generic?"true":"false",scope?"true":"false",independent?"true":"false",failure?"true":"false",unknown?"true":"false",(unsigned long)wcslen(first),all_languages?"true":"false",max_translated);
    fclose(f);return passed?0:1;
}
