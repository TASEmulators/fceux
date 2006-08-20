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

#include "main.h"
#include "dface.h"
#include "input.h"

#include "sdl-video.h"

#include "../common/cheat.h"
#include "../../fceu.h"

/** GLOBALS **/
int NoWaiting=1;


/* UsrInputType[] is user-specified.  InputType[] is current
        (game loading can override user settings)
*/
static int UsrInputType[3] = {SI_GAMEPAD, SI_GAMEPAD, SIFC_NONE};
static int InputType[3]    = {0, 0, 0};
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

/**
 * Parse keyboard commands and execute accordingly.
 */
static void
KeyboardCommands()
{
    int is_shift, is_alt;

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

    // f4 controls rendering
    if(keyonly(F4)) {
        if(is_shift) {
            FCEUI_SetRenderDisable(-1, 2);
        } else {
            FCEUI_SetRenderDisable(2, -1);
        }
    }

    // Alt-Enter to toggle full-screen
    if(keyonly(ENTER) && is_alt) {
        ToggleFS();
    }

    // Toggle throttling
    NoWaiting &= ~1;
    if(KEY(GRAVE)) {
        NoWaiting |= 1;
    }

    // Famicom disk-system games
    if(gametype==GIT_FDS) {
        if(keyonly(F6)) {
            FCEUI_FDSSelect();
        }
        if(keyonly(F8)) {
            FCEUI_FDSInsert();
        }
    }

    // f9 is save snapshot key
    if(keyonly(F9)) {
        FCEUI_SaveSnapshot();
    }

    // if not NES Sound Format
    if(gametype != GIT_NSF) {
        // f2 to enable cheats
        if(keyonly(F2)) {
            DoCheatSeq();
        }

        // f5 to save state, Shift-f5 to save movie
        if(keyonly(F5)) {
            if(is_shift) {
                FCEUI_SaveMovie(NULL,0,NULL);
            } else {
                FCEUI_SaveState(NULL);
            }
        }

        // f7 to load state, Shift-f7 to load movie
        if(keyonly(F7)) {
            if(is_shift) {
                //mbg merge 7/23/06 loadmovie takes another arg now
                FCEUI_LoadMovie(NULL,0,0);
            } else {
                FCEUI_LoadState(NULL);
            }
        }
    }

    // f1 to toggle tile view
    if(keyonly(F1)) {
        FCEUI_ToggleTileView();
    }

    // - to decrease speed, = to increase speed
    if(keyonly(MINUS)) {
        DecreaseEmulationSpeed();
    }
    if(keyonly(EQUAL)) {
        IncreaseEmulationSpeed();
    }

    if(keyonly(BACKSPACE)) {
        FCEUI_MovieToggleFrameDisplay();
    }
    if(keyonly(BACKSLASH)) {
        FCEUI_ToggleEmulationPause();
    }
    if(keyonly(RIGHTCONTROL)) {
        FCEUI_FrameAdvance();
    }

    // f10 reset, f11 power
    if(keyonly(F10)) {
        FCEUI_ResetNES();
    }
    if(keyonly(F11)) {
        FCEUI_PowerNES();
    }

    // F12 or Esc close game
    if(KEY(F12) || KEY(ESCAPE)) {
        CloseGame();
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
    } else {                                      \
        if(is_shift) {                            \
            FCEUI_SelectMovie(x,1);               \
        } else {                                  \
            FCEUI_SelectState(x,1);               \
	}                                         \
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
static void
GetMouseData(uint32 *d)
{
    int x,y;
    uint32 t;

    // Don't get input when a movie is playing back
    if(FCEUI_IsMovieActive() < 0)
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
            CloseGame();
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
    screen = SDL_SetVideoMode(300, 1, 8, 0); 
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
    if(FCEUI_IsMovieActive() < 0) {
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
    if(FCEUI_IsMovieActive() < 0) {
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
        FCEUI_SetInput(x, InputType[x], InputDPtr, attrib);
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

    FCEUI_SetInputFC(InputType[2], InputDPtr, attrib);
    FCEUI_DisableFourScore(eoptions & EO_NOFOURSCORE);
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

    SDL_WM_SetCaption((const char *)text,0);
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
void
ConfigDevice(int which,
             int arg)
{
    uint8 buf[256];
    int x;
    char *str[10]={"A","B","SELECT","START","UP","DOWN","LEFT","RIGHT","Rapid A","Rapid B"};

    ButtonConfigBegin();
    switch(which) {
    case FCFGD_QUIZKING:
        for(x = 0; x < 6; x++) {
            sprintf((char *)buf, "Quiz King Buzzer #%d", x+1);
            ConfigButton((char *)buf, &QuizKingButtons[x]);
        }
        break;
    case FCFGD_HYPERSHOT:
        for(x = 0; x < 4; x++) {
            sprintf((char *)buf, "Hyper Shot %d: %s",
                    ((x & 2) >> 1) + 1, (x & 1) ? "JUMP" : "RUN");
            ConfigButton((char *)buf, &HyperShotButtons[x]);
        }
        break;
    case FCFGD_POWERPAD:
        for(x = 0; x < 12; x++) {
            sprintf((char *)buf, "PowerPad %d: %d", (arg & 1) + 1, x + 11);
            ConfigButton((char *)buf, &powerpadsc[arg&1][x]);
        }
        break;

    case FCFGD_GAMEPAD:
        for(x = 0; x < 10; x++) {
            sprintf((char *)buf, "GamePad #%d: %s", arg + 1, str[x]);
            ConfigButton((char *)buf, &GamePadConfig[arg][x]);
        }
        break;
    }

    ButtonConfigEnd();
}


/**
 * Update the button configuration for a device, specified by a text string.
 */
static void
InputCfg(char *text)
{
    if(!strncasecmp(text, "gamepad", strlen("gamepad"))) {
        ConfigDevice(FCFGD_GAMEPAD, (text[strlen("gamepad")] - '1') & 3);
    } else if(!strncasecmp(text, "powerpad", strlen("powerpad"))) {
        ConfigDevice(FCFGD_POWERPAD, (text[strlen("powerpad")] - '1') & 1);
    } else if(!strcasecmp(text,"hypershot")) {
        ConfigDevice(FCFGD_HYPERSHOT, 0);
    } else if(!strcasecmp(text,"quizking")) {
        ConfigDevice(FCFGD_QUIZKING, 0);
    }
}

/**
 * Specify a FamiCom Expansion device as the 3rd input device.  Takes
 * a text string describing the device.
 */
static void
FCExp(char *text)
{
    static char *fccortab[11]={"none","arkanoid","shadow","4player","fkb","hypershot",
                               "mahjong","quizking","ftrainera","ftrainerb","oekakids"};
           
    static int fccortabi[11]={SIFC_NONE,SIFC_ARKANOID,SIFC_SHADOW,
                              SIFC_4PLAYER,SIFC_FKB,SIFC_HYPERSHOT,SIFC_MAHJONG,SIFC_QUIZKING,
                              SIFC_FTRAINERA,SIFC_FTRAINERB,SIFC_OEKAKIDS};
 
    int y;
    for(y = 0; y < 11; y++) {
        if(!strcmp(fccortab[y], text)) {
            UsrInputType[2] = fccortabi[y];
        }
    }
}

static char *cortab[6]={"none","gamepad","zapper","powerpada","powerpadb","arkanoid"};
static int cortabi[6]={SI_NONE,SI_GAMEPAD,
                               SI_ZAPPER,SI_POWERPADA,SI_POWERPADB,SI_ARKANOID};

/**
 * Set the 1st user-specified input device.  Specified as a text
 * string.
 */
static void
Input1(char *text)
{
    int y;

    for(y = 0; y < 6; y++) {
        if(!strcmp(cortab[y], text)) {
            UsrInputType[0] = cortabi[y];
        }
    }
}

/**
 * Set the 2nd user-specified input device.  Specified as a text
 * string.
 */
static void
Input2(char *text)
{
    int y;

    for(y = 0; y < 6; y++) {
        if(!strcmp(cortab[y], text)) {
            UsrInputType[1] = cortabi[y];
        }
    }
}

/** GLOBALS **/

CFGSTRUCT InputConfig[]={
        ACA(UsrInputType),
        AC(powerpadsc),
        AC(QuizKingButtons),
        AC(FTrainerButtons),
        AC(HyperShotButtons),
        AC(MahjongButtons),
        AC(GamePadConfig),
        AC(fkbmap),
        ENDCFGSTRUCT
};

ARGPSTRUCT InputArgs[]={
	{"-inputcfg",0,(void *)InputCfg,0x2000},
	{"-fcexp",0,(void *)FCExp,0x2000},
	{"-input1",0,(void *)Input1,0x2000},
	{"-input2",0,(void *)Input2,0x2000},
	{0,0,0,0}
};

