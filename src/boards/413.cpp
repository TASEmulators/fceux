/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2024
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
#include "../ines.h"

static uint8 reg[4];
static uint8 IRQCount;
static uint8 IRQReload;
static uint8 IRQa;
static uint8 serialControl;
static uint32 serialAddress;

static SFORMAT StateRegs[] = {
	{ reg,            4, "REGS" },
	{ &IRQCount,      1, "IRQC" },
	{ &IRQReload,     1, "IRQR" },
	{ &IRQa,          1, "IRQA" },
	{ &serialAddress, 4, "ADDR" },
	{ &serialControl, 1, "CTRL" },
	{ 0 }
};

static void Sync(void) {
	setprg4(0x5000, 0x01);
	setprg8(0x6000, reg[0]);

	setprg8(0x8000, reg[1]);
	setprg8(0xA000, reg[2]);
	setprg4(0xD000, 0x07);
	setprg8(0xE000, 0x04);

	setchr4(0x0000, reg[3]);
	setchr4(0x1000, ~0x02);
}

static uint64 lreset;
static uint32 laddr;
static DECLFR(M413ReadPCM) {
	uint8 ret = X.DB;
	if ((A == laddr) && ((timestampbase + timestamp) < (lreset + 4))) {
		return ret;
	}
	if (serialControl & 0x02) {
		ret = MiscROM[serialAddress++ & (MiscROM_size - 1)];
	} else {
		ret = MiscROM[serialAddress & (MiscROM_size - 1)];
	}
	laddr = A;
	lreset = timestampbase + timestamp;
	return ret;
}

static DECLFW(M413Write) {
	switch (A & 0xF000) {
	case 0x8000:
		IRQReload = V;
		break;
	case 0x9000:
		IRQCount = 0;
		break;
	case 0xA000:
	case 0xB000:
		IRQa = (A & 0x1000) != 0;
		if (!IRQa) {
			X6502_IRQEnd(FCEU_IQEXT);
		}
		break;
	case 0xC000:
		serialAddress = (serialAddress << 1) | (V >> 7);
		break;
	case 0xD000:
		serialControl = V;
		break;
	case 0xE000:
	case 0xF000:
		reg[V >> 6] = V & 0x3F;
		Sync();
		break;
	}
}

static void M413Power(void) {
	serialAddress = 0;
	serialControl = 0;

	IRQCount = 0;
	IRQReload = 0;
	IRQa = 0;

	reg[0] = 0;
	reg[1] = 0;
	reg[2] = 0;
	reg[3] = 0;

	laddr = 0;
	lreset = 0;

	Sync();

	SetReadHandler(0x4800, 0x4FFF, M413ReadPCM);
	SetReadHandler(0x5000, 0x7FFF, CartBR);
	SetReadHandler(0x8000, 0xBFFF, CartBR);
	SetReadHandler(0xC000, 0xCFFF, M413ReadPCM);
	SetReadHandler(0xD000, 0xFFFF, CartBR);

	SetWriteHandler(0x8000, 0xFFFF, M413Write);
}

static void M413IRQHook(void) {
	if (IRQCount == 0) {
		IRQCount = IRQReload;
	} else {
		IRQCount--;
	}
	if ((IRQCount == 0) && IRQa) {
		X6502_IRQBegin(FCEU_IQEXT);
	}
}

static void StateRestore(int version) {
	Sync();
}

void Mapper413_Init(CartInfo *info) {
	info->Power      = M413Power;
	GameHBIRQHook    = M413IRQHook;
	GameStateRestore = StateRestore;
	AddExState(&StateRegs, ~0, 0, 0);
}
