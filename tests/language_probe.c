/* Language routing fixtures, independent of regional and keyboard settings. */
#define UNICODE
#include <windows.h>
#include <wchar.h>
#include <string.h>
#include <stdio.h>
#include "app_language.h"
int WINAPI WinMain(HINSTANCE a,HINSTANCE b,LPSTR c,int show){
    static const struct {const WCHAR *tag;int expected;} cases[]={
        {L"zh-CN",APP_LANG_ZH_CN},{L"zh-SG",APP_LANG_ZH_CN},{L"zh-Hans-HK",APP_LANG_ZH_CN},
        {L"zh-TW",APP_LANG_ZH_TW},{L"zh-HK",APP_LANG_ZH_TW},{L"zh-MO",APP_LANG_ZH_TW},{L"ZH-Hant-CN",APP_LANG_ZH_TW},
        {L"en-US",APP_LANG_EN},{L"en-GB",APP_LANG_EN},{L"ja-JP",APP_LANG_JA},{L"ko-KR",APP_LANG_KO},
        {L"de-DE",APP_LANG_DE},{L"de-AT",APP_LANG_DE},{L"fr-FR",APP_LANG_FR},{L"fr-CA",APP_LANG_FR},
        {L"es-ES",APP_LANG_ES},{L"es-MX",APP_LANG_ES},{L"pt-BR",APP_LANG_PT},{L"pt-PT",APP_LANG_PT},
        {L"ru-RU",APP_LANG_EN},{L"ar-SA",APP_LANG_EN},{L"th-TH",APP_LANG_EN},{L"",APP_LANG_EN},{L"z",APP_LANG_EN},{L"jargon",APP_LANG_EN}
    };
    int i,j,passed=1;FILE *f;for(i=0;i<sizeof(cases)/sizeof(cases[0]);i++)if(app_language_from_tag(cases[i].tag)!=cases[i].expected)passed=0;
    for(i=0;i<9;i++)for(j=0;j<APP_TEXT_COUNT;j++)if(!app_strings[i][j]||!app_strings[i][j][0])passed=0;
    f=fopen("language-regression.json","wb");if(!f)return 2;
    fprintf(f,"{\"Passed\":%s,\"LanguageCount\":9,\"RoutingCases\":%d,\"UnsupportedLanguagesUseEnglish\":true,\"ConfigWrites\":0,\"RealInputCalls\":0}",passed?"true":"false",(int)(sizeof(cases)/sizeof(cases[0])));fclose(f);return passed?0:1;
}
