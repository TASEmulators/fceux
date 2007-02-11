#ifndef WIN_WINDOW_H
#define WIN_WINDOW_H

#include "common.h"

static int EnableBackgroundInput = 0;

extern char *recent_files[];
extern char *recent_directories[10];

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

#endif