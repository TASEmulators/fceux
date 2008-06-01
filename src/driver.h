#ifndef __DRIVER_H_
#define __DRIVER_H_

#include <stdio.h>
#include <string>

#include "types.h"
#include "git.h"

FILE *FCEUD_UTF8fopen(const char *fn, const char *mode);


//mbg 7/23/06
const char *FCEUD_GetCompilerString();

/* This makes me feel dirty for some reason. */
void FCEU_printf(char *format, ...);
#define FCEUI_printf FCEU_printf

/* Video interface */
void FCEUD_SetPalette(uint8 index, uint8 r, uint8 g, uint8 b);
void FCEUD_GetPalette(uint8 i,uint8 *r, uint8 *g, uint8 *b);

/* Displays an error.  Can block or not. */
void FCEUD_PrintError(const char *s);
void FCEUD_Message(const char *s);

/* Network interface */

/* Call only when a game is loaded. */
int FCEUI_NetplayStart(int nlocal, int divisor);

/* Call when network play needs to stop. */
void FCEUI_NetplayStop(void);

/* Note:  YOU MUST NOT CALL ANY FCEUI_* FUNCTIONS WHILE IN FCEUD_SendData() or
   FCEUD_RecvData().
*/

/* Return 0 on failure, 1 on success. */
int FCEUD_SendData(void *data, uint32 len);
int FCEUD_RecvData(void *data, uint32 len);

/* Display text received over the network. */
void FCEUD_NetplayText(uint8 *text);

/* Encode and send text over the network. */
void FCEUI_NetplayText(uint8 *text);

/* Called when a fatal error occurred and network play can't continue.  This function
   should call FCEUI_NetplayStop() after it has deinitialized the network on the driver
   side.
*/
void FCEUD_NetworkClose(void);

bool FCEUI_BeginWaveRecord(const char *fn);
int FCEUI_EndWaveRecord(void);

void FCEUI_ResetNES(void);
void FCEUI_PowerNES(void);

void FCEUI_NTSCSELHUE(void);
void FCEUI_NTSCSELTINT(void);
void FCEUI_NTSCDEC(void);
void FCEUI_NTSCINC(void);
void FCEUI_GetNTSCTH(int *tint, int *hue);
void FCEUI_SetNTSCTH(int n, int tint, int hue);

void FCEUI_SetInput(int port, int type, void *ptr, int attrib);
void FCEUI_SetInputFC(int type, void *ptr, int attrib);
void FCEUI_DisableFourScore(int s);
void FCEUI_UseInputPreset(int preset);

#define SI_NONE      0
#define SI_GAMEPAD   1
#define SI_ZAPPER    2
#define SI_POWERPADA  3
#define SI_POWERPADB  4
#define SI_ARKANOID   5
#define SI_MOUSE   6 //mbg merge 7/17/06 added

#define SIFC_NONE      0
#define SIFC_ARKANOID  1
#define SIFC_SHADOW      2
#define SIFC_4PLAYER    3
#define SIFC_FKB      4
#define SIFC_SUBORKB    5
#define SIFC_HYPERSHOT  6
#define SIFC_MAHJONG  7
#define SIFC_QUIZKING  8
#define SIFC_FTRAINERA  9
#define SIFC_FTRAINERB  10
#define SIFC_OEKAKIDS  11
#define SIFC_BWORLD      12
#define SIFC_TOPRIDER  13

#define SIS_NONE  0
#define SIS_DATACH  1
#define SIS_NWC    2
#define SIS_VSUNISYSTEM  3
#define SIS_NSF    4

/* New interface functions */

/* 0 to order screen snapshots numerically(0.png), 1 to order them file base-numerically(smb3-0.png). */
void FCEUI_SetSnapName(int a);

/* 0 to keep 8-sprites limitation, 1 to remove it */
void FCEUI_DisableSpriteLimitation(int a);

/* -1 = no change, 0 = show, 1 = hide, 2 = internal toggle */
void FCEUI_SetRenderDisable(int sprites, int bg);

/* name=path and file to load.  returns 0 on failure, 1 on success */
FCEUGI *FCEUI_LoadGame(const char *name, int OverwriteVidMode);

/* allocates memory.  0 on failure, 1 on success. */
int FCEUI_Initialize(void);

/* Emulates a frame. */
void FCEUI_Emulate(uint8 **, int32 **, int32 *, int);

/* Closes currently loaded game */
void FCEUI_CloseGame(void);

/* Deallocates all allocated memory.  Call after FCEUI_Emulate() returns. */
void FCEUI_Kill(void);

/* Enable/Disable game genie. a=0 disable, a=1 enable */
void FCEUI_SetGameGenie(int a);

/* Set video system a=0 NTSC, a=1 PAL */
void FCEUI_SetVidSystem(int a);

/* Convenience function; returns currently emulated video system(0=NTSC, 1=PAL).  */
int FCEUI_GetCurrentVidSystem(int *slstart, int *slend);

#ifdef FRAMESKIP
/* Should be called from FCEUD_BlitScreen().  Specifies how many frames
   to skip until FCEUD_BlitScreen() is called.  FCEUD_BlitScreenDummy()
   will be called instead of FCEUD_BlitScreen() when when a frame is skipped.
*/
void FCEUI_FrameSkip(int x);
#endif

/* First and last scanlines to render, for ntsc and pal emulation. */
void FCEUI_SetRenderedLines(int ntscf, int ntscl, int palf, int pall);

/* Sets the base directory(save states, snapshots, etc. are saved in directories
   below this directory. */
void FCEUI_SetBaseDirectory(const char *dir);

/* Tells FCE Ultra to copy the palette data pointed to by pal and use it.
   Data pointed to by pal needs to be 64*3 bytes in length.
*/
void FCEUI_SetPaletteArray(uint8 *pal);

/* Sets up sound code to render sound at the specified rate, in samples
   per second.  Only sample rates of 44100, 48000, and 96000 are currently
   supported.
   If "Rate" equals 0, sound is disabled.
*/
void FCEUI_Sound(int Rate);
void FCEUI_SetSoundVolume(uint32 volume);
void FCEUI_SetSoundQuality(int quality);

void FCEUD_SoundToggle(void);
void FCEUD_SoundVolumeAdjust(int);

int FCEUI_SelectState(int, int);

/* "fname" overrides the default save state filename code if non-NULL. */
void FCEUI_SaveState(char *fname);
void FCEUI_LoadState(char *fname);

void FCEUD_SaveStateAs(void);
void FCEUD_LoadStateFrom(void);

//movie was recorded from poweron. the alternative is from a savestate (or from reset)
#define MOVIE_FLAG_FROM_POWERON (1<<3)

//an ARCHAIC flag which means the movie was recorded from a soft reset.
//WHY would you do this?? do not create any new movies with this flag
#define MOVIE_FLAG_FROM_RESET   (1<<1)

#define MOVIE_FLAG_PAL          (1<<2)

// set in newer version, used for old movie compatibility
//TODO - only use this flag to print a warning that the sync might be bad
//so that we can get rid of the sync hack code
#define MOVIE_FLAG_NOSYNCHACK          (1<<4) 

#define MOVIE_MAX_METADATA      512

typedef struct
{
 int    movie_version;					// version of the movie format in the file
 uint32 num_frames;
 uint32 rerecord_count;
 bool poweron, reset, pal, nosynchack;
 int    read_only;
 uint32 emu_version_used;				// 9813 = 0.98.13
 char*  metadata;						// caller-supplied buffer to store metadata.  can be NULL.
 int    metadata_size;					// size of the buffer pointed to by metadata
 MD5DATA md5_of_rom_used;
 bool md5_of_rom_used_present;		// v1 movies don't have md5 info available
 std::string name_of_rom_used;				
} MOVIE_INFO;

void FCEUI_SaveMovie(char *fname, uint8 flags);
void FCEUI_LoadMovie(char *fname, bool read_only, int _stopframe);
void FCEUI_StopMovie(void);
int FCEUI_IsMovieActive(void);
int FCEUI_MovieGetInfo(const char* fname, MOVIE_INFO* /* [in, out] */ info);
char* FCEUI_MovieGetCurrentName(int addSlotNumber);
void FCEUI_MovieToggleReadOnly(void);
bool FCEUI_GetMovieToggleReadOnly();
void FCEUI_MovieToggleFrameDisplay();
void FCEUI_ToggleInputDisplay(void);


void FCEUD_MovieRecordTo(void);
void FCEUD_MovieReplayFrom(void);

int32 FCEUI_GetDesiredFPS(void);
void FCEUI_SaveSnapshot(void);
void FCEU_DispMessage(char *format, ...);
#define FCEUI_DispMessage FCEU_DispMessage

int FCEUI_DecodePAR(const char *code, uint16 *a, uint8 *v, int *c, int *type);
int FCEUI_DecodeGG(const char *str, uint16 *a, uint8 *v, int *c);
int FCEUI_AddCheat(const char *name, uint32 addr, uint8 val, int compare, int type);
int FCEUI_DelCheat(uint32 which);
int FCEUI_ToggleCheat(uint32 which);

int32 FCEUI_CheatSearchGetCount(void);
void FCEUI_CheatSearchGetRange(uint32 first, uint32 last, int (*callb)(uint32 a, uint8 last, uint8 current));
void FCEUI_CheatSearchGet(int (*callb)(uint32 a, uint8 last, uint8 current, void *data), void *data);
void FCEUI_CheatSearchBegin(void);
void FCEUI_CheatSearchEnd(int type, uint8 v1, uint8 v2);
void FCEUI_ListCheats(int (*callb)(char *name, uint32 a, uint8 v, int compare, int s, int type, void *data), void *data);

int FCEUI_GetCheat(uint32 which, char **name, uint32 *a, uint8 *v, int *compare, int *s, int *type);
int FCEUI_SetCheat(uint32 which, const char *name, int32 a, int32 v, int compare,int s, int type);

void FCEUI_CheatSearchShowExcluded(void);
void FCEUI_CheatSearchSetCurrentAsOriginal(void);

#define FCEUIOD_ROMS    0
#define FCEUIOD_NV      1
#define FCEUIOD_STATES  2
#define FCEUIOD_FDSROM  3
#define FCEUIOD_SNAPS   4
#define FCEUIOD_CHEATS  5
#define FCEUIOD_MOVIES  6
#define FCEUIOD_MEMW    7
#define FCEUIOD_BBOT    8
#define FCEUIOD_MACRO   9
#define FCEUIOD_INPUT   10
#define FCEUIOD_LUA     11

#define FCEUIOD__COUNT  12

void FCEUI_SetDirOverride(int which, char *n);

void FCEUI_MemDump(uint16 a, int32 len, void (*callb)(uint16 a, uint8 v));
uint8 FCEUI_MemSafePeek(uint16 A);
void FCEUI_MemPoke(uint16 a, uint8 v, int hl);
void FCEUI_NMI(void);
void FCEUI_IRQ(void);
uint16 FCEUI_Disassemble(void *XA, uint16 a, char *stringo);
void FCEUI_GetIVectors(uint16 *reset, uint16 *irq, uint16 *nmi);

uint32 FCEUI_CRC32(uint32 crc, uint8 *buf, uint32 len);

void FCEUI_ToggleTileView(void);
void FCEUI_SetLowPass(int q);

void FCEUI_NSFSetVis(int mode);
int FCEUI_NSFChange(int amount);
int FCEUI_NSFGetInfo(uint8 *name, uint8 *artist, uint8 *copyright, int maxlen);

void FCEUI_VSUniToggleDIPView(void);
void FCEUI_VSUniToggleDIP(int w);
uint8 FCEUI_VSUniGetDIPs(void);
void FCEUI_VSUniSetDIP(int w, int state);
void FCEUI_VSUniCoin(void);

void FCEUI_FDSInsert(void); //mbg merge 7/17/06 changed to void fn(void) to make it an EMUCMDFN
//int FCEUI_FDSEject(void);
void FCEUI_FDSSelect(void);

int FCEUI_DatachSet(const uint8 *rcode);

///returns a flag indicating whether emulation is paused
int FCEUI_EmulationPaused(void);
///returns a flag indicating whether a one frame step has been requested
int FCEUI_EmulationFrameStepped();
///clears the framestepped flag. use it after youve stepped your one frame
void FCEUI_ClearEmulationFrameStepped();
///sets the EmulationPaused flags
void FCEUI_SetEmulationPaused(int val);
///toggles the paused bit (bit0) for EmulationPaused. caused FCEUD_DebugUpdate() to fire if the emulation pauses
void FCEUI_ToggleEmulationPause(void);

///called when the emulator closes a game
void FCEUD_OnCloseGame(void);

void FCEUI_FrameAdvance(void);
void FCEUI_FrameAdvanceEnd(void);

/* AVI Output */
int FCEUI_AviBegin(const char* fname);
void FCEUI_AviEnd(void);
void FCEUI_AviVideoUpdate(const unsigned char* buffer);
void FCEUI_AviSoundUpdate(void* soundData, int soundLen);
int FCEUI_AviIsRecording(void);

void FCEUD_AviRecordTo(void);
void FCEUD_AviStop(void);

///A callback that the emu core uses to poll the state of a given emulator command key 
typedef int TestCommandState(int cmd);
///Signals the emu core to poll for emulator commands and take actions
void FCEUI_HandleEmuCommands(TestCommandState* testfn);


/* Emulation speed */
enum EMUSPEED_SET
{
	EMUSPEED_SLOWEST=0,
	EMUSPEED_SLOWER,
	EMUSPEED_NORMAL,
	EMUSPEED_FASTER,
	EMUSPEED_FASTEST
};
void FCEUD_SetEmulationSpeed(int cmd);
void FCEUD_TurboOn(void);
void FCEUD_TurboOff(void);

int FCEUD_ShowStatusIcon(void);
void FCEUD_ToggleStatusIcon(void);
void FCEUD_HideMenuToggle(void);

///signals the driver to perform a file open GUI operation
void FCEUD_CmdOpen(void);

//new merge-era driver routines here:

///signals that the cpu core hit a breakpoint. this function should not return until the core is ready for the next cycle
void FCEUD_DebugBreakpoint();

///the driver should log the current instruction, if it wants (we should move the code in the win driver that does this to the shared area)
void FCEUD_TraceInstruction();

///the driver might should update its NTView (only used if debugging support is compiled in)
void FCEUD_UpdateNTView(int scanline, int drawall);

///the driver might should update its PPUView (only used if debugging support is compiled in)
void FCEUD_UpdatePPUView(int scanline, int drawall);

///I am dissatisfied with this method of getting an option from the driver to the core. but that is what we're using for now
bool FCEUD_PauseAfterPlayback();

#ifdef __cplusplus
extern "C" 
#endif 
FILE *FCEUI_UTF8fopen_C(const char *n, const char *m);

#endif /* __DRIVER_H_ */
