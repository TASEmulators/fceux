/* FCEUmm - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2024 negativeExponent
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

/* NES 2.0 Mapper 471 denotes the Impact Soft IM1 circuit board, used for
 * Haratyler (without HG or MP) and Haraforce. It is basically INES Mapper 201
 * with the addition of a scanline IRQ.*/

#include "mapinc.h"

static uint32 latch;

static void Sync() {
	setprg32(0x8000, latch);
	setchr8(latch);
}

static DECLFW(Write) {
	X6502_IRQEnd(FCEU_IQEXT);
	latch = A;
	Sync();
}

static void Reset() {
	latch = 0;
	Sync();
}

static void Power() {
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x8000, 0xFFFF, Write);
	Reset();
}

static void StateRestore(int version) {
	Sync();
}

static void HBHook() {
	X6502_IRQBegin(FCEU_IQEXT);
}

void Mapper471_Init(CartInfo *info) {
	info->Power = Power;
	info->Reset = Reset;
	GameHBIRQHook = HBHook;
	GameStateRestore = StateRestore;
	AddExState(&latch, sizeof(latch), 0, "LATC");
}
