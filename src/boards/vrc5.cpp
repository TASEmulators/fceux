/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005-2019 CaH4e3
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
 *
 * VRC-V (CAI Shogakko no Sansu)
 *
 */

#include "mapinc.h"

//#define CAI_DEBUG

// main tiles RAM is 8K in size, but unless other non-CHR ROM type carts,
// this one accesses the $0000 and $1000 pages based on extra NT RAM on board
// which is similar to MMC5 but much simpler because there are no additional
// bankings here.
// extra NT RAM handling is in PPU code now.

static uint16 CHRSIZE = 8192;
// there are two separate WRAMs 8K each, on main system cartridge (not battery
// backed), and one on the daughter cart (with battery). both are accessed
// via the same registers with additional selector flags.
static uint16 WRAMSIZE = 8192 + 8192;
static uint8 *CHRRAM = NULL;
static uint8 *WRAM = NULL;

static uint8 IRQa, K4IRQ;
static uint32 IRQLatch, IRQCount;

// some kind of 16-bit text  encoding (actually 14-bit) used in game resources
// may be converted by the hardware into the tile indexes for internal CHR ROM
// not sure whey they made it hardware, because most of calculations are just
// bit shifting. the main purpose of this table is to calculate actual CHR ROM
// bank for every character. there is a some kind of regularity, so this table
// may be calculated in software easily.

// table read out from hardware registers as is

///*
static uint8 conv_tbl[4][8] = {
	{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
	{ 0x00, 0x00, 0x40, 0x10, 0x28, 0x00, 0x18, 0x30 },
	{ 0x00, 0x00, 0x48, 0x18, 0x30, 0x08, 0x20, 0x38 },
	{ 0x00, 0x00, 0x80, 0x20, 0x38, 0x10, 0x28, 0xB0 }
};
//*/
/*
static uint8 conv_tbl[64][4] = {
	{ 0x40, 0x40, 0x40, 0x40 }, // 00  | A  - 40 41 42 43 44 45 46 47  
	{ 0x41, 0x41, 0x41, 0x41 }, // 02  | B  - 48 49 4A 4B 4C 4D 4E 4F  
	{ 0x42, 0x42, 0x42, 0x42 }, // 04  | C  - 50 51 52 53 54 55 56 57
	{ 0x43, 0x43, 0x43, 0x43 }, // 06  | D  - 58 59 5A 5B 5C 5D 5E 5F
	{ 0x44, 0x44, 0x44, 0x44 }, // 08  | E  - 60 61 62 63 64 65 66 67
	{ 0x45, 0x45, 0x45, 0x45 }, // 0A  | F  - 68 69 6A 6B 6C 6D 6E 6F
	{ 0x46, 0x46, 0x46, 0x46 }, // 0C  | G  - 70 71 72 73 74 75 76 77  
	{ 0x47, 0x47, 0x47, 0x47 }, // 0E  | H  - 78 79 7A 7B 7C 7D 7E 7F
	{ 0x40, 0x40, 0x40, 0x40 }, // 10  | 
	{ 0x41, 0x41, 0x41, 0x41 }, // 12  +----------------------------
	{ 0x42, 0x42, 0x42, 0x42 }, // 14  | A  A  A  A
	{ 0x43, 0x43, 0x43, 0x43 }, // 16  | A  A  A  A
	{ 0x44, 0x44, 0x44, 0x44 }, // 18  | A  A' B' A"
	{ 0x45, 0x45, 0x45, 0x45 }, // 1A  | A  C  D  E
	{ 0x46, 0x46, 0x46, 0x46 }, // 1C  | A  F  G  H
	{ 0x47, 0x47, 0x47, 0x47 }, // 1E  | A  A  B  C
	{ 0x40, 0x40, 0x48, 0x44 }, // 20  | A  D  E  F
	{ 0x41, 0x41, 0x49, 0x45 }, // 22  | A  G  H  G"
	{ 0x42, 0x42, 0x4A, 0x46 }, // 24  +----------------------------
	{ 0x43, 0x43, 0x4B, 0x47 }, // 26  | A' - 40 41 42 43 40 41 42 43  
	{ 0x44, 0x40, 0x48, 0x44 }, // 28  | A" - 44 45 46 47 44 45 46 47
	{ 0x45, 0x41, 0x49, 0x45 }, // 2A  | B' - 48 49 4A 4B 48 49 4A 4B
	{ 0x46, 0x42, 0x4A, 0x46 }, // 2C  | G" - 74 75 76 77 74 75 76 77
	{ 0x47, 0x43, 0x4B, 0x47 }, // 2E
	{ 0x40, 0x50, 0x58, 0x60 }, // 30
	{ 0x41, 0x51, 0x59, 0x61 }, // 32
	{ 0x42, 0x52, 0x5A, 0x62 }, // 34
	{ 0x43, 0x53, 0x5B, 0x63 }, // 36
	{ 0x44, 0x54, 0x5C, 0x64 }, // 38
	{ 0x45, 0x55, 0x5D, 0x65 }, // 3A
	{ 0x46, 0x56, 0x5E, 0x66 }, // 3C
	{ 0x47, 0x57, 0x5F, 0x67 }, // 3E
	{ 0x40, 0x68, 0x70, 0x78 }, // 40
	{ 0x41, 0x69, 0x71, 0x79 }, // 42
	{ 0x42, 0x6A, 0x72, 0x7A }, // 44
	{ 0x43, 0x6B, 0x73, 0x7B }, // 46
	{ 0x44, 0x6C, 0x74, 0x7C }, // 48
	{ 0x45, 0x6D, 0x75, 0x7D }, // 4A
	{ 0x46, 0x6E, 0x76, 0x7E }, // 4C
	{ 0x47, 0x6F, 0x77, 0x7F }, // 4E
	{ 0x40, 0x40, 0x48, 0x50 }, // 50
	{ 0x41, 0x41, 0x49, 0x51 }, // 52
	{ 0x42, 0x42, 0x4A, 0x52 }, // 54
	{ 0x43, 0x43, 0x4B, 0x53 }, // 56
	{ 0x44, 0x44, 0x4C, 0x54 }, // 58
	{ 0x45, 0x45, 0x4D, 0x55 }, // 5A
	{ 0x46, 0x46, 0x4E, 0x56 }, // 5C
	{ 0x47, 0x47, 0x4F, 0x57 }, // 5E
	{ 0x40, 0x58, 0x60, 0x68 }, // 60
	{ 0x41, 0x59, 0x61, 0x69 }, // 62
	{ 0x42, 0x5A, 0x62, 0x6A }, // 64
	{ 0x43, 0x5B, 0x63, 0x6B }, // 66
	{ 0x44, 0x5C, 0x64, 0x6C }, // 68
	{ 0x45, 0x5D, 0x65, 0x6D }, // 6A
	{ 0x46, 0x5E, 0x66, 0x6E }, // 6C
	{ 0x47, 0x5F, 0x67, 0x6F }, // 6E
	{ 0x40, 0x70, 0x78, 0x74 }, // 70
	{ 0x41, 0x71, 0x79, 0x75 }, // 72
	{ 0x42, 0x72, 0x7A, 0x76 }, // 74
	{ 0x43, 0x73, 0x7B, 0x77 }, // 76
	{ 0x44, 0x74, 0x7C, 0x74 }, // 78
	{ 0x45, 0x75, 0x7D, 0x75 }, // 7A
	{ 0x46, 0x76, 0x7E, 0x76 }, // 7C
	{ 0x47, 0x77, 0x7F, 0x77 }, // 7E
};
*/

static uint8 regs[16];
static SFORMAT StateRegs[] =
{
	{ &IRQCount, 1, "IRQC" },
	{ &IRQLatch, 1, "IRQL" },
	{ &IRQa, 1, "IRQA" },
	{ &K4IRQ, 1, "KIRQ" },
	{ regs, 16, "REGS" },
	{ 0 }
};

static void chrSync(void) {
	setchr4r(0x10, 0x0000, regs[5] & 1);
	// 30.06.19 CaH4e3 there is much more complicated behaviour with second banking register, you may actually
	// view the content of the internal character CHR rom via this window, but it is useless because hardware
	// does not use this area to access the internal ROM. not sure why they did this, but I see no need to
	// emulate this behaviour carefully, unless I find something that I missed...
	setchr4r(0x10, 0x1000, 1);
}

static void Sync(void) {
	chrSync();
	setprg4r(0x10, 0x6000, (regs[0] & 1) | (regs[0] >> 2));	// two 4K banks are identical, either internal or excernal
	setprg4r(0x10, 0x7000, (regs[1] & 1) | (regs[1] >> 2)); // SRAMs may be mapped in any bank independently
	if (PRGptr[1] == NULL) {	// for iNES 2.0 version it even more hacky lol
		setprg8(0x8000, (regs[2] & 0x3F) + ((regs[2] & 0x40) >> 2));
		setprg8(0xA000, (regs[3] & 0x3F) + ((regs[3] & 0x40) >> 2));
		setprg8(0xC000, (regs[4] & 0x3F) + ((regs[4] & 0x40) >> 2));
		setprg8(0xE000, 0x10 + 0x3F);
	} else {
		setprg8r((regs[2] >> 6) & 1, 0x8000, (regs[2] & 0x3F));
		setprg8r((regs[3] >> 6) & 1, 0xA000, (regs[3] & 0x3F));
		setprg8r((regs[4] >> 6) & 1, 0xC000, (regs[4] & 0x3F));
		setprg8r(1, 0xE000, ~0);	// always sees the last bank of the external cart, so can't be booted without it.
	}
	setmirror(((regs[0xA]&2)>>1)^1);
}

static DECLFW(QTAiWrite) {
	regs[(A & 0x0F00) >> 8] = V;	// IRQ pretty the same as in other VRC mappers by Konami
	switch (A) {
	case 0xd600: IRQLatch &= 0xFF00; IRQLatch |= V; break;
	case 0xd700: IRQLatch &= 0x00FF; IRQLatch |= V << 8; break;
	case 0xd900: IRQCount = IRQLatch; IRQa = V & 2; K4IRQ = V & 1; X6502_IRQEnd(FCEU_IQEXT); break;
	case 0xd800: IRQa = K4IRQ; X6502_IRQEnd(FCEU_IQEXT); break;
	case 0xda00: qtaintramreg = regs[0xA] & 3;	break; // register shadow to share it with ppu
	}
	Sync();
}

static DECLFR(QTAiRead) {

//	uint8 res1 = conv_tbl[(regs[0xD] & 0x7F) >> 1][(regs[0xC] >> 5) & 3];
//	uint8 res2 = ((regs[0xD] & 1) << 7) | ((regs[0xC] & 0x1F) << 2) | (regs[0xB] & 3);

	uint8 tabl = conv_tbl[(regs[0xC] >> 5) & 3][(regs[0xD] & 0x7F) >> 4];
	uint8 res1 = 0x40 | (tabl & 0x3F) | ((regs[0xD] >> 1) & 7) | ((regs[0xB] & 4) << 5);
	uint8 res2 = ((regs[0xD] & 1) << 7) | ((regs[0xC] & 0x1F) << 2) | (regs[0xB] & 3);
	
	if (tabl & 0x40)
		res1 &= 0xFB;
	else if (tabl & 0x80)
		res1 |= 0x04;

	if (A == 0xDD00) {
		return res1;
	} else if (A == 0xDC00) {
#ifdef CAI_DEBUG
		FCEU_printf("%02x:%02x+%d -> %02x:%02x\n", regs[0xD], regs[0xC], regs[0xB], res1, res2);
#endif
		return res2;
	} else
		return 0;
}

static void VRC5IRQ(int a) {
	if (IRQa) {
		IRQCount += a;
		if (IRQCount & 0x10000) {
			X6502_IRQBegin(FCEU_IQEXT);
			IRQCount = IRQLatch;
		}
	}
}

static void QTAiPower(void) {
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	SetWriteHandler(0x8000, 0xFFFF, QTAiWrite);
	SetReadHandler(0xDC00, 0xDC00, QTAiRead);
	SetReadHandler(0xDD00, 0xDD00, QTAiRead);
	FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
	Sync();
}

static void QTAiClose(void) {
	if (CHRRAM)
		FCEU_gfree(CHRRAM);
	CHRRAM = NULL;
	if (WRAM)
		FCEU_gfree(WRAM);
	WRAM = NULL;
}

static void StateRestore(int version) {
	Sync();
}

void QTAi_Init(CartInfo *info) {
	QTAIHack = 1;

	info->Power = QTAiPower;
	info->Close = QTAiClose;
	GameStateRestore = StateRestore;

	MapIRQHook = VRC5IRQ;

	CHRRAM = (uint8*)FCEU_gmalloc(CHRSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRSIZE, 1);
	AddExState(CHRRAM, CHRSIZE, 0, "CRAM");

	WRAM = (uint8*)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");

	if (info->battery) {
		info->SaveGame[0] = WRAM;
		// note, only extrnal cart's SRAM is battery backed, the the part on the main cartridge is just
		// an additional work ram. so we may save only half here, but I forgot what part is saved lol, will 
		// find out later.
		info->SaveGameLen[0] = WRAMSIZE;
	}

	AddExState(&StateRegs, ~0, 0, 0);
}
