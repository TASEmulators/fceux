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

#include <string.h>
#include <stdio.h>

#include "main.h"
#include "dface.h"
#include "input.h"
#include "config.h"


#include "sdl-video.h"

#include "../common/cheat.h"
#include "../../movie.h"
#include "../../fceu.h"
#include "../../driver.h"
#ifdef _S9XLUA_H
#include "../../fceulua.h"
#endif


/** GLOBALS **/
int NoWaiting=1;
extern Config *g_config;
extern bool bindSavestate, frameAdvanceLagSkip, lagCounterDisplay;


/* UsrInputType[] is user-specified.  InputType[] is current
        (game loading can override user settings)
*/
static int UsrInputType[NUM_INPUT_DEVICES];
static int InputType[NUM_INPUT_DEVICES];
static int cspec = 0;
   
extern int gametype;

/**
 * Necessary for proper GUI functioning (configuring when a game isn't loaded).
 */
void
InputUserActiveFix()
{
    int x;
    for(x = 0; x < 3; x++) {
        InputType[x] = UsrInputType[x];
    }
}

/**
 * Parse game information and configure the input devices accordingly.
 */
void
ParseGIInput(FCEUGI *gi)
{
    gametype=gi->type;
 
    InputType[0] = UsrInputType[0];
    InputType[1] = UsrInputType[1];
    InputType[2] = UsrInputType[2];
 
    if(gi->input[0]>=0) {
        InputType[0] = gi->input[0];
    }
    if(gi->input[1]>=0) {
        InputType[1] = gi->input[1];
    }
    if(gi->inputfc>=0) {
        InputType[2] = gi->inputfc;
    }
    cspec = gi->cspecial;
}


static uint8  QuizKingData  = 0;
static uint8  HyperShotData = 0;
static uint32 MahjongData   = 0;
static uint32 FTrainerData  = 0;
static uint8  TopRiderData  = 0;
static uint8  BWorldData[1+13+1];

static void UpdateFKB(void);
static void UpdateGamepad(void);
static void UpdateQuizKing(void);
static void UpdateHyperShot(void);
static void UpdateMahjong(void);
static void UpdateFTrainer(void);
static void UpdateTopRider(void);

static uint32 JSreturn=0;

/**
 * Configure cheat devices (game genie, etc.).  Restarts the keyboard
 * and video subsystems.
 */
static void
DoCheatSeq()
{
    SilenceSound(1);
    KillVideo();

    DoConsoleCheatConfig();
    InitVideo(GameInfo);
    SilenceSound(0);
}

#include "keyscan.h"
static uint8 *g_keyState = 0;
static int    DIPS = 0;

static uint8 keyonce[MKK_COUNT];
#define KEY(__a) g_keyState[MKK(__a)]

static int
_keyonly(int a)
{
    if(g_keyState[a]) {
        if(!keyonce[a]) {
            keyonce[a] = 1;
            return(1);
        }
    } else {
        keyonce[a] = 0;
    }
    return(0);
}

#define keyonly(__a) _keyonly(MKK(__a))

static int g_fkbEnabled = 0;

// key definitions
int cheatMenuKey;
int loadLuaKey;
int renderBgKey;
int saveStateKey;
int loadStateKey;
int resetKey;
int screenshotKey;
int pauseKey;
int decreaseSpeedKey;
int increaseSpeedKey;
int frameAdvanceKey;
int powerKey;
int bindStateKey;
int frameAdvanceLagSkipKey;
int quitKey;
int stateKey[10];
int movieToggleFrameDisplayKey;
int lagCounterDisplayKey;
int SubtitleDisplayKey;
int InputDisplayKey;
#ifdef CREATE_AVI
int MuteCaptureKey;
#endif

// this function loads the sdl hotkeys from the config file into the
// global scope.  this elimates the need for accessing the config file
// on every cycle of keyboardinput()
void setHotKeys()
{
	g_config->getOption("SDL.Hotkeys.CheatMenu", &cheatMenuKey);
	#ifdef _S9XLUA_H
	g_config->getOption("SDL.Hotkeys.LoadLua", &loadLuaKey);
	#endif
	g_config->getOption("SDL.Hotkeys.RenderBG", &renderBgKey);
	g_config->getOption("SDL.Hotkeys.SaveState", &saveStateKey);
	g_config->getOption("SDL.Hotkeys.LoadState", &loadStateKey);
	g_config->getOption("SDL.Hotkeys.Reset", &resetKey);
	g_config->getOption("SDL.Hotkeys.Screenshot", &screenshotKey);
	g_config->getOption("SDL.Hotkeys.Pause", &pauseKey);
	g_config->getOption("SDL.Hotkeys.IncreaseSpeed", &increaseSpeedKey);
	g_config->getOption("SDL.Hotkeys.DecreaseSpeed", &decreaseSpeedKey);
	g_config->getOption("SDL.Hotkeys.FrameAdvance", &frameAdvanceKey);
	g_config->getOption("SDL.Hotkeys.Power", &powerKey);
	g_config->getOption("SDL.Hotkeys.Pause", &bindStateKey);
	g_config->getOption("SDL.Hotkeys.FrameAdvanceLagSkip", &frameAdvanceLagSkipKey);
	g_config->getOption("SDL.Hotkeys.Quit", &quitKey);
	g_config->getOption("SDL.Hotkeys.SelectState0", &stateKey[0]);
	g_config->getOption("SDL.Hotkeys.SelectState1", &stateKey[1]);
	g_config->getOption("SDL.Hotkeys.SelectState2", &stateKey[2]);
	g_config->getOption("SDL.Hotkeys.SelectState3", &stateKey[3]);
	g_config->getOption("SDL.Hotkeys.SelectState4", &stateKey[4]);
	g_config->getOption("SDL.Hotkeys.SelectState5", &stateKey[5]);
	g_config->getOption("SDL.Hotkeys.SelectState6", &stateKey[6]);
	g_config->getOption("SDL.Hotkeys.SelectState7", &stateKey[7]);
	g_config->getOption("SDL.Hotkeys.SelectState8", &stateKey[8]);
	g_config->getOption("SDL.Hotkeys.SelectState9", &stateKey[9]);
	g_config->getOption("SDL.Hotkeys.LagCounterDisplay", &lagCounterDisplayKey);
	g_config->getOption("SDL.Hotkeys.MovieToggleFrameDisplay", &movieToggleFrameDisplayKey);
	g_config->getOption("SDL.Hotkeys.SubtitleDisplay", &SubtitleDisplayKey);
	g_config->getOption("SDL.Hotkeys.InputDisplay", &InputDisplayKey);
	#ifdef CREATE_AVI
	g_config->getOption("SDL.Hotkeys.MuteCapture", &MuteCaptureKey);
	#endif
	/*
	config->addOption(prefix + "FrameAdvance", SDLK_BACKSLASH);
	config->addOption(prefix + "Power", 0);
	config->addOption(prefix + "BindState", SDLK_F2);
	config->addOption(prefix + "FrameAdvanceLagSkip", SDLK_F6);
	config->addOption(prefix + "LagCounterDisplay", SDLK_F8);
	config->addOption(prefix + "SelectState0", SDLK_0);
	config->addOption(prefix + "SelectState1", SDLK_1);
	config->addOption(prefix + "SelectState2", SDLK_2);
	config->addOption(prefix + "SelectState3", SDLK_3);
	config->addOption(prefix + "SelectState4", SDLK_4);
	config->addOption(prefix + "SelectState5", SDLK_5);
	config->addOption(prefix + "SelectState6", SDLK_6);
	config->addOption(prefix + "SelectState7", SDLK_7);
	config->addOption(prefix + "SelectState8", SDLK_8);
	config->addOption(prefix + "SelectState9", SDLK_9);*/
	return;
}

/*** 
 * This function opens a file chooser dialog and returns the filename the 
 * user selected.
 * */
std::string GetFilename(const char* title)
{
    if (FCEUI_EmulationPaused() == 0)
        FCEUI_ToggleEmulationPause();
    std::string fname = "";
    
    #ifdef WIN32
    OPENFILENAME ofn;       // common dialog box structure
    char szFile[260];       // buffer for file name
    HWND hwnd;              // owner window
    HANDLE hf;              // file handle

    // Initialize OPENFILENAME
    memset(&ofn,0,sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hwnd;
    ofn.lpstrFile = szFile;
    // Set lpstrFile[0] to '\0' so that GetOpenFileName does not 
    // use the contents of szFile to initialize itself.
    ofn.lpstrFile[0] = '\0';
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "All\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.lpstrFileTitle = NULL;
    ofn.nMaxFileTitle = 0;
    ofn.lpstrInitialDir = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    // Display the Open dialog box. 
    fname = GetOpenFileName(&ofn);

    #else
	int fullscreen = 0;
	g_config->getOption("SDL.Fullscreen", &fullscreen);
	if(fullscreen)
		ToggleFS();
    FILE *fpipe;
    std::string command = "zenity --file-selection --title=\"";
    command.append(title);
    command.append("\"");
    if ( !(fpipe = (FILE*)popen(command.c_str(),"r")) )
    {  // If fpipe is NULL
        FCEUD_PrintError("Pipe error on opening zenity");
    }
      
    int c; 
    while( (c = fgetc(fpipe)) )
    {
        if (c == EOF)
            break;
        if (c == '\n')
            break;
        fname += c;
    }
    pclose(fpipe);
    #endif
    FCEUI_ToggleEmulationPause();
    return fname;
}

/**
 * Parse keyboard commands and execute accordingly.
 */
static void
KeyboardCommands()
{
    int is_shift, is_alt;
    char* movie_fname = "";
    // get the keyboard input
    g_keyState = SDL_GetKeyState(NULL);

    // check if the family keyboard is enabled
    if(InputType[2] == SIFC_FKB) {
        if(keyonly(SCROLLLOCK)) {
            g_fkbEnabled ^= 1;
            FCEUI_DispMessage("Family Keyboard %sabled.",
                              g_fkbEnabled ? "en" : "dis");
        }
        SDL_WM_GrabInput(g_fkbEnabled ? SDL_GRAB_ON : SDL_GRAB_OFF);
        if(g_fkbEnabled) {
            return;
        }
    }

    is_shift = KEY(LEFTSHIFT) | KEY(RIGHTSHIFT);
    is_alt = KEY(LEFTALT) | KEY(RIGHTALT);
    
    if(_keyonly(renderBgKey)) {
        if(is_shift) {
            FCEUI_SetRenderPlanes(true, false);
        } else {
            FCEUI_SetRenderPlanes(true, true);
        }
    }

    // Alt-Enter to toggle full-screen
    if(keyonly(ENTER) && is_alt) {
        ToggleFS();
    }

    // Toggle throttling
    NoWaiting &= ~1;
    if(KEY(TAB)) {
        NoWaiting |= 1;
    }
    
    // Toggle Movie auto-backup
    if(keyonly(M) && is_shift) {
        autoMovieBackup ^= 1;
        FCEUI_DispMessage("Automatic movie backup %sabled.",
          autoMovieBackup ? "en" : "dis");
    }
    

    // Famicom disk-system games
    if(gametype==GIT_FDS) 
	{
        if(keyonly(F6)) {
            FCEUI_FDSSelect();
        }
        if(keyonly(F8)) {
            FCEUI_FDSInsert();
        }
	}
    
    if(_keyonly(screenshotKey)) {
        FCEUI_SaveSnapshot();
    }

    // if not NES Sound Format
    if(gametype != GIT_NSF) {
        if(_keyonly(cheatMenuKey)) {
            DoCheatSeq();
        }
		
		// f5 (default) save key, hold shift to save movie
        if(_keyonly(saveStateKey)) {
            if(is_shift) {
                movie_fname = const_cast<char*>(FCEU_MakeFName(FCEUMKF_MOVIE, 0, 0).c_str());
				FCEUI_printf("Recording movie to %s\n", movie_fname);
                FCEUI_SaveMovie(movie_fname, MOVIE_FLAG_NONE, L"");
            } else {
                FCEUI_SaveState(NULL);
            }
        }
        
        // f7 to load state, Shift-f7 to load movie
        if(_keyonly(loadStateKey)) {
            if(is_shift) {
                FCEUI_StopMovie();
                std::string fname;
                fname = GetFilename("Open FM2 movie for playback...");
                if(fname != "")
                {
                    if(fname.find(".fm2") != std::string::npos)
                    {
				        FCEUI_printf("Playing back movie located at %s\n", fname.c_str());
                        FCEUI_LoadMovie(fname.c_str(), false, false, false);
                    }
                    else
                    {
                        FCEUI_printf("Only FM2 movies are supported.\n");
                    }
                }
            } else {
                FCEUI_LoadState(NULL);
            }
        }
    }

    
    if(_keyonly(decreaseSpeedKey)) {
        DecreaseEmulationSpeed();
    }

    if(_keyonly(increaseSpeedKey)) {
        IncreaseEmulationSpeed();
    }

    if(_keyonly(movieToggleFrameDisplayKey)) {
        FCEUI_MovieToggleFrameDisplay();
    }
    
    if(_keyonly(InputDisplayKey)) {
        FCEUI_ToggleInputDisplay();
        extern int input_display;
        g_config->setOption("SDL.InputDisplay", input_display);
    }
    
    #ifdef CREATE_AVI
    if(_keyonly(MuteCaptureKey)) {
        extern int mutecapture;
        mutecapture ^= 1;
    }
    #endif

    if(_keyonly(pauseKey)) {
        FCEUI_ToggleEmulationPause();
    }
    
    static bool frameAdvancing = false;
    if(g_keyState[frameAdvanceKey])
    {
        if(frameAdvancing == false)
        {
            FCEUI_FrameAdvance();
            frameAdvancing = true;
        }
    }
    else
    {
        if(frameAdvancing)
        {
            FCEUI_FrameAdvanceEnd();
            frameAdvancing = false;
        }
    }
    
    if(_keyonly(resetKey)) {
        FCEUI_ResetNES();
    }
    if(_keyonly(powerKey)) {
        FCEUI_PowerNES();
    }
	
    if(_keyonly(quitKey)) {
        FCEUI_CloseGame();
    }
    #ifdef _S9XLUA_H
    if(_keyonly(loadLuaKey)) {
        std::string fname;
        fname = GetFilename("Open LUA script...");
        if(fname != "")
            FCEU_LoadLuaCode(fname.c_str());
    }
    #endif
	
	for(int i=0; i<10; i++)
		if(_keyonly(stateKey[i]))
			FCEUI_SelectState(i, 1);
	
	if(_keyonly(bindStateKey)) {
        bindSavestate ^= 1;
        FCEUI_DispMessage("Savestate binding to movie %sabled.",
            bindSavestate ? "en" : "dis");
    }
    
    if(_keyonly(frameAdvanceLagSkipKey)) {
        frameAdvanceLagSkip ^= 1;
        FCEUI_DispMessage("Skipping lag in Frame Advance %sabled.",
            frameAdvanceLagSkip ? "en" : "dis");
    }
    
    if(_keyonly(lagCounterDisplayKey)) {
        lagCounterDisplay ^= 1;
    }
    
    if (_keyonly(SubtitleDisplayKey)) {
        extern int movieSubtitles;
        movieSubtitles ^= 1;
        FCEUI_DispMessage("Movie subtitles o%s.",
            movieSubtitles ? "n" : "ff");
    }

    // VS Unisystem games
    if(gametype == GIT_VSUNI) {
        // insert coin
        if(keyonly(F8)) FCEUI_VSUniCoin();

        // toggle dipswitch display
        if(keyonly(F6)) {
            DIPS^=1;
            FCEUI_VSUniToggleDIPView();
        }
        if(!(DIPS&1)) goto DIPSless;

        // toggle the various dipswitches
        if(keyonly(1)) FCEUI_VSUniToggleDIP(0);
        if(keyonly(2)) FCEUI_VSUniToggleDIP(1);
        if(keyonly(3)) FCEUI_VSUniToggleDIP(2);
        if(keyonly(4)) FCEUI_VSUniToggleDIP(3);
        if(keyonly(5)) FCEUI_VSUniToggleDIP(4);
        if(keyonly(6)) FCEUI_VSUniToggleDIP(5);
        if(keyonly(7)) FCEUI_VSUniToggleDIP(6);
        if(keyonly(8)) FCEUI_VSUniToggleDIP(7);
    } else {
        static uint8 bbuf[32];
        static int bbuft;
        static int barcoder = 0;

        if(keyonly(H)) FCEUI_NTSCSELHUE();
        if(keyonly(T)) FCEUI_NTSCSELTINT();
        if(KEY(KP_MINUS) || KEY(MINUS)) FCEUI_NTSCDEC();
        if(KEY(KP_PLUS) || KEY(EQUAL)) FCEUI_NTSCINC();

        if((InputType[2] == SIFC_BWORLD) || (cspec == SIS_DATACH)) {
            if(keyonly(F8)) {
                barcoder ^= 1;
                if(!barcoder) {
                    if(InputType[2] == SIFC_BWORLD) {
                        strcpy((char *)&BWorldData[1], (char *)bbuf);
                        BWorldData[0] = 1;
                    } else {
                        FCEUI_DatachSet(bbuf);
                    }
                    FCEUI_DispMessage("Barcode Entered");
                } else { 
                    bbuft = 0;
                    FCEUI_DispMessage("Enter Barcode");
                }
            }
        } else {
            barcoder = 0;
        }

#define SSM(x)                                    \
do {                                              \
    if(barcoder) {                                \
        if(bbuft < 13) {                          \
            bbuf[bbuft++] = '0' + x;              \
            bbuf[bbuft] = 0;                      \
        }                                         \
        FCEUI_DispMessage("Barcode: %s", bbuf);   \
	}                                             \
} while(0)

    DIPSless:
        if(keyonly(0)) SSM(0);
        if(keyonly(1)) SSM(1);
        if(keyonly(2)) SSM(2);
        if(keyonly(3)) SSM(3);
        if(keyonly(4)) SSM(4);
        if(keyonly(5)) SSM(5);
        if(keyonly(6)) SSM(6);
        if(keyonly(7)) SSM(7);
        if(keyonly(8)) SSM(8);
        if(keyonly(9)) SSM(9);
#undef SSM
    }
}

/**
 * Return the state of the mouse buttons.  Input 'd' is an array of 3
 * integers that store <x, y, button state>.
 */
void // removed static for a call in lua-engine.cpp
GetMouseData(uint32 (&d)[3])
{
    int x,y;
    uint32 t;

    // Don't get input when a movie is playing back
    if(FCEUMOV_Mode(MOVIEMODE_PLAY))
        return;

    // retrieve the state of the mouse from SDL
    t = SDL_GetMouseState(&x, &y);

    d[2] = 0;
    if(t & SDL_BUTTON(1)) {
        d[2] |= 0x1;
    }
    if(t & SDL_BUTTON(3)) {
        d[2] |= 0x2;
    }

    // get the mouse position from the SDL video driver
    t = PtoV(x, y);
    d[0] = t & 0xFFFF;
    d[1] = (t >> 16) & 0xFFFF;
}

/**
 * Handles outstanding SDL events.
 */
static void
UpdatePhysicalInput()
{
    SDL_Event event;

    // loop, handling all pending events
    while(SDL_PollEvent(&event)) {
        switch(event.type) {
        case SDL_QUIT:
            FCEUI_CloseGame();
            puts("Quit");
            break;
        default:
            // do nothing
            break;
        }
    }
    //SDL_PumpEvents();
}


static int bcpv,bcpj;

/**
 *  Begin configuring the buttons by placing the video and joystick
 *  subsystems into a well-known state.  Button configuration really
 *  needs to be cleaned up after the new config system is in place.
 */
static int
ButtonConfigBegin()
{
    SDL_Surface *screen;

    // XXX soules - why are we doing this right before KillVideo()?
    SDL_QuitSubSystem(SDL_INIT_VIDEO);

    // shut down the video and joystick subsystems
    bcpv=KillVideo();
    bcpj=KillJoysticks();
 
    // reactivate the video subsystem
    if(!SDL_WasInit(SDL_INIT_VIDEO)) {
        if(SDL_InitSubSystem(SDL_INIT_VIDEO) == -1) {
                FCEUD_Message(SDL_GetError());
                return(0);
        }
    }

    // set the screen and notify the user of button configuration
    screen = SDL_SetVideoMode(420, 200, 8, 0); 
    SDL_WM_SetCaption("Button Config",0);

    // XXX soules - why did we shut this down?
    // initialize the joystick subsystem
    InitJoysticks();
 
    return(1);
}

/**
 *  Finish configuring the buttons by reverting the video and joystick
 *  subsystems to their previous state.  Button configuration really
 *  needs to be cleaned up after the new config system is in place.
 */
static void
ButtonConfigEnd()
{ 
    extern FCEUGI *GameInfo;

    // shutdown the joystick and video subsystems
    KillJoysticks();
    SDL_QuitSubSystem(SDL_INIT_VIDEO); 

    // re-initialize joystick and video subsystems if they were active before
    if(!bcpv) {
        InitVideo(GameInfo);
    }
    if(!bcpj) {
        InitJoysticks();
    }
}

/**
 * Tests to see if a specified button is currently pressed.
 */
static int
DTestButton(ButtConfig *bc)
{
    int x;

    for(x = 0; x < bc->NumC; x++) {
        if(bc->ButtType[x] == BUTTC_KEYBOARD) {
            if(g_keyState[bc->ButtonNum[x]]) {
                return(1);
            }
        } else if(bc->ButtType[x] == BUTTC_JOYSTICK) {
            if(DTestButtonJoy(bc)) {
                return(1);
            }
        }
    }
    return(0);
}




#define MK(x)       {{BUTTC_KEYBOARD},{0},{MKK(x)},1}
#define MK2(x1,x2)  {{BUTTC_KEYBOARD},{0},{MKK(x1),MKK(x2)},2}
#define MKZ()       {{0},{0},{0},0}
#define GPZ()       {MKZ(), MKZ(), MKZ(), MKZ()}

static ButtConfig GamePadConfig[4][10]={
        /* Gamepad 1 */
        { MK(KP3), MK(KP2), MK(TAB), MK(ENTER),
          MK(W), MK(Z), MK(A), MK(S), MKZ(), MKZ() },

        /* Gamepad 2 */
        GPZ(),

        /* Gamepad 3 */
        GPZ(),
  
        /* Gamepad 4 */
        GPZ()
};

/**
 * Update the status of the gamepad input devices.
 */
static void
UpdateGamepad(void)
{
    // don't update during movie playback
    if(FCEUMOV_Mode(MOVIEMODE_PLAY)) {
        return;
    }

    static int rapid=0;
    uint32 JS=0;
    int x;
    int wg;

    rapid ^= 1;

    // go through each of the four game pads
    for(wg = 0; wg < 4; wg++) {
        // the 4 directional buttons, start, select, a, b
        for(x = 0; x < 8; x++) {
            if(DTestButton(&GamePadConfig[wg][x])) {
                JS |= (1 << x) << (wg << 3);
            }
        }

        // rapid-fire a, rapid-fire b
        if(rapid) {
            for(x = 0; x < 2; x++) {
                if(DTestButton(&GamePadConfig[wg][8+x])) {
                    JS |= (1 << x) << (wg << 3);
                }
            }
        }
    }

    //  for(x=0;x<32;x+=8)	/* Now, test to see if anything weird(up+down at same time)
    //			   is happening, and correct */
    //  {
    //   if((JS & (0xC0<<x) ) == (0xC0<<x) ) JS&=~(0xC0<<x);
    //   if((JS & (0x30<<x) ) == (0x30<<x) ) JS&=~(0x30<<x);
    //  }

    JSreturn = JS;
}

static ButtConfig powerpadsc[2][12]={
                              {
                               MK(O),MK(P),MK(BRACKET_LEFT),
                               MK(BRACKET_RIGHT),MK(K),MK(L),MK(SEMICOLON),
				MK(APOSTROPHE),
                               MK(M),MK(COMMA),MK(PERIOD),MK(SLASH)
                              },
                              {
                               MK(O),MK(P),MK(BRACKET_LEFT),
                               MK(BRACKET_RIGHT),MK(K),MK(L),MK(SEMICOLON),
                                MK(APOSTROPHE),
                               MK(M),MK(COMMA),MK(PERIOD),MK(SLASH)
                              }
                             };

static uint32 powerpadbuf[2]={0,0};

/**
 * Update the status of the power pad input device.
 */
static uint32
UpdatePPadData(int w)
{
    // don't update if a movie is playing
    if(FCEUMOV_Mode(MOVIEMODE_PLAY)) {
        return 0;
    }

    uint32 r = 0;
    ButtConfig *ppadtsc = powerpadsc[w];
    int x;

    // update each of the 12 buttons
    for(x = 0; x < 12; x++) {
        if(DTestButton(&ppadtsc[x])) {
            r |= 1 << x;
        }
    }

    return r;
}

static uint32 MouseData[3]={0,0,0};
static uint8 fkbkeys[0x48];

/**
 * Update all of the input devices required for the active game.
 */
void
FCEUD_UpdateInput()
{
    int x;
    int t = 0;

    UpdatePhysicalInput();
    KeyboardCommands();

    for(x = 0; x < 2; x++) {
        switch(InputType[x]) {
        case SI_GAMEPAD:
            t |= 1;
            break;
        case SI_ARKANOID:
            t |= 2;
            break;
        case SI_ZAPPER:
            t |= 2;
            break;
        case SI_POWERPADA:
        case SI_POWERPADB:
            powerpadbuf[x] = UpdatePPadData(x);
            break;
        }
    }

    switch(InputType[2]) {
    case SIFC_ARKANOID:
        t |= 2;
        break;
    case SIFC_SHADOW:
        t |= 2;
        break;
    case SIFC_FKB:
        if(g_fkbEnabled) {
            UpdateFKB();
        }
        break;
    case SIFC_HYPERSHOT:
        UpdateHyperShot();
        break;
    case SIFC_MAHJONG:
        UpdateMahjong();
        break;
    case SIFC_QUIZKING:
        UpdateQuizKing();
        break;
    case SIFC_FTRAINERB:
    case SIFC_FTRAINERA:
        UpdateFTrainer();
        break;
    case SIFC_TOPRIDER:
        UpdateTopRider();
        break;
    case SIFC_OEKAKIDS:
        t |= 2;
        break;
    }

    if(t & 1) {
        UpdateGamepad();
    }

    if(t & 2) {
        GetMouseData(MouseData);
    }
}

void FCEUD_SetInput(bool fourscore, ESI port0, ESI port1, ESIFC fcexp)
{
	eoptions &= ~EO_FOURSCORE;
	if(fourscore) eoptions |= EO_FOURSCORE;

	InputType[0]=port0;
	InputType[1]=port1;
	InputType[2]=fcexp;
	InitInputInterface();
}

/**
 * Initialize the input device interface between the emulation and the driver.
 */
void
InitInputInterface()
{
    void *InputDPtr;

    int t;
    int x;
    int attrib;

    for(t = 0, x = 0; x < 2; x++) {
        attrib    = 0;
        InputDPtr = 0;

        switch(InputType[x]) {
        case SI_POWERPADA:
        case SI_POWERPADB:
            InputDPtr = &powerpadbuf[x];
            break;
        case SI_GAMEPAD:
            InputDPtr = &JSreturn;
            break;     
        case SI_ARKANOID:
            InputDPtr = MouseData;
            t |= 1;
            break;
        case SI_ZAPPER:
            InputDPtr = MouseData;
            t |= 1;
            attrib = 1;
            break;
        }
        FCEUI_SetInput(x, (ESI)InputType[x], InputDPtr, attrib);
    }

    attrib    = 0;
    InputDPtr = 0;
    switch(InputType[2]) {
    case SIFC_SHADOW:
        InputDPtr = MouseData;
        t |= 1;
        attrib = 1;
        break;
    case SIFC_OEKAKIDS:
        InputDPtr = MouseData;
        t |= 1;
        attrib = 1;
        break;
    case SIFC_ARKANOID:
        InputDPtr = MouseData;
        t |= 1;
        break;
    case SIFC_FKB:
        InputDPtr = fkbkeys;
        break;
    case SIFC_HYPERSHOT:
        InputDPtr = &HyperShotData;
        break;
    case SIFC_MAHJONG:
        InputDPtr = &MahjongData;
        break;
    case SIFC_QUIZKING:
        InputDPtr = &QuizKingData;
        break;
    case SIFC_TOPRIDER:
        InputDPtr = &TopRiderData;
        break;
    case SIFC_BWORLD:
        InputDPtr = BWorldData;
        break;
    case SIFC_FTRAINERA:
    case SIFC_FTRAINERB:
        InputDPtr = &FTrainerData;
        break;
    }

    FCEUI_SetInputFC((ESIFC)InputType[2], InputDPtr, attrib);
    FCEUI_SetInputFourscore((eoptions & EO_FOURSCORE)!=0);
}


static ButtConfig fkbmap[0x48]=
{
 MK(F1),MK(F2),MK(F3),MK(F4),MK(F5),MK(F6),MK(F7),MK(F8),
 MK(1),MK(2),MK(3),MK(4),MK(5),MK(6),MK(7),MK(8),MK(9),MK(0),
        MK(MINUS),MK(EQUAL),MK(BACKSLASH),MK(BACKSPACE),
 MK(ESCAPE),MK(Q),MK(W),MK(E),MK(R),MK(T),MK(Y),MK(U),MK(I),MK(O),
        MK(P),MK(GRAVE),MK(BRACKET_LEFT),MK(ENTER),
 MK(LEFTCONTROL),MK(A),MK(S),MK(D),MK(F),MK(G),MK(H),MK(J),MK(K),
        MK(L),MK(SEMICOLON),MK(APOSTROPHE),MK(BRACKET_RIGHT),MK(INSERT),
 MK(LEFTSHIFT),MK(Z),MK(X),MK(C),MK(V),MK(B),MK(N),MK(M),MK(COMMA),
        MK(PERIOD),MK(SLASH),MK(RIGHTALT),MK(RIGHTSHIFT),MK(LEFTALT),MK(SPACE),
 MK(DELETE),MK(END),MK(PAGEDOWN),
 MK(CURSORUP),MK(CURSORLEFT),MK(CURSORRIGHT),MK(CURSORDOWN)
};

/**
 * Update the status of the Family KeyBoard.
 */
static void
UpdateFKB()
{
    int x;

    for(x = 0; x < 0x48; x++) {
        fkbkeys[x] = 0;

        if(DTestButton(&fkbmap[x])) {
            fkbkeys[x] = 1;
        }
    }
}

static ButtConfig HyperShotButtons[4]=
{
 MK(Q),MK(W),MK(E),MK(R)
};

/**
 * Update the status of the HyperShot input device.
 */
static void
UpdateHyperShot()
{
    int x;

    HyperShotData=0;
    for(x = 0; x < 0x4; x++) {
        if(DTestButton(&HyperShotButtons[x])) {
            HyperShotData |= 1 << x;
        }
    }
}

static ButtConfig MahjongButtons[21]=
{
 MK(Q),MK(W),MK(E),MK(R),MK(T),
 MK(A),MK(S),MK(D),MK(F),MK(G),MK(H),MK(J),MK(K),MK(L),
 MK(Z),MK(X),MK(C),MK(V),MK(B),MK(N),MK(M)
};

/**
 * Update the status of for the Mahjong input device.
 */
static void
UpdateMahjong()
{
    int x;
        
    MahjongData = 0;
    for(x = 0; x < 21; x++) {
        if(DTestButton(&MahjongButtons[x])) {
            MahjongData |= 1 << x;
        }
    }
}

static ButtConfig QuizKingButtons[6]=
{
 MK(Q),MK(W),MK(E),MK(R),MK(T),MK(Y)
};

/**
 * Update the status of the QuizKing input device.
 */
static void
UpdateQuizKing()
{
    int x;

    QuizKingData=0;

    for(x = 0; x < 6; x++) {
        if(DTestButton(&QuizKingButtons[x])) {
            QuizKingData |= 1 << x;
        }
    }
}

static ButtConfig TopRiderButtons[8]=
{
 MK(Q),MK(W),MK(E),MK(R),MK(T),MK(Y),MK(U),MK(I)
};

/**
 * Update the status of the TopRider input device.
 */
static void
UpdateTopRider()
{
    int x;
    TopRiderData=0;
    for(x = 0; x < 8; x++) {
        if(DTestButton(&TopRiderButtons[x])) {
            TopRiderData |= (1 << x);
        }
    }
}

static ButtConfig FTrainerButtons[12]=
{
                               MK(O),MK(P),MK(BRACKET_LEFT),
                               MK(BRACKET_RIGHT),MK(K),MK(L),MK(SEMICOLON),
                                MK(APOSTROPHE),
                               MK(M),MK(COMMA),MK(PERIOD),MK(SLASH)
};

/**
 * Update the status of the FTrainer input device.
 */
static void
UpdateFTrainer()
{
    int x;
    FTrainerData = 0;

    for(x = 0; x < 12; x++) {
        if(DTestButton(&FTrainerButtons[x])) {
            FTrainerData |= (1 << x);
        }
    }
}

/**
 * Waits for a button input and returns the information as to which
 * button was pressed.  Used in button configuration.
 */
static int
DWaitButton(const uint8 *text,
            ButtConfig *bc,
            int wb)
{
    SDL_Event event;
    static int32 LastAx[64][64];
    int x,y;
    std::string title = "Press a key for ";
	title += (const char*)text;
    SDL_WM_SetCaption(title.c_str(),0);
    puts((const char *)text);
    for(x = 0; x < 64; x++) {
        for(y = 0; y < 64; y++) {
            LastAx[x][y]=0x100000;
        }
    }

    while(SDL_WaitEvent(&event)) {
        switch(event.type) {
        case SDL_KEYDOWN:
            bc->ButtType[wb]  = BUTTC_KEYBOARD;
            bc->DeviceNum[wb] = 0;
            bc->ButtonNum[wb] = event.key.keysym.sym;
            return(1);
        case SDL_JOYBUTTONDOWN:
            bc->ButtType[wb]  = BUTTC_JOYSTICK;
            bc->DeviceNum[wb] = event.jbutton.which;
            bc->ButtonNum[wb] = event.jbutton.button; 
            return(1);
        case SDL_JOYHATMOTION:
            if(event.jhat.value != SDL_HAT_CENTERED) {
                bc->ButtType[wb]  = BUTTC_JOYSTICK;
                bc->DeviceNum[wb] = event.jhat.which;
                bc->ButtonNum[wb] = (0x2000 | ((event.jhat.hat & 0x1F) << 8) |
                                     event.jhat.value);
                return(1);
            }
            break;
        case SDL_JOYAXISMOTION: 
            if(LastAx[event.jaxis.which][event.jaxis.axis] == 0x100000) {
                if(abs(event.jaxis.value) < 1000) {
                    LastAx[event.jaxis.which][event.jaxis.axis] = event.jaxis.value;
                }
            } else {
                if(abs(LastAx[event.jaxis.which][event.jaxis.axis] - event.jaxis.value) >= 8192)  {
                    bc->ButtType[wb]  = BUTTC_JOYSTICK;
                    bc->DeviceNum[wb] = event.jaxis.which;
                    bc->ButtonNum[wb] = (0x8000 | event.jaxis.axis |
                                         ((event.jaxis.value < 0)
                                          ? 0x4000 : 0));
                    return(1);
                }
            }
            break;
        }
    }

    return(0);
}

/**
 * This function takes in button inputs until either it sees two of
 * the same button presses in a row or gets four inputs and then saves
 * the total number of button presses.  Each of the keys pressed is
 * used as input for the specified button, thus allowing up to four
 * possible settings for each input button.
 */
static void
ConfigButton(char *text,
       ButtConfig *bc)
{
    uint8 buf[256];
    int wc;

    for(wc = 0; wc < MAXBUTTCONFIG; wc++) {
        sprintf((char *)buf, "%s (%d)", text, wc + 1);
        DWaitButton(buf, bc, wc);

        if(wc &&
           bc->ButtType[wc] == bc->ButtType[wc - 1] &&
           bc->DeviceNum[wc] == bc->DeviceNum[wc - 1] &&
           bc->ButtonNum[wc]==bc->ButtonNum[wc-1]) {
            break;
        }
    }
    bc->NumC = wc;
}

/**
 * Update the button configuration for a specified device.
 */
extern Config *g_config;

void
ConfigDevice(int which,
             int arg)
{
    char buf[256];
    int x;
    std::string prefix;
    char *str[10]={"A","B","SELECT","START","UP","DOWN","LEFT","RIGHT","Rapid A","Rapid B"};

    // XXX soules - set the configuration options so that later calls
    //              don't override these.  This is a temp hack until I
    //              can clean up this file.

    ButtonConfigBegin();
    switch(which) {
    case FCFGD_QUIZKING:
        prefix = "SDL.Input.QuizKing.";
        for(x = 0; x < 6; x++) {
            sprintf(buf, "Quiz King Buzzer #%d", x+1);
            ConfigButton(buf, &QuizKingButtons[x]);

            g_config->setOption(prefix + QuizKingNames[x],
                                QuizKingButtons[x].ButtonNum[0]);
        }

        if(QuizKingButtons[0].ButtType[0] == BUTTC_KEYBOARD) {
            g_config->setOption(prefix + "DeviceType", "Keyboard");
        } else if(QuizKingButtons[0].ButtType[0] == BUTTC_JOYSTICK) {
            g_config->setOption(prefix + "DeviceType", "Joystick");
        } else {
            g_config->setOption(prefix + "DeviceType", "Unknown");
        }
        g_config->setOption(prefix + "DeviceNum",
                            QuizKingButtons[0].DeviceNum[0]);
        break;
    case FCFGD_HYPERSHOT:
        prefix = "SDL.Input.HyperShot.";
        for(x = 0; x < 4; x++) {
            sprintf(buf, "Hyper Shot %d: %s",
                    ((x & 2) >> 1) + 1, (x & 1) ? "JUMP" : "RUN");
            ConfigButton(buf, &HyperShotButtons[x]);

            g_config->setOption(prefix + HyperShotNames[x],
                                HyperShotButtons[x].ButtonNum[0]);
        }

        if(HyperShotButtons[0].ButtType[0] == BUTTC_KEYBOARD) {
            g_config->setOption(prefix + "DeviceType", "Keyboard");
        } else if(HyperShotButtons[0].ButtType[0] == BUTTC_JOYSTICK) {
            g_config->setOption(prefix + "DeviceType", "Joystick");
        } else {
            g_config->setOption(prefix + "DeviceType", "Unknown");
        }
        g_config->setOption(prefix + "DeviceNum",
                            HyperShotButtons[0].DeviceNum[0]);
        break;
    case FCFGD_POWERPAD:
        snprintf(buf, 256, "SDL.Input.PowerPad.%d", (arg & 1));
        prefix = buf;
        for(x = 0; x < 12; x++) {
            sprintf(buf, "PowerPad %d: %d", (arg & 1) + 1, x + 11);
            ConfigButton(buf, &powerpadsc[arg&1][x]);

            g_config->setOption(prefix + PowerPadNames[x],
                                powerpadsc[arg & 1][x].ButtonNum[0]);
        }

        if(powerpadsc[arg & 1][0].ButtType[0] == BUTTC_KEYBOARD) {
            g_config->setOption(prefix + "DeviceType", "Keyboard");
        } else if(powerpadsc[arg & 1][0].ButtType[0] == BUTTC_JOYSTICK) {
            g_config->setOption(prefix + "DeviceType", "Joystick");
        } else {
            g_config->setOption(prefix + "DeviceType", "Unknown");
        }
        g_config->setOption(prefix + "DeviceNum",
                            powerpadsc[arg & 1][0].DeviceNum[0]);
        break;

    case FCFGD_GAMEPAD:
        snprintf(buf, 256, "SDL.Input.GamePad.%d", arg);
        prefix = buf;
        for(x = 0; x < 10; x++) {
            sprintf(buf, "GamePad #%d: %s", arg + 1, str[x]);
            ConfigButton(buf, &GamePadConfig[arg][x]);

            g_config->setOption(prefix + GamePadNames[x],
                                GamePadConfig[arg][x].ButtonNum[0]);
        }

        if(GamePadConfig[arg][0].ButtType[0] == BUTTC_KEYBOARD) {
            g_config->setOption(prefix + "DeviceType", "Keyboard");
        } else if(GamePadConfig[arg][0].ButtType[0] == BUTTC_JOYSTICK) {
            g_config->setOption(prefix + "DeviceType", "Joystick");
        } else {
            g_config->setOption(prefix + "DeviceType", "Unknown");
        }
        g_config->setOption(prefix + "DeviceNum",
                            GamePadConfig[arg][0].DeviceNum[0]);
        break;
    }

    ButtonConfigEnd();
}


/**
 * Update the button configuration for a device, specified by a text string.
 */
void
InputCfg(const std::string &text)
{
    if(text.find("gamepad") != std::string::npos) {
        ConfigDevice(FCFGD_GAMEPAD, (text[strlen("gamepad")] - '1') & 3);
    } else if(text.find("powerpad") != std::string::npos) {
        ConfigDevice(FCFGD_POWERPAD, (text[strlen("powerpad")] - '1') & 1);
    } else if(text.find("hypershot") != std::string::npos) {
        ConfigDevice(FCFGD_HYPERSHOT, 0);
    } else if(text.find("quizking") != std::string::npos) {
        ConfigDevice(FCFGD_QUIZKING, 0);
    }
}


/**
 * Hack to map the new configuration onto the existing button
 * configuration management.  Will probably want to change this in the
 * future - soules.
 */
void
UpdateInput(Config *config)
{
    char buf[64];
    std::string device, prefix;

    for(unsigned int i = 0; i < 3; i++) {
        snprintf(buf, 64, "SDL.Input.%d", i);
        config->getOption(buf, &device);

        if(device == "None") {
            UsrInputType[i] = SI_NONE;
        } else if(device.find("GamePad") != std::string::npos) {
            UsrInputType[i] = (i < 2) ? (int)SI_GAMEPAD : (int)SIFC_NONE;
        } else if(device.find("PowerPad.0") != std::string::npos) {
            UsrInputType[i] = (i < 2) ? (int)SI_POWERPADA : (int)SIFC_NONE;
        } else if(device.find("PowerPad.1") != std::string::npos) {
            UsrInputType[i] = (i < 2) ? (int)SI_POWERPADB : (int)SIFC_NONE;
        } else if(device.find("QuizKing") != std::string::npos) {
            UsrInputType[i] = (i < 2) ? (int)SI_NONE : (int)SIFC_QUIZKING;
        } else if(device.find("HyperShot") != std::string::npos) {
            UsrInputType[i] = (i < 2) ? (int)SI_NONE : (int)SIFC_HYPERSHOT;
        } else if(device.find("Mahjong") != std::string::npos) {
            UsrInputType[i] = (i < 2) ? (int)SI_NONE : (int)SIFC_MAHJONG;
        } else if(device.find("TopRider") != std::string::npos) {
            UsrInputType[i] = (i < 2) ? (int)SI_NONE : (int)SIFC_TOPRIDER;
        } else if(device.find("FTrainer") != std::string::npos) {
            UsrInputType[i] = (i < 2) ? (int)SI_NONE : (int)SIFC_FTRAINERA;
        } else if(device.find("FamilyKeyBoard") != std::string::npos) {
            UsrInputType[i] = (i < 2) ? (int)SI_NONE : (int)SIFC_FKB;
        } else if(device.find("OekaKids") != std::string::npos) {
            UsrInputType[i] = (i < 2) ? (int)SI_NONE : (int)SIFC_OEKAKIDS;
        } else if(device.find("Arkanoid") != std::string::npos) {
            UsrInputType[i] = (i < 2) ? (int)SI_ARKANOID : (int)SIFC_ARKANOID;
        } else if(device.find("Shadow") != std::string::npos) {
            UsrInputType[i] = (i < 2) ? (int)SI_NONE : (int)SIFC_SHADOW;
        } else if(device.find("Zapper") != std::string::npos) {
            UsrInputType[i] = (i < 2) ? (int)SI_ZAPPER : (int)SIFC_NONE;
        } else if(device.find("BWorld") != std::string::npos) {
            UsrInputType[i] = (i < 2) ? (int)SI_NONE : (int)SIFC_BWORLD;
        } else if(device.find("4Player") != std::string::npos) {
            UsrInputType[i] = (i < 2) ? (int)SI_NONE : (int)SIFC_4PLAYER;
        } else {
            // Unknown device
            UsrInputType[i] = SI_NONE;
        }
    }

    // update each of the devices' configuration structure
    // XXX soules - this is temporary until this file is cleaned up to
    //              simplify the interface between configuration and
    //              structure data.  This will likely include the
    //              removal of multiple input buttons for a single
    //              input device key.
    int type, devnum, button;

    // gamepad 0 - 3
    for(unsigned int i = 0; i < GAMEPAD_NUM_DEVICES; i++) {
        char buf[64];
        snprintf(buf, 20, "SDL.Input.GamePad.%d.", i);
        prefix = buf;

        config->getOption(prefix + "DeviceType", &device);
        if(device.find("Keyboard") != std::string::npos) {
            type = BUTTC_KEYBOARD;
        } else if(device.find("Joystick") != std::string::npos) {
            type = BUTTC_JOYSTICK;
        } else {
            type = 0;
        }

        config->getOption(prefix + "DeviceNum", &devnum);
        for(unsigned int j = 0; j < GAMEPAD_NUM_BUTTONS; j++) {
            config->getOption(prefix + GamePadNames[j], &button);

            GamePadConfig[i][j].ButtType[0] = type;
            GamePadConfig[i][j].DeviceNum[0] = devnum;
            GamePadConfig[i][j].ButtonNum[0] = button;
            GamePadConfig[i][j].NumC = 1;
        }
    }

    // PowerPad 0 - 1
    for(unsigned int i = 0; i < POWERPAD_NUM_DEVICES; i++) {
        char buf[64];
        snprintf(buf, 20, "SDL.Input.PowerPad.%d.", i);
        prefix = buf;

        config->getOption(prefix + "DeviceType", &device);
        if(device.find("Keyboard") != std::string::npos) {
            type = BUTTC_KEYBOARD;
        } else if(device.find("Joystick") != std::string::npos) {
            type = BUTTC_JOYSTICK;
        } else {
            type = 0;
        }

        config->getOption(prefix + "DeviceNum", &devnum);
        for(unsigned int j = 0; j < POWERPAD_NUM_BUTTONS; j++) {
            config->getOption(prefix + PowerPadNames[j], &button);

            powerpadsc[i][j].ButtType[0] = type;
            powerpadsc[i][j].DeviceNum[0] = devnum;
            powerpadsc[i][j].ButtonNum[0] = button;
            powerpadsc[i][j].NumC = 1;
        }
    }

    // QuizKing
    prefix = "SDL.Input.QuizKing.";
    config->getOption(prefix + "DeviceType", &device);
    if(device.find("Keyboard") != std::string::npos) {
        type = BUTTC_KEYBOARD;
    } else if(device.find("Joystick") != std::string::npos) {
        type = BUTTC_JOYSTICK;
    } else {
        type = 0;
    }
    config->getOption(prefix + "DeviceNum", &devnum);
    for(unsigned int j = 0; j < QUIZKING_NUM_BUTTONS; j++) {
        config->getOption(prefix + QuizKingNames[j], &button);

        QuizKingButtons[j].ButtType[0] = type;
        QuizKingButtons[j].DeviceNum[0] = devnum;
        QuizKingButtons[j].ButtonNum[0] = button;
        QuizKingButtons[j].NumC = 1;
    }

    // HyperShot
    prefix = "SDL.Input.HyperShot.";
    config->getOption(prefix + "DeviceType", &device);
    if(device.find("Keyboard") != std::string::npos) {
        type = BUTTC_KEYBOARD;
    } else if(device.find("Joystick") != std::string::npos) {
        type = BUTTC_JOYSTICK;
    } else {
        type = 0;
    }
    config->getOption(prefix + "DeviceNum", &devnum);
    for(unsigned int j = 0; j < HYPERSHOT_NUM_BUTTONS; j++) {
        config->getOption(prefix + HyperShotNames[j], &button);

        HyperShotButtons[j].ButtType[0] = type;
        HyperShotButtons[j].DeviceNum[0] = devnum;
        HyperShotButtons[j].ButtonNum[0] = button;
        HyperShotButtons[j].NumC = 1;
    }

    // Mahjong
    prefix = "SDL.Input.Mahjong.";
    config->getOption(prefix + "DeviceType", &device);
    if(device.find("Keyboard") != std::string::npos) {
        type = BUTTC_KEYBOARD;
    } else if(device.find("Joystick") != std::string::npos) {
        type = BUTTC_JOYSTICK;
    } else {
        type = 0;
    }
    config->getOption(prefix + "DeviceNum", &devnum);
    for(unsigned int j = 0; j < MAHJONG_NUM_BUTTONS; j++) {
        config->getOption(prefix + MahjongNames[j], &button);

        MahjongButtons[j].ButtType[0] = type;
        MahjongButtons[j].DeviceNum[0] = devnum;
        MahjongButtons[j].ButtonNum[0] = button;
        MahjongButtons[j].NumC = 1;
    }

    // TopRider
    prefix = "SDL.Input.TopRider.";
    config->getOption(prefix + "DeviceType", &device);
    if(device.find("Keyboard") != std::string::npos) {
        type = BUTTC_KEYBOARD;
    } else if(device.find("Joystick") != std::string::npos) {
        type = BUTTC_JOYSTICK;
    } else {
        type = 0;
    }
    config->getOption(prefix + "DeviceNum", &devnum);
    for(unsigned int j = 0; j < TOPRIDER_NUM_BUTTONS; j++) {
        config->getOption(prefix + TopRiderNames[j], &button);

        TopRiderButtons[j].ButtType[0] = type;
        TopRiderButtons[j].DeviceNum[0] = devnum;
        TopRiderButtons[j].ButtonNum[0] = button;
        TopRiderButtons[j].NumC = 1;
    }

    // FTrainer
    prefix = "SDL.Input.FTrainer.";
    config->getOption(prefix + "DeviceType", &device);
    if(device.find("Keyboard") != std::string::npos) {
        type = BUTTC_KEYBOARD;
    } else if(device.find("Joystick") != std::string::npos) {
        type = BUTTC_JOYSTICK;
    } else {
        type = 0;
    }
    config->getOption(prefix + "DeviceNum", &devnum);
    for(unsigned int j = 0; j < FTRAINER_NUM_BUTTONS; j++) {
        config->getOption(prefix + FTrainerNames[j], &button);

        FTrainerButtons[j].ButtType[0] = type;
        FTrainerButtons[j].DeviceNum[0] = devnum;
        FTrainerButtons[j].ButtonNum[0] = button;
        FTrainerButtons[j].NumC = 1;
    }

    // FamilyKeyBoard
    prefix = "SDL.Input.FamilyKeyBoard.";
    config->getOption(prefix + "DeviceType", &device);
    if(device.find("Keyboard") != std::string::npos) {
        type = BUTTC_KEYBOARD;
    } else if(device.find("Joystick") != std::string::npos) {
        type = BUTTC_JOYSTICK;
    } else {
        type = 0;
    }
    config->getOption(prefix + "DeviceNum", &devnum);
    for(unsigned int j = 0; j < FAMILYKEYBOARD_NUM_BUTTONS; j++) {
        config->getOption(prefix + FamilyKeyBoardNames[j], &button);

        fkbmap[j].ButtType[0] = type;
        fkbmap[j].DeviceNum[0] = devnum;
        fkbmap[j].ButtonNum[0] = button;
        fkbmap[j].NumC = 1;
    }
}

// Definitions from main.h:
// GamePad defaults
const char *GamePadNames[GAMEPAD_NUM_BUTTONS] =
    {"A", "B", "Select", "Start",
     "Up", "Down", "Left", "Right", "TurboA", "TurboB"};
const char *DefaultGamePadDevice[GAMEPAD_NUM_DEVICES] =
    {"Keyboard", "None", "None", "None"};
const int DefaultGamePad[GAMEPAD_NUM_DEVICES][GAMEPAD_NUM_BUTTONS] =
    { { SDLK_j, SDLK_k, SDLK_TAB, SDLK_RETURN,
        SDLK_w, SDLK_s, SDLK_a, SDLK_d, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } };

// PowerPad defaults
const char *PowerPadNames[POWERPAD_NUM_BUTTONS] =
    {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B"};
const char *DefaultPowerPadDevice[POWERPAD_NUM_DEVICES] =
    {"Keyboard", "None"};
const int DefaultPowerPad[POWERPAD_NUM_DEVICES][POWERPAD_NUM_BUTTONS] =
    { { SDLK_o, SDLK_p, SDLK_LEFTBRACKET, SDLK_RIGHTBRACKET,
        SDLK_k, SDLK_l, SDLK_SEMICOLON, SDLK_QUOTE,
        SDLK_m, SDLK_COMMA, SDLK_PERIOD, SDLK_SLASH },
      { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } };

// QuizKing defaults
const char *QuizKingNames[QUIZKING_NUM_BUTTONS] =
    { "0", "1", "2", "3", "4", "5" };
const char *DefaultQuizKingDevice = "Keyboard";
const int DefaultQuizKing[QUIZKING_NUM_BUTTONS] =
    { SDLK_q, SDLK_w, SDLK_e, SDLK_r, SDLK_t, SDLK_y };

// HyperShot defaults
const char *HyperShotNames[HYPERSHOT_NUM_BUTTONS] =
    { "0", "1", "2", "3" };
const char *DefaultHyperShotDevice = "Keyboard";
const int DefaultHyperShot[HYPERSHOT_NUM_BUTTONS] =
    { SDLK_q, SDLK_w, SDLK_e, SDLK_r };

// Mahjong defaults
const char *MahjongNames[MAHJONG_NUM_BUTTONS] =
    { "00", "01", "02", "03", "04", "05", "06", "07",
      "08", "09", "10", "11", "12", "13", "14", "15",
      "16", "17", "18", "19", "20" };
const char *DefaultMahjongDevice = "Keyboard";
const int DefaultMahjong[MAHJONG_NUM_BUTTONS] =
    { SDLK_q, SDLK_w, SDLK_e, SDLK_r, SDLK_t, SDLK_a, SDLK_s, SDLK_d,
      SDLK_f, SDLK_g, SDLK_h, SDLK_j, SDLK_k, SDLK_l, SDLK_z, SDLK_x,
      SDLK_c, SDLK_v, SDLK_b, SDLK_n, SDLK_m };

// TopRider defaults
const char *TopRiderNames[TOPRIDER_NUM_BUTTONS] =
    { "0", "1", "2", "3", "4", "5", "6", "7" };
const char *DefaultTopRiderDevice = "Keyboard";
const int DefaultTopRider[TOPRIDER_NUM_BUTTONS] =
    { SDLK_q, SDLK_w, SDLK_e, SDLK_r, SDLK_t, SDLK_y, SDLK_u, SDLK_i };

// FTrainer defaults
const char *FTrainerNames[FTRAINER_NUM_BUTTONS] =
    { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "A", "B" };
const char *DefaultFTrainerDevice = "Keyboard";
const int DefaultFTrainer[FTRAINER_NUM_BUTTONS] =
    { SDLK_o, SDLK_p, SDLK_LEFTBRACKET, SDLK_RIGHTBRACKET,
      SDLK_k, SDLK_l, SDLK_SEMICOLON, SDLK_QUOTE,
      SDLK_m, SDLK_COMMA, SDLK_PERIOD, SDLK_SLASH };

// FamilyKeyBoard defaults
const char *FamilyKeyBoardNames[FAMILYKEYBOARD_NUM_BUTTONS] =
    { "F1", "F2", "F3", "F4", "F5", "F6", "F7", "F8",
      "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
      "MINUS", "EQUAL", "BACKSLASH", "BACKSPACE",
      "ESCAPE", "Q", "W", "E", "R", "T", "Y", "U", "I", "O",
      "P", "GRAVE", "BRACKET_LEFT", "ENTER",
      "LEFTCONTROL", "A", "S", "D", "F", "G", "H", "J", "K",
      "L", "SEMICOLON", "APOSTROPHE", "BRACKET_RIGHT", "INSERT",
      "LEFTSHIFT", "Z", "X", "C", "V", "B", "N", "M", "COMMA",
      "PERIOD", "SLASH", "RIGHTALT", "RIGHTSHIFT", "LEFTALT", "SPACE",
      "DELETE", "END", "PAGEDOWN",
      "CURSORUP", "CURSORLEFT", "CURSORRIGHT", "CURSORDOWN" };
const char *DefaultFamilyKeyBoardDevice = "Keyboard";
const int DefaultFamilyKeyBoard[FAMILYKEYBOARD_NUM_BUTTONS] =
    { SDLK_F1, SDLK_F2, SDLK_F3, SDLK_F4, SDLK_F5, SDLK_F6, SDLK_F7, SDLK_F8,
      SDLK_1, SDLK_2, SDLK_3, SDLK_4, SDLK_5,
      SDLK_6, SDLK_7, SDLK_8, SDLK_9, SDLK_0,
      SDLK_MINUS, SDLK_EQUALS, SDLK_BACKSLASH, SDLK_BACKSPACE,
      SDLK_ESCAPE, SDLK_q, SDLK_w, SDLK_e, SDLK_r, SDLK_t, SDLK_y, SDLK_u,
      SDLK_i, SDLK_o, SDLK_p, SDLK_BACKQUOTE, SDLK_LEFTBRACKET, SDLK_RETURN,
      SDLK_LCTRL, SDLK_a, SDLK_s, SDLK_d, SDLK_f, SDLK_g, SDLK_h, SDLK_j,
      SDLK_k, SDLK_l, SDLK_SEMICOLON, SDLK_QUOTE, SDLK_RIGHTBRACKET,
      SDLK_INSERT, SDLK_LSHIFT, SDLK_z, SDLK_x, SDLK_c, SDLK_v, SDLK_b,
      SDLK_n, SDLK_m, SDLK_COMMA, SDLK_PERIOD, SDLK_SLASH, SDLK_RALT,
      SDLK_RSHIFT, SDLK_LALT, SDLK_SPACE, SDLK_DELETE, SDLK_END, SDLK_PAGEDOWN,
      SDLK_UP, SDLK_LEFT, SDLK_RIGHT, SDLK_DOWN };
