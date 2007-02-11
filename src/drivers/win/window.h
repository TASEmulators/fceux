#ifndef WIN_WINDOW_H
#define WIN_WINDOW_H

#include "common.h"

// #defines

#define MAX(x,y) ((x)<(y)?(y):(x))
#define MIN(x,y) ((x)>(y)?(y):(x))

// Type definitions

struct CreateMovieParameters
{
	char* szFilename;				// on Dialog creation, this is the default filename to display.  On return, this is the filename that the user chose.
	int recordFrom;				// 0 = "Power-On", 1 = "Reset", 2 = "Now", 3+ = savestate file in szSavestateFilename
	char* szSavestateFilename;
	WCHAR metadata[MOVIE_MAX_METADATA];
};

static int EnableBackgroundInput = 0;

extern char *recent_files[];
extern char *recent_directories[10];
extern HWND pwindow;

void ShowCursorAbs(int set_visible);
void HideFWindow(int h);
void SetMainWindowStuff();
int GetClientAbsRect(LPRECT lpRect);
void FixWXY(int pref);
void ByebyeWindow();
void DoTimingConfigFix();
int CreateMainWindow();
void UpdateCheckedMenuItems();
void ALoad(char *nameo);
void LoadNewGamey(HWND hParent, const char *initialdir);
int BrowseForFolder(HWND hParent, const char *htext, char *buf);

#endif