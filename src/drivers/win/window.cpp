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

// File description: Everything relevant for the main window should go here. This
//                   does not include functions relevant for dialog windows.

#include "../../input.h"
#include "../../state.h"
#include "window.h"
#include "main.h"
#include "state.h"

#include "sound.h"
#include "wave.h"
#include "input.h"
#include "video.h"
#include "input.h"
#include "fceu.h"

#include "memwatch.h"
#include "ppuview.h"
#include "debugger.h"
#include "cheat.h"
#include "debug.h"
#include "ntview.h"
#include "memview.h"
#include "tracer.h"
#include "cdlogger.h"
#include "throttle.h"
#include "monitor.h"
#include "tasedit.h"
#include "keyboard.h"
#include "joystick.h"
#include "oldmovie.h"
#include "movie.h"
#include "texthook.h"
#include "guiconfig.h"
#include "timing.h"
#include "palette.h"
#include "directories.h"
#include "gui.h"
#include "help.h"
#include "movie.h"
#include "fceulua.h"
#include "utils/xstring.h"
#include "file.h"
#include "mapinput.h"
#include "movieoptions.h"

#include <fstream>
#include <sstream>

using namespace std;
//********************************************************************************
//Globals
//********************************************************************************

//Handles---------------------------------------------
static HMENU fceumenu = 0;	//Main menu.
HWND pwindow;				//Client Area
static HMENU recentmenu;	//Recent Menu
HMENU hfceuxcontext;		//Handle to context menu
HMENU hfceuxcontextsub;		//Handle to context sub menu

//Extern variables------------------------------------
extern bool movieSubtitles;
extern FCEUGI *GameInfo;
extern int EnableAutosave;
extern bool frameAdvanceLagSkip;
extern bool turbo;
extern int luaRunning;
extern bool movie_readonly;

// Extern functions
char *md5_asciistr(uint8 digest[16]);

void ShowNetplayConsole(void); //mbg merge 7/17/06 YECH had to add
void MapInput(void);
extern BOOL CALLBACK ReplayMetadataDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);	//Metadata dialog
extern bool CheckFileExists(const char* filename);	//Receives a filename (fullpath) and checks to see if that file exists

//AutoFire-----------------------------------------------
void SetAutoFirePattern(int onframes, int offframes);
void SetAutoFireOffset(int offset);
static int CheckedAutoFirePattern = MENU_AUTOFIRE_PATTERN_1;
static int CheckedAutoFireOffset = MENU_AUTOFIRE_OFFSET_1;
int GetCheckedAutoFirePattern();
int GetCheckedAutoFireOffset();

//Internal variables-------------------------------------

static int winwidth, winheight;
static volatile int nofocus = 0;
static int tog = 0;					//Toggle for Hide Menu
static bool loggingSound = false;
static LONG WindowXC=1<<30,WindowYC;
int MainWindow_wndx, MainWindow_wndy;
static uint32 mousex,mousey,mouseb;
static int vchanged = 0;

//Function Declarations
void ChangeMenuItemText(int menuitem, string text);			//Alters a menu item name
void ChangeContextMenuItemText(int menuitem, string text, HMENU menu);	//Alters a context menu item name

//Recent Menu Strings ------------------------------------
char *recent_files[] = { 0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 ,0 };
const unsigned int MENU_FIRST_RECENT_FILE = 600;
const unsigned int MAX_NUMBER_OF_RECENT_FILES = sizeof(recent_files)/sizeof(*recent_files);

//Exported variables ------------------------------------

int EnableBackgroundInput = 0;
int ismaximized = 0;

//Help Menu subtopics
string moviehelp = "{695C964E-B83F-4A6E-9BA2-1A975387DB55}";		 //Movie Recording
string gettingstartedhelp = "{C76AEBD9-1E27-4045-8A37-69E5A52D0F9A}";//Getting Started

//********************************************************************************

int GetCheckedAutoFirePattern()
{
//adelikat: sorry, I'm sure there is an easier way to accomplish this.
//This function allows the proper check to be displayed on the auto-fire pattern offset at start up when another autofire pattern is saved in config
	if (AFon == 1 && AFoff == 2) return MENU_AUTOFIRE_PATTERN_2;
	if (AFon == 1 && AFoff == 3) return MENU_AUTOFIRE_PATTERN_3;
	if (AFon == 1 && AFoff == 4) return MENU_AUTOFIRE_PATTERN_4;
	if (AFon == 1 && AFoff == 5) return MENU_AUTOFIRE_PATTERN_5;
	if (AFon == 2 && AFoff == 1) return MENU_AUTOFIRE_PATTERN_6;
	if (AFon == 2 && AFoff == 2) return MENU_AUTOFIRE_PATTERN_7;
	if (AFon == 2 && AFoff == 3) return MENU_AUTOFIRE_PATTERN_8;
	if (AFon == 2 && AFoff == 4) return MENU_AUTOFIRE_PATTERN_9;
	if (AFon == 3 && AFoff == 1) return MENU_AUTOFIRE_PATTERN_10;
	if (AFon == 3 && AFoff == 2) return MENU_AUTOFIRE_PATTERN_11;
	if (AFon == 3 && AFoff == 3) return MENU_AUTOFIRE_PATTERN_12;
	if (AFon == 4 && AFoff == 1) return MENU_AUTOFIRE_PATTERN_13;
	if (AFon == 4 && AFoff == 2) return MENU_AUTOFIRE_PATTERN_14;
	if (AFon == 5 && AFoff == 1) return MENU_AUTOFIRE_PATTERN_15;

return MENU_AUTOFIRE_PATTERN_1;
}

int GetCheckedAutoFireOffset()
{
	if (AutoFireOffset == 1) return MENU_AUTOFIRE_OFFSET_2;
	if (AutoFireOffset == 2) return MENU_AUTOFIRE_OFFSET_3;
	if (AutoFireOffset == 3) return MENU_AUTOFIRE_OFFSET_4;
	if (AutoFireOffset == 4) return MENU_AUTOFIRE_OFFSET_5;
	if (AutoFireOffset == 5) return MENU_AUTOFIRE_OFFSET_6;
	return MENU_AUTOFIRE_OFFSET_1;
}

// Internal functions



static void ConvertFCM(HWND hwndOwner)
{
	std::string initdir = FCEU_GetPath(FCEUMKF_MOVIE);

	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hwndOwner;
	ofn.lpstrFilter = "FCEU <2.0 Movie Files (*.fcm)\0*.fcm\0All files(*.*)\0*.*\0\0";
	ofn.lpstrFile = new char[640*1024]; //640K should be enough for anyone
	ofn.lpstrFile[0] = 0;
	ofn.nMaxFile = 640*1024;
	ofn.lpstrInitialDir = initdir.c_str();
	ofn.Flags = OFN_NOCHANGEDIR | OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT | OFN_EXPLORER;
	ofn.lpstrDefExt = "fcm";
	ofn.lpstrTitle = "Select old movie(s) for conversion";

	if(GetOpenFileName(&ofn))
	{
		std::vector<std::string> todo;

		//build a list of movies to convert. this might be one, or it might be many, depending on what the user selected
		if(ofn.nFileExtension==0)
		{
			//multiselect
			std::string dir = ofn.lpstrFile;
			char* cp = ofn.lpstrFile + dir.size() + 1;
			while(*cp) 
			{
				std::string fname = cp;
				todo.push_back(dir + "/" + fname);
				cp += fname.size() + 1;
			}
		}
		else
		{
			todo.push_back(ofn.lpstrFile);
		}

		SetCursor(LoadCursor(0,IDC_WAIT));

		//convert each movie
		int okcount = 0;
		for(uint32 i=0;i<todo.size();i++)
		{
			std::string infname = todo[i];
			
			//produce output filename
			std::string outname;
			size_t dot = infname.find_last_of(".");
			if(dot == std::string::npos)
				outname = infname + ".fm2";
			else
				outname = infname.substr(0,dot) + ".fm2";

			MovieData md;
			EFCM_CONVERTRESULT result = convert_fcm(md, infname);
			if(result==FCM_CONVERTRESULT_SUCCESS)
			{
				okcount++;
				std::fstream* outf = FCEUD_UTF8_fstream(outname, "wb");
				md.dump(outf,false);
				delete outf;
			} else {
				std::string msg = "Failure converting " + infname + "\r\n\r\n" + EFCM_CONVERTRESULT_message(result);
				MessageBox(hwndOwner,msg.c_str(),"Failure converting fcm", 0);
			}
		}

		std::string okmsg = "Converted " + stditoa(okcount) + " movie(s). There were " + stditoa(todo.size()-okcount) + " failure(s).";
		MessageBox(hwndOwner,okmsg.c_str(),"FCM Conversion results", 0);
	}

	delete[] ofn.lpstrFile;
}

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

/// Updates the menu items that should only be enabled if a game is loaded.
/// @param enable Flag that indicates whether the menus should be enabled (1) or disabled (0). 
void updateGameDependentMenus(unsigned int enable)
{
	const int menu_ids[]= {
		MENU_CLOSE_FILE,
		MENU_RESET,
		MENU_POWER,
		MENU_INSERT_COIN,
		MENU_SWITCH_DISK,
		MENU_RECORD_AVI,
		MENU_STOP_AVI,
		MENU_RECORD_WAV,
		MENU_STOP_WAV,
		MENU_HIDE_MENU,
		MENU_DEBUGGER,
		MENU_PPUVIEWER,
		MENU_NAMETABLEVIEWER,
		MENU_HEXEDITOR,
		MENU_TRACELOGGER,
		MENU_CDLOGGER,
		MENU_GAMEGENIEDECODER,
		MENU_CHEATS,
		ID_TOOLS_TEXTHOOKER
	};

	for (unsigned int i = 0; i < sizeof(menu_ids) / sizeof(*menu_ids); i++)
	{
#ifndef _USE_SHARED_MEMORY_
		if(simpled[x] == MENU_BASIC_BOT)
			EnableMenuItem(fceumenu,menu_ids[i],MF_BYCOMMAND| MF_GRAYED);
		else
#endif
			EnableMenuItem(fceumenu, menu_ids[i], MF_BYCOMMAND | (enable ? MF_ENABLED : MF_GRAYED));
	}
}

//Updates menu items which need to be checked or unchecked.
void UpdateCheckedMenuItems()
{
	bool spr, bg;
	FCEUI_GetRenderPlanes(spr,bg);

	static int *polo[] = { &genie, &pal_emulation, &status_icon};
	static int polo2[]={ MENU_GAME_GENIE, MENU_PAL, MENU_SHOW_STATUS_ICON };
	int x;

	// Check or uncheck the necessary menu items
	for(x = 0; x < sizeof(polo) / sizeof(*polo); x++)
	{
		CheckMenuItem(fceumenu, polo2[x], *polo[x] ? MF_CHECKED : MF_UNCHECKED);
	}
	//NES Menu
	CheckMenuItem(fceumenu, ID_NES_PAUSE, EmulationPaused ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(fceumenu, ID_NES_TURBO, turbo ? MF_CHECKED : MF_UNCHECKED);

	//Config Menu
	CheckMenuItem(fceumenu, MENU_PAUSEAFTERPLAYBACK, pauseAfterPlayback ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(fceumenu, MENU_RUN_IN_BACKGROUND, eoptions & EO_BGRUN ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(fceumenu, MENU_BACKGROUND_INPUT, EnableBackgroundInput ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(fceumenu, MENU_ENABLE_AUTOSAVE, EnableAutosave ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(fceumenu, MENU_DISPLAY_FA_LAGSKIP, frameAdvanceLagSkip?MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(fceumenu, MENU_CONFIG_BINDSAVES, bindSavestate?MF_CHECKED : MF_UNCHECKED);

	//Config - Display SubMenu
	CheckMenuItem(fceumenu, MENU_DISPLAY_LAGCOUNTER, lagCounterDisplay?MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(fceumenu, ID_DISPLAY_FRAMECOUNTER, frame_display ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(fceumenu, ID_DISPLAY_MOVIESUBTITLES, movieSubtitles?MF_CHECKED:MF_UNCHECKED);
	CheckMenuItem(fceumenu, ID_DISPLAY_MOVIESUBTITLES_AVI, subtitlesOnAVI?MF_CHECKED:MF_UNCHECKED);
	CheckMenuItem(fceumenu, MENU_DISPLAY_BG, bg?MF_CHECKED:MF_UNCHECKED);
	CheckMenuItem(fceumenu, MENU_DISPLAY_OBJ, spr?MF_CHECKED:MF_UNCHECKED);
	
	//Tools Menu
	CheckMenuItem(fceumenu, MENU_ALTERNATE_AB, GetAutoFireDesynch() ? MF_CHECKED : MF_UNCHECKED);

	//AutoFire Patterns
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
	CheckedAutoFirePattern = GetCheckedAutoFirePattern();
	CheckedAutoFireOffset =  GetCheckedAutoFireOffset();
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

	//Check input display
	CheckMenuItem(fceumenu, MENU_INPUTDISPLAY_0, MF_UNCHECKED);
	CheckMenuItem(fceumenu, MENU_INPUTDISPLAY_1, MF_UNCHECKED);
	CheckMenuItem(fceumenu, MENU_INPUTDISPLAY_2, MF_UNCHECKED);
	CheckMenuItem(fceumenu, MENU_INPUTDISPLAY_4, MF_UNCHECKED);
	switch (input_display)
	{
		case 0: //Off
			CheckMenuItem(fceumenu, MENU_INPUTDISPLAY_0, MF_CHECKED);
			break;
		case 1: //1 player
			CheckMenuItem(fceumenu, MENU_INPUTDISPLAY_1, MF_CHECKED);
			break;
		case 2: //2 player
			CheckMenuItem(fceumenu, MENU_INPUTDISPLAY_2, MF_CHECKED);
			break;
		//note: input display can actually have a 3 player display option but is skipped in the hotkey toggle so it is skipped here as well
		case 4: //4 player
			CheckMenuItem(fceumenu, MENU_INPUTDISPLAY_4, MF_CHECKED);
			break;
		default:
			break;
	}
}

void UpdateContextMenuItems(HMENU context, int whichContext)
{
	string undoLoadstate = "Undo loadstate";
	string redoLoadstate = "Redo loadstate";
	string redoSavestate = "Redo savestate";
	string undoSavestate = "Undo savestate";
	
	//Undo Loadstate
	if (CheckBackupSaveStateExist() && (undoLS || redoLS))
		EnableMenuItem(context,FCEUX_CONTEXT_UNDOLOADSTATE,MF_BYCOMMAND | MF_ENABLED);
	else
		EnableMenuItem(context,FCEUX_CONTEXT_UNDOLOADSTATE,MF_BYCOMMAND | MF_GRAYED);
	if (redoLS)
		ChangeContextMenuItemText(FCEUX_CONTEXT_UNDOLOADSTATE, redoLoadstate, context);
	else
		ChangeContextMenuItemText(FCEUX_CONTEXT_UNDOLOADSTATE, undoLoadstate, context);

	//Undo Savestate
	if (undoSS || redoSS)		//If undo or redo, enable Undo savestate, else keep it gray
		EnableMenuItem(context,FCEUX_CONTEXT_UNDOSAVESTATE,MF_BYCOMMAND | MF_ENABLED);
	else
		EnableMenuItem(context,FCEUX_CONTEXT_UNDOSAVESTATE,MF_BYCOMMAND | MF_GRAYED);
	if (redoSS)
		ChangeContextMenuItemText(FCEUX_CONTEXT_UNDOSAVESTATE, redoSavestate, context);
	else
		ChangeContextMenuItemText(FCEUX_CONTEXT_UNDOSAVESTATE, undoSavestate, context);
}

/// Updates recent files / recent directories menu
/// @param menu Menu handle of the main window's menu
/// @param strs Strings to add to the menu
/// @param mitem Menu ID of the recent files / directory menu
/// @param baseid Menu ID of the first subitem
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
		// Skip empty strings
		if(!strs[x])
			continue;

		std::string tmp = strs[x];
		std::string archiveName, fileName, fileToOpen;
		FCEU_SplitArchiveFilename(tmp,archiveName,fileName,fileToOpen);
		if(archiveName != "")
			tmp = archiveName + " <" + fileName + ">";

		//clamp this string to 128 chars
		if(tmp.size()>128)
			tmp = tmp.substr(0,128);

		moo.cbSize = sizeof(moo);
		moo.fMask = MIIM_DATA | MIIM_ID | MIIM_TYPE;

		//// Fill in the menu text.
		//if(strlen(strs[x]) < 128)
		//{
		//	sprintf(tmp, "&%d. %s", ( x + 1 ) % 10, strs[x]);
		//}
		//else
		//{
		//	sprintf(tmp, "&%d. %s", ( x + 1 ) % 10, strs[x] + strlen( strs[x] ) - 127);
		//}

		// Insert the menu item
		moo.cch = tmp.size();
		moo.fType = 0;
		moo.wID = baseid + x;
		moo.dwTypeData = (LPSTR)tmp.c_str();
		InsertMenuItem(menu, 0, 1, &moo);
	}

	DrawMenuBar(hAppWnd);
}

/// Helper function to populate the recent directories and recent files arrays.
/// @param addString String to add to the array.
/// @param bufferArray Array where the string will be added.
/// @param arrayLen Length of the bufferArray.
/// @param menu Menu handle of the main menu.
/// @param menuItem
/// @param baseID
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

/// Add a filename to the recent files list.
/// @param filename Name of the file to add.
void AddRecentFile(const char *filename)
{
	UpdateRecentArray(filename, recent_files, MAX_NUMBER_OF_RECENT_FILES, recentmenu, MENU_RECENT_FILES, MENU_FIRST_RECENT_FILE);
}

/// Hides the main menu.
///@param hide_menu Flag to turn the main menu on (0) or off (1) 
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

//Toggles the display status of the main menu.
void ToggleHideMenu(void)
{ 
	if(!fullscreen && (GameInfo || tog))
	{
		tog ^= 1;
		HideMenu(tog);
		SetMainWindowStuff();
	}
}

//Toggles the display status of the main menu.
//TODO: We could get rid of this one.
void FCEUD_HideMenuToggle(void)
{
	ToggleHideMenu();
}

void CloseGame()
{
	if (GameInfo)
	{
		FCEUI_CloseGame();
		KillMemView();
		updateGameDependentMenus(GameInfo != 0);
	}
}

void ALoad(char *nameo, char* innerFilename)
{
	if (GameInfo) FCEUI_CloseGame();
	if(FCEUI_LoadGameVirtual(nameo, 1))
	{
		pal_emulation = FCEUI_GetCurrentVidSystem(0, 0);

		UpdateCheckedMenuItems();

		PushCurrentVideoSettings();

		std::string recentFileName = nameo;
		if(GameInfo->archiveFilename && GameInfo->archiveCount>1)
			recentFileName = (std::string)GameInfo->archiveFilename + "|" + GameInfo->filename;
		else
			recentFileName = nameo;
		
		AddRecentFile(recentFileName.c_str());

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

/// Shows an Open File dialog and opens the ROM if the user selects a ROM file.
/// @param hParent Handle of the main window
/// @param initialdir Directory that's pre-selected in the Open File dialog.
void LoadNewGamey(HWND hParent, const char *initialdir)
{
	const char filter[] = "All usable files(*.nes,*.nsf,*.fds,*.unf,*.zip,*.rar,*.7z,*.gz)\0*.nes;*.nsf;*.fds;*.unf;*.zip;*.rar;*.7z;*.gz\0All non-compressed usable files(*.nes,*.nsf,*.fds,*.unf)\0*.nes;*.nsf;*.fds;*.unf\0All files (*.*)\0*.*\0";
	char nameo[2048];

	// Create the Open File dialog
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));

	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle=FCEU_NAME" Open File...";
	ofn.lpstrFilter=filter;
	nameo[0]=0;
	ofn.hwndOwner=hParent;
	ofn.lpstrFile=nameo;
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY; //OFN_EXPLORER|OFN_ENABLETEMPLATE|OFN_ENABLEHOOK;
	std::string stdinitdir =FCEU_GetPath(FCEUMKF_ROMS);
	ofn.lpstrInitialDir = initialdir ? initialdir : stdinitdir.c_str();

	// Show the Open File dialog
	if(GetOpenFileName(&ofn))
	{
		ALoad(nameo);
	}
}

void GetMouseData(uint32 (&md)[3])
{
	if(FCEUMOV_Mode() == MOVIEMODE_PLAY)
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

//Message loop of the main window
LRESULT FAR PASCAL AppWndProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	int whichContext = 0;

	switch(msg)
	{
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
		mouseb=wParam;
		goto proco;

	case WM_RBUTTONUP:
	{
		hfceuxcontext = LoadMenu(fceu_hInstance,"FCEUCONTEXTMENUS");

		//If There is a movie loaded in read only
		if (GameInfo && FCEUMOV_Mode(MOVIEMODE_PLAY|MOVIEMODE_RECORD) && movie_readonly)
		{
			hfceuxcontextsub = GetSubMenu(hfceuxcontext,0);
			whichContext = 0;
		}
		
		//If there is a movie loaded in read+write
		else if (GameInfo && FCEUMOV_Mode(MOVIEMODE_PLAY|MOVIEMODE_RECORD) && !movie_readonly)
		{
			hfceuxcontextsub = GetSubMenu(hfceuxcontext,3);
			whichContext = 3;
		}

		
		//If there is a ROM loaded but no movie
		else if (GameInfo)
		{
			hfceuxcontextsub = GetSubMenu(hfceuxcontext,1);
			whichContext = 1;
		}
		
		//Else no ROM
		else
		{
			hfceuxcontextsub = GetSubMenu(hfceuxcontext,2);
			whichContext = 2;
		}
		UpdateContextMenuItems(hfceuxcontextsub, whichContext);
		TrackPopupMenu(hfceuxcontextsub,0,(MainWindow_wndx+mousex),(MainWindow_wndy+mousey),TPM_RIGHTBUTTON,hWnd,0);
		
	}

	case WM_MOVE: 
	{
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
			case SIZE_MAXIMIZED: 
				ismaximized = 1;
				SetMainWindowStuff();
				break;
			case SIZE_RESTORED: 
				ismaximized = 0;
				SetMainWindowStuff();
				break;
		}
		break;
	case WM_SIZING:
		{
			RECT *wrect=(RECT *)lParam;
			RECT srect;

			int h=wrect->bottom-wrect->top;
			int w=wrect->right-wrect->left;
			int how = 0;

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

			len=DragQueryFile((HDROP)wParam,0,0,0)+1; 
			if((ftmp=(char*)malloc(len))) 
			{
				DragQueryFile((HDROP)wParam,0,ftmp,len); 
				string fileDropped = ftmp;
				
				//-------------------------------------------------------
				//Check if Movie file
				//-------------------------------------------------------
				if (!(fileDropped.find(".fm2") == string::npos))	 //ROM is already loaded and .fm2 in filename
				{
					if (GameInfo && !(fileDropped.find(".fm2") == string::npos)) //.fm2 is at the end of the filename so that must be the extension
						FCEUI_LoadMovie(ftmp, 1, false, false);		 //We are convinced it is a movie file, attempt to load it
				}
				//-------------------------------------------------------
				//If not a movie, Load it as a ROM file
				//-------------------------------------------------------
				else
				{
				ALoad(ftmp);
				free(ftmp);
				}
				
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
				char*& fname = recent_files[wParam - MENU_FIRST_RECENT_FILE];
				if(fname)
				{
					ALoad(fname);
				}
			}
			switch(LOWORD(wParam))
			{
			//File Menu-------------------------------------------------------------
			case FCEU_CONTEXT_OPENROM:
			case MENU_OPEN_FILE:
				LoadNewGamey(hWnd, 0);
				break;
			case FCEU_CONTEXT_CLOSEROM:
			case MENU_CLOSE_FILE:
				CloseGame();
				break;
			case MENU_SAVE_STATE:	//Save state as
				FCEUD_SaveStateAs();
				break;
			case MENU_LOAD_STATE:	//Load state as
				FCEUD_LoadStateFrom();
				break;

			//Movie submenu
			case FCEUX_CONTEXT_RECORDMOVIE:
			case MENU_RECORD_MOVIE:
				FCEUD_MovieRecordTo();
				break;
			case FCEUX_CONTEXT_REPLAYMOVIE:
			case MENU_REPLAY_MOVIE:
				// Replay movie menu was selected
				FCEUD_MovieReplayFrom();
				break;
			case FCEU_CONTEXT_STOPMOVIE:
			case MENU_STOP_MOVIE:
				FCEUI_StopMovie();
				break;
			case FCEU_CONTEXT_PLAYMOVIEFROMBEGINNING:
			case ID_FILE_PLAYMOVIEFROMBEGINNING:
				FCEUI_MoviePlayFromBeginning();
				break;

			//Record Avi/Wav submenu
			case MENU_RECORD_AVI:
				FCEUD_AviRecordTo();
				break;
			case MENU_STOP_AVI:
				FCEUD_AviStop();
				break;
			case MENU_RECORD_WAV: 
				loggingSound = CreateSoundSave();
				break;
			case MENU_STOP_WAV: 
				CloseWave();
				loggingSound = false;
				break;

			case FCEUX_CONTEXT_SCREENSHOT:
			case ID_FILE_SCREENSHOT:
				FCEUI_SaveSnapshot(); 
				break;

			//Lua submenu
			case ID_FILE_RUNLUASCRIPT:
				FCEUD_LuaRunFrom(); 
				break;
			case ID_FILE_STOPLUASCRIPT:
				FCEU_LuaStop();
				break;

			case MENU_EXIT:
				DoFCEUExit();
				break;

			//NES Menu--------------------------------------------------------------
			case MENU_RESET:
				FCEUI_ResetNES();
				break;
			case MENU_POWER:
				FCEUI_PowerNES();
				break;
			case MENU_EJECT_DISK:
				FCEUI_FDSInsert();
				break;
			case MENU_SWITCH_DISK:
				FCEUI_FDSSelect();
				break;
			case MENU_INSERT_COIN:
				FCEUI_VSUniCoin();
				break;
			
			//Emulation submenu
			case ID_NES_PAUSE:
				FCEUI_ToggleEmulationPause();
				UpdateCheckedMenuItems();
				break;
			case ID_NES_TURBO:
				FCEUD_TurboToggle();
				break;

			//Emulation speed submenu
			case ID_NES_SPEEDUP:
				FCEUD_SetEmulationSpeed(3);
				break;
			case ID_NES_SLOWDOWN:
				FCEUD_SetEmulationSpeed(1);
				break;
			case ID_NES_SLOWESTSPEED:
				FCEUD_SetEmulationSpeed(0);
				break;
			case ID_NES_NORMALSPEED:
				FCEUD_SetEmulationSpeed(2);
				break;
			case ID_NES_FASTESTSPEED:
				FCEUD_SetEmulationSpeed(4);
				break;
			
			//Config Menu-----------------------------------------------------------
			case MENU_HIDE_MENU:
				ToggleHideMenu();
				break;
			case MENU_RUN_IN_BACKGROUND:
				eoptions ^= EO_BGRUN;
				if((eoptions & EO_BGRUN) == 0)
				{
					EnableBackgroundInput = 0;
					KeyboardSetBackgroundAccess(EnableBackgroundInput!=0);
					JoystickSetBackgroundAccess(EnableBackgroundInput!=0);
				}
				UpdateCheckedMenuItems();
				break;
			case MENU_BACKGROUND_INPUT:
				EnableBackgroundInput ^= 1;
				eoptions |= EO_BGRUN * EnableBackgroundInput;
				KeyboardSetBackgroundAccess(EnableBackgroundInput!=0);
				JoystickSetBackgroundAccess(EnableBackgroundInput!=0);
				UpdateCheckedMenuItems();
				break;
			case MENU_ENABLE_AUTOSAVE:
				EnableAutosave ^= 1;
				UpdateCheckedMenuItems();
				break;
			case MENU_DISPLAY_FA_LAGSKIP:
				frameAdvanceLagSkip ^= 1;
				UpdateCheckedMenuItems();
				break;
			
			//Display submenu
			case MENU_INPUTDISPLAY_0: //Input display off
				input_display = 0;
				UpdateCheckedMenuItems();
				break;
			case MENU_INPUTDISPLAY_1: //Input display - 1 player
				input_display = 1;
				UpdateCheckedMenuItems();
				break;
			case MENU_INPUTDISPLAY_2: //Input display - 2 player
				input_display = 2;
				UpdateCheckedMenuItems();
				break;
			case MENU_INPUTDISPLAY_4: //Input display - 4 player
				input_display = 4;
				UpdateCheckedMenuItems();
				break;
			case MENU_DISPLAY_LAGCOUNTER:
				lagCounterDisplay ^= 1;
				UpdateCheckedMenuItems();
				break;
			case ID_DISPLAY_FRAMECOUNTER:
				FCEUI_MovieToggleFrameDisplay();
				UpdateCheckedMenuItems();
				break;
			case MENU_DISPLAY_BG:
			case MENU_DISPLAY_OBJ:
				{
					bool spr, bg;
					FCEUI_GetRenderPlanes(spr,bg);
					if(LOWORD(wParam)==MENU_DISPLAY_BG)
						bg = !bg;
					else
						spr = !spr;
					FCEUI_SetRenderPlanes(spr,bg);
				}
				break;

			case MENU_GAME_GENIE:
				genie ^= 1;
				FCEUI_SetGameGenie(genie!=0);
				UpdateCheckedMenuItems();
				break;
			case MENU_PAL:
				pal_emulation ^= 1;
				FCEUI_SetVidSystem(pal_emulation);
				RefreshThrottleFPS();
				UpdateCheckedMenuItems();
				PushCurrentVideoSettings();
				break;
				case MENU_DIRECTORIES:
				ConfigDirectories();
				break;
			case MENU_GUI_OPTIONS:
				ConfigGUI();
				break;
			case MENU_INPUT:
				ConfigInput(hWnd);
				break;
			case MENU_NETWORK:
				ShowNetplayConsole();
				break;
			case MENU_PALETTE:
				ConfigPalette();
				break;
			case MENU_SOUND:
				ConfigSound();
				break;
			case MENU_TIMING:
				ConfigTiming();
				break;
			case MENU_VIDEO:
				ConfigVideo();
				break;
			case MENU_HOTKEYS:
				MapInput();
				break;
			case MENU_MOVIEOPTIONS:
				OpenMovieOptions();
				break;

			//Tools Menu---------------------------------------------------------------
			case MENU_CHEATS:
				ConfigCheats(hWnd);
				break;
			case MENU_MEMORY_WATCH:
				CreateMemWatch();
				break;
			case MENU_RAMFILTER:
				DoByteMonitor(); 
				break;
			case ACCEL_CTRL_E:
			case MENU_TASEDIT:
				DoTasEdit();
				break;
			case MENU_CONVERT_MOVIE:
				ConvertFCM(hWnd);
				break;

			//AutoFire Pattern submenu
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
			//Autofire Offset submenu
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
			case ID_TOOLS_TEXTHOOKER:
				DoTextHooker();
				break;

			//Debug Menu-------------------------------------------------------------
			case MENU_DEBUGGER:
				DoDebug(0);
				break;
			case MENU_PPUVIEWER:
				DoPPUView();
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
			case MENU_CDLOGGER:
				DoCDLogger();
				break;
			case MENU_GAMEGENIEDECODER:
				DoGGConv();
				break;

			//Help Menu--------------------------------------------------------------
			case MENU_HELP:
				OpenHelpWindow();
				break;
			case MENU_MSGLOG:
				MakeLogWindow();
				break;
			case MENU_ABOUT:
				ShowAboutBox();
				break;
			
			//Context Menus------------------------------------------------------
			//View comments and subtitles
			case FCEUX_CONTEXT_VIEWCOMMENTSSUBTITLES:
				CreateDialog(fceu_hInstance, "IDD_REPLAY_METADATA", hWnd, ReplayMetadataDialogProc);
				break;

			//Undo Savestate
			case FCEUX_CONTEXT_UNDOSAVESTATE:
				SwapSaveState();
				break;

			//Undo Loadstate
			case FCEUX_CONTEXT_UNDOLOADSTATE:
				if (CheckBackupSaveStateExist() && redoLS)
					RedoLoadState();
				else if (CheckBackupSaveStateExist() && undoLS)
					LoadBackup();
				break;

			//Load last auto-save
			case FCEUX_CONTEXT_REWINDTOLASTAUTO:
				FCEUI_Autosave();
				break;

			//Create a backup movie file
			case FCEUX_CONTEXT_MAKEBACKUP:
				FCEUI_MakeBackupMovie(true);
				break;
			//Game + Movie - Help
			case FCEU_CONTEXT_MOVIEHELP:
				OpenHelpWindow(moviehelp);
				break;
			
			//No Game - Help
			case FCEU_CONTEXT_FCEUHELP:
				OpenHelpWindow(gettingstartedhelp);
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
			break; // Hopefully this won't break DInput...

		if(fullscreen || tog)
		{
			if(wParam==VK_MENU)
				break;
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
			//Only disable command keys if a game is loaded(and the other conditions are right, of course).
			if(InputType[2]==SIFC_FKB)
			{
				if(wParam==VK_SCROLL)
				{
					cidisabled^=1;
					FCEUI_DispMessage("Family Keyboard %sabled.",cidisabled?"en":"dis");
				}
				if(cidisabled)
					break; // Hopefully this won't break DInput...
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
		UpdateCheckedMenuItems();
		UpdateMenuHotkeys();
		EnableMenuItem(fceumenu,MENU_RESET,MF_BYCOMMAND | (FCEU_IsValidUI(FCEUI_RESET)?MF_ENABLED:MF_GRAYED));
		EnableMenuItem(fceumenu,MENU_POWER,MF_BYCOMMAND | (FCEU_IsValidUI(FCEUI_POWER)?MF_ENABLED:MF_GRAYED));
		EnableMenuItem(fceumenu,MENU_TASEDIT,MF_BYCOMMAND | (FCEU_IsValidUI(FCEUI_TASEDIT)?MF_ENABLED:MF_GRAYED));
		EnableMenuItem(fceumenu,MENU_CLOSE_FILE,MF_BYCOMMAND | (FCEU_IsValidUI(FCEUI_CLOSEGAME)?MF_ENABLED:MF_GRAYED));
		EnableMenuItem(fceumenu,MENU_RECENT_FILES,MF_BYCOMMAND | (FCEU_IsValidUI(FCEUI_OPENGAME)?MF_ENABLED:MF_GRAYED));
		EnableMenuItem(fceumenu,MENU_OPEN_FILE,MF_BYCOMMAND | (FCEU_IsValidUI(FCEUI_OPENGAME)?MF_ENABLED:MF_GRAYED));
		EnableMenuItem(fceumenu,MENU_RECORD_MOVIE,MF_BYCOMMAND | (FCEU_IsValidUI(FCEUI_RECORDMOVIE)?MF_ENABLED:MF_GRAYED));
		EnableMenuItem(fceumenu,MENU_REPLAY_MOVIE,MF_BYCOMMAND | (FCEU_IsValidUI(FCEUI_PLAYMOVIE)?MF_ENABLED:MF_GRAYED));
		EnableMenuItem(fceumenu,MENU_STOP_MOVIE,MF_BYCOMMAND | (FCEU_IsValidUI(FCEUI_STOPMOVIE)?MF_ENABLED:MF_GRAYED));
		EnableMenuItem(fceumenu,ID_FILE_PLAYMOVIEFROMBEGINNING,MF_BYCOMMAND | (FCEU_IsValidUI(FCEUI_PLAYFROMBEGINNING)?MF_ENABLED:MF_GRAYED));
		EnableMenuItem(fceumenu,MENU_SAVE_STATE,MF_BYCOMMAND | (FCEU_IsValidUI(FCEUI_SAVESTATE)?MF_ENABLED:MF_GRAYED));
		EnableMenuItem(fceumenu,MENU_LOAD_STATE,MF_BYCOMMAND | (FCEU_IsValidUI(FCEUI_LOADSTATE)?MF_ENABLED:MF_GRAYED));
		EnableMenuItem(fceumenu,MENU_STOP_AVI,MF_BYCOMMAND | (FCEUI_AviIsRecording()?MF_ENABLED:MF_GRAYED));
		EnableMenuItem(fceumenu,MENU_STOP_WAV,MF_BYCOMMAND | (loggingSound?MF_ENABLED:MF_GRAYED));
		EnableMenuItem(fceumenu,ID_FILE_STOPLUASCRIPT,MF_BYCOMMAND | (luaRunning?MF_ENABLED:MF_GRAYED));
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
		if(winsizemuly < mult && mult < 3) //11/14/2008, adelikat: added && mult < 3 and extra code to meet mult >=3 conditions
		{
			if(eoptions&EO_FORCEASPECT)
				winsizemulx *= mult / winsizemuly;
			winsizemuly = mult;
		}
		else if (winsizemuly < mult&& mult >= 3) //11/14/2008, adelikat: This was probably a hacky solution.  But when special scalar = 3 and aspect correction is on,
			if(eoptions&EO_FORCEASPECT)			//then x is corrected to a wider ratio (.5 of what this code seems to expect) so I added a special circumstance for these 2 situations
				winsizemulx *= (mult+0.5) / winsizemuly;	//And adjusted the special scaling by .5
			winsizemuly = mult;
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


//Destroys the main window
void ByebyeWindow()
{
	SetMenu(hAppWnd, 0);
	DestroyMenu(fceumenu);
	DestroyWindow(hAppWnd);
}

/// reates the main window.
/// @return Flag that indicates failure (0) or success (1)
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

	// Update recent files menu
	UpdateRMenu(recentmenu, recent_files, MENU_RECENT_FILES, MENU_FIRST_RECENT_FILE);

	updateGameDependentMenus(0);
	if (MainWindow_wndx==-32000) MainWindow_wndx=0; //Just in case
	if (MainWindow_wndy==-32000) MainWindow_wndy=0;
	hAppWnd = CreateWindowEx(
		0,
		"FCEULTRA",
		FCEU_NAME_AND_VERSION,
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

/// @return Flag that indicates failure (0) or success (1).
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

//Shows an Open File menu and starts recording an AVI
void FCEUD_AviRecordTo(void)
{
	OPENFILENAME ofn;
	char szChoice[MAX_PATH];

	string tempFilename, aviFilename;
		
	std::string aviDirectory = FCEU_GetPath(21);  //21 = FCEUMKF_AVI
	if (aviDirectory.find_last_of("\\") != (aviDirectory.size()-1))
		aviDirectory.append("\\");			//if directory override has no / then add one

	//if we are playing a movie, construct the filename from the current movie.
	if(FCEUMOV_Mode(MOVIEMODE_PLAY|MOVIEMODE_RECORD))
	{
		tempFilename = GetMfn();	//get movie filename
		tempFilename.erase(0,1);	//remove dot
	}
	//else construct it from the ROM name.
	else
		tempFilename = GetRomName();
	
	aviFilename = aviDirectory + tempFilename;	//concate avi directory and movie filename
				
	strcpy(szChoice, aviFilename.c_str());
	char* dot = strrchr(szChoice,'.');

	if (dot) *dot='\0';
	strcat(szChoice, ".avi");

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

//Stop AVI recording
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

INT_PTR CALLBACK DlgLuaScriptDialog(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam) {

	static int *success;

	switch (msg) {
	case WM_INITDIALOG:
		{

		// Nothing very useful to do
		success = (int*)lParam;
		return TRUE;
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
			case IDOK:
			{
				char filename[MAX_PATH];
				GetDlgItemText(hDlg, 1096, filename, MAX_PATH);
				if (FCEU_LoadLuaCode(filename)) {
					*success = 1;
					// For user's convenience, don't close dialog unless we're done.
					// Users who make syntax errors and fix/reload will thank us.
					EndDialog(hDlg, 1);
				} else {
					//MessageBox(hDlg, "Couldn't load script.", "Oops", MB_OK); // XXX better if errors are displayed by the Lua code.
					*success = 0;
				}
				return TRUE;
			}
			case IDCANCEL:
			{
				EndDialog(hDlg, 0);
				return TRUE;
			}
			case 1359:
			{
				OPENFILENAME  ofn;
				char  szFileName[MAX_PATH];
				szFileName[0] = '\0';
				ZeroMemory( (LPVOID)&ofn, sizeof(OPENFILENAME) );
				ofn.lStructSize = sizeof(OPENFILENAME);
				ofn.hwndOwner = hDlg;
				ofn.lpstrFilter = "Lua scripts\0*.lua\0All files\0*.*\0\0";
				ofn.lpstrFile = szFileName;
				ofn.lpstrDefExt = "lua";
				ofn.nMaxFile = MAX_PATH;
				ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST; // hide previously-ignored read-only checkbox (the real read-only box is in the open-movie dialog itself)
				if(GetOpenFileName( &ofn ))
				{
					SetWindowText(GetDlgItem(hDlg, 1096), szFileName);
				}
				//SetCurrentDirectory(movieDirectory);
				return TRUE;
			}
		}


	}
	//char message[1024];
//	sprintf(message, "Unkonwn command %d,%d",msg,wParam);
	//MessageBox(hDlg, message, TEXT("Range Error"), MB_OK);

//	printf("Unknown entry %d,%d,%d\n",msg,wParam,lParam);
	// All else, fall off
	return FALSE;

}

void FCEUD_LuaRunFrom(void)
{
	int success = 0;

	//mbg 8/2/08 - i decided i didnt like this dialog box. so for now we are just going to run the script directly
	//DialogBoxParam(fceu_hInstance, "IDD_LUA_ADD", hAppWnd, DlgLuaScriptDialog,(LPARAM) &success);

	OPENFILENAME  ofn;
	char  szFileName[MAX_PATH];
	szFileName[0] = '\0';
	ZeroMemory( (LPVOID)&ofn, sizeof(OPENFILENAME) );
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hAppWnd;
	ofn.lpstrFilter = "Lua scripts\0*.lua\0All files\0*.*\0\0";
	ofn.lpstrFile = szFileName;
	ofn.lpstrDefExt = "lua";
	ofn.nMaxFile = MAX_PATH;
	ofn.Flags = OFN_HIDEREADONLY | OFN_FILEMUSTEXIST; // hide previously-ignored read-only checkbox (the real read-only box is in the open-movie dialog itself)
	std::string initdir = FCEU_GetPath(FCEUMKF_LUA);
	ofn.lpstrInitialDir=initdir.c_str();
	if(GetOpenFileName( &ofn ))
	{
		FCEU_LoadLuaCode(szFileName);
	}
}

void ChangeContextMenuItemText(int menuitem, string text, HMENU menu)
{
	MENUITEMINFO moo;
	moo.cbSize = sizeof(moo);
	moo.fMask = MIIM_TYPE;
	moo.cch = NULL;
	GetMenuItemInfo(menu, menuitem, FALSE, &moo);
	moo.dwTypeData = (LPSTR)text.c_str();
	SetMenuItemInfo(menu, menuitem, FALSE, &moo);
}

void ChangeMenuItemText(int menuitem, string text)
{
	MENUITEMINFO moo;
	moo.cbSize = sizeof(moo);
	moo.fMask = MIIM_TYPE;
	moo.cch = NULL;
	GetMenuItemInfo(fceumenu, menuitem, FALSE, &moo);
	moo.dwTypeData = (LPSTR)text.c_str();
	SetMenuItemInfo(fceumenu, menuitem, FALSE, &moo);
}

void UpdateMenuHotkeys()
{
	//Update all menu items that can be called from a hotkey to include the current hotkey assignment
	string combo, combined;

	//-------------------------------FILE---------------------------------------
	//Open ROM
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_OPENROM]);
	combined = "&Open ROM...\t" + combo;
	ChangeMenuItemText(MENU_OPEN_FILE, combined);

	//Close ROM
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_CLOSEROM]);
	combined = "&Close\t" + combo;
	ChangeMenuItemText(MENU_CLOSE_FILE, combined);

	//Loadstate from
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_LOAD_STATE_FROM]);
	combined = "Load State From...\t" + combo;
	ChangeMenuItemText(MENU_LOAD_STATE, combined);

	//Savestate as
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_SAVE_STATE_AS]);
	combined = "Save State As...\t" + combo;
	ChangeMenuItemText(MENU_SAVE_STATE, combined);

	//Record Movie
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_MOVIE_RECORD_TO]);
	combined = "Record Movie...\t" + combo;
	ChangeMenuItemText(MENU_RECORD_MOVIE, combined);

	//Replay movie
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_MOVIE_REPLAY_FROM]);
	combined = "Replay Movie...\t" + combo;
	ChangeMenuItemText(MENU_REPLAY_MOVIE, combined); 

	//Stop movie
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_MOVIE_STOP]);
	combined = "Stop Movie\t" + combo;
	ChangeMenuItemText(MENU_STOP_MOVIE, combined); 

	//Play Movie from Beginning
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_MOVIE_PLAY_FROM_BEGINNING]);
	combined = "Play from beginning\t" + combo;
	ChangeMenuItemText(ID_FILE_PLAYMOVIEFROMBEGINNING, combined); 

	//Screenshot
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_SCREENSHOT]);
	combined = "Screenshot\t" + combo;
	ChangeMenuItemText(ID_FILE_SCREENSHOT, combined); 

	//Record AVI
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_AVI_RECORD_AS]);
	combined = "Record AVI...\t" + combo;
	ChangeMenuItemText(MENU_RECORD_AVI, combined); 

	//Stop AVI
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_AVI_STOP]);
	combined = "Stop AVI\t" + combo;
	ChangeMenuItemText(MENU_STOP_AVI, combined); 

	//-------------------------------NES----------------------------------------
	//Reset
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_RESET]);
	combined = "&Reset\t" + combo;
	ChangeMenuItemText(MENU_RESET, combined); 

	//Power
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_POWER]);
	combined = "&Power\t" + combo;
	ChangeMenuItemText(MENU_POWER, combined);

	//Eject/Insert Disk
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_FDS_EJECT_INSERT]);
	combined = "&Eject/Insert Disk\t" + combo;
	ChangeMenuItemText(MENU_EJECT_DISK, combined);

	//Switch Disk Side
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_FDS_SIDE_SELECT]);
	combined = "&Switch Disk Side\t" + combo;
	ChangeMenuItemText(MENU_SWITCH_DISK, combined);
	
	//Insert Coin
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_VSUNI_COIN]);
	combined = "&Insert Coin\t" + combo;
	ChangeMenuItemText(MENU_INSERT_COIN, combined);

	//Pause
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_PAUSE]);
	combined = "Pause\t" + combo;
	ChangeMenuItemText(ID_NES_PAUSE, combined);

	//Frame Advance
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_FRAME_ADVANCE]);
	combined = "Frame Advance\t" + combo;
	ChangeMenuItemText(ID_NES_FRAMEADVANCE, combined);

	//Turbo
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_SPEED_TURBO_TOGGLE]);
	combined = "Turbo\t" + combo;
	ChangeMenuItemText(ID_NES_TURBO, combined);

	//Speed Up
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_SPEED_FASTER]);
	combined = "Speed Up\t" + combo;
	ChangeMenuItemText(ID_NES_SPEEDUP, combined);

	//Slow Down
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_SPEED_SLOWER]);
	combined = "Slow Down\t" + combo;
	ChangeMenuItemText(ID_NES_SLOWDOWN, combined);

	//Slowest Speed
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_SPEED_SLOWEST]);
	combined = "Slowest Speeed\t" + combo;
	ChangeMenuItemText(ID_NES_SLOWESTSPEED, combined);

	//Normal Speed
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_SPEED_NORMAL]);
	combined = "Normal Speed\t" + combo;
	ChangeMenuItemText(ID_NES_NORMALSPEED, combined);

	//Fastest Speed
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_SPEED_FASTEST]);
	combined = "Fastest Speed\t" + combo;
	ChangeMenuItemText(ID_NES_FASTESTSPEED, combined);

	//-------------------------------Config-------------------------------------
	//Hide Menu
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_HIDE_MENU_TOGGLE]);
	combined = "Hide Menu\t" + combo;
	ChangeMenuItemText(MENU_HIDE_MENU, combined);

	//Frame Adv. skip lag
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_FRAMEADV_SKIPLAG]);
	combined = "Frame Adv. - Skip Lag\t" + combo;
	ChangeMenuItemText(MENU_DISPLAY_FA_LAGSKIP, combined);

	//Movie Status Icon
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_MOVIE_ICON_DISPLAY_TOGGLE]);
	combined = "Movie Status Icon\t" + combo;
	ChangeMenuItemText(MENU_SHOW_STATUS_ICON, combined);

	//Lag Counter
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_MISC_DISPLAY_LAGCOUNTER_TOGGLE]);
	combined = "Lag Counter\t" + combo;
	ChangeMenuItemText(MENU_DISPLAY_LAGCOUNTER, combined);

	//Frame Counter
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_MOVIE_FRAME_DISPLAY_TOGGLE]);
	combined = "Frame Counter\t" + combo;
	ChangeMenuItemText(ID_DISPLAY_FRAMECOUNTER, combined);

	//Movie Subtitles
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_MISC_DISPLAY_MOVIESUBTITLES]);
	combined = "Movie Subtitles\t" + combo;
	ChangeMenuItemText(ID_DISPLAY_MOVIESUBTITLES, combined);

	//Graphics: BG
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_MISC_DISPLAY_BG_TOGGLE]);
	combined = "Graphics: BG\t" + combo;
	ChangeMenuItemText(MENU_DISPLAY_BG, combined);

	//Graphics: OBJ
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_MISC_DISPLAY_OBJ_TOGGLE]);
	combined = "Graphics: OBJ\t" + combo;
	ChangeMenuItemText(MENU_DISPLAY_OBJ, combined);

	//-------------------------------Tools--------------------------------------
	//Open Cheats
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_TOOL_OPENCHEATS]);
	combined = "&Cheats...\t" + combo;
	ChangeMenuItemText(MENU_CHEATS, combined);

	//Open Memory Watch
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_TOOL_OPENMEMORYWATCH]);
	combined = "&Memory Watch...\t" + combo;
	ChangeMenuItemText(MENU_MEMORY_WATCH, combined);

	//-------------------------------Debug--------------------------------------
	//Open Debugger
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_TOOL_OPENDEBUGGER]);
	combined = "Debugger...\t" + combo;
	ChangeMenuItemText(MENU_DEBUGGER, combined);

	//Open PPU Viewer
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_TOOL_OPENPPU]);
	combined = "PPU Viewer...\t" + combo;
	ChangeMenuItemText(MENU_PPUVIEWER, combined);

	//Open Hex editor
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_TOOL_OPENHEX]);
	combined = "Hex Editor...\t" + combo;
	ChangeMenuItemText(MENU_HEXEDITOR, combined);

	//Open Trace Logger
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_TOOL_OPENTRACELOGGER]);
	combined = "Trace Logger...\t" + combo;
	ChangeMenuItemText(MENU_TRACELOGGER, combined);
	
	//Open Code/Data Logger
	combo = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_TOOL_OPENCDLOGGER]);
	combined = "Code/Data Logger...\t" + combo;
	ChangeMenuItemText(MENU_CDLOGGER, combined);
}