/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
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

/**
* File description: Everything relevant for the main window should go here. This
*                   does not include functions relevant for dialog windows.
**/

#include "window.h"
#include "main.h"
#include "state.h"      /* Save/Load state AS */

#include "sound.h"
#include "wave.h"
#include "input.h"
#include "video.h"
#include "../../input.h"
#include "../../fceu.h"

#include "memwatch.h"
#include "ppuview.h"
#include "debugger.h"
#include "cheat.h"
#include "debug.h"
#include "ntview.h"
#include "memview.h"
#include "tracer.h"
#include "cdlogger.h"
#include "basicbot.h"
#include "throttle.h"
#include "monitor.h"
#include "tasedit.h"

#include "guiconfig.h"
#include "timing.h"
#include "palette.h"
#include "directories.h"
#include "gui.h"

// Extern variables

extern FCEUGI *GameInfo;
extern int EnableRewind;

// Extern functions

char *md5_asciistr(uint8 digest[16]);

void SetAutoFirePattern(int onframes, int offframes);
void SetAutoFireOffset(int offset);

void RestartMovieOrReset(unsigned int pow);
int KeyboardSetBackgroundAccess(int on); //mbg merge 7/17/06 YECH had to add
void SetJoystickBackgroundAccess(int background); //mbg merge 7/17/06 YECH had to add
void ShowNetplayConsole(void); //mbg merge 7/17/06 YECH had to add
int FCEUMOV_IsPlaying(void); //mbg merge 7/17/06 YECH had to add
void DoPPUView();//mbg merge 7/19/06 yech had to add

void MapInput(void);

// Internal variables
int pauseAfterPlayback = 0;

// Contains recent file strings
char *recent_files[] = { 0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 };
char *recent_directories[10] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

const unsigned int MENU_FIRST_RECENT_FILE = 600;
const unsigned int MAX_NUMBER_OF_RECENT_FILES = sizeof(recent_files)/sizeof(*recent_files);
const unsigned int MAX_NUMBER_OF_RECENT_DIRS = sizeof(recent_directories)/sizeof(*recent_directories);

HWND pwindow;

/**
* Menu handle of the main menu.
**/
static HMENU fceumenu = 0;

static int tog = 0;
static int CheckedAutoFirePattern = MENU_AUTOFIRE_PATTERN_1;
static int CheckedAutoFireOffset = MENU_AUTOFIRE_OFFSET_1;
//static int EnableBackgroundInput = 0;

static HMENU recentmenu, recentdmenu;

static LONG WindowXC=1<<30,WindowYC;
int MainWindow_wndx, MainWindow_wndy;

static uint32 mousex,mousey,mouseb;

static int vchanged = 0;

// Internal functions

void CalcWindowSize(RECT *al)
{
	al->left = 0;
	al->right = VNSWID * winsizemulx;
	al->top = 0;
	al->bottom = FSettings.TotalScanlines() * winsizemuly;

	AdjustWindowRectEx(al,
		GetWindowLong(hAppWnd, GWL_STYLE),
		GetMenu(hAppWnd) != NULL,
		GetWindowLong(hAppWnd, GWL_EXSTYLE)
	);

	al->right -= al->left;
	al->left = 0;
	al->bottom -= al->top;
	al->top=0;
}

/**
* Updates the menu items that should only be enabled if a game is loaded.
*
* @param enable Flag that indicates whether the menus should be enabled (1) or disabled (0). 
**/
void updateGameDependentMenus(unsigned int enable)
{
	const int menu_ids[]= {
		MENU_CLOSE_FILE,
		MENU_SAVE_STATE,
		MENU_LOAD_STATE,
		MENU_RESET,
		MENU_POWER,
		MENU_INSERT_COIN,
		MENU_SWITCH_DISK,
		MENU_RECORD_MOVIE,
		MENU_REPLAY_MOVIE,
		MENU_STOP_MOVIE,
		MENU_RECORD_AVI,
		MENU_STOP_AVI,
		MENU_LOG_SOUND,
		MENU_HIDE_MENU,
		40003,
		MENU_DEBUGGER,
		MENU_PPUVIEWER,
		MENU_NAMETABLEVIEWER,
		MENU_HEXEDITOR,
		MENU_TRACELOGGER,
		MENU_CDLOGGER,
		MENU_GAMEGENIEDECODER,
		MENU_CHEATS
	};

	for (unsigned int i = 0; i < sizeof(menu_ids) / sizeof(*menu_ids); i++)
	{
#ifndef FCEUDEF_DEBUGGER
		if(simpled[i] == 203)
			EnableMenuItem(fceumenu,menu_ids[i],MF_BYCOMMAND | MF_GRAYED);
		else
#endif
#ifndef _USE_SHARED_MEMORY_
			if(simpled[x] == MENU_BASIC_BOT || simpled[i] == 40003)
				EnableMenuItem(fceumenu,menu_ids[i],MF_BYCOMMAND| MF_GRAYED);
			else
#endif
				EnableMenuItem(fceumenu, menu_ids[i], MF_BYCOMMAND | (enable ? MF_ENABLED : MF_GRAYED));
	}
}

/**
* Updates menu items which need to be checked or unchecked.
**/
void UpdateCheckedMenuItems()
{
	static int *polo[] = { &genie, &pal_emulation, &status_icon };
	static int polo2[]={ MENU_GAME_GENIE, MENU_PAL, MENU_SHOW_STATUS_ICON };
	int x;

	// Check or uncheck the necessary menu items
	for(x = 0; x < sizeof(polo) / sizeof(*polo); x++)
	{
		CheckMenuItem(fceumenu, polo2[x], *polo[x] ? MF_CHECKED : MF_UNCHECKED);
	}

	CheckMenuItem(fceumenu, MENU_PAUSEAFTERPLAYBACK, pauseAfterPlayback ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(fceumenu, MENU_RUN_IN_BACKGROUND, eoptions & EO_BGRUN ? MF_CHECKED : MF_UNCHECKED);

	CheckMenuItem(fceumenu, 40003, FCEU_BotMode() ? MF_CHECKED : MF_UNCHECKED);

	CheckMenuItem(fceumenu, 40025, GetAutoFireDesynch() ? MF_CHECKED : MF_UNCHECKED);

	CheckMenuItem(fceumenu, MENU_BACKGROUND_INPUT, EnableBackgroundInput ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(fceumenu, MENU_ENABLE_REWIND, EnableRewind ? MF_CHECKED : MF_UNCHECKED);

	int AutoFirePatternIDs[] = {
		MENU_AUTOFIRE_PATTERN_1,
		MENU_AUTOFIRE_PATTERN_2,
		MENU_AUTOFIRE_PATTERN_3,
		MENU_AUTOFIRE_PATTERN_4,
		MENU_AUTOFIRE_PATTERN_5,
		MENU_AUTOFIRE_PATTERN_6,
		MENU_AUTOFIRE_PATTERN_7,
		MENU_AUTOFIRE_PATTERN_8,
		MENU_AUTOFIRE_PATTERN_9,
		MENU_AUTOFIRE_PATTERN_10,
		MENU_AUTOFIRE_PATTERN_11,
		MENU_AUTOFIRE_PATTERN_12,
		MENU_AUTOFIRE_PATTERN_13,
		MENU_AUTOFIRE_PATTERN_14,
		MENU_AUTOFIRE_PATTERN_15,
		0};

	int AutoFireOffsetIDs[] = {
		MENU_AUTOFIRE_OFFSET_1,
		MENU_AUTOFIRE_OFFSET_2,
		MENU_AUTOFIRE_OFFSET_3,
		MENU_AUTOFIRE_OFFSET_4,
		MENU_AUTOFIRE_OFFSET_5,
		MENU_AUTOFIRE_OFFSET_6,
	0};

	x = 0;

	while(AutoFirePatternIDs[x])
	{
		CheckMenuItem(fceumenu, AutoFirePatternIDs[x],
			AutoFirePatternIDs[x] == CheckedAutoFirePattern ? MF_CHECKED : MF_UNCHECKED);
		x++;
	}

	x = 0;

	while(AutoFireOffsetIDs[x])
	{
		CheckMenuItem(fceumenu, AutoFireOffsetIDs[x],
			AutoFireOffsetIDs[x] == CheckedAutoFireOffset ? MF_CHECKED : MF_UNCHECKED);
		x++;
	}
}

/**
* Updates recent files / recent directories menu
*
* @param menu Menu handle of the main window's menu
* @param strs Strings to add to the menu
* @param mitem Menu ID of the recent files / directory menu
* @param baseid Menu ID of the first subitem
**/
void UpdateRMenu(HMENU menu, char **strs, unsigned int mitem, unsigned int baseid)
{
	// UpdateRMenu(recentmenu, recent_files, MENU_RECENT_FILES, MENU_FIRST_RECENT_FILE);

	MENUITEMINFO moo;
	int x;

	moo.cbSize = sizeof(moo);
	moo.fMask = MIIM_SUBMENU | MIIM_STATE;

	GetMenuItemInfo(GetSubMenu(fceumenu, 0), mitem, FALSE, &moo);
	moo.hSubMenu = menu;
	moo.fState = strs[0] ? MFS_ENABLED : MFS_GRAYED;

	SetMenuItemInfo(GetSubMenu(fceumenu, 0), mitem, FALSE, &moo);

	// Remove all recent files submenus
	for(x = 0; x < MAX_NUMBER_OF_RECENT_FILES; x++)
	{
		RemoveMenu(menu, baseid + x, MF_BYCOMMAND);
	}

	// Recreate the menus
	for(x = MAX_NUMBER_OF_RECENT_FILES - 1; x >= 0; x--)
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

/**
* Helper function to populate the recent directories and recent files arrays.
*
* @param addString String to add to the array.
* @param bufferArray Array where the string will be added.
* @param arrayLen Length of the bufferArray.
* @param menu Menu handle of the main menu.
* @param menuItem
* @param baseID
**/
void UpdateRecentArray(const char* addString, char** bufferArray, unsigned int arrayLen, HMENU menu, unsigned int menuItem, unsigned int baseId)
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
				UpdateRMenu(menu, bufferArray, menuItem, baseId);

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
	UpdateRMenu(menu, bufferArray, menuItem, baseId);
}

/**
* Add a filename to the recent files list.
*
* @param filename Name of the file to add.
**/
void AddRecentFile(const char *filename)
{
	UpdateRecentArray(filename, recent_files, MAX_NUMBER_OF_RECENT_FILES, recentmenu, MENU_RECENT_FILES, MENU_FIRST_RECENT_FILE);
}

/**
* Add a directory to the recent directories list.
*
* @param dirname Name of the directory to add.
**/
void AddRecentDirectory(const char *dirname)
{
	UpdateRecentArray(dirname, recent_directories, MAX_NUMBER_OF_RECENT_DIRS, recentdmenu, 103, 700);
}

/**
* Hides the main menu.
* 
* @param hide_menu Flag to turn the main menu on (0) or off (1) 
**/
void HideMenu(unsigned int hide_menu)
{
	if(hide_menu)
	{
		SetMenu(hAppWnd, 0);
	}
	else
	{
		SetMenu(hAppWnd, fceumenu);
	}
}

void HideFWindow(int h)
{
	LONG desa;

	if(h)  /* Full-screen. */
	{
		RECT bo;
		GetWindowRect(hAppWnd, &bo);
		WindowXC = bo.left;
		WindowYC = bo.top;

		SetMenu(hAppWnd, 0);
		desa=WS_POPUP | WS_CLIPSIBLINGS;  
	}
	else
	{
		desa = WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS;
		HideMenu(tog);

		/* Stupid DirectDraw bug(I think?) requires this.  Need to investigate it. */
		SetWindowPos(hAppWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOSIZE);
	}

	SetWindowLong(hAppWnd, GWL_STYLE ,desa | ( GetWindowLong(hAppWnd, GWL_STYLE) & WS_VISIBLE ));
	SetWindowPos(hAppWnd, 0 ,0 ,0 ,0 ,0 ,SWP_FRAMECHANGED | SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOREPOSITION | SWP_NOSIZE | SWP_NOZORDER);
}

/**
* Toggles the display status of the main menu.
**/
void ToggleHideMenu(void)
{ 
	if(!fullscreen && (GameInfo || tog))
	{
		tog ^= 1;
		HideMenu(tog);
		SetMainWindowStuff();
	}
}

/**
* Toggles the display status of the main menu.
*
* TODO: We could get rid of this one.
**/
void FCEUD_HideMenuToggle(void)
{
	ToggleHideMenu();
}

void ALoad(char *nameo)
{
	if(FCEUI_LoadGame(nameo, 1))
	{
		pal_emulation = FCEUI_GetCurrentVidSystem(0, 0);

		UpdateCheckedMenuItems();

		SetMainWindowStuff();

		AddRecentFile(nameo);

		RefreshThrottleFPS();

		if(eoptions & EO_HIDEMENU && !tog)
		{
			ToggleHideMenu();
		}

		if(eoptions & EO_FSAFTERLOAD)
		{
			SetFSVideoMode();
		}
	}

	ParseGIInput(GameInfo);

	updateGameDependentMenus(GameInfo != 0);
}

/**
* Shows an Open File dialog and opens the ROM if the user selects a ROM file.
*
* @param hParent Handle of the main window
* @param initialdir Directory that's pre-selected in the Open File dialog.
**/
void LoadNewGamey(HWND hParent, const char *initialdir)
{
	const char filter[] = "All usable files(*.nes,*.nsf,*.fds,*.unf,*.zip,*.gz)\0*.nes;*.nsf;*.fds;*.unf;*.zip;*.gz\0All non-compressed usable files(*.nes,*.nsf,*.fds,*.unf)\0*.nes;*.nsf;*.fds;*.unf\0All files (*.*)\0*.*\0";
	char nameo[2048];

	// Create the Open File dialog
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));

	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="FCE Ultra Open File...";
	ofn.lpstrFilter=filter;
	nameo[0]=0;
	ofn.hwndOwner=hParent;
	ofn.lpstrFile=nameo;
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY; //OFN_EXPLORER|OFN_ENABLETEMPLATE|OFN_ENABLEHOOK;
	ofn.lpstrInitialDir = initialdir ? initialdir : FCEU_GetPath(FCEUMKF_ROMS);

	// Show the Open File dialog
	if(GetOpenFileName(&ofn))
	{
		char *tmpdir = (char *)malloc( ofn.nFileOffset + 1 ); //mbg merge 7/17/06 added cast

		if(tmpdir)
		{
			// Get the directory from the filename
			strncpy(tmpdir, ofn.lpstrFile, ofn.nFileOffset);
			tmpdir[ofn.nFileOffset]=0;

			// Add the directory to the list of recent directories
			AddRecentDirectory(tmpdir);

			// Prevent setting the File->Open default
			// directory when a "Recent Directory" is selected.
			if(!initialdir)             
			{
				if(gfsdir)
				{
					free(gfsdir);
				}

				gfsdir = tmpdir;
			}
			else
			{
				free(tmpdir);
			}
		}

		ALoad(nameo);
	}
}

void GetMouseData(uint32 *md)
{
	if(FCEUI_IsMovieActive() < 0)
	{
		return;
	}

	md[0] = mousex;
	md[1] = mousey;

	if(!fullscreen)
	{
		if(ismaximized)
		{
			RECT t;
			GetClientRect(hAppWnd, &t);
			md[0] = md[0] * VNSWID / (t.right ? t.right : 1);
			md[1] = md[1] * FSettings.TotalScanlines() / (t.bottom ? t.bottom : 1);
		}
		else
		{
			md[0] /= winsizemulx;
			md[1] /= winsizemuly;
		}

		md[0] += VNSCLIP;
	}

	md[1] += FSettings.FirstSLine;
	md[2] = ((mouseb == MK_LBUTTON) ? 1 : 0) | (( mouseb == MK_RBUTTON ) ? 2 : 0);
}

/**
* Message loop of the main window
**/
LRESULT FAR PASCAL AppWndProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
  switch(msg)
  {
	  case WM_LBUTTONDOWN:
	  case WM_LBUTTONUP:
	  case WM_RBUTTONDOWN:
	  case WM_RBUTTONUP:
		  mouseb=wParam;
		  goto proco;

	  case WM_MOVE: {
		RECT wrect;
		GetWindowRect(hWnd,&wrect);
		MainWindow_wndx = wrect.left;
		MainWindow_wndy = wrect.top;
	}

	  case WM_MOUSEMOVE:
		  {
			  mousex=LOWORD(lParam);
			  mousey=HIWORD(lParam);
		  }
		  goto proco;

	  case WM_ERASEBKGND:
		  if(xbsave)
			  return(0);
		  else
			  goto proco;

	  case WM_PAINT:
		  if(xbsave)
		  {
			  PAINTSTRUCT ps;
			  BeginPaint(hWnd,&ps);
			  FCEUD_BlitScreen(xbsave);
			  EndPaint(hWnd,&ps);
			  return(0);
		  }
		  goto proco;

    case WM_SIZE:
                if(!fullscreen && !changerecursive)
                 switch(wParam)
                 {
                  case SIZE_MAXIMIZED: ismaximized = 1;SetMainWindowStuff();break;
                  case SIZE_RESTORED: ismaximized = 0;SetMainWindowStuff();break;
                 }
                 break;
    case WM_SIZING:
                 {
                  RECT *wrect=(RECT *)lParam;
                  RECT srect;

                  int h=wrect->bottom-wrect->top;
                  int w=wrect->right-wrect->left;
                  int how;

                  if(wParam == WMSZ_BOTTOM || wParam == WMSZ_TOP)
                   how = 2;
                  else if(wParam == WMSZ_LEFT || wParam == WMSZ_RIGHT)
                   how = 1;
                  else if(wParam == WMSZ_BOTTOMLEFT || wParam == WMSZ_BOTTOMRIGHT
                        || wParam == WMSZ_TOPRIGHT || wParam == WMSZ_TOPLEFT)
                   how = 3;
                  if(how & 1)
                   winsizemulx*= (double)w/winwidth;
                  if(how & 2)
                   winsizemuly*= (double)h/winheight;
                  if(how & 1) FixWXY(0);
                  else FixWXY(1);

                  CalcWindowSize(&srect);
                  winwidth=srect.right;
                  winheight=srect.bottom;
                  wrect->right = wrect->left + srect.right;
                  wrect->bottom = wrect->top + srect.bottom;
                 }
                 //sizchange=1;
                 //break;
                 goto proco;
    case WM_DISPLAYCHANGE:
                if(!fullscreen && !changerecursive)
                 vchanged=1;
                goto proco;
    case WM_DROPFILES:
                {
                 UINT len;
                 char *ftmp;

                 len=DragQueryFile((HDROP)wParam,0,0,0)+1; //mbg merge 7/17/06 changed (HANDLE) to (HDROP)
                 if((ftmp=(char*)malloc(len))) //mbg merge 7/17/06 added cast
                 {
                  DragQueryFile((HDROP)wParam,0,ftmp,len); //mbg merge 7/17/06 changed (HANDLE) to (HDROP)
                  ALoad(ftmp);
                  free(ftmp);
                 }                 
                }
                break;

    case WM_COMMAND:

		if(HIWORD(wParam) == 0 || HIWORD(wParam) == 1)
		{
			wParam &= 0xFFFF;

			// A menu item from the recent files menu was clicked.
			if(wParam >= MENU_FIRST_RECENT_FILE && wParam <= MENU_FIRST_RECENT_FILE + MAX_NUMBER_OF_RECENT_FILES - 1)
			{
				if(recent_files[wParam - MENU_FIRST_RECENT_FILE])
				{
					ALoad(recent_files[wParam - MENU_FIRST_RECENT_FILE]);
				}
			}
			else if(wParam >= 700 && wParam <= 709) // Recent dirs
			{
				// TODO: Do menu items 700 - 709 even exist?
				if(recent_directories[wParam-700])
					LoadNewGamey(hWnd, recent_directories[wParam - 700]);
			}
			switch(LOWORD(wParam))
			{
				//-------
				//mbg merge 7/18/06 added XD tools
			case MENU_DEBUGGER:
				DoDebug(0);
				break;

			case MENU_PPUVIEWER:
				DoPPUView();
				break;

			case MENU_RAMFILTER:
				DoByteMonitor(); 
				break;

			case MENU_NAMETABLEVIEWER:
				DoNTView();
				break;

			case MENU_HEXEDITOR:
				DoMemView();
				break;

			case MENU_TRACELOGGER:
				DoTracer();
				break;

			case MENU_GAMEGENIEDECODER:
				DoGGConv();
				break;

			case MENU_CDLOGGER:
				DoCDLogger();
				break;

			case MENU_AUTOFIRE_PATTERN_1:
				SetAutoFirePattern(1,1);
				CheckedAutoFirePattern = wParam;
				UpdateCheckedMenuItems();
				break;

			case MENU_AUTOFIRE_PATTERN_2:
				SetAutoFirePattern(1,2);
				CheckedAutoFirePattern = wParam;
				UpdateCheckedMenuItems();
				break;

			case MENU_AUTOFIRE_PATTERN_3:
				SetAutoFirePattern(1,3);
				CheckedAutoFirePattern = wParam;
				UpdateCheckedMenuItems();
				break;

			case MENU_AUTOFIRE_PATTERN_4:
				SetAutoFirePattern(1,4);
				CheckedAutoFirePattern = wParam;
				UpdateCheckedMenuItems();
				break;

			case MENU_AUTOFIRE_PATTERN_5:
				SetAutoFirePattern(1,5);
				CheckedAutoFirePattern = wParam;
				UpdateCheckedMenuItems();
				break;

			case MENU_AUTOFIRE_PATTERN_6:
				SetAutoFirePattern(2,1);
				CheckedAutoFirePattern = wParam;
				UpdateCheckedMenuItems();
				break;

			case MENU_AUTOFIRE_PATTERN_7:
				SetAutoFirePattern(2,2);
				CheckedAutoFirePattern = wParam;
				UpdateCheckedMenuItems();
				break;

			case MENU_AUTOFIRE_PATTERN_8:
				SetAutoFirePattern(2,3);
				CheckedAutoFirePattern = wParam;
				UpdateCheckedMenuItems();
				break;

			case MENU_AUTOFIRE_PATTERN_9:
				SetAutoFirePattern(2,4);
				CheckedAutoFirePattern = wParam;
				UpdateCheckedMenuItems();
				break;

			case MENU_AUTOFIRE_PATTERN_10:
				SetAutoFirePattern(3,1);
				CheckedAutoFirePattern = wParam;
				UpdateCheckedMenuItems();
				break;

			case MENU_AUTOFIRE_PATTERN_11:
				SetAutoFirePattern(3,2);
				CheckedAutoFirePattern = wParam;
				UpdateCheckedMenuItems();
				break;

			case MENU_AUTOFIRE_PATTERN_12:
				SetAutoFirePattern(3,3);
				CheckedAutoFirePattern = wParam;
				UpdateCheckedMenuItems();
				break;

			case MENU_AUTOFIRE_PATTERN_13:
				SetAutoFirePattern(4,1);
				CheckedAutoFirePattern = wParam;
				UpdateCheckedMenuItems();
				break;

			case MENU_AUTOFIRE_PATTERN_14:
				SetAutoFirePattern(4,2);
				CheckedAutoFirePattern = wParam;
				UpdateCheckedMenuItems();
				break;

			case MENU_AUTOFIRE_PATTERN_15:
				SetAutoFirePattern(5,1);
				CheckedAutoFirePattern = wParam;
				UpdateCheckedMenuItems();
				break;

			case MENU_AUTOFIRE_OFFSET_1:
			case MENU_AUTOFIRE_OFFSET_2:
			case MENU_AUTOFIRE_OFFSET_3:
			case MENU_AUTOFIRE_OFFSET_4:
			case MENU_AUTOFIRE_OFFSET_5:
			case MENU_AUTOFIRE_OFFSET_6:
				SetAutoFireOffset(wParam - MENU_AUTOFIRE_OFFSET_1);
				CheckedAutoFireOffset = wParam;
				UpdateCheckedMenuItems();
				break;

			case MENU_ALTERNATE_AB:
				SetAutoFireDesynch(GetAutoFireDesynch()^1);
				UpdateCheckedMenuItems();
				break;

			case MENU_TASEDIT:
				DoTasEdit();
				break;

			case MENU_EXTERNAL_INPUT:
				// TODO: ???
				break;

			case MENU_HIDE_MENU:
				// Hide menu menu was selected
				ToggleHideMenu();
				break;

			case MENU_PAUSEAFTERPLAYBACK:
				pauseAfterPlayback = pauseAfterPlayback?0:1;
				UpdateCheckedMenuItems();
				break;

			case MENU_RUN_IN_BACKGROUND:
				// Run in Background menu was selected
				// TODO: Does this even work?

				eoptions ^= EO_BGRUN;

				if((eoptions & EO_BGRUN) == 0)
				{
					EnableBackgroundInput = 0;
					KeyboardSetBackgroundAccess(EnableBackgroundInput);
					SetJoystickBackgroundAccess(EnableBackgroundInput);
				}

				UpdateCheckedMenuItems();
				break;

			case MENU_BACKGROUND_INPUT:
				// Enable background input menu was selected
				// TODO: Does this even work?
				EnableBackgroundInput ^= 1;
				eoptions |= EO_BGRUN * EnableBackgroundInput;
				KeyboardSetBackgroundAccess(EnableBackgroundInput);
				SetJoystickBackgroundAccess(EnableBackgroundInput);
				UpdateCheckedMenuItems();
				break;

			case MENU_ENABLE_REWIND:
				EnableRewind ^= 1;
				UpdateCheckedMenuItems();
				break;

			case MENU_SHOW_STATUS_ICON:
				// Show status icon menu was selected
				// TODO: This does not work.
				status_icon = !status_icon;
				UpdateCheckedMenuItems();
				break;

			case MENU_GAME_GENIE:
				// Game Genie menu was selected

				genie ^= 1;
				FCEUI_SetGameGenie(genie);
				UpdateCheckedMenuItems();
				break;

			case MENU_PAL:
				// PAL Emulation menu was selected
				pal_emulation ^= 1;
				FCEUI_SetVidSystem(pal_emulation);
				RefreshThrottleFPS();
				UpdateCheckedMenuItems();
				//						   DoVideoConfigFix();
				SetMainWindowStuff();
				break;

			case MENU_CHEATS:
				ConfigCheats(hWnd);
				break;

			case 40003:
				FCEU_SetBotMode(1^FCEU_BotMode());
				UpdateCheckedMenuItems();
				break;

			case MENU_BASIC_BOT:
				CreateBasicBot();
				break;
			
			case MENU_DIRECTORIES:
				// Directories menu was selected
				ConfigDirectories();
				break;

			case MENU_GUI_OPTIONS:
				// GUI Options menu was selected.
				ConfigGUI();
				break;

			case MENU_INPUT:
				// Input menu was selected
				ConfigInput(hWnd);
				break;

			case MENU_TIMING:
				// Timing menu was selected
				ConfigTiming();
				break;

			case MENU_NETWORK:
				// Network Play menu was selected
				ShowNetplayConsole();
				break;

			case MENU_PALETTE:
				// Palette menu was selected
				ConfigPalette();
				break;

			case MENU_SOUND:
				// Sound menu was selected
				ConfigSound();
				break;

			case MENU_VIDEO:
				// Video menu was selected
				ConfigVideo();
				break;

			case MENU_HOTKEYS:
				// Hotkeys menu was selected
				MapInput();
				break;

			case MENU_RESET:
				// The reset menu was selected
				RestartMovieOrReset(0);
				break;

			case MENU_POWER:
				// The power menu was selected
				RestartMovieOrReset(1);
				break;

			case MENU_SWITCH_DISK:
				// Switch disk menu was selected
				FCEUI_FDSSelect();
				break;

			case MENU_EJECT_DISK:
				// Eject disk menu was selected
				FCEUI_FDSInsert();
				break;

			case MENU_INSERT_COIN:
				// Insert coin menu was selected
				FCEUI_VSUniCoin();
				break;


#ifdef FCEUDEF_DEBUGGER
				//case 203:BeginDSeq(hWnd);break; //mbg merge 7/18/06 removed as part of old debugger
#endif

				//case 204:ConfigAddCheat(hWnd);break; //mbg merge TODO 7/17/06 - had to remove this
				
			case MENU_MEMORY_WATCH:
				CreateMemWatch();
				break;

			case ACCEL_CTRL_O:
			case MENU_OPEN_FILE:
				// User selected the Open File menu => Show the file selection dialog
				LoadNewGamey(hWnd, 0);
				break;

			case MENU_CLOSE_FILE:
				// User selected the Close File menu => Close the game if necessary
				if(GameInfo)
				{
					FCEUI_CloseGame();                            
					updateGameDependentMenus(GameInfo != 0);
				}
				break;

			case MENU_SAVE_STATE:
				// Save state as menu was selected
				FCEUD_SaveStateAs();
				break;

			case MENU_LOAD_STATE:
				// Load state from menu was selected
				FCEUD_LoadStateFrom();
				break;

			case MENU_LOG_SOUND: //mbg merge 7/18/06 changed ID from 120
				// Record sound menu was selected
				// TODO: Proper stop logging
				{
					MENUITEMINFO mi;
					// Evil:
                                        char *strT = "Stop Sound Logging";
                                        char *strF = "Log Sound As...";
					char *str = CreateSoundSave() ? strT : strF;

					memset(&mi,0,sizeof(mi));
					mi.fMask=MIIM_DATA|MIIM_TYPE;
					mi.cbSize=sizeof(mi);
					GetMenuItemInfo(fceumenu,120,0,&mi);                           
					mi.fMask=MIIM_DATA|MIIM_TYPE;
					mi.cbSize=sizeof(mi);
					mi.dwTypeData=str;
					mi.cch=strlen(str);
					SetMenuItemInfo(fceumenu,120,0,&mi);
				}
				break;

			case MENU_EXIT:
				// Exit menu was selected
				DoFCEUExit();
				break;

			case MENU_RECORD_MOVIE:
				// Record movie menu was selected
				FCEUD_MovieRecordTo();
				break;

			case MENU_REPLAY_MOVIE:
				// Replay movie menu was selected
				FCEUD_MovieReplayFrom();
				break;

			case MENU_STOP_MOVIE:
				// Stop movie menu was selected
				FCEUI_StopMovie();
				break;

			case MENU_RECORD_AVI:
				// Record AVI menu was selected
				FCEUD_AviRecordTo();
				break;

			case MENU_STOP_AVI:
				// Stop AVI menu was selected
				FCEUD_AviStop();
				break;

			case MENU_ABOUT:
				// About menu was selected
				ShowAboutBox();
				break;

			case MENU_MSGLOG:
				// Message Log menu was selected
				MakeLogWindow();
				break;
			}
		}
		break;


    case WM_SYSCOMMAND:
               if(GameInfo && wParam == SC_SCREENSAVE && (goptions & GOO_DISABLESS))
                return(0);

               if(wParam==SC_KEYMENU)
               {
                if(GameInfo && InputType[2]==SIFC_FKB && cidisabled)
                 break;
                if(lParam == VK_RETURN || fullscreen || tog) break;
               }
               goto proco;
    case WM_SYSKEYDOWN:
               if(GameInfo && InputType[2]==SIFC_FKB && cidisabled)
                break; /* Hopefully this won't break DInput... */

                if(fullscreen || tog)
                {
                 if(wParam==VK_MENU)
                  break;
                }
                if(wParam==VK_F10)
                {
					return 0;
				/*
                 if(!moocow) FCEUD_PrintError("Iyee");
                 if(!(lParam&0x40000000))
                  FCEUI_ResetNES();
                 break;
				*/
                }

                if(wParam == VK_RETURN)
		{
                 if(!(lParam&(1<<30)))
                 {
                  UpdateCheckedMenuItems();
                  changerecursive=1;
                  if(!SetVideoMode(fullscreen^1))
                   SetVideoMode(fullscreen);
                  changerecursive=0;
                 }
                 break;
		}
                goto proco;

    case WM_KEYDOWN:
              if(GameInfo)
	      {
	       /* Only disable command keys if a game is loaded(and the other
		  conditions are right, of course). */
               if(InputType[2]==SIFC_FKB)
	       {
		if(wParam==VK_SCROLL)
		{
 		 cidisabled^=1;
		 FCEUI_DispMessage("Family Keyboard %sabled.",cidisabled?"en":"dis");
		}
		if(cidisabled)
                 break; /* Hopefully this won't break DInput... */
	       }
	      }
                goto proco;
    case WM_CLOSE:
    case WM_DESTROY:
    case WM_QUIT:
		DoFCEUExit();
		break;
    case WM_ACTIVATEAPP:       
       if((BOOL)wParam)
       {
        nofocus=0;
       }
       else
       {
        nofocus=1;
       }
       goto proco;
    case WM_ENTERMENULOOP:
      EnableMenuItem(fceumenu,MENU_STOP_MOVIE,MF_BYCOMMAND | (FCEUI_IsMovieActive()?MF_ENABLED:MF_GRAYED));
      EnableMenuItem(fceumenu,MENU_STOP_AVI,MF_BYCOMMAND | (FCEUI_AviIsRecording()?MF_ENABLED:MF_GRAYED));
    default:
      proco:
      return DefWindowProc(hWnd,msg,wParam,lParam);
   }
  return 0;
}

void FixWXY(int pref)
{
	if(eoptions&EO_FORCEASPECT)
	{
		/* First, make sure the ratio is valid, and if it's not, change
		it so that it doesn't break everything.
		*/
		if(saspectw < 0.01) saspectw = 0.01;
		if(saspecth < 0.01) saspecth = 0.01;
		if((saspectw / saspecth) > 100) saspecth = saspectw;
		if((saspecth / saspectw) > 100) saspectw = saspecth;

		if((saspectw / saspecth) < 0.1) saspecth = saspectw;
		if((saspecth / saspectw) > 0.1) saspectw = saspecth;

		if(!pref)
		{
			winsizemuly = winsizemulx * 256 / FSettings.TotalScanlines() * 3 / 4 * saspectw / saspecth;
		}
		else
		{
			winsizemulx = winsizemuly * FSettings.TotalScanlines() / 256 * 4 / 3 * saspecth / saspectw;
		}
	}
	if(winspecial)
	{
		int mult;
		if(winspecial == 1 || winspecial == 2) mult = 2;
		else mult = 3;
		if(winsizemulx < mult)
		{
			if(eoptions&EO_FORCEASPECT)
				winsizemuly *= mult / winsizemulx;
			winsizemulx = mult;
		}
		if(winsizemuly < mult)
		{
			if(eoptions&EO_FORCEASPECT)
				winsizemulx *= mult / winsizemuly;
			winsizemuly = mult;
		}
	}

	if(winsizemulx<0.1)
		winsizemulx=0.1;
	if(winsizemuly<0.1)
		winsizemuly=0.1;

	if(eoptions & EO_FORCEISCALE)
	{
		int x,y;

		x = winsizemulx * 2;
		y = winsizemuly * 2;

		x = (x>>1) + (x&1);
		y = (y>>1) + (y&1);

		if(!x) x=1;
		if(!y) y=1;

		winsizemulx = x;
		winsizemuly = y;    
	}

	if(winsizemulx > 100) winsizemulx = 100;
	if(winsizemuly > 100) winsizemuly = 100;
}

void UpdateFCEUWindow(void)
{
	if(vchanged && !fullscreen && !changerecursive && !nofocus)
	{
		SetVideoMode(0);
		vchanged = 0;
	}

	BlockingCheck();

	if(!(eoptions & EO_BGRUN))
	{
		while(nofocus)
		{

			Sleep(75);
			BlockingCheck();
		}
	}
}

/**
* Destroys the main window
**/
void ByebyeWindow()
{
	SetMenu(hAppWnd, 0);
	DestroyMenu(fceumenu);
	DestroyWindow(hAppWnd);
}

/**
* Creates the main window.
* 
* @return Flag that indicates failure (0) or success (1)
**/
int CreateMainWindow()
{
	WNDCLASSEX winclass;
	RECT tmp;

	// Create and register the window class

	memset(&winclass, 0, sizeof(winclass));
	winclass.cbSize = sizeof(WNDCLASSEX);
	winclass.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_SAVEBITS;
	winclass.lpfnWndProc = AppWndProc;
	winclass.cbClsExtra = 0;
	winclass.cbWndExtra = 0;
	winclass.hInstance = fceu_hInstance;
	winclass.hIcon = LoadIcon(fceu_hInstance, "ICON_1");
	winclass.hIconSm = LoadIcon(fceu_hInstance, "ICON_1");
	winclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	winclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH); //mbg merge 7/17/06 added cast
	winclass.lpszClassName = "FCEULTRA";

	if(!RegisterClassEx(&winclass))
	{
		return FALSE;
	}

	AdjustWindowRectEx(&tmp, WS_OVERLAPPEDWINDOW, 1, 0);

	fceumenu = LoadMenu(fceu_hInstance,"FCEUMENU");

	recentmenu = CreateMenu();
	recentdmenu = CreateMenu();

	// Update recent files menu
	UpdateRMenu(recentmenu, recent_files, MENU_RECENT_FILES, MENU_FIRST_RECENT_FILE);
	UpdateRMenu(recentdmenu, recent_directories, 103, 700);

	updateGameDependentMenus(0);

	hAppWnd = CreateWindowEx(
		0,
		"FCEULTRA",
		"FCE Ultra",
		WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS,  /* Style */
		MainWindow_wndx,
		MainWindow_wndy,
		256,
		FSettings.TotalScanlines(),  /* X,Y ; Width, Height */
		NULL,
		fceumenu,
		fceu_hInstance,
		NULL );  

	//CenterWindowOnScreen(hAppWnd);

	DragAcceptFiles(hAppWnd, 1);

	SetMainWindowStuff();

	return 1;
}

void SetMainWindowStuff()
{
	RECT tmp;

	GetWindowRect(hAppWnd, &tmp);

	if(ismaximized)
	{
		winwidth = tmp.right - tmp.left;
		winheight = tmp.bottom - tmp.top;

		ShowWindow(hAppWnd, SW_SHOWMAXIMIZED);
	}
	else
	{
		RECT srect;

		if(WindowXC != ( 1 << 30 ))
		{
			/* Subtracting and adding for if(eoptions&EO_USERFORCE) below. */
			tmp.bottom -= tmp.top;
			tmp.bottom += WindowYC;

			tmp.right -= tmp.left;
			tmp.right += WindowXC;


			tmp.left = WindowXC;
			tmp.top = WindowYC;
			WindowXC = 1 << 30;
		}

		CalcWindowSize(&srect);

		SetWindowPos(
			hAppWnd,
			HWND_TOP,
			tmp.left,
			tmp.top,
			srect.right,
			srect.bottom,
			SWP_SHOWWINDOW
		);

		winwidth = srect.right;
		winheight = srect.bottom;

		ShowWindow(hAppWnd, SW_SHOWNORMAL);
	}
}

/**
* @return Flag that indicates failure (0) or success (1).
**/
int GetClientAbsRect(LPRECT lpRect)
{
	POINT point;
	point.x = point.y = 0;

	if(!ClientToScreen(hAppWnd, &point))
	{
		return 0;
	}

	lpRect->top = point.y;
	lpRect->left = point.x;

	if(ismaximized)
	{
		RECT al;
		GetClientRect(hAppWnd, &al);
		lpRect->right = point.x + al.right;
		lpRect->bottom = point.y + al.bottom;
	}
	else
	{
		lpRect->right = point.x + VNSWID * winsizemulx;
		lpRect->bottom = point.y + FSettings.TotalScanlines() * winsizemuly;
	}
	return 1;
}

/**
* Shows an Open File menu and starts recording an AVI
* 
* TODO: Does this even work?
**/
void FCEUD_AviRecordTo(void)
{
	OPENFILENAME ofn;
	char szChoice[MAX_PATH];

	if(FCEUMOV_IsPlaying())
	{
		extern char curMovieFilename[];
		strcpy(szChoice, curMovieFilename);
		char* dot = strrchr(szChoice,'.');
		
		if (dot)
		{
			*dot='\0';
		}

		strcat(szChoice, ".avi");
	}
	else
	{
		extern char FileBase[];
		sprintf(szChoice, "%s.avi", FileBase);
	}

	// avi record file browser
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hAppWnd;
	ofn.lpstrFilter = "AVI Files (*.avi)\0*.avi\0\0";
	ofn.lpstrFile = szChoice;
	ofn.lpstrDefExt = "avi";
	ofn.lpstrTitle = "Save AVI as";

	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;

	if(GetSaveFileName(&ofn))
	{
		FCEUI_AviBegin(szChoice);
	}
}

/**
* Stop AVI recording
**/
void FCEUD_AviStop(void)
{
	FCEUI_AviEnd();
}

void FCEUD_CmdOpen(void)
{
	
	LoadNewGamey(hAppWnd, 0);
}

bool FCEUD_PauseAfterPlayback()
{
	return pauseAfterPlayback!=0;
}
