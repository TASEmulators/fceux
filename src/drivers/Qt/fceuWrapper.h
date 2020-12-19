// fceuWrapper.h
//
#include "Qt/config.h"

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
extern bool pauseAfterPlayback;
extern bool suggestReadOnlyReplay;
extern unsigned int gui_draw_area_width;
extern unsigned int gui_draw_area_height;

// global configuration object
extern Config *g_config;

int LoadGame(const char *path, bool silent = false);
int CloseGame(void);
int reloadLastGame(void);

int  fceuWrapperInit( int argc, char *argv[] );
int  fceuWrapperClose( void );
int  fceuWrapperUpdate( void );
void fceuWrapperLock(void);
bool fceuWrapperTryLock(int timeout = 1000);
bool fceuWrapperIsLocked(void);
void fceuWrapperUnLock(void);
int  fceuWrapperSoftReset(void);
int  fceuWrapperHardReset(void);
int  fceuWrapperTogglePause(void);
bool fceuWrapperGameLoaded(void);
void fceuWrapperRequestAppExit(void);

