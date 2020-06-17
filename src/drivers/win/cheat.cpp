/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Ben Parnell
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "common.h"
#include "cheat.h"
#include "memview.h"
#include "memwatch.h"
#include "debugger.h"
#include "ramwatch.h"
#include "../../fceu.h"
#include "../../cart.h"
#include "../../cheat.h" // For FCEU_LoadGameCheats()
#include <map>

// static HWND pwindow = 0;	    // owomomo: removed pwindow because ambiguous, perhaps it is some obseleted early future plan from half developed old FCEUX? 
HWND hCheat = 0;			 //Handle to Cheats dialog
HWND hCheatTip = 0;          //Handle to tooltip
HMENU hCheatcontext = 0;     //Handle to cheat context menu

bool pauseWhileActive = false;	//For checkbox "Pause while active"
extern int globalCheatDisabled;
extern int disableAutoLSCheats;
extern bool wasPausedByCheats;

int CheatWindow;
int CheatStyle = 1;
int CheatMapUsers = 0; // how many windows using cheatmap

#define GGLISTSIZE 128 //hopefully this is enough for all cases


int selcheat;
int selcheatcount;
int ChtPosX,ChtPosY;
int GGConv_wndx=0, GGConv_wndy=0;
static HFONT hFont,hNewFont;

// static int scrollindex;
// static int scrollnum;
// static int scrollmax;
std::map<int, SEARCHPOSSIBLE> possiList;

int possiStart = 0;
int possiItemCount = 0;
int possiTotalCount = 0;
bool possibleUpdate = false;

int lbfocus = 0;
int searchdone;
static int knownvalue = 0;

// int GGaddr, GGcomp, GGval;
// char GGcode[10];
int GGlist[GGLISTSIZE];
static int dontupdateGG; //this eliminates recursive crashing
char* GameGenieLetters = "APZLGITYEOXUKSVN";

// bool dodecode;

HWND hGGConv = 0;

void EncodeGG(char *str, int a, int v, int c);
void ListGGAddresses(HWND hwndDlg);

uint16 StrToU16(char *s)
{
	unsigned int ret=0;
	sscanf(s,"%4x",&ret);
	return ret;
}

uint8 StrToU8(char *s)
{
	unsigned int ret=0;
	sscanf(s,"%2x",&ret);
	return ret;
}

char *U16ToStr(uint16 a)
{
	static char str[5];
	sprintf(str,"%04X",a);
	return str;
}

char *U8ToStr(uint8 a)
{
	static char str[3];
	sprintf(str,"%02X",a);
	return str;
}

//int RedoCheatsCallB(char *name, uint32 a, uint8 v, int s) { //bbit edited: this commented out line was changed to the below for the new fceud
int RedoCheatsCallB(char *name, uint32 a, uint8 v, int c, int s, int type, void* data)
{
	char str[256] = { 0 };
	GetCheatStr(str, a, v, c);

	LVITEM lvi = { 0 };
	lvi.mask = LVIF_TEXT;
	lvi.iItem = data ? *((int*)data) : GGLISTSIZE;
	lvi.pszText = str;

	if (data)
		SendDlgItemMessage(hCheat, IDC_LIST_CHEATS, LVM_SETITEM, 0, (LPARAM)&lvi);
	else
		lvi.iItem = SendDlgItemMessage(hCheat, IDC_LIST_CHEATS, LVM_INSERTITEM, 0, (LPARAM)&lvi);
	lvi.iSubItem = 1;
	lvi.pszText = name;
	SendDlgItemMessage(hCheat, IDC_LIST_CHEATS, LVM_SETITEM, 0, (LPARAM)&lvi);

	lvi.mask = LVIF_STATE;
	lvi.stateMask = LVIS_STATEIMAGEMASK;
	lvi.state = INDEXTOSTATEIMAGEMASK(s ? 2 : 1);
	SendDlgItemMessage(hCheat, IDC_LIST_CHEATS, LVM_SETITEMSTATE, lvi.iItem, (LPARAM)&lvi);


	return 1;
}

void RedoCheatsLB(HWND hwndDlg)
{
	SendDlgItemMessage(hwndDlg, IDC_LIST_CHEATS, LVM_DELETEALLITEMS, 0, 0);
	FCEUI_ListCheats(RedoCheatsCallB, 0);

	if (selcheat >= 0)
	{
		EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_CHEAT_DEL), TRUE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_CHEAT_UPD), TRUE);
	} else
	{
		EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_CHEAT_DEL), FALSE);
		EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_CHEAT_UPD), FALSE);
	}
}

HWND InitializeResultsList(HWND hwnd)
{
	HWND hwndResults = GetDlgItem(hwnd, IDC_CHEAT_LIST_POSSIBILITIES);

	// prepare columns
	LVCOLUMN lv = { 0 };
	lv.mask = LVCF_TEXT | LVCF_WIDTH;

	lv.pszText = "Addr";
	lv.cx = 50;
	SendMessage(hwndResults, LVM_INSERTCOLUMN, 0, (LPARAM)&lv);

	lv.pszText = "Pre";
	lv.mask |= LVCF_FMT;
	lv.fmt = LVCFMT_RIGHT;
	lv.cx = 36;
	SendMessage(hwndResults, LVM_INSERTCOLUMN, 1, (LPARAM)&lv);

	lv.pszText = "Cur";
	SendMessage(hwndResults, LVM_INSERTCOLUMN, 2, (LPARAM)&lv);


	// set style to full row select
	SendMessage(hwndResults, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);
	

	return hwndResults;
}

int ShowResultsCallB(uint32 a, uint8 last, uint8 current)
{
	if (hCheat)
	{
		if (possiList[possiItemCount].update =
			(possiList[possiItemCount].addr != a ||
				possiList[possiItemCount].previous != last ||
				possiList[possiItemCount].current != current))
		{
			possiList[possiItemCount].addr = a;
			possiList[possiItemCount].previous = last;
			possiList[possiItemCount].current = current;
			possibleUpdate |= possiList[possiItemCount].update;
		}
		++possiItemCount;
	}

	return 1;
}

int ShowResults(HWND hwndDlg, bool supressUpdate = false)
{

	if (possiList.size() > 64)
		possiList.clear();

	int count = FCEUI_CheatSearchGetCount();

	if (count != possiTotalCount)
	{
		char str[20];
		sprintf(str, "%d Possibilit%s", count, count == 1 ? "y" : "ies");
		SetDlgItemText(hwndDlg, IDC_CHEAT_BOX_POSSIBILITIES, str);
		SendDlgItemMessage(hwndDlg, IDC_CHEAT_LIST_POSSIBILITIES, LVM_SETITEMCOUNT, count, 0);
		possiTotalCount = count;
	}

	if (count)
	{
		int first = SendDlgItemMessage(hwndDlg, IDC_CHEAT_LIST_POSSIBILITIES, LVM_GETTOPINDEX, 0, 0);
		if (first < 0)
			first = 0;
		int last = first + possiItemCount + 1;
		if (last > count)
			last = count;

		int tmpPossiItemCount = possiItemCount;
		possiItemCount = first;
		FCEUI_CheatSearchGetRange(first, last, ShowResultsCallB);
		possiItemCount = tmpPossiItemCount;
		if (possibleUpdate && !supressUpdate)
		{
			int start = -1, end = -1;
			for (int i = first; i < last; ++i)
				if (possiList[i - first].update)
				{
					if (start == -1)
						start = i;
					end = i;
				}

			SendDlgItemMessage(hwndDlg, IDC_CHEAT_LIST_POSSIBILITIES, LVM_REDRAWITEMS, start, end);
		}
		possibleUpdate = false;
		possiStart = first;
	}

	return 1;
}

void EnableCheatButtons(HWND hwndDlg, int enable)
{
	EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_CHEAT_KNOWN), enable);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHEAT_VAL_KNOWN), enable);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHEAT_LABEL_KNOWN), enable);
	EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_CHEAT_EQ), enable);
	EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_CHEAT_NE), enable);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHEAT_CHECK_NE_BY), enable);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHEAT_VAL_NE_BY), enable);
	EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_CHEAT_GT), enable);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHEAT_CHECK_GT_BY), enable);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHEAT_VAL_GT_BY), enable);
	EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_CHEAT_LT), enable);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHEAT_CHECK_LT_BY), enable);
	EnableWindow(GetDlgItem(hwndDlg, IDC_CHEAT_VAL_LT_BY), enable);

}

HWND InitializeCheatList(HWND hwnd)
{
	HWND hwndChtList = GetDlgItem(hwnd, IDC_LIST_CHEATS);

	// prepare the columns
	LVCOLUMN lv = { 0 };
	lv.mask = LVCF_TEXT | LVCF_WIDTH;

	lv.pszText = "Code";
	lv.cx = 100;
	SendMessage(hwndChtList, LVM_INSERTCOLUMN, 0, (LPARAM)&lv);

	lv.pszText = "Name";
	lv.cx = 132;
	SendMessage(hwndChtList, LVM_INSERTCOLUMN, 1, (LPARAM)&lv);

	// Add a checkbox to indicate if the cheat is activated
	SendMessage(hwndChtList, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES, LVS_EX_FULLROWSELECT | LVS_EX_CHECKBOXES);

	return hwndChtList;
}

INT_PTR CALLBACK CheatConsoleCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch (uMsg)
	{
		case WM_INITDIALOG:
		{
			POINT pt;
			if (ChtPosX != 0 && ChtPosY != 0)
			{
				pt.x = ChtPosX;
				pt.y = ChtPosY;
				pt = CalcSubWindowPos(hwndDlg, &pt);
			}
			else
				pt = CalcSubWindowPos(hwndDlg, NULL);

			ChtPosX = pt.x;
			ChtPosY = pt.y;


			// SetWindowPos(hwndDlg, 0, ChtPosX, ChtPosY, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOOWNERZORDER);

			CheckDlgButton(hwndDlg, IDC_CHEAT_PAUSEWHENACTIVE, pauseWhileActive ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_CHEAT_GLOBAL_SWITCH, globalCheatDisabled ? BST_UNCHECKED : BST_CHECKED);
			CheckDlgButton(hwndDlg, IDC_CHEAT_AUTOLOADSAVE, disableAutoLSCheats == 2 ? BST_UNCHECKED : disableAutoLSCheats == 1 ? BST_INDETERMINATE : BST_CHECKED);

			//setup font
			SetupCheatFont(hwndDlg);

			SendDlgItemMessage(hwndDlg, IDC_CHEAT_ADDR, WM_SETFONT, (WPARAM)hNewFont, FALSE);
			SendDlgItemMessage(hwndDlg, IDC_CHEAT_VAL, WM_SETFONT, (WPARAM)hNewFont, FALSE);
			SendDlgItemMessage(hwndDlg, IDC_CHEAT_COM, WM_SETFONT, (WPARAM)hNewFont, FALSE);
			SendDlgItemMessage(hwndDlg, IDC_CHEAT_VAL_KNOWN, WM_SETFONT, (WPARAM)hNewFont, FALSE);
			SendDlgItemMessage(hwndDlg, IDC_CHEAT_LIST_POSSIBILITIES, WM_SETFONT, (WPARAM)hNewFont, FALSE);
			SendDlgItemMessage(hwndDlg, IDC_CHEAT_VAL_NE_BY, WM_SETFONT, (WPARAM)hNewFont, FALSE);
			SendDlgItemMessage(hwndDlg, IDC_CHEAT_VAL_GT_BY, WM_SETFONT, (WPARAM)hNewFont, FALSE);
			SendDlgItemMessage(hwndDlg, IDC_CHEAT_VAL_LT_BY, WM_SETFONT, (WPARAM)hNewFont, FALSE);
			SendDlgItemMessage(hwndDlg, IDC_CHEAT_TEXT, WM_SETFONT, (WPARAM)hNewFont, FALSE);

			//text limits
			SendDlgItemMessage(hwndDlg, IDC_CHEAT_ADDR, EM_SETLIMITTEXT, 4, 0);
			SendDlgItemMessage(hwndDlg, IDC_CHEAT_VAL, EM_SETLIMITTEXT, 2, 0);
			SendDlgItemMessage(hwndDlg, IDC_CHEAT_COM, EM_SETLIMITTEXT, 2, 0);
			SendDlgItemMessage(hwndDlg, IDC_CHEAT_NAME, EM_SETLIMITTEXT, 256, 0);
			SendDlgItemMessage(hwndDlg, IDC_CHEAT_VAL_KNOWN, EM_SETLIMITTEXT, 2, 0);
			SendDlgItemMessage(hwndDlg, IDC_CHEAT_VAL_NE_BY, EM_SETLIMITTEXT, 2, 0);
			SendDlgItemMessage(hwndDlg, IDC_CHEAT_VAL_GT_BY, EM_SETLIMITTEXT, 2, 0);
			SendDlgItemMessage(hwndDlg, IDC_CHEAT_VAL_LT_BY, EM_SETLIMITTEXT, 2, 0);
			SendDlgItemMessage(hwndDlg, IDC_CHEAT_TEXT, EM_SETLIMITTEXT, 10, 0);
			SendDlgItemMessage(hwndDlg, IDC_CHEAT_GAME_GENIE_TEXT, EM_SETLIMITTEXT, 8, 0);

			// limit their characters
			DefaultEditCtrlProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_CHEAT_ADDR), GWLP_WNDPROC, (LONG_PTR)FilterEditCtrlProc);
			SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_CHEAT_ADDR), GWLP_WNDPROC, (LONG_PTR)FilterEditCtrlProc);
			SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_CHEAT_VAL), GWLP_WNDPROC, (LONG_PTR)FilterEditCtrlProc);
			SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_CHEAT_COM), GWLP_WNDPROC, (LONG_PTR)FilterEditCtrlProc);
			SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_CHEAT_VAL_KNOWN), GWLP_WNDPROC, (LONG_PTR)FilterEditCtrlProc);
			SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_CHEAT_VAL_NE_BY), GWLP_WNDPROC, (LONG_PTR)FilterEditCtrlProc);
			SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_CHEAT_VAL_GT_BY), GWLP_WNDPROC, (LONG_PTR)FilterEditCtrlProc);
			SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_CHEAT_VAL_LT_BY), GWLP_WNDPROC, (LONG_PTR)FilterEditCtrlProc);
			SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_CHEAT_TEXT), GWLP_WNDPROC, (LONG_PTR)FilterEditCtrlProc);
			SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_CHEAT_GAME_GENIE_TEXT), GWLP_WNDPROC, (LONG_PTR)FilterEditCtrlProc);

			// Create popup to "Auto load / save with game", since it has 3 states and the text need some explanation
			SetCheatToolTip(hwndDlg, IDC_CHEAT_AUTOLOADSAVE);

			possiTotalCount = 0;
			possiItemCount = SendDlgItemMessage(hwndDlg, IDC_CHEAT_LIST_POSSIBILITIES, LVM_GETCOUNTPERPAGE, 0, 0);

			ShowResults(hwndDlg);
			EnableCheatButtons(hwndDlg, possiTotalCount != 0);

			//add header for cheat list and possibilities
			InitializeCheatList(hwndDlg);
			InitializeResultsList(hwndDlg);

			//misc setup
			searchdone = 0;
			SetDlgItemText(hwndDlg, IDC_CHEAT_VAL_KNOWN, (LPCSTR)U8ToStr(knownvalue));

			// Enable Context Sub-Menus
			hCheatcontext = LoadMenu(fceu_hInstance, "CHEATCONTEXTMENUS");

			break;
		}
		case WM_KILLFOCUS:
			break;
		case WM_NCACTIVATE:
			if (pauseWhileActive) 
			{
				if (EmulationPaused == 0) 
				{
					EmulationPaused = 1;
					wasPausedByCheats = true;
					FCEU_printf("Emulation paused: %d\n", EmulationPaused);
				}
			}
			if (CheatStyle && possiTotalCount) {
				if ((!wParam) && searchdone) {
					searchdone = 0;
					FCEUI_CheatSearchSetCurrentAsOriginal();
				}
				ShowResults(hwndDlg);   
			}
			break;
		case WM_QUIT:
		case WM_CLOSE:
			DestroyWindow(hCheatTip);
			if (CheatStyle)
				DestroyWindow(hwndDlg);
			else
				EndDialog(hwndDlg, 0);
			break;
		case WM_DESTROY:
			CheatWindow = 0;
			hCheat = NULL;
			DeleteCheatFont();
			if (searchdone)
				FCEUI_CheatSearchSetCurrentAsOriginal();
			possiList.clear();
			break;
		case WM_MOVE:
			if (!IsIconic(hwndDlg)) {
				RECT wrect;
				GetWindowRect(hwndDlg, &wrect);
				ChtPosX = wrect.left;
				ChtPosY = wrect.top;

				#ifdef WIN32
				WindowBoundsCheckNoResize(ChtPosX, ChtPosY, wrect.right);
				#endif
			}
			break;
		case WM_CONTEXTMENU:
		{
			// Handle certain subborn context menus for nearly incapable controls.
			HWND itemHwnd = (HWND)wParam;
			int dlgId = GetDlgCtrlID(itemHwnd);
			int sel = SendMessage(itemHwnd, LVM_GETSELECTIONMARK, 0, 0);
			HMENU hCheatcontextsub = NULL;
			switch (dlgId) {
				case IDC_LIST_CHEATS:
					// Only open the menu if a cheat is selected
					if (selcheat >= 0)
						// Open IDC_LIST_CHEATS Context Menu
						hCheatcontextsub = GetSubMenu(hCheatcontext, 0);
				break;
				case IDC_CHEAT_LIST_POSSIBILITIES:
					if (sel != -1) {
						hCheatcontextsub = GetSubMenu(hCheatcontext, 1);
						SetMenuDefaultItem(hCheatcontextsub, CHEAT_CONTEXT_POSSI_ADDTOMEMORYWATCH, false);
					}
			}

			if (hCheatcontextsub)
			{
				POINT point;
				if (lParam != -1)
				{
					point.x = LOWORD(lParam);
					point.y = HIWORD(lParam);
				} else {
					// Handle the context menu keyboard key
					RECT wrect;
					wrect.left = LVIR_BOUNDS;
					SendMessage(itemHwnd, LVM_GETITEMRECT, sel, (LPARAM)&wrect);
					point.x = wrect.left + (wrect.right - wrect.left) / 2;
					point.y = wrect.top + (wrect.bottom - wrect.top) / 2;
					ClientToScreen(itemHwnd, &point);
				}
				TrackPopupMenu(hCheatcontextsub, TPM_RIGHTBUTTON, point.x, point.y, 0, hwndDlg, 0);	//Create menu
			}
		}
		break;
		case WM_COMMAND:
		{
			static int editMode = -1;

			switch (HIWORD(wParam))
			{
				case BN_CLICKED:
					switch (LOWORD(wParam))
					{
						case CHEAT_CONTEXT_LIST_TOGGLECHEAT:
						{
							LVITEM lvi;
							lvi.mask = LVIF_STATE;
							lvi.stateMask = LVIS_STATEIMAGEMASK;
							int tmpsel = SendDlgItemMessage(hCheat, IDC_LIST_CHEATS, LVM_GETNEXTITEM, -1, LVNI_ALL | LVNI_SELECTED);

							char* name = ""; int s;
							while (tmpsel != -1)
							{
								FCEUI_GetCheat(tmpsel, &name, NULL, NULL, NULL, &s, NULL);
								FCEUI_SetCheat(tmpsel, name, -1, -1, -2, s ^= 1, 1);

								lvi.iItem = tmpsel;
								lvi.state = INDEXTOSTATEIMAGEMASK(s ? 2 : 1);
								SendDlgItemMessage(hCheat, IDC_LIST_CHEATS, LVM_SETITEMSTATE, tmpsel, (LPARAM)&lvi);

								tmpsel = SendDlgItemMessage(hwndDlg, IDC_LIST_CHEATS, LVM_GETNEXTITEM, tmpsel, LVNI_ALL | LVNI_SELECTED);
							}

							UpdateCheatRelatedWindow();
							UpdateCheatListGroupBoxUI();
						}
						break;
						case CHEAT_CONTEXT_LIST_POKECHEATVALUE:
						{
							uint32 a; uint8 v;
							FCEUI_GetCheat(selcheat, NULL, &a, &v, NULL, NULL, NULL);
							BWrite[a](a, v);
						}
						break;
						case CHEAT_CONTEXT_LIST_GOTOINHEXEDITOR:
						{
							uint32 a;
							FCEUI_GetCheat(selcheat, NULL, &a, NULL, NULL, NULL, NULL);
							DoMemView();
							SetHexEditorAddress(a);
						}
						break;
						case CHEAT_CONTEXT_POSSI_ADDCHEAT:
						{
							char str[256] = { 0 };
							int sel = SendDlgItemMessage(hwndDlg, IDC_CHEAT_LIST_POSSIBILITIES, LVM_GETSELECTIONMARK, 0, 0);
							if (sel != -1)
							{
								char str[256] = { 0 };
								GetDlgItemText(hwndDlg, IDC_CHEAT_NAME, str, 256);
								SEARCHPOSSIBLE& possible = possiList[sel];
								if (FCEUI_AddCheat(str, possible.addr, possible.current, -1, 1))
								{
									RedoCheatsCallB(str, possible.addr, possible.current, -1, 1, 1, NULL);

									int newselcheat = SendDlgItemMessage(hwndDlg, IDC_LIST_CHEATS, LVM_GETITEMCOUNT, 0, 0) - 1;
									ListView_MoveSelectionMark(GetDlgItem(hwndDlg, IDC_LIST_CHEATS), selcheat, newselcheat);
									selcheat = newselcheat;
								}

								UpdateCheatRelatedWindow();
								UpdateCheatListGroupBoxUI();
							}
						}
						break;
						case CHEAT_CONTEXT_POSSI_ADDTOMEMORYWATCH:
						{
							char addr[32] = { 0 };
							int sel = SendDlgItemMessage(hwndDlg, IDC_CHEAT_LIST_POSSIBILITIES, LVM_GETSELECTIONMARK, 0, 0);
							if (sel != -1)
							{
								sprintf(addr, "%04X", possiList[sel].addr);
								AddMemWatch(addr);
							}
						}
						break;
						case CHEAT_CONTEXT_POSSI_ADDTORAMWATCH:
						{
							int sel = SendDlgItemMessage(hwndDlg, IDC_CHEAT_LIST_POSSIBILITIES, LVM_GETSELECTIONMARK, 0, 0);
							if (sel != -1)
							{
								AddressWatcher tempWatch;
								tempWatch.Size = 'b';
								tempWatch.Type = 'h';
								tempWatch.Address = possiList[sel].addr;
								tempWatch.WrongEndian = false;
								if (InsertWatch(tempWatch, hwndDlg) && !RamWatchHWnd)
									SendMessage(hAppWnd, WM_COMMAND, ID_RAM_WATCH, 0);
								SetForegroundWindow(RamWatchHWnd);
							}
						}
						break;
						case CHEAT_CONTEXT_POSSI_GOTOINHEXEDITOR:
						{
							int sel = SendDlgItemMessage(hwndDlg, IDC_CHEAT_LIST_POSSIBILITIES, LVM_GETSELECTIONMARK, 0, 0);
							if (sel != -1)
							{
								DoMemView();
								SetHexEditorAddress(possiList[sel].addr);
							}
						}
						break;
						case IDC_CHEAT_PAUSEWHENACTIVE:
							pauseWhileActive ^= 1;
							if ((EmulationPaused == 1 ? true : false) != pauseWhileActive)
							{
								EmulationPaused = (pauseWhileActive ? 1 : 0);
								wasPausedByCheats = pauseWhileActive;
								if (EmulationPaused)
									FCEU_printf("Emulation paused: %d\n", EmulationPaused);
							}
							break;
						case IDC_BTN_CHEAT_ADD:
						{
							char name[256] = { 0 }; uint32 a; uint8 v = 0; int c = 0;
							GetUICheatInfo(hwndDlg, name, &a, &v, &c);

							if (FCEUI_AddCheat(name, a, v, c, 1)) {
								RedoCheatsCallB(name, a, v, c, 1, 1, NULL);

								int newselcheat = SendDlgItemMessage(hwndDlg, IDC_LIST_CHEATS, LVM_GETITEMCOUNT, 0, 0) - 1;
								ListView_MoveSelectionMark(GetDlgItem(hwndDlg, IDC_LIST_CHEATS), selcheat, newselcheat);
								selcheat = newselcheat;

								ClearCheatListText(hwndDlg);
							}
							UpdateCheatRelatedWindow();
							UpdateCheatListGroupBoxUI();
							break;
						}
						case CHEAT_CONTEXT_LIST_DELETESELECTEDCHEATS:
						case IDC_BTN_CHEAT_DEL:
							if (selcheatcount > 1)
							{
								if (IDYES == MessageBox(hwndDlg, "Multiple cheats selected. Continue with delete?", "Delete multiple cheats?", MB_ICONQUESTION | MB_YESNO)) { //Get message box
									selcheat = -1;

									int selcheattemp = SendDlgItemMessage(hwndDlg, IDC_LIST_CHEATS, LVM_GETNEXTITEM, -1, LVNI_ALL | LVNI_SELECTED);
									LVITEM lvi;
									lvi.mask = LVIF_STATE;
									lvi.stateMask = LVIS_SELECTED;
									lvi.state = 0;
									while (selcheattemp != -1)
									{
										FCEUI_DelCheat(selcheattemp);
										SendDlgItemMessage(hwndDlg, IDC_LIST_CHEATS, LVM_DELETEITEM, selcheattemp, 0);
										selcheattemp = SendDlgItemMessage(hwndDlg, IDC_LIST_CHEATS, LVM_GETNEXTITEM, -1, LVNI_ALL | LVNI_SELECTED);
									}

									ClearCheatListText(hwndDlg);

									UpdateCheatRelatedWindow();
									UpdateCheatListGroupBoxUI();
								}
							}
							else {
								if (selcheat >= 0) {
									FCEUI_DelCheat(selcheat);
									SendDlgItemMessage(hwndDlg, IDC_LIST_CHEATS, LVM_DELETEITEM, selcheat, 0);
									selcheat = -1;
									ClearCheatListText(hwndDlg);
								}
								UpdateCheatRelatedWindow();
								UpdateCheatListGroupBoxUI();
							}
							break;
						case IDC_BTN_CHEAT_UPD:
						{
							selcheat = SendDlgItemMessage(hwndDlg, IDC_LIST_CHEATS, LVM_GETSELECTIONMARK, 0, 0);
							if (selcheat < 0)
								break;

							char name[256]; uint32 a; uint8 v; int s; int c;
							GetUICheatInfo(hwndDlg, name, &a, &v, &c);

							FCEUI_SetCheat(selcheat, name, a, v, c, -1, 1);
							FCEUI_GetCheat(selcheat, NULL, &a, &v, &c, &s, NULL);
							RedoCheatsCallB(name, a, v, c, s, 1, &selcheat);
							SendDlgItemMessage(hwndDlg, IDC_LIST_CHEATS, LVM_SETSELECTIONMARK, 0, selcheat);

							SetDlgItemText(hwndDlg, IDC_CHEAT_ADDR, (LPCSTR)U16ToStr(a));
							SetDlgItemText(hwndDlg, IDC_CHEAT_VAL, (LPCSTR)U8ToStr(v));
							if (c == -1)
								SetDlgItemText(hwndDlg, IDC_CHEAT_COM, (LPCSTR)"");
							else
								SetDlgItemText(hwndDlg, IDC_CHEAT_COM, (LPCSTR)U8ToStr(c));
							UpdateCheatRelatedWindow();
							UpdateCheatListGroupBoxUI();
							// UpdateCheatAdded();
							break;
						}
						case IDC_BTN_CHEAT_ADDFROMFILE:
						{
							char filename[2048];
							if (ShowCheatFileBox(hwndDlg, filename, false))
							{
								FILE* file = FCEUD_UTF8fopen(filename, "rb");
								if (file)
								{
									FCEU_LoadGameCheats(file, 0);
									UpdateCheatsAdded();
									UpdateCheatRelatedWindow();
									savecheats = 1;
								}
							}
						}
						break;
						case IDC_BTN_CHEAT_EXPORTTOFILE:
							SaveCheatAs(hwndDlg);
						break;
						case IDC_BTN_CHEAT_RESET:
							FCEUI_CheatSearchBegin();
							ShowResults(hwndDlg);
							EnableCheatButtons(hwndDlg, TRUE);
							break;
						case IDC_BTN_CHEAT_KNOWN:
						{
							char str[256] = { 0 };
							searchdone = 1;
							GetDlgItemText(hwndDlg, IDC_CHEAT_VAL_KNOWN, str, 3);
							knownvalue = StrToU8(str);
							FCEUI_CheatSearchEnd(FCEU_SEARCH_NEWVAL_KNOWN, knownvalue, 0);
							ShowResults(hwndDlg);
						}
						break;
						case IDC_BTN_CHEAT_EQ:
							searchdone = 1;
							FCEUI_CheatSearchEnd(FCEU_SEARCH_PUERLY_RELATIVE_CHANGE, 0, 0);
							ShowResults(hwndDlg);
							break;
						case IDC_BTN_CHEAT_NE:
						{
							char str[256] = { 0 };
							searchdone = 1;
							if (IsDlgButtonChecked(hwndDlg, IDC_CHEAT_CHECK_NE_BY) == BST_CHECKED) {
								GetDlgItemText(hwndDlg, IDC_CHEAT_VAL_NE_BY, str, 3);
								FCEUI_CheatSearchEnd(FCEU_SEARCH_PUERLY_RELATIVE_CHANGE, 0, StrToU8(str));
							}
							else FCEUI_CheatSearchEnd(FCEU_SEARCH_ANY_CHANGE, 0, 0);
							ShowResults(hwndDlg);
						}
						break;
						case IDC_BTN_CHEAT_GT:
						{
							char str[256] = { 0 };
							searchdone = 1;
							if (IsDlgButtonChecked(hwndDlg, IDC_CHEAT_CHECK_GT_BY) == BST_CHECKED) {
								GetDlgItemText(hwndDlg, IDC_CHEAT_VAL_GT_BY, str, 3);
								FCEUI_CheatSearchEnd(FCEU_SEARCH_NEWVAL_GT_KNOWN, 0, StrToU8(str));
							}
							else FCEUI_CheatSearchEnd(FCEU_SEARCH_NEWVAL_GT, 0, 0);
							ShowResults(hwndDlg);
						}
						break;
						case IDC_BTN_CHEAT_LT:
						{
							char str[256] = { 0 };
							searchdone = 1;
							if (IsDlgButtonChecked(hwndDlg, IDC_CHEAT_CHECK_LT_BY) == BST_CHECKED) {
								GetDlgItemText(hwndDlg, IDC_CHEAT_VAL_LT_BY, str, 3);
								FCEUI_CheatSearchEnd(FCEU_SEARCH_NEWVAL_LT_KNOWN, 0, StrToU8(str));
							}
							else FCEUI_CheatSearchEnd(FCEU_SEARCH_NEWVAL_LT, 0, 0);
							ShowResults(hwndDlg);
						}
						break;
						case IDC_CHEAT_GLOBAL_SWITCH:
							if (FCEUI_GlobalToggleCheat(IsDlgButtonChecked(hwndDlg, IDC_CHEAT_GLOBAL_SWITCH)))
							{
								UpdateCheatRelatedWindow();
								UpdateCheatListGroupBoxUI();
							}
						break;
						case IDC_CHEAT_AUTOLOADSAVE:
						{
							switch (IsDlgButtonChecked(hwndDlg, IDC_CHEAT_AUTOLOADSAVE))
							{
								case BST_CHECKED: disableAutoLSCheats = 0; break;
								case BST_INDETERMINATE: disableAutoLSCheats = 1; break;
								case BST_UNCHECKED: 
									if(MessageBox(hwndDlg, "If this option is unchecked, you must manually save the cheats by yourself, or all the changes you made to the cheat list would be discarded silently without any asking once you close the game!\nDo you really want to do it in this way?", "Cheat warning", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) == IDYES)
										disableAutoLSCheats = 2;
									else
									{
										disableAutoLSCheats = 0;
										CheckDlgButton(hwndDlg, IDC_CHEAT_AUTOLOADSAVE, BST_CHECKED);
									}
							}
							SetCheatToolTip(hwndDlg, IDC_CHEAT_AUTOLOADSAVE);
						}
					}
					break;
					case EN_SETFOCUS:
						switch (LOWORD(wParam))
						{
							case IDC_CHEAT_ADDR:
							case IDC_CHEAT_VAL:
							case IDC_CHEAT_COM: editMode = 0; break;
							case IDC_CHEAT_TEXT: editMode = 1; break;
							case IDC_CHEAT_GAME_GENIE_TEXT: editMode = 2; break;
							default: editMode = -1;
						}
						break;
					case EN_KILLFOCUS:
						switch (LOWORD(wParam))
						{
							case IDC_CHEAT_ADDR:
							case IDC_CHEAT_VAL:
							case IDC_CHEAT_COM:
							case IDC_CHEAT_TEXT:
							case IDC_CHEAT_GAME_GENIE_TEXT:
							default: editMode = -1; break;
						}
						break;
					case EN_UPDATE:
						switch (LOWORD(wParam))
						{
							case IDC_CHEAT_ADDR:
							case IDC_CHEAT_VAL:
							case IDC_CHEAT_COM:
							{
								if (editMode == 0)
								{
									char buf[16]; uint32 a; uint8 v; int c;
									GetUICheatInfo(hwndDlg, NULL, &a, &v, &c);
									buf[0] = 0;
									GetCheatCodeStr(buf, a, v, c);
									SetDlgItemText(hwndDlg, IDC_CHEAT_TEXT, buf);
									buf[0] = 0;
									if (a > 0x7FFF && v != -1)
										EncodeGG(buf, a, v, c);
									SetDlgItemText(hwndDlg, IDC_CHEAT_GAME_GENIE_TEXT, buf);
								}
							}
							break;
							case IDC_CHEAT_TEXT:
							{
								if (editMode == 1)
								{
									char buf[16];
									GetDlgItemText(hwndDlg, IDC_CHEAT_TEXT, buf, 16);
									int a = -1, v = -1, c = -1;
									if (strchr(buf, ':'))
									{
										if (strchr(buf, '?'))
											sscanf(buf, "%X:%X?%X", (unsigned int*)&a, (unsigned int*)&c, (unsigned int*)&v);
										else
											sscanf(buf, "%X:%X", (unsigned int*)&a, (unsigned int*)&v);
									}

									SetDlgItemText(hwndDlg, IDC_CHEAT_ADDR, (LPCSTR)(a == -1 ? "" : U16ToStr(a)));
									SetDlgItemText(hwndDlg, IDC_CHEAT_VAL, (LPCSTR)(v == -1 ? "" : U8ToStr(v)));
									SetDlgItemText(hwndDlg, IDC_CHEAT_COM, (LPCSTR)(c == -1 ? "" : U8ToStr(c)));
									buf[0] = 0;
									if (a > 0x7FFF && v != -1)
										EncodeGG(buf, a, v, c);
									SetDlgItemText(hwndDlg, IDC_CHEAT_GAME_GENIE_TEXT, buf);
								}
							}
							break;
							case IDC_CHEAT_GAME_GENIE_TEXT:
							{
								if (editMode == 2)
								{
									char buf[16];
									GetDlgItemText(hwndDlg, IDC_CHEAT_GAME_GENIE_TEXT, buf, 16);
									int a = -1, v = -1, c = -1;
									if (strlen(buf) == 6 || strlen(buf) == 8)
										FCEUI_DecodeGG(buf, &a, &v, &c);

									SetDlgItemText(hwndDlg, IDC_CHEAT_ADDR, (LPCSTR)(a == -1 ? "" : U16ToStr(a)));
									SetDlgItemText(hwndDlg, IDC_CHEAT_VAL, (LPCSTR)(v == -1 ? "" : U8ToStr(v)));
									SetDlgItemText(hwndDlg, IDC_CHEAT_COM, (LPCSTR)(c == -1 ? "" : U8ToStr(c)));

									buf[0] = 0;
									if (a != -1 && v != -1)
										GetCheatCodeStr(buf, a, v, c);
									SetDlgItemText(hwndDlg, IDC_CHEAT_TEXT, buf);
								}
							}
						}
				}
			}
			break;
			case WM_NOTIFY:
			{
				switch (wParam)
				{
					case IDC_LIST_CHEATS:
					{
						NMHDR* lP = (NMHDR*)lParam;
						switch (lP->code) {
							case LVN_ITEMCHANGED:
							{
								NMLISTVIEW* pNMListView = (NMLISTVIEW*)lP;

//								selcheat = pNMListView->iItem;
								selcheatcount = SendDlgItemMessage(hwndDlg, IDC_LIST_CHEATS, LVM_GETSELECTEDCOUNT, 0, 0);
								if (pNMListView->uNewState & LVIS_FOCUSED ||
									!(pNMListView->uOldState & LVIS_SELECTED) && pNMListView->uNewState & LVIS_SELECTED)
								{
									selcheat = pNMListView->iItem;
									if (selcheat >= 0)
									{
										char* name = ""; uint32 a; uint8 v; int s; int c;
										FCEUI_GetCheat(selcheat, &name, &a, &v, &c, &s, NULL);
										SetDlgItemText(hwndDlg, IDC_CHEAT_NAME, (LPCSTR)name);
										SetDlgItemText(hwndDlg, IDC_CHEAT_ADDR, (LPCSTR)U16ToStr(a));
										SetDlgItemText(hwndDlg, IDC_CHEAT_VAL, (LPCSTR)U8ToStr(v));
										SetDlgItemText(hwndDlg, IDC_CHEAT_COM, (LPCSTR)(c == -1 ? "" : U8ToStr(c)));
										
										char code[32];
										code[0] = 0;
										GetCheatCodeStr(code, a, v, c);
										SetDlgItemText(hwndDlg, IDC_CHEAT_TEXT, code);
										code[0] = 0;
										if (a > 0x7FFF && v != -1)
											EncodeGG(code, a, v, c);
										SetDlgItemText(hwndDlg, IDC_CHEAT_GAME_GENIE_TEXT, code);

									}

									EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_CHEAT_DEL), selcheatcount > 0);
									EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_CHEAT_UPD), selcheatcount > 0);
								}

								if (pNMListView->uChanged & LVIF_STATE)
									// uncheck -> check
									if (pNMListView->uOldState & INDEXTOSTATEIMAGEMASK(1) &&
										pNMListView->uNewState & INDEXTOSTATEIMAGEMASK(2))
									{
										int tmpsel = pNMListView->iItem;
										char* name = ""; int s;
										FCEUI_GetCheat(tmpsel, &name, NULL, NULL, NULL, &s, NULL);
										if (!s)
										{
											FCEUI_SetCheat(tmpsel, name, -1, -1, -2, s ^= 1, 1);
											UpdateCheatRelatedWindow();
											UpdateCheatListGroupBoxUI();
										}
									}
									// check -> uncheck
									else if (pNMListView->uOldState & INDEXTOSTATEIMAGEMASK(2) &&
										pNMListView->uNewState & INDEXTOSTATEIMAGEMASK(1))
									{
										int tmpsel = pNMListView->iItem;
										char* name = ""; int s;
										FCEUI_GetCheat(tmpsel, &name, NULL, NULL, NULL, &s, NULL);
										if (s)
										{
											FCEUI_SetCheat(tmpsel, name, -1, -1, -2, s ^= 1, 1);

											UpdateCheatRelatedWindow();
											UpdateCheatListGroupBoxUI();
										}
									}
							}
						}
					}
					break;
					case IDC_CHEAT_LIST_POSSIBILITIES:
					{
						LPNMHDR lP = (LPNMHDR)lParam;
						switch (lP->code)
						{
							case LVN_ITEMCHANGED:
							{
								NMLISTVIEW* pNMListView = (NMLISTVIEW*)lP;
								if (pNMListView->uNewState & LVIS_FOCUSED ||
									!(pNMListView->uOldState & LVIS_SELECTED) && pNMListView->uNewState & LVIS_SELECTED)
								{

									SEARCHPOSSIBLE& possible = possiList[pNMListView->iItem];
									char str[16];
									sprintf(str, "%04X", possible.addr);
									SetDlgItemText(hwndDlg, IDC_CHEAT_ADDR, (LPCSTR)str);
									sprintf(str, "%02X", possible.current);
									SetDlgItemText(hwndDlg, IDC_CHEAT_VAL, (LPCSTR)str);
									SetDlgItemText(hwndDlg, IDC_CHEAT_COM, (LPCSTR)"");
								}
							}
							break;
							case LVN_GETDISPINFO:
							{
								NMLVDISPINFO* info = (NMLVDISPINFO*)lParam;
								
								if (!possiList.count(info->item.iItem))
									ShowResults(hwndDlg, true);

								static char num[32];
								switch (info->item.iSubItem)
								{
									case 0:
										sprintf(num, "$%04X", possiList[info->item.iItem].addr);
									break;
									case 1:
										sprintf(num, "%02X", possiList[info->item.iItem].previous);
									break;
									case 2:
										sprintf(num, "%02X", possiList[info->item.iItem].current);
									break;
								}
								info->item.pszText = num;
							}
							break;
							case NM_DBLCLK:
							{
								char addr[32];
								sprintf(addr, "%04X", possiList[((NMITEMACTIVATE*)lParam)->iItem].addr);
								AddMemWatch(addr);
							}
							break;
						}
					}
					break;
				}
			}
	}
	return 0;
}


void ConfigCheats(HWND hParent)
{
	if (!GameInfo)
	{
		FCEUD_PrintError("You must have a game loaded before you can manipulate cheats.");
		return;
	}
	//if (GameInfo->type==GIT_NSF) {
	//	FCEUD_PrintError("Sorry, you can't cheat with NSFs.");
	//	return;
	//}

	if (!CheatWindow) 
	{
		selcheat = -1;
		CheatWindow = 1;
		if (CheatStyle)
			hCheat = CreateDialog(fceu_hInstance, "CHEATCONSOLE", hParent, CheatConsoleCallB);
		else
			DialogBox(fceu_hInstance, "CHEATCONSOLE", hParent, CheatConsoleCallB);
		UpdateCheatsAdded();
		UpdateCheatRelatedWindow();
	} else
	{
		ShowWindow(hCheat, SW_SHOWNORMAL);
		SetForegroundWindow(hCheat);
	}
}

void UpdateCheatList()
{
	if (!hCheat)
		return;
	else
		ShowResults(hCheat);
}

void UpdateCheatListGroupBoxUI()
{
	char temp[64];
	if (FrozenAddressCount < 256)
	{
		sprintf(temp, "Active Cheats %u", FrozenAddressCount);
		EnableWindow(GetDlgItem(hCheat, IDC_BTN_CHEAT_ADD), TRUE);
		EnableWindow(GetDlgItem(hCheat, IDC_BTN_CHEAT_ADDFROMFILE), TRUE);
	}
	else if (FrozenAddressCount == 256)
	{
		sprintf(temp, "Active Cheats %u (Max Limit)", FrozenAddressCount);
		EnableWindow(GetDlgItem(hCheat, IDC_BTN_CHEAT_ADD), FALSE);
		EnableWindow(GetDlgItem(hCheat, IDC_BTN_CHEAT_ADDFROMFILE), FALSE);
	}
	else
	{
		sprintf(temp, "%u Error: Too many cheats loaded!", FrozenAddressCount);
		EnableWindow(GetDlgItem(hCheat, IDC_BTN_CHEAT_ADD), FALSE);
		EnableWindow(GetDlgItem(hCheat, IDC_BTN_CHEAT_ADDFROMFILE), FALSE);
	}
	SetDlgItemText(hCheat, IDC_GROUPBOX_CHEATLIST, temp);

	EnableWindow(GetDlgItem(hCheat, IDC_BTN_CHEAT_EXPORTTOFILE), cheats != 0);
}

//Used by cheats and external dialogs such as hex editor to update items in the cheat search dialog
void UpdateCheatsAdded()
{
	RedoCheatsLB(hCheat);
	UpdateCheatListGroupBoxUI();
}

INT_PTR CALLBACK GGConvCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

	switch(uMsg) {
		case WM_MOVE: {
			if (!IsIconic(hwndDlg)) {
			RECT wrect;
			GetWindowRect(hwndDlg,&wrect);
			GGConv_wndx = wrect.left;
			GGConv_wndy = wrect.top;

			#ifdef WIN32
			WindowBoundsCheckNoResize(GGConv_wndx,GGConv_wndy,wrect.right);
			#endif
			}
			break;
		};
		case WM_INITDIALOG:
		{
			POINT pt;
			if (GGConv_wndx != 0 && GGConv_wndy != 0)
			{
				pt.x = GGConv_wndx;
				pt.y = GGConv_wndy;
				pt = CalcSubWindowPos(hwndDlg, &pt);
			}
			else
				pt = CalcSubWindowPos(hwndDlg, NULL);

			GGConv_wndx = pt.x;
			GGConv_wndy = pt.y;

			// text limits;
			SendDlgItemMessage(hwndDlg, IDC_GAME_GENIE_CODE, EM_SETLIMITTEXT, 8, 0);
			SendDlgItemMessage(hwndDlg, IDC_GAME_GENIE_ADDR, EM_SETLIMITTEXT, 4, 0);
			SendDlgItemMessage(hwndDlg, IDC_GAME_GENIE_COMP, EM_SETLIMITTEXT, 2, 0);
			SendDlgItemMessage(hwndDlg, IDC_GAME_GENIE_VAL, EM_SETLIMITTEXT, 2, 0);

			// setup font
			SetupCheatFont(hwndDlg);

			SendDlgItemMessage(hwndDlg, IDC_GAME_GENIE_ADDR, WM_SETFONT, (WPARAM)hNewFont, FALSE);
			SendDlgItemMessage(hwndDlg, IDC_GAME_GENIE_COMP, WM_SETFONT, (WPARAM)hNewFont, FALSE);
			SendDlgItemMessage(hwndDlg, IDC_GAME_GENIE_VAL, WM_SETFONT, (WPARAM)hNewFont, FALSE);

			// limit their characters
			DefaultEditCtrlProc = (WNDPROC)SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_GAME_GENIE_CODE), GWLP_WNDPROC, (LONG_PTR)FilterEditCtrlProc);
			SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_GAME_GENIE_ADDR), GWLP_WNDPROC, (LONG_PTR)FilterEditCtrlProc);
			SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_GAME_GENIE_COMP), GWLP_WNDPROC, (LONG_PTR)FilterEditCtrlProc);
			SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_GAME_GENIE_VAL), GWLP_WNDPROC, (LONG_PTR)FilterEditCtrlProc);
		}
		break;
		case WM_CLOSE:
		case WM_QUIT:
			DestroyWindow(hGGConv);
			break;
		case WM_DESTROY:
			hGGConv = NULL;
			DeleteCheatFont();
		case WM_COMMAND:
			switch(HIWORD(wParam)) {
				case EN_UPDATE:
					if(dontupdateGG)break;
					dontupdateGG = 1;
					switch(LOWORD(wParam))
					{
						//lets find out what edit control got changed
						case IDC_GAME_GENIE_CODE: //The Game Genie Code - in this case decode it.
						{
							char buf[9];
							GetDlgItemText(hGGConv, IDC_GAME_GENIE_CODE, buf, 9);

							int a = -1, v = -1, c = -1;
							if (strlen(buf) == 6 || strlen(buf) == 8)
								FCEUI_DecodeGG(buf, &a, &v, &c);

							SetDlgItemText(hwndDlg, IDC_GAME_GENIE_ADDR, a == -1 ? "" : U16ToStr(a));
							SetDlgItemText(hwndDlg, IDC_GAME_GENIE_COMP, c == -1 ? "" : U8ToStr(c));
							SetDlgItemText(hwndDlg, IDC_GAME_GENIE_VAL, v == -1 ? "" : U8ToStr(v));
						}
						break;

						case IDC_GAME_GENIE_ADDR:
						case IDC_GAME_GENIE_COMP:
						case IDC_GAME_GENIE_VAL:

							uint32 a = -1; uint8 v = -1; int c = -1;
							GetUIGGInfo(hwndDlg, &a, &v, &c);
							
							char buf[9] = { 0 };
							if (a > 0x7FFF && v != -1)
								EncodeGG(buf, a, v, c);
							SetDlgItemText(hwndDlg, IDC_GAME_GENIE_CODE, buf);
							//ListGGAddresses();
							break;
						}
						ListGGAddresses(hwndDlg);
						dontupdateGG = 0;
					break;
					case BN_CLICKED:
						switch (LOWORD(wParam)) {
							case IDC_BTN_ADD_TO_CHEATS:
								//ConfigCheats(fceu_hInstance);

								char buf[9];
								uint32 a = -1; uint8 v = -1; int c = -1;
								GetUIGGInfo(hwndDlg, &a, &v, &c);
								GetDlgItemText(hwndDlg, IDC_GAME_GENIE_CODE, buf, 9);

								if(a < 0x8000) a += 0x8000;

								if (FCEUI_AddCheat(buf, a, v, c, 1) && hCheat) {
									RedoCheatsCallB(buf, a, v, c, 1, 1, NULL);
									int newselcheat = SendDlgItemMessage(hCheat, IDC_LIST_CHEATS, LVM_GETITEMCOUNT, 0, 0) - 1;
									ListView_MoveSelectionMark(GetDlgItem(hCheat, IDC_LIST_CHEATS), selcheat, newselcheat);
									selcheat = newselcheat;

									SetDlgItemText(hCheat, IDC_CHEAT_ADDR, (LPCSTR)U16ToStr(a));
									SetDlgItemText(hCheat, IDC_CHEAT_VAL, (LPCSTR)U8ToStr(v));
									SetDlgItemText(hCheat, IDC_CHEAT_COM, (LPCSTR)(c == -1 ? "" : U8ToStr(c)));

									EnableWindow(GetDlgItem(hCheat, IDC_BTN_CHEAT_DEL), TRUE);
									EnableWindow(GetDlgItem(hCheat, IDC_BTN_CHEAT_UPD), TRUE);

									UpdateCheatRelatedWindow();
									UpdateCheatListGroupBoxUI();
								}
						}
					break;
					case LBN_DBLCLK:
					switch (LOWORD(wParam)) {
						case IDC_LIST_GGADDRESSES:
							ChangeMemViewFocus(3,GGlist[SendDlgItemMessage(hwndDlg, IDC_LIST_GGADDRESSES, LB_GETCURSEL, 0, 0)],-1);
						break;
					}
					break;
				}
			break;
	}
	return FALSE;
}


//The code in this function is a modified version
//of Chris Covell's work - I'd just like to point that out
void EncodeGG(char *str, int a, int v, int c)
{
	uint8 num[8];
	int i;
	
	a&=0x7fff;

	num[0]=(v&7)+((v>>4)&8);
	num[1]=((v>>4)&7)+((a>>4)&8);
	num[2]=((a>>4)&7);
	num[3]=(a>>12)+(a&8);
	num[4]=(a&7)+((a>>8)&8);
	num[5]=((a>>8)&7);

	if (c == -1){
		num[5]+=v&8;
		for(i = 0;i < 6;i++)str[i] = GameGenieLetters[num[i]];
		str[6] = 0;
	} else {
		num[2]+=8;
		num[5]+=c&8;
		num[6]=(c&7)+((c>>4)&8);
		num[7]=((c>>4)&7)+(v&8);
		for(i = 0;i < 8;i++)str[i] = GameGenieLetters[num[i]];
		str[8] = 0;
	}
	return;
}

void ListGGAddresses(HWND hwndDlg)
{
	uint32 i, j = 0; //mbg merge 7/18/06 changed from int
	char str[20], code[9];
	SendDlgItemMessage(hwndDlg, IDC_LIST_GGADDRESSES, LB_RESETCONTENT,0,0);

	uint32 a = -1; uint8 v = -1; int c = -1;
	GetUIGGInfo(hwndDlg, &a, &v, &c);

	// also enable/disable the add GG button here
	GetDlgItemText(hwndDlg, IDC_GAME_GENIE_CODE, code, 9);
	EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_ADD_TO_CHEATS), a >= 0 && (strlen(code) == 6 || strlen(code) == 8));

	if (a != -1 && v != -1)
		for(i = 0; i < PRGsize[0]; i += 0x2000)
			if(c == -1 || PRGptr[0][i + (a & 0x1FFF)] == c){
				GGlist[j] = i + (a & 0x1FFF) + 0x10;
				if(++j > GGLISTSIZE)
					return;
				sprintf(str, "%06X", i + (a & 0x1FFF) + 0x10);
				SendDlgItemMessage(hwndDlg, IDC_LIST_GGADDRESSES, LB_ADDSTRING, 0, (LPARAM)str);
			}
}

//A different model for this could be to have everything
//set in the INITDIALOG message based on the internal
//variables, and have this simply call that.
void SetGGConvFocus(int address, int compare)
{
	char str[10];
	if(!hGGConv)
		DoGGConv();
	// GGaddr = address;
	// GGcomp = compare;

	dontupdateGG = 1; //little hack to fix a nasty bug

	sprintf(str, "%04X", address);
	SetDlgItemText(hGGConv, IDC_GAME_GENIE_ADDR, str);

	dontupdateGG = 0;

	sprintf(str, "%02X", compare);
	SetDlgItemText(hGGConv, IDC_GAME_GENIE_COMP, str);

	GetDlgItemText(hGGConv, IDC_GAME_GENIE_VAL, str, 3);
	uint8 val = StrToU8(str);

	if(val < 0)
		SetDlgItemText(hGGConv, IDC_GAME_GENIE_CODE, "");
	else {
		str[0] = 0;
		if (val > 0x7FFF)
			EncodeGG(str, address, val, compare);
		SetDlgItemText(hGGConv, IDC_GAME_GENIE_CODE, str);
	}

	SetFocus(GetDlgItem(hGGConv, IDC_GAME_GENIE_VAL));

	return;
}

void DoGGConv()
{
	if (hGGConv)
	{
		ShowWindow(hGGConv, SW_SHOWNORMAL);
		SetForegroundWindow(hGGConv);
	} else
		hGGConv = CreateDialog(fceu_hInstance,"GGCONV", hAppWnd, GGConvCallB);
}

inline void GetCheatStr(char* buf, int a, int v, int c)
{
	if (a > 0x7FFF)
		EncodeGG(buf, a, v, c);
	else {
		GetCheatCodeStr(buf, a, v, c);
	}
}

inline void GetCheatCodeStr(char* buf, int a, int v, int c)
{
	if (c == -1)
		sprintf(buf, "%04X:%02X", a, v);
	else
		sprintf(buf, "%04X?%02X:%02X", a, c, v);
}

static void SetCheatToolTip(HWND hwndDlg, UINT id)
{
	TOOLINFO info;
	memset(&info, 0, sizeof(TOOLINFO));
	info.cbSize = sizeof(TOOLINFO);
	info.uFlags = TTF_SUBCLASS | TTF_IDISHWND;
	info.hwnd = hwndDlg;
	info.lpszText = GetCheatToolTipStr(hwndDlg, id);
	info.uId = (UINT_PTR)GetDlgItem(hwndDlg, id);

	if (hCheatTip)
		SendMessage(hCheatTip, TTM_UPDATETIPTEXT, 0, (LPARAM)&info);
	else
	{
		if (hCheatTip = CreateWindow(TOOLTIPS_CLASS, NULL, WS_POPUP | TTS_ALWAYSTIP, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, hwndDlg, NULL, fceu_hInstance, NULL)) {
			SendMessage(hCheatTip, TTM_ADDTOOL, 0, (LPARAM)&info);
			SendMessage(hCheatTip, TTM_SETDELAYTIME, TTDT_AUTOPOP, 30000);
			SendMessage(hCheatTip, TTM_SETMAXTIPWIDTH, 0, 8000);
		}
	}
}

char* GetCheatToolTipStr(HWND hwndDlg, UINT id)
{
	switch (id)
	{
		case IDC_CHEAT_AUTOLOADSAVE:
			switch (disableAutoLSCheats)
			{
				case 0: return "Automatically load/save cheat file along with the game.";
				case 1: return
					"Don't add cheat on game load, but prompt for saving on game closes.\r\n"
					"You must manually import cht file when it's needed.";
				case 2: return
					"Don't add cheat on game load, and don't save cheat on game closes.\r\n"
					"You must manually import/export cht file by yourself,\nor all your changes to cheat will be lost!";
				default:
					return "Mysterious undocumented state.";
			}
	}

	return NULL;
}

void GetUICheatInfo(HWND hwndDlg, char* name, uint32* a, uint8* v, int* c)
{
	char buf[16];
	GetDlgItemText(hwndDlg, IDC_CHEAT_ADDR, buf, 5);
	*a = StrToU16(buf);
	GetDlgItemText(hwndDlg, IDC_CHEAT_VAL, buf, 3);
	*v = StrToU8(buf);
	GetDlgItemText(hwndDlg, IDC_CHEAT_COM, buf, 3);
	*c = (buf[0] == 0) ? -1 : StrToU8(buf);
	if (name)
		GetDlgItemText(hwndDlg, IDC_CHEAT_NAME, name, 256);
}

void GetUIGGInfo(HWND hwndDlg, uint32* a, uint8* v, int* c)
{
	char buf[16];
	GetDlgItemText(hwndDlg, IDC_GAME_GENIE_ADDR, buf, 5);
	*a = StrToU16(buf);
	GetDlgItemText(hwndDlg, IDC_GAME_GENIE_VAL, buf, 3);
	*v = StrToU8(buf);
	GetDlgItemText(hwndDlg, IDC_GAME_GENIE_COMP, buf, 3);
	*c = (buf[0] == 0 ? -1 : StrToU8(buf));
}

void DisableAllCheats()
{
	if(FCEU_DisableAllCheats() && hCheat){
		LVITEM lvi;
		lvi.mask = LVIF_STATE;
		lvi.stateMask = LVIS_STATEIMAGEMASK;
		lvi.state = INDEXTOSTATEIMAGEMASK(1);
		for (int current = SendDlgItemMessage(hCheat, IDC_LIST_CHEATS, LB_GETCOUNT, 0, 0) - 1; current >= 0; --current)
			SendDlgItemMessage(hCheat, IDC_LIST_CHEATS, LVM_GETITEMSTATE, current, (LPARAM)&lvi);
		UpdateCheatListGroupBoxUI();
	}	
}

void UpdateCheatRelatedWindow()
{
	// hex editor
	if (hMemView)
		UpdateColorTable(); //if the memory viewer is open then update any blue freeze locations in it as well

	// ram search
	extern HWND RamSearchHWnd;
	if (RamSearchHWnd)
		// if ram search is open then update the ram list.
		SendDlgItemMessage(RamSearchHWnd, IDC_RAMLIST, LVM_REDRAWITEMS, 
			SendDlgItemMessage(RamSearchHWnd, IDC_RAMLIST, LVM_GETTOPINDEX, 0, 0),
			SendDlgItemMessage(RamSearchHWnd, IDC_RAMLIST, LVM_GETTOPINDEX, 0, 0) + 
			SendDlgItemMessage(RamSearchHWnd, IDC_RAMLIST, LVM_GETCOUNTPERPAGE, 0, 0) + 1);

	// ram watch
	extern void UpdateWatchCheats();
	UpdateWatchCheats();
	extern HWND RamWatchHWnd;
	if (RamWatchHWnd)
		// if ram watch is open then update the ram list.
		SendDlgItemMessage(RamWatchHWnd, IDC_WATCHLIST, LVM_REDRAWITEMS,
			SendDlgItemMessage(RamWatchHWnd, IDC_WATCHLIST, LVM_GETTOPINDEX, 0, 0),
			SendDlgItemMessage(RamSearchHWnd, IDC_RAMLIST, LVM_GETTOPINDEX, 0, 0) + 
			SendDlgItemMessage(RamWatchHWnd, IDC_WATCHLIST, LVM_GETCOUNTPERPAGE, 0, 0) + 1);

}

bool ShowCheatFileBox(HWND hwnd, char* buf, bool save)
{
	if (!buf)
		return false;

	char filename[2048] = { 0 };

	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hInstance = fceu_hInstance;
	ofn.hwndOwner = hwnd;
	ofn.lpstrTitle = save ? "Save cheats file" : "Open cheats file";
	ofn.lpstrFilter = "Cheat files (*.cht)\0*.cht\0All Files (*.*)\0*.*\0\0";

	// I gave up setting the default filename for import cheat dialog, since the filename display contains a bug.
	if (save)
	{
		if (GameInfo)
		{
			char* _filename;
			if ((_filename = strrchr(GameInfo->filename, '\\')) || (_filename = strrchr(GameInfo->filename, '/')))
				strcpy(filename, _filename + 1);
			else
				strcpy(filename, GameInfo->filename);

			_filename = strrchr(filename, '.');
			if (_filename)
				strcpy(_filename, ".cht");
			else
				strcat(filename, ".cht");
		}
	}

	ofn.lpstrFile = filename;
	ofn.nMaxFile = sizeof(filename);
	ofn.lpstrDefExt = "cht";
	ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST;
	ofn.lpstrInitialDir = FCEU_GetPath(FCEUMKF_CHEAT).c_str();

	if (save ? GetSaveFileName(&ofn) : GetOpenFileName(&ofn))
	{
		strcpy(buf, filename);
		return true;
	}

	return false;
}

void AskSaveCheat()
{
	if (cheats)
	{
		HWND hwnd = hCheat ? hCheat : hAppWnd;
		if (MessageBox(hwnd, "Save cheats?", "Cheat Console", MB_YESNO | MB_ICONASTERISK) == IDYES)
			SaveCheatAs(hwnd, true);
	}
}


void SaveCheatAs(HWND hwnd, bool flush)
{
	if (cheats)
	{
		char filename[2048];
		if (ShowCheatFileBox(hwnd, filename, true))
		{
			FILE* file = FCEUD_UTF8fopen(filename, "wb");
			if (file)
			{
				if (flush)
				{
					savecheats = 1;
					FCEU_FlushGameCheats(file, 0);
				}
				else
					FCEU_SaveGameCheats(file);
				fclose(file);
			}
			else
				MessageBox(hwnd, "Error saving cheats!", "Cheat Console", MB_OK | MB_ICONERROR);
		}
	}
}

void SetupCheatFont(HWND hwnd)
{
	if (!hCheat && !hGGConv)
	{
		hFont = (HFONT)SendMessage(hwnd, WM_GETFONT, 0, 0);
		LOGFONT lf;
		GetObject(hFont, sizeof(LOGFONT), &lf);
		strcpy(lf.lfFaceName, "Courier New");
		hNewFont = CreateFontIndirect(&lf);
	}
}

void DeleteCheatFont()
{
	if (!hCheat && !hGGConv)
	{
		DeleteObject(hFont);
		DeleteObject(hNewFont);
		hFont = NULL;
		hNewFont = NULL;
	}
}

void CreateCheatMap()
{
	if (!CheatMapUsers)
		FCEUI_CreateCheatMap();
	++CheatMapUsers;
}

void ReleaseCheatMap()
{
	--CheatMapUsers;
	// printf("CheatMapUsers: %d\n", CheatMapUsers);
	if (!CheatMapUsers)
		FCEUI_ReleaseCheatMap();
}

