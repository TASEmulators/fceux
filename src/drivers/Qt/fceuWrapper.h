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
extern bool emulatorCycleToggle;
extern bool showStatusIconOpt;
extern bool drawInputAidsEnable;
extern bool usePaletteForVideoBg;
extern unsigned int gui_draw_area_width;
extern unsigned int gui_draw_area_height;
extern unsigned int emulatorCycleCount;

// global configuration object
extern Config *g_config;

int LoadGame(const char *path, bool silent = false);
int CloseGame(void);
int reloadLastGame(void);
int LoadGameFromLua( const char *path );

int  fceuWrapperPreInit( int argc, char *argv[] );
int  fceuWrapperInit( int argc, char *argv[] );
int  fceuWrapperMemoryCleanup( void );
int  fceuWrapperClose( void );
int  fceuWrapperUpdate( void );
void fceuWrapperLock(void);
void fceuWrapperLock(const char *filename, int line, const char *func);
bool fceuWrapperTryLock(int timeout = 1000);
bool fceuWrapperTryLock(const char *filename, int line, const char *func, int timeout = 1000);
bool fceuWrapperIsLocked(void);
void fceuWrapperUnLock(void);
int  fceuWrapperSoftReset(void);
int  fceuWrapperHardReset(void);
int  fceuWrapperTogglePause(void);
bool fceuWrapperGameLoaded(void);
void fceuWrapperRequestAppExit(void);

class  fceuCriticalSection
{
	public:
		fceuCriticalSection( const char *filename, int lineNum, const char *func )
		{
			//printf("Wrapper Lock\n");
			fceuWrapperLock( filename, lineNum, func );
		}

		~fceuCriticalSection(void)
		{
			//printf("Wrapper UnLock\n");
			fceuWrapperUnLock();
		}
};

#define  FCEU_WRAPPER_LOCK()   \
	fceuWrapperLock( __FILE__, __LINE__, __func__ )

#define  FCEU_WRAPPER_TRYLOCK(timeout)   \
	fceuWrapperTryLock( __FILE__, __LINE__, __func__, timeout )

#define  FCEU_WRAPPER_UNLOCK()   \
	fceuWrapperUnLock()

#define  FCEU_CRITICAL_SECTION(x)  \
	fceuCriticalSection x(__FILE__, __LINE__, __func__)
