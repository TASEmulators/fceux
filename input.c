/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 1998 BERO 
 *  Copyright (C) 2002 Xodnizel
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
#include <string.h>

#include "types.h"
#include "x6502.h"

#include "fceu.h"
#include "sound.h"
#include "netplay.h"
#include "movie.h"
#include "state.h"

#include "input.h"
#include "vsuni.h"
#include "fds.h"

extern INPUTC *FCEU_InitZapper(int w);
extern INPUTC *FCEU_InitPowerpadA(int w);
extern INPUTC *FCEU_InitPowerpadB(int w);
extern INPUTC *FCEU_InitArkanoid(int w);

extern INPUTCFC *FCEU_InitArkanoidFC(void);
extern INPUTCFC *FCEU_InitSpaceShadow(void);
extern INPUTCFC *FCEU_InitFKB(void);
extern INPUTCFC *FCEU_InitSuborKB(void);
extern INPUTCFC *FCEU_InitHS(void);
extern INPUTCFC *FCEU_InitMahjong(void);
extern INPUTCFC *FCEU_InitQuizKing(void);
extern INPUTCFC *FCEU_InitFamilyTrainerA(void);
extern INPUTCFC *FCEU_InitFamilyTrainerB(void);
extern INPUTCFC *FCEU_InitOekaKids(void);
extern INPUTCFC *FCEU_InitTopRider(void);
extern INPUTCFC *FCEU_InitBarcodeWorld(void);

static uint8 joy_readbit[2];
static uint8 joy[4]={0,0,0,0};
static uint8 LastStrobe;

static int BotMode = 0;
#ifdef _USE_SHARED_MEMORY_
static int BotPointer = 0;
#endif

/* This function is a quick hack to get the NSF player to use emulated gamepad
   input.
*/
uint8 FCEU_GetJoyJoy(void)
{
 return(joy[0]|joy[1]|joy[2]|joy[3]);
}
extern uint8 coinon;

static int FSDisable=0;  /* Set to 1 if NES-style four-player adapter is disabled. */
static int JPAttrib[2]={0,0};
static int JPType[2]={0,0};
static void *InputDataPtr[2];

static int JPAttribFC=0;
static int JPTypeFC=0;
static void *InputDataPtrFC;

void (*InputScanlineHook)(uint8 *bg, uint8 *spr, uint32 linets, int final);


static INPUTC DummyJPort={0,0,0,0,0};
static INPUTC *JPorts[2]={&DummyJPort,&DummyJPort};
static INPUTCFC *FCExp=0;

static uint8 FP_FASTAPASS(1) ReadGPVS(int w)
{
                uint8 ret=0;
  
                if(joy_readbit[w]>=8)
                 ret=1;
                else
                {
                 ret = ((joy[w]>>(joy_readbit[w]))&1);
                 if(!fceuindbg)
                  joy_readbit[w]++;
                }
                return ret;
}

static uint8 FP_FASTAPASS(1) ReadGP(int w)
{
                uint8 ret;

                if(joy_readbit[w]>=8)
                 ret = ((joy[2+w]>>(joy_readbit[w]&7))&1);
                else
                 ret = ((joy[w]>>(joy_readbit[w]))&1);
                if(joy_readbit[w]>=16) ret=0;
                if(FSDisable)
		{
	  	 if(joy_readbit[w]>=8) ret|=1;
		}
		else
		{
                 if(joy_readbit[w]==19-w) ret|=1;
		}
		if(!fceuindbg)
		 joy_readbit[w]++;
                return ret;
}

static DECLFR(JPRead)
{
	uint8 ret=0;

	if(JPorts[A&1]->Read)
	 ret|=JPorts[A&1]->Read(A&1);
	
	if(FCExp)
	 if(FCExp->Read)
	  ret=FCExp->Read(A&1,ret);

	ret|=X.DB&0xC0;
	return(ret);
}

static DECLFW(B4016)
{
	if(FCExp)
	 if(FCExp->Write)
	  FCExp->Write(V&7);

	if(JPorts[0]->Write)
	 JPorts[0]->Write(V&1);
        if(JPorts[1]->Write)
         JPorts[1]->Write(V&1);

        if((LastStrobe&1) && (!(V&1)))
        {
	 /* This strobe code is just for convenience.  If it were
	    with the code in input / *.c, it would more accurately represent
	    what's really going on.  But who wants accuracy? ;)
	    Seriously, though, this shouldn't be a problem.
	 */
	 if(JPorts[0]->Strobe)
	  JPorts[0]->Strobe(0);
         if(JPorts[1]->Strobe)
          JPorts[1]->Strobe(1);
	 if(FCExp)
	  if(FCExp->Strobe)
	   FCExp->Strobe();
	 }
         LastStrobe=V&0x1;
}

static void FP_FASTAPASS(1) StrobeGP(int w)
{
	joy_readbit[w]=0;
}

static INPUTC GPC={ReadGP,0,StrobeGP,0,0,0};
static INPUTC GPCVS={ReadGPVS,0,StrobeGP,0,0,0};

int FCEU_BotMode()
{
	return BotMode;
}

void FCEU_SetBotMode(int x)
{
	BotMode = x;
}

void FCEU_DrawInput(uint8 *buf)
{
 int x;

 for(x=0;x<2;x++)
  if(JPorts[x]->Draw)
   JPorts[x]->Draw(x,buf,JPAttrib[x]);
 if(FCExp)
  if(FCExp->Draw)
   FCExp->Draw(buf,JPAttribFC);
}

void FCEU_UpdateBot()
{
#ifdef _USE_SHARED_MEMORY_
	//This is the external input (aka bot) code
	if(!BotMode)
		return;
	if(BotInput[0])
	{
		BotPointer++;
		switch(BotInput[BotPointer] >> 16)
		{
		case 0:
			joy[0] = BotInput[BotPointer] & 255;
			joy[1] = BotInput[BotPointer] >> 8;
			joy[2] = joy[3] = 0;
			FCEUI_FrameAdvance();
			break;
		case 1:
			FCEUI_LoadState(0);
			break;
		default:
			break;
		}

		//Bot input ends; let the world know we're done
		if(BotPointer >= BotInput[0] || BotPointer >= 1023)
		{
			BotInput[0] = 0;
			BotPointer = 0;
		}
	}
	else
	{
		BotPointer = 0;
		joy[0] = joy[1] = joy[2] = joy[3] = 0;
	}
#endif //_USE_SHARED_MEMORY_
}

void FCEU_UpdateInput(void)
{
	if(!FCEUMOV_IsPlaying() && !BotMode)
	{
		int x;

		for(x=0;x<2;x++)
		{
			switch(JPType[x])
			{
			case SI_GAMEPAD:
				if(!x)
				{
					joy[0]=*(uint32 *)InputDataPtr[0];
					joy[2]=*(uint32 *)InputDataPtr[0] >> 16;
				}
				else
				{
					joy[1]=*(uint32 *)InputDataPtr[1] >>8;
					joy[3]=*(uint32 *)InputDataPtr[1] >>24;
				}
				break;
			default:
				if(JPorts[x]->Update)
					JPorts[x]->Update(x,InputDataPtr[x],JPAttrib[x]);
				break;
			}
		}
		if(FCExp)
			if(FCExp->Update)
				FCExp->Update(InputDataPtrFC,JPAttribFC);
	}

	if(FCEUGameInfo->type==GIT_VSUNI)
		if(coinon) coinon--;

	if(FCEUnetplay)
		NetplayUpdate(joy);

	FCEUMOV_AddJoy(joy, BotMode);

	if(FCEUGameInfo->type==GIT_VSUNI)
		FCEU_VSUniSwap(&joy[0],&joy[1]);
}

static DECLFR(VSUNIRead0)
{ 
        uint8 ret=0; 
  
        if(JPorts[0]->Read)   
         ret|=(JPorts[0]->Read(0))&1;
 
        ret|=(vsdip&3)<<3;
        if(coinon)
         ret|=0x4;
        return ret;
}
 
static DECLFR(VSUNIRead1)
{
        uint8 ret=0;
 
        if(JPorts[1]->Read)
         ret|=(JPorts[1]->Read(1))&1;
        ret|=vsdip&0xFC;   
        return ret;
} 

static void SLHLHook(uint8 *bg, uint8 *spr, uint32 linets, int final)
{
 int x;

 for(x=0;x<2;x++)
  if(JPorts[x]->SLHook)
   JPorts[x]->SLHook(x,bg,spr,linets,final);
 if(FCExp) 
  if(FCExp->SLHook)
   FCExp->SLHook(bg,spr,linets,final);
}

static void CheckSLHook(void)
{
        InputScanlineHook=0;
        if(JPorts[0]->SLHook || JPorts[1]->SLHook)
         InputScanlineHook=SLHLHook;
        if(FCExp)
         if(FCExp->SLHook)
          InputScanlineHook=SLHLHook;
}

static void FASTAPASS(1) SetInputStuff(int x)
{
 	 switch(JPType[x])
	 {
	  case SI_GAMEPAD:
           if(FCEUGameInfo->type==GIT_VSUNI)
	    JPorts[x] = &GPCVS;
	   else
	    JPorts[x]=&GPC;
	  break;
	  case SI_ARKANOID:JPorts[x]=FCEU_InitArkanoid(x);break;
	  case SI_ZAPPER:JPorts[x]=FCEU_InitZapper(x);break;
          case SI_POWERPADA:JPorts[x]=FCEU_InitPowerpadA(x);break;
	  case SI_POWERPADB:JPorts[x]=FCEU_InitPowerpadB(x);break;
	  case SI_NONE:JPorts[x]=&DummyJPort;break;
         }

	CheckSLHook();
}

static uint8 F4ReadBit[2];
static void StrobeFami4(void)
{
 F4ReadBit[0]=F4ReadBit[1]=0;
}

static uint8 FP_FASTAPASS(2) ReadFami4(int w, uint8 ret)
{
 ret&=1;

 ret |= ((joy[2+w]>>(F4ReadBit[w]))&1)<<1;
 if(F4ReadBit[w]>=8) ret|=2;
 else F4ReadBit[w]++;

 return(ret);
}

static INPUTCFC FAMI4C={ReadFami4,0,StrobeFami4,0,0,0};
static void SetInputStuffFC(void)
{
        switch(JPTypeFC)
        {
         case SIFC_NONE:FCExp=0;break;
         case SIFC_ARKANOID:FCExp=FCEU_InitArkanoidFC();break;
	 case SIFC_SHADOW:FCExp=FCEU_InitSpaceShadow();break;
	 case SIFC_OEKAKIDS:FCExp=FCEU_InitOekaKids();break;
         case SIFC_4PLAYER:FCExp=&FAMI4C;memset(&F4ReadBit,0,sizeof(F4ReadBit));break;
	 case SIFC_FKB:FCExp=FCEU_InitFKB();break;
   case SIFC_SUBORKB:FCExp=FCEU_InitSuborKB();break;
	 case SIFC_HYPERSHOT:FCExp=FCEU_InitHS();break;
	 case SIFC_MAHJONG:FCExp=FCEU_InitMahjong();break;
	 case SIFC_QUIZKING:FCExp=FCEU_InitQuizKing();break;
	 case SIFC_FTRAINERA:FCExp=FCEU_InitFamilyTrainerA();break;
	 case SIFC_FTRAINERB:FCExp=FCEU_InitFamilyTrainerB();break;
	 case SIFC_BWORLD:FCExp=FCEU_InitBarcodeWorld();break;
	 case SIFC_TOPRIDER:FCExp=FCEU_InitTopRider();break;
        }
	CheckSLHook();
}

void InitializeInput(void)
{ 
	memset(joy_readbit,0,sizeof(joy_readbit));
        memset(joy,0,sizeof(joy));
	LastStrobe = 0;

        if(FCEUGameInfo->type==GIT_VSUNI)
        {
         SetReadHandler(0x4016,0x4016,VSUNIRead0);
         SetReadHandler(0x4017,0x4017,VSUNIRead1);
        } 
        else
         SetReadHandler(0x4016,0x4017,JPRead);

        SetWriteHandler(0x4016,0x4016,B4016);

        SetInputStuff(0);
        SetInputStuff(1);
	SetInputStuffFC();
}

void FCEUI_SetInput(int port, int type, void *ptr, int attrib)
{
 JPAttrib[port]=attrib;
 JPType[port]=type;
 InputDataPtr[port]=ptr;
 SetInputStuff(port);
}

void FCEUI_DisableFourScore(int s)
{
 FSDisable=s;
}

void FCEUI_SetInputFC(int type, void *ptr, int attrib)
{
 JPAttribFC=attrib;
 JPTypeFC=type;
 InputDataPtrFC=ptr;
 SetInputStuffFC();
}

SFORMAT FCEUCTRL_STATEINFO[]={
  { joy_readbit, 2, "JYRB"},
  { joy, 4, "JOYS"},
  { &LastStrobe, 1, "LSTS"},
  { 0 }
 };

void FCEU_DoSimpleCommand(int cmd)
{
	switch(cmd)
	{
	case FCEUNPCMD_FDSINSERT: FCEU_FDSInsert();break;
	case FCEUNPCMD_FDSSELECT: FCEU_FDSSelect();break;
		//   case FCEUNPCMD_FDSEJECT: FCEU_FDSEject();break;
	case FCEUNPCMD_VSUNICOIN: FCEU_VSUniCoin(); break;
	case FCEUNPCMD_VSUNIDIP0: //mbg merge 7/17/06 removed case range syntax
	case FCEUNPCMD_VSUNIDIP0+1:
	case FCEUNPCMD_VSUNIDIP0+2:
	case FCEUNPCMD_VSUNIDIP0+3:
	case FCEUNPCMD_VSUNIDIP0+4:
	case FCEUNPCMD_VSUNIDIP0+5:
	case FCEUNPCMD_VSUNIDIP0+6:
	case FCEUNPCMD_VSUNIDIP0+7:
		FCEU_VSUniToggleDIP(cmd - FCEUNPCMD_VSUNIDIP0);break;
	case FCEUNPCMD_POWER: PowerNES();break;
	case FCEUNPCMD_RESET: ResetNES();break;
	}
}

void FCEU_QSimpleCommand(int cmd)
{
 if(FCEUnetplay)
  FCEUNET_SendCommand(cmd, 0);
 else
 {
  FCEU_DoSimpleCommand(cmd);
  if(FCEUMOV_IsRecording())
   FCEUMOV_AddCommand(cmd);
 }
}

void FCEUI_FDSSelect(void)
{ 
 FCEU_QSimpleCommand(FCEUNPCMD_FDSSELECT);
} 

//mbg merge 7/17/06 changed to void fn(void) to make it an EMUCMDFN
void FCEUI_FDSInsert(void)
{ 
 FCEU_QSimpleCommand(FCEUNPCMD_FDSINSERT);
 //return(1);
}

/*
int FCEUI_FDSEject(void)
{
    FCEU_QSimpleCommand(FCEUNPCMD_FDSEJECT);
    return(1);
}
*/
void FCEUI_VSUniToggleDIP(int w)
{
 FCEU_QSimpleCommand(FCEUNPCMD_VSUNIDIP0 + w);
}

void FCEUI_VSUniCoin(void)
{
 FCEU_QSimpleCommand(FCEUNPCMD_VSUNICOIN);
}

void FCEUI_ResetNES(void)
{
	FCEU_QSimpleCommand(FCEUNPCMD_RESET);
}
  
void FCEUI_PowerNES(void)
{
        FCEU_QSimpleCommand(FCEUNPCMD_POWER);
}

const char* FCEUI_CommandTypeNames[]=
{
	"Misc.",
	"Speed",
	"State",
	"Movie",
	"Sound",
	"AVI",
	"FDS",
	"VS Sys",
};

static void CommandUnImpl(void);
static void CommandToggleDip(void);
static void CommandStateLoad(void);
static void CommandStateSave(void);
static void CommandSelectSaveSlot(void);
static void CommandEmulationSpeed(void);
static void CommandMovieSelectSlot(void);
static void CommandMovieRecord(void);
static void CommandMovieReplay(void);
static void CommandSoundAdjust(void);
static void ViewSlots(void);

struct EMUCMDTABLE FCEUI_CommandTable[]=
{
	{ EMUCMD_POWER,							EMUCMDTYPE_MISC,	FCEUI_PowerNES, 0, 0, "Power", },
	{ EMUCMD_RESET,							EMUCMDTYPE_MISC,	FCEUI_ResetNES, 0, 0, "Reset", },
	{ EMUCMD_PAUSE,							EMUCMDTYPE_MISC,	FCEUI_ToggleEmulationPause, 0, 0, "Pause", },
	{ EMUCMD_FRAME_ADVANCE,					EMUCMDTYPE_MISC,	FCEUI_FrameAdvance, 0, 0, "Frame Advance", },
	{ EMUCMD_SCREENSHOT,					EMUCMDTYPE_MISC,	FCEUI_SaveSnapshot, 0, 0, "Screenshot", },
	{ EMUCMD_HIDE_MENU_TOGGLE,				EMUCMDTYPE_MISC,	FCEUD_HideMenuToggle, 0, 0, "Hide Menu Toggle", },

	{ EMUCMD_SPEED_SLOWEST,					EMUCMDTYPE_SPEED,	CommandEmulationSpeed, 0, 0, "Slowest Speed", },
	{ EMUCMD_SPEED_SLOWER,					EMUCMDTYPE_SPEED,	CommandEmulationSpeed, 0, 0, "Speed Down", },
	{ EMUCMD_SPEED_NORMAL,					EMUCMDTYPE_SPEED,	CommandEmulationSpeed, 0, 0, "Normal Speed", },
	{ EMUCMD_SPEED_FASTER,					EMUCMDTYPE_SPEED,	CommandEmulationSpeed, 0, 0, "Speed Up", },
	{ EMUCMD_SPEED_FASTEST,					EMUCMDTYPE_SPEED,	CommandEmulationSpeed, 0, 0, "Fastest Speed", },
	{ EMUCMD_SPEED_TURBO,					EMUCMDTYPE_SPEED,	FCEUD_TurboOn, FCEUD_TurboOff, 0, "Turbo", },

	{ EMUCMD_SAVE_SLOT_0,					EMUCMDTYPE_STATE,	CommandSelectSaveSlot, 0, 0, "Savestate Slot 0", },
	{ EMUCMD_SAVE_SLOT_1,					EMUCMDTYPE_STATE,	CommandSelectSaveSlot, 0, 0, "Savestate Slot 1", },
	{ EMUCMD_SAVE_SLOT_2,					EMUCMDTYPE_STATE,	CommandSelectSaveSlot, 0, 0, "Savestate Slot 2", },
	{ EMUCMD_SAVE_SLOT_3,					EMUCMDTYPE_STATE,	CommandSelectSaveSlot, 0, 0, "Savestate Slot 3", },
	{ EMUCMD_SAVE_SLOT_4,					EMUCMDTYPE_STATE,	CommandSelectSaveSlot, 0, 0, "Savestate Slot 4", },
	{ EMUCMD_SAVE_SLOT_5,					EMUCMDTYPE_STATE,	CommandSelectSaveSlot, 0, 0, "Savestate Slot 5", },
	{ EMUCMD_SAVE_SLOT_6,					EMUCMDTYPE_STATE,	CommandSelectSaveSlot, 0, 0, "Savestate Slot 6", },
	{ EMUCMD_SAVE_SLOT_7,					EMUCMDTYPE_STATE,	CommandSelectSaveSlot, 0, 0, "Savestate Slot 7", },
	{ EMUCMD_SAVE_SLOT_8,					EMUCMDTYPE_STATE,	CommandSelectSaveSlot, 0, 0, "Savestate Slot 8", },
	{ EMUCMD_SAVE_SLOT_9,					EMUCMDTYPE_STATE,	CommandSelectSaveSlot, 0, 0, "Savestate Slot 9", },
	{ EMUCMD_SAVE_SLOT_NEXT,				EMUCMDTYPE_STATE,	CommandSelectSaveSlot, 0, 0, "Next Savestate Slot", },
	{ EMUCMD_SAVE_SLOT_PREV,				EMUCMDTYPE_STATE,	CommandSelectSaveSlot, 0, 0, "Previous Savestate Slot", },
	{ EMUCMD_SAVE_STATE,					EMUCMDTYPE_STATE,	CommandStateSave, 0, 0, "Save State", },
	{ EMUCMD_SAVE_STATE_AS,					EMUCMDTYPE_STATE,	FCEUD_SaveStateAs, 0, 0, "Save State As...", },
	{ EMUCMD_SAVE_STATE_SLOT_0,				EMUCMDTYPE_STATE,	CommandStateSave, 0, 0, "Save State to Slot 0", },
	{ EMUCMD_SAVE_STATE_SLOT_1,				EMUCMDTYPE_STATE,	CommandStateSave, 0, 0, "Save State to Slot 1", },
	{ EMUCMD_SAVE_STATE_SLOT_2,				EMUCMDTYPE_STATE,	CommandStateSave, 0, 0, "Save State to Slot 2", },
	{ EMUCMD_SAVE_STATE_SLOT_3,				EMUCMDTYPE_STATE,	CommandStateSave, 0, 0, "Save State to Slot 3", },
	{ EMUCMD_SAVE_STATE_SLOT_4,				EMUCMDTYPE_STATE,	CommandStateSave, 0, 0, "Save State to Slot 4", },
	{ EMUCMD_SAVE_STATE_SLOT_5,				EMUCMDTYPE_STATE,	CommandStateSave, 0, 0, "Save State to Slot 5", },
	{ EMUCMD_SAVE_STATE_SLOT_6,				EMUCMDTYPE_STATE,	CommandStateSave, 0, 0, "Save State to Slot 6", },
	{ EMUCMD_SAVE_STATE_SLOT_7,				EMUCMDTYPE_STATE,	CommandStateSave, 0, 0, "Save State to Slot 7", },
	{ EMUCMD_SAVE_STATE_SLOT_8,				EMUCMDTYPE_STATE,	CommandStateSave, 0, 0, "Save State to Slot 8", },
	{ EMUCMD_SAVE_STATE_SLOT_9,				EMUCMDTYPE_STATE,	CommandStateSave, 0, 0, "Save State to Slot 9", },
	{ EMUCMD_LOAD_STATE,					EMUCMDTYPE_STATE,	CommandStateLoad, 0, 0, "Load State", },
	{ EMUCMD_LOAD_STATE_FROM,				EMUCMDTYPE_STATE,	FCEUD_LoadStateFrom, 0, 0, "Load State From...", },
	{ EMUCMD_LOAD_STATE_SLOT_0,				EMUCMDTYPE_STATE,	CommandStateLoad, 0, 0, "Load State from Slot 0", },
	{ EMUCMD_LOAD_STATE_SLOT_1,				EMUCMDTYPE_STATE,	CommandStateLoad, 0, 0, "Load State from Slot 1", },
	{ EMUCMD_LOAD_STATE_SLOT_2,				EMUCMDTYPE_STATE,	CommandStateLoad, 0, 0, "Load State from Slot 2", },
	{ EMUCMD_LOAD_STATE_SLOT_3,				EMUCMDTYPE_STATE,	CommandStateLoad, 0, 0, "Load State from Slot 3", },
	{ EMUCMD_LOAD_STATE_SLOT_4,				EMUCMDTYPE_STATE,	CommandStateLoad, 0, 0, "Load State from Slot 4", },
	{ EMUCMD_LOAD_STATE_SLOT_5,				EMUCMDTYPE_STATE,	CommandStateLoad, 0, 0, "Load State from Slot 5", },
	{ EMUCMD_LOAD_STATE_SLOT_6,				EMUCMDTYPE_STATE,	CommandStateLoad, 0, 0, "Load State from Slot 6", },
	{ EMUCMD_LOAD_STATE_SLOT_7,				EMUCMDTYPE_STATE,	CommandStateLoad, 0, 0, "Load State from Slot 7", },
	{ EMUCMD_LOAD_STATE_SLOT_8,				EMUCMDTYPE_STATE,	CommandStateLoad, 0, 0, "Load State from Slot 8", },
	{ EMUCMD_LOAD_STATE_SLOT_9,				EMUCMDTYPE_STATE,	CommandStateLoad, 0, 0, "Load State from Slot 9", },

/*	{ EMUCMD_MOVIE_SLOT_0,					EMUCMDTYPE_MOVIE,	CommandMovieSelectSlot, 0, 0, "Movie Slot 0", },
	{ EMUCMD_MOVIE_SLOT_1,					EMUCMDTYPE_MOVIE,	CommandMovieSelectSlot, 0, 0, "Movie Slot 1", },
	{ EMUCMD_MOVIE_SLOT_2,					EMUCMDTYPE_MOVIE,	CommandMovieSelectSlot, 0, 0, "Movie Slot 2", },
	{ EMUCMD_MOVIE_SLOT_3,					EMUCMDTYPE_MOVIE,	CommandMovieSelectSlot, 0, 0, "Movie Slot 3", },
	{ EMUCMD_MOVIE_SLOT_4,					EMUCMDTYPE_MOVIE,	CommandMovieSelectSlot, 0, 0, "Movie Slot 4", },
	{ EMUCMD_MOVIE_SLOT_5,					EMUCMDTYPE_MOVIE,	CommandMovieSelectSlot, 0, 0, "Movie Slot 5", },
	{ EMUCMD_MOVIE_SLOT_6,					EMUCMDTYPE_MOVIE,	CommandMovieSelectSlot, 0, 0, "Movie Slot 6", },
	{ EMUCMD_MOVIE_SLOT_7,					EMUCMDTYPE_MOVIE,	CommandMovieSelectSlot, 0, 0, "Movie Slot 7", },
	{ EMUCMD_MOVIE_SLOT_8,					EMUCMDTYPE_MOVIE,	CommandMovieSelectSlot, 0, 0, "Movie Slot 8", },
	{ EMUCMD_MOVIE_SLOT_9,					EMUCMDTYPE_MOVIE,	CommandMovieSelectSlot, 0, 0, "Movie Slot 9", },
	{ EMUCMD_MOVIE_SLOT_NEXT,				EMUCMDTYPE_MOVIE,	CommandMovieSelectSlot, 0, 0, "Next Movie Slot", },
	{ EMUCMD_MOVIE_SLOT_PREV,				EMUCMDTYPE_MOVIE,	CommandMovieSelectSlot, 0, 0, "Previous Movie Slot", },
	{ EMUCMD_MOVIE_RECORD,					EMUCMDTYPE_MOVIE,	CommandMovieRecord, 0, 0, "Record Movie", },*/
	{ EMUCMD_MOVIE_RECORD_TO,				EMUCMDTYPE_MOVIE,	FCEUD_MovieRecordTo, 0, 0, "Record Movie To...", },
/*	{ EMUCMD_MOVIE_RECORD_SLOT_0,			EMUCMDTYPE_MOVIE,	CommandMovieRecord, 0, 0, "Record Movie to Slot 0", },
	{ EMUCMD_MOVIE_RECORD_SLOT_1,			EMUCMDTYPE_MOVIE,	CommandMovieRecord, 0, 0, "Record Movie to Slot 1", },
	{ EMUCMD_MOVIE_RECORD_SLOT_2,			EMUCMDTYPE_MOVIE,	CommandMovieRecord, 0, 0, "Record Movie to Slot 2", },
	{ EMUCMD_MOVIE_RECORD_SLOT_3,			EMUCMDTYPE_MOVIE,	CommandMovieRecord, 0, 0, "Record Movie to Slot 3", },
	{ EMUCMD_MOVIE_RECORD_SLOT_4,			EMUCMDTYPE_MOVIE,	CommandMovieRecord, 0, 0, "Record Movie to Slot 4", },
	{ EMUCMD_MOVIE_RECORD_SLOT_5,			EMUCMDTYPE_MOVIE,	CommandMovieRecord, 0, 0, "Record Movie to Slot 5", },
	{ EMUCMD_MOVIE_RECORD_SLOT_6,			EMUCMDTYPE_MOVIE,	CommandMovieRecord, 0, 0, "Record Movie to Slot 6", },
	{ EMUCMD_MOVIE_RECORD_SLOT_7,			EMUCMDTYPE_MOVIE,	CommandMovieRecord, 0, 0, "Record Movie to Slot 7", },
	{ EMUCMD_MOVIE_RECORD_SLOT_8,			EMUCMDTYPE_MOVIE,	CommandMovieRecord, 0, 0, "Record Movie to Slot 8", },
	{ EMUCMD_MOVIE_RECORD_SLOT_9,			EMUCMDTYPE_MOVIE,	CommandMovieRecord, 0, 0, "Record Movie to Slot 9", },
	{ EMUCMD_MOVIE_REPLAY,					EMUCMDTYPE_MOVIE,	CommandMovieReplay, 0, 0, "Replay Movie", },*/
	{ EMUCMD_MOVIE_REPLAY_FROM,				EMUCMDTYPE_MOVIE,	FCEUD_MovieReplayFrom, 0, 0, "Replay Movie From...", },
/*	{ EMUCMD_MOVIE_REPLAY_SLOT_0,			EMUCMDTYPE_MOVIE,	CommandMovieReplay, 0, 0, "Replay Movie from Slot 0", },
	{ EMUCMD_MOVIE_REPLAY_SLOT_1,			EMUCMDTYPE_MOVIE,	CommandMovieReplay, 0, 0, "Replay Movie from Slot 1", },
	{ EMUCMD_MOVIE_REPLAY_SLOT_2,			EMUCMDTYPE_MOVIE,	CommandMovieReplay, 0, 0, "Replay Movie from Slot 2", },
	{ EMUCMD_MOVIE_REPLAY_SLOT_3,			EMUCMDTYPE_MOVIE,	CommandMovieReplay, 0, 0, "Replay Movie from Slot 3", },
	{ EMUCMD_MOVIE_REPLAY_SLOT_4,			EMUCMDTYPE_MOVIE,	CommandMovieReplay, 0, 0, "Replay Movie from Slot 4", },
	{ EMUCMD_MOVIE_REPLAY_SLOT_5,			EMUCMDTYPE_MOVIE,	CommandMovieReplay, 0, 0, "Replay Movie from Slot 5", },
	{ EMUCMD_MOVIE_REPLAY_SLOT_6,			EMUCMDTYPE_MOVIE,	CommandMovieReplay, 0, 0, "Replay Movie from Slot 6", },
	{ EMUCMD_MOVIE_REPLAY_SLOT_7,			EMUCMDTYPE_MOVIE,	CommandMovieReplay, 0, 0, "Replay Movie from Slot 7", },
	{ EMUCMD_MOVIE_REPLAY_SLOT_8,			EMUCMDTYPE_MOVIE,	CommandMovieReplay, 0, 0, "Replay Movie from Slot 8", },
	{ EMUCMD_MOVIE_REPLAY_SLOT_9,			EMUCMDTYPE_MOVIE,	CommandMovieReplay, 0, 0, "Replay Movie from Slot 9", },
*/
	{ EMUCMD_MOVIE_STOP,					EMUCMDTYPE_MOVIE,	FCEUI_StopMovie, 0, 0, "Stop Movie", },
	{ EMUCMD_MOVIE_READONLY_TOGGLE,			EMUCMDTYPE_MOVIE,	FCEUI_MovieToggleReadOnly, 0, 0, "Toggle Read-Only", },
	{ EMUCMD_MOVIE_FRAME_DISPLAY_TOGGLE,	EMUCMDTYPE_MOVIE,	FCEUI_MovieToggleFrameDisplay, 0, 0, "Movie Frame Display Toggle", },
	{ EMUCMD_MOVIE_INPUT_DISPLAY_TOGGLE,	EMUCMDTYPE_MISC,	FCEUI_ToggleInputDisplay, 0, 0, "Toggle Input Display", },
	{ EMUCMD_MOVIE_ICON_DISPLAY_TOGGLE,		EMUCMDTYPE_MISC,	FCEUD_ToggleStatusIcon, 0, 0, "Toggle Status Icon", },

	{ EMUCMD_SOUND_TOGGLE,					EMUCMDTYPE_SOUND,	FCEUD_SoundToggle, 0, 0, "Sound Mute Toggle", },
	{ EMUCMD_SOUND_VOLUME_UP,				EMUCMDTYPE_SOUND,	CommandSoundAdjust, 0, 0, "Sound Volume Up", },
	{ EMUCMD_SOUND_VOLUME_DOWN,				EMUCMDTYPE_SOUND,	CommandSoundAdjust, 0, 0, "Sound Volume Down", },
	{ EMUCMD_SOUND_VOLUME_NORMAL,			EMUCMDTYPE_SOUND,	CommandSoundAdjust, 0, 0, "Sound Volume Normal", },

	{ EMUCMD_AVI_RECORD_AS,					EMUCMDTYPE_AVI,		FCEUD_AviRecordTo, 0, 0, "Record AVI As...", },
	{ EMUCMD_AVI_STOP,						EMUCMDTYPE_AVI,		FCEUD_AviStop, 0, 0, "Stop AVI", },

	{ EMUCMD_FDS_EJECT_INSERT,				EMUCMDTYPE_FDS,		FCEUI_FDSInsert, 0, 0, "Eject or Insert FDS Disk", },
	{ EMUCMD_FDS_SIDE_SELECT,				EMUCMDTYPE_FDS,		FCEUI_FDSSelect, 0, 0, "Switch FDS Disk Side", },

	{ EMUCMD_VSUNI_COIN,					EMUCMDTYPE_VSUNI,	FCEUI_VSUniCoin, 0, 0, "Insert Coin", },
	{ EMUCMD_VSUNI_TOGGLE_DIP_0,			EMUCMDTYPE_VSUNI,	CommandToggleDip, 0, 0, "Toggle Dipswitch 0", },
	{ EMUCMD_VSUNI_TOGGLE_DIP_1,			EMUCMDTYPE_VSUNI,	CommandToggleDip, 0, 0, "Toggle Dipswitch 1", },
	{ EMUCMD_VSUNI_TOGGLE_DIP_2,			EMUCMDTYPE_VSUNI,	CommandToggleDip, 0, 0, "Toggle Dipswitch 2", },
	{ EMUCMD_VSUNI_TOGGLE_DIP_3,			EMUCMDTYPE_VSUNI,	CommandToggleDip, 0, 0, "Toggle Dipswitch 3", },
	{ EMUCMD_VSUNI_TOGGLE_DIP_4,			EMUCMDTYPE_VSUNI,	CommandToggleDip, 0, 0, "Toggle Dipswitch 4", },
	{ EMUCMD_VSUNI_TOGGLE_DIP_5,			EMUCMDTYPE_VSUNI,	CommandToggleDip, 0, 0, "Toggle Dipswitch 5", },
	{ EMUCMD_VSUNI_TOGGLE_DIP_6,			EMUCMDTYPE_VSUNI,	CommandToggleDip, 0, 0, "Toggle Dipswitch 6", },
	{ EMUCMD_VSUNI_TOGGLE_DIP_7,			EMUCMDTYPE_VSUNI,	CommandToggleDip, 0, 0, "Toggle Dipswitch 7", },
	{ EMUCMD_VSUNI_TOGGLE_DIP_8,			EMUCMDTYPE_VSUNI,	CommandToggleDip, 0, 0, "Toggle Dipswitch 8", },
	{ EMUCMD_VSUNI_TOGGLE_DIP_9,			EMUCMDTYPE_VSUNI,	CommandToggleDip, 0, 0, "Toggle Dipswitch 9", },
	{ EMUCMD_MISC_REWIND,					EMUCMDTYPE_MISC,	FCEUI_Rewind, 0, 0, "Rewind", },
	{ EMUCMD_MISC_SHOWSTATES,				EMUCMDTYPE_MISC,    ViewSlots,	0,0, "View save slots", },
};

#define NUM_EMU_CMDS		(sizeof(FCEUI_CommandTable)/sizeof(FCEUI_CommandTable[0]))

static int execcmd;

void FCEUI_HandleEmuCommands(TestCommandState* testfn)
{
	for(execcmd=0; execcmd<NUM_EMU_CMDS; ++execcmd)
	{
		int new_state = (*testfn)(execcmd);
		int old_state = FCEUI_CommandTable[execcmd].state;
		if (new_state == 1 && old_state == 0 && FCEUI_CommandTable[execcmd].fn_on)
			(*(FCEUI_CommandTable[execcmd].fn_on))();
		else if (new_state == 0 && old_state == 1 && FCEUI_CommandTable[execcmd].fn_off)
			(*(FCEUI_CommandTable[execcmd].fn_off))();
		FCEUI_CommandTable[execcmd].state = new_state;
	}
}

static void CommandUnImpl(void)
{
	FCEU_DispMessage("command '%s' unimplemented.", FCEUI_CommandTable[execcmd].name);
}

static void CommandToggleDip(void)
{
	if (FCEUGameInfo->type==GIT_VSUNI)
		FCEUI_VSUniToggleDIP(execcmd-EMUCMD_VSUNI_TOGGLE_DIP_0);
}

static void CommandEmulationSpeed(void)
{
	FCEUD_SetEmulationSpeed(EMUSPEED_SLOWEST+(execcmd-EMUCMD_SPEED_SLOWEST));
}

void FCEUI_SelectStateNext(int);

static void ViewSlots(void)
{
	FCEUI_SelectState(CurrentState, 1);
}

static void CommandSelectSaveSlot(void)
{
	if(execcmd <= EMUCMD_SAVE_SLOT_9)
		FCEUI_SelectState(execcmd-EMUCMD_SAVE_SLOT_0, 1);
	else if(execcmd == EMUCMD_SAVE_SLOT_NEXT)
		FCEUI_SelectStateNext(1);
	else if(execcmd == EMUCMD_SAVE_SLOT_PREV)
		FCEUI_SelectStateNext(-1);
}

static void CommandStateSave(void)
{
//	FCEU_PrintError("execcmd=%d, EMUCMD_SAVE_STATE_SLOT_0=%d, EMUCMD_SAVE_STATE_SLOT_9=%d", execcmd,EMUCMD_SAVE_STATE_SLOT_0,EMUCMD_SAVE_STATE_SLOT_9);
	if(execcmd >= EMUCMD_SAVE_STATE_SLOT_0 && execcmd <= EMUCMD_SAVE_STATE_SLOT_9)
	{
		int oldslot=FCEUI_SelectState(execcmd-EMUCMD_SAVE_STATE_SLOT_0, 0);
		FCEUI_SaveState(0);
		FCEUI_SelectState(oldslot, 0);
	}
	else
		FCEUI_SaveState(0);
}

static void CommandStateLoad(void)
{
	if(execcmd >= EMUCMD_LOAD_STATE_SLOT_0 && execcmd <= EMUCMD_LOAD_STATE_SLOT_9)
	{
		int oldslot=FCEUI_SelectState(execcmd-EMUCMD_LOAD_STATE_SLOT_0, 0);
		FCEUI_LoadState(0);
		FCEUI_SelectState(oldslot, 0);
	}
	else
		FCEUI_LoadState(0);
}

void FCEUI_SelectMovieNext(int);
/*
static void CommandMovieSelectSlot(void)
{
	if(execcmd <= EMUCMD_MOVIE_SLOT_9)
		FCEUI_SelectMovie(execcmd-EMUCMD_MOVIE_SLOT_0, 1);
	else if(execcmd == EMUCMD_MOVIE_SLOT_NEXT)
		FCEUI_SelectMovieNext(1);
	else if(execcmd == EMUCMD_MOVIE_SLOT_PREV)
		FCEUI_SelectMovieNext(-1);
}

static void CommandMovieRecord(void)
{
	if(execcmd >= EMUCMD_MOVIE_RECORD_SLOT_0 && execcmd <= EMUCMD_MOVIE_RECORD_SLOT_9)
	{
		int oldslot=FCEUI_SelectMovie(execcmd-EMUCMD_MOVIE_RECORD_SLOT_0, 0);
		FCEUI_SaveMovie(0, 0, 0);
		FCEUI_SelectMovie(oldslot, 0);
	}
	else
		FCEUI_SaveMovie(0, 0, 0);
}

static void CommandMovieReplay(void)
{
	if(execcmd >= EMUCMD_MOVIE_REPLAY_SLOT_0 && execcmd <= EMUCMD_MOVIE_REPLAY_SLOT_9)
	{
		int oldslot=FCEUI_SelectMovie(execcmd-EMUCMD_MOVIE_REPLAY_SLOT_0, 0);
		FCEUI_LoadMovie(0, 0);
		FCEUI_SelectMovie(oldslot, 0);
	}
	else
		FCEUI_LoadMovie(0, 0);
}
*/
static void CommandSoundAdjust(void)
{
	int n;
	switch(execcmd)
	{
	case EMUCMD_SOUND_VOLUME_UP:		n=1;  break;
	case EMUCMD_SOUND_VOLUME_DOWN:		n=-1;  break;
	case EMUCMD_SOUND_VOLUME_NORMAL:	n=0;  break;
	}

	FCEUD_SoundVolumeAdjust(n);
}

