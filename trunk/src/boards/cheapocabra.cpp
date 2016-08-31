/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
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

// mapper 111 - Cheapocabra board by Memblers
// http://forums.nesdev.com/viewtopic.php?p=146039
//
// 512k PRG-ROM in 32k pages
// 32k CHR-ROM used as:
//     2 x 8k pattern pages
//     2 x 8k nametable pages
//
// Notes:
// - PRG-ROM is actually flash RAM. Self-flashing should be implemented if battery backed.
// - CHR-RAM for nametables maps to $3000-3FFF as well, but FCEUX internally limits to 4k?

#include "mapinc.h"

static uint8 reg;
static uint8 *CHRRAM = NULL;
static uint32 CHRRAMSIZE;

static SFORMAT StateRegs[] =
{
	{ &reg, 1, "REG" },
	{ 0 }
};

static void Sync(void) {

	// bit 7 controls green LED
	// bit 6 controls red LED
	int nt  = (reg & 0x20) ? 8192 : 0; // bit 5 controls 8k nametable page
	int chr = (reg & 0x10) ? 1 : 0; // bit 4 selects 8k CHR page
	int prg = (reg & 0x0F); // bits 0-3 select 32k PRG page

	nt += (16 * 1024);
	for (int n=0; n<4; ++n)
	{
		setntamem(CHRRAM + nt + (1024 * n),1,n);
	}
	setchr8r(0x10, chr);
	setprg32(0x8000,prg);
}

static DECLFW(M111Write) {
	if ((A >= 0x5000 && A <= 0x5FFF) || (A >= 0x7000 && A <= 0x7FFF))
	{
		reg = V;
		Sync();
	}
}

static void M111Power(void) {
	reg = 0xFF;
	Sync();
	SetReadHandler(0x8000, 0xffff, CartBR);
	SetWriteHandler(0x5000, 0x5fff, M111Write);
	SetWriteHandler(0x7000, 0x7fff, M111Write);
}

static void StateRestore(int version) {
	Sync();
}

void Mapper111_Init(CartInfo *info) {
	info->Power = M111Power;

	CHRRAMSIZE = 1024 * 32;
	CHRRAM = (uint8*)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);

	GameStateRestore = StateRestore;
	AddExState(&StateRegs, ~0, 0, 0);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CRAM");
}
