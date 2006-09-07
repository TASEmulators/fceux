/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
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

// For commctrl.h below
#define _WIN32_IE	0x0300

#include "common.h"
#include "dinput.h"
#include <windows.h>
#include <commctrl.h>

#include "input.h"
#include "keyboard.h"
#include "joystick.h"
#include "../../fceu.h" //mbg merge 7/17/06 added

#include "keyscan.h"

LPDIRECTINPUT7 lpDI=0;

/* UsrInputType[] is user-specified.  InputType[] is current
        (game loading can override user settings)
*/

int UsrInputType[3]={SI_GAMEPAD,SI_GAMEPAD,SIFC_NONE};
int InputType[3]={0,0,0};
int cspec=0;
   
int gametype=0;

int InitDInput(void)
{
 HRESULT ddrval;

 //mbg merge 7/17/06 changed:
 //ddrval=DirectInputCreateEx(fceu_hInstance,DIRECTINPUT_VERSION,&IID_IDirectInput7,(LPVOID *)&lpDI,0);
 ddrval=DirectInputCreateEx(fceu_hInstance,DIRECTINPUT_VERSION,IID_IDirectInput7,(LPVOID *)&lpDI,0);
 if(ddrval!=DI_OK)
 {
  FCEUD_PrintError("DirectInput: Error creating DirectInput object.");
  return 0;
 }
 return 1;
}

static uint32 MouseData[3];
static int screenmode=0;
void InputScreenChanged(int fs)
{
 int x;
 if(GameInfo)
 {
  for(x=0;x<2;x++)
   if(InputType[x]==SI_ZAPPER)
    FCEUI_SetInput(x,SI_ZAPPER,MouseData,fs);
  if(InputType[2]==SIFC_SHADOW)
   FCEUI_SetInputFC(SIFC_SHADOW,MouseData,fs);
 }
 screenmode=fs;
}

/* Necessary for proper GUI functioning(configuring when a game isn't loaded). */
void InputUserActiveFix(void)
{
 int x;
 for(x=0;x<3;x++) InputType[x]=UsrInputType[x];
}

void ParseGIInput(FCEUGI *gi)
{ 
 InputType[0]=UsrInputType[0];
 InputType[1]=UsrInputType[1];
 InputType[2]=UsrInputType[2];
 
 if(gi)
 {
  if(gi->input[0]>=0)
   InputType[0]=gi->input[0];
  if(gi->input[1]>=0)
   InputType[1]=gi->input[1];
  if(gi->inputfc>=0)
   InputType[2]=gi->inputfc;
  cspec = gi->cspecial;
  gametype=gi->type;

  InitOtherInput();
 }
 else cspec=gametype=0;
}


static uint8 QuizKingData=0;
static uint8 HyperShotData=0;
static uint32 MahjongData=0;
static uint32 FTrainerData=0;
static uint8 TopRiderData=0;

static uint8 BWorldData[1+13+1];

static void UpdateFKB(void);
static void UpdateSuborKB(void);
       void UpdateGamepad(void);
static void UpdateQuizKing(void);
static void UpdateHyperShot(void);
static void UpdateMahjong(void);
static void UpdateFTrainer(void);
static void UpdateTopRider(void);

static uint32 JSreturn=0;
int NoWaiting=0;
bool turbo = false;

#include "keyscan.h"
static unsigned char *keys=0;
static unsigned char *keys_nr=0;
static int DIPS=0;

static uint8 keyonce[MKK_COUNT];
//#define KEY(__a) keys_nr[MKK(__a)]

static int _keyonly(int a)
{
 if(keys_nr[a])
 {
  if(!keyonce[a]) 
  {
   keyonce[a]=1;
   return(1);
  }
 }
 else
  keyonce[a]=0;
 return(0);
}

int cidisabled=0;

#define MK(x)   {{BUTTC_KEYBOARD},{0},{MKK(x)},1}
#define MC(x)   {{BUTTC_KEYBOARD},{0},{x},1}
#define MK2(x1,x2)        {{BUTTC_KEYBOARD},{0},{MKK(x1),MKK(x2)},2}

#define MKZ()   {{0},{0},{0},0}

#define GPZ()   {MKZ(), MKZ(), MKZ(), MKZ()}

ButtConfig GamePadConfig[4][10]={
        /* Gamepad 1 */
        { 
         MK(LEFTALT), MK(LEFTCONTROL), MK(TAB), MK(ENTER), MK(BL_CURSORUP),
          MK(BL_CURSORDOWN),MK(BL_CURSORLEFT),MK(BL_CURSORRIGHT)
	},

        /* Gamepad 2 */
        GPZ(),

        /* Gamepad 3 */
        GPZ(),
  
        /* Gamepad 4 */
        GPZ()
};

extern int rapidAlternator; // for auto-fire / autofire
int DesynchAutoFire=0; // A and B not at same time
uint32 JSAutoHeld=0, JSAutoHeldAffected=0; // for auto-hold
uint8 autoHoldOn=0, autoHoldReset=0, autoHoldRefire=0; // for auto-hold

void SetAutoFireDesynch(int DesynchOn)
{
	if(DesynchOn)
	{
		DesynchAutoFire = 1;
	}
	else
	{
		DesynchAutoFire = 0;
	}
}

int GetAutoFireDesynch()
{
	return DesynchAutoFire;
}

int DTestButton(ButtConfig *bc)
{
 uint32 x;//mbg merge 7/17/06 changed to uint

 for(x=0;x<bc->NumC;x++)
 {
  if(bc->ButtType[x]==BUTTC_KEYBOARD)
  {
   if(keys_nr[bc->ButtonNum[x]])
   {
    return(1);
   }
  }
 }
 if(DTestButtonJoy(bc)) return(1);
 return(0);
}


void UpdateGamepad()
{
 if(FCEUI_IsMovieActive()<0)
   return;

 uint32 JS=0;
 int x;
 int wg;
 if(FCEUI_IsMovieActive()>0)
	 AutoFire();

 for(wg=0;wg<4;wg++)
 {
  for(x=0;x<8;x++)
   if(DTestButton(&GamePadConfig[wg][x]))
    JS|=(1<<x)<<(wg<<3);

//  if(rapidAlternator)
   for(x=0;x<2;x++)
     if(DTestButton(&GamePadConfig[wg][8+x]))
		 JS|=((1<<x)<<(wg<<3))*(rapidAlternator^(x*DesynchAutoFire));
 }

 if(autoHoldOn)
 {
	 if(autoHoldRefire)
	 {
		 autoHoldRefire--;
		 if(!autoHoldRefire)
			 JSAutoHeldAffected = 0;
	 }

	 for(wg=0;wg<4;wg++)
		for(x=0;x<8;x++)
			if(DTestButton(&GamePadConfig[wg][x]))
			{
				if(!autoHoldRefire || !(JSAutoHeldAffected&(1<<x)<<(wg<<3)))
				{
					JSAutoHeld^=(1<<x)<<(wg<<3);
					JSAutoHeldAffected|=(1<<x)<<(wg<<3);
					autoHoldRefire = 192;
				}
			}

	char inputstr [32];
	{
		uint32 c = JSAutoHeld;
		sprintf(inputstr, "%c%c%c%c%c%c%c%c %c%c%c%c%c%c%c%c",
			(c&0x40)?'<':' ', (c&0x10)?'^':' ', (c&0x80)?'>':' ', (c&0x20)?'v':' ',
			(c&0x01)?'A':' ', (c&0x02)?'B':' ', (c&0x08)?'S':' ', (c&0x04)?'s':' ',
			(c&0x4000)?'<':' ', (c&0x1000)?'^':' ', (c&0x8000)?'>':' ', (c&0x2000)?'v':' ',
			(c&0x0100)?'A':' ', (c&0x0200)?'B':' ', (c&0x0800)?'S':' ', (c&0x0400)?'s':' ');
		if(!(c&0xff00))
			inputstr[8] = '\0';
	}
	FCEU_DispMessage("Held: %s", inputstr);
 }
 else
 {
	 JSAutoHeldAffected = 0;
	 autoHoldRefire = 0;
 }

 if(autoHoldReset)
 {
	 FCEU_DispMessage("Held:         ");
	 JSAutoHeld = 0;
	 JSAutoHeldAffected = 0;
	 autoHoldRefire = 0;
 }

 // apply auto-hold
 if(JSAutoHeld)
	 JS ^= JSAutoHeld;

  JSreturn=JS;
}

ButtConfig powerpadsc[2][12]={
                              {
                               MK(O),MK(P),MK(BRACKET_LEFT),
                               MK(BRACKET_RIGHT),

                               MK(K),MK(L),MK(SEMICOLON),
				MK(APOSTROPHE),
                               MK(M),MK(COMMA),MK(PERIOD),MK(SLASH)
                              },
                              {
                               MK(O),MK(P),MK(BRACKET_LEFT),
                               MK(BRACKET_RIGHT),MK(K),MK(L),MK(SEMICOLON),
                                MK(APOSTROPHE),
                               MK(M),MK(COMMA),MK(PERIOD),MK(SLASH)
                              }
                             };

static uint32 powerpadbuf[2];

static uint32 UpdatePPadData(int w)
{
 uint32 r=0;
 ButtConfig *ppadtsc=powerpadsc[w];
 int x;

 for(x=0;x<12;x++)
  if(DTestButton(&ppadtsc[x])) r|=1<<x;

 return r;
}


static uint8 fkbkeys[0x48];
static uint8 suborkbkeys[0x60];

void KeyboardUpdateState(void); //mbg merge 7/17/06 yech had to add this
void GetMouseData(uint32 *md); //mbg merge 7/17/06 yech had to add this

void FCEUD_UpdateInput()
{
	int x;
	int t=0;

	KeyboardUpdateState();
	UpdateJoysticks();

	//UpdatePhysicalInput();
	//KeyboardCommands();
	FCEUI_HandleEmuCommands(FCEUD_TestCommandState);

	{
		for(x=0;x<2;x++)
			switch(InputType[x])
			{
				case SI_GAMEPAD:t|=1;break;
				case SI_ARKANOID:t|=2;break;
				case SI_ZAPPER:t|=2;break;
				case SI_POWERPADA:
				case SI_POWERPADB:powerpadbuf[x]=UpdatePPadData(x);break;
			}

		switch(InputType[2])
		{
			case SIFC_ARKANOID:t|=2;break;
			case SIFC_SHADOW:t|=2;break;
			case SIFC_FKB:if(cidisabled) UpdateFKB();break;
			case SIFC_SUBORKB:if(cidisabled) UpdateSuborKB();break;
			case SIFC_HYPERSHOT: UpdateHyperShot();break;
			case SIFC_MAHJONG: UpdateMahjong();break;
			case SIFC_QUIZKING: UpdateQuizKing();break;
			case SIFC_FTRAINERB:
			case SIFC_FTRAINERA: UpdateFTrainer();break;
			case SIFC_TOPRIDER: UpdateTopRider();break;
			case SIFC_OEKAKIDS:t|=2;break;
		}

		if(t&1)
			UpdateGamepad();

		if(t&2)
			GetMouseData(MouseData);
	}
}

void InitOtherInput(void)
{
   void *InputDPtr;

   int t;
   int x;
   int attrib;

   for(t=0,x=0;x<2;x++)
   {
    attrib=0;
    InputDPtr=0;
    switch(InputType[x])
    {
     case SI_POWERPADA:
     case SI_POWERPADB:InputDPtr=&powerpadbuf[x];break;
     case SI_GAMEPAD:InputDPtr=&JSreturn;break;     
     case SI_ARKANOID:InputDPtr=MouseData;t|=1;break;
     case SI_ZAPPER:InputDPtr=MouseData;
                                t|=1;
                                attrib=screenmode;
                                break;
    }
    FCEUI_SetInput(x,InputType[x],InputDPtr,attrib);
   }

   attrib=0;
   InputDPtr=0;
   switch(InputType[2])
   {
    case SIFC_SHADOW:InputDPtr=MouseData;t|=1;attrib=screenmode;break;
    case SIFC_OEKAKIDS:InputDPtr=MouseData;t|=1;attrib=screenmode;break;
    case SIFC_ARKANOID:InputDPtr=MouseData;t|=1;break;
    case SIFC_FKB:InputDPtr=fkbkeys;break;
    case SIFC_SUBORKB:InputDPtr=suborkbkeys;break;
    case SIFC_HYPERSHOT:InputDPtr=&HyperShotData;break;
    case SIFC_MAHJONG:InputDPtr=&MahjongData;break;
    case SIFC_QUIZKING:InputDPtr=&QuizKingData;break;
    case SIFC_TOPRIDER:InputDPtr=&TopRiderData;break;
    case SIFC_BWORLD:InputDPtr=BWorldData;break;
    case SIFC_FTRAINERA:
    case SIFC_FTRAINERB:InputDPtr=&FTrainerData;break;
   }

   FCEUI_SetInputFC(InputType[2],InputDPtr,attrib);
   FCEUI_DisableFourScore(eoptions&EO_NOFOURSCORE);
}

ButtConfig fkbmap[0x48]=
{
 MK(F1),MK(F2),MK(F3),MK(F4),MK(F5),MK(F6),MK(F7),MK(F8),
 MK(1),MK(2),MK(3),MK(4),MK(5),MK(6),MK(7),MK(8),MK(9),MK(0),
        MK(MINUS),MK(EQUAL),MK(BACKSLASH),MK(BACKSPACE),
 MK(ESCAPE),MK(Q),MK(W),MK(E),MK(R),MK(T),MK(Y),MK(U),MK(I),MK(O),
        MK(P),MK(GRAVE),MK(BRACKET_LEFT),MK(ENTER),
 MK(LEFTCONTROL),MK(A),MK(S),MK(D),MK(F),MK(G),MK(H),MK(J),MK(K),
        MK(L),MK(SEMICOLON),MK(APOSTROPHE),MK(BRACKET_RIGHT),MK(INSERT),
 MK(LEFTSHIFT),MK(Z),MK(X),MK(C),MK(V),MK(B),MK(N),MK(M),MK(COMMA),
        MK(PERIOD),MK(SLASH),MK(RIGHTALT),MK(RIGHTSHIFT),MK(LEFTALT),MK(SPACE),
 MK(BL_DELETE),
 MK(BL_END),
 MK(BL_PAGEDOWN),
 MK(BL_CURSORUP),MK(BL_CURSORLEFT),MK(BL_CURSORRIGHT),MK(BL_CURSORDOWN)
};

ButtConfig suborkbmap[0x60]=
{
 MC(0x01),MC(0x3b),MC(0x3c),MC(0x3d),MC(0x3e),MC(0x3f),MC(0x40),MC(0x41),MC(0x42),MC(0x43),
 MC(0x44),MC(0x57),MC(0x58),MC(0x45),MC(0x29),MC(0x02),MC(0x03),MC(0x04),MC(0x05),MC(0x06),
 MC(0x07),MC(0x08),MC(0x09),MC(0x0a),MC(0x0b),MC(0x0c),MC(0x0d),MC(0x0e),MC(0xd2),MC(0xc7),
 MC(0xc9),MC(0xc5),MC(0xb5),MC(0x37),MC(0x4a),MC(0x0f),MC(0x10),MC(0x11),MC(0x12),MC(0x13),
 MC(0x14),MC(0x15),MC(0x16),MC(0x17),MC(0x18),MC(0x19),MC(0x1a),MC(0x1b),MC(0x1c),MC(0xd3),
 MC(0xca),MC(0xd1),MC(0x47),MC(0x48),MC(0x49),MC(0x4e),MC(0x3a),MC(0x1e),MC(0x1f),MC(0x20),
 MC(0x21),MC(0x22),MC(0x23),MC(0x24),MC(0x25),MC(0x26),MC(0x27),MC(0x28),MC(0x4b),MC(0x4c),
 MC(0x4d),MC(0x2a),MC(0x2c),MC(0x2d),MC(0x2e),MC(0x2f),MC(0x30),MC(0x31),MC(0x32),MC(0x33),
 MC(0x34),MC(0x35),MC(0x2b),MC(0xc8),MC(0x4f),MC(0x50),MC(0x51),MC(0x1d),MC(0x38),MC(0x39),
 MC(0xcb),MC(0xd0),MC(0xcd),MC(0x52),MC(0x53)
};


static void UpdateFKB(void)
{
 int x;

 for(x=0;x<0x48;x++)
 {
  fkbkeys[x]=0;

  if(DTestButton(&fkbmap[x]))
   fkbkeys[x]=1;
 }
}

static void UpdateSuborKB(void)
{
 int x;

 for(x=0;x<0x60;x++)
 {
  suborkbkeys[x]=0;

  if(DTestButton(&suborkbmap[x]))
   suborkbkeys[x]=1;
 }
}

static ButtConfig HyperShotButtons[4]=
{
 MK(Q),MK(W),MK(E),MK(R)
};

static void UpdateHyperShot(void)
{
 int x;

 HyperShotData=0;
 for(x=0;x<0x4;x++)
 {
  if(DTestButton(&HyperShotButtons[x]))
   HyperShotData|=1<<x;
 }
}

static ButtConfig MahjongButtons[21]=
{
 MK(Q),MK(W),MK(E),MK(R),MK(T),
 MK(A),MK(S),MK(D),MK(F),MK(G),MK(H),MK(J),MK(K),MK(L),
 MK(Z),MK(X),MK(C),MK(V),MK(B),MK(N),MK(M)
};

static void UpdateMahjong(void)
{
 int x;
        
 MahjongData=0;
 for(x=0;x<21;x++)
 {  
  if(DTestButton(&MahjongButtons[x]))
   MahjongData|=1<<x;
 }
}

ButtConfig QuizKingButtons[6]=
{
 MK(Q),MK(W),MK(E),MK(R),MK(T),MK(Y)
};

static void UpdateQuizKing(void)
{
 int x;

 QuizKingData=0;

 for(x=0;x<6;x++)
 {
  if(DTestButton(&QuizKingButtons[x]))
   QuizKingData|=1<<x;
 }

}

ButtConfig TopRiderButtons[8]=
{
 MK(Q),MK(W),MK(E),MK(R),MK(T),MK(Y),MK(U),MK(I)
};

static void UpdateTopRider(void)
{
 int x;
 TopRiderData=0;
 for(x=0;x<8;x++)
  if(DTestButton(&TopRiderButtons[x]))
   TopRiderData|=1<<x;
}

ButtConfig FTrainerButtons[12]=
{
                               MK(O),MK(P),MK(BRACKET_LEFT),
                               MK(BRACKET_RIGHT),MK(K),MK(L),MK(SEMICOLON),
                                MK(APOSTROPHE),
                               MK(M),MK(COMMA),MK(PERIOD),MK(SLASH)
};

static void UpdateFTrainer(void)
{
 int x;

 FTrainerData=0;

 for(x=0;x<12;x++)
 {
  if(DTestButton(&FTrainerButtons[x]))
   FTrainerData|=1<<x;
 }
}

int DWaitButton(HWND hParent, const uint8 *text, ButtConfig *bc);
int DWaitSimpleButton(HWND hParent, const uint8 *text);

CFGSTRUCT InputConfig[]={
        ACA(UsrInputType),
        AC(powerpadsc),
        AC(QuizKingButtons),
        AC(FTrainerButtons),
        AC(HyperShotButtons),
        AC(MahjongButtons),
        AC(GamePadConfig),
        AC(fkbmap),
        AC(suborkbmap),
        ENDCFGSTRUCT
};

void InitInputStuff(void)
{
   int x,y;

   KeyboardInitialize();
   InitJoysticks(hAppWnd);

   for(x=0; x<4; x++)
    for(y=0; y<10; y++)
     JoyClearBC(&GamePadConfig[x][y]);

   for(x=0; x<2; x++)
    for(y=0; y<12; y++)    
     JoyClearBC(&powerpadsc[x][y]);

   for(x=0; x<0x48; x++)
    JoyClearBC(&fkbmap[x]);
   for(x=0; x<0x60; x++)
    JoyClearBC(&suborkbmap[x]);

   for(x=0; x<6; x++)
    JoyClearBC(&QuizKingButtons[x]);
   for(x=0; x<12; x++)
    JoyClearBC(&FTrainerButtons[x]);
   for(x=0; x<21; x++)
    JoyClearBC(&MahjongButtons[x]);
   for(x=0; x<4; x++)
    JoyClearBC(&HyperShotButtons[x]);
}



static void FCExp(char *text)
{
        static char *fccortab[12]={"none","arkanoid","shadow","4player","fkb","suborkb",
                    "hypershot","mahjong","quizking","ftrainera","ftrainerb","oekakids"};

        static int fccortabi[12]={SIFC_NONE,SIFC_ARKANOID,SIFC_SHADOW,
                                 SIFC_4PLAYER,SIFC_FKB,SIFC_SUBORKB,SIFC_HYPERSHOT,SIFC_MAHJONG,SIFC_QUIZKING,
                                 SIFC_FTRAINERA,SIFC_FTRAINERB,SIFC_OEKAKIDS};
	int y;
        for(y=0;y<12;y++)
	 if(!strcmp(fccortab[y],text))
	  UsrInputType[2]=fccortabi[y];
}

static char *cortab[6]={"none","gamepad","zapper","powerpada","powerpadb","arkanoid"};
static int cortabi[6]={SI_NONE,SI_GAMEPAD,
                               SI_ZAPPER,SI_POWERPADA,SI_POWERPADB,SI_ARKANOID};

static void Input1(char *text)
{
	int y;

	for(y=0;y<6;y++)
	 if(!strcmp(cortab[y],text))
	  UsrInputType[0]=cortabi[y];
}

static void Input2(char *text)
{
	int y;

	for(y=0;y<6;y++)
	 if(!strcmp(cortab[y],text))
	  UsrInputType[1]=cortabi[y];
}

ARGPSTRUCT InputArgs[]={
	{"-fcexp",0,(void *)FCExp,0x2000},
	{"-input1",0,(void *)Input1,0x2000},
	{"-input2",0,(void *)Input2,0x2000},
	{0,0,0,0}
};


static char *MakeButtString(ButtConfig *bc)
{
 uint32 x; //mbg merge 7/17/06  changed to uint
 char tmpstr[512];
 char *astr;

 tmpstr[0] = 0;

 for(x=0;x<bc->NumC;x++)
 {
  if(x) strcat(tmpstr, ", ");

  if(bc->ButtType[x] == BUTTC_KEYBOARD)
  {
   strcat(tmpstr,"KB: ");
   if(!GetKeyNameText(bc->ButtonNum[x]<<16,tmpstr+strlen(tmpstr),16))
   {
	   switch(bc->ButtonNum[x])
	   {
		case 200: strcpy(tmpstr+strlen(tmpstr),"Up Arrow"); break;
		case 203: strcpy(tmpstr+strlen(tmpstr),"Left Arrow"); break;
		case 205: strcpy(tmpstr+strlen(tmpstr),"Right Arrow"); break;
		case 208: strcpy(tmpstr+strlen(tmpstr),"Down Arrow"); break;
		default: sprintf(tmpstr+strlen(tmpstr),"%03d",bc->ButtonNum[x]); break;
	   }
   }
  }
  else if(bc->ButtType[x] == BUTTC_JOYSTICK)
  {
   strcat(tmpstr,"JS ");
   sprintf(tmpstr+strlen(tmpstr), "%d ", bc->DeviceNum[x]);
   if(bc->ButtonNum[x] & 0x8000)
   {
    char *asel[3]={"x","y","z"};
    sprintf(tmpstr+strlen(tmpstr), "axis %s%s", asel[bc->ButtonNum[x] & 3],(bc->ButtonNum[x]&0x4000)?"-":"+");
   }
   else if(bc->ButtonNum[x] & 0x2000)
   {
    sprintf(tmpstr+strlen(tmpstr), "hat %d:%d", (bc->ButtonNum[x] >> 4)&3,
        bc->ButtonNum[x]&3);
   }
   else
   {
    sprintf(tmpstr+strlen(tmpstr), "button %d", bc->ButtonNum[x] & 127);
   }

  }
 }

 astr=(char*)malloc(strlen(tmpstr) + 1); //mbg merge 7/17/06 added cast
 strcpy(astr,tmpstr);
 return(astr);
}


static int DWBStarted;
static ButtConfig *DWBButtons;
static const uint8 *DWBText;

static HWND die;

static BOOL CALLBACK DWBCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

  switch(uMsg) {
   case WM_DESTROY:
                   die = NULL;                         
                   return(0);
   case WM_TIMER:
                {
                 uint8 devicenum;
                 uint16 buttonnum;
                 GUID guid;

                 if(DoJoyWaitTest(&guid, &devicenum, &buttonnum))
                 {
                  ButtConfig *bc = DWBButtons;
                  char *nstr;
                  int wc;
                  if(DWBStarted)
                  {
                       ButtConfig *bc = DWBButtons;
                       bc->NumC = 0;
                       DWBStarted = 0;
                  }
                  wc = bc->NumC;
                  //FCEU_printf("%d: %d\n",devicenum,buttonnum);
                  bc->ButtType[wc]=BUTTC_JOYSTICK;
                  bc->DeviceNum[wc]=devicenum;
                  bc->ButtonNum[wc]=buttonnum;
                  bc->DeviceInstance[wc] = guid;

                  /* Stop config if the user pushes the same button twice in a row. */
                  if(wc && bc->ButtType[wc]==bc->ButtType[wc-1] && bc->DeviceNum[wc]==bc->DeviceNum[wc-1] &&
                   bc->ButtonNum[wc]==bc->ButtonNum[wc-1])   
                    goto gornk;

                   bc->NumC++;

                   /* Stop config if we reached our maximum button limit. */
                   if(bc->NumC >= MAXBUTTCONFIG)
                   goto gornk;
                   nstr = MakeButtString(bc);
                   SetDlgItemText(hwndDlg, 100, nstr);
                   free(nstr);
                 }
                }
                break;
   case WM_USER + 666:
                      //SetFocus(GetDlgItem(hwndDlg,100));
                      if(DWBStarted)
                      {
                       char *nstr;
                       ButtConfig *bc = DWBButtons;
                       bc->NumC = 0;
                       DWBStarted = 0;
                       nstr = MakeButtString(bc);
                       SetDlgItemText(hwndDlg, 100, nstr);
                       free(nstr);
                      }

                      {
                       ButtConfig *bc = DWBButtons;
                       int wc = bc->NumC;
                       char *nstr;

                       bc->ButtType[wc]=BUTTC_KEYBOARD;
                       bc->DeviceNum[wc]=0;
                       bc->ButtonNum[wc]=lParam&255;

                       /* Stop config if the user pushes the same button twice in a row. */
                       if(wc && bc->ButtType[wc]==bc->ButtType[wc-1] && bc->DeviceNum[wc]==bc->DeviceNum[wc-1] &&
                        bc->ButtonNum[wc]==bc->ButtonNum[wc-1])   
                        goto gornk;

                       bc->NumC++;
                       /* Stop config if we reached our maximum button limit. */
                       if(bc->NumC >= MAXBUTTCONFIG)
                        goto gornk;

                       nstr = MakeButtString(bc);
                       SetDlgItemText(hwndDlg, 100, nstr);
                       free(nstr);
                      }
                      break;
   case WM_INITDIALOG:
                      SetWindowText(hwndDlg, (char*)DWBText); //mbg merge 7/17/06 added cast
                      BeginJoyWait(hwndDlg);
                      SetTimer(hwndDlg,666,25,0);     /* Every 25ms.*/
                      {
                       char *nstr = MakeButtString(DWBButtons);
                       SetDlgItemText(hwndDlg, 100, nstr);
                       free(nstr);
                      }
                      break;
   case WM_CLOSE:
   case WM_QUIT: goto gornk;

   case WM_COMMAND:
                 switch(wParam&0xFFFF)
                 {
                  case 200:
                           {
                            ButtConfig *bc = DWBButtons;                
                            char *nstr;
                            bc->NumC = 0;
                            nstr = MakeButtString(bc);
                            SetDlgItemText(hwndDlg, 100, nstr);
                            free(nstr);
                           }
                           break;
                  case 201:
                         gornk:
                         KillTimer(hwndDlg,666);
                         EndJoyWait(hAppWnd);
                         SetForegroundWindow(GetParent(hwndDlg));
                         DestroyWindow(hwndDlg);
                         break;
                 }
              }
  return 0;
}

int DWaitButton(HWND hParent, const uint8 *text, ButtConfig *bc)
{
 DWBText=text;
 DWBButtons = bc;
 DWBStarted = 1;

 die = CreateDialog(fceu_hInstance, "DWBDIALOG", hParent, DWBCallB);

 EnableWindow(hParent, 0);
 
 ShowWindow(die, 1);

 while(die)
 {
  MSG msg;
  while(PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE))
  {
   if(GetMessage(&msg, 0, 0, 0) > 0)
   {
    if(msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN)
    {
     LPARAM tmpo;

     tmpo=((msg.lParam>>16)&0x7F)|((msg.lParam>>17)&0x80);
     PostMessage(die,WM_USER+666,0,tmpo);
     continue;
    }
    if(msg.message == WM_SYSCOMMAND) continue;
    if(!IsDialogMessage(die, &msg))
    {
     TranslateMessage(&msg);
     DispatchMessage(&msg);
    }
   }
  }
  Sleep(10);
 }
 
 EnableWindow(hParent, 1);
 return 0; //mbg merge TODO 7/17/06  - had to add this return value--is it right?
}

int DWaitSimpleButton(HWND hParent, const uint8 *text)
{
 DWBStarted = 1;
 int ret = 0;

 die = CreateDialog(fceu_hInstance, "DWBDIALOGSIMPLE", hParent, NULL);
 SetWindowText(die, (char*)text); //mbg merge 7/17/06 added cast
 EnableWindow(hParent, 0);
 
 ShowWindow(die, 1);

 while(die)
 {
  MSG msg;
  while(PeekMessage(&msg, 0, 0, 0, PM_NOREMOVE))
  {
   if(GetMessage(&msg, 0, 0, 0) > 0)
   {
    if(msg.message == WM_KEYDOWN || msg.message == WM_SYSKEYDOWN)
    {
     LPARAM tmpo;

     tmpo=((msg.lParam>>16)&0x7F)|((msg.lParam>>17)&0x80);
	 ret = tmpo;
	 goto done;
    }
    if(msg.message == WM_SYSCOMMAND) continue;
    if(!IsDialogMessage(die, &msg))
    {
     TranslateMessage(&msg);
     DispatchMessage(&msg);
    }
   }
  }
  Sleep(10);
 }
done:
 EndDialog(die,0);
 EnableWindow(hParent, 1);

 if(ret == 1) // convert Esc to nothing (why is it 1 and not VK_ESCAPE?)
	 ret = 0;
 return ret;
}


static ButtConfig *DoTBButtons=0;
static const char *DoTBTitle=0;
static int DoTBMax=0;
static int DoTBType=0,DoTBPort=0;

static BOOL CALLBACK DoTBCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch(uMsg) {
   case WM_INITDIALOG:
                      if(DoTBType == SI_GAMEPAD)
                      {
                       char buf[32];
                       sprintf(buf,"Virtual Gamepad %d",DoTBPort+1);
                       SetDlgItemText(hwndDlg, 100,buf);

                       sprintf(buf,"Virtual Gamepad %d",DoTBPort+3);
                       SetDlgItemText(hwndDlg, 101, buf);

                       CheckDlgButton(hwndDlg,400,(eoptions & EO_NOFOURSCORE)?BST_CHECKED:BST_UNCHECKED);
                      }
                      SetWindowText(hwndDlg, DoTBTitle);
                      break;
   case WM_CLOSE:
   case WM_QUIT: goto gornk;

   case WM_COMMAND:
                {
                 int b;
                 b=wParam&0xFFFF;
                 if(b>= 300 && b < (300 + DoTBMax))
                 {
                  char btext[128];
                  btext[0]=0;
                  GetDlgItemText(hwndDlg, b, btext, 128);
                  DWaitButton(hwndDlg, (uint8*)btext,&DoTBButtons[b - 300]); //mbg merge 7/17/06 added cast
                 }
                 else switch(wParam&0xFFFF)
                 {
                  case 1:
                         gornk:

                         if(DoTBType == SI_GAMEPAD)
                         {
                          eoptions &= ~EO_NOFOURSCORE;
                          if(IsDlgButtonChecked(hwndDlg,400)==BST_CHECKED)
                           eoptions|=EO_NOFOURSCORE;
                         }
                         EndDialog(hwndDlg,0);
                         break;
                 }
                }
              }
  return 0;

}

static void DoTBConfig(HWND hParent, const char *text, char *_template, ButtConfig *buttons, int max)
{
  DoTBTitle=text;
  DoTBButtons = buttons;
  DoTBMax = max;
  DialogBox(fceu_hInstance,_template,hParent,DoTBCallB);
}

static BOOL CALLBACK InputConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  static const char *strn[6]={"<none>","Gamepad","Zapper","Power Pad A","Power Pad B","Arkanoid Paddle"};
  static const char *strf[14]=
  {"<none>","Arkanoid Paddle","Hyper Shot gun","4-Player Adapter",
  "Family Keyboard","Subor Keyboard","HyperShot Pads", "Mahjong", "Quiz King Buzzers",
  "Family Trainer A","Family Trainer B", "Oeka Kids Tablet", "Barcode World",
  "Top Rider"};
  static const int haven[6]={0,1,0,1,1,0};
  static const int havef[14]={0,0,0,0, 1,1,0,0, 1,1,1,0, 0,0};
  int x;
  
  switch(uMsg) {
   case WM_INITDIALOG:                
	   SetDlgItemText(hwndDlg,65488,"Select the device you want to be enabled on input ports 1 and 2, and the Famicom expansion port.  You may configure the device listed above each drop-down list by pressing \"Configure\", if applicable.  The device currently being emulated on the each port is listed above the drop down list; loading certain games will override your settings, but only temporarily.  If you select a device to be on the emulated Famicom Expansion Port, you should probably have emulated gamepads on the emulated NES-style input ports.");
                for(x=0;x<2;x++)        
                {
                 int y;

                 for(y=0;y<6;y++)
                  SendDlgItemMessage(hwndDlg,104+x,CB_ADDSTRING,0,(LPARAM)(LPSTR)strn[y]);

                 SendDlgItemMessage(hwndDlg,104+x,CB_SETCURSEL,UsrInputType[x],(LPARAM)(LPSTR)0);
                 EnableWindow(GetDlgItem(hwndDlg,106+x),haven[InputType[x]]);
                 SetDlgItemText(hwndDlg,200+x,(LPTSTR)strn[InputType[x]]);
                }


                {
                 int y;
                 for(y=0;y<13;y++)
                  SendDlgItemMessage(hwndDlg,110,CB_ADDSTRING,0,(LPARAM)(LPSTR)strf[y]);
                 SendDlgItemMessage(hwndDlg,110,CB_SETCURSEL,UsrInputType[2],(LPARAM)(LPSTR)0);                
                 EnableWindow(GetDlgItem(hwndDlg,111),havef[InputType[2]]);
                 SetDlgItemText(hwndDlg,202,(LPTSTR)strf[InputType[2]]);
                }
                
				extern int autoHoldKey, autoHoldClearKey;
				char btext[128];
				if(autoHoldKey)
				{
					if(!GetKeyNameText(autoHoldKey<<16,btext,128))
						sprintf(btext, "KB: %d", autoHoldKey);
				}
				else
					sprintf(btext, "not assigned");
				SetDlgItemText(hwndDlg, 115, btext);

				if(autoHoldClearKey)
				{
					if(!GetKeyNameText(autoHoldClearKey<<16,btext,128))
						sprintf(btext, "KB: %d", autoHoldClearKey);
				}
				else
					sprintf(btext, "not assigned");
				SetDlgItemText(hwndDlg, 116, btext);

                break;
   case WM_CLOSE:
   case WM_QUIT: goto gornk;
   case WM_COMMAND:
                if(HIWORD(wParam)==CBN_SELENDOK)
                {
                 switch(LOWORD(wParam))
                 {
                  case 104:
                  case 105:UsrInputType[LOWORD(wParam)-104]=InputType[LOWORD(wParam)-104]=SendDlgItemMessage(hwndDlg,LOWORD(wParam),CB_GETCURSEL,0,(LPARAM)(LPSTR)0);
                           EnableWindow( GetDlgItem(hwndDlg,LOWORD(wParam)+2),haven[InputType[LOWORD(wParam)-104]]);
                           SetDlgItemText(hwndDlg,200+LOWORD(wParam)-104,(LPTSTR)strn[InputType[LOWORD(wParam)-104]]);
                           break;
                  case 110:UsrInputType[2]=InputType[2]=SendDlgItemMessage(hwndDlg,110,CB_GETCURSEL,0,(LPARAM)(LPSTR)0);
                           EnableWindow(GetDlgItem(hwndDlg,111),havef[InputType[2]]);
                           SetDlgItemText(hwndDlg,202,(LPTSTR)strf[InputType[2]]);
                           break;
                           
                 }

                }
                if(!(wParam>>16))
                switch(wParam&0xFFFF)
                {                 
                 case 111:
                 {
                  const char *text = strf[InputType[2]];
                  DoTBType=DoTBPort=0;

                  switch(InputType[2])
                  {
                   case SIFC_FTRAINERA:
                   case SIFC_FTRAINERB:DoTBConfig(hwndDlg, text, "POWERPADDIALOG", FTrainerButtons, 12); break;
                   case SIFC_FKB:DoTBConfig(hwndDlg, text, "FKBDIALOG",fkbmap,0x48);break;
                   case SIFC_SUBORKB:DoTBConfig(hwndDlg, text, "SUBORKBDIALOG",suborkbmap,0x60);break;
                   case SIFC_QUIZKING:DoTBConfig(hwndDlg, text, "QUIZKINGDIALOG",QuizKingButtons,6);break;
                  }
                 }
                 break;

                 case 107:
                 case 106:
                  {
                   int which=(wParam&0xFFFF)-106;
                   const char *text = strn[InputType[which]];

                   DoTBType=DoTBPort=0;
                   switch(InputType[which])
                   {
                    case SI_GAMEPAD:
                    {
                     ButtConfig tmp[10 + 10];

                     memcpy(tmp, GamePadConfig[which], 10 * sizeof(ButtConfig));
                     memcpy(&tmp[10], GamePadConfig[which+2], 10 * sizeof(ButtConfig));

                     DoTBType=SI_GAMEPAD;
                     DoTBPort=which;
                     DoTBConfig(hwndDlg, text, "GAMEPADDIALOG", tmp, 10 + 10);

                     memcpy(GamePadConfig[which], tmp, 10 * sizeof(ButtConfig));
                     memcpy(GamePadConfig[which+2], &tmp[10], 10 * sizeof(ButtConfig));
                    }
                    break;

                    case SI_POWERPADA:
                    case SI_POWERPADB:
                     DoTBConfig(hwndDlg, text, "POWERPADDIALOG",powerpadsc[which],12);
                     break;
                   }
                  }
                  break;

                 case 112: // auto-hold button
					 {
						char btext[128];
						btext[0]=0;
						GetDlgItemText(hwndDlg, 112, btext, 128);
						int button = DWaitSimpleButton(hwndDlg, (uint8*)btext); //mbg merge 7/17/06 
						if(button)
						{
						    if(!GetKeyNameText(button<<16,btext,128))
								sprintf(btext, "KB: %d", button);
						}
						else
							sprintf(btext, "not assigned");
						extern int autoHoldKey;
						autoHoldKey = button;
						SetDlgItemText(hwndDlg, 115, btext);
					 }
					 break;
                 case 114: // auto-hold clear button
					 {
						char btext[128];
						btext[0]=0;
						GetDlgItemText(hwndDlg, 114, btext, 128);
						int button = DWaitSimpleButton(hwndDlg, (uint8*)btext); //mbg merge 7/17/06 added cast
						if(button)
						{
						    if(!GetKeyNameText(button<<16,btext,128))
								sprintf(btext, "KB: %d", button);
						}
						else
							sprintf(btext, "not assigned");
						extern int autoHoldClearKey;
						autoHoldClearKey = button;
						SetDlgItemText(hwndDlg, 116, btext);
					 }
					 break;

                 case 1:
                        gornk:
                        EndDialog(hwndDlg,0);
                        break;
               }
              }
  return 0;
}

void ConfigInput(HWND hParent)
{
  DialogBox(fceu_hInstance,"INPUTCONFIG",hParent,InputConCallB);
  if(GameInfo)
   InitOtherInput();
}



void DestroyInput(void)
{
 if(lpDI)
 {
  KillJoysticks();
  KeyboardClose();
  IDirectInput7_Release(lpDI);
 }
}

#define CMD_KEY_MASK			0xff
#define CMD_KEY_LSHIFT			(1<<16)
#define CMD_KEY_RSHIFT			(1<<17)
#define CMD_KEY_SHIFT			(CMD_KEY_LSHIFT|CMD_KEY_RSHIFT)
#define CMD_KEY_LCTRL			(1<<18)
#define CMD_KEY_RCTRL			(1<<19)
#define CMD_KEY_CTRL			(CMD_KEY_LCTRL|CMD_KEY_RCTRL)
#define CMD_KEY_LALT			(1<<20)
#define CMD_KEY_RALT			(1<<21)
#define CMD_KEY_ALT				(CMD_KEY_LALT|CMD_KEY_RALT)

int FCEUD_CommandMapping[EMUCMD_MAX];

CFGSTRUCT HotkeyConfig[]={
	AC(FCEUD_CommandMapping),
	ENDCFGSTRUCT
};

static struct
{
	int cmd;
	int key;
} DefaultCommandMapping[]=
{
	{ EMUCMD_RESET, 					SCAN_R | CMD_KEY_CTRL },
	{ EMUCMD_PAUSE, 					SCAN_F2, },
	{ EMUCMD_FRAME_ADVANCE, 			SCAN_TAB, },
	{ EMUCMD_SCREENSHOT,				SCAN_F9 },
	{ EMUCMD_HIDE_MENU_TOGGLE,			SCAN_ESCAPE },
	{ EMUCMD_SPEED_SLOWER, 				SCAN_MINUS, }, // think about these
	{ EMUCMD_SPEED_FASTER, 				SCAN_EQUAL, }, // think about these
	{ EMUCMD_SPEED_TURBO, 				SCAN_GRAVE, }, //tilde
	{ EMUCMD_SAVE_SLOT_0, 				SCAN_0, },
	{ EMUCMD_SAVE_SLOT_1, 				SCAN_1, },
	{ EMUCMD_SAVE_SLOT_2, 				SCAN_2, },
	{ EMUCMD_SAVE_SLOT_3, 				SCAN_3, },
	{ EMUCMD_SAVE_SLOT_4, 				SCAN_4, },
	{ EMUCMD_SAVE_SLOT_5, 				SCAN_5, },
	{ EMUCMD_SAVE_SLOT_6, 				SCAN_6, },
	{ EMUCMD_SAVE_SLOT_7, 				SCAN_7, },
	{ EMUCMD_SAVE_SLOT_8, 				SCAN_8, },
	{ EMUCMD_SAVE_SLOT_9, 				SCAN_9, },
	{ EMUCMD_SAVE_STATE, 				SCAN_F5, },
	{ EMUCMD_LOAD_STATE, 				SCAN_F7, },
	//get feedback from TAS people about these
	/*{ EMUCMD_MOVIE_FRAME_DISPLAY_TOGGLE, 	SCAN_PERIOD, },
	{ EMUCMD_FDS_EJECT_INSERT, 			SCAN_F8, },
	{ EMUCMD_FDS_SIDE_SELECT, 			SCAN_F6, },
	{ EMUCMD_MOVIE_INPUT_DISPLAY_TOGGLE, 	SCAN_COMMA, },
	{ EMUCMD_MOVIE_READONLY_TOGGLE, 	SCAN_8 | CMD_KEY_SHIFT, },*/
	{ EMUCMD_MISC_REWIND, SCAN_R, },
	//mbg 7/31/06 - these have been removed as defaults until we decide whether hotkey philosophy permits them
	//{ EMUCMD_SAVE_STATE_SLOT_0, 		SCAN_F10 | CMD_KEY_SHIFT, },
	//{ EMUCMD_SAVE_STATE_SLOT_1, 		SCAN_F1 | CMD_KEY_SHIFT, },
	//{ EMUCMD_SAVE_STATE_SLOT_2, 		SCAN_F2 | CMD_KEY_SHIFT, },
	//{ EMUCMD_SAVE_STATE_SLOT_3, 		SCAN_F3 | CMD_KEY_SHIFT, },
	//{ EMUCMD_SAVE_STATE_SLOT_4, 		SCAN_F4 | CMD_KEY_SHIFT, },
	//{ EMUCMD_SAVE_STATE_SLOT_5, 		SCAN_F5 | CMD_KEY_SHIFT, },
	//{ EMUCMD_SAVE_STATE_SLOT_6, 		SCAN_F6 | CMD_KEY_SHIFT, },
	//{ EMUCMD_SAVE_STATE_SLOT_7, 		SCAN_F7 | CMD_KEY_SHIFT, },
	//{ EMUCMD_SAVE_STATE_SLOT_8, 		SCAN_F8 | CMD_KEY_SHIFT, },
	//{ EMUCMD_SAVE_STATE_SLOT_9, 		SCAN_F9 | CMD_KEY_SHIFT, },
	//{ EMUCMD_LOAD_STATE_SLOT_0, 		SCAN_F10, },
	//{ EMUCMD_LOAD_STATE_SLOT_1, 		SCAN_F1, },
	//{ EMUCMD_LOAD_STATE_SLOT_2, 		SCAN_F2, },
	//{ EMUCMD_LOAD_STATE_SLOT_3, 		SCAN_F3, },
	//{ EMUCMD_LOAD_STATE_SLOT_4, 		SCAN_F4, },
	//{ EMUCMD_LOAD_STATE_SLOT_5, 		SCAN_F5, },
	//{ EMUCMD_LOAD_STATE_SLOT_6, 		SCAN_F6, },
	//{ EMUCMD_LOAD_STATE_SLOT_7, 		SCAN_F7, },
	//{ EMUCMD_LOAD_STATE_SLOT_8, 		SCAN_F8, },
	//{ EMUCMD_LOAD_STATE_SLOT_9, 		SCAN_F9, },
/*	{ EMUCMD_MOVIE_SLOT_0, 				SCAN_0 | CMD_KEY_ALT, },
	{ EMUCMD_MOVIE_SLOT_1, 				SCAN_1 | CMD_KEY_ALT, },
	{ EMUCMD_MOVIE_SLOT_2, 				SCAN_2 | CMD_KEY_ALT, },
	{ EMUCMD_MOVIE_SLOT_3, 				SCAN_3 | CMD_KEY_ALT, },
	{ EMUCMD_MOVIE_SLOT_4, 				SCAN_4 | CMD_KEY_ALT, },
	{ EMUCMD_MOVIE_SLOT_5, 				SCAN_5 | CMD_KEY_ALT, },
	{ EMUCMD_MOVIE_SLOT_6, 				SCAN_6 | CMD_KEY_ALT, },
	{ EMUCMD_MOVIE_SLOT_7, 				SCAN_7 | CMD_KEY_ALT, },
	{ EMUCMD_MOVIE_SLOT_8, 				SCAN_8 | CMD_KEY_ALT, },
	{ EMUCMD_MOVIE_SLOT_9, 				SCAN_9 | CMD_KEY_ALT, },
	{ EMUCMD_MOVIE_RECORD, 				SCAN_F5 | CMD_KEY_ALT, },
	{ EMUCMD_MOVIE_REPLAY, 				SCAN_F7 | CMD_KEY_ALT, },*/
};

#define NUM_DEFAULT_MAPPINGS		(sizeof(DefaultCommandMapping)/sizeof(DefaultCommandMapping[0]))

void ApplyDefaultCommandMapping(void)
{
	int i;

	memset(FCEUD_CommandMapping, 0, sizeof(FCEUD_CommandMapping));
	for(i=0; i<NUM_DEFAULT_MAPPINGS; ++i)
		FCEUD_CommandMapping[DefaultCommandMapping[i].cmd] = DefaultCommandMapping[i].key;
}

int FCEUD_TestCommandState(int c)
{
	int cmd=FCEUD_CommandMapping[c];
	int cmdmask=cmd&CMD_KEY_MASK;

	// allow certain commands be affected by key repeat
	if(c == EMUCMD_FRAME_ADVANCE/*
	|| c == EMUCMD_SOUND_VOLUME_UP
	|| c == EMUCMD_SOUND_VOLUME_DOWN
	|| c == EMUCMD_SPEED_SLOWER
	|| c == EMUCMD_SPEED_FASTER*/)
	{
		keys=GetKeyboard(); 
/*		if((cmdmask & CMD_KEY_LALT) == CMD_KEY_LALT
		|| (cmdmask & CMD_KEY_RALT) == CMD_KEY_RALT
		|| (cmdmask & CMD_KEY_LALT) == CMD_KEY_LALT
		|| (cmdmask & CMD_KEY_LCTRL) == CMD_KEY_LCTRL
		|| (cmdmask & CMD_KEY_RCTRL) == CMD_KEY_RCTRL
		|| (cmdmask & CMD_KEY_LSHIFT) == CMD_KEY_LSHIFT
		|| (cmdmask & CMD_KEY_RSHIFT) == CMD_KEY_RSHIFT)*/
			keys_nr=GetKeyboard();
//		else
//			keys_nr=GetKeyboard_nr();
	}
	else if(c != EMUCMD_SPEED_TURBO) // TODO: this should be made more general by detecting if the command has an "off" function, but right now Turbo is the only command that has it
	{
		keys=GetKeyboard_jd();
		keys_nr=GetKeyboard_nr(); 
	}
	else
	{
		keys=GetKeyboard_nr(); 
		keys_nr=GetKeyboard_nr();
	}

	/* test CTRL, SHIFT, ALT */
	if (cmd & CMD_KEY_ALT)
	{
		int ctlstate =	(cmd & CMD_KEY_LALT) ? keys_nr[SCAN_LEFTALT] : 0;
		ctlstate |=		(cmd & CMD_KEY_RALT) ? keys_nr[SCAN_RIGHTALT] : 0;
		if (!ctlstate)
			return 0;
	}
	else if((cmdmask != SCAN_LEFTALT && keys_nr[SCAN_LEFTALT]) || (cmdmask != SCAN_RIGHTALT && keys_nr[SCAN_RIGHTALT]))
		return 0;

	if (cmd & CMD_KEY_CTRL)
	{
		int ctlstate =	(cmd & CMD_KEY_LCTRL) ? keys_nr[SCAN_LEFTCONTROL] : 0;
		ctlstate |=		(cmd & CMD_KEY_RCTRL) ? keys_nr[SCAN_RIGHTCONTROL] : 0;
		if (!ctlstate)
			return 0;
	}
	else if((cmdmask != SCAN_LEFTCONTROL && keys_nr[SCAN_LEFTCONTROL]) || (cmdmask != SCAN_RIGHTCONTROL && keys_nr[SCAN_RIGHTCONTROL]))
		return 0;

	if (cmd & CMD_KEY_SHIFT)
	{
		int ctlstate =	(cmd & CMD_KEY_LSHIFT) ? keys_nr[SCAN_LEFTSHIFT] : 0;
		ctlstate |=		(cmd & CMD_KEY_RSHIFT) ? keys_nr[SCAN_RIGHTSHIFT] : 0;
		if (!ctlstate)
			return 0;
	}
	else if((cmdmask != SCAN_LEFTSHIFT && keys_nr[SCAN_LEFTSHIFT]) || (cmdmask != SCAN_RIGHTSHIFT && keys_nr[SCAN_RIGHTSHIFT]))
		return 0;

	return keys[cmdmask] ? 1 : 0;
}

static const char* ScanNames[256]=
{
	/* 0x00-0x0f */ 0, "Escape", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0", "Minus", "Equals", "Backspace", "Tab",
	/* 0x10-0x1f */ "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P", "[", "]", "Enter", "Left Ctrl", "A", "S",
	/* 0x20-0x2f */ "D", "F", "G", "H", "J", "K", "L", "Semicolon", "Apostrophe", "Tilde", "Left Shift", "Backslash", "Z", "X", "C", "V",
	/* 0x30-0x3f */	"B", "N", "M", "Comma", "Period", "Slash", "Right Shift", "Numpad *", "Left Alt", "Space", "Caps Lock", "F1", "F2", "F3", "F4", "F5",
	/* 0x40-0x4f */ "F6", "F7", "F8", "F9", "F10", "NumLock", "ScrollLock", "Numpad 7", "Numpad 8", "Numpad 9", "Numpad Minus", "Numpad 4", "Numpad 5", "Numpad 6", "Numpad Plus", "Numpad 1",
	/* 0x50-0x5f */	"Numpad 2", "Numpad 3", "Numpad 0", "Numpad Period", 0, 0, "Backslash", "F11", "F12", 0, 0, 0, 0, 0, 0, 0,
	/* 0x60-0x6f */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x70-0x7f */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x80-0x8f */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0x90-0x9f */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "Numpad Enter", "Right Ctrl", 0, 0,
	/* 0xa0-0xaf */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xb0-0xbf */ 0, 0, 0, 0, 0, "Numpad Divide", 0, "PrintScrn", "Right Alt", 0, 0, 0, 0, 0, 0, 0,
	/* 0xc0-0xcf */ 0, 0, 0, 0, 0, "Pause", 0, "Home", "Up Arrow", "PgUp", 0, "Left Arrow", 0, "Right Arrow", 0, "End",
	/* 0xd0-0xdf */ "Down Arrow", "PgDn", "Ins", "Del", 0, 0, 0, 0, 0, 0, 0, "Left Win", "Right Win", "AppMenu", 0, 0,
	/* 0xe0-0xef */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 0xf0-0xff */ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static const char* GetKeyName(int code)
{
	static char name[16];

	code &= 0xff;
	if(ScanNames[code])
		return ScanNames[code];

	sprintf(name, "Key 0x%.2x", code);
	return name;
}

static char* GetKeyComboName(int c)
{
	static char text[80];

	text[0]='\0';
	if(!c)
		return text;

	if ((c & CMD_KEY_CTRL) == CMD_KEY_CTRL)			strcat(text, "Ctrl + ");
	else if ((c & CMD_KEY_CTRL) == CMD_KEY_LCTRL)	strcat(text, "Left Ctrl + ");
	else if ((c & CMD_KEY_CTRL) == CMD_KEY_RCTRL)	strcat(text, "Right Ctrl + ");
	if ((c & CMD_KEY_ALT) == CMD_KEY_ALT)			strcat(text, "Alt + ");
	else if ((c & CMD_KEY_ALT) == CMD_KEY_LALT)		strcat(text, "Left Alt + ");
	else if ((c & CMD_KEY_ALT) == CMD_KEY_RALT)		strcat(text, "Right Alt + ");
	if ((c & CMD_KEY_SHIFT) == CMD_KEY_SHIFT)		strcat(text, "Shift + ");
	else if ((c & CMD_KEY_SHIFT) == CMD_KEY_LSHIFT)	strcat(text, "Left Shift + ");
	else if ((c & CMD_KEY_SHIFT) == CMD_KEY_RSHIFT)	strcat(text, "Right Shift + ");

	strcat(text, GetKeyName(c & CMD_KEY_MASK));

	return text;
}

struct INPUTDLGTHREADARGS
{
	HANDLE hThreadExit;
	HWND hwndDlg;
};

static DWORD WINAPI NewInputDialogThread(LPVOID lpvArg)
{
	struct INPUTDLGTHREADARGS* args = (struct INPUTDLGTHREADARGS*)lpvArg;

	while (args->hThreadExit)
	{
		if (WaitForSingleObject(args->hThreadExit, 20) == WAIT_OBJECT_0)
			break;

		// Poke our owner dialog periodically.
		PostMessage(args->hwndDlg, WM_USER, 0, 0);
	}

	return 0;
}

static int GetKeyPressed()
{
	int key=0;
	int i;

	keys_nr=GetKeyboard_nr();

	for(i=0; i<256 && !key; ++i)
	{
		if(_keyonly(i))
			key=i;
	}
	return key;
}

static int NothingPressed()
{
	int i;

	keys_nr=GetKeyboard_nr();

	for(i=0; i<256; ++i)
	{
		if(keys_nr[i])
			return 0;
	}
	return 1;
}

static int GetKeyMeta(int key)
{
	int meta = key & (~CMD_KEY_MASK);
	switch(key & CMD_KEY_MASK)
	{
	case SCAN_LEFTCONTROL:
	case SCAN_RIGHTCONTROL:		return CMD_KEY_CTRL | meta;
	case SCAN_LEFTALT:
	case SCAN_RIGHTALT:			return CMD_KEY_ALT | meta;
	case SCAN_LEFTSHIFT:
	case SCAN_RIGHTSHIFT:		return CMD_KEY_SHIFT | meta;
	default:
		break;
	}

	return meta;
}

static void ClearExtraMeta(int* key)
{
	switch((*key)&0xff)
	{
	case SCAN_LEFTCONTROL:
	case SCAN_RIGHTCONTROL:		*key &= ~(CMD_KEY_CTRL);  break;
	case SCAN_LEFTALT:
	case SCAN_RIGHTALT:			*key &= ~(CMD_KEY_ALT);  break;
	case SCAN_LEFTSHIFT:
	case SCAN_RIGHTSHIFT:		*key &= ~(CMD_KEY_SHIFT);  break;
	default:
		break;
	}
}

static BOOL CALLBACK ChangeInputDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HANDLE hThread = NULL;
	static DWORD dwThreadId = 0;
	static struct INPUTDLGTHREADARGS threadargs;
	static int key=0;

	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			// Start the message thread.
			threadargs.hThreadExit = CreateEvent(NULL, TRUE, FALSE, NULL);
			threadargs.hwndDlg = hwndDlg;
			hThread = CreateThread(NULL, 0, NewInputDialogThread, (LPVOID)&threadargs, 0, &dwThreadId);
			key=0;
			memset(keyonce, 0, sizeof(keyonce));
			KeyboardSetBackgroundAccess(1);
			SetFocus(GetDlgItem(hwndDlg, 100));
		}
		return FALSE;

	case WM_COMMAND:
		if(LOWORD(wParam) == 202 && HIWORD(wParam) == BN_CLICKED)
		{
			key=0;
			PostMessage(hwndDlg, WM_USER+1, 0, 0);		// Send quit message.
		}
		else if(LOWORD(wParam) == 200 && HIWORD(wParam) == BN_CLICKED)
		{
			key=-1;
			PostMessage(hwndDlg, WM_USER+1, 0, 0);		// Send quit message.
		}

		break;

	case WM_USER:
		{
			// Our thread sent us a timer signal.
			int newkey;
			int meta=GetKeyMeta(key);

			KeyboardUpdateState();

			if((newkey=GetKeyPressed()) != 0)
			{
				key = newkey | meta;
				ClearExtraMeta(&key);
				SetDlgItemText(hwndDlg, 100, GetKeyComboName(key));
			}
			else if(NothingPressed() && key)
				PostMessage(hwndDlg, WM_USER+1, 0, 0);		// Send quit message.
		}
		break;

	case WM_USER+1:
		{
			// Done with keyboard.
			KeyboardSetBackgroundAccess(0);

			// Kill the thread.
			SetEvent(threadargs.hThreadExit);
			WaitForSingleObject(hThread, INFINITE);
			CloseHandle(hThread);
			CloseHandle(threadargs.hThreadExit);

			// End the dialog.
			EndDialog(hwndDlg, key);
			return TRUE;
		}

	default:
		break;
	}

	return FALSE;
}

static int* ConflictTable=0;

static int ShouldDisplayMapping(int mapn, int filter)
{
	//mbg merge 7/17/06 changed to if..elseif
	if(filter==0) /* No filter */
		return 1;
	else if(filter <= EMUCMDTYPE_MAX) /* Filter by type */
		return (FCEUI_CommandTable[mapn].type == filter-1);	
	else if(filter == EMUCMDTYPE_MAX+1) /* Assigned */
		return FCEUD_CommandMapping[mapn];
	else if(filter == EMUCMDTYPE_MAX+2) /* Unassigned */
		return !(FCEUD_CommandMapping[mapn]);
	else if(filter == EMUCMDTYPE_MAX+3) /* Conflicts */
		return ConflictTable[mapn];
	else 
		return 0;
}

static void PopulateMappingDisplay(HWND hwndDlg)
{
	LVITEM lvi;
	int i;
	int idx;
	HWND hwndListView = GetDlgItem(hwndDlg, 1003);
	int num=SendMessage(hwndListView, LVM_GETITEMCOUNT, 0, 0);
	int filter = (int)SendDlgItemMessage(hwndDlg, 300, CB_GETCURSEL, 0, 0);

	// Delete everything in the current display.
	for(i=num; i>0; --i)
		SendMessage(hwndListView, LVM_DELETEITEM, (WPARAM)i-1, 0);

	// Get the filter type.
	ConflictTable=0;
	if (filter == EMUCMDTYPE_MAX+3)
	{
		// Set up the conflict table.
		ConflictTable=(int*)malloc(sizeof(int)*EMUCMD_MAX);
		memset(ConflictTable, 0, sizeof(int)*EMUCMD_MAX);
		for(i=0; i<EMUCMD_MAX-1; ++i)
		{
			int j;
			for(j=i+1; j<EMUCMD_MAX; ++j)
			{
				if(FCEUD_CommandMapping[i] &&
					FCEUD_CommandMapping[i] == FCEUD_CommandMapping[j])
				{
					ConflictTable[i]=1;
					ConflictTable[j]=1;
				}
			}
		}
	}

	// Populate display.
	for(i=0, idx=0; i<EMUCMD_MAX; ++i)
	{
		if(ShouldDisplayMapping(i, filter))
		{
			memset(&lvi, 0, sizeof(lvi));
			lvi.mask = LVIF_TEXT | LVIF_PARAM;
			lvi.iItem = idx;
			lvi.iSubItem = 0;
			lvi.pszText = (char*)FCEUI_CommandTypeNames[FCEUI_CommandTable[i].type];
			lvi.lParam = (LPARAM)i;

			SendMessage(hwndListView, LVM_INSERTITEM, (WPARAM)0, (LPARAM)&lvi);

			memset(&lvi, 0, sizeof(lvi));
			lvi.mask = LVIF_TEXT;
			lvi.iItem = idx;
			lvi.iSubItem = 1;
			lvi.pszText = FCEUI_CommandTable[i].name;

			SendMessage(hwndListView, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvi);

			memset(&lvi, 0, sizeof(lvi));
			lvi.mask = LVIF_TEXT;
			lvi.iItem = idx;
			lvi.iSubItem = 2;
			lvi.pszText = GetKeyComboName(FCEUD_CommandMapping[i]);

			SendMessage(hwndListView, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvi);

			idx++;
		}
	}

	if(ConflictTable)
		free(ConflictTable);
}

static BOOL CALLBACK MapInputDialogProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
	case WM_INITDIALOG:
		{
			HWND hwndListView = GetDlgItem(hwndDlg, 1003);
			LVCOLUMN lv;
			//LVITEM lvi; //mbg merge 7/17/06 removed
			int i;

			// Set full row select.
			SendMessage(hwndListView, LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT, LVS_EX_FULLROWSELECT);

			// Init ListView columns.
			memset(&lv, 0, sizeof(lv));
			lv.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
			lv.fmt = LVCFMT_LEFT;
			lv.pszText = "Type";
			lv.cx = 40;

			SendMessage(hwndListView, LVM_INSERTCOLUMN, (WPARAM)0, (LPARAM)&lv);

			memset(&lv, 0, sizeof(lv));
			lv.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH;
			lv.fmt = LVCFMT_LEFT;
			lv.pszText = "Command";
			lv.cx = 180;

			SendMessage(hwndListView, LVM_INSERTCOLUMN, (WPARAM)1, (LPARAM)&lv);

			memset(&lv, 0, sizeof(lv));
			lv.mask = LVCF_FMT | LVCF_TEXT;
			lv.fmt = LVCFMT_LEFT;
			lv.pszText = "Input";

			SendMessage(hwndListView, LVM_INSERTCOLUMN, (WPARAM)2, (LPARAM)&lv);

			// Populate the filter combobox.
			SendDlgItemMessage(hwndDlg, 300, CB_INSERTSTRING, 0, (LPARAM)"None");
			for(i=0; i<EMUCMDTYPE_MAX; ++i)
				SendDlgItemMessage(hwndDlg, 300, CB_INSERTSTRING, i+1, (LPARAM)FCEUI_CommandTypeNames[i]);
			SendDlgItemMessage(hwndDlg, 300, CB_INSERTSTRING, ++i, (LPARAM)"Assigned");
			SendDlgItemMessage(hwndDlg, 300, CB_INSERTSTRING, ++i, (LPARAM)"Unassigned");
			SendDlgItemMessage(hwndDlg, 300, CB_INSERTSTRING, ++i, (LPARAM)"Conflicts");

			SendDlgItemMessage(hwndDlg, 300, CB_SETCURSEL, 0, 0);				// Default filter is "none".

			// Now populate the mapping display.
			PopulateMappingDisplay(hwndDlg);

			// Autosize last column.
			SendMessage(hwndListView, LVM_SETCOLUMNWIDTH, (WPARAM)2, MAKELPARAM(LVSCW_AUTOSIZE_USEHEADER, 0));

		}
		return FALSE;

	case WM_COMMAND:
		if(HIWORD(wParam) == CBN_SELCHANGE)
		{
			PopulateMappingDisplay(hwndDlg);
			break;
		}

		switch(LOWORD(wParam))
		{
		case IDOK:
			EndDialog(hwndDlg, 1);
			return TRUE;

		case IDCANCEL:
			EndDialog(hwndDlg, 0);
			return TRUE;

		case 200:
			ApplyDefaultCommandMapping();
			PopulateMappingDisplay(hwndDlg);
			return TRUE;

		default:
			break;
		}
		break;

	case WM_NOTIFY:
		switch(LOWORD(wParam))
		{
		case 1003:
			if (lParam)
			{
				HWND hwndListView = GetDlgItem(hwndDlg, 1003);
				NMHDR* pnm = (NMHDR*)lParam;
				if (pnm->code == LVN_ITEMACTIVATE)
				{
					int nSel = SendMessage(hwndListView, LVM_GETNEXTITEM, (WPARAM)-1, LVNI_SELECTED);
					if (nSel != -1)
					{
						int nCmd;
						int nRet;
						LVITEM lvi;

						// Get the corresponding input
						memset(&lvi, 0, sizeof(lvi));
						lvi.mask = LVIF_PARAM;
						lvi.iItem = nSel;
						lvi.iSubItem = 0;
						SendMessage(hwndListView, LVM_GETITEM, 0, (LPARAM)&lvi);
						nCmd = lvi.lParam;

						nRet = DialogBox(fceu_hInstance,"NEWINPUT",hwndListView,ChangeInputDialogProc);
						if (nRet)
						{
							// nRet will be -1 when the user selects "clear".
							FCEUD_CommandMapping[nCmd] = (nRet<0) ? 0 : nRet;

							memset(&lvi, 0, sizeof(lvi));
							lvi.mask = LVIF_TEXT;
							lvi.iItem = nSel;
							lvi.iSubItem = 2;
							lvi.pszText = GetKeyComboName(FCEUD_CommandMapping[nCmd]);
							SendMessage(hwndListView, LVM_SETITEM, (WPARAM)0, (LPARAM)&lvi);
						}
					}
				}
				return TRUE;
			}
			break;

		default:
			break;
		}
		break;

	default:
		break;
	}

	return FALSE;
}

void MapInput(void)
{
  // Make a backup of the current mappings, in case the user changes their mind.
  int* backupmapping = (int*)malloc(sizeof(FCEUD_CommandMapping));
  memcpy(backupmapping, FCEUD_CommandMapping, sizeof(FCEUD_CommandMapping));

  if(!DialogBox(fceu_hInstance,"MAPINPUT",hAppWnd,MapInputDialogProc))
	memcpy(FCEUD_CommandMapping, backupmapping, sizeof(FCEUD_CommandMapping));

  free(backupmapping);
}

void FCEUD_TurboOn(void)
{
	//NoWaiting|=1;
	turbo = true;
}

void FCEUD_TurboOff(void)
{
	//NoWaiting&=~1;
	turbo = false;
}

