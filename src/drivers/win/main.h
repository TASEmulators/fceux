#include "../../types.h"

static int genie;
static int pal_emulation;
static int status_icon;
static int fullscreen;
static int vmod;
static char *gfsdir;
static char *directory_names[6];
static double winsizemulx, winsizemuly;
static double saspectw, saspecth;
static int soundrate;
static int soundbuftime;
static int soundvolume;
static int soundquality;
static uint32 goptions;
static uint8 cpalette[192];
static int srendlinen;
static int erendlinen;
static int srendlinep;
static int erendlinep;
static int ismaximized;
static int maxconbskip;
static int ffbskip;
static int moviereadonly;

extern int soundo;
extern int eoptions;
extern int soundoptions;
