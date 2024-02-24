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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "types.h"
#include "x6502.h"
#include "fceu.h"
#include "input.h"
#include "netplay.h"
#include "vsuni.h"
#include "state.h"
#include "driver.h"
#include "cart.h"
#include "ines.h"

#include <cstring>
#include <cstdio>

static int DIPS_howlong = 0;
uint8 vsdip = 0;

void FCEU_VSUniToggleDIP(int w) {
	if (GameInfo->type != GIT_VSUNI) {
		FCEU_DispMessage("Not Vs. System; toggle DIP switch.", 0);
		return;
	}
	vsdip ^= 1 << w;
	DIPS_howlong = 180;
	FCEU_DispMessage("DIP switch %d is %s.", 0, w, vsdip & (1 << w) ? "on" : "off");
}

void FCEUI_VSUniSetDIP(int w, int state) {
	if (((vsdip >> w) & 1) != state)
		FCEUI_VSUniToggleDIP(w);
}

uint8 FCEUI_VSUniGetDIPs(void) {
	return(vsdip);
}

static uint8 secdata_tko[32] =
{
	0xff, 0xbf, 0xb7, 0x97, 0x97, 0x17, 0x57, 0x4f,
	0x6f, 0x6b, 0xeb, 0xa9, 0xb1, 0x90, 0x94, 0x14,
	0x56, 0x4e, 0x6f, 0x6b, 0xeb, 0xa9, 0xb1, 0x90,
	0xd4, 0x5c, 0x3e, 0x26, 0x87, 0x83, 0x13, 0x00
};
static uint8 secdata_rbi[32] =
{
	0x00, 0x00, 0x00, 0x00, 0xb4, 0x00, 0x00, 0x00,
	0x00, 0x6F, 0x00, 0x00, 0x00, 0x00, 0x94, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static uint8 *secptr;

static uint8 VSindex;

static DECLFR(VSSecRead) {
	switch (A) {
	case 0x5e00: VSindex = 0; return X.DB;
	case 0x5e01: return(secptr[(VSindex++) & 0x1F]);
	}
	return(0x00);
}

uint8 coinon = 0;
uint8 coinon2 = 0;
uint8 service = 0;

void FCEU_VSUniCoin(uint8 slot) {
	if (GameInfo->type != GIT_VSUNI) 
		FCEU_DispMessage("Not Vs. System; can't insert coin.", 0);
	else {
		switch (slot) {
		case 0:
			coinon = 6; break;
		case 1:
			coinon2 = 6; break;
		}
	}
}

void FCEU_VSUniService() {
	if (GameInfo->type != GIT_VSUNI)
		FCEU_DispMessage("Not Vs. System; can't press service button.", 0);
	else
		service = 6;
}

static readfunc OldReadPPU;
static writefunc OldWritePPU[2];

static DECLFR(A2002_Gumshoe) {
	return((OldReadPPU(A) & ~0x3F) | 0x1C);
}

static DECLFR(A2002_Topgun) {
	return((OldReadPPU(A) & ~0x3F) | 0x1B);
}

static DECLFR(A2002_MBJ) { // Mighty Bomb Jack
	return((OldReadPPU(A) & ~0x3F) | 0x3D);
}
static DECLFW(B2000_2001_2C05) {
	OldWritePPU[(A & 1) ^ 1](A ^ 1, V);
}

static uint8 xevselect = 0;
static DECLFR(XevRead) {
	if (A == 0x54FF) {
		return(0x5);
	} else if (A == 0x5678) {
		return(xevselect ? 0 : 1);
	} else if (A == 0x578F) {
		return(xevselect ? 0xd1 : 0x89);
	} else if (A == 0x5567) {
		xevselect ^= 1;
		return(xevselect ? 0x37 : 0x3E);
	}
	return(X.DB);
}

void FCEU_VSUniSwap(uint8 *j0, uint8 *j1) {
	if (GameInfo->vs_cswitch) {
		uint16 t = *j0;
		*j0 = (*j0 & 0xC) | (*j1 & 0xF3);
		*j1 = (*j1 & 0xC) | (t & 0xF3);
	}
}

void FCEU_VSUniPower(void) {
	coinon = coinon2 = service = 0;
	VSindex = 0;

	if (secptr)
		SetReadHandler(0x5e00, 0x5e01, VSSecRead);

	switch (GameInfo->vs_ppu) {
	case GIPPU_RP2C04_0001:
	case GIPPU_RP2C04_0002:
	case GIPPU_RP2C04_0003:
	case GIPPU_RP2C04_0004:
		default_palette_selection = GameInfo->vs_ppu;
		break;
	default:
		// nothing todo
		break;
	}
	if (GameInfo->vs_ppu == GIPPU_RC2C05_04) {
		OldReadPPU = GetReadHandler(0x2002);
		SetReadHandler(0x2002, 0x2002, A2002_Topgun);
	} else if (GameInfo->vs_ppu == GIPPU_RC2C05_03) {
		OldReadPPU = GetReadHandler(0x2002);
		SetReadHandler(0x2002, 0x2002, A2002_Gumshoe);
	} else if (GameInfo->vs_ppu == GIPPU_RC2C05_02) {
		OldReadPPU = GetReadHandler(0x2002);
		SetReadHandler(0x2002, 0x2002, A2002_MBJ);
	}
	if (GameInfo->vs_ppu == GIPPU_RC2C05_01 || GameInfo->vs_ppu == GIPPU_RC2C05_02 || GameInfo->vs_ppu == GIPPU_RC2C05_03 || GameInfo->vs_ppu == GIPPU_RC2C05_04) {
		OldWritePPU[0] = GetWriteHandler(0x2000);
		OldWritePPU[1] = GetWriteHandler(0x2001);
		SetWriteHandler(0x2000, 0x2001, B2000_2001_2C05);
	}
	if (GameInfo->vs_type == EGIVS_XEVIOUS) { /* Super Xevious */
		SetReadHandler(0x5400, 0x57FF, XevRead);
	}
}

/* Games that will probably not be supported ever(or for a long time), since they require
 dual-system:

	 Balloon Fight
	 VS Mahjong
	 VS Tennis
	 Wrecking Crew
*/

/* Games/PPU list.  Information copied from MAME.  ROMs are exchangable, so don't take
 this list as "this game must use this PPU".

RP2C04-001:
- Baseball
- Freedom Force
- Gradius
- Hogan's Alley
- Mach Rider (Japan, Fighting Course)
- Pinball
- Platoon
- Super Xevious

RP2C04-002:
- Castlevania
- Ladies golf
- Mach Rider (Endurance Course)
- Raid on Bungeling Bay (Japan)
- Slalom
- Stroke N' Match Golf
- Wrecking Crew

RP2C04-003:
- Dr mario
- Excite Bike
- Goonies
- Soccer
- TKO Boxing

RP2c05-004:
- Clu Clu Land
- Excite Bike (Japan)
- Ice Climber
- Ice Climber Dual (Japan)
- Super Mario Bros.

Rcp2c03b:
- Battle City
- Duck Hunt
- Mahjang
- Pinball (Japan)
- Rbi Baseball
- Star Luster
- Stroke and Match Golf (Japan)
- Super Skykid
- Tennis
- Tetris

RC2C05-01:
- Ninja Jajamaru Kun (Japan)

RC2C05-02:
- Mighty Bomb Jack (Japan)

RC2C05-03:
- Gumshoe

RC2C05-04:
- Top Gun
*/

VSUNIENTRY VSUniGames[] =
{
	{ "Baseball", 0x691d4200ea42be45ULL, 99, 2, GIPPU_RP2C04_0001, 0, 0, EGIVS_NORMAL },
	{ "Battle City", 0x8540949d74c4d0ebULL, 99, 2, GIPPU_RP2C04_0001, 0, 0, EGIVS_NORMAL },
	{ "Battle City(Bootleg)", 0x8093cbe7137ac031ULL, 99, 2, GIPPU_RP2C04_0001, 0, 0, EGIVS_NORMAL },

	{ "Clu Clu Land", 0x1b8123218f62b1eeULL, 99, 2, GIPPU_RP2C04_0004, VS_OPTION_SWAPDIRAB, 0, EGIVS_NORMAL },
	{ "Dr Mario", 0xe1af09c477dc0081ULL, 1, 0, GIPPU_RP2C04_0003, VS_OPTION_SWAPDIRAB, 0, EGIVS_NORMAL },
	{ "Duck Hunt", 0x47735d1e5f1205bbULL, 99, 2, GIPPU_RC2C03B, VS_OPTION_GUN, 0, EGIVS_NORMAL },
	{ "Excitebike", 0x3dcd1401bcafde77ULL, 99, 2, GIPPU_RP2C04_0003, 0, 0, EGIVS_NORMAL },
	{ "Excitebike (J)", 0x7ea51c9d007375f0ULL, 99, 2, GIPPU_RP2C04_0004, 0, 0, EGIVS_NORMAL },
	{ "Freedom Force", 0xed96436bd1b5e688ULL, 4, 0, GIPPU_RP2C04_0001, VS_OPTION_GUN, 0, EGIVS_NORMAL }, /* Wrong color in game select screen? */
	{ "Stroke and Match Golf", 0x612325606e82bc66ULL, 99, 2, GIPPU_RP2C04_0002, VS_OPTION_SWAPDIRAB | VS_OPTION_PREDIP, 0x01, EGIVS_NORMAL },

	{ "Goonies", 0xb4032d694e1d2733ULL, 151, 1, GIPPU_RP2C04_0003, 0, 0, EGIVS_NORMAL },
	{ "Gradius", 0x50687ae63bdad976ULL, 151, 1, GIPPU_RP2C04_0001, VS_OPTION_SWAPDIRAB, 0, EGIVS_NORMAL },
	{ "Gumshoe", 0x87161f8ee37758d3ULL, 99, 2, GIPPU_RC2C05_03, VS_OPTION_GUN, 0, EGIVS_NORMAL },
	{ "Gumshoe", 0xb8500780bf69ce29ULL, 99, 2, GIPPU_RC2C05_03, VS_OPTION_GUN, 0, EGIVS_NORMAL },
	{ "Hogan's Alley", 0xd78b7f0bb621fb45ULL, 99, 2, GIPPU_RP2C04_0001, VS_OPTION_GUN, 0, EGIVS_NORMAL },
	{ "Ice Climber", 0xd21e999513435e2aULL, 99, 2, GIPPU_RP2C04_0004, VS_OPTION_SWAPDIRAB, 0, EGIVS_NORMAL },
	{ "Ladies Golf", 0x781b24be57ef6785ULL, 99, 2, GIPPU_RP2C04_0002, VS_OPTION_SWAPDIRAB | VS_OPTION_PREDIP, 0x1, EGIVS_NORMAL },

	{ "Mach Rider", 0x015672618af06441ULL, 99, 2, GIPPU_RP2C04_0002, 0, 0, EGIVS_NORMAL },
	{ "Mach Rider (J)", 0xa625afb399811a8aULL, 99, 2, GIPPU_RP2C04_0001, 0, 0, EGIVS_NORMAL },
	{ "Mighty Bomb Jack", 0xe6a89f4873fac37bULL, 0, 2, GIPPU_RC2C05_02, 0, 0, EGIVS_NORMAL },
	{ "Ninja Jajamaru Kun", 0xb26a2c31474099c0ULL, 99, 2, GIPPU_RC2C05_01, VS_OPTION_SWAPDIRAB, 0, EGIVS_NORMAL },
	{ "Pinball", 0xc5f49d3def2e9b8ULL, 99, 2, GIPPU_RP2C04_0001, VS_OPTION_PREDIP, 0x01, EGIVS_NORMAL },
	{ "Pinball (J)", 0x66ab1a3828cc901cULL, 99, 2, GIPPU_RC2C03B, VS_OPTION_PREDIP, 0x01, EGIVS_NORMAL },
	{ "Platoon", 0x160f237351c19f1fULL, 68, 1, GIPPU_RP2C04_0001, 0, 0, EGIVS_NORMAL },
	{ "RBI Baseball", 0x6a02d345812938afULL, 4, 1, GIPPU_RP2C04_0001, VS_OPTION_SWAPDIRAB, 0, EGIVS_RBI },
	{ "Soccer", 0xd4e7a9058780eda3ULL, 99, 2, GIPPU_RP2C04_0003, VS_OPTION_SWAPDIRAB, 0, EGIVS_NORMAL },
	{ "Star Luster", 0x8360e134b316d94cULL, 99, 2, GIPPU_RC2C03B, 0, 0, EGIVS_NORMAL },
	{ "Stroke and Match Golf (J)", 0x869bb83e02509747ULL, 99, 2, GIPPU_RC2C03B, VS_OPTION_SWAPDIRAB | VS_OPTION_PREDIP, 0x01, EGIVS_NORMAL },
	{ "Super Sky Kid", 0x78d04c1dd4ec0101ULL, 4, 1, GIPPU_RC2C03B, VS_OPTION_SWAPDIRAB | VS_OPTION_PREDIP, 0x20, EGIVS_NORMAL },

	{ "Super Xevious", 0x2d396247cf58f9faULL, 206, 0, GIPPU_RP2C04_0001, 0, 0, EGIVS_XEVIOUS },
	{ "Tetris", 0x531a5e8eea4ce157ULL, 99, 2, GIPPU_RC2C03B, VS_OPTION_PREDIP, 0x20, EGIVS_NORMAL },
	{ "Top Gun", 0xf1dea36e6a7b531dULL, 2, 0, GIPPU_RC2C05_04, 0, 0, EGIVS_NORMAL },
	{ "VS Castlevania", 0x92fd6909c81305b9ULL, 2, 1, GIPPU_RP2C04_0002, 0, 0, EGIVS_NORMAL },
	{ "VS Slalom", 0x4889b5a50a623215ULL, 0, 1, GIPPU_RP2C04_0002, 0, 0, EGIVS_NORMAL },
	{ "VS Super Mario Bros", 0x39d8cfa788e20b6cULL, 99, 2, GIPPU_RP2C04_0004, 0, 0, EGIVS_NORMAL },
	{ "VS Super Mario Bros [a1]", 0xfc182e5aefbce14dULL, 99, 2, GIPPU_RP2C04_0004, 0, 0, EGIVS_NORMAL },
	{ "VS TKO Boxing", 0x6e1ee06171d8ce3aULL, 4, 1, GIPPU_RP2C04_0003, VS_OPTION_PREDIP, 0x00, EGIVS_TKO },
	{ 0 }
};

void FCEU_VSUniCheck(uint64 md5partial, int *MapperNo, uint8 *Mirroring) {
	VSUNIENTRY *vs = VSUniGames;

	while (vs->name) {
		if (md5partial == vs->md5partial) {
			int32 tofix = 0;
			if (*MapperNo != vs->mapper) {
				tofix |= 1;
				*MapperNo = vs->mapper;
			}
			if (*Mirroring != vs->mirroring) {
				tofix |= 2;
				*Mirroring = vs->mirroring;
			}
			if (GameInfo->type != GIT_VSUNI) {
				tofix |= 4;
				GameInfo->type = GIT_VSUNI;
			}
			if (GameInfo->vs_type != vs->type) {
				tofix |= 8;
				GameInfo->vs_type = vs->type;
			}
			if (vs->ppu && (GameInfo->vs_ppu != vs->ppu)) {
				tofix |= 16;
				GameInfo->vs_ppu = vs->ppu;
			}

			secptr = 0;
			switch (GameInfo->vs_type)
			{
			case EGIVS_RBI: secptr = secdata_rbi; break;
			case EGIVS_TKO: secptr = secdata_tko; break;
			default: secptr = 0; break;
			}

			vsdip = 0x0;
			if (vs->ioption & VS_OPTION_PREDIP) {
				vsdip = vs->predip;
			}
			if ((vs->ioption & VS_OPTION_GUN) && !head.expansion) {
				tofix |= 32;
				GameInfo->input[0] = SI_ZAPPER;
				GameInfo->input[1] = SI_NONE;
				GameInfo->inputfc = SIFC_NONE;
			}
			else if (!head.expansion) {
				GameInfo->input[0] = GameInfo->input[1] = SI_GAMEPAD;
				GameInfo->inputfc = SIFC_NONE;
			}
			if ((vs->ioption & VS_OPTION_SWAPDIRAB) && !GameInfo->vs_cswitch) {
				tofix |= 64;
				GameInfo->vs_cswitch = 1;
			}

			if (tofix)
			{
				char tmpStr[128];
				std::string gigastr;
				gigastr.reserve(768);
				gigastr.assign("The iNES header contains incorrect information.  For now, the information will be corrected in RAM.  ");
				if (tofix & 4) {
					snprintf(tmpStr, sizeof(tmpStr), "Game type should be set to Vs. System.  ");
					gigastr.append(tmpStr);
				}
				if (tofix & 1)
				{
					snprintf(tmpStr, sizeof(tmpStr), "The mapper number should be set to %d.  ", *MapperNo);
					gigastr.append(tmpStr);
				}
				if (tofix & 2)
				{
					const char* mstr[3] = { "Horizontal", "Vertical", "Four-screen" };
					snprintf(tmpStr, sizeof(tmpStr), "Mirroring should be set to \"%s\".  ", mstr[vs->mirroring & 3]);
					gigastr.append(tmpStr);
				}
				if (tofix & 8) {
					const char* mstr[4] = { "Normal", "RBI Baseball protection", "TKO Boxing protection", "Super Xevious protection"};
					snprintf(tmpStr, sizeof(tmpStr), "Vs. System type should be set to \"%s\".  ", mstr[vs->type]);
					gigastr.append(tmpStr);
				}
				if (tofix & 16)
				{
					const char* mstr[10] = { "Default", "RP2C04-0001", "RP2C04-0002", "RP2C04-0003", "RP2C04-0004", "RC2C03B", "RC2C05-01", "RC2C05-02" , "RC2C05-03" , "RC2C05-04" };
					snprintf(tmpStr, sizeof(tmpStr), "Vs. System PPU should be set to \"%s\".  ", mstr[vs->ppu]);
					gigastr.append(tmpStr);
				}
				if (tofix & 32)
				{
					snprintf(tmpStr, sizeof(tmpStr), "The controller type should be set to zapper.  ");
					gigastr.append(tmpStr);
				}
				if (tofix & 64)
				{
					snprintf(tmpStr, sizeof(tmpStr), "The controllers should be swapped.  ");
					gigastr.append(tmpStr);
				}
				gigastr.append("\n");
				FCEU_printf("%s", gigastr.c_str());
			}

			return;
		}
		vs++;
	}
}

void FCEU_VSUniDraw(uint8 *XBuf) {
	uint32 *dest;
	int y, x;

	if (DIPS_howlong-- <= 0) return;

	dest = (uint32*)(XBuf + 256 * 12 + 164);
	for (y = 24; y; y--, dest += (256 - 72) >> 2) {
		for (x = 72 >> 2; x; x--, dest++)
			*dest = 0;
	}

	dest = (uint32*)(XBuf + 256 * (12 + 4) + 164 + 6);
	for (y = 16; y; y--, dest += (256 >> 2) - 16)
		for (x = 8; x; x--) {
			*dest = 0x01010101;
			dest += 2;
		}

	dest = (uint32*)(XBuf + 256 * (12 + 4) + 164 + 6);
	for (x = 0; x < 8; x++, dest += 2) {
		uint32 *da = dest + (256 >> 2);

		if (!((vsdip >> x) & 1))
			da += (256 >> 2) * 10;
		for (y = 4; y; y--, da += 256 >> 2)
			*da = 0;
	}
}


SFORMAT FCEUVSUNI_STATEINFO[] = {
	{ &vsdip, 1, "vsdp" },
	{ &coinon, 1, "vscn" },
	{ &coinon2, 1, "vsc2" },
	{ &service, 1, "vssv" },
	{ &VSindex, 1, "vsin" },
	{ 0 }
};
