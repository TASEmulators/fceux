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


#include "common.h"

// I like hacks.
#define uint8 __UNO492032
#include <winsock.h>
#include "ddraw.h"
#undef LPCWAVEFORMATEX
#include "dsound.h"
#include "dinput.h"
#include <commctrl.h>
#include <shlobj.h>     // For directories configuration dialog.
#undef uint8

#include "../../types.h"
#include "../../fceu.h"
#include "../../state.h"
#include "../../debug.h"
#include "ppuview.h"
#include "debugger.h"
#include "input.h"
#include "netplay.h"
#include "joystick.h"
#include "keyboard.h"
#include "cheat.h"
#include "debug.h"
#include "ppuview.h"
#include "ntview.h"
#include "memview.h"
#include "tracer.h"
#include "cdlogger.h"

#include "basicbot.h"
#include "args.h"
#include "config.h"

// #defines

#define VNSCLIP  ((eoptions&EO_CLIPSIDES)?8:0)
#define VNSWID   ((eoptions&EO_CLIPSIDES)?240:256)

#define SO_FORCE8BIT  1
#define SO_SECONDARY  2
#define SO_GFOCUS     4
#define SO_D16VOL     8
#define SO_MUTEFA     16
#define SO_OLDUP      32

#define GOO_DISABLESS   1       /* Disable screen saver when game is loaded. */
#define GOO_CONFIRMEXIT 2       /* Confirmation before exiting. */
#define GOO_POWERRESET  4       /* Confirm on power/reset. */

//---------------------------
//mbg merge 6/29/06 - new aboutbox

#ifdef _M_X64
  #define _MSVC_ARCH "x64"
#else
  #define _MSVC_ARCH "x86"
#endif
#ifdef _DEBUG
 #define _MSVC_BUILD "debug"
#else 
 #define _MSVC_BUILD "release"
#endif
#define __COMPILER__STRING__ "msvc " _Py_STRINGIZE(_MSC_VER) " " _MSVC_ARCH " " _MSVC_BUILD
#define _Py_STRINGIZE(X) _Py_STRINGIZE1((X))
#define _Py_STRINGIZE1(X) _Py_STRINGIZE2 ## X
#define _Py_STRINGIZE2(X) #X
//re: http://72.14.203.104/search?q=cache:HG-okth5NGkJ:mail.python.org/pipermail/python-checkins/2002-November/030704.html+_msc_ver+compiler+version+string&hl=en&gl=us&ct=clnk&cd=5

// External functions

void ResetVideo(void);
void ShowCursorAbs(int w);
void HideFWindow(int h);
void FixWXY(int pref);
void SetMainWindowStuff(void);
int GetClientAbsRect(LPRECT lpRect);
void UpdateFCEUWindow(void);
void FCEUD_Update(uint8 *XBuf, int32 *Buffer, int Count);
void ApplyDefaultCommandMapping(void);

// Internal variables

uint8 *xbsave = NULL;
int eoptions = EO_BGRUN | EO_FORCEISCALE;

/**
* Handle of the main window.
**/
HWND hAppWnd = 0;

/**
* FCE Ultra Instance 
**/
HINSTANCE fceu_hInstance;

HRESULT  ddrval;

/** 
* Contains the names of the overridden standard directories
* in the order cheats, misc, nonvol, states, snaps, ..., base
**/
static char *directory_names[6] = {0, 0, 0, 0, 0, 0};

/**
* Contains the names of the default directories.
**/
static const char *default_directory_names[5] = {"cheats", "sav", "fcs", "snaps", "movie"};

#define NUMBER_OF_DIRECTORIES sizeof(directory_names) / sizeof(*directory_names)
#define NUMBER_OF_DEFAULT_DIRECTORIES sizeof(default_directory_names) / sizeof(*default_directory_names)

static char TempArray[2048];

/**
* Contains the base directory of FCE
**/
static char BaseDirectory[2048];

static int exiting = 0;
static volatile int moocow = 0;

/* Some timing-related variables (now ignored). */
static int maxconbskip = 32;             /* Maximum consecutive blit skips. */
static int ffbskip = 32;              /* Blit skips per blit when FF-ing */

static int moviereadonly = 1;

static int fullscreen = 0;
static int soundflush = 0;
// Flag that indicates whether Game Genie is enabled or not.
static int genie = 0;

// Flag that indicates whether PAL Emulation is enabled or not.
static int pal_emulation = 0;
static int status_icon = 1;
static int windowedfailed;
static double saspectw = 1, saspecth = 1;
static double winsizemulx = 1, winsizemuly = 1;
static int winwidth, winheight;
static int ismaximized = 0;

static volatile int nofocus = 0;
static volatile int _userpause = 0; //mbg merge 7/18/06 changed tasbuild was using this only in a couple of places

static uint32 goptions = GOO_DISABLESS;

static int soundrate = 44100;
static int soundbuftime = 50;
/*static*/ int soundoptions = SO_SECONDARY | SO_GFOCUS;
static int soundvolume = 100;
static int soundquality = 0;
extern int autoHoldKey, autoHoldClearKey;
extern int frame_display, input_display;

//mbg merge 7/17/06 did these have to be unsigned?
static int srendline, erendline;
static int srendlinen = 8;
static int erendlinen = 231;
static int srendlinep = 0;
static int erendlinep = 239;
static int totallines;

static uint8 cpalette[192];
static int vmod = 0;
int soundo = 1;
static int ntsccol = 0, ntsctint, ntschue;

//mbg 6/30/06 - indicates that the main loop should close the game as soon as it can
int closeGame = 0;

static int changerecursive=0;

// qfox 09/17/06: moved the skipcount outside because it was completely pointless
//                in there.
/**
 * Counts the number of frames that have not been displayed
 * Used for the bot, to skip frames (and save time).
 **/
int skipcount = 0;

// Internal functions

void SetDirs()
{
	int x;

	static int jlist[6]= {
		FCEUIOD_CHEATS,
		FCEUIOD_MISC,
		FCEUIOD_NV,
		FCEUIOD_STATE,
		FCEUIOD_SNAPS, 
		FCEUIOD__COUNT};

	FCEUI_SetSnapName(eoptions & EO_SNAPNAME);

	for(x=0; x < sizeof(jlist) / sizeof(*jlist); x++)
	{
		FCEUI_SetDirOverride(jlist[x], directory_names[x]);  
	}

	if(directory_names[5])
	{
		FCEUI_SetBaseDirectory(directory_names[5]);
	}
	else
	{
		FCEUI_SetBaseDirectory(BaseDirectory);
	}
}

/**
* Creates a directory.
*
* @param dirname Name of the directory to create.
**/
void DirectoryCreator(const char* dirname)
{
	CreateDirectory(dirname, 0);
}

/**
* Removes a directory.
*
* @param dirname Name of the directory to remove.
**/
void DirectoryRemover(const char* dirname)
{
	RemoveDirectory(dirname);
}

/**
* Used to walk over the default directories array.
*
* @param callback Callback function that's called for every default directory name.
**/
void DefaultDirectoryWalker(void (*callback)(const char*))
{
	unsigned int curr_dir;

	for(curr_dir = 0; curr_dir < NUMBER_OF_DEFAULT_DIRECTORIES; curr_dir++)
	{
		if(!directory_names[curr_dir])
		{
			sprintf(
				TempArray,
				"%s\\%s",
				directory_names[NUMBER_OF_DEFAULT_DIRECTORIES] ? directory_names[NUMBER_OF_DEFAULT_DIRECTORIES] : BaseDirectory,
				default_directory_names[curr_dir]
			);

			callback(TempArray);
		}
	}
}

/* Remove empty, unused directories. */
void RemoveDirs()
{
	DefaultDirectoryWalker(DirectoryRemover);
}

/**
* Creates the default directories.
**/
void CreateDirs(void)
{
	DefaultDirectoryWalker(DirectoryCreator);
}

static char *gfsdir=0;

/**
* Fills the BaseDirectory string
*
* TODO: Potential buffer overflow caused by limited size of BaseDirectory?
**/
void GetBaseDirectory(void)
{
	unsigned int i;
	GetModuleFileName(0, (LPTSTR)BaseDirectory, sizeof(BaseDirectory) - 1);

	// Search for the last / or \ in the directory and terminate the string  there
	for(i = strlen(BaseDirectory); i >= 0 ; i--)
	{
		if(BaseDirectory[i]=='\\' || BaseDirectory[i]=='/')
		{
			BaseDirectory[i] = 0;
			return;
		}
	}
}

int BlockingCheck(void)
{
	MSG msg;
	moocow = 1;

	while( PeekMessage( &msg, 0, 0, 0, PM_NOREMOVE ) )
	{
		if( GetMessage( &msg, 0,  0, 0)>0 )
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	moocow = 0;

	return exiting ? 0 : 1;
}

static void FixFL(void)
{
	FCEUI_GetCurrentVidSystem(&srendline, &erendline);
	totallines = erendline - srendline + 1;
}

static void UpdateRendBounds(void)
{ 
	FCEUI_SetRenderedLines(srendlinen, erendlinen, srendlinep, erendlinep);
	FixFL(); 
}

/**
* Shows an error message in a message box.
*
* @param errormsg Text of the error message.
**/
void FCEUD_PrintError(const char *errormsg)
{
	AddLogText(errormsg, 1);
	
	if(fullscreen)
	{
		ShowCursorAbs(1);
	}

	MessageBox(0, errormsg, "FCE Ultra Error", MB_ICONERROR | MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
	
	if(fullscreen)
	{
		ShowCursorAbs(0);
	}
}

/**
* Generates a compiler identification string.
*
* @return Compiler identification string
**/
const char *FCEUD_GetCompilerString()
{
	return 	__COMPILER__STRING__;
}

/**
* Displays the about box
**/
void ShowAboutBox(void)
{
	MessageBox(hAppWnd, FCEUI_GetAboutString(), FCEU_NAME, MB_OK);
}

/**
* Exits FCE Ultra
**/
void DoFCEUExit()
{
	/* Wolfenstein 3D had cute exit messages. */
	char *emsg[4]={"Are you sure you want to leave?  I'll become lonely!",
		"If you exit, I'll... EAT YOUR MOUSE.",
		"You can never really exit, you know.",
		"E-x-i-t?"
	};

	KillDebugger(); //mbg merge 7/19/06 added

	if(exiting)    /* Eh, oops.  I'll need to try to fix this later. */
		return;

	if(goptions & GOO_CONFIRMEXIT)
	{
		if(IDYES != MessageBox(hAppWnd, emsg[rand() & 3], "Exit FCE Ultra?", MB_ICONQUESTION | MB_YESNO) )
		{
			return;
		}
	}

	FCEUI_StopMovie();
	FCEUD_AviStop();

	exiting = 1;
	closeGame = 1;//mbg 6/30/06 - for housekeeping purposes we need to exit after the emulation cycle finishes
}

/**
* Changes the thread priority of the main thread.
**/
void DoPriority(void)
{
	if(eoptions & EO_HIGHPRIO)
	{
		if(!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST))
		{
			AddLogText("Error setting thread priority to THREAD_PRIORITY_HIGHEST.", 1);
		}
	}
	else
	{
		if(!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_NORMAL))
		{
			AddLogText("Error setting thread priority to THREAD_PRIORITY_NORMAL.", 1);
		}
	}
}

// TODO: HORRIBLE

#include "sound.cpp"
#include "video.cpp"
#include "window.cpp"

int DriverInitialize()
{
	if(soundo)
	{
		soundo = InitSound();
	}

	SetVideoMode(fullscreen);
	InitInputStuff();             /* Initialize DInput interfaces. */

	return 1;
}

static void DriverKill(void)
{ 
	// Save config file
	sprintf(TempArray, "%s/fceu98.cfg", BaseDirectory);
	SaveConfig(TempArray);

	DestroyInput();

	ResetVideo();

	if(soundo)
	{
		TrashSound();
	}

	CloseWave();

	ByebyeWindow();
}

//mbg merge 7/18/06 - the function that contains the code that used to just be UpdateMemWatch()
void _updateMemWatch() {
	//UpdateMemWatch()
	//but soon we will do more!
}

#ifdef _USE_SHARED_MEMORY_
HANDLE mapGameMemBlock;
HANDLE mapRAM;
uint32 *BotInput;

void win_AllocBuffers(uint8 **GameMemBlock, uint8 **RAM)
{
	mapGameMemBlock = CreateFileMapping((HANDLE)0xFFFFFFFF,NULL,PAGE_READWRITE, 0, 131072,"fceu.GameMemBlock");

	if(mapGameMemBlock == NULL || GetLastError() == ERROR_ALREADY_EXISTS)
	{
		//mbg 7/28/06 - is this the proper error handling?
		//do we need to indicate to user somehow that this failed in this emu instance?
		CloseHandle(mapGameMemBlock);

		mapGameMemBlock = NULL;
		*GameMemBlock = (uint8 *) malloc(131072);
		*RAM = (uint8 *) malloc(2048);
	}
	else
	{
		*GameMemBlock = (uint8 *)MapViewOfFile(mapGameMemBlock, FILE_MAP_WRITE, 0, 0, 0);

		// set up shared memory mappings
		mapRAM = CreateFileMapping((HANDLE)0xFFFFFFFF,NULL,PAGE_READWRITE, 0, 0x800,"fceu.RAM");
		*RAM = (uint8 *)MapViewOfFile(mapRAM, FILE_MAP_WRITE, 0, 0, 0);
	}

	// Give RAM pointer to state structure
	//mbg 7/28/06 - wtf?
	extern SFORMAT SFCPU[];
	SFCPU[6].v = *RAM;

	//Bot input
	// qfox: tossed mapping alltogether
	//mapBotInput = CreateFileMapping((HANDLE)0xFFFFFFFF,NULL,PAGE_READWRITE,0, BOT_MAXFRAMES*sizeof(int), "fceu.BotInput");
	//BotInput = (uint32 *) MapViewOfFile(mapBotInput, FILE_MAP_WRITE, 0, 0, 0);
	// qfox: not working with a map anymore
	BotInput = new uint32[BOT_MAXFRAMES];
	BotInput[0] = 0;
}

void win_FreeBuffers(uint8 *GameMemBlock, uint8 *RAM)
{
	//clean up shared memory 
	if(mapRAM)
	{
		UnmapViewOfFile(mapRAM);
		CloseHandle(mapRAM);
		RAM = NULL;
	}
	else
	{
		free(RAM);
		RAM = NULL;
	}
	if(mapGameMemBlock)
	{
		UnmapViewOfFile(mapGameMemBlock);
		CloseHandle(mapGameMemBlock);
		GameMemBlock = NULL;
	}
	else
	{
		free(GameMemBlock);
		GameMemBlock = NULL;
	}

	// qfox: not working with map anymore
	delete BotInput;
}
#endif

void do_exit()
{
	DriverKill();
	timeEndPeriod(1);
	FCEUI_Kill();
}

int main(int argc,char *argv[])
{
	char *t;

	if(timeBeginPeriod(1) != TIMERR_NOERROR)
	{
		AddLogText("Error setting timer granularity to 1ms.", DO_ADD_NEWLINE);
	}

	InitCommonControls();

	if(!FCEUI_Initialize())
	{
		do_exit();
		return 1;
	}

	ApplyDefaultCommandMapping();

	srand(GetTickCount());        // rand() is used for some GUI sillyness.

	fceu_hInstance = GetModuleHandle(0);

	// Get the base directory
	GetBaseDirectory();

	// Load the config information
	sprintf(TempArray,"%s\\fceu98.cfg",BaseDirectory);
	LoadConfig(TempArray);

	// Parse the commandline arguments
	t = ParseArgies(argc, argv);

	/* Bleh, need to find a better place for this. */
	{
        pal_emulation &= 1;
        FCEUI_SetVidSystem(pal_emulation);

        genie &= 1;
        FCEUI_SetGameGenie(genie);

        fullscreen &= 1;
        soundo &= 1;

        FCEUI_SetSoundVolume(soundvolume);
		FCEUI_SetSoundQuality(soundquality);
	}

	ParseGIInput(NULL);      /* Since a game doesn't have to be
                     loaded before the GUI can be used, make
                     sure the temporary input type variables
                     are set.
                  */

	// Initialize default directories
	CreateDirs();
	SetDirs();

	DoVideoConfigFix();
	DoTimingConfigFix();

	if(eoptions & EO_CPALETTE)
	{
		FCEUI_SetPaletteArray(cpalette);
	}

	if(!t)
	{
		fullscreen=0;
	}

	CreateMainWindow();

	if(!InitDInput())
	{
		do_exit();
		return 1;
	}

	if(!DriverInitialize())
	{
		do_exit();
		return 1;
	}
 
	UpdateCheckedMenuItems();

	if(t)
	{
		ALoad(t);
	}
	else if(eoptions & EO_FOAFTERSTART)
	{
		LoadNewGamey(hAppWnd, 0);
	}

doloopy:
	UpdateFCEUWindow();  
	if(GameInfo)
	{
		while(GameInfo)
		{
	        uint8 *gfx=0; ///contains framebuffer
			int32 *sound=0; ///contains sound data buffer
			int32 ssize=0; ///contains sound samples count

			#ifdef _USE_SHARED_MEMORY_
			UpdateBasicBot();
			#endif
			FCEU_UpdateBot();

			FCEUI_Emulate(&gfx, &sound, &ssize, 0); //emulate a single frame
			FCEUD_Update(gfx, sound, ssize); //update displays and debug tools

			 //mbg 6/30/06 - close game if we were commanded to by calls nested in FCEUI_Emulate()
			 if(closeGame)
			 {
				FCEUI_CloseGame();
				GameInfo = 0;
			 }

		}
		//xbsave = NULL;
		RedrawWindow(hAppWnd,0,0,RDW_ERASE|RDW_INVALIDATE);
	}
	Sleep(50);
	if(!exiting)
		goto doloopy;

	DriverKill();
	timeEndPeriod(1);
	FCEUI_Kill();
	return(0);
}

			//mbg merge 7/19/06 
			//--------this code was added by tasbuild
			//it catches a paused condition and 
			/*static int stopCount=0;
			if(FCEUI_EmulationPaused() & 1)
			{
				if(stopCount==0)
					_updateMemWatch();

				stopCount++;
				if(stopCount > 8)
				{
					StopSound();
					stopCount = 0;
				}

				//if in bot mode, don't idle.  eat the CPU up :)
				//mbg - why should we do this? does bot mode set the paused flag? that doesnt seem right...
				if(!FCEU_BotMode())
				{
					int notAlternateThrottle = !(soundoptions&SO_OLDUP) && soundo && ((NoWaiting&1)?(256*16):fps_scale) >= 64;
					if(notAlternateThrottle)
						Sleep(5); // HACK to fix 100% CPU usage that happens sporadically when paused in background - also, this affects the repeat rate of frame advance and its sound quality
					else
						Sleep(1); // lesser, so frame advance repeat doesn't take too long with the alternate throttling
				}
				else if(stopCount == 1)
				{
					Sleep(0);
				}
			}
			else
			{
				//_updateMemWatch();
				stopCount=0;
			}*/
			//-----------------------------------



//mbg merge 7/19/06 - the function that contains the code that used to just be UpdateFCEUWindow() and FCEUD_UpdateInput()
void _updateWindow()
{
	UpdateFCEUWindow();
	FCEUD_UpdateInput();
	PPUViewDoBlit();
	UpdateMemoryView(0);
	UpdateCDLogger();
	UpdateLogWindow();
	NTViewDoBlit(0);
}

//void FCEUD_Update(uint8 *XBuf, int32 *Buffer, int Count)
//{
//	static int skipcount = 0;
//	int temp_fps_scale=(NoWaiting&1)?(256*16):fps_scale;
//	int maxskip = (temp_fps_scale<=256) ? 0 : temp_fps_scale>>8;
//
//	int ocount = Count;
//	// apply frame scaling to Count
//	Count = (Count<<8)/temp_fps_scale;
//
//	//Disable sound and throttling for BotMode--we want max speed!
//	if(FCEU_BotMode())
//	{
//		if(XBuf && (skipcount >= 64))
//		{
//			skipcount = 0;
//			FCEUD_BlitScreen(XBuf);
//		}
//		else
//		{
//			skipcount++;
//		}
//		UpdateFCEUWindow();
//		FCEUD_UpdateInput();
//		return;
//	}
//
//	if(!(soundoptions&SO_OLDUP) && soundo && temp_fps_scale >= 64)
//	{
//		// sounds better with FPS scaling, and is much less code than the other version...
//
//		int32 writeSize = GetWriteSound();
//		int32 writeCount = Count;
///*
//		// prevents delay when exiting fast-forward
//		if((NoWaiting&1) && writeCount>writeSize)
//			writeCount=writeSize;
//*/
//
//		if(Buffer && (writeCount))
//			FCEUD_WriteSoundData(Buffer,temp_fps_scale,MAX(writeSize,writeCount));
//
//		if(XBuf && (skipcount >= maxskip))
//		{
//			skipcount = 0;
//			FCEUD_BlitScreen(XBuf);
//			_updateMemWatch();
//		}
//		else
//			skipcount++;
//
//		_updateWindow();
//	}
//	else
//	{
//		// I don't understand this code, so I kept it around as an option ("old sound update")
//		// in case it's doing something clever and necessary that I can't fathom
//		// (oops, also it seems to be important for speeds <25% so it's always used then)
//
//		const int soundScale = !(soundoptions&SO_OLDUP) ? temp_fps_scale : 256;
//
//		if(Count)
//		{
//			int32 can=GetWriteSound();
//			static int uflow=0;
//			int32 tmpcan;
//			extern int FCEUDnetplay;
//
//			// don't underflow when scaling fps
//			if(can >= GetMaxSound() && fps_scale<=256) uflow=1;	// Go into massive underflow mode. 
//
//			if(can > Count) can=Count;
//			else uflow=0;
//
//			FCEUD_WriteSoundData(Buffer,soundScale,can);
//
//			tmpcan = GetWriteSound();
//			// don't underflow when scaling fps
//			if(fps_scale>256 || ((tmpcan < Count*0.90) && !uflow) || (skipcount >= maxskip))
//			{
//				if(XBuf && (skipcount >= maxskip))
//				{
//					skipcount = 0;
//					FCEUD_BlitScreen(XBuf);
//					_updateMemWatch();
//				}
//				else
//				{
//					skipcount++;
//					//FCEU_printf("Skipped0");
//					//	FCEU_PrintError("Skipped0");
//				}
//				Buffer+=can;
//				Count-=can;
//				if(Count)
//				{
//					if(NoWaiting)
//					{
//						can=GetWriteSound(); 
//						if(Count>can) Count=can;
//						FCEUD_WriteSoundData(Buffer,soundScale,Count);
//					}
//					else
//					{
//						int cnum=0;
//						extern int silencer;
//						while(Count>0)
//						{
//							FCEUD_WriteSoundData(Buffer,soundScale,(Count<ocount) ? Count : ocount);
//							if(!(soundoptions&SO_OLDUP))
//							{
//								cnum++;
//								if(cnum>2)
//									silencer=1;
//							}
//							Count -= ocount;
//							// prevent long updates from interfering with gui responsiveness:
//
//							//mbg merge 7/19/06 
//							//UpdateFCEUWindow();
//							//FCEUD_UpdateInput();
//							_updateWindow();
//						}
//						silencer=0;
//					}
//				}
//			}
//			else
//			{
//				skipcount++;
//				//FCEU_printf("Skipped");
//#ifdef NETWORK
//				if(!NoWaiting && FCEUDnetplay && (uflow || tmpcan >= (Count * 0.90)))
//				{
//					if(Count > tmpcan) Count=tmpcan;
//					while(tmpcan > 0)
//					{
//						//printf("Overwrite: %d\n", (Count <= tmpcan)?Count : tmpcan);
//						FCEUD_WriteSoundData(Buffer,soundScale,(Count <= tmpcan)?Count : tmpcan);
//						tmpcan -= Count;
//					}
//				}
//#endif
//			}
//		}
//		else
//		{
//			/* This complex statement deserves some explanation.
//			Make sure this special speed throttling hasn't been disabled by the user
//			first. Second, we don't want to throttle the speed if the fast-forward
//			button is pressed down(or during certain network play conditions).
//
//			Now, if we're at this point, we'll throttle speed if sound is disabled.
//			Otherwise, it gets a bit more complicated.  We'll throttle speed if focus
//			to FCE Ultra has been lost and we're writing to the primary sound buffer
//			because our sound code won't block.  Blocking does seem to work when
//			writing to a secondary buffer, so we won't throttle when a secondary
//			buffer is used.
//			*/
//
//			//doagain: //mbg merge 6/30/06
//
//			int skipthis = 0;
//
//			if(!(eoptions&EO_NOTHROTTLE) || fps_scale != 256)
//				if(!NoWaiting)
//					if(!soundo || (soundo && nofocus && !(soundoptions&SO_SECONDARY)) || FCEUI_EmulationPaused() )
//						skipthis = SpeedThrottle();
//
//			if(XBuf)
//			{
//				if((!skipthis && !NoWaiting) || (skipcount >= maxskip))
//				{
//					FCEUD_BlitScreen(XBuf);
//					_updateMemWatch();
//					skipcount = 0;
//				}
//				else
//				{
//					skipcount++;
//				}
//			}
//
//			//mbg merge 7/19/06 - since tasbuild we have code in main that attempts to do stuff like this
//		    //mbg merge 6/30/06
//			//if(FCEUI_EmulationPaused())
//			//{
//			//	StopSound();
//			//	Sleep(50);
//			//	BlockingCheck();
//			//	goto doagain;
//			//}
//		}
//
//		//mbg merge 7/19/06 
//		//UpdateFCEUWindow();
//		//FCEUD_UpdateInput();
//		_updateWindow();
//
//	} // end of !(old sound code) block
//}

/**
 * Update the game and gamewindow with a new frame
 **/
void FCEUD_Update(uint8 *XBuf, int32 *Buffer, int Count)
{
	//mbg merge 7/19/06 - leaving this untouched but untested
	//its probably not optimal
	if(FCEU_BotMode()) {
		//this counts the number of frames we've skipped blitting
		// qfox 09/17/06: for bot evaluation purposes, the number
		//                of frames to be skipped is set from the
		//                bot gui.
		if(XBuf && (skipcount++ >= BotFrameSkip()))
		{
			skipcount = 0;
			FCEUD_BlitScreen(XBuf);
			// qfox 17/09/06: it can be wishfull for a coder to evaluate his bot.
			//                slowing it down can help here :) defaults at 0
			if (BotFramePause() > 0) Sleep(BotFramePause());
		}
		UpdateFCEUWindow();
		FCEUD_UpdateInput();
		return;
	}

	win_SoundSetScale(fps_scale);

	//write all the sound we generated.
	if(soundo && Buffer && Count) {
		win_SoundWriteData(Buffer,Count);
	}
	
	//blit the framebuffer
	if(XBuf)
		FCEUD_BlitScreen(XBuf);

	//update debugging displays
	_updateWindow();

	//throttle
	extern bool turbo; //needs to be declared better
	if(!(eoptions&EO_NOTHROTTLE)) //if throttling is enabled..
		if(!turbo) //and turbo is disabled..
			if(!FCEUI_EmulationPaused())
				//then throttle
				win_Throttle();

	//delay until we unpause. we will only stick here if we're paused by a breakpoint or debug command
	while(FCEUI_EmulationPaused() && inDebugger)
	{
		Sleep(50);
		BlockingCheck();
		FCEUD_UpdateInput(); //should this update the CONTROLS??? or only the hotkeys etc?
	}

	//something of a hack, but straightforward:
	//if we were paused, but not in the debugger, then unpause ourselves and step.
	//this is so that the cpu won't cut off execution due to being paused, but the debugger _will_
	//cut off execution as soon as it makes it into the main cpu cycle loop
	if(FCEUI_EmulationPaused() && !inDebugger) {
		FCEUI_ToggleEmulationPause();
		FCEUI_Debugger().step = 1;
	}
	

//	if(soundo) //&& temp_fps_scale >= 64
//	{
//		// sounds better with FPS scaling, and is much less code than the other version...
//
//		int32 writeSize = GetWriteSound();
//		int32 writeCount = Count;
///*
//		// prevents delay when exiting fast-forward
//		if((NoWaiting&1) && writeCount>writeSize)
//			writeCount=writeSize;
//*/
//
//		if(Buffer && (writeCount))
//			FCEUD_WriteSoundData(Buffer,temp_fps_scale,MAX(writeSize,writeCount));
//
//		if(XBuf && (skipcount >= maxskip))
//		{
//			skipcount = 0;
//			FCEUD_BlitScreen(XBuf);
//			_updateMemWatch();
//		}
//		else
//			skipcount++;
//
//		_updateWindow();
//	}
	
//#ifdef NETWORK
//				if(!NoWaiting && FCEUDnetplay && (uflow || tmpcan >= (Count * 0.90)))
//				{
//					if(Count > tmpcan) Count=tmpcan;
//					while(tmpcan > 0)
//					{
//						//printf("Overwrite: %d\n", (Count <= tmpcan)?Count : tmpcan);
//						FCEUD_WriteSoundData(Buffer,soundScale,(Count <= tmpcan)?Count : tmpcan);
//						tmpcan -= Count;
//					}
//				}
//#endif
	//	{
	//		/* This complex statement deserves some explanation.
	//		Make sure this special speed throttling hasn't been disabled by the user
	//		first. Second, we don't want to throttle the speed if the fast-forward
	//		button is pressed down(or during certain network play conditions).

	//		Now, if we're at this point, we'll throttle speed if sound is disabled.
	//		Otherwise, it gets a bit more complicated.  We'll throttle speed if focus
	//		to FCE Ultra has been lost and we're writing to the primary sound buffer
	//		because our sound code won't block.  Blocking does seem to work when
	//		writing to a secondary buffer, so we won't throttle when a secondary
	//		buffer is used.
	//		*/

	//		//doagain: //mbg merge 6/30/06

	//		int skipthis = 0;

	//		if(!(eoptions&EO_NOTHROTTLE) || fps_scale != 256)
	//			if(!NoWaiting)
	//				if(!soundo || (soundo && nofocus && !(soundoptions&SO_SECONDARY)) || FCEUI_EmulationPaused() )
	//					skipthis = SpeedThrottle();

	//		if(XBuf)
	//		{
	//			if((!skipthis && !NoWaiting) || (skipcount >= maxskip))
	//			{
	//				FCEUD_BlitScreen(XBuf);
	//				_updateMemWatch();
	//				skipcount = 0;
	//			}
	//			else
	//			{
	//				skipcount++;
	//			}
	//		}

	//		//mbg merge 7/19/06 - since tasbuild we have code in main that attempts to do stuff like this
	//	    //mbg merge 6/30/06
	//		//if(FCEUI_EmulationPaused())
	//		//{
	//		//	StopSound();
	//		//	Sleep(50);
	//		//	BlockingCheck();
	//		//	goto doagain;
	//		//}
	//	}

	//	//mbg merge 7/19/06 
	//	//UpdateFCEUWindow();
	//	//FCEUD_UpdateInput();
	//	_updateWindow();

	//} // end of !(old sound code) block
}


/*
void FCEUD_Update(uint8 *XBuf, int32 *Buffer, int Count)
{
 FCEUD_BlitScreen(XBuf);
 if(Count && soundo)
  FCEUD_WriteSoundData(Buffer,Count);
 FCEUD_UpdateInput();
}
*/

static void FCEUD_MakePathDirs(const char *fname)
{
	char path[MAX_PATH];
	const char* div = fname;

	do
	{
		const char* fptr = strchr(div, '\\');

		if(!fptr)
		{
			fptr = strchr(div, '/');
		}

		if(!fptr)
		{
			break;
		}

		int off = fptr - fname;
		strncpy(path, fname, off);
		path[off] = '\0';
		mkdir(path);

		div = fptr + 1;
		
		while(div[0] == '\\' || div[0] == '/')
		{
			div++;
		}

	} while(1);
}

FILE *FCEUD_UTF8fopen(const char *n, const char *m)
{
	if(strchr(m, 'w') || strchr(m, '+'))
	{
		FCEUD_MakePathDirs(n);
	}

	return(fopen(n, m));
}

int FCEUD_ShowStatusIcon(void)
{
	return status_icon;
}

void FCEUD_ToggleStatusIcon(void)
{
	status_icon = !status_icon;
	UpdateCheckedMenuItems();
}
