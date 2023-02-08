/* FCEUmm - NES/Famicom Emulator
*
* Copyright notice for this file:
*  Copyright (C) 2022
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

static uint16 latchAddr;
static uint8  latchData;
static uint8  submapper;

static SFORMAT StateRegs[] =
{
	{ &latchAddr, 2, "ADDR" },
	{ &latchData, 1, "DATA" },
	{ 0 }
};

static void Mapper354_Sync(void)
{
	int prg = latchData & 0x3F | latchAddr << 2 & 0x40 | latchAddr >> 5 & 0x80;
	switch (latchAddr & 7)
	{
	case 0: case 4:
		setprg32(0x8000, prg >> 1);
		break;
	case 1:
		setprg16(0x8000, prg);
		setprg16(0xC000, prg | 7);
		break;
	case 2: case 6:
		setprg8(0x8000, prg << 1 | latchData >> 7);
		setprg8(0xA000, prg << 1 | latchData >> 7);
		setprg8(0xC000, prg << 1 | latchData >> 7);
		setprg8(0xE000, prg << 1 | latchData >> 7);
		break;
	case 3: case 7:
		setprg16(0x8000, prg);
		setprg16(0xC000, prg);
		break;
	case 5:
		setprg8(0x6000, prg << 1 | latchData >> 7);
		setprg32(0x8000, prg >> 1 | 3);
		break;
	}
	setchr8(0);
	setmirror(latchData & 0x40 ? MI_H : MI_V);
}

static DECLFW(Mapper354_WriteLatch)
{
	latchData = V;
	latchAddr = A & 0xFFFF;
	Mapper354_Sync();
}

static void Mapper354_Reset(void)
{
	latchAddr = latchData = 0;
	Mapper354_Sync();
}

static void Mapper354_Power(void)
{
	latchAddr = latchData = 0;
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(submapper == 1 ? 0xE000 : 0xF000, 0xFFFF, Mapper354_WriteLatch);
	Mapper354_Sync();
}

static void StateRestore(int version) {
	Mapper354_Sync();
}

void Mapper354_Init(CartInfo *info)
{
	submapper = info->submapper;
	info->Power = Mapper354_Power;
	info->Reset = Mapper354_Reset;
	GameStateRestore = StateRestore;
	AddExState(StateRegs, ~0, 0, 0);
}
