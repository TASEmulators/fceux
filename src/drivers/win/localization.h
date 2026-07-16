#ifndef WIN_LOCALIZATION_H
#define WIN_LOCALIZATION_H

#include <windows.h>
#include <string>

enum Win32UILanguage
{
	WIN32_UI_LANGUAGE_AUTO = 0,
	WIN32_UI_LANGUAGE_EN_US,
	WIN32_UI_LANGUAGE_ZH_CN
};

extern char* Win32UILanguageSetting;

void Win32Localization_Init();
void Win32Localization_SetConfiguredLanguage(Win32UILanguage language);
Win32UILanguage Win32Localization_GetConfiguredLanguage();
Win32UILanguage Win32Localization_GetEffectiveLanguage();
bool Win32Localization_IsChinese();

std::wstring Win32Localization_LoadStringW(UINT id);
std::wstring Win32Localization_LoadStringW(UINT id, const wchar_t* fallback);
std::wstring Win32Localization_FormatStringW(UINT id, const wchar_t* fallback, ...);
std::wstring Win32Localization_LoadFilterW(UINT id, const wchar_t* fallback);
std::wstring Win32Localization_LocalizeText(const char* text);

int FCEU_MessageBoxResource(HWND hwnd, UINT textId, UINT captionId, UINT type);
BOOL FCEU_SetWindowTextResource(HWND hwnd, UINT textId);
BOOL FCEU_SetDlgItemTextResource(HWND hwnd, int id, UINT textId);
int FCEU_MessageBox(HWND hwnd, LPCSTR text, LPCSTR caption, UINT type);
BOOL FCEU_SetWindowText(HWND hwnd, LPCSTR text);
BOOL FCEU_SetDlgItemText(HWND hwnd, int id, LPCSTR text);

#ifndef WIN_LOCALIZATION_IMPLEMENTATION
#ifdef MessageBox
#undef MessageBox
#endif
#ifdef SetWindowText
#undef SetWindowText
#endif
#ifdef SetDlgItemText
#undef SetDlgItemText
#endif
#define MessageBox FCEU_MessageBox
#define SetWindowText FCEU_SetWindowText
#define SetDlgItemText FCEU_SetDlgItemText
#endif

#endif
