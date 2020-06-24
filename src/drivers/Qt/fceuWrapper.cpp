// fceuWrapper.cpp
//
#include <stdio.h>
#include <stdlib.h>

#include "main.h"
#include "throttle.h"
#include "config.h"
#include "dface.h"
#include "fceuWrapper.h"
#include "input.h"
#include "sdl-video.h"
#include "unix-netplay.h"

#include "../common/cheat.h"
#include "../../fceu.h"
#include "../../movie.h"
#include "../../version.h"

#ifdef _S9XLUA_H
#include "../../fceulua.h"
#endif

#include "../common/configSys.h"
#include "../../oldmovie.h"
#include "../../types.h"

#ifdef CREATE_AVI
#include "../videolog/nesvideos-piece.h"
#endif

//*****************************************************************
// Define Global Variables to be shared with FCEU Core
//*****************************************************************
int  dendy = 0;
int eoptions=0;
int isloaded=0;
int pal_emulation=0;
int gametype = 0;
int closeFinishedMovie = 0;
int KillFCEUXonFrame = 0;

bool swapDuty = 0;
bool turbo = false;
unsigned int gui_draw_area_width   = 256;
unsigned int gui_draw_area_height  = 256;

// global configuration object
Config *g_config = NULL;

static int inited = 0;
static int noconfig=0;

//*****************************************************************
// Define Global Functions to be shared with FCEU Core
//*****************************************************************
//

/**
* Prints a textual message without adding a newline at the end.
*
* @param text The text of the message.
*
* TODO: This function should have a better name.
**/
void FCEUD_Message(const char *text)
{
	fputs(text, stdout);
}

/**
* Shows an error message in a message box.
* (For now: prints to stderr.)
* 
* If running in GTK mode, display a dialog message box of the error.
*
* @param errormsg Text of the error message.
**/
void FCEUD_PrintError(const char *errormsg)
{
	fprintf(stderr, "%s\n", errormsg);
}

/**
 * Opens a file, C++ style, to be read a byte at a time.
 */
FILE *FCEUD_UTF8fopen(const char *fn, const char *mode)
{
	return(fopen(fn,mode));
}

/**
 * Opens a file to be read a byte at a time.
 */
EMUFILE_FILE* FCEUD_UTF8_fstream(const char *fn, const char *m)
{
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
    return new EMUFILE_FILE(fn, m);
	//return new std::fstream(fn,mode);
}

static const char *s_linuxCompilerString = "g++ " __VERSION__;
/**
 * Returns the compiler string.
 */
const char *FCEUD_GetCompilerString(void)
{
	return s_linuxCompilerString;
}

/**
 * Get the time in ticks.
 */
uint64
FCEUD_GetTime(void)
{
	return SDL_GetTicks();
}

/**
 * Get the tick frequency in Hz.
 */
uint64
FCEUD_GetTimeFreq(void)
{
	// SDL_GetTicks() is in milliseconds
	return 1000;
}

void FCEUD_DebugBreakpoint( int addr )
{
   // TODO
}

/**
 * Initialize all of the subsystem drivers: video, audio, and joystick.
 */
static int
DriverInitialize(FCEUGI *gi)
{
	if(InitVideo(gi) < 0) return 0;
	inited|=4;

	if(InitSound())
		inited|=1;

	if(InitJoysticks())
		inited|=2;

	int fourscore=0;
	g_config->getOption("SDL.FourScore", &fourscore);
	eoptions &= ~EO_FOURSCORE;
	if(fourscore)
		eoptions |= EO_FOURSCORE;

	InitInputInterface();
	return 1;
}

/**
 * Shut down all of the subsystem drivers: video, audio, and joystick.
 */
static void
DriverKill()
{
	if (!noconfig)
		g_config->save();

	if(inited&2)
		KillJoysticks();
	if(inited&4)
		KillVideo();
	if(inited&1)
		KillSound();
	inited=0;
}

/**
 * Loads a game, given a full path/filename.  The driver code must be
 * initialized after the game is loaded, because the emulator code
 * provides data necessary for the driver code(number of scanlines to
 * render, what virtual input devices to use, etc.).
 */
int LoadGame(const char *path)
{
	if (isloaded){
		CloseGame();
	}
	if(!FCEUI_LoadGame(path, 1)) {
		return 0;
	}

    int state_to_load;
    g_config->getOption("SDL.AutoLoadState", &state_to_load);
    if (state_to_load >= 0 && state_to_load < 10){
        FCEUI_SelectState(state_to_load, 0);
        FCEUI_LoadState(NULL, false);
    }

	ParseGIInput(GameInfo);
	RefreshThrottleFPS();

	if(!DriverInitialize(GameInfo)) {
		return(0);
	}
	
	// set pal/ntsc
	int id;
	g_config->getOption("SDL.PAL", &id);
	FCEUI_SetRegion(id);

	g_config->getOption("SDL.SwapDuty", &id);
	swapDuty = id;
	
	std::string filename;
	g_config->getOption("SDL.Sound.RecordFile", &filename);
	if(filename.size()) {
		if(!FCEUI_BeginWaveRecord(filename.c_str())) {
			g_config->setOption("SDL.Sound.RecordFile", "");
		}
	}
	isloaded = 1;

	FCEUD_NetworkConnect();
	return 1;
}

/**
 * Closes a game.  Frees memory, and deinitializes the drivers.
 */
int
CloseGame(void)
{
	std::string filename;

	if(!isloaded) {
		return(0);
	}

    int state_to_save;
    g_config->getOption("SDL.AutoSaveState", &state_to_save);
    if (state_to_save < 10 && state_to_save >= 0){
        FCEUI_SelectState(state_to_save, 0);
        FCEUI_SaveState(NULL, false);
    }
	FCEUI_CloseGame();

	DriverKill();
	isloaded = 0;
	GameInfo = 0;

	g_config->getOption("SDL.Sound.RecordFile", &filename);
	if(filename.size()) {
		FCEUI_EndWaveRecord();
	}

	InputUserActiveFix();
	return(1);
}

// dummy functions

#define DUMMY(__f) \
    void __f(void) {\
        printf("%s\n", #__f);\
        FCEU_DispMessage("Not implemented.",0);\
    }
DUMMY(FCEUD_HideMenuToggle)
DUMMY(FCEUD_MovieReplayFrom)
DUMMY(FCEUD_ToggleStatusIcon)
DUMMY(FCEUD_AviRecordTo)
DUMMY(FCEUD_AviStop)
void FCEUI_AviVideoUpdate(const unsigned char* buffer) { }
int FCEUD_ShowStatusIcon(void) {return 0;}
bool FCEUI_AviIsRecording(void) {return false;}
void FCEUI_UseInputPreset(int preset) { }
bool FCEUD_PauseAfterPlayback() { return false; }

void FCEUD_TurboOn	 (void) { /* TODO */ };
void FCEUD_TurboOff   (void) { /* TODO */ };
void FCEUD_TurboToggle(void) { /* TODO */ };

FCEUFILE* FCEUD_OpenArchiveIndex(ArchiveScanRecord& asr, std::string &fname, int innerIndex) { return 0; }
FCEUFILE* FCEUD_OpenArchive(ArchiveScanRecord& asr, std::string& fname, std::string* innerFilename) { return 0; }
FCEUFILE* FCEUD_OpenArchiveIndex(ArchiveScanRecord& asr, std::string &fname, int innerIndex, int* userCancel) { return 0; }
FCEUFILE* FCEUD_OpenArchive(ArchiveScanRecord& asr, std::string& fname, std::string* innerFilename, int* userCancel) { return 0; }
ArchiveScanRecord FCEUD_ScanArchive(std::string fname) { return ArchiveScanRecord(); }

