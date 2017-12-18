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
 * TXC/Micro Genius simplified mapper
 */

#include "mapinc.h"

static uint8 latche, mirr;

static SFORMAT StateRegs[] =
{
	{ &latche, 1, "LATC" },
	{ &mirr, 1, "MIRR" },
	{ 0 }
};

static void Sync(void) {
	setprg32(0x8000, latche >> 4);
	setchr8(latche & 0xf);
}

static DECLFW(M36Write) {
	
	switch((A>>12)&7) {				// need to 4-in-1 MGC-26 BMC, doesnt break other games though
		case 0: mirr = MI_V; setmirror(mirr); break;
		case 4: mirr = MI_H; setmirror(mirr); break;
	}
	latche = V;
	Sync();
}

static DECLFR(M36Read) {
	return latche;  // Need by Strike Wolf, being simplified mapper, this cart still uses some TCX mapper features andrely on it
}

static void M36Power(void) {
	latche = 0;
	Sync();
	SetReadHandler(0x4100, 0x4100, M36Read);
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFE, M36Write);  // Actually, BUS conflict there preventing from triggering the wrong banks
}

static void M36Restore(int version) {
	Sync();
}

void Mapper36_Init(CartInfo *info) {
	info->Power = M36Power;
	GameStateRestore = M36Restore;
	AddExState(StateRegs, ~0, 0, 0);
}
