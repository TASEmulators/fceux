#ifndef DEBUGGER_H
#define DEBUGGER_H

//#define GetMem(x) (((x < 0x2000) || (x >= 0x4020))?ARead[x](x):0xFF)
#include <windows.h>
//#include "debug.h"

//extern volatile int userpause; //mbg merge 7/18/06 removed for merging
extern int scanline; //current scanline! :D
extern HWND hDebug;

extern int childwnd,numWPs; //mbg merge 7/18/06 had to make extern

BOOL CenterWindow(HWND hwndDlg);
void DoPatcher(int address,HWND hParent);
void UpdatePatcher(HWND hwndDlg);
int GetEditHex(HWND hwndDlg, int id);

extern void AddBreakList();

void UpdateDebugger();
void DoDebug(uint8 halt);

extern bool inDebugger;



#endif
