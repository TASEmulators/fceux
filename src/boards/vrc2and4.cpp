/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2007 CaH4e3
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

#include "mapinc.h"

static bool isPirate;
static uint8 is22, reg1mask, reg2mask;
static uint16 IRQCount;
static uint8 IRQLatch, IRQa;
static uint8 prgreg[2], chrreg[8];
static uint16 chrhi[8];
static uint8 regcmd, irqcmd, mirr, big_bank;
static uint16 acount = 0;

static uint8 *WRAM = NULL;
static uint32 WRAMSIZE;

static SFORMAT StateRegs[] =
{
	{ prgreg, 2, "PREG" },
	{ chrreg, 8, "CREG" },
	{ chrhi, 16, "CRGH" },
	{ &acount, 2, "ACNT" },
	{ &regcmd, 1, "CMDR" },
	{ &irqcmd, 1, "CMDI" },
	{ &mirr, 1, "MIRR" },
	{ &big_bank, 1, "BIGB" },
	{ &IRQCount, 2, "IRQC" },
	{ &IRQLatch, 1, "IRQL" },
	{ &IRQa, 1, "IRQA" },
	{ 0 }
};

static void Sync(void) {
	if (regcmd & 2) {
		setprg8(0xC000, prgreg[0] | big_bank);
		setprg8(0x8000, ((~1) & 0x1F) | big_bank);
	} else {
		setprg8(0x8000, prgreg[0] | big_bank);
		setprg8(0xC000, ((~1) & 0x1F) | big_bank);
	}
	setprg8(0xA000, prgreg[1] | big_bank);
	setprg8(0xE000, ((~0) & 0x1F) | big_bank);
	if (UNIFchrrama)
		setchr8(0);
	else{
		uint8 i;
		for (i = 0; i < 8; i++)
			setchr1(i << 10, (chrhi[i] | chrreg[i]) >> is22);
	}
	switch (mirr & 0x3) {
	case 0: setmirror(MI_V); break;
	case 1: setmirror(MI_H); break;
	case 2: setmirror(MI_0); break;
	case 3: setmirror(MI_1); break;
	}
}

static DECLFW(VRC24Write) {
	A = A & 0xF000 | !!(A & reg2mask) << 1 | !!(A & reg1mask);
	if ((A >= 0xB000) && (A <= 0xE003)) {
		if (UNIFchrrama)
			big_bank = (V & 8) << 2;							// my personally many-in-one feature ;) just for support pirate cart 2-in-1
		else{
			uint16 i = ((A >> 1) & 1) | ((A - 0xB000) >> 11);
			uint16 nibble = ((A & 1) << 2);
			chrreg[i] = (chrreg[i] & (0xF0 >> nibble)) | ((V & 0xF) << nibble);
			if(nibble)
				chrhi[i] = (V & 0x10) << 4;						// another one many in one feature from pirate carts
		}
		Sync();
	} else
		switch (A & 0xF003) {
		case 0x8000:
		case 0x8001:
		case 0x8002:
		case 0x8003:
			if (!isPirate) {
				prgreg[0] = V & 0x1F;
				Sync();
			}
			break;
		case 0xA000:
		case 0xA001:
		case 0xA002:
		case 0xA003:
			if (!isPirate)
				prgreg[1] = V & 0x1F;
			else{
				prgreg[0] = (V & 0x1F) << 1;
				prgreg[1] = ((V & 0x1F) << 1) | 1;
			}
			Sync();
			break;
		case 0x9000:
		case 0x9001: if (V != 0xFF) mirr = V; Sync(); break;
		case 0x9002:
		case 0x9003: regcmd = V; Sync(); break;
		case 0xF000: X6502_IRQEnd(FCEU_IQEXT); IRQLatch &= 0xF0; IRQLatch |= V & 0xF; break;
		case 0xF001: X6502_IRQEnd(FCEU_IQEXT); IRQLatch &= 0x0F; IRQLatch |= V << 4; break;
		case 0xF002: X6502_IRQEnd(FCEU_IQEXT); acount = 0; IRQCount = IRQLatch; IRQa = V & 2; irqcmd = V & 1; break;
		case 0xF003: X6502_IRQEnd(FCEU_IQEXT); IRQa = irqcmd; break;
		}
}

static void VRC24Power(void) {
	big_bank = 0x20;
	Sync();
	if (WRAM) {
		setprg8r(0x10, 0x6000, 0);
		SetReadHandler(0x6000, 0x7FFF, CartBR);
		SetWriteHandler(0x6000, 0x7FFF, CartBW);
		FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
	}
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, VRC24Write);
}

void VRC24IRQHook(int a) {
	#define LCYCS 341
	if (IRQa) {
		acount += a * 3;
		if (acount >= LCYCS) {
			while (acount >= LCYCS) {
				acount -= LCYCS;
				IRQCount++;
				if (IRQCount & 0x100) {
					X6502_IRQBegin(FCEU_IQEXT);
					IRQCount = IRQLatch;
				}
			}
		}
	}
}

static void StateRestore(int version) {
	Sync();
}

static void VRC24Close(void) {
	if (WRAM)
		FCEU_gfree(WRAM);
	WRAM = NULL;
}

static void VRC24_Init(CartInfo *info) {
	info->Power = VRC24Power;
	info->Close = VRC24Close;
	MapIRQHook = VRC24IRQHook;
	GameStateRestore = StateRestore;

	WRAMSIZE = 8192;
	WRAM = (uint8*)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");

	if(info->battery) {
		info->SaveGame[0]=WRAM;
		info->SaveGameLen[0]=WRAMSIZE;
	}

	AddExState(&StateRegs, ~0, 0, 0);
}

void Mapper21_Init(CartInfo *info) {
	isPirate = false;
	is22 = 0;
	reg1mask = 0x42;
	reg2mask = 0x84;
	VRC24_Init(info);
}

void Mapper22_Init(CartInfo *info) {
	isPirate = false;
	is22 = 1;
	reg1mask = 2;
	reg2mask = 1;

	// no IRQ (all mapper 22 games are VRC2)
	// no WRAM
	info->Power = VRC24Power;
	GameStateRestore = StateRestore;

	AddExState(&StateRegs, ~0, 0, 0);
}

void Mapper23_Init(CartInfo *info) {
	isPirate = false;
	is22 = 0;
	reg1mask = 0x15;
	reg2mask = 0x2a;
	VRC24_Init(info);
}

void Mapper25_Init(CartInfo *info) {
	isPirate = false;
	is22 = 0;
	reg1mask = 0xa;
	reg2mask = 0x5;
	VRC24_Init(info);
}

void UNLT230_Init(CartInfo *info) {
	isPirate = true;
	is22 = 0;
	reg1mask = 0x15;
	reg2mask = 0x2a;
	VRC24_Init(info);
}
