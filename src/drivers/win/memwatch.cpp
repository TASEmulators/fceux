/* FCE Ultra - NES/Famicom Emulator
*
* Copyright notice for this file:
*  Copyright (C) 2006 Luke Gustafson
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
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "common.h"
#include "../../fceu.h"
#include "memwatch.h"
#include "../../debug.h"
#include "debugger.h"
#include "../../utils/xstring.h"

const int NUMWATCHES = 24;
const int LABELLENGTH = 64;
const int ADDRESSLENGTH = 16;

int MemWatch_wndx=0, MemWatch_wndy=0;
static HDC hdc;
HWND hwndMemWatch=0;
static char addresses[NUMWATCHES][ADDRESSLENGTH];
static char labels[NUMWATCHES][LABELLENGTH];
static int NeedsInit = 1;
char *MemWatchDir = 0;
char memwLastFilename[2048];
bool fileChanged = false;
bool MemWatchLoadOnStart = false;
bool MemWatchLoadFileOnStart = false;
static HMENU memwmenu = 0;

char *memw_recent_files[] = { 0 ,0 ,0 ,0 ,0 };

const unsigned int MEMW_MENU_FIRST_RECENT_FILE = 600;
const unsigned int MEMW_MAX_NUMBER_OF_RECENT_FILES = sizeof(memw_recent_files)/sizeof(*memw_recent_files);

static HMENU memwrecentmenu, memwrecentdmenu;

void UpdateMemw_RMenu(HMENU menu, char **strs, unsigned int mitem, unsigned int baseid)
{
	MENUITEMINFO moo;
	int x;

	moo.cbSize = sizeof(moo);
	moo.fMask = MIIM_SUBMENU | MIIM_STATE;

	GetMenuItemInfo(GetSubMenu(memwmenu, 0), mitem, FALSE, &moo);
	moo.hSubMenu = menu;
	moo.fState = strs[0] ? MFS_ENABLED : MFS_GRAYED;

	SetMenuItemInfo(GetSubMenu(memwmenu, 0), mitem, FALSE, &moo);

	// Remove all recent files submenus
	for(x = 0; x < MEMW_MAX_NUMBER_OF_RECENT_FILES; x++)
	{
		RemoveMenu(menu, baseid + x, MF_BYCOMMAND);
	}

	// Recreate the menus
	for(x = MEMW_MAX_NUMBER_OF_RECENT_FILES - 1; x >= 0; x--)
	{  
		char tmp[128 + 5];

		// Skip empty strings
		if(!strs[x])
		{
			continue;
		}

		moo.cbSize = sizeof(moo);
		moo.fMask = MIIM_DATA | MIIM_ID | MIIM_TYPE;

		// Fill in the menu text.
		if(strlen(strs[x]) < 128)
		{
			sprintf(tmp, "&%d. %s", ( x + 1 ) % 10, strs[x]);
		}
		else
		{
			sprintf(tmp, "&%d. %s", ( x + 1 ) % 10, strs[x] + strlen( strs[x] ) - 127);
		}

		// Insert the menu item
		moo.cch = strlen(tmp);
		moo.fType = 0;
		moo.wID = baseid + x;
		moo.dwTypeData = tmp;
		InsertMenuItem(menu, 0, 1, &moo);
	}

	DrawMenuBar(hAppWnd);
}



void UpdateMemwRecentArray(const char* addString, char** bufferArray, unsigned int arrayLen, HMENU menu, unsigned int menuItem, unsigned int baseId)
{
	// Try to find out if the filename is already in the recent files list.
	for(unsigned int x = 0; x < arrayLen; x++)
	{
		if(bufferArray[x])
		{
			if(!strcmp(bufferArray[x], addString))    // Item is already in list.
			{
				// If the filename is in the file list don't add it again.
				// Move it up in the list instead.

				int y;
				char *tmp;

				// Save pointer.
				tmp = bufferArray[x];
				
				for(y = x; y; y--)
				{
					// Move items down.
					bufferArray[y] = bufferArray[y - 1];
				}

				// Put item on top.
				bufferArray[0] = tmp;

				// Update the recent files menu
				UpdateMemw_RMenu(menu, bufferArray, menuItem, baseId);

				return;
			}
		}
	}

	// The filename wasn't found in the list. That means we need to add it.

	// If there's no space left in the recent files list, get rid of the last
	// item in the list.
	if(bufferArray[arrayLen - 1])
	{
		free(bufferArray[arrayLen - 1]);
	}

	// Move the other items down.
	for(unsigned int x = arrayLen - 1; x; x--)
	{
		bufferArray[x] = bufferArray[x - 1];
	}

	// Add the new item.
	bufferArray[0] = (char*)malloc(strlen(addString) + 1); //mbg merge 7/17/06 added cast
	strcpy(bufferArray[0], addString);

	// Update the recent files menu
	UpdateMemw_RMenu(menu, bufferArray, menuItem, baseId);
}

/**
* Add a filename to the recent files list.
*
* @param filename Name of the file to add.
**/
void MemwAddRecentFile(const char *filename)
{
	UpdateMemwRecentArray(filename, memw_recent_files, MEMW_MAX_NUMBER_OF_RECENT_FILES, memwrecentmenu, ID_FILE_RECENT, MEMW_MENU_FIRST_RECENT_FILE);
}

static const int MW_ADDR_Lookup[] = {
	MW_ADDR00,MW_ADDR01,MW_ADDR02,MW_ADDR03,
	MW_ADDR04,MW_ADDR05,MW_ADDR06,MW_ADDR07,
	MW_ADDR08,MW_ADDR09,MW_ADDR10,MW_ADDR11,
	MW_ADDR12,MW_ADDR13,MW_ADDR14,MW_ADDR15,
	MW_ADDR16,MW_ADDR17,MW_ADDR18,MW_ADDR19,
	MW_ADDR20,MW_ADDR21,MW_ADDR22,MW_ADDR23
};
inline int MW_ADDR(int i) { return MW_ADDR_Lookup[i]  ; }
inline int MW_NAME(int i) { return MW_ADDR_Lookup[i]-1; }
inline int MW_VAL (int i) { return MW_ADDR_Lookup[i]+1; }

static const int MWNUM = ARRAY_SIZE(MW_ADDR_Lookup);

static int yPositions[MWNUM];
static int xPositions[MWNUM];

static struct MWRec
{
	static int findIndex(WORD ctl)
	{
		for(int i=0;i<MWNUM;i++)
			if(MW_ADDR(i) == ctl)
				return i;
		return -1;
	}

	void parse(WORD ctl)
	{
		char TempArray[16];
		GetDlgItemText(hwndMemWatch,ctl,TempArray,16);
		TempArray[15]=0;

		valid = hex = twobytes = false;
		switch(TempArray[0])
		{
			case 0:
				break;
			case '!':
				twobytes=true;
				addr=FastStrToU16(TempArray+1,valid);
				break;
			case 'x':
				hex = true;
				valid = true;
				addr=FastStrToU16(TempArray+1,valid);
				break;
			case 'X':
				hex = twobytes = true;
				valid = true;
				addr = FastStrToU16(TempArray+1,valid);
				break;
			default:
				valid = true;
				addr=FastStrToU16(TempArray,valid);
				break;
			}
	}
	bool valid, twobytes, hex;
	uint16 addr;
} mwrecs[MWNUM];

//Update the values in the Memory Watch window
void UpdateMemWatch()
{
	if(hwndMemWatch)
	{
		SetTextColor(hdc,RGB(0,0,0));
		SetBkColor(hdc,GetSysColor(COLOR_3DFACE));

		for(int i = 0; i < MWNUM; i++)
		{
			MWRec& mwrec = mwrecs[i];

			char* text;
			if(mwrec.valid && GameInfo)
			{
				if(mwrec.hex)
				{
					if(mwrec.twobytes)
					{
						text = U16ToHexStr(GetMem(mwrec.addr)+(GetMem(mwrec.addr+1)<<8));
					}
					else
					{
						text = U8ToHexStr(GetMem(mwrec.addr));
					}
				}
				else
				{
					if(mwrec.twobytes)
					{
						text = U16ToDecStr(GetMem(mwrec.addr)+(GetMem(mwrec.addr+1)<<8));
					}
					else
					{
						text = U8ToDecStr(GetMem(mwrec.addr));
					}
				}
			}
			else
			{
				text = "-";
			}

			MoveToEx(hdc,xPositions[i],yPositions[i],NULL);
			TextOut(hdc,0,0,text,strlen(text));
		}
	}
}

//Save labels/addresses so next time dialog is opened,
//you don't lose what you've entered.
static void SaveStrings()
{
	int i;
	for(i=0;i<MWNUM;i++)
	{
		GetDlgItemText(hwndMemWatch,MW_ADDR(i),addresses[i],ADDRESSLENGTH);
		GetDlgItemText(hwndMemWatch,MW_NAME(i),labels[i],LABELLENGTH);
	}
}

//replaces spaces with a dummy character
static void TakeOutSpaces(int i)
{
	int j;
	for(j=0;j<ADDRESSLENGTH;j++)
	{
		if(addresses[i][j] == ' ') addresses[i][j] = '|';
		if(labels[i][j] == ' ') labels[i][j] = '|';
	}
	for(;j<LABELLENGTH;j++)
	{
		if(labels[i][j] == ' ') labels[i][j] = '|';
	}
}

//replaces dummy characters with spaces
static void PutInSpaces(int i)
{
	int j;
	for(j=0;j<ADDRESSLENGTH;j++)
	{
		if(addresses[i][j] == '|') addresses[i][j] = ' ';
		if(labels[i][j] == '|') labels[i][j] = ' ';
	}
	for(;j<LABELLENGTH;j++)
	{
		if(labels[i][j] == '|') labels[i][j] = ' ';
	}
}


bool iftextchanged()
{
//Decides if any edit box has anything	
	int i,j;
	for(i=0;i<NUMWATCHES;i++)
	{
		for(j=0;j<LABELLENGTH;j++)
		{
			if(addresses[i][j] != NULL || labels [i][j] != NULL)
				return true;
		}
		for(;j<LABELLENGTH;j++)
		{
		if(labels[i][j] != NULL)
			return true;
		}
	}
return false;
}

//Saves all the addresses and labels to disk
static void SaveMemWatch()
{
	const char filter[]="Memory address list(*.txt)\0*.txt\0";
	 
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Save Memory Watch As...";
	ofn.lpstrFilter=filter;
	memwLastFilename[0]=0;
	ofn.lpstrFile=memwLastFilename;
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
	std::string initdir =  FCEU_GetPath(FCEUMKF_MEMW);
	ofn.lpstrInitialDir=initdir.c_str();
	if(GetSaveFileName(&ofn))
	{
		int i;

		//Save the directory
		if(ofn.nFileOffset < 1024)
		{
			free(MemWatchDir);
			MemWatchDir=(char*)malloc(strlen(ofn.lpstrFile)+1);
			strcpy(MemWatchDir,ofn.lpstrFile);
			MemWatchDir[ofn.nFileOffset]=0;
		}

		//quick get length of memwLastFilename
		for(i=0;i<2048;i++)
		{
			if(memwLastFilename[i] == 0)
			{
				break;
			}
		}

		//add .txt if memwLastFilename doesn't have it
		if((i < 4 || memwLastFilename[i-4] != '.') && i < 2040)
		{
			memwLastFilename[i] = '.';
			memwLastFilename[i+1] = 't';
			memwLastFilename[i+2] = 'x';
			memwLastFilename[i+3] = 't';
			memwLastFilename[i+4] = 0;
		}
		
		SaveStrings();
		FILE *fp=FCEUD_UTF8fopen(memwLastFilename,"w");
		for(i=0;i<NUMWATCHES;i++)
		{
			//Use dummy strings to fill empty slots
			if(labels[i][0] == 0)
			{
				labels[i][0] = '|';
				labels[i][1] = 0;
			}
			if(addresses[i][0] == 0)
			{
				addresses[i][0] = '|';
				addresses[i][1] = 0;
			}
			//spaces can be a problem for scanf so get rid of them
			TakeOutSpaces(i);
			fprintf(fp, "%s %s\n", addresses[i], labels[i]);
			PutInSpaces(i);
		}
		fileChanged=false;
		fclose(fp);
	}
}

static void QuickSaveMemWatch() //Save rather than Save as
{
	if (fileChanged==false) //Checks if current watch has been changed, if so quick save has no effect.
		return;	
	if (memwLastFilename[0]!=NULL) // Check if there is there something to save else default to save as
	{
		SaveStrings();
		FILE *fp=FCEUD_UTF8fopen(memwLastFilename,"w");
		for(int i=0;i<NUMWATCHES;i++)
		{
			//Use dummy strings to fill empty slots
			if(labels[i][0] == 0)
			{
				labels[i][0] = '|';
				labels[i][1] = 0;
			}
			if(addresses[i][0] == 0)
			{
				addresses[i][0] = '|';
				addresses[i][1] = 0;
			}
			//spaces can be a problem for scanf so get rid of them
			TakeOutSpaces(i);
			fprintf(fp, "%s %s\n", addresses[i], labels[i]);
			PutInSpaces(i);
			
		}
		fileChanged = false;
		fclose (fp);
	}
	else
		SaveMemWatch();
}

//Loads a previously saved file
static void LoadMemWatch()
{
	const char filter[]="Memory address list(*.txt)\0*.txt\0";
	char watchfcontents[2048]; 
	
	//Now clear last file used variable (memwLastFilename)
	//This might be unecessary
		for (int i=0;i<2048;i++)
		{
			memwLastFilename[i] = NULL;
		}
	
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Load Memory Watch...";
	ofn.lpstrFilter=filter;
	memwLastFilename[0]=0;
	ofn.lpstrFile=memwLastFilename;
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
	std::string initdir = FCEU_GetPath(FCEUMKF_MEMW);
	ofn.lpstrInitialDir=initdir.c_str();

	if(GetOpenFileName(&ofn))
	{
		int i,j;

		//Save the directory
		if(ofn.nFileOffset < 1024)
		{
			free(MemWatchDir);
			MemWatchDir=(char*)malloc(strlen(ofn.lpstrFile)+1);
			strcpy(MemWatchDir,ofn.lpstrFile);
			MemWatchDir[ofn.nFileOffset]=0;
		}
			
		FILE *fp=FCEUD_UTF8fopen(memwLastFilename,"r");
			MemwAddRecentFile(memwLastFilename);

		for(i=0;i<NUMWATCHES;i++)
		{
			fscanf(fp, "%s ", watchfcontents); //Reads contents of newly opened file
			for(j = 0; j < ADDRESSLENGTH; j++)
				addresses[i][j] = watchfcontents[j];
			fscanf(fp, "%s\n", watchfcontents);
			for(j = 0; j < LABELLENGTH; j++)
				labels[i][j] = watchfcontents[j];

			//Replace dummy strings with empty strings
			if(addresses[i][0] == '|')
			{
				addresses[i][0] = 0;
			}
			if(labels[i][0] == '|')
			{
				labels[i][0] = 0;
			}
			PutInSpaces(i);

			int templl = LABELLENGTH - 1;
			int tempal = ADDRESSLENGTH - 1;
			addresses[i][tempal] = 0;
			labels[i][templl] = 0; //just in case

			SetDlgItemText(hwndMemWatch,MW_VAL (i),(LPTSTR) "---");
			SetDlgItemText(hwndMemWatch,MW_ADDR(i),(LPTSTR) addresses[i]);
			SetDlgItemText(hwndMemWatch,MW_NAME(i),(LPTSTR) labels[i]);
		}
		fclose(fp);
	}
fileChanged = false;
}

// Loads a recent file given the recent files array number(0-4) 
void OpenMemwatchRecentFile(int memwRFileNumber)
{
	int rnum=memwRFileNumber;
		if (rnum > MEMW_MAX_NUMBER_OF_RECENT_FILES) return; //just in case
		
	char* x = memw_recent_files[rnum];
	if (x == NULL) return; //If no recent files exist just return.  Useful for Load last file on startup (or if something goes screwy)
	char watchfcontents[2048];

	FILE *fp=FCEUD_UTF8fopen(x,"r");
	strcpy(memwLastFilename,x);

	if (rnum != 0) //Change order of recent files if not most recent
		MemwAddRecentFile(x);

	int i,j;
	for(i=0;i<NUMWATCHES;i++)
		{
			fscanf(fp, "%s ", watchfcontents); //Reads contents of newly opened file
			for(j = 0; j < ADDRESSLENGTH; j++)
				addresses[i][j] = watchfcontents[j];
			fscanf(fp, "%s\n", watchfcontents);
			for(j = 0; j < LABELLENGTH; j++)
				labels[i][j] = watchfcontents[j];

			//Replace dummy strings with empty strings
			if(addresses[i][0] == '|')
			{
				addresses[i][0] = 0;
			}
			if(labels[i][0] == '|')
			{
				labels[i][0] = 0;
			}
			PutInSpaces(i);

			int templl = LABELLENGTH - 1;
			int tempal = ADDRESSLENGTH - 1;
			addresses[i][tempal] = 0;
			labels[i][templl] = 0; //just in case

			SetDlgItemText(hwndMemWatch,MW_VAL (i),(LPTSTR) "---");
			SetDlgItemText(hwndMemWatch,MW_ADDR(i),(LPTSTR) addresses[i]);
			SetDlgItemText(hwndMemWatch,MW_NAME(i),(LPTSTR) labels[i]);
	}
		fclose(fp);

fileChanged = false;
}

void CloseMemoryWatch()
{
	if(hwndMemWatch)
	{
		SaveStrings();

		//TODO: save window x,y and last file opened to some variables then save them to config when fceu closes
		if (fileChanged==true)
		{
			if(MessageBox(hwndMemWatch, "Save Changes?", "Memory Watch Settings", MB_YESNO)==IDYES)
			{
				SaveMemWatch();
			}
		}

		DestroyWindow(hwndMemWatch);
		hwndMemWatch=0;
	}

}


void ClearAllText()
{
	if (fileChanged==true) //If contents have changed
		{
			if(MessageBox(hwndMemWatch, "Save Changes?", "Save", MB_YESNO)==IDYES)
			{
				SaveMemWatch();

			}
		}
					
					int i;
					for(i=0;i<MWNUM;i++)
					{
						addresses[i][0] = 0;
						labels[i][0] = 0;
						SetDlgItemText(hwndMemWatch,MW_ADDR(i),(LPTSTR) addresses[i]);
						SetDlgItemText(hwndMemWatch,MW_NAME(i),(LPTSTR) labels[i]);
					}
					//Now clear last file used variable (memwLastFilename)
					for (int i=0;i<2048;i++)
					{
						memwLastFilename[i] = NULL;
					}
fileChanged = false;
				
}

static BOOL CALLBACK MemWatchCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	
	const int kLabelControls[] = {MW_ValueLabel1,MW_ValueLabel2};

	switch(uMsg)
	{
	case WM_MOVE: {
		RECT wrect;
		GetWindowRect(hwndDlg,&wrect);
		MemWatch_wndx = wrect.left;
		MemWatch_wndy = wrect.top;
		break;
	};

	case WM_INITDIALOG:
		SetWindowPos(hwndDlg,0,MemWatch_wndx,MemWatch_wndy,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER);
		hdc = GetDC(hwndDlg);
		SelectObject (hdc, debugSystem->hFixedFont);
		SetTextAlign(hdc,TA_UPDATECP | TA_TOP | TA_LEFT);

		//find the positions where we should draw string values
		for(int i=0;i<MWNUM;i++) {
			int col=0;
			if(i>=MWNUM/2)
				col=1;
			RECT r;
			GetWindowRect(GetDlgItem(hwndDlg,MW_ADDR(i)),&r);
			ScreenToClient(hwndDlg,(LPPOINT)&r);
			ScreenToClient(hwndDlg,(LPPOINT)&r.right);
			yPositions[i] = r.top;
			yPositions[i] += ((r.bottom-r.top)-debugSystem->fixedFontHeight)/2; //vertically center
			GetWindowRect(GetDlgItem(hwndDlg,kLabelControls[col]),&r);
			ScreenToClient(hwndDlg,(LPPOINT)&r);
			xPositions[i] = r.left;
		}
		break;

	case WM_PAINT:
		PAINTSTRUCT ps;
		BeginPaint(hwndDlg, &ps);
		EndPaint(hwndDlg, &ps);
		UpdateMemWatch();
		break;
	case WM_INITMENU:
		CheckMenuItem(memwmenu, MEMW_OPTIONS_LOADSTART, MemWatchLoadOnStart ? MF_CHECKED : MF_UNCHECKED);
		CheckMenuItem(memwmenu, MEMW_OPTIONS_LOADLASTFILE, MemWatchLoadFileOnStart ? MF_CHECKED : MF_UNCHECKED);
		break;
	case WM_CLOSE:
	case WM_QUIT:
		CloseMemoryWatch();
		//DeleteObject(hdc);
		break;
	/*
	case WM_KEYDOWN:
		if (wParam == VK_HOME)
			ClearAllText();
	*/
	case WM_COMMAND:

		//Menu Items
		switch(LOWORD(wParam))
		{
		case ACCEL_CTRL_W:
		case MEMW_FILE_CLOSE:  
			CloseMemoryWatch();
			break;

		case ACCEL_CTRL_O:
		case MEMW_FILE_OPEN:
			LoadMemWatch();
			break;
		
		case ACCEL_CTRL_S:
		case MEMW_FILE_SAVE:
			QuickSaveMemWatch();
			break;

		case ACCEL_CTRL_SHIFT_S:
		case MEMW_FILE_SAVEAS:
			SaveMemWatch();
			break;
		
		case ACCEL_CTRL_N:
		case MEMW_FILE_NEW:
			ClearAllText();
			break;
		
		case MEMW_FILE_RECENT:
			break;

		case MEMW_OPTIONS_LOADSTART: //Load on Start up
				MemWatchLoadOnStart ^= 1;
				CheckMenuItem(memwmenu, MEMW_OPTIONS_LOADSTART, MemWatchLoadOnStart ? MF_CHECKED : MF_UNCHECKED);
			break;

		case MEMW_OPTIONS_LOADLASTFILE: //Load last file when opening memwatch
				MemWatchLoadFileOnStart ^= 1;
				CheckMenuItem(memwmenu, MEMW_OPTIONS_LOADLASTFILE, MemWatchLoadFileOnStart ? MF_CHECKED : MF_UNCHECKED);
			break;

		case MEMW_HELP_WCOMMANDS:
			break;

		default:
			if (LOWORD(wParam) >= MEMW_MENU_FIRST_RECENT_FILE && LOWORD(wParam) < MEMW_MENU_FIRST_RECENT_FILE+MEMW_MAX_NUMBER_OF_RECENT_FILES)
				OpenMemwatchRecentFile(LOWORD(wParam) - MEMW_MENU_FIRST_RECENT_FILE);
		break;
		}

		switch(HIWORD(wParam))
		{
		
		case EN_CHANGE:
			{
				fileChanged = iftextchanged();			
				//the contents of an address box changed. re-parse it.
				//first, find which address changed
				int changed = MWRec::findIndex(LOWORD(wParam));
				if(changed==-1) break;
				mwrecs[changed].parse(LOWORD(wParam));
				break;
			}
		
#if 0
		case BN_CLICKED:
			
			switch(LOWORD(wParam))
			{
			case 101: //Save button clicked
				//StopSound(); //mbg 5/7/08
				//SaveMemWatch();  //5/13/08 Buttons removed (I didn't remove the code so it would be easy to add them back one day)
				break;			
			case 102: //Load button clicked
				//StopSound(); //mbg 5/7/08
				//LoadMemWatch();  //5/13/08 Buttons removed
				break;
			case 103: //Clear button clicked
				//ClearAllText();  //5/13/08 Buttons removed
				break;
			default:
				break;
			} 
#endif
		}
		
#if 0
		if(!(wParam>>16)) //Close button clicked
		{
			switch(wParam&0xFFFF)
			{
			case 1:
				//CloseMemoryWatch();  //5/13/08 Buttons removed
				break;
			}
		}
#endif
		
		break;
	default:
		break;
		
	}
	return 0;
}

//Open the Memory Watch dialog
void CreateMemWatch()
//void CreateMemWatch(HWND parent) - adelikat: old code made use of this but parent was only used in a line commented out.  Taking out for my own needs :P

{
	if(NeedsInit) //Clear the strings
	{
		NeedsInit = 0;
		int i,j;
		for(i=0;i<NUMWATCHES;i++)
		{
			for(j=0;j<LABELLENGTH;j++)
			{
				addresses[i][j] = 0;
				labels[i][j] = 0;
			}
		}
	}

	if(hwndMemWatch) //If already open, give focus
	{
		SetFocus(hwndMemWatch);
		return;
	}

	//Create
		hwndMemWatch=CreateDialog(fceu_hInstance,"MEMWATCH",NULL,MemWatchCallB);
	memwmenu=GetMenu(hwndMemWatch);
	UpdateMemWatch();
	memwrecentmenu = CreateMenu();
	memwrecentdmenu = CreateMenu();

	// Update recent files menu
	UpdateMemw_RMenu(memwrecentmenu, memw_recent_files, ID_FILE_RECENT, MEMW_MENU_FIRST_RECENT_FILE);
		
	//Initialize values to previous entered addresses/labels
	{
		int i;
		for(i = 0; i < MWNUM; i++)
		{
			SetDlgItemText(hwndMemWatch,MW_VAL (i),(LPTSTR) "---");
			SetDlgItemText(hwndMemWatch,MW_ADDR(i),(LPTSTR) addresses[i]);
			SetDlgItemText(hwndMemWatch,MW_NAME(i),(LPTSTR) labels[i]);
		}
	}
if (MemWatchLoadFileOnStart) OpenMemwatchRecentFile(0);
}
void AddMemWatch(char memaddress[32])
{
 char TempArray[32];
	int i;
 CreateMemWatch();
	for(i = 0; i < MWNUM; i++)
	{
	 GetDlgItemText(hwndMemWatch,MW_ADDR(i),TempArray,32);
	 if (TempArray[0] == 0)
	 {
		 SetDlgItemText(hwndMemWatch,MW_ADDR(i),memaddress);
		 break;
		}
	}
}

