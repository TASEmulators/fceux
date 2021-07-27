/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020 mjbudd77
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
#include <QDir>

#include "Qt/main.h"
#include "Qt/throttle.h"
#include "Qt/config.h"

#include "fceu.h"
#include "ppu.h"
#include "../common/cheat.h"

#include "Qt/input.h"
#include "Qt/dface.h"

#include "Qt/sdl.h"
#include "Qt/sdl-video.h"
#include "Qt/unix-netplay.h"

#ifdef WIN32
#include <windows.h>
#endif

//#include <unistd.h>

#include <csignal>
#include <cstring>
#include <cerrno>
#include <cstdio>
#include <cstdlib>

#ifdef WIN32
#include <direct.h>
#else
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif

extern bool force_grayscale;
extern bool palupdate;
extern  int palnotch;
extern  int palsaturation;
extern  int palsharpness;
extern  int palcontrast;
extern  int palbrightness;

int getHotKeyConfig( int i, const char **nameOut, const char **keySeqOut, const char **titleOut )
{
	const char *name = "";
	const char *keySeq = "";
	const char *title = NULL;

	switch ( i )
	{
		case HK_OPEN_ROM: 
			name = "OpenROM"; keySeq = "Ctrl+O"; title = "Open ROM";
		break;
		case HK_CLOSE_ROM: 
			name = "CloseROM"; keySeq = "Ctrl+C"; title = "Close ROM";
		break;
		case HK_CHEAT_MENU:
			name = "CheatMenu"; keySeq = ""; title = "Open Cheat Window";
		break;
		case HK_BIND_STATE:
			name = "BindState"; keySeq = ""; title = "Bind Save State to Movie";
		break;
		case HK_LOAD_LUA:
			name = "LoadLua"; keySeq = "Ctrl+L";	
		break;
		case HK_TOGGLE_BG:
			name = "ToggleBG"; keySeq = "";	title = "Toggle Background Display";
		break;
		case HK_TOGGLE_FG:
			name = "ToggleFG"; keySeq = "";	title = "Toggle Object Display";
		break;
		// Save States
		case HK_SAVE_STATE:
			name = "SaveState"; keySeq = "I"; title = "Save State";
		break;
		case HK_SAVE_STATE_0:
			name = "SaveState0"; keySeq = "Shift+F10"; title = "Save State to Slot 0";
		break;
		case HK_SAVE_STATE_1:
			name = "SaveState1"; keySeq = "Shift+F1"; title = "Save State to Slot 1";
		break;
		case HK_SAVE_STATE_2:
			name = "SaveState2"; keySeq = "Shift+F2"; title = "Save State to Slot 2";
		break;
		case HK_SAVE_STATE_3:
			name = "SaveState3"; keySeq = "Shift+F3"; title = "Save State to Slot 3";
		break;
		case HK_SAVE_STATE_4:
			name = "SaveState4"; keySeq = "Shift+F4"; title = "Save State to Slot 4";
		break;
		case HK_SAVE_STATE_5:
			name = "SaveState5"; keySeq = "Shift+F5"; title = "Save State to Slot 5";
		break;
		case HK_SAVE_STATE_6:
			name = "SaveState6"; keySeq = "Shift+F6"; title = "Save State to Slot 6";
		break;
		case HK_SAVE_STATE_7:
			name = "SaveState7"; keySeq = "Shift+F7"; title = "Save State to Slot 7";
		break;
		case HK_SAVE_STATE_8:
			name = "SaveState8"; keySeq = "Shift+F8"; title = "Save State to Slot 8";
		break;
		case HK_SAVE_STATE_9:
			name = "SaveState9"; keySeq = "Shift+F9"; title = "Save State to Slot 9";
		break;
		// Load States
		case HK_LOAD_STATE:
			name = "LoadState"; keySeq = "P";	title = "Load State";
		break;
		case HK_LOAD_STATE_0:
			name = "LoadState0"; keySeq = "F10"; title = "Load State From Slot 0";
		break;
		case HK_LOAD_STATE_1:
			name = "LoadState1"; keySeq = "F1"; title = "Load State From Slot 1";
		break;
		case HK_LOAD_STATE_2:
			name = "LoadState2"; keySeq = "F2"; title = "Load State From Slot 2";
		break;
		case HK_LOAD_STATE_3:
			name = "LoadState3"; keySeq = "F3"; title = "Load State From Slot 3";
		break;
		case HK_LOAD_STATE_4:
			name = "LoadState4"; keySeq = "F4"; title = "Load State From Slot 4";
		break;
		case HK_LOAD_STATE_5:
			name = "LoadState5"; keySeq = "F5"; title = "Load State From Slot 5";
		break;
		case HK_LOAD_STATE_6:
			name = "LoadState6"; keySeq = "F6"; title = "Load State From Slot 6";
		break;
		case HK_LOAD_STATE_7:
			name = "LoadState7"; keySeq = "F7"; title = "Load State From Slot 7";
		break;
		case HK_LOAD_STATE_8:
			name = "LoadState8"; keySeq = "F8"; title = "Load State From Slot 8";
		break;
		case HK_LOAD_STATE_9:
			name = "LoadState9"; keySeq = "F9"; title = "Load State From Slot 9";
		break;
		case HK_FDS_SELECT:
			name = "FDSSelect"; keySeq = ""; title = "Switch FDS Disk Side";
		break;
		case HK_FDS_EJECT:
			name = "FDSEject"; keySeq = "";	title = "Eject FDS Disk";
		break;
		case HK_VS_INSERT_COIN:
			name = "VSInsertCoin"; keySeq = ""; title = "VS Insert Coin";
		break;
		case HK_VS_TOGGLE_DIPSWITCH:
			name = "VSToggleDip"; keySeq = ""; title = "VS Toggle Dipswitch";
		break;
		case HK_TOGGLE_FRAME_DISPLAY:
			name = "MovieToggleFrameDisplay"; keySeq = ".";	title = "Toggle Frame Display";
		break;
		case HK_TOGGLE_SUBTITLE:
			name = "SubtitleDisplay"; keySeq = ""; title = "Toggle Movie Subtitles";
		break;
		case HK_POWER:
			name = "Power"; keySeq = ""; title = "Power";
		break;
		case HK_RESET:
			name = "Reset"; keySeq = "Ctrl+R"; title = "Reset";
		break;
		case HK_PAUSE:
			name = "Pause"; keySeq = "Pause"; title = "Pause";
		break;
		case HK_QUIT:
			name = "Quit"; keySeq = "Ctrl+Q"; title = "Exit Application";
		break;
		case HK_SCREENSHOT:
			name = "Screenshot"; keySeq = "F12";
		break;
		case HK_DECREASE_SPEED:
			name = "DecreaseSpeed"; keySeq = "-";
		break;
		case HK_INCREASE_SPEED:
			name = "IncreaseSpeed"; keySeq = "=";
		break;
		case HK_FRAME_ADVANCE:
			name = "FrameAdvance"; keySeq = "\\";
		break;
		case HK_TURBO:
			name = "Turbo"; keySeq = "Tab";
		break;
		case HK_TOGGLE_INPUT_DISPLAY:
			name = "ToggleInputDisplay"; keySeq = ",";
		break;
		case HK_MOVIE_TOGGLE_RW:
			name = "ToggleMovieRW"; keySeq = "Q";
		break;
		case HK_PLAY_MOVIE_FROM:
			name = "PlayMovieFrom"; keySeq = "";
		break;
		case HK_MOVIE_PLAY_RESTART:
			name = "PlayMovieFromBeginning"; keySeq = "";
		break;
		case HK_RECORD_MOVIE_TO:
			name = "RecordMovieTo"; keySeq = "";
		break;
		case HK_STOP_MOVIE:
			name = "StopMovie"; keySeq = "";
		break;
		case HK_RECORD_AVI:
			name = "RecordAvi"; keySeq = "";
		break;
		case HK_RECORD_AVI_TO:
			name = "RecordAviTo"; keySeq = "";
		break;
		case HK_STOP_AVI:
			name = "StopAvi"; keySeq = "";
		break;
		case HK_RECORD_WAV:
			name = "RecordWav"; keySeq = "";
		break;
		case HK_RECORD_WAV_TO:
			name = "RecordWavTo"; keySeq = "";
		break;
		case HK_STOP_WAV:
			name = "StopWav"; keySeq = "";
		break;
		case HK_MUTE_CAPTURE:
			name = "MuteCapture"; keySeq = "'";
		break;
		case HK_FA_LAG_SKIP:
			name = "FrameAdvanceLagSkip"; keySeq = "Delete";
		break;
		case HK_LAG_COUNTER_DISPLAY:
			name = "LagCounterDisplay"; keySeq = "/";
		break;
		case HK_SELECT_STATE_0:
			name = "SelectState0"; keySeq = "0"; title = "Select State Slot 0";
		break;
		case HK_SELECT_STATE_1:
			name = "SelectState1"; keySeq = "1"; title = "Select State Slot 1";
		break;
		case HK_SELECT_STATE_2:
			name = "SelectState2"; keySeq = "2"; title = "Select State Slot 2";
		break;
		case HK_SELECT_STATE_3:
			name = "SelectState3"; keySeq = "3"; title = "Select State Slot 3";
		break;
		case HK_SELECT_STATE_4:
			name = "SelectState4"; keySeq = "4"; title = "Select State Slot 4";
		break;
		case HK_SELECT_STATE_5:
			name = "SelectState5"; keySeq = "5"; title = "Select State Slot 5";
		break;
		case HK_SELECT_STATE_6:
			name = "SelectState6"; keySeq = "6"; title = "Select State Slot 6";
		break;
		case HK_SELECT_STATE_7:
			name = "SelectState7"; keySeq = "7"; title = "Select State Slot 7";
		break;
		case HK_SELECT_STATE_8:
			name = "SelectState8"; keySeq = "8"; title = "Select State Slot 8";
		break;
		case HK_SELECT_STATE_9:
			name = "SelectState9"; keySeq = "9"; title = "Select State Slot 9";
		break;
		case HK_SELECT_STATE_NEXT:
			name = "SelectStateNext"; keySeq = ""; title = "Select Next State Slot";
		break;
		case HK_SELECT_STATE_PREV:
			name = "SelectStatePrev"; keySeq = ""; title = "Select Previous State Slot";
		break;
		case HK_VOLUME_DOWN:
			name = "VolumeDown"; keySeq = "";
		break;
		case HK_VOLUME_UP:
			name = "VolumeUp"; keySeq = "";
		break;
		case HK_FKB_ENABLE:
			name = "FKB_Enable"; keySeq = "ScrollLock"; title = "Toggle Family Keyboard Enable";
		break;
		case HK_FULLSCREEN:
			name = "FullScreen"; keySeq = "Alt+Return"; title = "Toggle Fullscreen View";
		break;
		case HK_MAIN_MENU_HIDE:
			name = "MainMenuHide"; keySeq = "Alt+/"; title = "Toggle Main Menu Visibility";
		break;
		default:
		case HK_MAX:
			name = ""; keySeq = "";
		break;

	}

	if ( nameOut )
	{
		*nameOut = name;
	}
	if ( keySeqOut )
	{
		*keySeqOut = keySeq;
	}
	if ( titleOut )
	{
		if ( title == NULL )
		{
			title = name;
		}
		*titleOut = title;
	}
	return 0;
}

/**
 * Read a custom pallete from a file and load it into the core.
 */
int
LoadCPalette(const std::string &file)
{
	uint8 tmpp[192];
	FILE *fp;

	if(!(fp = FCEUD_UTF8fopen(file.c_str(), "rb"))) {
		char errorMsg[256];
		strcpy(errorMsg, "Error loading custom palette from file: ");
		strcat(errorMsg, file.c_str());
		FCEUD_PrintError(errorMsg);
		return 0;
	}
	size_t result = fread(tmpp, 1, 192, fp);
	if(result != 192) {
		char errorMsg[256];
		strcpy(errorMsg, "Error loading custom palette from file: ");
		strcat(errorMsg, file.c_str());
		FCEUD_PrintError(errorMsg);
		return 0;
	}
	FCEUI_SetUserPalette(tmpp, result/3);
	fclose(fp);
	return 1;
}

/**
 * Creates the subdirectories used for saving snapshots, movies, game
 * saves, etc.  Hopefully obsolete with new configuration system.
 */
static void
CreateDirs(const std::string &dir)
{
	const char *subs[]={"fcs","snaps","gameinfo","sav","cheats","avi","wav","movies","input", NULL };
	std::string subdir;
	int x=0;

#if defined(WIN32) || defined(NEED_MINGW_HACKS)
	mkdir(dir.c_str());
	chmod(dir.c_str(), 755);
	while ( subs[x] != NULL )
	{
		subdir = dir + PSS + subs[x];
		mkdir(subdir.c_str());
		x++;
	}
#else
	mkdir(dir.c_str(), S_IRWXU);
	while ( subs[x] != NULL )
	{
		subdir = dir + PSS + subs[x];
		mkdir(subdir.c_str(), S_IRWXU);
		x++;
	}
#endif
}

/**
 * Attempts to locate FCEU's application directory.  This will
 * hopefully become obsolete once the new configuration system is in
 * place.
 */
static void
GetBaseDirectory(std::string &dir)
{
	char *home = getenv("FCEUX_HOME");

#ifdef WIN32
	// Windows users want base directory to be where executable resides.
	// Only way to override this behavior is to set an FCEUX_HOME 
	// environment variable prior to starting the application.
	//if ( home == NULL )
	//{
	//	home = getenv("USERPROFILE");
	//}
	//if ( home == NULL )
	//{
	//	home = getenv("HOMEPATH");
	//}
#else
	if ( home == NULL )
	{
		home = getenv("HOME");
	}
#endif

	if (home) 
	{
		dir = std::string(home) + "/.fceux";
	} else {
#ifdef WIN32
		home = new char[MAX_PATH + 1];
		GetModuleFileNameA(NULL, home, MAX_PATH + 1);

		char *lastBS = strrchr(home,'\\');
		if(lastBS) {
			*lastBS = 0;
		}

		dir = std::string(home);
		delete[] home;
#else
		dir = "";
#endif
	}
}

// returns a config structure with default options
// also creates config base directory (ie: /home/user/.fceux as well as subdirs
Config *
InitConfig()
{
	std::string dir, prefix, savPath, movPath;
	Config *config;

	GetBaseDirectory(dir);

	FCEUI_SetBaseDirectory(dir.c_str());
	CreateDirs(dir);

	config = new Config(dir);

	// sound options
	config->addOption('s', "sound", "SDL.Sound", 1);
	config->addOption("volume", "SDL.Sound.Volume", 150);
	config->addOption("trianglevol", "SDL.Sound.TriangleVolume", 255);
	config->addOption("square1vol", "SDL.Sound.Square1Volume", 255);
	config->addOption("square2vol", "SDL.Sound.Square2Volume", 255);
	config->addOption("noisevol", "SDL.Sound.NoiseVolume", 255);
	config->addOption("pcmvol", "SDL.Sound.PCMVolume", 255);
	config->addOption("soundrate", "SDL.Sound.Rate", 44100);
	config->addOption("soundq", "SDL.Sound.Quality", 1);
	config->addOption("soundrecord", "SDL.Sound.RecordFile", "");
	config->addOption("soundbufsize", "SDL.Sound.BufSize", 128);
	config->addOption("lowpass", "SDL.Sound.LowPass", 0);
    
	config->addOption('g', "gamegenie", "SDL.GameGenie", 0);
	config->addOption("pal", "SDL.PAL", 0);
	config->addOption("autoPal", "SDL.AutoDetectPAL", 1);
	config->addOption("frameskip", "SDL.Frameskip", 0);
	config->addOption("clipsides", "SDL.ClipSides", 0);
	config->addOption("nospritelim", "SDL.DisableSpriteLimit", 1);
	config->addOption("swapduty", "SDL.SwapDuty", 0);
	config->addOption("ramInit", "SDL.RamInitMethod", 0);

	// color control
	config->addOption('p', "palette", "SDL.Palette", "");
	config->addOption("tint", "SDL.Tint", 56);
	config->addOption("hue", "SDL.Hue", 72);
	config->addOption("ntsccolor", "SDL.NTSCpalette", 0);
	config->addOption("SDL.ForceGrayScale", 0);
	config->addOption("SDL.DeempBitSwap", 0);
	config->addOption("SDL.PalNotch", 100);
	config->addOption("SDL.PalSaturation", 100);
	config->addOption("SDL.PalSharpness", 0);
	config->addOption("SDL.PalContrast", 100);
	config->addOption("SDL.PalBrightness", 50);

	// scanline settings
	config->addOption("SDL.ScanLineStartNTSC", 0+8);
	config->addOption("SDL.ScanLineEndNTSC", 239-8);
	config->addOption("SDL.ScanLineStartPAL", 0);
	config->addOption("SDL.ScanLineEndPAL", 239);

	// video controls
	config->addOption('f', "fullscreen", "SDL.Fullscreen", 0);
	config->addOption("videoDriver", "SDL.VideoDriver", 0);

	// set x/y res to 0 for automatic fullscreen resolution detection (no change)
	config->addOption('x', "xres", "SDL.XResolution", 0);
	config->addOption('y', "yres", "SDL.YResolution", 0);
	config->addOption("SDL.LastXRes", 0);
	config->addOption("SDL.LastYRes", 0);
	config->addOption("SDL.WinSizeX", 0);
	config->addOption("SDL.WinSizeY", 0);
	config->addOption("doublebuf", "SDL.DoubleBuffering", 1);
	config->addOption("autoscale", "SDL.AutoScale", 1);
	config->addOption("forceAspect", "SDL.ForceAspect", 1);
	config->addOption("aspectSelect", "SDL.AspectSelect", 0);
	config->addOption("aspectX", "SDL.AspectX", 1.000);
	config->addOption("aspectY", "SDL.AspectY", 1.000);
	config->addOption("xscale", "SDL.XScale", 2.000);
	config->addOption("yscale", "SDL.YScale", 2.000);
	config->addOption("xstretch", "SDL.XStretch", 0);
	config->addOption("ystretch", "SDL.YStretch", 0);
	config->addOption("noframe", "SDL.NoFrame", 0);
	config->addOption("special", "SDL.SpecialFilter", 0);
	config->addOption("showfps", "SDL.ShowFPS", 0);
	config->addOption("togglemenu", "SDL.ToggleMenu", 0);
	config->addOption("cursorType", "SDL.CursorType", 0);
	config->addOption("cursorVis" , "SDL.CursorVis", 1);
	config->addOption("SDL.DrawInputAids", 1);

	// OpenGL options
	config->addOption("opengl", "SDL.OpenGL", 1);
	config->addOption("openglip", "SDL.OpenGLip", 0);
	config->addOption("SDL.SpecialFilter", 0);
	config->addOption("SDL.SpecialFX", 0);
	config->addOption("SDL.Vsync", 1);

	// network play options - netplay is broken
	config->addOption("server", "SDL.NetworkIsServer", 0);
	config->addOption('n', "net", "SDL.NetworkIP", "");
	config->addOption('u', "user", "SDL.NetworkUsername", "");
	config->addOption('w', "pass", "SDL.NetworkPassword", "");
	config->addOption('k', "netkey", "SDL.NetworkGameKey", "");
	config->addOption("port", "SDL.NetworkPort", 4046);
	config->addOption("players", "SDL.NetworkPlayers", 1);
     
	// input configuration options
	config->addOption("input1", "SDL.Input.0", "GamePad.0");
	config->addOption("input2", "SDL.Input.1", "GamePad.1");
	config->addOption("input3", "SDL.Input.2", "Gamepad.2");
	config->addOption("input4", "SDL.Input.3", "Gamepad.3");

	config->addOption("autoInputPreset", "SDL.AutoInputPreset", 0);
	config->addOption("SDL.AutofireOnFrames" , 1);
	config->addOption("SDL.AutofireOffFrames", 1);
	config->addOption("SDL.AutofireCustomOnFrames" , 1);
	config->addOption("SDL.AutofireCustomOffFrames", 1);

	// display input
	config->addOption("inputdisplay", "SDL.InputDisplay", 0);

	// enable / disable opposite directionals (left + right or up + down simultaneously)
	config->addOption("opposite-directionals", "SDL.Input.EnableOppositeDirectionals", 1);
    
	// pause movie playback at frame x
	config->addOption("pauseframe", "SDL.PauseFrame", 0);
	config->addOption("recordhud", "SDL.RecordHUD", 1);
	config->addOption("moviemsg", "SDL.MovieMsg", 1);
	config->addOption("SDL.AviVideoFormat", 0);

	// Hex Editor Options
	config->addOption("hexEditBgColor", "SDL.HexEditBgColor", "#000000");
	config->addOption("hexEditFgColor", "SDL.HexEditFgColor", "#FFFFFF");
	config->addOption("SDL.HexEditCursorColorRC", "#000080");
	config->addOption("SDL.HexEditAltColColor"  , "#545454");
	config->addOption("SDL.HexEditFont"  , "");
	config->addOption("SDL.HexEditActivityHlgt", true);
	config->addOption("SDL.HexEditReverseVideo", true);
	config->addOption("SDL.HexEditRowColumnHlgt", false);
	config->addOption("SDL.HexEditAltnColumnColor", false);
	config->addOption("SDL.HexEditRefreshRate", 10);
    
	// Debugger Options
	config->addOption("autoLoadDebugFiles"     , "SDL.AutoLoadDebugFiles", 1);
	config->addOption("autoOpenDebugger"       , "SDL.AutoOpenDebugger"  , 0);
	config->addOption("debuggerPCPlacementMode", "SDL.DebuggerPCPlacement"  , 0);
	config->addOption("debuggerPCDLineOffset"  , "SDL.DebuggerPCLineOffset" , 0);
	config->addOption("SDL.DebuggerAsmFont"        , "");
	config->addOption("SDL.DebuggerStackFont"      , "");
	config->addOption("SDL.DebuggerCpuStatusFont"  , "");
	config->addOption("SDL.AsmSyntaxColorOpcode", "");
	config->addOption("SDL.AsmSyntaxColorAddress", "");
	config->addOption("SDL.AsmSyntaxColorImmediate", "");
	config->addOption("SDL.AsmSyntaxColorLabel", "");
	config->addOption("SDL.AsmSyntaxColorComment", "");
	config->addOption("SDL.AsmSyntaxColorPC", "");
	config->addOption("SDL.AsmShowByteCodes", 0);
	config->addOption("SDL.AsmShowRomOffsets", 0);
	config->addOption("SDL.DebuggerShowSymNames", 1);
	config->addOption("SDL.DebuggerShowRegNames", 1);
	config->addOption("SDL.DebuggerBreakOnBadOpcodes", 0);
	config->addOption("SDL.DebuggerBreakOnUnloggedCode", 0);
	config->addOption("SDL.DebuggerBreakOnUnloggedData", 0);

	// Code Data Logger Options
	config->addOption("autoSaveCDL"  , "SDL.AutoSaveCDL", 1);
	config->addOption("autoLoadCDL"  , "SDL.AutoLoadCDL", 1);
	config->addOption("autoResumeCDL", "SDL.AutoResumeCDL", 0);

	// Trace Logger Options
	config->addOption("SDL.TraceLogRegisterState", 1);
	config->addOption("SDL.TraceLogProcessorState", 1);
	config->addOption("SDL.TraceLogNewInstructions", 0);
	config->addOption("SDL.TraceLogNewData", 0);
	config->addOption("SDL.TraceLogFrameCount", 0);
	config->addOption("SDL.TraceLogCycleCount", 0);
	config->addOption("SDL.TraceLogInstructionCount", 0);
	config->addOption("SDL.TraceLogMessages", 1);
	config->addOption("SDL.TraceLogBreakpointHits", 1);
	config->addOption("SDL.TraceLogBankNumber", 0);
	config->addOption("SDL.TraceLogSymbolic", 0);
	config->addOption("SDL.TraceLogStackTabbing", 1);
	config->addOption("SDL.TraceLogLeftDisassembly", 1);
	
	// overwrite the config file?
	config->addOption("no-config", "SDL.NoConfig", 0);

	config->addOption("autoresume", "SDL.AutoResume", 0);
    
	// video playback
	config->addOption("playmov", "SDL.Movie", "");
	config->addOption("subtitles", "SDL.SubtitleDisplay", 1);
	config->addOption("movielength", "SDL.MovieLength", 0);
	
	config->addOption("fourscore", "SDL.FourScore", 0);

	config->addOption("nofscursor", "SDL.NoFullscreenCursor", 1);
    
    #ifdef _S9XLUA_H
	// load lua script
	config->addOption("loadlua", "SDL.LuaScript", "");
    #endif
    
    #ifdef CREATE_AVI
	config->addOption("videolog",  "SDL.VideoLog",  "");
	config->addOption("mute", "SDL.MuteCapture", 0);
    #endif
    
    // auto load/save on gameload/close
	config->addOption("loadstate", "SDL.AutoLoadState", INVALID_STATE);
	config->addOption("savestate", "SDL.AutoSaveState", INVALID_STATE);

	//TODO implement this
	config->addOption("periodicsaves", "SDL.PeriodicSaves", 0);

	savPath = dir + "/sav";
	movPath = dir + "/movies";

	// prefixed with _ because they are internal (not cli options)
	config->addOption("_lastopenfile", "SDL.LastOpenFile", dir);
	config->addOption("_laststatefrom", "SDL.LastLoadStateFrom", savPath );
	config->addOption("_lastopennsf", "SDL.LastOpenNSF", dir);
	config->addOption("_lastsavestateas", "SDL.LastSaveStateAs", savPath );
	config->addOption("_lastopenmovie", "SDL.LastOpenMovie", movPath);
	config->addOption("_lastloadlua", "SDL.LastLoadLua", "");

	for (unsigned int i=0; i<10; i++)
	{
		char buf[128];
		sprintf(buf, "SDL.RecentRom%02u", i);

		config->addOption( buf, "");
	}

	config->addOption("_useNativeFileDialog", "SDL.UseNativeFileDialog", false);
	config->addOption("_useNativeMenuBar"   , "SDL.UseNativeMenuBar", false);
	config->addOption("SDL.PauseOnMainMenuAccess", false);
	config->addOption("SDL.ContextMenuEnable", true);
	config->addOption("SDL.GuiStyle", "");
	config->addOption("SDL.QtStyleSheet", "");
	config->addOption("SDL.QPaletteFile", "");
	config->addOption("SDL.UseCustomQss", 0);
	config->addOption("SDL.UseCustomQPal", 0);

	config->addOption("_setSchedParam"      , "SDL.SetSchedParam" , 0);
	config->addOption("_emuSchedPolicy"     , "SDL.EmuSchedPolicy", 0);
	config->addOption("_emuSchedNice"       , "SDL.EmuSchedNice"  , 0);
	config->addOption("_emuSchedPrioRt"     , "SDL.EmuSchedPrioRt", 40);
	config->addOption("_guiSchedPolicy"     , "SDL.GuiSchedPolicy", 0);
	config->addOption("_guiSchedNice"       , "SDL.GuiSchedNice"  , 0);
	config->addOption("_guiSchedPrioRt"     , "SDL.GuiSchedPrioRt", 40);
	config->addOption("_emuTimingMech"      , "SDL.EmuTimingMech" , 0);

	// fcm -> fm2 conversion
	config->addOption("fcmconvert", "SDL.FCMConvert", "");
    
	// fm2 -> srt conversion
	config->addOption("ripsubs", "SDL.RipSubs", "");
	
	// enable new PPU core
	config->addOption("newppu", "SDL.NewPPU", 0);

	// PPU Viewer Preferences
	config->addOption("SDL.NT_TileFocusPolicy", 0);
	config->addOption("SDL.PPU_TileFocusPolicy", 0);
	config->addOption("SDL.OAM_TileFocusPolicy", 0);
	config->addOption("SDL.PPU_MaskUnused", 0);
	config->addOption("SDL.PPU_InvertMask", 0);
	config->addOption("SDL.PPU_View1_8x16", 0);
	config->addOption("SDL.PPU_View2_8x16", 0);
	config->addOption("SDL.PPU_ViewScanLine", 0);
	config->addOption("SDL.PPU_ViewRefreshFrames", 1);
	config->addOption("SDL.NT_TileSelColor", "#FFFFFF");
	config->addOption("SDL.NT_TileGridColor", "#FF0000");
	config->addOption("SDL.NT_AttrGridColor", "#0000FF");
	config->addOption("SDL.NT_ViewScanLine", 0);
	config->addOption("SDL.NT_DrawScrollLines", 1);
	config->addOption("SDL.NT_DrawTileGridLines", 1);
	config->addOption("SDL.NT_DrawAttrGridLines", 0);
	config->addOption("SDL.NT_DrawAttrbView", 0);
	config->addOption("SDL.NT_IgnoreHidePal", 0);
	config->addOption("SDL.NT_RefreshFrames", 1);
	config->addOption("SDL.PPU_TileSelColor0", "#FFFFFF");
	config->addOption("SDL.PPU_TileGridColor0", "#7F7F7F");
	config->addOption("SDL.PPU_TileSelColor1", "#FFFFFF");
	config->addOption("SDL.PPU_TileGridColor1", "#7F7F7F");
	config->addOption("SDL.PPU_TileShowGrid0", 1);
	config->addOption("SDL.PPU_TileShowGrid1", 1);
	config->addOption("SDL.OAM_TileShowGrid", 0);
	config->addOption("SDL.OAM_TileSelColor", "#FFFFFF");
	config->addOption("SDL.OAM_TileGridColor", "#7F7F7F");
	config->addOption("SDL.OAM_LocatorColor", "#7F7F7F");
	config->addOption("SDL.OAM_ShowPosHex", 0);

	// quit when a+b+select+start is pressed
	config->addOption("4buttonexit", "SDL.ABStartSelectExit", 0);

	// GamePad 0 - 3
	for(unsigned int i = 0; i < GAMEPAD_NUM_DEVICES; i++) 
	{
		char buf[64];
		snprintf(buf, sizeof(buf)-1, "SDL.Input.GamePad.%u.", i);
		prefix = buf;

		config->addOption(prefix + "DeviceType", DefaultGamePadDevice[i]);
		config->addOption(prefix + "DeviceGUID", "");
		config->addOption(prefix + "Profile"   , "");
	}
    
	// PowerPad 0 - 1
	for(unsigned int i = 0; i < POWERPAD_NUM_DEVICES; i++) {
		char buf[64];
		snprintf(buf, sizeof(buf)-1, "SDL.Input.PowerPad.%u.", i);
		prefix = buf;

		config->addOption(prefix + "DeviceType", DefaultPowerPadDevice[i]);
		config->addOption(prefix + "DeviceNum",  0);
		for(unsigned int j = 0; j < POWERPAD_NUM_BUTTONS; j++) {
			config->addOption(prefix +PowerPadNames[j], DefaultPowerPad[i][j]);
		}
	}

	// QuizKing
	prefix = "SDL.Input.QuizKing.";
	config->addOption(prefix + "DeviceType", DefaultQuizKingDevice);
	config->addOption(prefix + "DeviceNum", 0);
	for(unsigned int j = 0; j < QUIZKING_NUM_BUTTONS; j++) {
		config->addOption(prefix + QuizKingNames[j], DefaultQuizKing[j]);
	}

	// HyperShot
	prefix = "SDL.Input.HyperShot.";
	config->addOption(prefix + "DeviceType", DefaultHyperShotDevice);
	config->addOption(prefix + "DeviceNum", 0);
	for(unsigned int j = 0; j < HYPERSHOT_NUM_BUTTONS; j++) {
		config->addOption(prefix + HyperShotNames[j], DefaultHyperShot[j]);
	}

	// Mahjong
	prefix = "SDL.Input.Mahjong.";
	config->addOption(prefix + "DeviceType", DefaultMahjongDevice);
	config->addOption(prefix + "DeviceNum", 0);
	for(unsigned int j = 0; j < MAHJONG_NUM_BUTTONS; j++) {
		config->addOption(prefix + MahjongNames[j], DefaultMahjong[j]);
	}

	// TopRider
	prefix = "SDL.Input.TopRider.";
	config->addOption(prefix + "DeviceType", DefaultTopRiderDevice);
	config->addOption(prefix + "DeviceNum", 0);
	for(unsigned int j = 0; j < TOPRIDER_NUM_BUTTONS; j++) {
		config->addOption(prefix + TopRiderNames[j], DefaultTopRider[j]);
	}

	// FTrainer
	prefix = "SDL.Input.FTrainer.";
	config->addOption(prefix + "DeviceType", DefaultFTrainerDevice);
	config->addOption(prefix + "DeviceNum", 0);
	for(unsigned int j = 0; j < FTRAINER_NUM_BUTTONS; j++) {
		config->addOption(prefix + FTrainerNames[j], DefaultFTrainer[j]);
	}

	// FamilyKeyBoard
	prefix = "SDL.Input.FamilyKeyBoard.";
	config->addOption(prefix + "DeviceType", DefaultFamilyKeyBoardDevice);
	config->addOption(prefix + "DeviceNum", 0);
	for(unsigned int j = 0; j < FAMILYKEYBOARD_NUM_BUTTONS; j++) {
		config->addOption(prefix + FamilyKeyBoardNames[j],
						DefaultFamilyKeyBoard[j]);
	}

	// for FAMICOM microphone in pad 2 pad 1 didn't have it
	// Takeshi no Chousenjou uses it for example.
	prefix = "SDL.Input.FamicomPad2.";
	config->addOption("rp2mic", prefix + "EnableMic", 0);

	// TODO: use a better data structure to store the hotkeys or something
	//			improve this code overall in the future to make it
	//			easier to maintain
	//const int Hotkeys[HK_MAX] = {
	//	SDLK_F1, // cheat menu
	//	SDLK_F2, // bind state
	//	SDLK_F3, // load lua
	//	SDLK_F4, // toggleBG
	//	SDLK_F5, // save state
	//	SDLK_F6, // fds select
	//	SDLK_F7, // load state
	//	SDLK_F8, // fds eject
	//	SDLK_F6, // VS insert coin
	//	SDLK_F8, // VS toggle dipswitch
	//	SDLK_PERIOD, // toggle frame display
	//	SDLK_F10, // toggle subtitle
	//	SDLK_F11, // reset
	//	SDLK_F12, // screenshot
	//	SDLK_PAUSE, // pause
	//	SDLK_MINUS, // speed++
	//	SDLK_EQUALS, // speed--
	//	SDLK_BACKSLASH, //frame advnace
	//	SDLK_TAB, // turbo
	//	SDLK_COMMA, // toggle input display
	//	SDLK_q, // toggle movie RW
	//	SDLK_QUOTE, // toggle mute capture
	//	0, // quit // edit 10/11/11 - don't map to escape, it causes ugly things to happen to sdl.  can be manually appended to config
	//	SDLK_DELETE, // frame advance lag skip
	//	SDLK_SLASH, // lag counter display
	//	SDLK_0, SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5,
	//	SDLK_6, SDLK_7, SDLK_8, SDLK_9,
	//	SDLK_PAGEUP, // select state next
	//	SDLK_PAGEDOWN, // select state prev
	//	0, // Volume Down Internal 
	//	0, // Volume Up Internal 
	//	SDLK_SCROLLLOCK }; // FKB Enable Toggle


	//Hotkeys[ HK_OPEN_ROM ].init( "OpenROM", QKeySequence(QKeySequence::Open) );

	prefix = "SDL.Hotkeys.";

	for(int i=0; i < HK_MAX; i++)
	{
		const char *hotKeyName, *hotKeySeq;
		std::string nameText, keyText;

		getHotKeyConfig( i, &hotKeyName, &hotKeySeq );

		//printf("Hot Key: '%s' = '%s' \n", hotKeyName, hotKeySeq );

		//keyText.assign(" mod=");

		//sprintf( buf, "  key=%s", SDL_GetKeyName( Hotkeys[i] ) );

		if ( hotKeyName[0] != 0 )
		{
			nameText.assign( hotKeyName );
			keyText.assign( hotKeySeq );

			config->addOption(prefix + nameText, keyText);

			Hotkeys[i].setConfigName( hotKeyName );
		}
	}
	// All mouse devices
	config->addOption("SDL.OekaKids.0.DeviceType", "Mouse");
	config->addOption("SDL.OekaKids.0.DeviceNum", 0);

	config->addOption("SDL.Arkanoid.0.DeviceType", "Mouse");
	config->addOption("SDL.Arkanoid.0.DeviceNum", 0);

	config->addOption("SDL.Shadow.0.DeviceType", "Mouse");
	config->addOption("SDL.Shadow.0.DeviceNum", 0);

	config->addOption("SDL.Zapper.0.DeviceType", "Mouse");
	config->addOption("SDL.Zapper.0.DeviceNum", 0);

	return config;
}

void
UpdateEMUCore(Config *config)
{
	int ntsccol, ntsctint, ntschue, flag, region;
	int startNTSC, endNTSC, startPAL, endPAL;
	std::string cpalette;

	config->getOption("SDL.NTSCpalette", &ntsccol);
	config->getOption("SDL.Tint", &ntsctint);
	config->getOption("SDL.Hue", &ntschue);
	FCEUI_SetNTSCTH(ntsccol, ntsctint, ntschue);

	config->getOption("SDL.ForceGrayScale", &force_grayscale);
	config->getOption("SDL.DeempBitSwap"  , &paldeemphswap);
	config->getOption("SDL.PalNotch"      , &palnotch);
	config->getOption("SDL.PalSaturation" , &palsaturation);
	config->getOption("SDL.PalSharpness"  , &palsharpness);
	config->getOption("SDL.PalContrast"   , &palcontrast);
	config->getOption("SDL.PalBrightness" , &palbrightness);
	palupdate = 1;

	config->getOption("SDL.Palette", &cpalette);
	if(cpalette.size()) {
		LoadCPalette(cpalette);
	}

	config->getOption("SDL.NewPPU", &newppu);

	config->getOption("SDL.PAL", &region);
	FCEUI_SetRegion(region);

	config->getOption("SDL.GameGenie", &flag);
	FCEUI_SetGameGenie(flag ? 1 : 0);

	config->getOption("SDL.Sound.LowPass", &flag);
	FCEUI_SetLowPass(flag ? 1 : 0);

	config->getOption("SDL.DisableSpriteLimit", &flag);
	FCEUI_DisableSpriteLimitation(flag ? 1 : 0);

	config->getOption("SDL.ScanLineStartNTSC", &startNTSC);
	config->getOption("SDL.ScanLineEndNTSC", &endNTSC);
	config->getOption("SDL.ScanLineStartPAL", &startPAL);
	config->getOption("SDL.ScanLineEndPAL", &endPAL);

#if DOING_SCANLINE_CHECKS
	for(int i = 0; i < 2; x++) {
		if(srendlinev[x]<0 || srendlinev[x]>239) srendlinev[x]=0;
		if(erendlinev[x]<srendlinev[x] || erendlinev[x]>239) erendlinev[x]=239;
	}
#endif

	FCEUI_SetRenderedLines(startNTSC, endNTSC, startPAL, endPAL);
}

