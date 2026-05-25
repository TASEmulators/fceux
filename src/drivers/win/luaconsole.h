#ifndef LUACONSOLE_H
#define LUACONSOLE_H

extern HWND LuaConsoleHWnd;
extern bool LuaArgCompat;

void PrintToWindowConsole(intptr_t hDlgAsInt, const char* str);
void WinLuaOnStart(intptr_t hDlgAsInt);
void WinLuaOnStop(intptr_t hDlgAsInt);
INT_PTR CALLBACK DlgLuaScriptDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
void UpdateLuaConsole(const char* fname);
bool GetLuaArgs(char *args, int len);

#endif
