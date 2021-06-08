#include "common.h"
#include "mapinput.h"
#include "input.h"
#include "keyscan.h"
#include "keyboard.h"
#include "gui.h"
#include "../../input.h"
#include <commctrl.h>
#include "window.h"
#include "taseditor/taseditor_window.h"

// whitch column does the sort based on, the default status -1 is natrually not sorted (or sort by FCEUI_CommandTable)
static int mapInputSortCol = -1;
// whether it's asc or desc sorting, when sortCol is -1, this value has no effect
static bool mapInputSortAsc = true;

// an ugly macro function
#define UpdateListViewSort(hwndListView, lpListView, filter) \
if (filter == EMUCMDTYPE_MAX + 3) \
	lpListView->iSubItem = 2; \
else if (filter > EMUCMDTYPE_MISC && filter < EMUCMDTYPE_MAX || filter == EMUCMDTYPE_MAX + 2) \
	lpListView->iSubItem = 1; \
else \
	lpListView->iSubItem = mapInputSortCol; \
if (SendMessage(hwndListView, LVM_SORTITEMS, (WPARAM)lpListView, (LPARAM)MapInputItemSortFunc)) \
	UpdateSortColumnIcon(hwndListView, mapInputSortCol, mapInputSortAsc)


void KeyboardUpdateState(void); //mbg merge 7/17/06 yech had to add this

struct INPUTDLGTHREADARGS
{
	HANDLE hThreadExit;
	HWND hwndDlg;
};

static struct
{
	int cmd;
	int key;
} DefaultCommandMapping[]=
{
	{ EMUCMD_RESET, 					SCAN_R | CMD_KEY_CTRL, },
	{ EMUCMD_PAUSE, 					SCAN_PAUSE, },
	{ EMUCMD_FRAME_ADVANCE, 			SCAN_BACKSLASH, },
	{ EMUCMD_SCREENSHOT,				SCAN_F12, },
	{ EMUCMD_HIDE_MENU_TOGGLE,			SCAN_ESCAPE },
	{ EMUCMD_SPEED_SLOWER, 				SCAN_MINUS, }, 
	{ EMUCMD_SPEED_FASTER, 				SCAN_EQUAL, },
	{ EMUCMD_SPEED_TURBO, 				SCAN_TAB, }, 
	{ EMUCMD_SAVE_SLOT_0, 				SCAN_0, },
	{ EMUCMD_SAVE_SLOT_1, 				SCAN_1, },
	{ EMUCMD_SAVE_SLOT_2, 				SCAN_2, },
	{ EMUCMD_SAVE_SLOT_3, 				SCAN_3, },
	{ EMUCMD_SAVE_SLOT_4, 				SCAN_4, },
	{ EMUCMD_SAVE_SLOT_5, 				SCAN_5, },
	{ EMUCMD_SAVE_SLOT_6, 				SCAN_6, },
	{ EMUCMD_SAVE_SLOT_7, 				SCAN_7, },
	{ EMUCMD_SAVE_SLOT_8, 				SCAN_8, },
	{ EMUCMD_SAVE_SLOT_9, 				SCAN_9, },
	{ EMUCMD_SAVE_STATE, 				SCAN_I, }, //adelikat, set to my defaults for lack of something better
	{ EMUCMD_LOAD_STATE, 				SCAN_P, }, //most people use the loadslotx / savestlotx style system which requires hogging all th F Keys.
	{ EMUCMD_MOVIE_FRAME_DISPLAY_TOGGLE, 	SCAN_PERIOD, },
	{ EMUCMD_MOVIE_INPUT_DISPLAY_TOGGLE, 	SCAN_COMMA, },
	{ EMUCMD_MISC_DISPLAY_LAGCOUNTER_TOGGLE, 	SCAN_SLASH, },
	{ EMUCMD_MOVIE_READONLY_TOGGLE, 	SCAN_Q, },
	{ EMUCMD_SAVE_STATE_SLOT_0, 		SCAN_F10 | CMD_KEY_SHIFT, },
	{ EMUCMD_SAVE_STATE_SLOT_1, 		SCAN_F1 | CMD_KEY_SHIFT, },
	{ EMUCMD_SAVE_STATE_SLOT_2, 		SCAN_F2 | CMD_KEY_SHIFT, },
	{ EMUCMD_SAVE_STATE_SLOT_3, 		SCAN_F3 | CMD_KEY_SHIFT, },
	{ EMUCMD_SAVE_STATE_SLOT_4, 		SCAN_F4 | CMD_KEY_SHIFT, },
	{ EMUCMD_SAVE_STATE_SLOT_5, 		SCAN_F5 | CMD_KEY_SHIFT, },
	{ EMUCMD_SAVE_STATE_SLOT_6, 		SCAN_F6 | CMD_KEY_SHIFT, },
	{ EMUCMD_SAVE_STATE_SLOT_7, 		SCAN_F7 | CMD_KEY_SHIFT, },
	{ EMUCMD_SAVE_STATE_SLOT_8, 		SCAN_F8 | CMD_KEY_SHIFT, },
	{ EMUCMD_SAVE_STATE_SLOT_9, 		SCAN_F9 | CMD_KEY_SHIFT, },
	{ EMUCMD_LOAD_STATE_SLOT_0, 		SCAN_F10, },
	{ EMUCMD_LOAD_STATE_SLOT_1, 		SCAN_F1, },
	{ EMUCMD_LOAD_STATE_SLOT_2, 		SCAN_F2, },
	{ EMUCMD_LOAD_STATE_SLOT_3, 		SCAN_F3, },
	{ EMUCMD_LOAD_STATE_SLOT_4, 		SCAN_F4, },
	{ EMUCMD_LOAD_STATE_SLOT_5, 		SCAN_F5, },
	{ EMUCMD_LOAD_STATE_SLOT_6, 		SCAN_F6, },
	{ EMUCMD_LOAD_STATE_SLOT_7, 		SCAN_F7, },
	{ EMUCMD_LOAD_STATE_SLOT_8, 		SCAN_F8, },
	{ EMUCMD_LOAD_STATE_SLOT_9, 		SCAN_F9, },
	{ EMUCMD_MOVIE_PLAY_FROM_BEGINNING, SCAN_R | CMD_KEY_SHIFT,		},
	{ EMUCMD_SCRIPT_RELOAD,				SCAN_L | CMD_KEY_SHIFT,		},
	{ EMUCMD_OPENROM,					SCAN_O | CMD_KEY_CTRL,	    },
	{ EMUCMD_CLOSEROM,					SCAN_W | CMD_KEY_CTRL,		},
	{ EMUCMD_RELOAD,					SCAN_F1 | CMD_KEY_CTRL ,	},
	{ EMUCMD_MISC_UNDOREDOSAVESTATE,	SCAN_Z | CMD_KEY_CTRL, },
	{ EMUCMD_MISC_TOGGLEFULLSCREEN,		SCAN_ENTER | CMD_KEY_ALT, },
	{ EMUCMD_RERECORD_DISPLAY_TOGGLE,	SCAN_M,	},
	{ EMUCMD_TASEDITOR_REWIND,			SCAN_BACKSPACE, },
	{ EMUCMD_TASEDITOR_RESTORE_PLAYBACK,	SCAN_SPACE,	},
	{ EMUCMD_TASEDITOR_CANCEL_SEEKING,	SCAN_ESCAPE, },
	{ EMUCMD_TASEDITOR_SWITCH_AUTORESTORING,	SCAN_SPACE | CMD_KEY_CTRL, },
	{ EMUCMD_TASEDITOR_SWITCH_MULTITRACKING,	SCAN_W, },
};

#define NUM_DEFAULT_MAPPINGS		(sizeof(DefaultCommandMapping)/sizeof(DefaultCommandMapping[0]))

/**
* Checks whether a key should be shown in the current filter mode.
*
* @param mapn Index of the key.
* @param filter Current filter.
* @param conflictTable A populated conflict table.
*
* @return Key should be shown (true) or not shown (false)
**/
bool ShouldDisplayMapping(int mapn, int filter, const int* conflictTable)
{
	//mbg merge 7/17/06 changed to if..elseif
	if(filter == 0) /* No filter */
	{
		return true;
	}
	else if(filter <= EMUCMDTYPE_MAX) /* Filter by type */
	{
		return FCEUI_CommandTable[mapn].type == filter - 1;
	}
	else if(filter == EMUCMDTYPE_MAX + 1) /* Assigned */
	{
		return FCEUD_CommandMapping[FCEUI_CommandTable[mapn].cmd].NumC != 0;
	}
	else if(filter == EMUCMDTYPE_MAX + 2) /* Unassigned */
	{
		return FCEUD_CommandMapping[FCEUI_CommandTable[mapn].cmd].NumC == 0;
	}
	else if(filter == EMUCMDTYPE_MAX + 3) /* Conflicts */
	{
		return conflictTable[FCEUI_CommandTable[mapn].cmd] != 0;
	}
	else 
	{
		return 0;
	}
}

/**
* Populates a conflict table.
*
* @param conflictTable The conflict table to populate.
**/
void PopulateConflictTable(int* conflictTable)
{
	// Check whether there are conflicts between the
	// selected hotkeys.
	for(unsigned i = 0; i < EMUCMD_MAX - 1; ++i)
	{
		for(unsigned int j = i + 1; j < EMUCMD_MAX; ++j)
		{
			bool hasConflicsts = false;
			for (unsigned int x = 0; x < FCEUD_CommandMapping[i].NumC; ++x)
			{
				for (unsigned int y = 0; y < FCEUD_CommandMapping[j].NumC; ++y)
				{
					if ((FCEUI_CommandTable[i].flags & EMUCMDFLAG_TASEDITOR) == (FCEUI_CommandTable[j].flags & EMUCMDFLAG_TASEDITOR)
						&& FCEUD_CommandMapping[i].ButtType[x] == FCEUD_CommandMapping[j].ButtType[y]
						&& FCEUD_CommandMapping[i].DeviceNum[x] == FCEUD_CommandMapping[j].DeviceNum[y]
						&& FCEUD_CommandMapping[i].ButtonNum[x] == FCEUD_CommandMapping[j].ButtonNum[y])
					{
						hasConflicsts = true;
						break;
					}
				}
				if (hasConflicsts) break;
			}
			if (hasConflicsts)
			{
				conflictTable[i] = 1;
				conflictTable[j] = 1;
			}
		}
	}
}

/**
* Populates the hotkey ListView according to the rules set
* by the current filter.
*
* @param hwndDlg Handle of the Map Input dialog.
**/
void PopulateMappingDisplay(HWND hwndDlg)
{
	// Get the ListView handle.
	HWND hwndListView = GetDlgItem(hwndDlg, LV_MAPPING);

	// Get the number of items in the ListView.
	int num = SendMessage(hwndListView, LVM_GETITEMCOUNT, 0, 0);

	// Get the selected filter.
	int filter = (int)SendDlgItemMessage(hwndDlg, COMBO_FILTER, CB_GETCURSEL, 0, 0);

	int* conflictTable = 0;

	// Get the filter type.
	if (filter == EMUCMDTYPE_MAX + 3)
	{
		// Set up the conflict table.
		conflictTable = (int*)malloc(sizeof(int)*EMUCMD_MAX);
		memset(conflictTable, 0, sizeof(int)*EMUCMD_MAX);

		PopulateConflictTable(conflictTable);
	}

	LVITEM lvi;


	int newItemCount = 0;

	// Populate display.
	for(int i = 0, idx = 0; i < EMUCMD_MAX; ++i)
	{
		// Check if the current key should be displayed
		// according to the current filter.
		if(ShouldDisplayMapping(i, filter, conflictTable))
		{
			memset(&lvi, 0, sizeof(lvi));
			lvi.mask = LVIF_TEXT | LVIF_PARAM;
			lvi.iItem = idx;
			lvi.iSubItem = 0;
			lvi.pszText = (char*)FCEUI_CommandTypeNames[FCEUI_CommandTable[i].type];
			lvi.lParam = (LPARAM)FCEUI_CommandTable[i].cmd;

			if(newItemCount<num)
				SendMessage(hwndListView, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvi);
			else
				SendMessage(hwndListView, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvi);

			newItemCount++;

			memset(&lvi, 0, sizeof(lvi));
			lvi.mask = LVIF_TEXT;
			lvi.iItem = idx;
			lvi.iSubItem = 1;
			lvi.pszText = (char*)FCEUI_CommandTable[i].name;

			SendMessage(hwndListView, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvi);

			memset(&lvi, 0, sizeof(lvi));
			lvi.mask = LVIF_TEXT;
			lvi.iItem = idx;
			lvi.iSubItem = 2;
			lvi.pszText = MakeButtString(&FCEUD_CommandMapping[FCEUI_CommandTable[i].cmd], 0);
			SendMessage(hwndListView, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvi);
			free(lvi.pszText);

			idx++;
		}
	}

	//delete unneeded items
	for(int i=newItemCount;i<num;i++)
	{
		SendMessage(hwndListView, LVM_DELETEITEM, (WPARAM)(newItemCount), 0);
	}

	if(conflictTable)
	{
		free(conflictTable);
	}

	if (mapInputSortCol != -1)
	{
		NMHDR hdr;
		hdr.hwndFrom = hwndListView;
		hdr.code = LVN_COLUMNCLICK;

		NMLISTVIEW* lpListView = new NMLISTVIEW;
		lpListView->hdr = hdr;

		UpdateListViewSort(hwndListView, lpListView, filter);

		delete lpListView;
	}
}

/**
* Initializes the ListView control that shows the key mappings.
*
* @param The handle of the Map Input dialog.
*
* @return The handle of the ListView control.
**/
HWND InitializeListView(HWND hwndDlg)
{
	HWND hwndListView = GetDlgItem(hwndDlg, LV_MAPPING);
	LVCOLUMN lv;

	// Set full row select.
	SendMessage(hwndListView, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

	// Init ListView columns.
	memset(&lv, 0, sizeof(lv));
	lv.mask = LVCF_TEXT | LVCF_WIDTH;
	lv.pszText = "Type";
	lv.cx = 80;

	SendMessage(hwndListView, LVM_INSERTCOLUMN, (WPARAM)0, (LPARAM)&lv);

	lv.pszText = "Command";
	lv.cx = 240;

	SendMessage(hwndListView, LVM_INSERTCOLUMN, (WPARAM)1, (LPARAM)&lv);

	lv.mask ^= LVCF_WIDTH;
	lv.fmt = LVCFMT_LEFT;
	lv.pszText = "Input";

	SendMessage(hwndListView, LVM_INSERTCOLUMN, (WPARAM)2, (LPARAM)&lv);

	return hwndListView;
}

/**
* Fills the filter combobox.
*
* @param hwndDlg Handle of the Map Input dialog.
**/
void InitFilterComboBox(HWND hwndDlg)
{
	// Populate the filter combobox.
	SendDlgItemMessage(hwndDlg, COMBO_FILTER, CB_INSERTSTRING, 0, (LPARAM)"None");

	unsigned i;

	for(i = 0; i < EMUCMDTYPE_MAX; ++i)
	{
		SendDlgItemMessage(hwndDlg, COMBO_FILTER, CB_INSERTSTRING, i+1, (LPARAM)FCEUI_CommandTypeNames[i]);
	}

	SendDlgItemMessage(hwndDlg, COMBO_FILTER, CB_INSERTSTRING, ++i, (LPARAM)"Assigned");
	SendDlgItemMessage(hwndDlg, COMBO_FILTER, CB_INSERTSTRING, ++i, (LPARAM)"Unassigned");
	SendDlgItemMessage(hwndDlg, COMBO_FILTER, CB_INSERTSTRING, ++i, (LPARAM)"Conflicts");

	// Default filter is "none".
	SendDlgItemMessage(hwndDlg, COMBO_FILTER, CB_SETCURSEL, 0, 0);
}

/**
* Checks what ListView line was selected and shows the dialog
* that prompts the user to enter a hotkey.
**/
void AskForHotkey(HWND hwndDlg, HWND hwndListView)
{
	int nSel = SendMessage(hwndListView, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);

	if (nSel != -1)
	{
		LVITEM lvi;

		// Get the corresponding input
		memset(&lvi, 0, sizeof(lvi));
		lvi.mask = LVIF_PARAM;
		lvi.iItem = nSel;
		lvi.iSubItem = 0;

		SendMessage(hwndListView, LVM_GETITEM, 0, (LPARAM)&lvi);

		int nCmd = lvi.lParam;

		DWaitButton(hwndDlg, FCEUI_CommandTable[nCmd].name, &FCEUD_CommandMapping[nCmd]);
		
		memset(&lvi, 0, sizeof(lvi));
		lvi.mask = LVIF_TEXT;
		lvi.iItem = nSel;
		lvi.iSubItem = 2;
		lvi.pszText = MakeButtString(&FCEUD_CommandMapping[nCmd], 0);
		SendMessage(hwndListView, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvi);
		free(lvi.pszText);
	}
}

/**
* Restores the default hotkey mapping.
**/
void ApplyDefaultCommandMapping()
{
	memset(FCEUD_CommandMapping, 0, sizeof(FCEUD_CommandMapping));

	for(unsigned i = 0; i < NUM_DEFAULT_MAPPINGS; ++i)
	{
		FCEUD_CommandMapping[DefaultCommandMapping[i].cmd].ButtType[0] = BUTTC_KEYBOARD;
		FCEUD_CommandMapping[DefaultCommandMapping[i].cmd].DeviceNum[0] = 0;
		FCEUD_CommandMapping[DefaultCommandMapping[i].cmd].ButtonNum[0] = DefaultCommandMapping[i].key;
		FCEUD_CommandMapping[DefaultCommandMapping[i].cmd].NumC = 1;
	}
}

/**
* Callback function of the Map Input dialog
**/
INT_PTR CALLBACK MapInputDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			HWND hwndListView = InitializeListView(hwndDlg);

			InitFilterComboBox(hwndDlg);

			// Now populate the mapping display.
			PopulateMappingDisplay(hwndDlg);

			// Autosize last column.
			SendMessage(hwndListView, LVM_SETCOLUMNWIDTH, (WPARAM)2, MAKELPARAM(LVSCW_AUTOSIZE, 0));
			SendMessage(hwndListView, LVM_SETCOLUMNWIDTH, (WPARAM)1, MAKELPARAM(LVSCW_AUTOSIZE, 0));
			RECT rect;
			GetClientRect(hwndListView, &rect);
			SendMessage(hwndListView, LVM_SETCOLUMNWIDTH, (WPARAM)2, MAKELPARAM(rect.right - rect.left - SendMessage(hwndListView, LVM_GETCOLUMNWIDTH, 0, 0) - SendMessage(hwndListView, LVM_GETCOLUMNWIDTH, 1, 0), 0));

			CenterWindowOnScreen(hwndDlg);
		}

		return FALSE;

	case WM_COMMAND:

		if(HIWORD(wParam) == CBN_SELCHANGE)
		{
			PopulateMappingDisplay(hwndDlg);
			break;
		}
		
		switch(LOWORD(wParam))
		{
			case IDOK:
				EndDialog(hwndDlg, 1);
				// Update TAS Editor's tooltips if it's opening.
				extern TASEDITOR_WINDOW taseditorWindow;
				if (taseditorWindow.hwndTASEditor)
					taseditorWindow.updateTooltips();
				// main menu is always shown, so we only need to update it,
				// the context menus are updated only when they are created
				UpdateMenuHotkeys(FCEUMENU_MAIN);
				return TRUE;

			case BTN_CANCEL:  // here true cause of ESC button handling as EXIT ;)
				EndDialog(hwndDlg, 0);
				return TRUE;

			case BTN_RESTORE_DEFAULTS:
				ApplyDefaultCommandMapping();
				PopulateMappingDisplay(hwndDlg);
				return TRUE;

			default:
				break;
		}
		break;
		
	case WM_CLOSE:
		EndDialog(hwndDlg, 0);
		break;
	case WM_DESTROY:
	case WM_QUIT:
			break;
	case WM_NOTIFY:

		switch(LOWORD(wParam))
		{
			case LV_MAPPING:
				if (lParam)
				{
					NMLISTVIEW* lpListView = (NMLISTVIEW*)lParam;
					UINT code = lpListView->hdr.code;
					HWND hwndListView = lpListView->hdr.hwndFrom;
					switch (code)
					{
						case LVN_ITEMACTIVATE:
							AskForHotkey(hwndDlg, hwndListView);

							// TODO: Only redraw if Conflicts filter
							// is active.
							PopulateMappingDisplay(hwndDlg);
							break;
						case LVN_COLUMNCLICK:
							// Sort the items
							if (mapInputSortCol != lpListView->iSubItem) {
								mapInputSortCol = lpListView->iSubItem;
								mapInputSortAsc = true;
							}
							else
								mapInputSortAsc = !mapInputSortAsc;

							int filter = (int)SendDlgItemMessage(hwndDlg, COMBO_FILTER, CB_GETCURSEL, 0, 0);
							UpdateListViewSort(hwndListView, lpListView, filter);
					}
					return TRUE;
				}
				break;
			default:
				break;
		}
		break;

	default:
		break;
	}

	return FALSE;
}

/**
* Show input mapping configuration dialog.
**/
void MapInput(void)
{
	// Make a backup of the current mappings, in case the user changes their mind.
	int* backupmapping = (int*)malloc(sizeof(FCEUD_CommandMapping));
	memcpy(backupmapping, FCEUD_CommandMapping, sizeof(FCEUD_CommandMapping));

	if(!DialogBox(fceu_hInstance, "MAPINPUT", hAppWnd, MapInputDialogProc))
	{
		memcpy(FCEUD_CommandMapping, backupmapping, sizeof(FCEUD_CommandMapping));
	}

	free(backupmapping);
}

static int CALLBACK MapInputItemSortFunc(LPARAM lp1, LPARAM lp2, LPARAM lpSort)
{
	NMLISTVIEW* lpListView = (NMLISTVIEW*)lpSort;
	HWND hwndListView = lpListView->hdr.hwndFrom;
	
	LVFINDINFO info1, info2;
	LVITEM item1, item2;
	memset(&info1, 0, sizeof(LVFINDINFO));
	memset(&info2, 0, sizeof(LVFINDINFO));
	memset(&item1, 0, sizeof(LVITEM));
	memset(&item2, 0, sizeof(LVITEM));

	info1.flags = LVFI_PARAM;
	info2.flags = LVFI_PARAM;
	info1.lParam = lp1;
	info2.lParam = lp2;

	int index1 = SendMessage(hwndListView, LVM_FINDITEM, -1, (LPARAM)&info1);
	int index2 = SendMessage(hwndListView, LVM_FINDITEM, -1, (LPARAM)&info2);

	item1.pszText = new char[64];
	item2.pszText = new char[64];
	item1.cchTextMax = 64;
	item2.cchTextMax = 64;
	item1.iSubItem = lpListView->iSubItem;
	item2.iSubItem = lpListView->iSubItem;

	SendMessage(hwndListView, LVM_GETITEMTEXT, index1, (LPARAM)&item1);
	SendMessage(hwndListView, LVM_GETITEMTEXT, index2, (LPARAM)&item2);

	int ret = strcmp(item1.pszText, item2.pszText);
	// "Type" and "Input" column can have repeated items, uses "Command" line for another sorting  
	if (ret == 0 && (lpListView->iSubItem == 0 || lpListView->iSubItem == 2))
	{
		item1.iSubItem = 1;
		item2.iSubItem = 1;
		SendMessage(hwndListView, LVM_GETITEMTEXT, index1, (LPARAM)&item1);
		SendMessage(hwndListView, LVM_GETITEMTEXT, index2, (LPARAM)&item2);
		ret = strcmp(item1.pszText, item2.pszText);
	}

	if (!mapInputSortAsc)
		ret = -ret;
	delete[] item1.pszText;
	delete[] item2.pszText;

	return ret;
}
