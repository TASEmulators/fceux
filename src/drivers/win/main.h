#ifndef WIN_MAIN_H
#define WIN_MAIN_H

#include "common.h"
#include <string>

// #defines

#define MAX(x,y) ((x)<(y)?(y):(x))
#define MIN(x,y) ((x)>(y)?(y):(x))

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

extern int maxconbskip;
extern int ffbskip;

static int fullscreen = 0;

// Flag that indicates whether Game Genie is enabled or not.
extern int genie;

// Flag that indicates whether PAL Emulation is enabled or not.
extern int pal_emulation;
extern int status_icon;
extern int frame_display;
extern int input_display;
extern int allowUDLR;
extern int pauseAfterPlayback;
extern int EnableBackgroundInput;
extern int AFon;
extern int AFoff;
extern int AutoFireOffset;

extern int vmod;

extern char* directory_names[13];

///Contains the names of the default directories.
static const char *default_directory_names[12] = {
	"",         // roms
	"sav",      // nonvol
	"fcs",      // states
	"",         // fdsrom
	"snaps",    // snaps
	"cheats",   // cheats
	"movies",   // movies
	"tools",    // memwatch
	"tools",    // macro
	"tools",    // input presets
	"tools"     // lua scripts
};

#define NUMBER_OF_DIRECTORIES sizeof(directory_names) / sizeof(*directory_names)
#define NUMBER_OF_DEFAULT_DIRECTORIES sizeof(default_directory_names) / sizeof(*default_directory_names)

extern double saspectw, saspecth;
extern double winsizemulx, winsizemuly;

extern int ismaximized;
extern int soundoptions;
extern int soundrate;
extern int soundbuftime;
extern int soundvolume;
extern int soundquality;

extern uint8 cpalette[192];
extern int srendlinen;
extern int erendlinen;
extern int srendlinep;
extern int erendlinep;

extern int ntsccol, ntsctint, ntschue;

//mbg merge 7/17/06 did these have to be unsigned?
//static int srendline, erendline;

static int changerecursive=0;

/// Contains the base directory of FCE
extern std::string BaseDirectory;

extern int soundo;
extern int eoptions;
extern int soundoptions;
extern uint8 *xbsave;
extern HRESULT ddrval;
extern int windowedfailed;
extern uint32 goptions;

void DoFCEUExit();
void ShowAboutBox();
int BlockingCheck();
void DoPriority();
void RemoveDirs();
void CreateDirs();
void SetDirs();
void initDirectories();

#endif
