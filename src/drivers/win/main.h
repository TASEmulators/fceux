#ifndef WIN_MAIN_H
#define WIN_MAIN_H

#include "common.h"

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

static int moviereadonly = 1;

static int fullscreen = 0;
static int soundflush = 0;
// Flag that indicates whether Game Genie is enabled or not.
static int genie = 0;

// Flag that indicates whether PAL Emulation is enabled or not.
static int pal_emulation = 0;
static int status_icon = 1;

static int vmod = 0;
static char *gfsdir=0;

extern char* directory_names[6];

/**
* Contains the names of the default directories.
**/
static const char *default_directory_names[5] = {"cheats", "sav", "fcs", "snaps", "movie"};

#define NUMBER_OF_DIRECTORIES sizeof(directory_names) / sizeof(*directory_names)
#define NUMBER_OF_DEFAULT_DIRECTORIES sizeof(default_directory_names) / sizeof(*default_directory_names)

static double saspectw = 1, saspecth = 1;
static double winsizemulx = 1, winsizemuly = 1;
static int winwidth, winheight;
static int ismaximized = 0;

static int soundrate = 44100;
static int soundbuftime = 50;
static int soundvolume = 100;
static int soundquality = 0;
static uint8 cpalette[192];
static int srendlinen = 8;
static int erendlinen = 231;
static int srendlinep = 0;
static int erendlinep = 239;

static int ntsccol = 0, ntsctint, ntschue;

extern int totallines;

//mbg merge 7/17/06 did these have to be unsigned?
static int srendline, erendline;

static int changerecursive=0;

static volatile int nofocus = 0;

/**
* Contains the base directory of FCE
**/
static char BaseDirectory[2048];

extern int soundo;
extern int eoptions;
extern int soundoptions;
extern uint8 *xbsave;
extern HRESULT ddrval;
extern int windowedfailed;
extern uint32 goptions;

void FixFL();
void DoFCEUExit();
void ShowAboutBox();
int BlockingCheck();
void DoPriority();
void RemoveDirs();
void CreateDirs();
void SetDirs();

#endif