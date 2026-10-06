#ifndef THREE_FINGER_APP_LANGUAGE_H
#define THREE_FINGER_APP_LANGUAGE_H
/* Selected once from Windows display language. Identifiers and config paths
   never depend on language. Unsupported display languages use English. */
enum APP_LANGUAGE {APP_LANG_ZH_CN,APP_LANG_ZH_TW,APP_LANG_EN,APP_LANG_JA,APP_LANG_KO,APP_LANG_DE,APP_LANG_FR,APP_LANG_ES,APP_LANG_PT};
static int app_language=APP_LANG_EN,app_language_initialized;
static int app_language_from_tag(const WCHAR *tag){
    int i;WCHAR value[85];for(i=0;tag[i]&&i<84;i++)value[i]=tag[i]>=L'A'&&tag[i]<=L'Z'?tag[i]+32:tag[i];value[i]=0;
    if(i<2)return APP_LANG_EN;
    if(!wcsncmp(value,L"zh",2)&&(value[2]==0||value[2]==L'-')){
        if(wcsstr(value,L"-hant"))return APP_LANG_ZH_TW;
        if(wcsstr(value,L"-hans"))return APP_LANG_ZH_CN;
        return wcsstr(value,L"-tw")||wcsstr(value,L"-hk")||wcsstr(value,L"-mo")?APP_LANG_ZH_TW:APP_LANG_ZH_CN;
    }
    if(value[2]!=0&&value[2]!=L'-')return APP_LANG_EN;
    if(!wcsncmp(value,L"ja",2))return APP_LANG_JA;if(!wcsncmp(value,L"ko",2))return APP_LANG_KO;
    if(!wcsncmp(value,L"de",2))return APP_LANG_DE;if(!wcsncmp(value,L"fr",2))return APP_LANG_FR;
    if(!wcsncmp(value,L"es",2))return APP_LANG_ES;if(!wcsncmp(value,L"pt",2))return APP_LANG_PT;return APP_LANG_EN;
}
static void app_language_init(void){
    WCHAR names[1024]={0};ULONG count=0,size=1024;HMODULE kernel;BOOL (WINAPI *preferred)(DWORD,PULONG,PWSTR,PULONG);
    if(app_language_initialized)return;app_language_initialized=1;kernel=GetModuleHandleW(L"kernel32.dll");
    preferred=(void*)GetProcAddress(kernel,"GetUserPreferredUILanguages");
    if(preferred&&preferred(8,&count,names,&size)&&count&&names[0]){app_language=app_language_from_tag(names);return;}
    {LANGID (WINAPI *current_ui)(void)=(void*)GetProcAddress(kernel,"GetUserDefaultUILanguage");LANGID id=current_ui?current_ui():0;switch(PRIMARYLANGID(id)){
        case LANG_CHINESE:app_language=id==0x0404||id==0x0C04||id==0x1404?APP_LANG_ZH_TW:APP_LANG_ZH_CN;break;
        case LANG_JAPANESE:app_language=APP_LANG_JA;break;case LANG_KOREAN:app_language=APP_LANG_KO;break;
        case LANG_GERMAN:app_language=APP_LANG_DE;break;case LANG_FRENCH:app_language=APP_LANG_FR;break;
        case LANG_SPANISH:app_language=APP_LANG_ES;break;case LANG_PORTUGUESE:app_language=APP_LANG_PT;break;default:app_language=APP_LANG_EN;
    }}
}
#include "generated/app_strings.h"
static const WCHAR *app_text(enum APP_TEXT_ID key){app_language_init();return app_strings[app_language][key];}
#ifndef APP_LANGUAGE_HOST
static const WCHAR *app_language_font(void){
    app_language_init();if(app_language==APP_LANG_ZH_CN)return L"Microsoft YaHei UI";
    if(app_language==APP_LANG_ZH_TW)return L"Microsoft JhengHei UI";
    if(app_language==APP_LANG_JA)return L"Yu Gothic UI";if(app_language==APP_LANG_KO)return L"Malgun Gothic";return L"Segoe UI";
}
/* Rendering-only override: never written to user configuration. */
static void app_language_preview(const char *command){
    const char *start=strstr(command,"--language=");WCHAR tag[85];int i;
    if(!start||(!strstr(command,"--render-")&&!strstr(command,"--hidden-check")&&!strstr(command,"--preview-tray")))return;
    start+=11;for(i=0;start[i]&&start[i]!=' '&&i<84;i++)tag[i]=(unsigned char)start[i];tag[i]=0;
    app_language=app_language_from_tag(tag);app_language_initialized=1;
}
#endif
#endif
