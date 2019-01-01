#ifndef WIN_WINDOW_H
#define WIN_WINDOW_H

#include "common.h"
#include <string>

using namespace std;

// Type definitions

struct CreateMovieParameters
{
	std::string szFilename;				// on Dialog creation, this is the default filename to display.  On return, this is the filename that the user chose.
	int recordFrom;				// 0 = "Power-On", 1 = "Reset", 2 = "Now", 3+ = savestate file in szSavestateFilename
	std::string szSavestateFilename;
	std::wstring authorName;
};

extern char *recent_files[];
extern char *recent_lua[];
extern char *recent_movie[];
extern HWND pwindow;

HWND GetMainHWND();

void SetMainWindowText();
void HideFWindow(int h);
void SetMainWindowStuff();
int GetClientAbsRect(LPRECT lpRect);
void FixWXY(int pref, bool shift_held = false);
void ByebyeWindow();
void DoTimingConfigFix();
int CreateMainWindow();
void UpdateCheckedMenuItems();
void LoadNewGamey(HWND hParent, const char *initialdir);
int BrowseForFolder(HWND hParent, const char *htext, char *buf);
void SetMainWindowStuff();
void GetMouseData(uint32 (&md)[3]);
void GetMouseRelative(int32 (&md)[3]);
//void ChangeMenuItemText(int menuitem, string text);
void UpdateMenuHotkeys();

template<int BUFSIZE>
inline std::string GetDlgItemText(HWND hDlg, int nIDDlgItem) {
	char buf[BUFSIZE];
	GetDlgItemText(hDlg, nIDDlgItem, buf, BUFSIZE);
	return buf;
}

template<int BUFSIZE>
inline std::wstring GetDlgItemTextW(HWND hDlg, int nIDDlgItem) {
	wchar_t buf[BUFSIZE];
	GetDlgItemTextW(hDlg, nIDDlgItem, buf, BUFSIZE);
	return buf;
}

enum FCEUMENU_HWND {
	FCEUMENU_CONTEXT_OFF, // NoGame
	FCEUMENU_CONTEXT_GAME, // Game+NoMovie
	FCEUMENU_CONTEXT_PLAYING_READONLY, //Game+Movie+Playing+ReadOnly
	FCEUMENU_CONTEXT_PLAYING_READWRITE, //Game+Movie+Playing+ReadWrite
	FCEUMENU_CONTEXT_RECORDING_READONLY, //Game+Movie+Recording+ReadOnly
	FCEUMENU_CONTEXT_RECORDING_READWRITE, //Game+Movie+Recording+Readwrite
	FCEUMENU_CONTEXT, // parent menu of all context menus,
	                  // not recommended to use it since there's duplicate ids in context menus,
	                  // unless you're quite sure the id is unique in all submenus.
	FCEUMENU_MAIN, // main fceux menu
	FCEUMENU_LIMIT
};

struct HOTKEYMENUINDEX {
	int menu_id; // menu ID
	int cmd_id;  // hotkey ID
	int hmenu_index = FCEUMENU_MAIN; // whitch menu it belongs to, refers to FCEUMENU_HWND
	int flags = MF_BYCOMMAND; // flags when searching and modifying menu item, usually MF_BYCOMMAND
	// returns an std::string contains original menu text followed with shortcut keys.
	std::string getQualifiedMenuText();
	// this is used when you only want to create a qualified menu text String
	static std::string getQualifiedMenuText(char* text, int cmdid);
	int updateMenuText();
	HOTKEYMENUINDEX(int id, int cmd, int menu = FCEUMENU_MAIN, int _flags = MF_BYCOMMAND) : menu_id(id), cmd_id(cmd), hmenu_index(menu), flags(_flags) {}
};

#endif
