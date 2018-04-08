/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2017 CaH4e3
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

static uint8 bios_prg, rom_prg, rom_mode, mirror;

static SFORMAT StateRegs[] =
{
	{ &bios_prg, 1, "BREG" },
	{ &rom_prg, 1, "RREG" },
	{ &rom_mode, 1, "RMODE" },
	{ 0 }
};

static void Sync(void) {
	setchr8(0);
	if(rom_mode&2) {
		setprg16r(0,0x8000,(bios_prg&0xF)|(rom_prg&0x70));
	} else {
		setprg16r(1,0x8000,bios_prg&3);
	}
	setprg16r(0,0xC000,rom_prg&0x7F);
	setmirror(((bios_prg>>4)&1)^1);
}

static DECLFW(BMC80013BWrite) {
	uint8 reg = (A>>13)&3;
	if(reg == 0) {
		bios_prg = V;
	} else {
		rom_prg = V;
		rom_mode = reg;
	}
	Sync();
}

static void BMC80013BPower(void) {
	bios_prg=rom_prg=rom_mode=mirror=0;
	Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, BMC80013BWrite);
}

static void BMC80013BReset(void) {
	bios_prg=rom_prg=rom_mode=mirror=0;
	Sync();
}

static void StateRestore(int version) {
	Sync();
}

void BMC80013B_Init(CartInfo *info) {
	info->Reset = BMC80013BReset;
	info->Power = BMC80013BPower;
	GameStateRestore = StateRestore;
	AddExState(&StateRegs, ~0, 0, 0);
}
