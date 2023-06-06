/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) Akerasoft 2023
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
 * FDS Conversion
 *
 */

#include "mapinc.h"

static DECLFR(M474ReadCart) {
	return Page[A >> 14][A];
}

static void Sync(void) {
	setchr8(0);
	setprg16(0x4000, 0);
	setprg16(0x8000, 1);
	setprg16(0xC000, 2);
}


static void M474Power(void) {
	SetupCartCHRMapping(0x00, &PRGptr[0][0xC000], 0x2000, 0);
	// not modify fceux other code,
	// fceux load prg is 64k, have 8k chr in 0xC000
	// fceux load chr is 8k,but 0xFF or 0x00
	SetReadHandler(0x4020, 0xFFFF, M474ReadCart);
	Sync();
}

static void M474Power_submapper1(void) {
	SetupCartCHRMapping(0x00, &PRGptr[0][0xC000], 0x2000, 0);
	// not modify fceux other code,
	// fceux load prg is 64k, have 8k chr in 0xC000
	// fceux load chr is 8k,but 0xFF or 0x00
	SetReadHandler(0x4800, 0xFFFF, M474ReadCart);
	Sync();
}

static void StateRestore(int version) {
	Sync();
}

void Mapper474_Init(CartInfo* info) {
	if (info->submapper == 1)
	{
		info->Power = M474Power_submapper1;
	}
	else
	{
		// submapper 0
		info->Power = M474Power;
	}
	GameStateRestore = StateRestore;
}
