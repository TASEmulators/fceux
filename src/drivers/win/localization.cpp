#define WIN_LOCALIZATION_IMPLEMENTATION
#include "localization.h"

#include "resource.h"

#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cwchar>
#include <cstring>
#include <map>
#include <string>
#include <vector>

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
	strings["Could not locate 7z.dll"] = IDS_LOC_7Z_DLL_NOT_FOUND;
	strings["7z.dll was invalid"] = IDS_LOC_7Z_DLL_INVALID;
	strings["Failure reading archive file"] = IDS_LOC_ARCHIVE_READ_FAILURE_TITLE;
	strings["Failure launching archive browser"] = IDS_LOC_ARCHIVE_BROWSER_FAILURE_TITLE;
	strings["Failure converting fcm"] = IDS_LOC_FCM_CONVERT_FAILURE_TITLE;
	strings["FCM Conversion results"] = IDS_LOC_FCM_CONVERT_RESULTS_TITLE;
	strings["Failed to open file"] = IDS_LOC_FAILED_OPEN_FILE_TITLE;
	strings["File from second instance failed to open"] = IDS_LOC_SECOND_INSTANCE_OPEN_FAILED;
	strings["Select old movie(s) for conversion"] = IDS_LOC_SELECT_OLD_MOVIES_TITLE;
	strings["Save State As..."] = IDS_LOC_SAVE_STATE_AS_TITLE;
	strings["Load State From..."] = IDS_LOC_LOAD_STATE_FROM_TITLE;
	strings["Load Code Data Log File..."] = IDS_LOC_LOAD_CDL_TITLE;
	strings["Save Code Data Log File As..."] = IDS_LOC_SAVE_CDL_AS_TITLE;
	strings["Save Stripped File As..."] = IDS_LOC_SAVE_STRIPPED_FILE_AS_TITLE;
	strings["Play Movie from File"] = IDS_LOC_PLAY_MOVIE_FROM_FILE_TITLE;
	strings["Start"] = IDS_LOC_START;
	strings["Pause"] = IDS_LOC_PAUSE;
	strings["Stop Logging"] = IDS_LOC_STOP_LOGGING;
	strings["Start Logging"] = IDS_LOC_START_LOGGING;
	strings["Power-On"] = IDS_LOC_POWER_ON;
	strings["Soft-Reset"] = IDS_LOC_SOFT_RESET;
	strings["Savestate"] = IDS_LOC_SAVESTATE;
	strings["Browse..."] = IDS_LOC_BROWSE_ELLIPSIS;
	strings["Now"] = IDS_LOC_NOW;
	strings["SaveRam"] = IDS_LOC_SAVERAM;
	strings["On "] = IDS_LOC_ON_PADDED;
	strings["Off"] = IDS_LOC_OFF;
	strings["Disallowed"] = IDS_LOC_DISALLOWED_TITLE;
	strings["Sorry, you're not allowed to save optimized NSFs yet. Please don't optimize individual banks, as there are still some issues with several NSFs to be fixed, and it is easier to fix those issues with as much of the bank data intact as possible."] = IDS_LOC_OPTIMIZED_NSF_DISALLOWED;
	strings["Unable to Generate Stripped ROM. Get Something Logged and try again."] = IDS_LOC_STRIPPED_ROM_NO_LOG;
	strings["Multiple cheats selected. Continue with delete?"] = IDS_LOC_DELETE_MULTI_CHEATS_PROMPT;
	strings["Delete multiple cheats?"] = IDS_LOC_DELETE_MULTI_CHEATS_TITLE;
	strings["If this option is unchecked, you must manually save the cheats by yourself, or all the changes you made to the cheat list would be discarded silently without any asking once you close the game!\nDo you really want to do it in this way?"] = IDS_LOC_CHEAT_AUTOSAVE_WARNING;
	strings["Edit Breakpoint..."] = IDS_LOC_EDIT_BREAKPOINT_TITLE;
	strings["Inline Assembler"] = IDS_LOC_INLINE_ASSEMBLER_TITLE;
	strings["Inline Assembler  *Patches Applied*"] = IDS_LOC_INLINE_ASSEMBLER_PATCHED_TITLE;
	strings["Inline Assembler  *Syntax Error*"] = IDS_LOC_INLINE_ASSEMBLER_SYNTAX_TITLE;
	strings["Patch data cannot exceed address 0xFFFF"] = IDS_LOC_PATCH_ADDRESS_TOO_LARGE;
	strings["Address error"] = IDS_LOC_ADDRESS_ERROR_TITLE;
	strings["Sorry, NES Header editing isn't supported by this tool. If you want to edit the header, please use NES Header Editor"] = IDS_LOC_NES_HEADER_EDIT_UNSUPPORTED;
	strings["Error: .Nes offset outside of PRG rom"] = IDS_LOC_NES_OFFSET_OUTSIDE_PRG;
	strings["Error Saving"] = IDS_LOC_ERROR_SAVING_TITLE;
	strings["Default window size"] = IDS_LOC_DEFAULT_WINDOW_SIZE;
	strings["Double-click on any address to prompt Add Breakpoint."] = IDS_LOC_DOUBLE_CLICK_ADD_BREAKPOINT;
	strings["Leftclick = Inline Assembler. Midclick = Game Genie. Rightclick = Hexeditor."] = IDS_LOC_DEBUGGER_CLICK_HELP;
	strings["Sorry, The Patcher only works on INES rom images"] = IDS_LOC_PATCHER_INES_ONLY;
	strings["No Offset Selected"] = IDS_LOC_NO_OFFSET_SELECTED;
	strings["Not Currently Loaded in ROM for disassembly"] = IDS_LOC_NOT_LOADED_IN_ROM;
	strings["Invalid offset"] = IDS_LOC_INVALID_OFFSET;
	strings["Please select a bookmark from the list"] = IDS_LOC_SELECT_BOOKMARK_PROMPT;
	strings["Address out of range"] = IDS_LOC_ADDRESS_OUT_OF_RANGE_TITLE;
	strings["This address already have a bookmark"] = IDS_LOC_BOOKMARK_DUPLICATED_PROMPT;
	strings["Bookmark duplicated"] = IDS_LOC_BOOKMARK_DUPLICATED_TITLE;
	strings["Error: Couldn't create directory. Please choose a different directory."] = IDS_LOC_CREATE_DIRECTORY_FAILED;
	strings["NES Header Editor"] = IDS_LOC_NES_HEADER_EDITOR_TITLE;
	strings["Invalid NES header."] = IDS_LOC_INVALID_NES_HEADER;
	strings["Editing header of an FDS file is not supported."] = IDS_LOC_FDS_HEADER_UNSUPPORTED;
	strings["Editing header of a UNIF file is not supported."] = IDS_LOC_UNIF_HEADER_UNSUPPORTED;
	strings["Editing header of an NSF file is not supported."] = IDS_LOC_NSF_HEADER_UNSUPPORTED;
	strings["The mapper# you have entered is invalid. Please enter a decimal number or select an item from the dropdown list."] = IDS_LOC_INVALID_MAPPER_NUMBER;
	strings["The sub mapper# should less than 16 in iNES 2.0 format."] = IDS_LOC_SUBMAPPER_TOO_LARGE;
	strings["The sub mapper# you have entered is invalid. Please enter a decimal number."] = IDS_LOC_INVALID_SUBMAPPER_NUMBER;
	strings["Invalid PRG RAM size"] = IDS_LOC_INVALID_PRG_RAM_SIZE;
	strings["Invalid PRG NVRAM size"] = IDS_LOC_INVALID_PRG_NVRAM_SIZE;
	strings["Invalid CHR RAM size"] = IDS_LOC_INVALID_CHR_RAM_SIZE;
	strings["Invalid CHR NVRAM size"] = IDS_LOC_INVALID_CHR_NVRAM_SIZE;
	strings["Invalid VS System hardware type."] = IDS_LOC_INVALID_VS_HARDWARE;
	strings["Invalid VS System PPU type."] = IDS_LOC_INVALID_VS_PPU;
	strings["Invalid extend system type"] = IDS_LOC_INVALID_EXTEND_SYSTEM;
	strings["Invalid input device."] = IDS_LOC_INVALID_INPUT_DEVICE;
	strings["Current input configuration has been set as Preset 1."] = IDS_LOC_INPUT_PRESET1_SET;
	strings["Current input configuration has been set as Preset 2."] = IDS_LOC_INPUT_PRESET2_SET;
	strings["Current input configuration has been set as Preset 3."] = IDS_LOC_INPUT_PRESET3_SET;
	strings["You can't edit ROM header here, however you can use NES Header Editor to edit the header if it's an iNES format file."] = IDS_LOC_ROM_HEADER_EDIT_HINT;
	strings["Saving failed"] = IDS_LOC_SAVING_FAILED;
	strings["Load failed"] = IDS_LOC_LOAD_FAILED;
	strings["Can't set more than 64 bookmarks."] = IDS_LOC_TOO_MANY_BOOKMARKS;
	strings["Error adding bookmark."] = IDS_LOC_ADD_BOOKMARK_FAILED;
	strings["This address already has a bookmark."] = IDS_LOC_BOOKMARK_EXISTS;
	strings["Error editing bookmark."] = IDS_LOC_EDIT_BOOKMARK_FAILED;
	strings["This address doesn't have a bookmark."] = IDS_LOC_BOOKMARK_MISSING;
	strings["Error removing bookmark."] = IDS_LOC_REMOVE_BOOKMARK_FAILED;
	strings["Remove All Bookmarks?"] = IDS_LOC_REMOVE_ALL_BOOKMARKS_PROMPT;
	strings["Bookmarks"] = IDS_LOC_BOOKMARKS_TITLE;
	strings["Error saving bookmarks."] = IDS_LOC_SAVE_BOOKMARKS_FAILED;
	strings["Error saving bookmarks"] = IDS_LOC_SAVE_BOOKMARKS_TITLE;
	strings["All your existing bookmarks will be discarded after importing the new bookmarks! Do you want to continue?"] = IDS_LOC_IMPORT_BOOKMARK_DISCARD_PROMPT;
	strings["Bookmark Import"] = IDS_LOC_BOOKMARK_IMPORT_TITLE;
	strings["Bookmark conflict"] = IDS_LOC_BOOKMARK_CONFLICT_TITLE;
	strings["Loading Hex Editor bookmarks"] = IDS_LOC_LOADING_HEX_BOOKMARKS_TITLE;
	strings["An error occurred while loading bookmarks."] = IDS_LOC_LOAD_BOOKMARKS_FAILED;
	strings["This file is not a Hex Editor bookmark list."] = IDS_LOC_NOT_HEX_BOOKMARK_FILE;
	strings["Error opening bookmark file"] = IDS_LOC_OPEN_BOOKMARK_FILE_FAILED;
	strings["Invalid String"] = IDS_LOC_INVALID_STRING;
	strings["String Not Found"] = IDS_LOC_STRING_NOT_FOUND;
	strings["Save Changes?"] = IDS_LOC_SAVE_CHANGES_GENERIC;
	strings["Memory Watch Settings"] = IDS_LOC_MEMORY_WATCH_SETTINGS_TITLE;
	strings["Save"] = IDS_LOC_SAVE_TITLE;
	strings["Ram Watch"] = IDS_LOC_RAM_WATCH_TITLE;
	strings["Error opening file."] = IDS_LOC_OPEN_FILE_ERROR;
	strings["Type must be specified."] = IDS_LOC_TYPE_REQUIRED;
	strings["Size must be specified."] = IDS_LOC_SIZE_REQUIRED;
	strings["Only 1 byte is supported on binary format."] = IDS_LOC_BINARY_ONE_BYTE_ONLY;
	strings["You must enter an address."] = IDS_LOC_ADDRESS_REQUIRED;
	strings["Sorry, you can't add cheat to a separator."] = IDS_LOC_CANNOT_CHEAT_SEPARATOR;
	strings["Autosearch - out of results."] = IDS_LOC_AUTOSEARCH_OUT_TITLE;
	strings["Invalid or out-of-bound entered value."] = IDS_LOC_INVALID_OR_OOB_VALUE;
	strings["Resetting search."] = IDS_LOC_RESETTING_SEARCH;
	strings["Out of results."] = IDS_LOC_OUT_OF_RESULTS_TITLE;
	strings["Movie playing problem"] = IDS_LOC_MOVIE_PLAYING_PROBLEM_TITLE;
	strings["Movie recording problem"] = IDS_LOC_MOVIE_RECORDING_PROBLEM_TITLE;
	strings["Save Project changes?"] = IDS_LOC_SAVE_PROJECT_CHANGES;
	strings["TAS Editor"] = IDS_LOC_TAS_EDITOR_TITLE;
	strings["Imported movie has the same Input.\nNo changes were made."] = IDS_LOC_IMPORTED_MOVIE_SAME_INPUT;
	strings["Error line is..."] = IDS_LOC_ERROR_LINE_IS_TITLE;
	strings["Enter a name for the selection first."] = IDS_LOC_SELECTION_NAME_REQUIRED;
	strings["Choose a selection first"] = IDS_LOC_SELECTION_REQUIRED;
	strings["OH NO!"] = IDS_LOC_OH_NO_TITLE;
	strings["File successfully saved!"] = IDS_LOC_FILE_SAVED;
	strings["There was a problem saving the table file."] = IDS_LOC_SAVE_TABLE_FAILED;
	strings["Please enter both a Japanese phrase and an English phrase."] = IDS_LOC_JA_EN_PHRASES_REQUIRED;
	strings["Overclocking is when you speed up your CPU, not slow it down!"] = IDS_LOC_OVERCLOCK_SLOW_ERROR;
	strings["Overclocking doesn't work with new PPU!"] = IDS_LOC_OVERCLOCK_NEW_PPU_ERROR;
	strings["Start Code/Data Logger?"] = IDS_LOC_START_CDLOGGER_TITLE;
	strings["Die!"] = IDS_LOC_DIE_PROMPT;
	strings["I'm dead!"] = IDS_LOC_IM_DEAD_TITLE;
	strings["Invalid parameter \"line\" in function parseLine"] = IDS_LOC_INVALID_PARAM_LINE_PARSELINE;
	strings["Invalid parameter \"n\" in function parseLine"] = IDS_LOC_INVALID_PARAM_N_PARSELINE;
	strings["Invalid parameter \"lines\" in function parse"] = IDS_LOC_INVALID_PARAM_LINES_PARSE;
	strings["Invalid parameter \"filename\" in function parse"] = IDS_LOC_INVALID_PARAM_FILENAME_PARSE;
	strings["PRG ROM size must be multiple of 16KB in iNES 1.0"] = IDS_LOC_PRG_ROM_SIZE_MULTIPLE_16;
	strings["PRG ROM size exceeded the limit of iNES 1.0 (4080KB)."] = IDS_LOC_PRG_ROM_SIZE_LIMIT_1;
	strings["PRG ROM size you entered is too large to fit into a cartridge, by the way this is an NES emulator, not for XBOX360 or PlayStation2."] = IDS_LOC_PRG_ROM_SIZE_TOO_LARGE;
	strings["PRG RAM size exceeded the limit (4096KB)"] = IDS_LOC_PRG_RAM_LIMIT_4096;
	strings["PRG RAM size must be multiple of 8KB in iNES 1.0"] = IDS_LOC_PRG_RAM_MULTIPLE_8;
	strings["PRG RAM size exceeded the limit (2040KB)"] = IDS_LOC_PRG_RAM_LIMIT_2040;
	strings["PRG NVRAM size exceeded the limit (4096KB)"] = IDS_LOC_PRG_NVRAM_LIMIT_4096;
	strings["CHR ROM size must be multiple of 8KB in iNES 1.0"] = IDS_LOC_CHR_ROM_MULTIPLE_8;
	strings["CHR ROM size exceeded the limit of iNES 1.0 (2040KB)."] = IDS_LOC_CHR_ROM_LIMIT_2040;
	strings["CHR ROM size you entered cannot be fitted in iNES 2.0."] = IDS_LOC_CHR_ROM_CANNOT_FIT_INES2;
	strings["CHR RAM size exceeded the limit (4096KB)"] = IDS_LOC_CHR_RAM_LIMIT_4096;
	strings["CHR NVRAM size exceeded the limit (4096KB)"] = IDS_LOC_CHR_NVRAM_LIMIT_4096;
	strings["Invalid miscellanous ROM(s) count. If you don't know what value should be, we recommend to set it to 0."] = IDS_LOC_INVALID_MISC_ROM_COUNT;
	strings["Miscellanous ROM(s) count has exceeded the limit of iNES 2.0 (3)"] = IDS_LOC_MISC_ROM_COUNT_LIMIT;
	strings["FCEUX Error"] = IDS_LOC_FCEUX_ERROR_TITLE;
	strings["Sorry"] = IDS_LOC_SORRY_TITLE;
	strings["Error loading bookmarks"] = IDS_LOC_ERROR_LOADING_BOOKMARKS_TITLE;
	strings["Error: Invalid address was specified as parameter to findBookmark"] = IDS_LOC_INVALID_FIND_BOOKMARK_ADDRESS;
	strings["Invalid byte value"] = IDS_LOC_INVALID_BYTE_VALUE;
	strings["Choosing Retry will reset the search once and continue autosearching.\nChoose Ignore will reset the search whenever necessary and continue autosearching.\nChoosing Abort will reset the search once and stop autosearching."] = IDS_LOC_AUTOSEARCH_OUT_PROMPT;
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

std::wstring Win32Localization_FormatStringW(UINT id, const wchar_t* fallback, ...)
{
	std::wstring format = Win32Localization_LoadStringW(id, fallback);
	va_list args;
	va_start(args, fallback);
	int count = _vscwprintf(format.c_str(), args);
	va_end(args);
	if (count < 0)
		return format;

	std::vector<wchar_t> buffer(count + 1);
	va_start(args, fallback);
	_vsnwprintf_s(&buffer[0], buffer.size(), _TRUNCATE, format.c_str(), args);
	va_end(args);
	return std::wstring(&buffer[0]);
}

std::wstring Win32Localization_LoadFilterW(UINT id, const wchar_t* fallback)
{
	std::wstring filter = Win32Localization_LoadStringW(id, fallback);
	std::replace(filter.begin(), filter.end(), L'|', L'\0');
	if (filter.empty() || filter[filter.size() - 1] != L'\0')
		filter.push_back(L'\0');
	if (filter.size() < 2 || filter[filter.size() - 2] != L'\0')
		filter.push_back(L'\0');
	return filter;
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

int FCEU_MessageBoxResource(HWND hwnd, UINT textId, UINT captionId, UINT type)
{
	std::wstring wideText = Win32Localization_LoadStringW(textId);
	std::wstring wideCaption = captionId ? Win32Localization_LoadStringW(captionId) : std::wstring();
	return MessageBoxW(hwnd, wideText.c_str(), wideCaption.c_str(), type);
}

BOOL FCEU_SetWindowTextResource(HWND hwnd, UINT textId)
{
	std::wstring wideText = Win32Localization_LoadStringW(textId);
	return SetWindowTextW(hwnd, wideText.c_str());
}

BOOL FCEU_SetDlgItemTextResource(HWND hwnd, int id, UINT textId)
{
	std::wstring wideText = Win32Localization_LoadStringW(textId);
	return SetDlgItemTextW(hwnd, id, wideText.c_str());
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
