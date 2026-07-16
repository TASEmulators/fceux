#define WIN_LOCALIZATION_IMPLEMENTATION
#include "localization.h"

#include "resource.h"

#include <algorithm>
#include <cstdlib>
#include <cwchar>
#include <cstring>
#include <map>
#include <string>

char* Win32UILanguageSetting = NULL;

static Win32UILanguage g_configuredLanguage = WIN32_UI_LANGUAGE_AUTO;
static Win32UILanguage g_effectiveLanguage = WIN32_UI_LANGUAGE_EN_US;

static const char* kLanguageAuto = "auto";
static const char* kLanguageEnglish = "en-US";
static const char* kLanguageChinese = "zh-CN";

static bool equalsIgnoreCase(const char* a, const char* b)
{
	if (!a || !b)
		return false;
	while (*a && *b)
	{
		char ca = *a;
		char cb = *b;
		if (ca >= 'A' && ca <= 'Z')
			ca = ca - 'A' + 'a';
		if (cb >= 'A' && cb <= 'Z')
			cb = cb - 'A' + 'a';
		if (ca != cb)
			return false;
		++a;
		++b;
	}
	return *a == 0 && *b == 0;
}

static Win32UILanguage parseConfiguredLanguage(const char* value)
{
	if (!value || !*value || equalsIgnoreCase(value, kLanguageAuto))
		return WIN32_UI_LANGUAGE_AUTO;
	if (equalsIgnoreCase(value, kLanguageEnglish) || equalsIgnoreCase(value, "english"))
		return WIN32_UI_LANGUAGE_EN_US;
	if (equalsIgnoreCase(value, kLanguageChinese) || equalsIgnoreCase(value, "zh_CN") || equalsIgnoreCase(value, "chinese"))
		return WIN32_UI_LANGUAGE_ZH_CN;
	return WIN32_UI_LANGUAGE_AUTO;
}

static const char* languageToConfigValue(Win32UILanguage language)
{
	switch (language)
	{
	case WIN32_UI_LANGUAGE_EN_US:
		return kLanguageEnglish;
	case WIN32_UI_LANGUAGE_ZH_CN:
		return kLanguageChinese;
	case WIN32_UI_LANGUAGE_AUTO:
	default:
		return kLanguageAuto;
	}
}

static bool isSimplifiedChineseUILanguage()
{
	LANGID langid = LANGIDFROMLCID(GetUserDefaultLCID());
	WORD primary = PRIMARYLANGID(langid);
	WORD sublang = SUBLANGID(langid);

	if (primary != LANG_CHINESE)
		return false;

	if (sublang == SUBLANG_CHINESE_TRADITIONAL ||
		sublang == SUBLANG_CHINESE_HONGKONG ||
		sublang == SUBLANG_CHINESE_MACAU)
		return false;

	return true;
}

static LANGID languageToLangId(Win32UILanguage language)
{
	return language == WIN32_UI_LANGUAGE_ZH_CN ?
		MAKELANGID(LANG_CHINESE, SUBLANG_CHINESE_SIMPLIFIED) :
		MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US);
}

static void applyThreadLanguage(Win32UILanguage language)
{
	SetThreadUILanguage(languageToLangId(language));
}

static std::wstring multibyteToWide(const char* text)
{
	if (!text)
		return std::wstring();

	int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text, -1, NULL, 0);
	UINT codePage = CP_UTF8;
	DWORD flags = MB_ERR_INVALID_CHARS;
	if (count <= 0)
	{
		codePage = CP_ACP;
		flags = 0;
		count = MultiByteToWideChar(codePage, flags, text, -1, NULL, 0);
	}
	if (count <= 0)
		return std::wstring(L"[text conversion failed]");

	std::wstring result(count - 1, L'\0');
	MultiByteToWideChar(codePage, flags, text, -1, &result[0], count);
	return result;
}

static const std::map<std::string, UINT>& dynamicStringMap()
{
	static std::map<std::string, UINT> strings;
	if (!strings.empty())
		return strings;

	strings["Exit FCEUX?"] = IDS_LOC_EXIT_FCEUX_TITLE;
	strings["File error"] = IDS_LOC_FILE_ERROR_TITLE;
	strings["Error"] = IDS_LOC_ERROR_TITLE;
	strings["Cheat warning"] = IDS_LOC_CHEAT_WARNING_TITLE;
	strings["Cheat Console"] = IDS_LOC_CHEAT_CONSOLE_TITLE;
	strings["Save cheats?"] = IDS_LOC_SAVE_CHEATS_PROMPT;
	strings["Error saving cheats!"] = IDS_LOC_ERROR_SAVING_CHEATS;
	strings["Remove from list?"] = IDS_LOC_REMOVE_FROM_LIST_PROMPT;
	strings["Could Not Open Recent File"] = IDS_LOC_RECENT_FILE_ERROR_TITLE;
	strings["Could Not Open Recent Project"] = IDS_LOC_RECENT_PROJECT_ERROR_TITLE;
	strings["Open ROM"] = IDS_LOC_OPEN_ROM_TITLE;
	strings["Press a key or a button"] = IDS_LOC_PRESS_KEY_OR_BUTTON;
	strings["Unable to Pause Code/Data Logger"] = IDS_LOC_UNABLE_PAUSE_CDLOGGER_TITLE;
	strings["The Trace Logger is currently using this for some of its features.\nPlease turn the Trace Logger off and try again."] = IDS_LOC_TRACE_LOGGER_USING_CDLOGGER;
	strings["Invalid breakpoint condition"] = IDS_LOC_INVALID_BREAKPOINT_CONDITION;
	strings["Too many breakpoints, please delete one and try again"] = IDS_LOC_TOO_MANY_BREAKPOINTS;
	strings["Breakpoint Error"] = IDS_LOC_BREAKPOINT_ERROR_TITLE;
	strings["Restore default colors"] = IDS_LOC_RESTORE_DEFAULT_COLORS_TITLE;
	strings["Do you want to restore all the colors to default?"] = IDS_LOC_RESTORE_DEFAULT_COLORS_PROMPT;
	strings["Step Out Already Active"] = IDS_LOC_STEP_OUT_ACTIVE_TITLE;
	strings["Step Out is currently in process. Cancel it and setup a new Step Out watch?"] = IDS_LOC_STEP_OUT_ACTIVE_PROMPT;
	strings["Step Over Already Active"] = IDS_LOC_STEP_OVER_ACTIVE_TITLE;
	strings["Step Over is currently in process. Cancel it and setup a new Step Over watch?"] = IDS_LOC_STEP_OVER_ACTIVE_PROMPT;
	strings["The new PPU doesn't support overclocking, it will be disabled. Do you want to continue?"] = IDS_LOC_NEW_PPU_OVERCLOCK_PROMPT;
	strings["Overclocking"] = IDS_LOC_OVERCLOCKING_TITLE;
	strings["Nothing was found."] = IDS_LOC_NOTHING_FOUND_DOT;
	strings["Nothing was found!"] = IDS_LOC_NOTHING_FOUND_BANG;
	strings["Find Note"] = IDS_LOC_FIND_NOTE_TITLE;
	strings["Find Similar Note"] = IDS_LOC_FIND_SIMILAR_NOTE_TITLE;
	strings["Marker Note under Playback cursor is empty!"] = IDS_LOC_MARKER_NOTE_EMPTY;
	strings["This project doesn't have any Markers!"] = IDS_LOC_NO_MARKERS;
	strings["Marker Note under Playback cursor doesn't have keywords!"] = IDS_LOC_MARKER_NOTE_NO_KEYWORDS;
	strings["Could not find more Notes similar to Marker Note under Playback cursor!"] = IDS_LOC_NO_MORE_SIMILAR_NOTES;
	strings["Could not find anything similar to Marker Note under Playback cursor!"] = IDS_LOC_NO_SIMILAR_NOTES;
	strings["ROM Checksum Mismatch"] = IDS_LOC_ROM_CHECKSUM_MISMATCH_TITLE;
	strings["FM3 Version Mismatch"] = IDS_LOC_FM3_VERSION_MISMATCH_TITLE;
	strings["Opening FM2 file"] = IDS_LOC_OPENING_FM2_TITLE;
	strings["Save changes to file?"] = IDS_LOC_SAVE_CHANGES_TITLE;
	strings["Unable to save changes to file"] = IDS_LOC_UNABLE_SAVE_CHANGES;
	strings["Error saving to file"] = IDS_LOC_ERROR_SAVING_FILE_TITLE;
	strings["Language change saved"] = IDS_LOC_LANGUAGE_RESTART_TITLE;
	strings["The interface language will be applied the next time FCEUX starts."] = IDS_LOC_LANGUAGE_RESTART_MESSAGE;
	return strings;
}

void Win32Localization_Init()
{
	g_configuredLanguage = parseConfiguredLanguage(Win32UILanguageSetting);
	if (g_configuredLanguage == WIN32_UI_LANGUAGE_AUTO)
		g_effectiveLanguage = isSimplifiedChineseUILanguage() ? WIN32_UI_LANGUAGE_ZH_CN : WIN32_UI_LANGUAGE_EN_US;
	else
		g_effectiveLanguage = g_configuredLanguage;

	applyThreadLanguage(g_effectiveLanguage);
}

void Win32Localization_SetConfiguredLanguage(Win32UILanguage language)
{
	g_configuredLanguage = language;
	if (Win32UILanguageSetting)
		free(Win32UILanguageSetting);
	Win32UILanguageSetting = _strdup(languageToConfigValue(language));
}

Win32UILanguage Win32Localization_GetConfiguredLanguage()
{
	return g_configuredLanguage;
}

Win32UILanguage Win32Localization_GetEffectiveLanguage()
{
	return g_effectiveLanguage;
}

bool Win32Localization_IsChinese()
{
	return g_effectiveLanguage == WIN32_UI_LANGUAGE_ZH_CN;
}

std::wstring Win32Localization_LoadStringW(UINT id)
{
	return Win32Localization_LoadStringW(id, NULL);
}

std::wstring Win32Localization_LoadStringW(UINT id, const wchar_t* fallback)
{
	wchar_t buffer[2048];
	int count = LoadStringW(GetModuleHandle(NULL), id, buffer, sizeof(buffer) / sizeof(buffer[0]));
	if (count > 0)
		return std::wstring(buffer, count);

	LANGID previous = SetThreadUILanguage(MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US));
	count = LoadStringW(GetModuleHandle(NULL), id, buffer, sizeof(buffer) / sizeof(buffer[0]));
	SetThreadUILanguage(previous);
	if (count > 0)
		return std::wstring(buffer, count);

	if (fallback)
		return fallback;

	wchar_t missing[64];
	swprintf(missing, sizeof(missing) / sizeof(missing[0]), L"[missing string %u]", id);
	return missing;
}

std::wstring Win32Localization_LocalizeText(const char* text)
{
	if (!text)
		return std::wstring();

	const std::map<std::string, UINT>& strings = dynamicStringMap();
	std::map<std::string, UINT>::const_iterator it = strings.find(text);
	if (it != strings.end())
		return Win32Localization_LoadStringW(it->second, multibyteToWide(text).c_str());

	return multibyteToWide(text);
}

int FCEU_MessageBox(HWND hwnd, LPCSTR text, LPCSTR caption, UINT type)
{
	std::wstring wideText = Win32Localization_LocalizeText(text);
	std::wstring wideCaption = Win32Localization_LocalizeText(caption);
	return MessageBoxW(hwnd, wideText.c_str(), wideCaption.c_str(), type);
}

BOOL FCEU_SetWindowText(HWND hwnd, LPCSTR text)
{
	std::wstring wideText = Win32Localization_LocalizeText(text);
	return SetWindowTextW(hwnd, wideText.c_str());
}

BOOL FCEU_SetDlgItemText(HWND hwnd, int id, LPCSTR text)
{
	std::wstring wideText = Win32Localization_LocalizeText(text);
	return SetDlgItemTextW(hwnd, id, wideText.c_str());
}
