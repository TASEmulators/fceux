/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
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
 * Moero!! Pro Tennis and Moero!! Pro Yakyuu '88 Ketteiban have an ADPCM chip with
 * internal ROM, used for voice samples (not dumped, so emulation isn't possible)
 */

#include "mapinc.h"

static uint8 preg, creg;
static void (*Sync)(void);

static SFORMAT StateRegs[] =
{
	{ &preg, 1, "PREG" },
	{ &creg, 1, "CREG" },
	{ 0 }
};

static void M72Sync(void) {
	setprg16(0x8000, preg);
	setprg16(0xC000, ~0);
	setchr8(creg);
}

static void M92Sync(void) {
	setprg16(0x8000, 0);
	setprg16(0xC000, preg);
	setchr8(creg);
}

static DECLFW(Write) {
	if (V & 0x80)
		preg = V & 0xF;
	if (V & 0x40)
		creg = V & 0xF;
	Sync();
}

static void Power(void) {
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, Write);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper72_Init(CartInfo *info) {
	Sync = M72Sync;
	info->Power = Power;
	GameStateRestore = StateRestore;

	AddExState(&StateRegs, ~0, 0, 0);
}

void Mapper92_Init(CartInfo *info) {
	Sync = M92Sync;
	info->Power = Power;
	GameStateRestore = StateRestore;

	AddExState(&StateRegs, ~0, 0, 0);
}
