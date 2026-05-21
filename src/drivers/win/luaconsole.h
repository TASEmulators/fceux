#ifndef LUACONSOLE_H
#define LUACONSOLE_H

extern HWND LuaConsoleHWnd;

INT_PTR CALLBACK DlgLuaScriptDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);
void UpdateLuaConsole(const char* fname);
bool GetLuaArgs(char *args, int len);

#endif
