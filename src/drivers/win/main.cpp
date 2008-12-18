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
#include <direct.h>
#include <commctrl.h>
#include <shlobj.h>     // For directories configuration dialog.
#undef uint8

#include <fstream>

#include "../../types.h"
#include "../../fceu.h"
#include "../../state.h"
#include "../../debug.h"
#include "../../movie.h"

#include "archive.h"
#include "input.h"
#include "netplay.h"
#include "memwatch.h"
#include "joystick.h"
#include "keyboard.h"
#include "ppuview.h"
#include "debugger.h"
#include "cheat.h"
#include "debug.h"
#include "ntview.h"
#include "memview.h"
#include "tracer.h"
#include "cdlogger.h"
#include "throttle.h"
#include "tasedit.h"

#include "main.h"
#include "args.h"
#include "config.h"
#include "sound.h"
#include "wave.h"
#include "video.h"
#include "utils/xstring.h"
#include <string.h>

//---------------------------
//mbg merge 6/29/06 - new aboutbox

#if defined(MSVC)
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
#elif defined(__GNUC__)
 #ifdef _DEBUG
  #define _GCC_BUILD "debug"
 #else 
  #define _GCC_BUILD "release"
 #endif
 #define __COMPILER__STRING__ "gcc " __VERSION__ " " _GCC_BUILD
#else
 #define __COMPILER__STRING__ "unknown"
#endif

// External functions
extern std::string cfgFile;		//Contains the filename of the config file used.
extern bool turbo;				//Is game in turbo mode?
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

//global variables
int soundoptions = SO_SECONDARY | SO_GFOCUS;
int soundrate = 44100;
int soundbuftime = 50;
int soundvolume = 100;
int soundquality = 0;
double saspectw = 1, saspecth = 1;
double winsizemulx = 1, winsizemuly = 1;
int genie = 0;
int pal_emulation = 0;
int ntsccol = 0, ntsctint, ntschue;
std::string BaseDirectory;
int PauseAfterLoad;
unsigned int skippy = 0;  //Frame skip
int frameSkipCounter = 0; //Counter for managing frame skip
// Contains the names of the overridden standard directories
// in the order roms, nonvol, states, fdsrom, snaps, cheats, movies, memwatch, macro, input presets, lua scripts, base
char *directory_names[14] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

//Handle of the main window.
HWND hAppWnd = 0;

uint32 goptions = GOO_DISABLESS;

// Some timing-related variables (now ignored).
int maxconbskip = 32;          //Maximum consecutive blit skips.
int ffbskip = 32;              //Blit skips per blit when FF-ing

HINSTANCE fceu_hInstance;
HACCEL fceu_hAccel;

HRESULT ddrval;

static char TempArray[2048];

static int exiting = 0;
static volatile int moocow = 0;

int windowedfailed;
int fullscreen = 0;	//Windows files only, variable that keeps track of fullscreen status

static volatile int _userpause = 0; //mbg merge 7/18/06 changed tasbuild was using this only in a couple of places

extern int autoHoldKey, autoHoldClearKey;
extern int frame_display, input_display;

int soundo = 1;

int srendlinen = 8;
int erendlinen = 231;
int srendlinep = 0;
int erendlinep = 239;

//mbg 6/30/06 - indicates that the main loop should close the game as soon as it can
bool closeGame = false;


// Counts the number of frames that have not been displayed.
// Used for the bot, to skip frames (makes things faster).
int BotFramesSkipped = 0;

// Internal functions
void SetDirs()
{
	int x;

	static int jlist[14]= {
		FCEUIOD_ROMS,
		FCEUIOD_NV,
		FCEUIOD_STATES,
		FCEUIOD_FDSROM,
		FCEUIOD_SNAPS,
		FCEUIOD_CHEATS,
		FCEUIOD_MOVIES,
		FCEUIOD_MEMW,
		FCEUIOD_BBOT,
		FCEUIOD_MACRO,
		FCEUIOD_INPUT,
		FCEUIOD_LUA,
		FCEUIOD_AVI,
		FCEUIOD__COUNT};

	FCEUI_SetSnapName((eoptions & EO_SNAPNAME)!=0);

	for(x=0; x < sizeof(jlist) / sizeof(*jlist); x++)
	{
		FCEUI_SetDirOverride(jlist[x], directory_names[x]);
	}

	if(directory_names[13])
	{
		FCEUI_SetBaseDirectory(directory_names[13]);
	}
	else
	{
		FCEUI_SetBaseDirectory(BaseDirectory);
	}
}

/// Creates a directory.
/// @param dirname Name of the directory to create.
void DirectoryCreator(const char* dirname)
{
	CreateDirectory(dirname, 0);
}

/// Removes a directory.
/// @param dirname Name of the directory to remove.
void DirectoryRemover(const char* dirname)
{
	RemoveDirectory(dirname);
}

/// Used to walk over the default directories array.
/// @param callback Callback function that's called for every default directory name.
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
				directory_names[NUMBER_OF_DEFAULT_DIRECTORIES] ? directory_names[NUMBER_OF_DEFAULT_DIRECTORIES] : BaseDirectory.c_str(),
				default_directory_names[curr_dir]
			);

			callback(TempArray);
		}
	}
}

/// Remove empty, unused directories.
void RemoveDirs()
{
	DefaultDirectoryWalker(DirectoryRemover);
}

///Creates the default directories.
void CreateDirs()
{
	DefaultDirectoryWalker(DirectoryCreator);
}



//Fills the BaseDirectory string
//TODO: Potential buffer overflow caused by limited size of BaseDirectory?
void GetBaseDirectory(void)
{
	char temp[2048];
	GetModuleFileName(0, temp, 2048);
	BaseDirectory = temp;

	size_t truncate_at = BaseDirectory.find_last_of("\\/");
	if(truncate_at != std::string::npos)
		BaseDirectory = BaseDirectory.substr(0,truncate_at);
}

int BlockingCheck()
{
	MSG msg;
	moocow = 1;

	while( PeekMessage( &msg, 0, 0, 0, PM_NOREMOVE ) )
	{
		if( GetMessage( &msg, 0,  0, 0)>0 )
		{
			//other accelerator capable dialogs could be added here
			extern HWND hwndMemWatch;
			extern HWND hwndTasEdit;
			int handled = 0;
			if(hwndMemWatch)
			{
				if(IsChild(hwndMemWatch,msg.hwnd))
					handled = TranslateAccelerator(hwndMemWatch,fceu_hAccel,&msg);
				if(!handled)
					handled = IsDialogMessage(hwndMemWatch,&msg);
			}
			if(!handled && hwndTasEdit)
			{
				if(IsChild(hwndTasEdit,msg.hwnd))
					handled = TranslateAccelerator(hwndTasEdit,fceu_hAccel,&msg);
			}
			/* //adelikat - Currently no accel keys are used in the main window.  Uncomment this block to activate them.
			if(!handled)
				if(msg.hwnd == hAppWnd)
				{
					handled = TranslateAccelerator(hAppWnd,fceu_hAccel,&msg);
					if(handled)
					{
						int zzz=9;
					}
				}
			*/
			if(!handled)
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	moocow = 0;

	return exiting ? 0 : 1;
}

void UpdateRendBounds()
{ 
	FCEUI_SetRenderedLines(srendlinen, erendlinen, srendlinep, erendlinep);
}

/// Shows an error message in a message box.
///@param errormsg Text of the error message.
void FCEUD_PrintError(const char *errormsg)
{
	AddLogText(errormsg, 1);
	
	if(fullscreen)
	{
		ShowCursorAbs(1);
	}

	MessageBox(0, errormsg, FCEU_NAME" Error", MB_ICONERROR | MB_OK | MB_SETFOREGROUND | MB_TOPMOST);
	
	if(fullscreen)
	{
		ShowCursorAbs(0);
	}
}

///Generates a compiler identification string.
/// @return Compiler identification string
const char *FCEUD_GetCompilerString()
{
	return 	__COMPILER__STRING__;
}

//Displays the about box
void ShowAboutBox()
{
	MessageBox(hAppWnd, FCEUI_GetAboutString(), FCEU_NAME, MB_OK);
}

//Exits FCE Ultra
void DoFCEUExit()
{
	if(exiting)    //Eh, oops.  I'll need to try to fix this later.
		return;

	if(goptions & GOO_CONFIRMEXIT)
	{
		//Wolfenstein 3D had cute exit messages.
		const char * const emsg[7]={"Are you sure you want to leave?  I'll become lonely!",
			"A strange game. The only winning move is not to play. How about a nice game of chess?",
			"If you exit, I'll... EAT YOUR MOUSE.",
			"You can never really exit, you know.",
			"E.X.I.T?",
			"I'm sorry, you missed your exit. There is another one in 19 miles",
			"Silly Exit Message goes here"

		};

		if(IDYES != MessageBox(hAppWnd, emsg[rand() & 6], "Exit FCE Ultra?", MB_ICONQUESTION | MB_YESNO) )
		{
			return;
		}
	}

	CloseMemoryWatch();	

	KillDebugger(); //mbg merge 7/19/06 added

	FCEUI_StopMovie();
	FCEUD_AviStop();

	exiting = 1;
	closeGame = true;//mbg 6/30/06 - for housekeeping purposes we need to exit after the emulation cycle finishes
}

void FCEUD_OnCloseGame()
{
}

//Changes the thread priority of the main thread.
void DoPriority()
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
	//sprintf(TempArray, "%s/fceux.cfg", BaseDirectory.c_str());
	sprintf(TempArray, "%s/%s", BaseDirectory.c_str(),cfgFile.c_str());
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

#ifdef _USE_SHARED_MEMORY_
HANDLE mapGameMemBlock;
HANDLE mapRAM;

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
}
#endif

void do_exit()
{
	DriverKill();
	timeEndPeriod(1);
	FCEUI_Kill();
}

//Puts the default directory names into the elements of the directory_names array that aren't already defined.
void initDirectories()
{
	for (unsigned int i = 0; i < NUMBER_OF_DEFAULT_DIRECTORIES; i++)
	{
		if (directory_names[i] == 0)
		{
			sprintf(
				TempArray,
				"%s\\%s",
				directory_names[i] ? directory_names[i] : BaseDirectory.c_str(),
				default_directory_names[i]
			);

			directory_names[i] = (char*)malloc(strlen(TempArray) + 1);
			strcpy(directory_names[i], TempArray);
		}
	}

	if (directory_names[NUMBER_OF_DIRECTORIES - 1] == 0)
	{
		directory_names[NUMBER_OF_DIRECTORIES - 1] = (char*)malloc(BaseDirectory.size() + 1);
		strcpy(directory_names[NUMBER_OF_DIRECTORIES - 1], BaseDirectory.c_str());
	}
}
#include "x6502.h"
int main(int argc,char *argv[])
{
	printf("%08x",opsize);

	char *t;

	initArchiveSystem();

	if(timeBeginPeriod(1) != TIMERR_NOERROR)
	{
		AddLogText("Error setting timer granularity to 1ms.", DO_ADD_NEWLINE);
	}

	InitCommonControls();
	debugSystem = new DebugSystem();

	if(!FCEUI_Initialize())
	{
		do_exit();
		return 1;
	}

	ApplyDefaultCommandMapping();

	fceu_hInstance = GetModuleHandle(0);
	fceu_hAccel = LoadAccelerators(fceu_hInstance,MAKEINTRESOURCE(IDR_ACCELERATOR1));

	// Get the base directory
	GetBaseDirectory();
	
	// Parse the commandline arguments
	t = ParseArgies(argc, argv);
	
	if (ConfigToLoad) cfgFile.assign(ConfigToLoad);

	// Load the config information
	sprintf(TempArray,"%s\\%s",BaseDirectory.c_str(),cfgFile.c_str());
	LoadConfig(TempArray);

	initDirectories();

	//Bleh, need to find a better place for this.
	{
        pal_emulation = !!pal_emulation;
        FCEUI_SetVidSystem(pal_emulation);

        FCEUI_SetGameGenie(genie!=0);

        fullscreen = !!fullscreen;
        soundo = !!soundo;
        frame_display = !!frame_display;
        allowUDLR = !!allowUDLR;
        pauseAfterPlayback = !!pauseAfterPlayback;
        EnableBackgroundInput = !!EnableBackgroundInput;

        FCEUI_SetSoundVolume(soundvolume);
		FCEUI_SetSoundQuality(soundquality);
	}
	
	//Since a game doesn't have to be loaded before the GUI can be used, make
	//sure the temporary input type variables are set.
	ParseGIInput(NULL);

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
 
	InitSpeedThrottle();

	if(t)
	{
		ALoad(t);
	}
	else if(eoptions & EO_FOAFTERSTART)
	{
		LoadNewGamey(hAppWnd, 0);
	}


	if(GameInfo && MovieToLoad)
	{
		//switch to readonly mode if the file is an archive
		if(FCEU_isFileInArchive(MovieToLoad))
				replayReadOnlySetting = true;

		FCEUI_LoadMovie(MovieToLoad, replayReadOnlySetting, false, replayStopFrameSetting!=0);
		free(MovieToLoad);
		MovieToLoad = NULL;
	}
	if(GameInfo && StateToLoad)
	{
		FCEUI_LoadState(StateToLoad);
		free(StateToLoad);
		StateToLoad = NULL;
	}
	if (MemWatchLoadOnStart) CreateMemWatch();
	if (PauseAfterLoad) FCEUI_ToggleEmulationPause();
	SetAutoFirePattern(AFon, AFoff);
	UpdateCheckedMenuItems();
doloopy:
	UpdateFCEUWindow();  
	if(GameInfo)
	{
		while(GameInfo)
		{
	        uint8 *gfx=0; ///contains framebuffer
			int32 *sound=0; ///contains sound data buffer
			int32 ssize=0; ///contains sound samples count
			
			if (turbo) 
			{
				if (!frameSkipCounter) 
				{
					frameSkipCounter = 15;
					skippy = 0;
				}
				else 
				{
					frameSkipCounter--;
					skippy = 1;
				}
				
			}
			else skippy = 0;

			FCEUI_Emulate(&gfx, &sound, &ssize, skippy); //emulate a single frame
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

	delete debugSystem;

	return(0);
}

		

//mbg merge 7/19/06 - the function that contains the code that used to just be UpdateFCEUWindow() and FCEUD_UpdateInput()
void _updateWindow()
{
	UpdateFCEUWindow();
	PPUViewDoBlit();
	UpdateMemoryView(0);
	UpdateCDLogger();
	UpdateLogWindow();
	UpdateMemWatch();
	NTViewDoBlit(0);
	UpdateTasEdit();
}

void win_debuggerLoop()
{
	//delay until something causes us to unpause. 
	//either a hotkey or a debugger command
	while(FCEUI_EmulationPaused() && !FCEUI_EmulationFrameStepped())
	{
		Sleep(50);
		FCEUD_UpdateInput();
		_updateWindow();
	}
	int zzz=9;
}

// Update the game and gamewindow with a new frame
void FCEUD_Update(uint8 *XBuf, int32 *Buffer, int Count)
{
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

	extern bool JustFrameAdvanced;

	//MBG TODO - think about this logic
	//throttle
	extern bool turbo; //needs to be declared better
	if(!(eoptions&EO_NOTHROTTLE)) //if throttling is enabled..
		if(!turbo) //and turbo is disabled..
			if(!FCEUI_EmulationPaused() 
				||JustFrameAdvanced
				)
				//then throttle
				while(SpeedThrottle()) {
					FCEUD_UpdateInput();
					_updateWindow();
				}


	//sleep just to be polite
	if(!JustFrameAdvanced && FCEUI_EmulationPaused()) {
		Sleep(50);
	}

	//while(EmulationPaused==1 && inDebugger)
	//{
	//	Sleep(50);
	//	BlockingCheck();
	//	FCEUD_UpdateInput(); //should this update the CONTROLS??? or only the hotkeys etc?
	//}

	////so, we're not paused anymore.

	////something of a hack, but straightforward:
	////if we were paused, but not in the debugger, then unpause ourselves and step.
	////this is so that the cpu won't cut off execution due to being paused, but the debugger _will_
	////cut off execution as soon as it makes it into the main cpu cycle loop
	//if(FCEUI_EmulationPaused() && !inDebugger) {
	//	FCEUI_ToggleEmulationPause();
	//	FCEUI_Debugger().step = 1;
	//	FCEUD_DebugBreakpoint();
	//}
	
	//make sure to update the input once per frame
	FCEUD_UpdateInput();


}

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

std::fstream* FCEUD_UTF8_fstream(const char *n, const char *m)
{
	if(strchr(m, 'w') || strchr(m, '+'))
	{
		FCEUD_MakePathDirs(n);
	}

	std::ios_base::openmode mode = std::ios_base::binary;
	if(!strcmp(m,"r") || !strcmp(m,"rb"))
		mode |= std::ios_base::in;
	else if(!strcmp(m,"w") || !strcmp(m,"wb"))
		mode |= std::ios_base::out | std::ios_base::trunc;
	else if(!strcmp(m,"a") || !strcmp(m,"ab"))
		mode |= std::ios_base::out | std::ios_base::app;
	else if(!strcmp(m,"r+") || !strcmp(m,"r+b"))
		mode |= std::ios_base::in | std::ios_base::out;
	else if(!strcmp(m,"w+") || !strcmp(m,"w+b"))
		mode |= std::ios_base::in | std::ios_base::out | std::ios_base::trunc;
	else if(!strcmp(m,"a+") || !strcmp(m,"a+b"))
		mode |= std::ios_base::in | std::ios_base::out | std::ios_base::app;
	
	std::fstream *fs = new std::fstream(n,mode);
	if(!fs->is_open()) {
		delete fs;
		return 0;
	} else return fs;
}

FILE *FCEUD_UTF8fopen(const char *n, const char *m)
{
	if(strchr(m, 'w') || strchr(m, '+'))
	{
		FCEUD_MakePathDirs(n);
	}

	return(fopen(n, m));
}

int status_icon = 1;

int FCEUD_ShowStatusIcon(void)
{
	return status_icon;
}

void FCEUD_ToggleStatusIcon(void)
{
	status_icon = !status_icon;
	UpdateCheckedMenuItems();
}

char *GetRomName()
{
	//The purpose of this function is to format the ROM name stored in LoadedRomFName
	//And return a char array with just the name with path or extension
	//The purpose of this function is to populate a save as dialog with the ROM name as a default filename
	extern char LoadedRomFName[2048];	//Contains full path of ROM
	std::string Rom;					//Will contain the formatted path
	if(GameInfo)						//If ROM is loaded
		{
		char drv[PATH_MAX], dir[PATH_MAX], name[PATH_MAX], ext[PATH_MAX];
		splitpath(LoadedRomFName,drv,dir,name,ext);	//Extract components of the ROM path
		Rom = name;						//Pull out the Name only
		}
	else
		Rom = "";
	char*mystring = (char*)malloc(2048*sizeof(char));
	strcpy(mystring, Rom.c_str());		//Convert string to char*

	return mystring;
}
