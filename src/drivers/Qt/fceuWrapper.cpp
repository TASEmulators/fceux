// fceuWrapper.cpp
//
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <unzip.h>

#include <QStyleFactory>
#include "Qt/main.h"
#include "Qt/throttle.h"
#include "Qt/config.h"
#include "Qt/dface.h"
#include "Qt/fceuWrapper.h"
#include "Qt/input.h"
#include "Qt/sdl.h"
#include "Qt/sdl-video.h"
#include "Qt/nes_shm.h"
#include "Qt/unix-netplay.h"
#include "Qt/HexEditor.h"
#include "Qt/SymbolicDebug.h"
#include "Qt/CodeDataLogger.h"
#include "Qt/ConsoleDebugger.h"
#include "Qt/ConsoleWindow.h"
#include "Qt/ConsoleUtilities.h"
#include "Qt/fceux_git_info.h"

#include "common/cheat.h"
#include "../../fceu.h"
#include "../../movie.h"
#include "../../version.h"

#ifdef _S9XLUA_H
#include "../../fceulua.h"
#endif

#include "common/configSys.h"
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
bool pauseAfterPlayback = false;
bool suggestReadOnlyReplay = true;
unsigned int gui_draw_area_width   = 256;
unsigned int gui_draw_area_height  = 256;

// global configuration object
Config *g_config = NULL;

static int inited = 0;
static int noconfig=0;
static int frameskip=0;
static int periodic_saves = 0;
static int   mutexLocks = 0;
static int   mutexPending = 0;
static bool  emulatorHasMutux = 0;
static unsigned int emulatorCycleCount = 0;

extern double g_fpsScale;

#ifdef CREATE_AVI
int mutecapture = 0;
#endif
//*****************************************************************
// Define Global Functions to be shared with FCEU Core
//*****************************************************************
//

// Message functions defined in MsgLogViewer.cpp
//void FCEUD_Message(const char *text)
//void FCEUD_PrintError(const char *errormsg)

/**
 * Opens a file, C++ style, to be read a byte at a time.
 */
FILE *FCEUD_UTF8fopen(const char *fn, const char *mode)
{
   FILE *fp = ::fopen(fn,mode);
	return(fp);
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

	KillJoysticks();

	if(inited&4)
		KillVideo();
	if(inited&1)
		KillSound();
	inited=0;
}

/**
 * Reloads last game
 */
int reloadLastGame(void)
{
	std::string lastRom;
	g_config->getOption(std::string("SDL.LastOpenFile"), &lastRom);
	return LoadGame(lastRom.c_str(), false);
}

/**
 * Loads a game, given a full path/filename.  The driver code must be
 * initialized after the game is loaded, because the emulator code
 * provides data necessary for the driver code(number of scanlines to
 * render, what virtual input devices to use, etc.).
 */
int LoadGame(const char *path, bool silent)
{
	char fullpath[4096];
	int gg_enabled, autoLoadDebug, autoOpenDebugger, autoInputPreset;

	if (isloaded){
		CloseGame();
	}

#if defined(__linux) || defined(__APPLE__)

	// Resolve absolute path to file
	if ( realpath( path, fullpath ) == NULL )
	{
		strcpy( fullpath, path );
	}
#else
	strcpy( fullpath, path );
#endif

	//printf("Fullpath: %zi '%s'\n", sizeof(fullpath), fullpath );

	// For some reason, the core of the emulator clears the state of 
	// the game genie option selection. So check the config each time
	// and re-enable the core game genie state if needed.
	g_config->getOption ("SDL.GameGenie", &gg_enabled);

	FCEUI_SetGameGenie (gg_enabled);

	if(!FCEUI_LoadGame(fullpath, 1, silent)) {
		return 0;
	}

	hexEditorLoadBookmarks();

	g_config->getOption( "SDL.AutoLoadDebugFiles", &autoLoadDebug );

	if ( autoLoadDebug )
	{
		loadGameDebugBreakpoints();
	}

	g_config->getOption( "SDL.AutoOpenDebugger", &autoOpenDebugger );

	if ( autoOpenDebugger && !debuggerWindowIsOpen() )
	{
		consoleWindow->openDebugWindow();
	}

	debugSymbolTable.loadGameSymbols();

	CDLoggerROMChanged();

	int state_to_load;
	g_config->getOption("SDL.AutoLoadState", &state_to_load);
	if (state_to_load >= 0 && state_to_load < 10){
	    FCEUI_SelectState(state_to_load, 0);
	    FCEUI_LoadState(NULL, false);
	}

	g_config->getOption( "SDL.AutoInputPreset", &autoInputPreset );

	if ( autoInputPreset )
	{
		loadInputSettingsFromFile();
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
	hexEditorSaveBookmarks();
	saveGameDebugBreakpoints();
	debuggerClearAllBreakpoints();

	debugSymbolTable.save();
	debugSymbolTable.clear();
	CDLoggerROMClosed();

   int state_to_save;
   g_config->getOption("SDL.AutoSaveState", &state_to_save);
   if (state_to_save < 10 && state_to_save >= 0){
       FCEUI_SelectState(state_to_save, 0);
       FCEUI_SaveState(NULL, false);
   }

	int autoInputPreset;
	g_config->getOption( "SDL.AutoInputPreset", &autoInputPreset );

	if ( autoInputPreset )
	{
		saveInputSettingsToFile();
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

int  fceuWrapperSoftReset(void)
{
	if ( isloaded )
	{
		ResetNES();
	}
	return 0;
}

int  fceuWrapperHardReset(void)
{
	if ( isloaded )
	{
		std::string lastFile;
		CloseGame();
		g_config->getOption ("SDL.LastOpenFile", &lastFile);
		LoadGame (lastFile.c_str());
	}
	return 0;
}

int  fceuWrapperTogglePause(void)
{
	if ( isloaded )
	{
		FCEUI_ToggleEmulationPause();
	}
	return 0;
}

bool fceuWrapperGameLoaded(void)
{
	return (isloaded ? true : false);
}

void fceuWrapperRequestAppExit(void)
{
	if ( consoleWindow )
	{
		consoleWindow->requestClose();
	}
}

static const char *DriverUsage =
"Option         Value   Description\n"
"--pal          {0|1}   Use PAL timing.\n"
"--newppu       {0|1}   Enable the new PPU core. (WARNING: May break savestates)\n"
"--input(1,2)   d       Set which input device to emulate for input 1 or 2.\n"
"                         Devices:  gamepad zapper powerpad.0 powerpad.1\n"
"                         arkanoid\n"
"--input(3,4)   d       Set the famicom expansion device to emulate for\n"
"                       input(3, 4)\n"
"                          Devices: quizking hypershot mahjong toprider ftrainer\n"
"                          familykeyboard oekakids arkanoid shadow bworld\n"
"                          4player\n"
"--gamegenie    {0|1}   Enable emulated Game Genie.\n"
"--frameskip    x       Set # of frames to skip per emulated frame.\n"
"--xres         x       Set horizontal resolution for full screen mode.\n"
"--yres         x       Set vertical resolution for full screen mode.\n"
"--autoscale    {0|1}   Enable autoscaling in fullscreen. \n"
"--keepratio    {0|1}   Keep native NES aspect ratio when autoscaling. \n"
"--(x/y)scale   x       Multiply width/height by x. \n"
"                         (Real numbers >0 with OpenGL, otherwise integers >0).\n"
"--(x/y)stretch {0|1}   Stretch to fill surface on x/y axis (OpenGL only).\n"
"--fullscreen   {0|1}   Enable full screen mode.\n"
"--noframe      {0|1}   Hide title bar and window decorations.\n"
"--special      {1-4}   Use special video scaling filters\n"
"                         (1 = hq2x; 2 = Scale2x; 3 = NTSC 2x; 4 = hq3x;\n"
"                         5 = Scale3x; 6 = Prescale2x; 7 = Prescale3x; 8=Precale4x; 9=PAL)\n"
"--palette      f       Load custom global palette from file f.\n"
"--sound        {0|1}   Enable sound.\n"
"--soundrate    x       Set sound playback rate to x Hz.\n"
"--soundq      {0|1|2}  Set sound quality. (0 = Low 1 = High 2 = Very High)\n"
"--soundbufsize x       Set sound buffer size to x ms.\n"
"--volume      {0-256}  Set volume to x.\n"
"--soundrecord  f       Record sound to file f.\n"
"--playmov      f       Play back a recorded FCM/FM2/FM3 movie from filename f.\n"
"--pauseframe   x       Pause movie playback at frame x.\n"
"--fcmconvert   f       Convert fcm movie file f to fm2.\n"
"--ripsubs      f       Convert movie's subtitles to srt\n"
"--subtitles    {0|1}   Enable subtitle display\n"
"--fourscore    {0|1}   Enable fourscore emulation\n"
"--no-config    {0|1}   Use default config file and do not save\n"
"--net          s       Connect to server 's' for TCP/IP network play.\n"
"--port         x       Use TCP/IP port x for network play.\n"
"--user         x       Set the nickname to use in network play.\n"
"--pass         x       Set password to use for connecting to the server.\n"
"--netkey       s       Use string 's' to create a unique session for the\n"
"                       game loaded.\n"
"--players      x       Set the number of local players in a network play\n"
"                       session.\n"
"--rp2mic       {0|1}   Replace Port 2 Start with microphone (Famicom).\n"
"--4buttonexit {0|1}    exit the emulator when A+B+Select+Start is pressed\n"
"--loadstate {0-9|>9}   load from the given state when the game is loaded\n"
"--savestate {0-9|>9}   save to the given state when the game is closed\n"
"                         to not save/load automatically provide a number\n"
"                         greater than 9\n"
"--periodicsaves {0|1}  enable automatic periodic saving.  This will save to\n"
"                         the state passed to --savestate\n";

static void ShowUsage(const char *prog)
{
	int i,j;
	printf("\nUsage is as follows:\n%s <options> filename\n\n",prog);
	puts(DriverUsage);
#ifdef _S9XLUA_H
	puts ("--loadlua      f       Loads lua script from filename f.");
#endif
#ifdef CREATE_AVI
	puts ("--videolog     c       Calls mencoder to grab the video and audio streams to\n                         encode them. Check the documentation for more on this.");
	puts ("--mute        {0|1}    Mutes FCEUX while still passing the audio stream to\n                         mencoder during avi creation.");
#endif
	puts ("--style=KEY            Use Qt GUI Style based on supplied key. Available system style keys are:\n");

	QStringList styleList = QStyleFactory::keys();

	j=0;
	for (i=0; i<styleList.size(); i++)
	{
		printf("  %16s  ", styleList[i].toStdString().c_str() ); j++;

		if ( j >= 4 )
		{
			printf("\n"); j=0;
		}
	}
	printf("\n\n");
	printf(" Custom Qt stylesheets (.qss files) may be used by setting an\n");
	printf(" environment variable named FCEUX_QT_STYLESHEET equal to the \n");
	printf(" full (absolute) path to the qss file.\n");

	puts("");
	printf("Compiled with SDL version %d.%d.%d\n", SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL );
	SDL_version v; 
	SDL_GetVersion(&v);
	printf("Linked with SDL version %d.%d.%d\n", v.major, v.minor, v.patch);
  	printf("Compiled with QT version %d.%d.%d\n", QT_VERSION_MAJOR, QT_VERSION_MINOR, QT_VERSION_PATCH );
	printf("git URL: %s\n", fceu_get_git_url() );
	printf("git Rev: %s\n", fceu_get_git_rev() );
	
}

int  fceuWrapperInit( int argc, char *argv[] )
{
	int error;
	std::string s;

	for (int i=0; i<argc; i++)
	{
		if ( (strcmp(argv[i], "--help") == 0) || (strcmp(argv[i],"-h") == 0) )
		{
			ShowUsage(argv[0]);
			exit(0);
		}
	}

	FCEUD_Message("Starting " FCEU_NAME_AND_VERSION "...\n");

	/* SDL_INIT_VIDEO Needed for (joystick config) event processing? */
	if (SDL_Init(SDL_INIT_VIDEO)) 
	{
		printf("Could not initialize SDL: %s.\n", SDL_GetError());
		exit(-1);
	}
	if ( SDL_SetHint( SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1" ) == SDL_FALSE )
	{
		printf("Error setting SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS\n");
	}

	// Initialize the configuration system
	g_config = InitConfig();

	if ( !g_config )
	{
		printf("Error: Could not initialize configuration system\n");
		exit(-1);
	}

	// initialize the infrastructure
	error = FCEUI_Initialize();

	if (error != 1) 
	{
		printf("Error: Initializing FCEUI\n");
		ShowUsage(argv[0]);
		//SDL_Quit();
		exit(-1);
	}

	int romIndex = g_config->parse(argc, argv);

	// This is here so that a default fceux.cfg will be created on first
	// run, even without a valid ROM to play.
	// Unless, of course, there's actually --no-config given
	// mbg 8/23/2008 - this is also here so that the inputcfg routines can have 
    // a chance to dump the new inputcfg to the fceux.cfg  in case you didnt 
    // specify a rom  filename
	g_config->getOption("SDL.NoConfig", &noconfig);

	if (!noconfig)
	{
		g_config->save();
	}

	// update the input devices
	UpdateInput(g_config);

	// check for a .fcm file to convert to .fm2
	g_config->getOption ("SDL.FCMConvert", &s);
	g_config->setOption ("SDL.FCMConvert", "");
	if (!s.empty())
	{
		int okcount = 0;
		std::string infname = s.c_str();
		// produce output filename
		std::string outname;
		size_t dot = infname.find_last_of (".");
		if (dot == std::string::npos)
			outname = infname + ".fm2";
		else
			outname = infname.substr(0,dot) + ".fm2";
	  
		MovieData md;
		EFCM_CONVERTRESULT result = convert_fcm (md, infname);

		if (result == FCM_CONVERTRESULT_SUCCESS) {
			okcount++;
        // *outf = new EMUFILE;
		EMUFILE_FILE* outf = FCEUD_UTF8_fstream (outname, "wb");
		md.dump (outf,false);
		delete outf;
		FCEUD_Message ("Your file has been converted to FM2.\n");
	}
	else {
		FCEUD_Message ("Something went wrong while converting your file...\n");
	}
	  
		DriverKill();
	  SDL_Quit();
	  return 0;
	}
	// If x/y res set to 0, store current display res in SDL.LastX/YRes
	int yres, xres;
	g_config->getOption("SDL.XResolution", &xres);
	g_config->getOption("SDL.YResolution", &yres);
	
	int autoResume;
	g_config->getOption("SDL.AutoResume", &autoResume);
	if(autoResume)
	{
		AutoResumePlay = true;
	}
	else
	{
		AutoResumePlay = false;
	}

	// check to see if recording HUD to AVI is enabled
	int rh;
	g_config->getOption("SDL.RecordHUD", &rh);
	if( rh == 0)
		FCEUI_SetAviEnableHUDrecording(true);
	else
		FCEUI_SetAviEnableHUDrecording(false);

	// check to see if movie messages are disabled
	int mm;
	g_config->getOption("SDL.MovieMsg", &mm);
	if( mm == 0)
		FCEUI_SetAviDisableMovieMessages(true);
	else
		FCEUI_SetAviDisableMovieMessages(false);
  
	// check for a .fm2 file to rip the subtitles
	g_config->getOption("SDL.RipSubs", &s);
	g_config->setOption("SDL.RipSubs", "");
	if (!s.empty())
	{
		MovieData md;
		std::string infname;
		infname = s.c_str();
		FCEUFILE *fp = FCEU_fopen(s.c_str(), 0, "rb", 0);
		
		// load the movie and and subtitles
		extern bool LoadFM2(MovieData&, EMUFILE*, int, bool);
		LoadFM2(md, fp->stream, INT_MAX, false);
		LoadSubtitles(md); // fill subtitleFrames and subtitleMessages
		delete fp;
		
		// produce .srt file's name and open it for writing
		std::string outname;
		size_t dot = infname.find_last_of (".");
		if (dot == std::string::npos)
			outname = infname + ".srt";
		else
			outname = infname.substr(0,dot) + ".srt";
		FILE *srtfile;
		srtfile = fopen(outname.c_str(), "w");
		
		if (srtfile != NULL)
		{
			extern std::vector<int> subtitleFrames;
			extern std::vector<std::string> subtitleMessages;
			float fps = (md.palFlag == 0 ? 60.0988 : 50.0069); // NTSC vs PAL
			float subduration = 3; // seconds for the subtitles to be displayed
			for (int i = 0; i < subtitleFrames.size(); i++)
			{
				fprintf(srtfile, "%i\n", i+1); // starts with 1, not 0
				double seconds, ms, endseconds, endms;
				seconds = subtitleFrames[i]/fps;
				if (i+1 < subtitleFrames.size()) // there's another subtitle coming after this one
				{
					if (subtitleFrames[i+1]-subtitleFrames[i] < subduration*fps) // avoid two subtitles at the same time
					{
						endseconds = (subtitleFrames[i+1]-1)/fps; // frame x: subtitle1; frame x+1 subtitle2
					} else {
						endseconds = seconds+subduration;
							}
				} else {
					endseconds = seconds+subduration;
				}
				ms = modf(seconds, &seconds);
				endms = modf(endseconds, &endseconds);
				// this is just beyond ugly, don't show it to your kids
				fprintf(srtfile,
				"%02.0f:%02d:%02d,%03d --> %02.0f:%02d:%02d,%03d\n", // hh:mm:ss,ms --> hh:mm:ss,ms
				floor(seconds/3600),	(int)floor(seconds/60   ) % 60, (int)floor(seconds)	% 60, (int)(ms*1000),
				floor(endseconds/3600), (int)floor(endseconds/60) % 60, (int)floor(endseconds) % 60, (int)(endms*1000));
				fprintf(srtfile, "%s\n\n", subtitleMessages[i].c_str()); // new line for every subtitle
			}
		fclose(srtfile);
		printf("%d subtitles have been ripped.\n", (int)subtitleFrames.size());
		} else {
		FCEUD_Message("Couldn't create output srt file...\n");
		}
	  
		DriverKill();
		SDL_Quit();
		return 0;
	}

	nes_shm = open_nes_shm();

	if ( nes_shm == NULL )
	{
		printf("Error: Failed to open NES Shared memory\n");
		return -1;
	}

	// update the emu core
	UpdateEMUCore(g_config);

	#ifdef CREATE_AVI
	g_config->getOption("SDL.VideoLog", &s);
	g_config->setOption("SDL.VideoLog", "");
	if(!s.empty())
	{
		NESVideoSetVideoCmd(s.c_str());
		LoggingEnabled = 1;
		g_config->getOption("SDL.MuteCapture", &mutecapture);
	} else {
		mutecapture = 0;
	}
	#endif

	{
		int id;
		g_config->getOption("SDL.InputDisplay", &id);
		extern int input_display;
		input_display = id;
		// not exactly an id as an true/false switch; still better than creating another int for that
		g_config->getOption("SDL.SubtitleDisplay", &id); 
		movieSubtitles = id ? true : false;
	}

	// Emulation Timing Mechanism
	{
		int timingMode;

		g_config->getOption("SDL.EmuTimingMech", &timingMode);

		setTimingMode( timingMode );
	}
	
	// load the hotkeys from the config life
	setHotKeys();

	if (romIndex >= 0)
	{
		// load the specified game
		error = LoadGame(argv[romIndex]);
		if (error != 1) 
		{
			DriverKill();
			SDL_Quit();
			return -1;
		}
		g_config->setOption("SDL.LastOpenFile", argv[romIndex]);
		g_config->save();
	}

	// movie playback
	g_config->getOption("SDL.Movie", &s);
	g_config->setOption("SDL.Movie", "");
	if (s != "")
	{
		if(s.find(".fm2") != std::string::npos || s.find(".fm3") != std::string::npos)
		{
			static int pauseframe;
			char replayReadOnlySetting;
			g_config->getOption("SDL.PauseFrame", &pauseframe);
			g_config->setOption("SDL.PauseFrame", 0);

			if (suggestReadOnlyReplay)
			{
				replayReadOnlySetting = true;
			}
			else
			{
				replayReadOnlySetting = FCEUI_GetMovieToggleReadOnly();
			}
			FCEUI_printf("Playing back movie located at %s\n", s.c_str());
			FCEUI_LoadMovie(s.c_str(), replayReadOnlySetting, pauseframe ? pauseframe : false);
		}
		else
		{
		  FCEUI_printf("Sorry, I don't know how to play back %s\n", s.c_str());
		}
		g_config->getOption("SDL.MovieLength",&KillFCEUXonFrame);
		printf("KillFCEUXonFrame %d\n",KillFCEUXonFrame);
	}
	
    int save_state;
    g_config->getOption("SDL.PeriodicSaves", &periodic_saves);
    g_config->getOption("SDL.AutoSaveState", &save_state);
    if(periodic_saves && save_state < 10 && save_state >= 0){
        FCEUI_SelectState(save_state, 0);
    } else {
        periodic_saves = 0;
    }
	
#ifdef _S9XLUA_H
	// load lua script if option passed
	g_config->getOption("SDL.LuaScript", &s);
	g_config->setOption("SDL.LuaScript", "");
	if (s != "")
	{
#if defined(__linux) || defined(__APPLE__)

		// Resolve absolute path to file
		char fullpath[2048];
		if ( realpath( s.c_str(), fullpath ) != NULL )
		{
			//printf("Fullpath: '%s'\n", fullpath );
			s.assign( fullpath );
		}
#endif
		FCEU_LoadLuaCode(s.c_str());
	}
#endif
	
	{
		int id;
		g_config->getOption("SDL.NewPPU", &id);
		if (id)
			newppu = 1;
	}

	g_config->getOption("SDL.Frameskip", &frameskip);

	return 0;
}

int  fceuWrapperClose( void )
{
	CloseGame();

	// exit the infrastructure
	FCEUI_Kill();
	SDL_Quit();

	return 0;
}

/**
 * Update the video, audio, and input subsystems with the provided
 * video (XBuf) and audio (Buffer) information.
 */
void
FCEUD_Update(uint8 *XBuf,
			 int32 *Buffer,
			 int Count)
{
	int blitDone = 0;
	extern int FCEUDnetplay;

	#ifdef CREATE_AVI
	if (LoggingEnabled == 2 || (eoptions&EO_NOTHROTTLE))
	{
		if(LoggingEnabled == 2)
		{
			int16* MonoBuf = new int16[Count];
			int n;
			for(n=0; n<Count; ++n)
			{
				MonoBuf[n] = Buffer[n] & 0xFFFF;
			}
			NESVideoLoggingAudio
			(
			  MonoBuf, 
			  FSettings.SndRate, 16, 1,
			  Count
			);
			delete [] MonoBuf;
		}
		Count /= 2;
		if (inited & 1)
		{
			if (Count > GetWriteSound()) Count = GetWriteSound();

			if (!mutecapture)
			{
				if(Count > 0 && Buffer) WriteSound(Buffer,Count);   
			}
		}
		//if (inited & 2)
		//	FCEUD_UpdateInput();
	  	if(XBuf && (inited & 4)) BlitScreen(XBuf);
	  
		return;
	}
	#endif
	
	int ocount = Count;
	// apply frame scaling to Count
	Count = (int)(Count / g_fpsScale);
	if (Count) 
	{
		int32 can=GetWriteSound();
		static int uflow=0;
		int32 tmpcan;

		// don't underflow when scaling fps
		if(can >= GetMaxSound() && g_fpsScale==1.0) uflow=1;	/* Go into massive underflow mode. */

		if(can > Count) can=Count;
		else uflow=0;

		#ifdef CREATE_AVI
		if (!mutecapture)
		#endif
		  WriteSound(Buffer,can);

		//if(uflow) puts("Underflow");
		tmpcan = GetWriteSound();
		// don't underflow when scaling fps
		if (g_fpsScale>1.0 || ((tmpcan < Count*0.90) && !uflow)) 
		{
			if (XBuf && (inited&4) && !(NoWaiting & 2))
			{
				BlitScreen(XBuf); blitDone = 1;
			}
			Buffer+=can;
			Count-=can;
			if(Count) 
			{
				if(NoWaiting) 
				{
					can=GetWriteSound();
					if(Count>can) Count=can;
					#ifdef CREATE_AVI
					if (!mutecapture)
					#endif
					  WriteSound(Buffer,Count);
				}
			  	else
			  	{
					while(Count>0) 
					{
						#ifdef CREATE_AVI
						if (!mutecapture)
						#endif
						  WriteSound(Buffer,(Count<ocount) ? Count : ocount);
						Count -= ocount;
					}
				}
			}
		} //else puts("Skipped");
		else if (!NoWaiting && FCEUDnetplay && (uflow || tmpcan >= (Count * 1.8))) 
		{
			if (Count > tmpcan) Count=tmpcan;
			while(tmpcan > 0) 
			{
				//	printf("Overwrite: %d\n", (Count <= tmpcan)?Count : tmpcan);
				#ifdef CREATE_AVI
				if (!mutecapture)
				#endif
				  WriteSound(Buffer, (Count <= tmpcan)?Count : tmpcan);
				tmpcan -= Count;
			}
		}
	}
  	else 
	{
		if (XBuf && (inited&4)) 
		{
			BlitScreen(XBuf); blitDone = 1;
		}
	}
	if ( !blitDone )
	{
		if (XBuf && (inited&4)) 
		{
			BlitScreen(XBuf); blitDone = 1;
		}
	}
	//FCEUD_UpdateInput();
}

static void DoFun(int frameskip, int periodic_saves)
{
	uint8 *gfx;
	int32 *sound;
	int32 ssize;
	static int fskipc = 0;
	//static int opause = 0;

    //TODO peroidic saves, working on it right now
    if (periodic_saves && FCEUD_GetTime() % PERIODIC_SAVE_INTERVAL < 30){
        FCEUI_SaveState(NULL, false);
    }
#ifdef FRAMESKIP
	fskipc = (fskipc + 1) % (frameskip + 1);
#endif

	if (NoWaiting) 
	{
		gfx = 0;
	}
	FCEUI_Emulate(&gfx, &sound, &ssize, fskipc);
	FCEUD_Update(gfx, sound, ssize);

	//if(opause!=FCEUI_EmulationPaused()) 
	//{
	//	opause=FCEUI_EmulationPaused();
	//	SilenceSound(opause);
	//}
	
	emulatorCycleCount++;
}

void fceuWrapperLock(void)
{
	mutexPending++;
	if ( consoleWindow != NULL )
	{
		consoleWindow->mutex->lock();
	}
	mutexPending--;
	mutexLocks++;
}

bool fceuWrapperTryLock(int timeout)
{
	bool lockAcq = false;

	mutexPending++;
	if ( consoleWindow != NULL )
	{
		lockAcq = consoleWindow->mutex->tryLock( timeout );
	}
	mutexPending--;

	if ( lockAcq )
	{
		mutexLocks++;
	}
	return lockAcq;
}

void fceuWrapperUnLock(void)
{
	if ( mutexLocks > 0 )
	{
		if ( consoleWindow != NULL )
		{
			consoleWindow->mutex->unlock();
		}
		mutexLocks--;
	}
	else
	{
		printf("Error: Mutex is Already UnLocked\n");
	}
}

bool fceuWrapperIsLocked(void)
{
	return mutexLocks > 0;
}

int  fceuWrapperUpdate( void )
{
	bool lock_acq;

	// If a request is pending, 
	// sleep to allow request to be serviced.
	if ( mutexPending > 0 )
	{
		usleep( 100000 );
	}

	lock_acq = fceuWrapperTryLock();

	if ( !lock_acq )
	{
		if ( GameInfo )
		{
			printf("Error: Emulator Failed to Acquire Mutex\n");
		}
		usleep( 100000 );

		return -1;
	}
	emulatorHasMutux = 1;
 
	if ( GameInfo )
	{
		DoFun(frameskip, periodic_saves);
	
		hexEditorUpdateMemoryValues();

		fceuWrapperUnLock();

		emulatorHasMutux = 0;

		while ( SpeedThrottle() )
		{
			// Input device processing is in main thread
			// because to MAC OS X SDL2 requires it.
			//FCEUD_UpdateInput(); 
		}
	}
	else
	{
		fceuWrapperUnLock();

		emulatorHasMutux = 0;

		usleep( 100000 );
	}
	return 0;
}

ArchiveScanRecord FCEUD_ScanArchive(std::string fname)
{
	int idx=0, ret;
	unzFile zf;
	unz_file_info fi;
	char filename[512];
	ArchiveScanRecord rec;
		
	zf = unzOpen( fname.c_str() );

	if ( zf == NULL )
	{
		//printf("Error: Failed to open Zip File: '%s'\n", fname.c_str() );
		return rec;
	}
	rec.type = 0;

	ret = unzGoToFirstFile( zf );

	//printf("unzGoToFirstFile: %i \n", ret );

	while ( ret == 0 )
	{
		FCEUARCHIVEFILEINFO_ITEM item;

		unzGetCurrentFileInfo( zf, &fi, filename, sizeof(filename), NULL, 0, NULL, 0 );

		//printf("Filename: %u '%s' \n", fi.uncompressed_size, filename );

		item.name.assign( filename );
		item.size  = fi.uncompressed_size;
		item.index = idx; idx++;

		rec.files.push_back( item );

		ret = unzGoToNextFile( zf );

		//printf("unzGoToNextFile: %i \n", ret );
	}
	rec.numFilesInArchive = idx;

	unzClose( zf );

	return rec;
}

FCEUFILE* FCEUD_OpenArchive(ArchiveScanRecord& asr, std::string& fname, std::string* innerFilename, int* userCancel)
{
	int ret;
	FCEUFILE* fp = 0;
	unzFile zf;
	unz_file_info fi;
	char filename[512];
	char foundFile = 0;
	void *tmpMem = NULL;
	std::string searchFile;

	if ( innerFilename != NULL )
	{
		searchFile = *innerFilename;
	}
	else
	{
		std::vector <std::string> fileList;

		for (size_t i=0; i<asr.files.size(); i++)
		{
			char base[512], suffix[32];

			getFileBaseName( asr.files[i].name.c_str(), base, suffix );

			if ( (strcasecmp( suffix, ".nes" ) == 0) ||
			     (strcasecmp( suffix, ".nsf" ) == 0) ||
			     (strcasecmp( suffix, ".fds" ) == 0) ||
			     (strcasecmp( suffix, ".unf" ) == 0) ||
			     (strcasecmp( suffix, ".unif") == 0) )
			{
				fileList.push_back( asr.files[i].name );
			}
		}

		if ( fileList.size() > 1 )
		{
			if ( consoleWindow != NULL )
			{
				int sel = consoleWindow->showListSelectDialog( "Select ROM From Archive", fileList );

				if ( sel < 0 )
				{
					if ( userCancel )
					{
						*userCancel = 1;
					}
					return fp;
				}
				searchFile = fileList[sel];
			}
		}
		else if ( fileList.size() > 0 )
		{
			searchFile = fileList[0];
		}
	}

	zf = unzOpen( fname.c_str() );

	if ( zf == NULL )
	{
		//printf("Error: Failed to open Zip File: '%s'\n", fname.c_str() );
		return fp;
	}

	//printf("Searching for %s in %s \n", searchFile.c_str(), fname.c_str() );

	ret = unzGoToFirstFile( zf );

	//printf("unzGoToFirstFile: %i \n", ret );

	while ( ret == 0 )
	{
		unzGetCurrentFileInfo( zf, &fi, filename, sizeof(filename), NULL, 0, NULL, 0 );

		//printf("Filename: %u '%s' \n", fi.uncompressed_size, filename );

		if ( strcmp( searchFile.c_str(), filename ) == 0 )
		{
		   //printf("Found Filename: %u '%s' \n", fi.uncompressed_size, filename );
			foundFile = 1; break;
		}

		ret = unzGoToNextFile( zf );

		//printf("unzGoToNextFile: %i \n", ret );
	}

	if ( !foundFile )
	{
		unzClose( zf );
		return fp;
	}

	tmpMem = ::malloc( fi.uncompressed_size );

	if ( tmpMem == NULL )
	{
		unzClose( zf );
		return fp;
	}

	EMUFILE_MEMORY* ms = new EMUFILE_MEMORY(fi.uncompressed_size);

	unzOpenCurrentFile( zf );
	unzReadCurrentFile( zf, tmpMem, fi.uncompressed_size );
	unzCloseCurrentFile( zf );

	ms->fwrite( tmpMem, fi.uncompressed_size );

	free( tmpMem );

	//if we extracted the file correctly
	fp = new FCEUFILE();
	fp->archiveFilename = fname;
	fp->filename = searchFile;
	fp->fullFilename = fp->archiveFilename + "|" + fp->filename;
	fp->archiveIndex = ret;
	fp->mode = FCEUFILE::READ;
	fp->size = fi.uncompressed_size;
	fp->stream = ms;
	fp->archiveCount = (int)asr.numFilesInArchive;
	ms->fseek(0,SEEK_SET); //rewind so that the rom analyzer sees a freshly opened file

	unzClose( zf );

	return fp;
}

FCEUFILE* FCEUD_OpenArchive(ArchiveScanRecord& asr, std::string& fname, std::string* innerFilename)
{
	int userCancel = 0;

	return FCEUD_OpenArchive( asr, fname, innerFilename, &userCancel );
}

FCEUFILE* FCEUD_OpenArchiveIndex(ArchiveScanRecord& asr, std::string &fname, int innerIndex, int* userCancel)
{
	int ret, idx=0;
	FCEUFILE* fp = 0;
	unzFile zf;
	unz_file_info fi;
	char filename[512];
	char foundFile = 0;
	void *tmpMem = NULL;

	zf = unzOpen( fname.c_str() );

	if ( zf == NULL )
	{
		//printf("Error: Failed to open Zip File: '%s'\n", fname.c_str() );
		return fp;
	}

	ret = unzGoToFirstFile( zf );

	//printf("unzGoToFirstFile: %i \n", ret );

	while ( ret == 0 )
	{
		unzGetCurrentFileInfo( zf, &fi, filename, sizeof(filename), NULL, 0, NULL, 0 );

		//printf("Filename: %u '%s' \n", fi.uncompressed_size, filename );

		if ( idx == innerIndex )
		{
		   //printf("Found Filename: %u '%s' \n", fi.uncompressed_size, filename );
			foundFile = 1; break;
		}
		idx++;

		ret = unzGoToNextFile( zf );

		//printf("unzGoToNextFile: %i \n", ret );
	}

	if ( !foundFile )
	{
		unzClose( zf );
		return fp;
	}

	tmpMem = ::malloc( fi.uncompressed_size );

	if ( tmpMem == NULL )
	{
		unzClose( zf );
		return fp;
	}

	EMUFILE_MEMORY* ms = new EMUFILE_MEMORY(fi.uncompressed_size);

	unzOpenCurrentFile( zf );
	unzReadCurrentFile( zf, tmpMem, fi.uncompressed_size );
	unzCloseCurrentFile( zf );

	ms->fwrite( tmpMem, fi.uncompressed_size );

	free( tmpMem );

	//if we extracted the file correctly
	fp = new FCEUFILE();
	fp->archiveFilename = fname;
	fp->filename = filename;
	fp->fullFilename = fp->archiveFilename + "|" + fp->filename;
	fp->archiveIndex = ret;
	fp->mode = FCEUFILE::READ;
	fp->size = fi.uncompressed_size;
	fp->stream = ms;
	fp->archiveCount = (int)asr.numFilesInArchive;
	ms->fseek(0,SEEK_SET); //rewind so that the rom analyzer sees a freshly opened file

	unzClose( zf );

	return fp;
}

FCEUFILE* FCEUD_OpenArchiveIndex(ArchiveScanRecord& asr, std::string &fname, int innerIndex)
{
	int userCancel = 0;

	return FCEUD_OpenArchiveIndex( asr, fname, innerIndex, &userCancel );
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
bool FCEUD_PauseAfterPlayback() { return pauseAfterPlayback; }

void FCEUD_TurboOn	 (void) { /* TODO */ };
void FCEUD_TurboOff   (void) { /* TODO */ };
void FCEUD_TurboToggle(void) { /* TODO */ };

