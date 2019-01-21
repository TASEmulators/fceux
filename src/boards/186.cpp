/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 CaH4e3
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
 * Family Study Box by Fukutake Shoten
 *
 * REG[0] R dddddddd / W ww---sss
 *  ddd - TAPE DATA BYTE (ready when IRQ occurs)
 *  sss - BRAM hi-bank
 *  ww  - PRAM bank
 * REG[1] R 0123---- / W ----PPPP
 *  0   - ?
 *  1   - ?
 *  2   - ?
 *  3   - ?
 *  PPPP- PROM bank
 * REG[2] R -?--R--- / W A-BC-DEF
 *  4   - ?
 *  R   - sb4x power supply status (active low)
 *  A   - ?
 *  B   - ?
 *  C   - ?
 *  D   - ?
 *  E   - ?
 *  F   - ?
 *
 * BRAM0	4400-4FFF, 3K bank  0   (32K SWRAM)	 [hardwired]
 * BRAMB	5000-5FFF, 4K banks 1-7 (32K SWRAM)	 [REG[0] W -----sss]
 * PRAMB	6000-7FFF, 8K banks 1-3 (32K PRAM) 	 [REG[0] W ww------]
 * PROMB    8000-BFFF, 16K banks 1-15 (256K PROM)[REG[1] W ----PPPP]
 * PROM0	C000-FFFF, 16K bank 0   (256K PROM)  [hardwired]
 *
 */

#include "mapinc.h"

static uint8 SWRAM[3072];
static uint8 *WRAM = NULL;
static uint8 regs[4];

static SFORMAT StateRegs[] =
{
	{ regs, 4, "DREG" },
	{ SWRAM, 3072, "SWRM" },
	{ 0 }
};

static void Sync(void) {
	setprg8r(0x10, 0x6000, regs[0] >> 6);
	setprg16(0x8000, regs[1]);
	setprg16(0xc000, 0);
}

static DECLFW(M186Write) {
	if (A & 0x4203) regs[A & 3] = V;
	Sync();
}

static DECLFR(M186Read) {
	switch (A) {
	case 0x4200: return 0x00; break;
	case 0x4201: return 0x00; break;
	case 0x4202: return 0x40; break;
	case 0x4203: return 0x00; break;
	}
	return 0xFF;
}

static DECLFR(ASWRAM) {
	return(SWRAM[A - 0x4400]);
}
static DECLFW(BSWRAM) {
	SWRAM[A - 0x4400] = V;
}

static void M186Power(void) {
	setchr8(0);
	SetReadHandler(0x6000, 0xFFFF, CartBR);
	SetWriteHandler(0x6000, 0xFFFF, CartBW);
	SetReadHandler(0x4200, 0x43FF, M186Read);
	SetWriteHandler(0x4200, 0x43FF, M186Write);
	SetReadHandler(0x4400, 0x4FFF, ASWRAM);
	SetWriteHandler(0x4400, 0x4FFF, BSWRAM);
	FCEU_CheatAddRAM(32, 0x6000, WRAM);
	regs[0] = regs[1] = regs[2] = regs[3];
	Sync();
}

static void M186Close(void) {
	if (WRAM)
		FCEU_gfree(WRAM);
	WRAM = NULL;
}

static void M186Restore(int version) {
	Sync();
}

void Mapper186_Init(CartInfo *info) {
	info->Power = M186Power;
	info->Close = M186Close;
	GameStateRestore = M186Restore;
	WRAM = (uint8*)FCEU_gmalloc(32768);
	SetupCartPRGMapping(0x10, WRAM, 32768, 1);
	AddExState(WRAM, 32768, 0, "WRAM");
	AddExState(StateRegs, ~0, 0, 0);
}
