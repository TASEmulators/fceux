#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sdl.h"
#include "sdl-video.h"
#include "unix-netplay.h"

#ifdef WIN32
#include <windows.h>
#endif

DSETTINGS Settings;
CFGSTRUCT DriverConfig[]={
	#ifdef OPENGL
	AC(_stretchx),
	AC(_stretchy),
	AC(_opengl),
	AC(_openglip),
	#endif
	AC(Settings.special),
	AC(Settings.specialfs),
	AC(_doublebuf),
	AC(_xscale),
	AC(_yscale),
	AC(_xscalefs),
	AC(_yscalefs),
	AC(_bpp),
	AC(_efx),
	AC(_efxfs),
	AC(_fullscreen),
        AC(_xres),
	AC(_yres),
        ACS(netplaynick),
	AC(netlocalplayers),
	AC(tport),
	ACS(netpassword),
	ACS(netgamekey),
        ENDCFGSTRUCT
};

//-fshack x       Set the environment variable SDL_VIDEODRIVER to \"x\" when
//                entering full screen mode and x is not \"0\".

char *DriverUsage=
"-xres   x	Set horizontal resolution to x for full screen mode.\n\
-yres   x       Set vertical resolution to x for full screen mode.\n\
-xscale(fs) x	Multiply width by x(Real numbers >0 with OpenGL, otherwise integers >0).\n\
-yscale(fs) x	Multiply height by x(Real numbers >0 with OpenGL, otherwise integers >0).\n\
-bpp(fs) x	Bits per pixel for SDL surface(and video mode in fs). 8, 16, 32.\n\
-opengl x	Enable OpenGL support if x is 1.\n\
-openglip x	Enable OpenGL linear interpolation if x is 1.\n\
-doublebuf x	\n\
-special(fs) x	Specify special scaling filter.\n\
-stretch(x/y) x	Stretch to fill surface on x or y axis(fullscreen, only with OpenGL).\n\
-efx(fs) x	Enable special effects.  Logically OR the following together:\n\
		 1 = scanlines(for yscale>=2).\n\
		 2 = TV blur(for bpp of 16 or 32).\n\
-fs	 x      Select full screen mode if x is non zero.\n\
-connect s      Connect to server 's' for TCP/IP network play.\n\
-netnick s	Set the nickname to use in network play.\n\
-netgamekey s 	Use key 's' to create a unique session for the game loaded.\n\
-netpassword s	Password to use for connecting to the server.\n\
-netlocalplayers x	Set the number of local players.\n\
-netport x      Use TCP/IP port x for network play.";

ARGPSTRUCT DriverArgs[]={
	#ifdef OPENGL
	 {"-opengl",0,&_opengl,0},
	 {"-openglip",0,&_openglip,0},
	 {"-stretchx",0,&_stretchx,0},
	 {"-stretchy",0,&_stretchy,0},
	#endif
	 {"-special",0,&Settings.special,0},
	 {"-specialfs",0,&Settings.specialfs,0},
	 {"-doublebuf",0,&_doublebuf,0},
	 {"-bpp",0,&_bpp,0},
	 {"-xscale",0,&_xscale,2},
	 {"-yscale",0,&_yscale,2},
	 {"-efx",0,&_efx,0},
         {"-xscalefs",0,&_xscalefs,2},
         {"-yscalefs",0,&_yscalefs,2},
         {"-efxfs",0,&_efxfs,0},
	 {"-xres",0,&_xres,0},
         {"-yres",0,&_yres,0},
         {"-fs",0,&_fullscreen,0},
         //{"-fshack",0,&_fshack,0x4001},
         {"-connect",0,&netplayhost,0x4001},
         {"-netport",0,&tport,0},
	 {"-netlocalplayers",0,&netlocalplayers,0},
	 {"-netnick",0,&netplaynick,0x4001},
	 {"-netpassword",0,&netpassword,0x4001},
         {0,0,0,0}
};

static void SetDefaults(void)
{
 Settings.special=Settings.specialfs=0;
 _bpp=8;
 _xres=640;
 _yres=480;
 _fullscreen=0;
 _xscale=2.50;
 _yscale=2;
 _xscalefs=_yscalefs=2;
 _efx=_efxfs=0;
 //_fshack=_fshacksave=0;
#ifdef OPENGL
 _opengl=1;
 _stretchx=1; 
 _stretchy=0;
 _openglip=1;
#endif
}

/**
 * Unimplemented.
 */
void DoDriverArgs(void)
{

}

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

/**
 * Attempts to locate FCEU's application directory.  This will
 * hopefully become obsolete once the new configuration system is in
 * place.
 */
uint8 *
GetBaseDirectory()
{
    uint8 *ol;
    uint8 *ret; 

    ol=(uint8 *)getenv("HOME");

    if(ol) {
        ret=(uint8 *)malloc(strlen((char *)ol)+1+strlen("./fceultra"));
        strcpy((char *)ret,(char *)ol);
        strcat((char *)ret,"/.fceultra");
    } else {
#ifdef WIN32
        char *sa;

        ret=(uint8*)malloc(MAX_PATH+1);
        GetModuleFileName(NULL,(char*)ret,MAX_PATH+1);

        sa=strrchr((char*)ret,'\\');
        if(sa)
            *sa = 0; 
#else
        ret=(uint8 *)malloc(sizeof(uint8));
        ret[0]=0;
#endif
        printf("%s\n",ret);
    }
    return(ret);
}

#ifdef OPENGL
int sdlhaveogl;
#endif


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

    {
        int ret = CLImain(argc, argv);
        SDL_Quit();
        return (ret) ? 0 : -1;
    }
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

#define DUMMY(f) void f(void) {FCEU_DispMessage("Not implemented.");}
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
void FCEUI_AviVideoUpdate(const unsigned char* buffer) {FCEU_DispMessage("Not implemented.");} 
int FCEUD_ShowStatusIcon(void) {return 0;} 
int FCEUI_AviIsRecording(void) {return 0;}

