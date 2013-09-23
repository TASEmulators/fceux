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
 *
 * FDS Conversions
 *
 * Super Mario Bros 2 J alt version is a BAD incomplete dump, should be mapper 43
 *
 * Both Voleyball and Zanac by Whirlind Manu shares the same PCB, but with
 * some differences: Voleyball has 8K CHR ROM and 8K ROM at 6000K, Zanac
 * have 8K CHR RAM and banked 16K ROM mapper at 6000 as two 8K banks.
 *
 * PCB for this mapper is "09-034A"
 */

#include "mapinc.h"

static uint8 prg;

static SFORMAT StateRegs[] =
{
	{ &prg, 1, "PRG" },
	{ 0 }
};

static void Sync(void) {
	setprg8r(1, 0x6000, prg);
	setprg32(0x8000, 0);
	setchr8(0);
}

static DECLFW(UNLSMB2JWrite) {
	prg = V & 1;
	Sync();
}

static void UNLSMB2JPower(void) {
	prg = 0;
	Sync();
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x4027, 0x4027, UNLSMB2JWrite);
}

static void StateRestore(int version) {
	Sync();
}

void UNLSMB2J_Init(CartInfo *info) {
	info->Power = UNLSMB2JPower;
	GameStateRestore = StateRestore;
	AddExState(&StateRegs, ~0, 0, 0);
}
