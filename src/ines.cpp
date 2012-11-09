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
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "types.h"
#include "x6502.h"
#include "fceu.h"
#include "cart.h"
#include "ppu.h"

#define INESPRIV
#include "ines.h"
#include "unif.h"
#include "state.h"
#include "file.h"
#include "utils/general.h"
#include "utils/memory.h"
#include "utils/crc32.h"
#include "utils/md5.h"
#include "utils/xstring.h"
#include "cheat.h"
#include "vsuni.h"
#include "driver.h"

extern SFORMAT FCEUVSUNI_STATEINFO[];

//mbg merge 6/29/06 - these need to be global
uint8 *trainerpoo=0;
uint8 *ROM = NULL;
uint8 *VROM = NULL;
iNES_HEADER head ;



static CartInfo iNESCart;

uint8 iNESMirroring=0;
uint16 iNESCHRBankList[8]={0,0,0,0,0,0,0,0};
int32 iNESIRQLatch=0,iNESIRQCount=0;
uint8 iNESIRQa=0;

uint32 ROM_size=0;
uint32 VROM_size=0;
char LoadedRomFName[2048]; //mbg merge 7/17/06 added

static int CHRRAMSize = -1;
static void iNESPower(void);
static int NewiNES_Init(int num);

void (*MapClose)(void);
void (*MapperReset)(void);

static int MapperNo=0;

/*  MapperReset() is called when the NES is reset(with the reset button).
Mapperxxx_init is called when the NES has been powered on.
*/


static DECLFR(TrainerRead)
{
	return(trainerpoo[A&0x1FF]);
}

static void iNES_ExecPower()
{
	if(CHRRAMSize != -1)
		FCEU_MemoryRand(VROM,CHRRAMSize);

	if(iNESCart.Power)
		iNESCart.Power();

	if(trainerpoo)
	{
		int x;
		for(x=0;x<512;x++)
		{
			X6502_DMW(0x7000+x,trainerpoo[x]);
			if(X6502_DMR(0x7000+x)!=trainerpoo[x])
			{
				SetReadHandler(0x7000,0x71FF,TrainerRead);
				break;
			}
		}
	}
}

void iNESGI(GI h) //bbit edited: removed static keyword
{
	switch(h)
	{
	case GI_RESETSAVE:
		FCEU_ClearGameSave(&iNESCart);
		break;

	case GI_RESETM2:
		if(MapperReset)
			MapperReset();
		if(iNESCart.Reset)
			iNESCart.Reset();
		break;
	case GI_POWER:
		iNES_ExecPower();

		break;
	case GI_CLOSE:
		{
			FCEU_SaveGameSave(&iNESCart);

			if(iNESCart.Close) iNESCart.Close();
			if(ROM) {free(ROM); ROM = NULL;}
			if(VROM) {free(VROM); VROM = NULL;}
			if(MapClose) MapClose();
			if(trainerpoo) {FCEU_gfree(trainerpoo);trainerpoo=0;}
		}
		break;
	}
}

uint32 iNESGameCRC32=0;

struct CRCMATCH  {
	uint32 crc;
	char *name;
};

struct INPSEL {
	uint32 crc32;
	ESI input1;
	ESI input2;
	ESIFC inputfc;
};

static void SetInput(void)
{
	static struct INPSEL moo[]=
	{
		{0x29de87af,	SI_GAMEPAD,		SI_GAMEPAD,		SIFC_FTRAINERB	},	// Aerobics Studio
		{0xd89e5a67,	SI_UNSET,		SI_UNSET,		SIFC_ARKANOID	},	// Arkanoid (J)
		{0x0f141525,	SI_UNSET,		SI_UNSET,		SIFC_ARKANOID	},	// Arkanoid 2(J)
		{0x32fb0583,	SI_UNSET,		SI_ARKANOID,	SIFC_NONE		},	// Arkanoid(NES)
		{0x60ad090a,	SI_GAMEPAD,		SI_GAMEPAD,		SIFC_FTRAINERA	},	// Athletic World
		{0x48ca0ee1,	SI_GAMEPAD,		SI_GAMEPAD,		SIFC_BWORLD		},	// Barcode World
		{0x4318a2f8,	SI_UNSET,		SI_ZAPPER,		SIFC_NONE		},	// Barker Bill's Trick Shooting
		{0x6cca1c1f,	SI_GAMEPAD,		SI_GAMEPAD,		SIFC_FTRAINERB	},	// Dai Undoukai
		{0x24598791,	SI_UNSET,		SI_ZAPPER,		SIFC_NONE		},	// Duck Hunt
		{0xd5d6eac4,	SI_UNSET,		SI_UNSET,		SIFC_SUBORKB	},	// Edu (As)
		{0xe9a7fe9e,	SI_UNSET,		SI_MOUSE,		SIFC_NONE		},	// Educational Computer 2000
		{0x8f7b1669,	SI_UNSET,		SI_UNSET,		SIFC_SUBORKB	},	// FP BASIC 3.3 by maxzhou88
		{0xf7606810,	SI_UNSET,		SI_UNSET,		SIFC_FKB		},	// Family BASIC 2.0A
		{0x895037bc,	SI_UNSET,		SI_UNSET,		SIFC_FKB		},	// Family BASIC 2.1a
		{0xb2530afc,	SI_UNSET,		SI_UNSET,		SIFC_FKB		},	// Family BASIC 3.0
		{0xea90f3e2,	SI_GAMEPAD,		SI_GAMEPAD,		SIFC_FTRAINERB	},	// Family Trainer:  Running Stadium
		{0xbba58be5,	SI_GAMEPAD,		SI_GAMEPAD,		SIFC_FTRAINERB	},	// Family Trainer: Manhattan Police
		{0x3e58a87e,	SI_UNSET,		SI_ZAPPER,		SIFC_NONE		},	// Freedom Force
		{0xd9f45be9,	SI_GAMEPAD,		SI_GAMEPAD,		SIFC_QUIZKING	},	// Gimme a Break ...
		{0x1545bd13,	SI_GAMEPAD,		SI_GAMEPAD,		SIFC_QUIZKING	},	// Gimme a Break ... 2
		{0x4e959173,	SI_UNSET,		SI_ZAPPER,		SIFC_NONE		},	// Gotcha! - The Sport!
		{0xbeb8ab01,	SI_UNSET,		SI_ZAPPER,		SIFC_NONE		},	// Gumshoe
		{0xff24d794,	SI_UNSET,		SI_ZAPPER,		SIFC_NONE		},	// Hogan's Alley
		{0x21f85681,	SI_GAMEPAD,		SI_GAMEPAD,		SIFC_HYPERSHOT	},	// Hyper Olympic (Gentei Ban)
		{0x980be936,	SI_GAMEPAD,		SI_GAMEPAD,		SIFC_HYPERSHOT	},	// Hyper Olympic
		{0x915a53a7,	SI_GAMEPAD,		SI_GAMEPAD,		SIFC_HYPERSHOT	},	// Hyper Sports
		{0x9fae4d46,	SI_GAMEPAD,		SI_GAMEPAD,		SIFC_MAHJONG	},	// Ide Yousuke Meijin no Jissen Mahjong
		{0x7b44fb2a,	SI_GAMEPAD,		SI_GAMEPAD,		SIFC_MAHJONG	},	// Ide Yousuke Meijin no Jissen Mahjong 2
		{0x2f128512,	SI_GAMEPAD,		SI_GAMEPAD,		SIFC_FTRAINERA	},	// Jogging Race
		{0xbb33196f,	SI_UNSET,		SI_UNSET,		SIFC_FKB		},	// Keyboard Transformer
		{0x8587ee00,	SI_UNSET,		SI_UNSET,		SIFC_FKB		},	// Keyboard Transformer
		{0x543ab532,	SI_UNSET,		SI_UNSET,		SIFC_SUBORKB	},	// LIKO Color Lines
		{0x368c19a8,	SI_UNSET,		SI_UNSET,		SIFC_SUBORKB	},	// LIKO Study Cartridge
		{0x5ee6008e,	SI_UNSET,		SI_ZAPPER,		SIFC_NONE		},	// Mechanized Attack
		{0x370ceb65,	SI_GAMEPAD,		SI_GAMEPAD,		SIFC_FTRAINERB	},	// Meiro Dai Sakusen
		{0x3a1694f9,	SI_GAMEPAD,		SI_GAMEPAD,		SIFC_4PLAYER	},	// Nekketsu Kakutou Densetsu
		{0x9d048ea4,	SI_GAMEPAD,		SI_GAMEPAD,		SIFC_OEKAKIDS	},	// Oeka Kids
		{0x2a6559a1,	SI_UNSET,		SI_ZAPPER,		SIFC_NONE		},	// Operation Wolf (J)
		{0xedc3662b,	SI_UNSET,		SI_ZAPPER,		SIFC_NONE		},	// Operation Wolf
		{0x912989dc,	SI_UNSET,		SI_UNSET,		SIFC_FKB		},	// Playbox BASIC
		{0x9044550e,	SI_GAMEPAD,		SI_GAMEPAD,		SIFC_FTRAINERA	},	// Rairai Kyonshizu
		{0xea90f3e2,	SI_GAMEPAD,		SI_GAMEPAD,		SIFC_FTRAINERB	},	// Running Stadium
		{0x851eb9be,	SI_GAMEPAD,		SI_ZAPPER,		SIFC_NONE		},	// Shooting Range
		{0x6435c095,	SI_GAMEPAD,		SI_POWERPADB,	SIFC_UNSET		},	// Short Order/Eggsplode
		{0xc043a8df,	SI_UNSET,		SI_MOUSE,		SIFC_NONE		},	// Shu Qi Yu - Shu Xue Xiao Zhuan Yuan (Ch)
		{0x2cf5db05,	SI_UNSET,		SI_MOUSE,		SIFC_NONE		},	// Shu Qi Yu - Zhi Li Xiao Zhuan Yuan (Ch)
		{0xad9c63e2,	SI_GAMEPAD,		SI_UNSET,		SIFC_SHADOW		},	// Space Shadow
		{0x61d86167,	SI_GAMEPAD,		SI_POWERPADB,	SIFC_UNSET		},	// Street Cop
		{0xabb2f974,	SI_UNSET,		SI_UNSET,		SIFC_SUBORKB	},	// Study and Game 32-in-1
		{0x41ef9ac4,	SI_UNSET,		SI_UNSET,		SIFC_SUBORKB	},	// Subor
		{0x8b265862,	SI_UNSET,		SI_UNSET,		SIFC_SUBORKB	},	// Subor
		{0x82f1fb96,	SI_UNSET,		SI_UNSET,		SIFC_SUBORKB	},	// Subor 1.0 Russian
		{0x9f8f200a,	SI_GAMEPAD,		SI_GAMEPAD,		SIFC_FTRAINERA	},	// Super Mogura Tataki!! - Pokkun Moguraa
		{0xd74b2719,	SI_GAMEPAD,		SI_POWERPADB,	SIFC_UNSET		},	// Super Team Games
		{0x74bea652,	SI_GAMEPAD,		SI_ZAPPER,		SIFC_NONE		},	// Supergun 3-in-1
		{0x5e073a1b,	SI_UNSET,		SI_UNSET,		SIFC_SUBORKB	},	// Supor English (Chinese)
		{0x589b6b0d,	SI_UNSET,		SI_UNSET,		SIFC_SUBORKB	},	// SuporV20
		{0x41401c6d,	SI_UNSET,		SI_UNSET,		SIFC_SUBORKB	},	// SuporV40
		{0x23d17f5e,	SI_GAMEPAD,		SI_ZAPPER,		SIFC_NONE		},	// The Lone Ranger
		{0xc3c0811d,	SI_GAMEPAD,		SI_GAMEPAD,		SIFC_OEKAKIDS	},	// The two "Oeka Kids" games
		{0xde8fd935,	SI_UNSET,		SI_ZAPPER,		SIFC_NONE		},	// To the Earth
		{0x47232739,	SI_GAMEPAD,		SI_GAMEPAD,		SIFC_TOPRIDER	},	// Top Rider
		{0x8a12a7d9,	SI_GAMEPAD,		SI_GAMEPAD,		SIFC_FTRAINERB	},	// Totsugeki Fuuun Takeshi Jou
		{0xb8b9aca3,	SI_UNSET,		SI_ZAPPER,		SIFC_NONE		},	// Wild Gunman
		{0x5112dc21,	SI_UNSET,		SI_ZAPPER,		SIFC_NONE		},	// Wild Gunman
		{0xaf4010ea,	SI_GAMEPAD,		SI_POWERPADB,	SIFC_UNSET		},	// World Class Track Meet
		{0x00000000,	SI_UNSET,		SI_UNSET,		SIFC_UNSET		}
	};
	int x=0;

	while(moo[x].input1>=0 || moo[x].input2>=0 || moo[x].inputfc>=0)
	{
		if(moo[x].crc32==iNESGameCRC32)
		{
			GameInfo->input[0]=moo[x].input1;
			GameInfo->input[1]=moo[x].input2;
			GameInfo->inputfc=moo[x].inputfc;
			break;
		}
		x++;
	}
}
#define INESB_INCOMPLETE  1
#define INESB_CORRUPT     2
#define INESB_HACKED      4

struct BADINF {
	uint64 md5partial;
	char *name;
	uint32 type;
};


static struct BADINF BadROMImages[]=
{
#include "ines-bad.h"
};

void CheckBad(uint64 md5partial)
{
	int x;

	x=0;
	//printf("0x%llx\n",md5partial);
	while(BadROMImages[x].name)
	{
		if(BadROMImages[x].md5partial == md5partial)
		{
			FCEU_PrintError("The copy game you have loaded, \"%s\", is bad, and will not work properly in FCEUX.", BadROMImages[x].name);
			return;
		}
		x++;
	}

}


struct CHINF {
	uint32 crc32;
	int32 mapper;
	int32 mirror;
	const char* params;
};

void MapperInit()
{
	if(NewiNES_Init(MapperNo))
	{

	}
	else
	{
		iNESCart.Power=iNESPower;
		if(head.ROM_type&2)
		{
			iNESCart.SaveGame[0]=WRAM;
			iNESCart.SaveGameLen[0]=8192;
		}
	}
}

static const TMasterRomInfo sMasterRomInfo[] = {
	{ 0x62b51b108a01d2beLL, "bonus=0" }, //4-in-1 (FK23C8021)[p1][!].nes
	{ 0x8bb48490d8d22711LL, "bonus=0" }, //4-in-1 (FK23C8033)[p1][!].nes
	{ 0xc75888d7b48cd378LL, "bonus=0" }, //4-in-1 (FK23C8043)[p1][!].nes
	{ 0xf81a376fa54fdd69LL, "bonus=0" }, //4-in-1 (FK23Cxxxx, S-0210A PCB)[p1][!].nes
	{ 0xa37eb9163e001a46LL, "bonus=0" }, //4-in-1 (FK23C8026) [p1][!].nes
	{ 0xde5ce25860233f7eLL, "bonus=0" }, //4-in-1 (FK23C8045) [p1][!].nes
	{ 0x5b3aa4cdc484a088LL, "bonus=0" }, //4-in-1 (FK23C8056) [p1][!].nes
	{ 0x9342bf9bae1c798aLL, "bonus=0" }, //4-in-1 (FK23C8079) [p1][!].nes
	{ 0x164eea6097a1e313LL, "busc=1" }, //Cybernoid - The Fighting Machine (U)[!].nes -- needs bus conflict emulation
};
const TMasterRomInfo* MasterRomInfo;
TMasterRomInfoParams MasterRomInfoParams;

static void CheckHInfo(void)
{
	/* ROM images that have the battery-backed bit set in the header that really
	don't have battery-backed RAM is not that big of a problem, so I'll
	treat this differently by only listing games that should have battery-backed RAM.

	Lower 64 bits of the MD5 hash.
	*/

	static uint64 savie[]=
	{
		0x498c10dc463cfe95LL,  /* Battle Fleet */
		0x6917ffcaca2d8466LL,  /* Famista '90 */

		0xd63dcc68c2b20adcLL,    /* Final Fantasy J */
		0x012df596e2b31174LL,    /* Final Fantasy 1+2 */
		0xf6b359a720549ecdLL,    /* Final Fantasy 2 */
		0x5a30da1d9b4af35dLL,    /* Final Fantasy 3 */

		0x2ee3417ba8b69706LL,  /* Hydlide 3*/

		0xebbce5a54cf3ecc0LL,  /* Justbreed */

		0x6a858da551ba239eLL,  /* Kaijuu Monogatari */
		0xa40666740b7d22feLL,  /* Mindseeker */

		0x77b811b2760104b9LL,    /* Mouryou Senki Madara */

		0x11b69122efe86e8cLL,  /* RPG Jinsei Game */

		0xa70b495314f4d075LL,  /* Ys 3 */


		0xc04361e499748382LL,  /* AD&D Heroes of the Lance */
		0xb72ee2337ced5792LL,  /* AD&D Hillsfar */
		0x2b7103b7a27bd72fLL,  /* AD&D Pool of Radiance */

		0x854d7947a3177f57LL,    /* Crystalis */

		0xb0bcc02c843c1b79LL,  /* DW */
		0x4a1f5336b86851b6LL,  /* DW */

		0x2dcf3a98c7937c22LL,  /* DW 2 */
		0x733026b6b72f2470LL,  /* Dw 3 */
		0x98e55e09dfcc7533LL,  /* DW 4*/
		0x8da46db592a1fcf4LL,  /* Faria */
		0x91a6846d3202e3d6LL,  /* Final Fantasy */
		0xedba17a2c4608d20LL,  /* ""    */

		0x94b9484862a26cbaLL,    /* Legend of Zelda */
		0x04a31647de80fdabLL,    /*      ""      */

		0x9aa1dc16c05e7de5LL,    /* Startropics */
		0x1b084107d0878bd0LL,    /* Startropics 2*/

		0x836c0ff4f3e06e45LL,    /* Zelda 2 */

		0x82000965f04a71bbLL,    /* Mirai Shinwa Jarvas */

		0      /* Abandon all hope if the game has 0 in the lower 64-bits of its MD5 hash */
	};

	static struct CHINF moo[]=
	{
#include "ines-correct.h"
	};
	int tofix=0;
	int x;
	uint64 partialmd5=0;

	for(x=0;x<8;x++)
	{
		partialmd5 |= (uint64)iNESCart.MD5[15-x] << (x*8);
		//printf("%16llx\n",partialmd5);
	}
	CheckBad(partialmd5);

	MasterRomInfo = NULL;
	for(int i=0;i<ARRAY_SIZE(sMasterRomInfo);i++)
	{
		const TMasterRomInfo& info = sMasterRomInfo[i];
		if(info.md5lower != partialmd5)
			continue;

		MasterRomInfo = &info;
		if(!info.params) break;

		std::vector<std::string> toks = tokenize_str(info.params,",");
		for(int j=0;j<(int)toks.size();j++)
		{
			std::vector<std::string> parts = tokenize_str(toks[j],"=");
			MasterRomInfoParams[parts[0]] = parts[1];
		}
		break;
	}

	x=0;

	do
	{
		if(moo[x].crc32==iNESGameCRC32)
		{
			if(moo[x].mapper>=0)
			{
				if(moo[x].mapper&0x800 && VROM_size)
				{
					VROM_size=0;
					free(VROM);
					VROM = NULL;
					tofix|=8;
				}
				if(MapperNo!=(moo[x].mapper&0xFF))
				{
					tofix|=1;
					MapperNo=moo[x].mapper&0xFF;
				}
			}
			if(moo[x].mirror>=0)
			{
				if(moo[x].mirror==8)
				{
					if(Mirroring==2)  /* Anything but hard-wired(four screen). */
					{
						tofix|=2;
						Mirroring=0;
					}
				}
				else if(Mirroring!=moo[x].mirror)
				{
					if(Mirroring!=(moo[x].mirror&~4))
						if((moo[x].mirror&~4)<=2)  /* Don't complain if one-screen mirroring
												   needs to be set(the iNES header can't
												   hold this information).
												   */
												   tofix|=2;
					Mirroring=moo[x].mirror;
				}
			}
			break;
		}
		x++;
	} while(moo[x].mirror>=0 || moo[x].mapper>=0);

	x=0;
	while(savie[x] != 0)
	{
		if(savie[x] == partialmd5)
		{
			if(!(head.ROM_type&2))
			{
				tofix|=4;
				head.ROM_type|=2;
			}
		}
		x++;
	}

	/* Games that use these iNES mappers tend to have the four-screen bit set
	when it should not be.
	*/
	if((MapperNo==118 || MapperNo==24 || MapperNo==26) && (Mirroring==2))
	{
		Mirroring=0;
		tofix|=2;
	}

	/* Four-screen mirroring implicitly set. */
	if(MapperNo==99)
		Mirroring=2;

	if(tofix)
	{
		char gigastr[768];
		strcpy(gigastr,"The iNES header contains incorrect information.  For now, the information will be corrected in RAM.  ");
		if(tofix&1)
			sprintf(gigastr+strlen(gigastr),"The mapper number should be set to %d.  ",MapperNo);
		if(tofix&2)
		{
			char *mstr[3]={"Horizontal","Vertical","Four-screen"};
			sprintf(gigastr+strlen(gigastr),"Mirroring should be set to \"%s\".  ",mstr[Mirroring&3]);
		}
		if(tofix&4)
			strcat(gigastr,"The battery-backed bit should be set.  ");
		if(tofix&8)
			strcat(gigastr,"This game should not have any CHR ROM.  ");
		strcat(gigastr,"\n");
		FCEU_printf("%s",gigastr);
	}
}

typedef struct {
	int mapper;
	void (*init)(CartInfo *);
} NewMI;

//this is for games that is not the a power of 2
//mapper based for now...
//not really accurate but this works since games
//that are not in the power of 2 tends to come
//in obscure mappers themselves which supports such
//size
static int not_power2[] =
{
    228
};
typedef struct {
	char* name;
	int number;
	void (*init)(CartInfo *);
} BMAPPINGLocal;

static BMAPPINGLocal bmap[] = {
	{"NROM",				  0, NROM_Init},
	{"MMC1",				  1, Mapper1_Init},
	{"UNROM",				  2, UNROM_Init},
	{"CNROM",				  3, CNROM_Init},
	{"MMC3",				  4, Mapper4_Init},
	{"MMC5",				  5, Mapper5_Init},
//	{"",					  6, Mapper6_Init},
	{"ANROM",				  7, ANROM_Init},
	{"",					  8, Mapper8_Init},		// Nogaems, it's worthless
//	{"",					  9, Mapper9_Init},
//	{"",					 10, Mapper10_Init},
	{"Color Dreams",		 11, Mapper11_Init},
	{"",					 12, Mapper12_Init},
	{"CPROM",				 13, CPROM_Init},
//	{"",					 14, Mapper14_Init},
	{"100-in-1",			 15, Mapper15_Init},
	{"Bandai",				 16, Mapper16_Init},
	{"",					 17, Mapper17_Init},
	{"",					 18, Mapper18_Init},
	{"Namcot 106",			 19, Mapper19_Init},
//	{"",					 20, Mapper20_Init},
	{"Konami VRC2/VRC4",	 21, Mapper21_Init},
	{"Konami VRC2/VRC4",	 22, Mapper22_Init},
	{"Konami VRC2/VRC4",	 23, Mapper23_Init},
//	{"",					 24, Mapper24_Init},
	{"Konami VRC2/VRC4",	 25, Mapper25_Init},
//	{"",					 26, Mapper26_Init},
//	{"",					 27, Mapper27_Init},	// Deprecated, dupe for VRC2/VRC4 mapper
	{"INL-ROM",				 28, Mapper28_Init},
//	{"",					 29, Mapper29_Init},
//	{"",					 30, Mapper30_Init},
//	{"",					 31, Mapper31_Init},
	{"IREM G-101",			 32, Mapper32_Init},
	{"TC0190FMC/TC0350FMR",	 33, Mapper33_Init},
	{"",					 34, Mapper34_Init},
	{"Wario Land 2",		 35, UNLSC127_Init},
	{"TXC Policeman",		 36, Mapper36_Init},
	{"",					 37, Mapper37_Init},
	{"Bit Corp.",			 38, Mapper38_Init},	// Crime Busters
//	{"",					 39, Mapper39_Init},
//	{"",					 40, Mapper40_Init},
//	{"",					 41, Mapper41_Init},
//	{"",					 42, Mapper42_Init},
	{"",					 43, Mapper43_Init},
	{"",					 44, Mapper44_Init},
	{"",					 45, Mapper45_Init},
//	{"",					 46, Mapper46_Init},
	{"",					 47, Mapper47_Init},
	{"TAITO TCxxx",			 48, Mapper48_Init},
	{"",					 49, Mapper49_Init},
//	{"",					 50, Mapper50_Init},
//	{"",					 51, Mapper51_Init},
	{"",					 52, Mapper52_Init},
//	{"",					 53, Mapper53_Init},	// iNES version of complex UNIF board, can't emulate properly as iNES
//	{"",					 54, Mapper54_Init},
//	{"",					 55, Mapper55_Init},
//	{"",					 56, Mapper56_Init},
	{"",					 57, Mapper57_Init},
	{"",					 58, BMCGK192_Init},
	{"",					 59, Mapper59_Init},	// Check this out
	{"",					 60, BMCD1038_Init},
//	{"",					 61, Mapper61_Init},
//	{"",					 62, Mapper62_Init},
//	{"",					 63, Mapper63_Init},
//	{"",					 64, Mapper64_Init},
//	{"",					 65, Mapper65_Init},
	{"MHOM",				 66, MHROM_Init},
//	{"",					 67, Mapper67_Init},
	{"Sunsoft Mapper #4",	 68, Mapper68_Init},
//	{"",					 69, Mapper69_Init},
	{"",					 70, Mapper70_Init},
//	{"",					 71, Mapper71_Init},
//	{"",					 72, Mapper72_Init},
//	{"",					 73, Mapper73_Init},
	{"",					 74, Mapper74_Init},
//	{"",					 75, Mapper75_Init},
//	{"",					 76, Mapper76_Init},
//	{"",					 77, Mapper77_Init},
	{"Irem 74HC161/32",		 78, Mapper78_Init},
//	{"",					 79, Mapper79_Init},
//	{"",					 80, Mapper80_Init},
//	{"",					 81, Mapper81_Init},
	{"",					 82, Mapper82_Init},
	{"",					 83, Mapper83_Init},
//	{"",					 84, Mapper84_Init},
//	{"",					 85, Mapper85_Init},
	{"",					 86, Mapper86_Init},
	{"",					 87, Mapper87_Init},
	{"",					 88, Mapper88_Init},
	{"",					 89, Mapper89_Init},
	{"",					 90, Mapper90_Init},
	{"",					 91, Mapper91_Init},
	{"",					 92, Mapper92_Init},
	{"Sunsoft UNROM",		 93, SUNSOFT_UNROM_Init},
	{"",					 94, Mapper94_Init},
	{"",					 95, Mapper95_Init},
	{"",					 96, Mapper96_Init},
	{"",					 97, Mapper97_Init},
//	{"",					 98, Mapper98_Init},
	{"",					 99, Mapper99_Init},
//	{"",					100, Mapper100_Init},
	{"",					101, Mapper101_Init},
//	{"",					102, Mapper102_Init},
	{"",					103, Mapper103_Init},
//	{"",					104, Mapper104_Init},
	{"",					105, Mapper105_Init},
	{"",					106, Mapper106_Init},
	{"",					107, Mapper107_Init},
	{"",					108, Mapper108_Init},
//	{"",					109, Mapper109_Init},
//	{"",					110, Mapper110_Init},
//	{"",					111, Mapper111_Init},
	{"",					112, Mapper112_Init},
	{"",					113, Mapper113_Init},
	{"",					114, Mapper114_Init},
	{"",					115, Mapper115_Init},
	{"",					116, UNLSL12_Init},
	{"",					117, Mapper117_Init},
	{"TSKROM",				118, TKSROM_Init},
	{"",					119, Mapper119_Init},
	{"",					120, Mapper120_Init},
	{"",					121, Mapper121_Init},
//	{"",					122, Mapper122_Init},
	{"UNLH2288",			123, UNLH2288_Init},
//	{"",					124, Mapper124_Init},
	{"",					125, LH32_Init},
//	{"",					126, Mapper126_Init},
//	{"",					127, Mapper127_Init},
//	{"",					128, Mapper128_Init},
//	{"",					129, Mapper129_Init},
//	{"",					130, Mapper130_Init},
//	{"",					131, Mapper131_Init},
	{"UNL22211",			132, UNL22211_Init},
	{"SA72008",				133, SA72008_Init},
	{"",					134, Mapper134_Init},
//	{"",					135, Mapper135_Init},
	{"TCU02",				136, TCU02_Init},
	{"S8259D",				137, S8259D_Init},
	{"S8259B",				138, S8259B_Init},
	{"S8259C",				139, S8259C_Init},
	{"",					140, Mapper140_Init},
	{"S8259A",				141, S8259A_Init},
	{"UNLKS7032",			142, UNLKS7032_Init},
	{"TCA01",				143, TCA01_Init},
	{"",					144, Mapper144_Init},
	{"SA72007",				145, SA72007_Init},
	{"SA0161M",				146, SA0161M_Init},
	{"TCU01",				147, TCU01_Init},
	{"SA0037",				148, SA0037_Init},
	{"SA0036",				149, SA0036_Init},
	{"S74LS374N",			150, S74LS374N_Init},
	{"",					151, Mapper151_Init},
	{"",					152, Mapper152_Init},
	{"",					153, Mapper153_Init},
	{"",					154, Mapper154_Init},
	{"",					155, Mapper155_Init},
	{"",					156, Mapper156_Init},
	{"",					157, Mapper157_Init},
//	{"",					158, Mapper158_Init},
//	{"",					159, Mapper159_Init},
	{"SA009",				160, SA009_Init},
//	{"",					161, Mapper161_Init},
	{"",					162, UNLFS304_Init},
	{"",					163, Mapper163_Init},
	{"",					164, Mapper164_Init},
	{"",					165, Mapper165_Init},
//	{"",					166, Mapper166_Init},
//	{"",					167, Mapper167_Init},
	{"",					168, Mapper168_Init},
//	{"",					169, Mapper169_Init},
	{"",					170, Mapper170_Init},
	{"",					171, Mapper171_Init},
	{"",					172, Mapper172_Init},
	{"",					173, Mapper173_Init},
//	{"",					174, Mapper174_Init},
	{"",					175, Mapper175_Init},
	{"BMCFK23C",			176, BMCFK23C_Init},	//zero 26-may-2012 - well, i have some WXN junk games that use 176 for instance ????. i dont know what game uses this BMCFK23C as mapper 176. we'll have to make a note when we find it.
	{"",					177, Mapper177_Init},
	{"",					178, Mapper178_Init},
//	{"",					179, Mapper179_Init},
	{"",					180, Mapper180_Init},
	{"",					181, Mapper181_Init},
//	{"",					182, Mapper182_Init},	// Deprecated, dupe
	{"",					183, Mapper183_Init},
	{"",					184, Mapper184_Init},
	{"",					185, Mapper185_Init},
	{"",					186, Mapper186_Init},
	{"",					187, Mapper187_Init},
	{"",					188, Mapper188_Init},
	{"",					189, Mapper189_Init},
//	{"",					190, Mapper190_Init},
	{"",					191, Mapper191_Init},
	{"",					192, Mapper192_Init},
	{"",					193, Mapper193_Init},
	{"",					194, Mapper194_Init},
	{"",					195, Mapper195_Init},
	{"",					196, Mapper196_Init},
	{"",					197, Mapper197_Init},
	{"",					198, Mapper198_Init},
	{"",					199, Mapper199_Init},
	{"",					200, Mapper200_Init},
	{"",					201, Mapper201_Init},
	{"",					202, Mapper202_Init},
	{"",					203, Mapper203_Init},
	{"",					204, Mapper204_Init},
	{"",					205, Mapper205_Init},
	{"DEIROM",				206, DEIROM_Init},
//	{"",					207, Mapper207_Init},
	{"",					208, Mapper208_Init},
	{"",					209, Mapper209_Init},
	{"",					210, Mapper210_Init},
	{"",					211, Mapper211_Init},
	{"",					212, Mapper212_Init},
	{"",					213, Mapper213_Init},
	{"",					214, Mapper214_Init},
	{"",					215, UNL8237_Init},
	{"",					216, Mapper216_Init},
	{"",					217, Mapper217_Init},	// Redefined to a new Discrete BMC mapper
//	{"",					218, Mapper218_Init},
	{"UNLA9746",			219, UNLA9746_Init},
	{"Debug Mapper",		220, UNLKS7057_Init},
	{"UNLN625092",			221, UNLN625092_Init},
	{"",					222, Mapper222_Init},
//	{"",					223, Mapper223_Init},
//	{"",					224, Mapper224_Init},
	{"",					225, Mapper225_Init},
	{"BMC 22+20-in-1",		226, Mapper226_Init},
	{"",					227, Mapper227_Init},
	{"",					228, Mapper228_Init},
	{"",					229, Mapper229_Init},
	{"BMC 22-in-1+Contra",	230, Mapper230_Init},
	{"",					231, Mapper231_Init},
	{"BMC QUATTRO",			232, Mapper232_Init},
	{"BMC 22+20-in-1 RST",	233, Mapper233_Init},
	{"BMC MAXI",			234, Mapper234_Init},
	{"",					235, Mapper235_Init},
//	{"",					236, Mapper236_Init},
//	{"",					237, Mapper237_Init},
	{"UNL6035052",			238, UNL6035052_Init},
//	{"",					239, Mapper239_Init},
	{"",					240, Mapper240_Init},
	{"",					241, Mapper241_Init},
	{"",					242, Mapper242_Init},
	{"S74LS374NA",			243, S74LS374NA_Init},
	{"DECATHLON",			244, Mapper244_Init},
	{"",					245, Mapper245_Init},
	{"FONG SHEN BANG",		246, Mapper246_Init},
//	{"",					247, Mapper247_Init},
//	{"",					248, Mapper248_Init},
	{"",					249, Mapper249_Init},
	{"",					250, Mapper250_Init},
//	{"",					251, Mapper251_Init},	// No good dumps for this mapper, use UNIF version
	{"SAN GUO ZHI PIRATE",	252, Mapper252_Init},
	{"DRAGON BALL PIRATE",	253, Mapper253_Init},
	{"",					254, Mapper254_Init},
//	{"",					255, Mapper255_Init},	// No good dumps for this mapper
	{"",					0, NULL}
};


int iNESLoad(const char *name, FCEUFILE *fp, int OverwriteVidMode)
{
	struct md5_context md5;

	if(FCEU_fread(&head,1,16,fp)!=16)
		return 0;

	if(memcmp(&head,"NES\x1a",4))
		return 0;

	head.cleanup();

	memset(&iNESCart,0,sizeof(iNESCart));

	MapperNo = (head.ROM_type>>4);
	MapperNo|=(head.ROM_type2&0xF0);
	Mirroring = (head.ROM_type&1);


	//  int ROM_size=0;
	if(!head.ROM_size)
	{
		//   FCEU_PrintError("No PRG ROM!");
		//   return(0);
		ROM_size=256;
		//head.ROM_size++;
	}
	else
		ROM_size=uppow2(head.ROM_size);

	//    ROM_size = head.ROM_size;
	VROM_size = head.VROM_size;

    int round = true;
    for (int i = 0; i != sizeof(not_power2)/sizeof(not_power2[0]); ++i)
    {
        //for games not to the power of 2, so we just read enough
        //prg rom from it, but we have to keep ROM_size to the power of 2
        //since PRGCartMapping wants ROM_size to be to the power of 2
        //so instead if not to power of 2, we just use head.ROM_size when
        //we use FCEU_read
        if (not_power2[i] == MapperNo)
        {
            round = false;
            break;
        }
    }

    if(VROM_size)
		VROM_size=uppow2(VROM_size);


	if(head.ROM_type&8) Mirroring=2;

	if((ROM = (uint8 *)FCEU_malloc(ROM_size<<14)) == NULL) return 0;

	if(VROM_size)
	{
		if((VROM = (uint8 *)FCEU_malloc(VROM_size<<13)) == NULL)
		{
			free(ROM);
			ROM = NULL;
			return 0;
		}
	}
	memset(ROM,0xFF,ROM_size<<14);
	if(VROM_size) memset(VROM,0xFF,VROM_size<<13);
	if(head.ROM_type&4)   /* Trainer */
	{
		trainerpoo=(uint8 *)FCEU_gmalloc(512);
		FCEU_fread(trainerpoo,512,1,fp);
	}

	ResetCartMapping();
	ResetExState(0,0);

	SetupCartPRGMapping(0,ROM,ROM_size*0x4000,0);
   // SetupCartPRGMapping(1,WRAM,8192,1);

    FCEU_fread(ROM,0x4000,(round) ? ROM_size : head.ROM_size,fp);

	if(VROM_size)
		FCEU_fread(VROM,0x2000,head.VROM_size,fp);

	md5_starts(&md5);
	md5_update(&md5,ROM,ROM_size<<14);

	iNESGameCRC32=CalcCRC32(0,ROM,ROM_size<<14);

	if(VROM_size)
	{
		iNESGameCRC32=CalcCRC32(iNESGameCRC32,VROM,VROM_size<<13);
		md5_update(&md5,VROM,VROM_size<<13);
	}
	md5_finish(&md5,iNESCart.MD5);
	memcpy(&GameInfo->MD5,&iNESCart.MD5,sizeof(iNESCart.MD5));

	iNESCart.CRC32=iNESGameCRC32;

	FCEU_printf(" PRG ROM:  %3d x 16KiB\n CHR ROM:  %3d x  8KiB\n ROM CRC32:  0x%08lx\n",
                (round) ? ROM_size : head.ROM_size, head.VROM_size,iNESGameCRC32);

	{
		int x;
		FCEU_printf(" ROM MD5:  0x");
		for(x=0;x<16;x++)
			FCEU_printf("%02x",iNESCart.MD5[x]);
		FCEU_printf("\n");
	}

	char* mappername = "Not Listed";

	for(int mappertest=0;mappertest< (sizeof bmap / sizeof bmap[0]) - 1;mappertest++)
	{
		if (bmap[mappertest].number == MapperNo) {
			mappername = bmap[mappertest].name;
			break;
		}
	}

	FCEU_printf(" Mapper #:  %d\n Mapper name: %s\n Mirroring: %s\n",
		MapperNo, mappername, Mirroring==2?"None (Four-screen)":Mirroring?"Vertical":"Horizontal");

	FCEU_printf(" Battery-backed: %s\n", (head.ROM_type&2)?"Yes":"No");
	FCEU_printf(" Trained: %s\n", (head.ROM_type&4)?"Yes":"No");
	// (head.ROM_type&8) = Mirroring: None(Four-screen)

	SetInput();
	CheckHInfo();
	{
		int x;
		uint64 partialmd5=0;

		for(x=0;x<8;x++)
		{
			partialmd5 |= (uint64)iNESCart.MD5[7-x] << (x*8);
		}

		FCEU_VSUniCheck(partialmd5, &MapperNo, &Mirroring);
	}
	/* Must remain here because above functions might change value of
	VROM_size and free(VROM).
	*/
	if(VROM_size)
		SetupCartCHRMapping(0,VROM,VROM_size*0x2000,0);

	if(Mirroring==2)
		SetupCartMirroring(4,1,ExtraNTARAM);
	else if(Mirroring>=0x10)
		SetupCartMirroring(2+(Mirroring&1),1,0);
	else
		SetupCartMirroring(Mirroring&1,(Mirroring&4)>>2,0);

	iNESCart.battery=(head.ROM_type&2)?1:0;
	iNESCart.mirror=Mirroring;

	//if(MapperNo != 18) {
	//  if(ROM) free(ROM);
	//  if(VROM) free(VROM);
	//  ROM=VROM=0;
	//  return(0);
	// }


	GameInfo->mappernum = MapperNo;
	MapperInit();
	FCEU_LoadGameSave(&iNESCart);

	strcpy(LoadedRomFName,name); //bbit edited: line added

	// Extract Filename only. Should account for Windows/Unix this way.
	if (strrchr(name, '/')) {
	name = strrchr(name, '/') + 1;
	} else if(strrchr(name, '\\')) {
	name = strrchr(name, '\\') + 1;
	}

	GameInterface=iNESGI;
	FCEU_printf("\n");

	// since apparently the iNES format doesn't store this information,
	// guess if the settings should be PAL or NTSC from the ROM name
	// TODO: MD5 check against a list of all known PAL games instead?
	if(OverwriteVidMode)
	{
		if(strstr(name,"(E)") || strstr(name,"(e)")
			|| strstr(name,"(Europe)") || strstr(name,"(PAL)")
			|| strstr(name,"(F)") || strstr(name,"(f)")
			|| strstr(name,"(G)") || strstr(name,"(g)")
			|| strstr(name,"(I)") || strstr(name,"(i)"))
			FCEUI_SetVidSystem(1);
		else
			FCEUI_SetVidSystem(0);
	}
	return 1;
}


//bbit edited: the whole function below was added
int iNesSave(){
	FILE *fp;
	char name[2048];

	if(GameInfo->type != GIT_CART)return 0;
	if(GameInterface!=iNESGI)return 0;

	strcpy(name,LoadedRomFName);
	if (strcmp(name+strlen(name)-4,".nes") != 0) { //para edit
		strcat(name,".nes");
	}

	fp = fopen(name,"wb");

	if(fwrite(&head,1,16,fp)!=16)
	{
		fclose(fp);
		return 0;
	}

	if(head.ROM_type&4) 	/* Trainer */
	{
		fwrite(trainerpoo,512,1,fp);
	}

	fwrite(ROM,0x4000,ROM_size,fp);

	if(head.VROM_size)fwrite(VROM,0x2000,head.VROM_size,fp);
	fclose(fp);

	return 1;
}

int iNesSaveAs(char* name)
{
	//adelikat: TODO: iNesSave() and this have pretty much the same code, outsource the common code to a single function
	FILE *fp;

	if(GameInfo->type != GIT_CART)return 0;
	if(GameInterface != iNESGI)return 0;

	fp = fopen(name,"wb");

	if(fwrite(&head,1,16,fp)!=16)
	{
		fclose(fp);
		return 0;
	}

	if(head.ROM_type&4) 	/* Trainer */
	{
		fwrite(trainerpoo,512,1,fp);
	}

	fwrite(ROM,0x4000,ROM_size,fp);

	if(head.VROM_size)fwrite(VROM,0x2000,head.VROM_size,fp);
	fclose(fp);

	return 1;
}

//para edit: added function below
char *iNesShortFName() {
	char *ret;

	if (!(ret = strrchr(LoadedRomFName,'\\'))) {
		if (!(ret = strrchr(LoadedRomFName,'/'))) return 0;
	}
	return ret+1;
}

void VRAM_BANK1(uint32 A, uint8 V)
{
	V&=7;
	PPUCHRRAM|=(1<<(A>>10));
	CHRBankList[(A)>>10]=V;
	VPage[(A)>>10]=&CHRRAM[V<<10]-(A);
}

void VRAM_BANK4(uint32 A, uint32 V)
{
	V&=1;
	PPUCHRRAM|=(0xF<<(A>>10));
	CHRBankList[(A)>>10]=(V<<2);
	CHRBankList[((A)>>10)+1]=(V<<2)+1;
	CHRBankList[((A)>>10)+2]=(V<<2)+2;
	CHRBankList[((A)>>10)+3]=(V<<2)+3;
	VPage[(A)>>10]=&CHRRAM[V<<10]-(A);
}
void VROM_BANK1(uint32 A,uint32 V)
{
	setchr1(A,V);
	CHRBankList[(A)>>10]=V;
}

void VROM_BANK2(uint32 A,uint32 V)
{
	setchr2(A,V);
	CHRBankList[(A)>>10]=(V<<1);
	CHRBankList[((A)>>10)+1]=(V<<1)+1;
}

void VROM_BANK4(uint32 A, uint32 V)
{
	setchr4(A,V);
	CHRBankList[(A)>>10]=(V<<2);
	CHRBankList[((A)>>10)+1]=(V<<2)+1;
	CHRBankList[((A)>>10)+2]=(V<<2)+2;
	CHRBankList[((A)>>10)+3]=(V<<2)+3;
}

void VROM_BANK8(uint32 V)
{
	setchr8(V);
	CHRBankList[0]=(V<<3);
	CHRBankList[1]=(V<<3)+1;
	CHRBankList[2]=(V<<3)+2;
	CHRBankList[3]=(V<<3)+3;
	CHRBankList[4]=(V<<3)+4;
	CHRBankList[5]=(V<<3)+5;
	CHRBankList[6]=(V<<3)+6;
	CHRBankList[7]=(V<<3)+7;
}

void ROM_BANK8(uint32 A, uint32 V)
{
	setprg8(A,V);
	if(A>=0x8000)
		PRGBankList[((A-0x8000)>>13)]=V;
}

void ROM_BANK16(uint32 A, uint32 V)
{
	setprg16(A,V);
	if(A>=0x8000)
	{
		PRGBankList[((A-0x8000)>>13)]=V<<1;
		PRGBankList[((A-0x8000)>>13)+1]=(V<<1)+1;
	}
}

void ROM_BANK32(uint32 V)
{
	setprg32(0x8000,V);
	PRGBankList[0]=V<<2;
	PRGBankList[1]=(V<<2)+1;
	PRGBankList[2]=(V<<2)+2;
	PRGBankList[3]=(V<<2)+3;
}

void onemir(uint8 V)
{
	if(Mirroring==2) return;
	if(V>1)
		V=1;
	Mirroring=0x10|V;
	setmirror(MI_0+V);
}

void MIRROR_SET2(uint8 V)
{
	if(Mirroring==2) return;
	Mirroring=V;
	setmirror(V);
}

void MIRROR_SET(uint8 V)
{
	if(Mirroring==2) return;
	V^=1;
	Mirroring=V;
	setmirror(V);
}

static void NONE_init(void)
{
	ROM_BANK16(0x8000,0);
	ROM_BANK16(0xC000,~0);

	if(VROM_size)
		VROM_BANK8(0);
	else
		setvram8(CHRRAM);
}

void (*MapInitTab[256])(void)=
{
	0,
	0,
	0, //Mapper2_init,
	0, //Mapper3_init,
	0,
	0,
	Mapper6_init,
	0, //Mapper7_init,
	0, //Mapper8_init,
	Mapper9_init,
	Mapper10_init,
	0, //Mapper11_init,
	0,
	0, //Mapper13_init,
	0,
	0, //Mapper15_init,
	0, //Mapper16_init,
	0, //Mapper17_init,
	0, //Mapper18_init,
	0,
	0,
	0, //Mapper21_init,
	0, //Mapper22_init,
	0, //Mapper23_init,
	Mapper24_init,
	0, //Mapper25_init,
	Mapper26_init,
	0, //Mapper27_init,
	0,
	0,
	0,
	0,
	0, //Mapper32_init,
	0, //Mapper33_init,
	0, //Mapper34_init,
	0,
	0,
	0,
	0,
	0,
	Mapper40_init,
	Mapper41_init,
	Mapper42_init,
	0, //Mapper43_init,
	0,
	0,
	Mapper46_init,
	0,
	0, //Mapper48_init,
	0,
	Mapper50_init,
	Mapper51_init,
	0,
	0,
	0,
	0,
	0,
	0, //Mapper57_init,
	0, //Mapper58_init,
	0, //Mapper59_init,
	0, //Mapper60_init,
	Mapper61_init,
	Mapper62_init,
	0,
	Mapper64_init,
	Mapper65_init,
	0, //Mapper66_init,
	Mapper67_init,
	0, //Mapper68_init,
	Mapper69_init,
	0, //Mapper70_init,
	Mapper71_init,
	Mapper72_init,
	Mapper73_init,
	0,
	Mapper75_init,
	Mapper76_init,
	Mapper77_init,
	0, //Mapper78_init,
	Mapper79_init,
	Mapper80_init,
	0,
	0, //Mapper82_init,
	0, //Mapper83_init,
	0,
	Mapper85_init,
	0, //Mapper86_init,
	0, //Mapper87_init,
	0, //Mapper88_init,
	0, //Mapper89_init,
	0,
	0, //Mapper91_init,
	0, //Mapper92_init,
	0, //Mapper93_init,
	0, //Mapper94_init,
	0,
	0, //Mapper96_init,
	0, //Mapper97_init,
	0,
	0, //Mapper99_init,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, //Mapper107_init,
	0,
	0,
	0,
	0,
	0,
	0, //Mapper113_init,
	0,
	0,
	0, //Mapper116_init,
	0, //Mapper117_init,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, //Mapper140_init,
	0,
	0,
	0,
	0, //Mapper144_init,
	0,
	0,
	0,
	0,
	0,
	0,
	0, //Mapper151_init,
	0, //Mapper152_init,
	0, //Mapper153_init,
	0, //Mapper154_init,
	0,
	0, //Mapper156_init,
	0, //Mapper157_init,
	0, //Mapper158_init, removed
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	Mapper166_init,
	Mapper167_init,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, //Mapper180_init,
	0,
	0,
	0,
	0, //Mapper184_init,
	0, //Mapper185_init,
	0,
	0,
	0,
	0, //Mapper189_init,
	0,
	0, //Mapper191_init,
	0,
	0, //Mapper193_init,
	0,
	0,
	0,
	0,
	0,
	0,
	0, //Mapper200_init,
	0, //Mapper201_init,
	0, //Mapper202_init,
	0, //Mapper203_init,
	0, //Mapper204_init,
	0,
	0,
	Mapper207_init,
	0,
	0,
	0,
	0, //Mapper211_init,
	0, //Mapper212_init,
	0, //Mapper213_init,
	0, //Mapper214_init,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, //Mapper225_init,
	0, //Mapper226_init,
	0, //Mapper227_init,
	0, //Mapper228_init,
	0, //Mapper229_init,
	0, //Mapper230_init,
	0, //Mapper231_init,
	0, //Mapper232_init,
	0,
	0, //Mapper234_init,
	0, //Mapper235_init,
	0,
	0,
	0,
	0,
	0, //Mapper240_init,
	0, //Mapper241_init,
	0, //Mapper242_init,
	0,
	0, //Mapper244_init,
	0,
	0, //Mapper246_init,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0,
	0, //Mapper255_init
};

static DECLFW(BWRAM)
{
	WRAM[A-0x6000]=V;
}

static DECLFR(AWRAM)
{
	return WRAM[A-0x6000];
}


void (*MapStateRestore)(int version);
void iNESStateRestore(int version)
{
	int x;

	if(!MapperNo) return;

	for(x=0;x<4;x++)
		setprg8(0x8000+x*8192,PRGBankList[x]);

	if(VROM_size)
		for(x=0;x<8;x++)
			setchr1(0x400*x,CHRBankList[x]);

	if(0) switch(Mirroring)
	{
		case 0:setmirror(MI_H);break;
		case 1:setmirror(MI_V);break;
		case 0x12:
		case 0x10:setmirror(MI_0);break;
		case 0x13:
		case 0x11:setmirror(MI_1);break;
	}
	if(MapStateRestore) MapStateRestore(version);
}

static void iNESPower(void)
{
	int x;
	int type=MapperNo;

	SetReadHandler(0x8000,0xFFFF,CartBR);
	GameStateRestore=iNESStateRestore;
	MapClose=0;
	MapperReset=0;
	MapStateRestore=0;

	setprg8r(1,0x6000,0);

	SetReadHandler(0x6000,0x7FFF,AWRAM);
	SetWriteHandler(0x6000,0x7FFF,BWRAM);
	FCEU_CheatAddRAM(8,0x6000,WRAM);

	/* This statement represents atrocious code.  I need to rewrite
	all of the iNES mapper code... */
	IRQCount=IRQLatch=IRQa=0;
	if(head.ROM_type&2)
		memset(GameMemBlock+8192,0,GAME_MEM_BLOCK_SIZE-8192);
	else
		memset(GameMemBlock,0,GAME_MEM_BLOCK_SIZE);

	NONE_init();
	ResetExState(0,0);

	if(GameInfo->type == GIT_VSUNI)
		AddExState(FCEUVSUNI_STATEINFO, ~0, 0, 0);

	AddExState(WRAM, 8192, 0, "WRAM");
	if(type==19 || type==6 || type==69 || type==85 || type==96)
		AddExState(MapperExRAM, 32768, 0, "MEXR");
	if((!VROM_size || type==6 || type==19) && (type!=13 && type!=96))
		AddExState(CHRRAM, 8192, 0, "CHRR");
	if(head.ROM_type&8)
		AddExState(ExtraNTARAM, 2048, 0, "EXNR");

	/* Exclude some mappers whose emulation code handle save state stuff
	themselves. */
	if(type && type!=13 && type!=96)
	{
		AddExState(mapbyte1, 32, 0, "MPBY");
		AddExState(&Mirroring, 1, 0, "MIRR");
		AddExState(&IRQCount, 4, 1, "IRQC");
		AddExState(&IRQLatch, 4, 1, "IQL1");
		AddExState(&IRQa, 1, 0, "IRQA");
		AddExState(PRGBankList, 4, 0, "PBL");
		for(x=0;x<8;x++)
		{
			char tak[8];
			sprintf(tak,"CBL%d",x);
			AddExState(&CHRBankList[x], 2, 1,tak);
		}
	}

	if(MapInitTab[type]) MapInitTab[type]();
	else if(type)
	{
		FCEU_PrintError("iNES mapper #%d is not supported at all.",type);
	}
}

static int NewiNES_Init(int num)
{
	BMAPPINGLocal *tmp=bmap;

	CHRRAMSize = -1;

	if(GameInfo->type == GIT_VSUNI)
		AddExState(FCEUVSUNI_STATEINFO, ~0, 0, 0);

	while(tmp->init)
	{
		if(num==tmp->number)
		{
			UNIFchrrama=0; // need here for compatibility with UNIF mapper code
			if(!VROM_size)
			{
				if(num==13)
				{
					CHRRAMSize=16384;
				}
				else
				{
					CHRRAMSize=8192;
				}
				if((VROM = (uint8 *)FCEU_dmalloc(CHRRAMSize)) == NULL) return 0;
				FCEU_MemoryRand(VROM,CHRRAMSize);

				UNIFchrrama=VROM;
				SetupCartCHRMapping(0,VROM,CHRRAMSize,1);
				AddExState(VROM,CHRRAMSize, 0, "CHRR");
			}
			if(head.ROM_type&8)
				AddExState(ExtraNTARAM, 2048, 0, "EXNR");
			tmp->init(&iNESCart);
			return 1;
		}
		tmp++;
	}
	return 0;
}
