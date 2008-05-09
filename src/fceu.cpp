/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2003 Xodnizel
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

#include  <string.h>
#include	<stdio.h>
#include	<stdlib.h>
#include	<stdarg.h>
#include	"types.h"
#include	"x6502.h"
#include	"fceu.h"
#include	"ppu.h"
#include	"sound.h"
#include	"netplay.h"
#include	"file.h"
#include	"utils/endian.h"
#include    "utils/memory.h"
#include	"utils/crc32.h"

#include	"cart.h"
#include	"nsf.h"
#include	"fds.h"
#include	"ines.h"
#include	"unif.h"
#include    "cheat.h"
#include	"palette.h"
#include	"state.h"
#include	"movie.h"
#include    "video.h"
#include	"input.h"
#include	"file.h"
#include	"vsuni.h"


uint64 timestampbase;


FCEUGI *GameInfo = 0;

void (*GameInterface)(int h);
void (*GameStateRestore)(int version);

readfunc ARead[0x10000];
writefunc BWrite[0x10000];
static readfunc *AReadG;
static writefunc *BWriteG;
static int RWWrap=0;

//mbg merge 7/18/06 docs
//bit0 indicates whether emulation is paused
//bit1 indicates whether emulation is in frame step mode
static int EmulationPaused=0;
bool frameAdvanceRequested=false;
int frameAdvanceDelay;

//indicates that the emulation core just frame advanced (consumed the frame advance state and paused)
bool JustFrameAdvanced=false;

static int RewindStatus[4] = {0, 0, 0, 0}; //is it safe to load rewind state
static int RewindIndex = 0; //which rewind state we're on

// Flag that indicates whether the rewind option is enabled or not
int EnableRewind = 0;

///a wrapper for unzip.c
extern "C" FILE *FCEUI_UTF8fopen_C(const char *n, const char *m) { return ::FCEUD_UTF8fopen(n,m); }

static DECLFW(BNull)
{

}

static DECLFR(ANull)
{
 return(X.DB);
}

int AllocGenieRW(void)
{
 if(!(AReadG=(readfunc *)FCEU_malloc(0x8000*sizeof(readfunc))))
  return 0;
 if(!(BWriteG=(writefunc *)FCEU_malloc(0x8000*sizeof(writefunc))))
  return 0;
 RWWrap=1;
 return 1;
}

void FlushGenieRW(void)
{
 int32 x;

 if(RWWrap)
 {
  for(x=0;x<0x8000;x++)
  {
   ARead[x+0x8000]=AReadG[x];
   BWrite[x+0x8000]=BWriteG[x];
  }
  free(AReadG);
  free(BWriteG);
  AReadG=0;
  BWriteG=0;
  RWWrap=0;
 }
}

readfunc FASTAPASS(1) GetReadHandler(int32 a)
{
  if(a>=0x8000 && RWWrap)
   return AReadG[a-0x8000];
  else
   return ARead[a];
}

void FASTAPASS(3) SetReadHandler(int32 start, int32 end, readfunc func)
{
  int32 x;

  if(!func)
   func=ANull;

  if(RWWrap)
   for(x=end;x>=start;x--)
   {
    if(x>=0x8000)
     AReadG[x-0x8000]=func;
    else
     ARead[x]=func;
   }
  else

   for(x=end;x>=start;x--)
    ARead[x]=func;
}

writefunc FASTAPASS(1) GetWriteHandler(int32 a)
{
  if(RWWrap && a>=0x8000)
   return BWriteG[a-0x8000];
  else
   return BWrite[a];
}

void FASTAPASS(3) SetWriteHandler(int32 start, int32 end, writefunc func)
{
  int32 x;

  if(!func)
   func=BNull;

  if(RWWrap)
   for(x=end;x>=start;x--)
   {
    if(x>=0x8000)
     BWriteG[x-0x8000]=func;
    else
     BWrite[x]=func;
   }
  else
   for(x=end;x>=start;x--)
    BWrite[x]=func;
}

uint8 *GameMemBlock;
uint8 *RAM;

//---------
//windows might need to allocate these differently, so we have some special code

static void AllocBuffers()
{
	
#ifdef _USE_SHARED_MEMORY_

	void win_AllocBuffers(uint8 **GameMemBlock, uint8 **RAM);
	win_AllocBuffers(&GameMemBlock, &RAM);
	
#else 

	GameMemBlock = (uint8*)FCEU_gmalloc(131072);
	RAM = (uint8*)FCEU_gmalloc(0x800);
	
#endif
}

static void FreeBuffers() {
	#ifdef _USE_SHARED_MEMORY_
	void win_FreeBuffers(uint8 *GameMemBlock, uint8 *RAM);
	win_FreeBuffers(GameMemBlock, RAM);
	#else 
	FCEU_free(GameMemBlock);
	FCEU_free(RAM);
	#endif
}
//------

uint8 PAL=0;

static DECLFW(BRAML)
{  
        RAM[A]=V;
}

static DECLFW(BRAMH)
{
        RAM[A&0x7FF]=V;
}

static DECLFR(ARAML)
{
        return RAM[A];
}

static DECLFR(ARAMH)
{
        return RAM[A&0x7FF];
}

static void CloseGame(void)
{
	if(GameInfo)
	{
		if(FCEUnetplay)
		{
			FCEUD_NetworkClose();
		}

		FCEUI_StopMovie();

		if(GameInfo->name)
		{
			free(GameInfo->name);
			GameInfo->name=0;
		}

		if(GameInfo->type!=GIT_NSF)
		{
			FCEU_FlushGameCheats(0,0);
		}

		GameInterface(GI_CLOSE);

		ResetExState(0,0);

		//mbg 5/9/08 - clear screen when game is closed
		//http://sourceforge.net/tracker/index.php?func=detail&aid=1787298&group_id=13536&atid=113536
		extern uint8 *XBuf;
		if(XBuf)
			memset(XBuf,0,256*256);

		CloseGenie();

		delete GameInfo;
		GameInfo = 0;
	}
}

void ResetGameLoaded(void)
{
        if(GameInfo) CloseGame();
		EmulationPaused = 0; //mbg 5/8/08 - loading games while paused was bad news. maybe this fixes it
        GameStateRestore=0;
        PPU_hook=0;
        GameHBIRQHook=0;
        if(GameExpSound.Kill)
         GameExpSound.Kill();
        memset(&GameExpSound,0,sizeof(GameExpSound));
        MapIRQHook=0;
        MMC5Hack=0;
        PAL&=1;
	pale=0;
}

int UNIFLoad(const char *name, FCEUFILE *fp);
int iNESLoad(const char *name, FCEUFILE *fp, int OverwriteVidMode);
int FDSLoad(const char *name, FCEUFILE *fp);
int NSFLoad(FCEUFILE *fp);

char lastLoadedGameName [2048] = {0,}; // hack for movie WRAM clearing on record from poweron

FCEUGI *FCEUI_LoadGame(const char *name, int OverwriteVidMode)
{
	//mbg merge 7/17/07 - why is this here
//#ifdef WIN32
//	StopSound();
//#endif

	//----------
	//attempt to open the files
	FCEUFILE *fp;
	char *ipsfn;

	FCEU_printf("Loading %s...\n\n",name);
	GetFileBase(name);

	ipsfn=FCEU_MakeFName(FCEUMKF_IPS,0,0);
	fp=FCEU_fopen(name,ipsfn,"rb",0);
	free(ipsfn);

	if(!fp) {
 		FCEU_PrintError("Error opening \"%s\"!",name);
		return 0;
	}
	//---------

	//file opened ok. start loading.

	ResetGameLoaded();
	
	RewindStatus[0] = RewindStatus[1] = 0;
	RewindStatus[2] = RewindStatus[3] = 0;

	CloseGame();
	GameInfo = new FCEUGI;
	memset(GameInfo, 0, sizeof(FCEUGI));
		
	GameInfo->soundchan = 0;
	GameInfo->soundrate = 0;
    GameInfo->name=0;
    GameInfo->type=GIT_CART;
    GameInfo->vidsys=GIV_USER;
    GameInfo->input[0]=GameInfo->input[1]=-1;
    GameInfo->inputfc=-1;
    GameInfo->cspecial=0;

		//try to load each different format
        if(iNESLoad(name,fp,OverwriteVidMode))
         goto endlseq;
        if(NSFLoad(fp))
         goto endlseq;
        if(UNIFLoad(name,fp))
         goto endlseq;
        if(FDSLoad(name,fp))
         goto endlseq;

        FCEU_PrintError("An error occurred while loading the file.");
		FCEUD_PrintError("An error occurred while loading the file.");
        FCEU_fclose(fp);

		delete GameInfo;

        return 0;

        endlseq:
		
        FCEU_fclose(fp);

        FCEU_ResetVidSys();
		
        if(GameInfo->type!=GIT_NSF)
         if(FSettings.GameGenie)
	  OpenGenie();
	  PowerNES();
		
	FCEUSS_CheckStates();
	FCEUMOV_CheckMovies();

        if(GameInfo->type!=GIT_NSF)
        {
         FCEU_LoadGamePalette();
         FCEU_LoadGameCheats(0);
        }
        
	FCEU_ResetPalette();
	FCEU_ResetMessages();	// Save state, status messages, etc.

	strcpy(lastLoadedGameName, name);

        return GameInfo;
}


/**
* Return: Flag that indicates whether the function was succesful or not.
**/
int FCEUI_Initialize(void)
{
	if(!FCEU_InitVirtualVideo())
	{
		return 0;
	}

	AllocBuffers();

	// Initialize some parts of the settings structure
	//mbg 5/7/08 - I changed the ntsc settings to match pal.
	//this is more for precision emulation, instead of entertainment, which is what fceux is all about nowadays
	memset(&FSettings,0,sizeof(FSettings));
	//FSettings.UsrFirstSLine[0]=8;
	FSettings.UsrFirstSLine[0]=0;
	FSettings.UsrFirstSLine[1]=0;
	//FSettings.UsrLastSLine[0]=231;
	FSettings.UsrLastSLine[0]=239;
	FSettings.UsrLastSLine[1]=239;
	FSettings.SoundVolume=100;
	
	FCEUPPU_Init();
	
	X6502_Init();
	
	return 1;
}

void FCEUI_Kill(void)
{
	FCEU_KillVirtualVideo();
	FCEU_KillGenie();
	FreeBuffers();
}

int rapidAlternator = 0;
int AutoFirePattern[8] = {1,0,0,0,0,0,0,0};
int AutoFirePatternLength = 2;
int AutoFireOffset = 0;

void SetAutoFirePattern(int onframes, int offframes)
{
	int i;
	for(i = 0; i < onframes && i < 8; i++)
	{
		AutoFirePattern[i] = 1;
	}
	for(;i < 8; i++)
	{
		AutoFirePattern[i] = 0;
	}
	if(onframes + offframes < 2)
	{
		AutoFirePatternLength = 2;
	}
	else if(onframes + offframes > 8)
	{
		AutoFirePatternLength = 8;
	}
	else
	{
		AutoFirePatternLength = onframes + offframes;
	}
}

void SetAutoFireOffset(int offset)
{
	if(offset < 0 || offset > 8) return;
	AutoFireOffset = offset;
}

void AutoFire(void)
{
	static int counter = 0;
	counter = (++counter) % (8*7*5*3);
	//If recording a movie, use the frame # for the autofire so the offset
	//doesn't get screwed up when loading.
	if(FCEUMOV_IsPlaying() || FCEUMOV_IsRecording())
	{
		rapidAlternator= AutoFirePattern[(AutoFireOffset + FCEUMOV_GetFrame())%AutoFirePatternLength];
	}
	else
	{
		rapidAlternator= AutoFirePattern[(AutoFireOffset + counter)%AutoFirePatternLength];
	}
}

void UpdateRewind(void);

///Emulates a single frame.

///Skip may be passed in, if FRAMESKIP is #defined, to cause this to emulate more than one frame
void FCEUI_Emulate(uint8 **pXBuf, int32 **SoundBuf, int32 *SoundBufSize, int skip)
{
 int r,ssize;
  
 JustFrameAdvanced = false;

 if(frameAdvanceRequested) {
	 if(frameAdvanceDelay==0) {
		EmulationPaused = 3;
		frameAdvanceDelay++;
	 } else {
		 if(frameAdvanceDelay>=10) {
			 EmulationPaused = 3;
		 } else frameAdvanceDelay++;
	 }
 }


 if(EmulationPaused&2)
  EmulationPaused &= ~1;        // clear paused flag temporarily (frame advance)
 else if(EmulationPaused&1 || FCEU_BotMode())
 {
  memcpy(XBuf, XBackBuf, 256*256);
  FCEU_PutImage();
  *pXBuf=XBuf;
  *SoundBuf=WaveFinal;
  *SoundBufSize=0;

  return;
 }

 if(!FCEU_BotMode())
 {
	AutoFire();
	UpdateRewind();
 }
 
 FCEU_UpdateInput();
 if(geniestage!=1) FCEU_ApplyPeriodicCheats();
 r=FCEUPPU_Loop(skip);
 
 ssize=FlushEmulateSound();

//#ifdef WIN32
//   FCEUI_AviVideoUpdate(XBuf);
//#endif

 timestampbase += timestamp;
 timestamp = 0;


 *pXBuf=skip?0:XBuf;
 *SoundBuf=WaveFinal;
 *SoundBufSize=ssize;

 //if we were asked to frame advance, then since we have just finished
 //a frame, we should switch to regular pause
 if(EmulationPaused&2)
 {
  EmulationPaused = 1;          // restore paused flag
  //mbg merge 7/28/06 don't like the looks of this...
//#ifdef WIN32
//  #define SO_MUTEFA     16
//  extern int soundoptions;
//  if(soundoptions&SO_MUTEFA)
//#endif
	*SoundBufSize=0;              // keep sound muted
	JustFrameAdvanced = true;
 }
}

void FCEUI_CloseGame(void)
{	
	CloseGame();
}

/**
* @param do_power_off Power off (1) or reset (0)
**/
void RestartMovieOrReset(unsigned int do_power_off)
{
	extern int movie_readonly;
	extern char curMovieFilename[512];

	if(FCEUMOV_IsPlaying() || FCEUMOV_IsRecording() && movie_readonly)
	{
		FCEUI_LoadMovie(curMovieFilename, movie_readonly, 0);

		if(FCEUI_IsMovieActive())
		{
			return;
		}
	}

	if(do_power_off)
	{
		FCEUI_PowerNES();
	}
	else
	{
		FCEUI_ResetNES();
	}
}

void ResetNES(void)
{
        FCEUMOV_AddCommand(FCEUNPCMD_RESET);
        if(!GameInfo) return;
        GameInterface(GI_RESETM2);
        FCEUSND_Reset();
        FCEUPPU_Reset();
        X6502_Reset();

	// clear back baffer
	extern uint8 *XBackBuf;
	memset(XBackBuf,0,256*256);
}

void FCEU_MemoryRand(uint8 *ptr, uint32 size)
{
 int x=0;
 while(size)
 {
  *ptr=(x&4)?0xFF:0x00;
  x++;
  size--;
  ptr++;
 }
}

void hand(X6502 *X, int type, unsigned int A)
{

}

int suppressAddPowerCommand=0; // hack... yeah, I know...
void PowerNES(void) 
{
	if(!suppressAddPowerCommand)
        FCEUMOV_AddCommand(FCEUNPCMD_POWER);
    if(!GameInfo) return;

	FCEU_CheatResetRAM();
	FCEU_CheatAddRAM(2,0,RAM);

        GeniePower();

	FCEU_MemoryRand(RAM,0x800);
	//memset(RAM,0xFF,0x800);
	
        SetReadHandler(0x0000,0xFFFF,ANull);
        SetWriteHandler(0x0000,0xFFFF,BNull);
        
		SetReadHandler(0,0x7FF,ARAML);
        SetWriteHandler(0,0x7FF,BRAML);
        
		SetReadHandler(0x800,0x1FFF,ARAMH);  /* Part of a little */
        SetWriteHandler(0x800,0x1FFF,BRAMH); /* hack for a small speed boost. */
 
		InitializeInput();
		FCEUSND_Power();
        FCEUPPU_Power();

	/* Have the external game hardware "powered" after the internal NES stuff.  
	   Needed for the NSF code and VS System code.
	*/
	GameInterface(GI_POWER);
	if(GameInfo->type==GIT_VSUNI)
	FCEU_VSUniPower();

	timestampbase=0;
	X6502_Power();
	FCEU_PowerCheats();
	// clear back baffer
	extern uint8 *XBackBuf;
	memset(XBackBuf,0,256*256);
}

void FCEU_ResetVidSys(void)
{
 int w;
  
 if(GameInfo->vidsys==GIV_NTSC)
  w=0; 
 else if(GameInfo->vidsys==GIV_PAL)
  w=1;  
 else
  w=FSettings.PAL;
  
 PAL=w?1:0;
 FCEUPPU_SetVideoSystem(w);
 SetSoundVariables();
}

FCEUS FSettings;

void FCEU_printf(char *format, ...)
{
 char temp[2048];

 va_list ap;

 va_start(ap,format);
 vsprintf(temp,format,ap);
 FCEUD_Message(temp);

 va_end(ap);
}

void FCEU_PrintError(char *format, ...)
{
 char temp[2048];

 va_list ap;

 va_start(ap,format);
 vsprintf(temp,format,ap);
 FCEUD_PrintError(temp);

 va_end(ap);
}

void FCEUI_SetRenderedLines(int ntscf, int ntscl, int palf, int pall)
{
 FSettings.UsrFirstSLine[0]=ntscf;
 FSettings.UsrLastSLine[0]=ntscl;
 FSettings.UsrFirstSLine[1]=palf;
 FSettings.UsrLastSLine[1]=pall;
 if(PAL)
 {
  FSettings.FirstSLine=FSettings.UsrFirstSLine[1];
  FSettings.LastSLine=FSettings.UsrLastSLine[1];
 }
 else
 {
  FSettings.FirstSLine=FSettings.UsrFirstSLine[0];
  FSettings.LastSLine=FSettings.UsrLastSLine[0];
 }

}

void FCEUI_SetVidSystem(int a)
{
 FSettings.PAL=a?1:0;
 if(GameInfo)
 {
  FCEU_ResetVidSys();
  FCEU_ResetPalette();
 }
}

int FCEUI_GetCurrentVidSystem(int *slstart, int *slend)
{
 if(slstart)
  *slstart=FSettings.FirstSLine;
 if(slend)
  *slend=FSettings.LastSLine;
 return(PAL);
}

/**
* Enable or disable Game Genie option.
**/
void FCEUI_SetGameGenie(int a)
{
	FSettings.GameGenie = a ? 1 : 0;
}

void FCEUI_SetSnapName(int a)
{
 FSettings.SnapName=a;
}

int32 FCEUI_GetDesiredFPS(void)
{
  if(PAL)
   return(838977920); // ~50.007
  else
   return(1008307711);	// ~60.1
}

int FCEUI_EmulationPaused(void)
{
	return (EmulationPaused&1);
}

int FCEUI_EmulationFrameStepped()
{
	return (EmulationPaused&2);
}

void FCEUI_ClearEmulationFrameStepped()
{
	EmulationPaused &=~2;
}

//mbg merge 7/18/06 added
//ideally maybe we shouldnt be using this, but i need it for quick merging
void FCEUI_SetEmulationPaused(int val) {
	EmulationPaused = val;
}

void FCEUI_ToggleEmulationPause(void)
{
	EmulationPaused = (EmulationPaused&1)^1;
}

void FCEUI_FrameAdvanceEnd(void)
{
	frameAdvanceRequested = false;
}

void FCEUI_FrameAdvance(void)
{
	frameAdvanceRequested = true;
	frameAdvanceDelay = 0;
}

static int RewindCounter = 0;

void UpdateRewind(void)
{
	if(!EnableRewind)
		return;
	
	char * f;
	RewindCounter = (RewindCounter + 1) % 256;
	if(RewindCounter == 0)
	{
		RewindIndex = (RewindIndex + 1) % 4;
		f = FCEU_MakeFName(FCEUMKF_REWINDSTATE,RewindIndex,0);
		FCEUSS_Save(f);
		free(f);
		RewindStatus[RewindIndex] = 1;
	}
}

void FCEUI_Rewind(void)
{
	if(!EnableRewind)
		return;

	if(RewindStatus[RewindIndex] == 1)
	{
		char * f;
		f = FCEU_MakeFName(FCEUMKF_REWINDSTATE,RewindIndex,0);
		FCEUSS_Load(f);
		free(f);
		
		//Set pointer to previous available slot
		if(RewindStatus[(RewindIndex + 3)%4] == 1)
		{
			RewindIndex = (RewindIndex + 3)%4;
		}

		//Reset time to next rewind save
		RewindCounter = 0;
	}
}

