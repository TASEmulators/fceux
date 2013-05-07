/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2007 CaH4e3
 *  Copyright (C) 2011 FCEUX team
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
 * Bandai mappers
 *
 */

//Famicom Jump 2 should get transformed to m153
//All other games are not supporting EEPROM saving right now.
//We may need to distinguish between 16 and 159 in order to know the EEPROM configuration.
//Until then, we just return 0x00 from the EEPROM read

#include "mapinc.h"

static uint8 reg[16], is153;
static uint8 IRQa;
static int16 IRQCount, IRQLatch;

static uint8 *WRAM = NULL;
static uint32 WRAMSIZE;

static SFORMAT StateRegs[] =
{
	{ reg, 16, "REGS" },
	{ &IRQa, 1, "IRQA" },
	{ &IRQCount, 2, "IRQC" },
	{ &IRQLatch, 2, "IRQL" }, // need for Famicom Jump II - Saikyou no 7 Nin (J) [!]
	{ 0 }
};

static void BandaiIRQHook(int a) {
	if (IRQa) {
		IRQCount -= a;
		if (IRQCount < 0) {
			X6502_IRQBegin(FCEU_IQEXT);
			IRQa = 0;
			IRQCount = -1;
		}
	}
}

static void BandaiSync(void) {
	if (is153) {
		int base = (reg[0] & 1) << 4;
		setchr8(0);
		setprg16(0x8000, (reg[8] & 0x0F) | base);
		setprg16(0xC000, 0x0F | base);
	} else {
		int i;
		for (i = 0; i < 8; i++) setchr1(i << 10, reg[i]);
		setprg16(0x8000, reg[8]);
		setprg16(0xC000, ~0);
	}
	switch (reg[9] & 3) {
	case 0: setmirror(MI_V); break;
	case 1: setmirror(MI_H); break;
	case 2: setmirror(MI_0); break;
	case 3: setmirror(MI_1); break;
	}
}

static DECLFW(BandaiWrite) {
	A &= 0x0F;
	if (A < 0x0A) {
		reg[A & 0x0F] = V;
		BandaiSync();
	} else
		switch (A) {
		case 0x0A: X6502_IRQEnd(FCEU_IQEXT); IRQa = V & 1; IRQCount = IRQLatch; break;
		case 0x0B: IRQLatch &= 0xFF00; IRQLatch |= V; break;
		case 0x0C: IRQLatch &= 0xFF; IRQLatch |= V << 8; break;
		case 0x0D: break;	// Serial EEPROM control port
		}
}

static DECLFR(BandaiRead) {
	return 0xef;    // TODO: EEPROM
}

static void BandaiPower(void) {
	BandaiSync();
	SetReadHandler(0x6000, 0x7FFF, BandaiRead);
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0xFFFF, BandaiWrite);
}

static void StateRestore(int version) {
	BandaiSync();
}

void Mapper16_Init(CartInfo *info) {
	is153 = 0;
	info->Power = BandaiPower;
	MapIRQHook = BandaiIRQHook;
	GameStateRestore = StateRestore;
	AddExState(&StateRegs, ~0, 0, 0);
}

// Famicom jump 2:
// 0-7: Lower bit of data selects which 256KB PRG block is in use.
// This seems to be a hack on the developers' part, so I'll make emulation
// of it a hack(I think the current PRG block would depend on whatever the
// lowest bit of the CHR bank switching register that corresponds to the
// last CHR address read).

static void M153Power(void) {
	BandaiSync();
	setprg8r(0x10, 0x6000, 0);
	SetReadHandler(0x6000, 0x7FFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, CartBW);
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, BandaiWrite);
}


static void M153Close(void) {
	if (WRAM)
		FCEU_gfree(WRAM);
	WRAM = NULL;
}

void Mapper153_Init(CartInfo *info) {
	is153 = 1;
	info->Power = M153Power;
	info->Close = M153Close;
	MapIRQHook = BandaiIRQHook;

	WRAMSIZE = 8192;
	WRAM = (uint8*)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");

	if (info->battery) {
		info->SaveGame[0] = WRAM;
		info->SaveGameLen[0] = WRAMSIZE;
	}

	GameStateRestore = StateRestore;
	AddExState(&StateRegs, ~0, 0, 0);
}

// Datach Barcode Battler

static uint8 BarcodeData[256];
static int BarcodeReadPos;
static int BarcodeCycleCount;
static uint32 BarcodeOut;

int FCEUI_DatachSet(const uint8 *rcode) {
	int prefix_parity_type[10][6] = {
		{ 0, 0, 0, 0, 0, 0 }, { 0, 0, 1, 0, 1, 1 }, { 0, 0, 1, 1, 0, 1 }, { 0, 0, 1, 1, 1, 0 },
		{ 0, 1, 0, 0, 1, 1 }, { 0, 1, 1, 0, 0, 1 }, { 0, 1, 1, 1, 0, 0 }, { 0, 1, 0, 1, 0, 1 },
		{ 0, 1, 0, 1, 1, 0 }, { 0, 1, 1, 0, 1, 0 }
	};
	int data_left_odd[10][7] = {
		{ 0, 0, 0, 1, 1, 0, 1 }, { 0, 0, 1, 1, 0, 0, 1 }, { 0, 0, 1, 0, 0, 1, 1 }, { 0, 1, 1, 1, 1, 0, 1 },
		{ 0, 1, 0, 0, 0, 1, 1 }, { 0, 1, 1, 0, 0, 0, 1 }, { 0, 1, 0, 1, 1, 1, 1 }, { 0, 1, 1, 1, 0, 1, 1 },
		{ 0, 1, 1, 0, 1, 1, 1 }, { 0, 0, 0, 1, 0, 1, 1 }
	};
	int data_left_even[10][7] = {
		{ 0, 1, 0, 0, 1, 1, 1 }, { 0, 1, 1, 0, 0, 1, 1 }, { 0, 0, 1, 1, 0, 1, 1 }, { 0, 1, 0, 0, 0, 0, 1 },
		{ 0, 0, 1, 1, 1, 0, 1 }, { 0, 1, 1, 1, 0, 0, 1 }, { 0, 0, 0, 0, 1, 0, 1 }, { 0, 0, 1, 0, 0, 0, 1 },
		{ 0, 0, 0, 1, 0, 0, 1 }, { 0, 0, 1, 0, 1, 1, 1 }
	};
	int data_right[10][7] = {
		{ 1, 1, 1, 0, 0, 1, 0 }, { 1, 1, 0, 0, 1, 1, 0 }, { 1, 1, 0, 1, 1, 0, 0 }, { 1, 0, 0, 0, 0, 1, 0 },
		{ 1, 0, 1, 1, 1, 0, 0 }, { 1, 0, 0, 1, 1, 1, 0 }, { 1, 0, 1, 0, 0, 0, 0 }, { 1, 0, 0, 0, 1, 0, 0 },
		{ 1, 0, 0, 1, 0, 0, 0 }, { 1, 1, 1, 0, 1, 0, 0 }
	};
	uint8 code[13 + 1];
	uint32 tmp_p = 0;
	int i, j;
	int len;

	for (i = len = 0; i < 13; i++) {
		if (!rcode[i]) break;
		if ((code[i] = rcode[i] - '0') > 9)
			return(0);
		len++;
	}
	if (len != 13 && len != 12 && len != 8 && len != 7) return(0);

	#define BS(x) BarcodeData[tmp_p] = x; tmp_p++

	for (j = 0; j < 32; j++) {
		BS(0x00);
	}

	/* Left guard bars */
	BS(1); BS(0); BS(1);

	if (len == 13 || len == 12) {
		uint32 csum;

		for (i = 0; i < 6; i++)
			if (prefix_parity_type[code[0]][i]) {
				for (j = 0; j < 7; j++) {
					BS(data_left_even[code[i + 1]][j]);
				}
			} else
				for (j = 0; j < 7; j++) {
					BS(data_left_odd[code[i + 1]][j]);
				}

		/* Center guard bars */
		BS(0); BS(1); BS(0); BS(1); BS(0);

		for (i = 7; i < 12; i++)
			for (j = 0; j < 7; j++) {
				BS(data_right[code[i]][j]);
			}
		csum = 0;
		for (i = 0; i < 12; i++) csum += code[i] * ((i & 1) ? 3 : 1);
		csum = (10 - (csum % 10)) % 10;
		for (j = 0; j < 7; j++) {
			BS(data_right[csum][j]);
		}
	} else if (len == 8 || len == 7) {
		uint32 csum = 0;

		for (i = 0; i < 7; i++) csum += (i & 1) ? code[i] : (code[i] * 3);

		csum = (10 - (csum % 10)) % 10;

		for (i = 0; i < 4; i++)
			for (j = 0; j < 7; j++) {
				BS(data_left_odd[code[i]][j]);
			}


		/* Center guard bars */
		BS(0); BS(1); BS(0); BS(1); BS(0);

		for (i = 4; i < 7; i++)
			for (j = 0; j < 7; j++) {
				BS(data_right[code[i]][j]);
			}

		for (j = 0; j < 7; j++) {
			BS(data_right[csum][j]);
		}
	}

	/* Right guard bars */
	BS(1); BS(0); BS(1);

	for (j = 0; j < 32; j++) {
		BS(0x00);
	}

	BS(0xFF);

	#undef BS

	BarcodeReadPos = 0;
	BarcodeOut = 0x8;
	BarcodeCycleCount = 0;
	return(1);
}

static void BarcodeIRQHook(int a) {
	BandaiIRQHook(a);

	BarcodeCycleCount += a;

	if (BarcodeCycleCount >= 1000) {
		BarcodeCycleCount -= 1000;
		if (BarcodeData[BarcodeReadPos] == 0xFF) {
			BarcodeOut = 0;
		} else {
			BarcodeOut = (BarcodeData[BarcodeReadPos] ^ 1) << 3;
			BarcodeReadPos++;
		}
	}
}

static DECLFR(BarcodeRead) {
	return BarcodeOut;
}

static void M157Power(void) {
	BarcodeData[0] = 0xFF;
	BarcodeReadPos = 0;
	BarcodeOut = 0;
	BarcodeCycleCount = 0;

	BandaiSync();

	SetWriteHandler(0x6000, 0xFFFF, BandaiWrite);
	SetReadHandler(0x6000, 0x7FFF, BarcodeRead);
	SetReadHandler(0x8000, 0xFFFF, CartBR);
}

void Mapper157_Init(CartInfo *info) {
	is153 = 0;
	info->Power = M157Power;
	MapIRQHook = BarcodeIRQHook;

	GameInfo->cspecial = SIS_DATACH;

	GameStateRestore = StateRestore;
	AddExState(&StateRegs, ~0, 0, 0);
}
