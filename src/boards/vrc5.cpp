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
 * VRC-5 (CAI Shogakko no Sansu)
 *
 */

#include "mapinc.h"

#define CAI_DEBUG

static writefunc old2006wrap;
static writefunc old2007wrap;
static uint8 QTAINTRAM[2048];
static uint16 qtaintramofs;

static uint16 CHRSIZE = 8192;
static uint16 WRAMSIZE = 8192 + 8192;
static uint8 *CHRRAM = NULL;
static uint8 *WRAM = NULL;

static uint8 IRQa, K4IRQ;
static uint32 IRQLatch, IRQCount;

static uint8 conv_tbl[128][4] = {
	{ 0x40, 0x40, 0x40, 0x40 },
	{ 0x41, 0x41, 0x41, 0x41 },
	{ 0x42, 0x42, 0x42, 0x42 },
	{ 0x43, 0x43, 0x43, 0x43 },
	{ 0x44, 0x44, 0x44, 0x44 },
	{ 0x45, 0x45, 0x45, 0x45 },
	{ 0x46, 0x46, 0x46, 0x46 },
	{ 0x47, 0x47, 0x47, 0x47 },
	{ 0x40, 0x40, 0x40, 0x40 },
	{ 0x41, 0x41, 0x41, 0x41 },
	{ 0x42, 0x42, 0x42, 0x42 },
	{ 0x43, 0x43, 0x43, 0x43 },
	{ 0x44, 0x44, 0x44, 0x44 },
	{ 0x45, 0x45, 0x45, 0x45 },
	{ 0x46, 0x46, 0x46, 0x46 },
	{ 0x47, 0x47, 0x47, 0x47 },
	{ 0x40, 0x40, 0x48, 0x44 },
	{ 0x41, 0x41, 0x49, 0x45 },
	{ 0x42, 0x42, 0x4A, 0x46 },
	{ 0x43, 0x43, 0x4B, 0x47 },
	{ 0x44, 0x40, 0x48, 0x44 },
	{ 0x45, 0x41, 0x49, 0x45 },
	{ 0x46, 0x42, 0x4A, 0x46 },
	{ 0x47, 0x43, 0x4B, 0x47 },
	{ 0x40, 0x50, 0x58, 0x60 },
	{ 0x41, 0x51, 0x59, 0x61 },
	{ 0x42, 0x52, 0x5A, 0x62 },
	{ 0x43, 0x53, 0x5B, 0x63 },
	{ 0x44, 0x54, 0x5C, 0x64 },
	{ 0x45, 0x55, 0x5D, 0x65 },
	{ 0x46, 0x56, 0x5E, 0x66 },
	{ 0x47, 0x57, 0x5F, 0x67 },
	{ 0x40, 0x68, 0x70, 0x78 },
	{ 0x41, 0x69, 0x71, 0x79 },
	{ 0x42, 0x6A, 0x72, 0x7A },
	{ 0x43, 0x6B, 0x73, 0x7B },
	{ 0x44, 0x6C, 0x74, 0x7C },
	{ 0x45, 0x6D, 0x75, 0x7D },
	{ 0x46, 0x6E, 0x76, 0x7E },
	{ 0x47, 0x6F, 0x77, 0x7F },
	{ 0x40, 0x40, 0x48, 0x50 },
	{ 0x41, 0x41, 0x49, 0x51 },
	{ 0x42, 0x42, 0x4A, 0x52 },
	{ 0x43, 0x43, 0x4B, 0x53 },
	{ 0x44, 0x44, 0x4C, 0x54 },
	{ 0x45, 0x45, 0x4D, 0x55 },
	{ 0x46, 0x46, 0x4E, 0x56 },
	{ 0x47, 0x47, 0x4F, 0x57 },
	{ 0x40, 0x58, 0x60, 0x68 },
	{ 0x41, 0x59, 0x61, 0x69 },
	{ 0x42, 0x5A, 0x62, 0x6A },
	{ 0x43, 0x5B, 0x63, 0x6B },
	{ 0x44, 0x5C, 0x64, 0x6C },
	{ 0x45, 0x5D, 0x65, 0x6D },
	{ 0x46, 0x5E, 0x66, 0x6E },
	{ 0x47, 0x5F, 0x67, 0x6F },
	{ 0x40, 0x70, 0x78, 0x74 },
	{ 0x41, 0x71, 0x79, 0x75 },
	{ 0x42, 0x72, 0x7A, 0x76 },
	{ 0x43, 0x73, 0x7B, 0x77 },
	{ 0x44, 0x74, 0x7C, 0x74 },
	{ 0x45, 0x75, 0x7D, 0x75 },
	{ 0x46, 0x76, 0x7E, 0x76 },
	{ 0x47, 0x77, 0x7F, 0x77 },
};

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
// one more hack to make visible some screens in common. will be replaced with proper code later
	setchr4r(0x10, 0x1000, QTAINTRAM[0] & 1);
//	setchr4r(0x10, 0x1000, 1);
}

static void Sync(void) {
	chrSync();
	setprg4r(0x10, 0x6000, (regs[0] & 1) | (regs[0] >> 2));
	setprg4r(0x10, 0x7000, (regs[1] & 1) | (regs[1] >> 2));
#ifdef CAI_DEBUG
	setprg8(0x8000, regs[2]);
	setprg8(0xA000, regs[3]);
	setprg8(0xC000, regs[4]);
	setprg8(0xE000, ~0);
#else
	setprg8r((regs[2] >> 6) & 1, 0x8000, (regs[2] & 0x3F));
	setprg8r((regs[3] >> 6) & 1, 0xA000, (regs[3] & 0x3F));
	setprg8r((regs[4] >> 6) & 1, 0xC000, (regs[4] & 0x3F));
	setprg8r(1, 0xE000, ~0);
#endif
	setmirror(((regs[0xA]&2)>>1)^1);
}

static DECLFW(QTAiWrite) {
	regs[(A & 0x0F00) >> 8] = V;
	switch (A) {
	case 0xd600: {
		IRQLatch &= 0xFF00;
		IRQLatch |= V;
#ifdef CAI_DEBUG
		FCEU_printf("irq latch lo=%02x\n", V);
#endif
		break;
	}
	case 0xd700: {
		IRQLatch &= 0x00FF;
		IRQLatch |= V << 8;
#ifdef CAI_DEBUG
		FCEU_printf("irq latch hi=%02x\n", V);
#endif
		break;
	}
	case 0xd900: {
		IRQCount = IRQLatch;
		IRQa = V & 2;
		K4IRQ = V & 1;
		X6502_IRQEnd(FCEU_IQEXT);
#ifdef CAI_DEBUG
		FCEU_printf("irq reload\n", V);
#endif
		break;
	}
	case 0xd800: {
		IRQa = K4IRQ;
		X6502_IRQEnd(FCEU_IQEXT);
#ifdef CAI_DEBUG
		FCEU_printf("irq stop\n", V);
#endif
		break;
	}
	default:
#ifdef CAI_DEBUG
		FCEU_printf("write %04x:%04x %d, %d\n", A, V, scanline, timestamp);
#endif
	}
	Sync();
}

#ifdef CAI_DEBUG
static DECLFR(DebugExtNT) {
	return QTAINTRAM[A & 0x07FF];
}
#endif

static DECLFR(QTAiRead) {
	//	OH = conv_tbl[DD00 >> 1][DC00 >> 6]
	//	OL = ((DC00 & 0x3F) << 2) + DB00
	if (A == 0xDD00)
		return conv_tbl[regs[0xD] >> 1][regs[0xC] >> 6];
	else if (A == 0xDC00)
		return ((regs[0xC] & 0x3F) << 2) + regs[0xB];
	else
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

// debug hack. mapper does not track ppu address himseld, instead the regular ppu offset is used
// these handlers must be moved in ppu code in order to be emulated properly.
//
static DECLFW(QTAi2006Wrap) {
	if (regs[0xA] & 1)
		qtaintramofs = (qtaintramofs << 8) | V;
	else
		old2006wrap(0x2006, V);
}

static DECLFW(QTAi2007Wrap) {
	if (regs[0xA] & 1) {
		QTAINTRAM[qtaintramofs & 0x07FF] = V;
		qtaintramofs++;
	} else
		old2007wrap(0x2007, V);
}

static void QTAiPower(void) {

	QTAIHack = 1;
	qtaintramofs = 0;

	old2006wrap = GetWriteHandler(0x2006);
	old2007wrap = GetWriteHandler(0x2007);
	SetWriteHandler(0x2006, 0x2006, QTAi2006Wrap);
	SetWriteHandler(0x2007, 0x2007, QTAi2007Wrap);

#ifdef CAI_DEBUG
	SetReadHandler(0x5000, 0x5FFF, DebugExtNT);
#endif

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
		info->SaveGameLen[0] = WRAMSIZE - 4096;
	}

	AddExState(&StateRegs, ~0, 0, 0);
}
