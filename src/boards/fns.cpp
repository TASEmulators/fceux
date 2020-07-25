/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 1998 BERO
 *  Copyright (C) 2002 Xodnizel
 *  Copyright (C) 2020 CaH4e3
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
 * Famicom Network System Base Unit + MMC1 cartridge board
 *
 */

#include "mapinc.h"

static uint8 DRegs[4];
static uint8 Buffer, BufferShift;

static uint32 WRAMSIZE;
static uint8 *WRAM = NULL;

static int kanji_pos, kanji_page, r40C0;
static int IRQa, IRQCount;

static DECLFW(MBWRAM) {
	if (!(DRegs[3] & 0x10))
		Page[A >> 11][A] = V;
}

static DECLFR(MAWRAM) {
	if (DRegs[3] & 0x10)
		return X.DB;
	return(Page[A >> 11][A]);
}

static void MMC1CHR(void) {
	setchr8((r40C0 >> 3) & 1);
}

static void MMC1PRG(void) {
	uint8 offs_16banks = DRegs[1] & 0x10;
	uint8 prg_reg = DRegs[3] & 0xF;

	setprg8r(0x10, 0x6000, DRegs[1] & 3);

	switch (DRegs[0] & 0xC) {
	case 0xC:
		setprg16(0x8000, (prg_reg + offs_16banks));
		setprg16(0xC000, 0xF + offs_16banks);
		break;
	case 0x8:
		setprg16(0xC000, (prg_reg + offs_16banks));
		setprg16(0x8000, offs_16banks);
		break;
	case 0x0:
	case 0x4:
		setprg16(0x8000, ((prg_reg & ~1) + offs_16banks));
		setprg16(0xc000, ((prg_reg & ~1) + offs_16banks + 1));
		break;
	}
}

static void MMC1MIRROR(void) {
	switch (DRegs[0] & 3) {
	case 2: setmirror(MI_V); break;
	case 3: setmirror(MI_H); break;
	case 0: setmirror(MI_0); break;
	case 1: setmirror(MI_1); break;
	}
}

static uint64 lreset;
static DECLFW(MMC1_write) {
	int n = (A >> 13) - 4;
	if ((timestampbase + timestamp) < (lreset + 2))
		return;

	if (V & 0x80) {
		DRegs[0] |= 0xC;
		BufferShift = Buffer = 0;
		MMC1PRG();
		lreset = timestampbase + timestamp;
		return;
	}

	Buffer |= (V & 1) << (BufferShift++);

	if (BufferShift == 5) {
		FCEU_printf("MMC1 REG%d:%02x (PC %04x)\n", n, Buffer, X.PC);
		DRegs[n] = Buffer;
		BufferShift = Buffer = 0;
		switch (n) {
		case 0: MMC1MIRROR();
		case 1: 
		case 2: 
		case 3: MMC1PRG(); break;
		}
	}
}

static void MMC1_Restore(int version) {
	MMC1MIRROR();
	MMC1CHR();
	MMC1PRG();
	lreset = 0;
}

static void MMC1CMReset(void) {
	int i;

	for (i = 0; i < 4; i++)
		DRegs[i] = 0;
	Buffer = BufferShift = 0;
	DRegs[0] = 0x1F;

	DRegs[1] = 0;
	DRegs[2] = 0;
	DRegs[3] = 0;

	MMC1MIRROR();
	MMC1CHR();
	MMC1PRG();
}

static DECLFW(FNC_cmd_write) {
	switch (A) {
	case 0x40A6: {
		IRQCount = (IRQCount & 0xFF00) | V;
		break;
	}
	case 0x40A7: {
		IRQCount = (IRQCount & 0x00FF) | (V << 8);
		break;
	}
	case 0x40A8: {
		IRQa = V;
		break;
	}
	case 0x40B0: {
		kanji_page = V & 1;
		break;
	}
	case 0x40C0: {
		FCEU_printf("FNS W %04x:%02x (PC %04x)\n", A, V, X.PC);
		r40C0 = V;
		MMC1CHR();
		break;
	}
	default:
		FCEU_printf("FNS W %04x:%02x (PC %04x)\n", A, V, X.PC);
	}
}

static DECLFR(FNC_stat_read) {
	switch (A) {
	case 0x40A2: {
		int ret = (IRQa >> 1);
		X6502_IRQEnd(FCEU_IQEXT);
		IRQa = 0;
		return ret;
	}
	case 0x40AC: {	// NMI/IRQ state reset (lookalike)
		return 0;
	}
	case 0x40B0: {
		kanji_pos = 0;
		return 0;
	}
	case 0x40C0: {
		FCEU_printf("FNS R %04x (PC %04x)\n", A, X.PC);
		int ret = r40C0;
		r40C0 &= 0;
		return ret;
	}
	default: {
		FCEU_printf("FNS R %04x (PC %04x)\n", A, X.PC);
		return 0xff;
	}
	}
}

static DECLFR(FNC_cart_i2c_read) {
	FCEU_printf("I2C R %04x (PC %04x)\n", A, X.PC);
	return 0;
}


static DECLFR(FNC_kanji_read) {
	int32 ofs = ((A & 0xFFF) << 5) + kanji_pos;
	kanji_pos++;
	kanji_pos &= 0x1F;
//	if (PRGptr[1] != NULL)		// iNES debug
		return PRGptr[1][ofs];
//	else
//		return CHRptr[0][ofs];
}

void  NFC_IRQ(int a) {
	if (IRQa) {
		IRQCount -= a;
		if (IRQCount <= 0)
			X6502_IRQBegin(FCEU_IQEXT);
	}
}

static void FNS_Power(void) {
	lreset = 0;
	SetWriteHandler(0x8000, 0xFFFF, MMC1_write);
	SetReadHandler(0x8000, 0xFFFF, CartBR);

	kanji_page = 0;
	kanji_pos = 0;
	r40C0 = 0xC0;

	SetWriteHandler(0x4080, 0x40FF, FNC_cmd_write);
	SetReadHandler(0x4080, 0x40FF, FNC_stat_read);
	SetReadHandler(0x5000, 0x5FFF, FNC_kanji_read);

	SetReadHandler(0x6000, 0x6000, FNC_cart_i2c_read);
	SetReadHandler(0x6001, 0x7FFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);

	FCEU_CheatAddRAM(8, 0x6000, WRAM);
	MMC1CMReset();
}

static void FNS_Close(void) {
	if (WRAM)
		FCEU_gfree(WRAM);
	WRAM = NULL;
}

void FNS_Init(CartInfo *info) {
	info->Close = FNS_Close;
	info->Power = FNS_Power;

	GameStateRestore = MMC1_Restore;
	MapIRQHook = NFC_IRQ;

	WRAMSIZE = (8 + 32) * 1024;
	WRAM = (uint8*)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");

	info->SaveGame[0] = WRAM;
	info->SaveGameLen[0] = WRAMSIZE;

	AddExState(DRegs, 4, 0, "DREG");
	AddExState(&lreset, 8, 1, "LRST");
	AddExState(&Buffer, 1, 1, "BFFR");
	AddExState(&BufferShift, 1, 1, "BFRS");
}
