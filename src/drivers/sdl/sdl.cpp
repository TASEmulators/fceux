#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>


#include "main.h"
#include "throttle.h"
#include "config.h"

#include "../common/cheat.h"

#include "input.h"
#include "dface.h"

#include "sdl.h"
#include "sdl-video.h"
#include "unix-netplay.h"

#ifdef WIN32
#include <windows.h>
#endif


/**
 * Unimplemented.  Returns 0.
 */
int
InitMouse(void)
{
    return 0;
}

/**
 * Unimplemented.
 */
void
KillMouse()
{ }

/**
 * Return the state of the mouse buttons.  Input 'd' is an array of 3
 * integers that store <x, y, button state>.
 */
void
GetMouseData(uint32 *d)
{
    int x,y;
    uint32 t;

    // XXX soules - why don't we do this when a movie is active?
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
 * Unimplemented.  Returns 1.
 */
int
InitKeyboard()
{
    return 1;
}

/**
 * Unimplemented.  Returns 1.
 */
int
UpdateKeyboard()
{
    return 1;
}

/**
 * Unimplemented.
 */
void
KillKeyboard()
{

}


/**
 * Handles outstanding SDL events.
 */
void
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

/**
 * Get and return the keyboard state from the SDL.
 */
static uint8 *KeyState = NULL;
char *
GetKeyboard()
{
    KeyState = SDL_GetKeyState(0);
    return ((char *)KeyState);
}


#ifdef OPENGL
int sdlhaveogl;
#endif


extern int32 fps_scale;

int CloseGame(void);

int soundvol=100;
long soundq=0;
int _sound=1;
long soundrate=48000;
#ifdef WIN32
long soundbufsize=52;
#else
long soundbufsize=24;
#endif

static int inited=0;
static int isloaded=0;	// Is game loaded?

int srendlinev[2]={8,0};
int erendlinev[2]={231,239};


int eoptions=0;

static void DriverKill(void);
static int DriverInitialize(FCEUGI *gi);
int gametype = 0;

FCEUGI *CurGame=NULL;

/**
 * Wrapper for ParseGIInput().
 */
static void
ParseGI(FCEUGI *gi)
{
    ParseGIInput(gi);
    gametype = gi->type;
}

/**
 * Prints an error string to STDOUT.
 */
void
FCEUD_PrintError(char *s)
{
    puts(s);
}

/**
 * Prints the given string to STDOUT.
 */
void FCEUD_Message(char *s)
{
    fputs(s, stdout);
}


#ifndef WIN32
/**
 * Capture and handle signals sent to FCEU.
 */
static void
SetSignals(void (*t)(int))
{
    // XXX soules - why do we capture these?  Seems unnecessary.
    int sigs[11]={SIGINT,SIGTERM,SIGHUP,SIGPIPE,SIGSEGV,SIGFPE,SIGKILL,SIGALRM,SIGABRT,SIGUSR1,SIGUSR2};
    int x;
    for(x = 0; x < 11; x++) {
        signal(sigs[x], t);
    }
}

static void
CloseStuff(int signum)
{
    // XXX soules - again, not clear why this is necessary
    DriverKill();
    printf("\nSignal %d has been caught and dealt with...\n",signum);
    switch(signum) {
    case SIGINT:printf("How DARE you interrupt me!\n");break;
    case SIGTERM:printf("MUST TERMINATE ALL HUMANS\n");break;
    case SIGHUP:printf("Reach out and hang-up on someone.\n");break;
    case SIGPIPE:printf("The pipe has broken!  Better watch out for floods...\n");break;
    case SIGSEGV:printf("Iyeeeeeeeee!!!  A segmentation fault has occurred.  Have a fluffy day.\n");break;
        /* So much SIGBUS evil. */
#ifdef SIGBUS
#if(SIGBUS!=SIGSEGV)
    case SIGBUS:printf("I told you to be nice to the driver.\n");break;
#endif
#endif
    case SIGFPE:printf("Those darn floating points.  Ne'er know when they'll bite!\n");break;
    case SIGALRM:printf("Don't throw your clock at the meowing cats!\n");break;
    case SIGABRT:printf("Abort, Retry, Ignore, Fail?\n");break;
    case SIGUSR1:
    case SIGUSR2:printf("Killing your processes is not nice.\n");break;
    }
    exit(1);
}
#endif


#include "usage.h"

/**
 * Loads a game, given a full path/filename.  The driver code must be
 * initialized after the game is loaded, because the emulator code
 * provides data necessary for the driver code(number of scanlines to
 * render, what virtual input devices to use, etc.).
 */
int LoadGame(const char *path)
{
    FCEUGI *tmp;

    CloseGame();
    if(!(tmp = FCEUI_LoadGame(path, 1))) {
        return 0;
    }
    CurGame=tmp;
    ParseGI(tmp);
    RefreshThrottleFPS();

    if(!DriverInitialize(tmp)) {
        return(0);
    }
    if(soundrecfn) {
        if(!FCEUI_BeginWaveRecord(soundrecfn)) {
            free(soundrecfn);
            soundrecfn=0;
        }
    }
    isloaded=1;

    FCEUD_NetworkConnect();
    return 1;
}

/**
 * Closes a game.  Frees memory, and deinitializes the drivers.
 */
int
CloseGame()
{
    if(!isloaded) {
        return(0);
    }
    FCEUI_CloseGame();
    DriverKill();
    isloaded=0;
    CurGame=0;

    if(soundrecfn) {
        FCEUI_EndWaveRecord();
    }

    InputUserActiveFix();
    return(1);
}

void FCEUD_Update(uint8 *XBuf, int32 *Buffer, int Count);

void DoFun(void)
{
    uint8 *gfx;  
    int32 *sound;
    int32 ssize;
    static int fskipc=0;
    static int opause=0;
         
#ifdef FRAMESKIP
    fskipc=(fskipc+1)%(frameskip+1);
#endif
         
    if(NoWaiting) {
        gfx = 0;
    }
    FCEUI_Emulate(&gfx, &sound, &ssize, fskipc);
    FCEUD_Update(gfx, sound, ssize);

    if(opause!=FCEUI_EmulationPaused()) {
        opause=FCEUI_EmulationPaused();
        SilenceSound(opause);
    }
}


/**
 * Initialize all of the subsystem drivers: video, audio, joystick,
 * keyboard, mouse.
 */
static int
DriverInitialize(FCEUGI *gi)
{
#ifndef WIN32
    SetSignals(CloseStuff);
#endif

    /* Initialize video before all else, due to some wacko dependencies
       in the SexyAL code(DirectSound) that need to be fixed.
    */

    if(InitVideo(gi) < 0) return 0;
    inited|=4;

    if(InitSound(gi))
        inited|=1;

    if(InitJoysticks())
        inited|=2;

    if(!InitKeyboard()) return 0;
    inited|=8;

    InitOtherInput();
    return 1;
}

/**
 * Shut down all of the subsystem drivers: video, audio, joystick,
 * keyboard.
 */
static void
DriverKill()
{
    SaveConfig();

#ifndef WIN32
    SetSignals(SIG_IGN);
#endif

    if(inited&2)
        KillJoysticks();
    if(inited&8)
        KillKeyboard();
    if(inited&4)
        KillVideo();
    if(inited&1)
        KillSound();
    if(inited&16)
        KillMouse();
    inited=0;
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
    extern int FCEUDnetplay;

    int ocount = Count;
    // apply frame scaling to Count
    Count = (Count<<8)/fps_scale;
    if(Count) {
        int32 can=GetWriteSound();
        static int uflow=0;
        int32 tmpcan;

        // don't underflow when scaling fps
        if(can >= GetMaxSound() && fps_scale<=256) uflow=1;	/* Go into massive underflow mode. */

        if(can > Count) can=Count;
        else uflow=0;

        WriteSound(Buffer,can);

        //if(uflow) puts("Underflow");
        tmpcan = GetWriteSound();
        // don't underflow when scaling fps
        if(fps_scale>256 || ((tmpcan < Count*0.90) && !uflow)) {
            if(XBuf && (inited&4) && !(NoWaiting & 2))
                BlitScreen(XBuf);
            Buffer+=can;
            Count-=can;
            if(Count) {
                if(NoWaiting) {
                    can=GetWriteSound(); 
                    if(Count>can) Count=can;
                    WriteSound(Buffer,Count);
                } else {
                    while(Count>0) {
                        WriteSound(Buffer,(Count<ocount) ? Count : ocount);
                        Count -= ocount;
                    }
                }
            }
        } //else puts("Skipped");
        else if(!NoWaiting && FCEUDnetplay && (uflow || tmpcan >= (Count * 1.8))) {
            if(Count > tmpcan) Count=tmpcan;
            while(tmpcan > 0) {
                //    printf("Overwrite: %d\n", (Count <= tmpcan)?Count : tmpcan);
                WriteSound(Buffer, (Count <= tmpcan)?Count : tmpcan);
                tmpcan -= Count;
            }
        }

    } else {
        if(!NoWaiting && (!(eoptions&EO_NOTHROTTLE) || FCEUI_EmulationPaused()))
            SpeedThrottle();
        if(XBuf && (inited&4)) {
            BlitScreen(XBuf);
        }
    }
    FCEUD_UpdateInput();
    //if(!Count && !NoWaiting && !(eoptions&EO_NOTHROTTLE))
    // SpeedThrottle();
    //if(XBuf && (inited&4))
    //{
    // BlitScreen(XBuf);
    //}
    //if(Count)
    // WriteSound(Buffer,Count,NoWaiting);
    //FCEUD_UpdateInput();
}


/* Maybe ifndef WXWINDOWS would be better? ^_^ */
/**
 * Opens a file to be read a byte at a time.
 */
FILE *FCEUD_UTF8fopen(const char *fn, const char *mode)
{
    return(fopen(fn,mode));
}

static char *s_linuxCompilerString = "g++ " __VERSION__;
/**
 * Returns the compiler string.
 */
char *FCEUD_GetCompilerString() {
    return (char *)s_linuxCompilerString;
}

/**
 * Unimplemented.
 */
void FCEUD_DebugBreakpoint() {
    return;
}

/**
 * Unimplemented.
 */
void FCEUD_TraceInstruction() {
    return;
}


/**
 * Involved with updating the button configuration, but not entirely
 * sure how yet - soules.
 */
int DTestButton(ButtConfig *bc)
{
    int x;

    for(x = 0; x < bc->NumC; x++) {
        if(bc->ButtType[x] == BUTTC_KEYBOARD) {
            if(KeyState[bc->ButtonNum[x]]) {
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

static int bcpv,bcpj;

/**
 *  Begin configuring the buttons by placing the video and joystick
 *  subsystems into a well-known state.  Button configuration really
 *  needs to be cleaned up after the new config system is in place.
 */
int
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
void
ButtonConfigEnd()
{ 
    extern FCEUGI *CurGame;

    // shutdown the joystick and video subsystems
    KillJoysticks();
    SDL_QuitSubSystem(SDL_INIT_VIDEO); 

    // re-initialize joystick and video subsystems if they were active before
    if(!bcpv) {
        InitVideo(CurGame);
    }
    if(!bcpj) {
        InitJoysticks();
    }
}

/**
 * Waits for a button input and returns the information as to which
 * button was pressed.  Used in button configuration.
 */
int
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
 * The main loop for the SDL.
 */
int
main(int argc,
     char *argv[])
{
    FCEUD_Message("\nStarting "FCEU_NAME_AND_VERSION"...\n");

#ifdef WIN32
    /* Taken from win32 sdl_main.c */
    SDL_SetModuleHandle(GetModuleHandle(NULL));
#endif

    /* SDL_INIT_VIDEO Needed for (joystick config) event processing? */
    if(SDL_Init(SDL_INIT_VIDEO)) {
        printf("Could not initialize SDL: %s.\n", SDL_GetError());
        return(-1);
    }

#ifdef OPENGL
#ifdef APPLEOPENGL
    sdlhaveogl = 1;	/* Stupid something...  Hack. */
#else
    if(!SDL_GL_LoadLibrary(0)) sdlhaveogl=1;
    else sdlhaveogl=0;
#endif
#endif

    SetDefaults();

    int error;

    // initialize the infrastructure
    error = FCEUI_Initialize();
    if(error != 1) {
        SDL_Quit();
        return -1;
    }

    // XXX configuration initialization
    error = InitConfig(argc, argv);
    if(error) {
        SDL_Quit();
        return -1;
    }

    // load the specified game
    error = LoadGame(argv[argc - 1]);
    if(error != 1) {
        DriverKill();
        SDL_Quit();
        return -1;
    }

    // loop playing the game
    while(CurGame) {
        DoFun();
    }
    CloseGame();

    // save the configuration information
    SaveConfig();

    // exit the infrastructure
    FCEUI_Kill();

    SDL_Quit();
    return 0;
}


/**
 * Get the time in ticks.
 */
uint64
FCEUD_GetTime()
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


// dummy functions

#define DUMMY(__f) void __f(void) {printf("%s\n", __f); FCEU_DispMessage("Not implemented.");}
DUMMY(FCEUD_HideMenuToggle)
DUMMY(FCEUD_TurboOn)
DUMMY(FCEUD_TurboOff)
DUMMY(FCEUD_SaveStateAs)
DUMMY(FCEUD_LoadStateFrom)
DUMMY(FCEUD_MovieRecordTo)
DUMMY(FCEUD_MovieReplayFrom)
DUMMY(FCEUD_ToggleStatusIcon)
DUMMY(FCEUD_AviRecordTo)
DUMMY(FCEUD_AviStop)
void FCEUI_AviVideoUpdate(const unsigned char* buffer) { } 
int FCEUD_ShowStatusIcon(void) {return 0;} 
int FCEUI_AviIsRecording(void) {return 0;}

