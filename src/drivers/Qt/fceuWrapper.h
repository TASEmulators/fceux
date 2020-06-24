// fceuWrapper.h
//
#include "config.h"

//*****************************************************************
// Define Global Variables to be shared with FCEU Core
//*****************************************************************
extern int dendy;
extern int eoptions;
extern int isLoaded;
extern int pal_emulation;
extern int gametype;
extern int closeFinishedMovie;
extern bool turbo;
extern bool swapDuty;
extern unsigned int gui_draw_area_width;
extern unsigned int gui_draw_area_height;

// global configuration object
extern Config *g_config;

int LoadGame(const char *path);
int CloseGame(void);
