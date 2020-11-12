/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020 CaH4e3
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
 * Double Dragon 310000-in-1 (4040R)
 * 700000-in-1 (BS-400R)(Unl)
 */

#include "mapinc.h"
#include "mmc3.h"

static uint8 pointer;
static uint8 offset;
static uint8 *WRAM = NULL;
static uint32 WRAMSIZE;

static int getPRGBankBS4XXXR(int bank)
{
	if (((~bank) & 1) && (pointer & 0x40))
		bank ^= 2;
	return (bank & 2) ? (0xFE | (bank & 1)) : EXPREGS[4 | (bank & 1)];
}

static void BS4XXXRPW(uint32 A, uint8 V) {

	if ((EXPREGS[3] >> 4) & 0x01)
	{
		int AND = ((EXPREGS[0] >> 1) & 1) ? 0x0F : 0x0F;
		int OR = (EXPREGS[0] & 7) << 4;
		int bank0 = getPRGBankBS4XXXR(0);
		int bank1 = getPRGBankBS4XXXR(1);
		if (!((EXPREGS[3] >> 1) & 1)) //16K Mode
		{
			setprg8(0x8000, ((bank0) & AND) | OR);
			setprg8(0xA000, ((bank1) & AND) | OR);
			setprg8(0xC000, ((bank0) & AND) | OR);
			setprg8(0xE000, ((bank1) & AND) | OR);
		}
		else // 32K Mode
		{
			setprg8(0x8000, ((bank0) & AND) | OR);
			setprg8(0xA000, ((bank1) & AND) | OR);
			setprg8(0xC000, ((bank0 | 2) & AND) | OR);
			setprg8(0xE000, ((bank1 | 2) & AND) | OR);
		}
	}
	else // MMC3 Mode
	{	
		int prgAND = ((EXPREGS[0] >> offset) & 1) ? 0x0F : 0x1F;
		int prgOR = (EXPREGS[0] & 7) << 4;
		setprg8(A, (V & prgAND) | (prgOR));
	}
	setprg8r(0x10, 0x6000, 0);
}

static void BS4XXXRCW(uint32 A, uint8 V) {
	if ((EXPREGS[3] >> 4) & 1)
	{
		int AND = ((EXPREGS[0] >> 1) & 1) ? 0x0F : 0x0F;
		int bank = EXPREGS[2] & AND;
		int chrOR = ((EXPREGS[0] >> 3) & 7) << 4;
		setchr8((bank) | (chrOR));
	}
	else
	{
		int chrAND = ((EXPREGS[0] >> 1) & 1) ? 0xFF : 0xFF;
		int chrOR = ((EXPREGS[0] >> 3) & 7) << 7;
		setchr1(A, (V & chrAND) | (chrOR));
	}
}

static DECLFW(BS4XXXRHiWrite) {
//	FCEU_printf("w: %04x-%02x\n",A,V)
	if (A==0x8000)
	{
		pointer = MMC3_cmd ^ V;
	}
	else if (A == 0x8001)
	{
		if((MMC3_cmd & 7) > 5)
			EXPREGS[4|(MMC3_cmd & 1)] = V;
	}
	MMC3_CMDWrite(A, V);
}

static DECLFW(BS4XXXRLoWrite) {
//	FCEU_printf("w: %04x-%02x\n", A, V);
	if (A & 0x800)
	{
		if (!(EXPREGS[3] & 0x80)) {
			EXPREGS[A & 0x03] = V;
			FixMMC3PRG(MMC3_cmd);
			FixMMC3CHR(MMC3_cmd);
		}
		else if (EXPREGS[3] & 0x10)
		{
			EXPREGS[A & 0x03] = V;
			FixMMC3PRG(MMC3_cmd);
			FixMMC3CHR(MMC3_cmd);
		}
		else
			WRAM[A - 0x6000] = V;
	}
	else
		WRAM[A - 0x6000] = V;
}


static void BSXXXXRPower(void) {
	EXPREGS[0] = EXPREGS[1] = EXPREGS[2] = EXPREGS[3] = EXPREGS[4] = EXPREGS[5] = 0;
	GenMMC3Power();
	SetReadHandler(0x6000, 0x7FFF, CartBR);
	SetWriteHandler(0x6000, 0x7FFF, BS4XXXRLoWrite);
	SetWriteHandler(0x8000, 0x9FFF, BS4XXXRHiWrite);
	FixMMC3PRG(MMC3_cmd);
	FixMMC3CHR(MMC3_cmd);
}

static void BS4XXXRClose(void) {
	if (WRAM)
		FCEU_gfree(WRAM);
	WRAM = NULL;
}

void BSXXXXR_Init(CartInfo *info) {
	GenMMC3_Init(info, 512, 256, 8, 0);
	cwrap = BS4XXXRCW;
	pwrap = BS4XXXRPW;

	info->Power = BSXXXXRPower;
	info->Close = BS4XXXRClose;

	WRAMSIZE = 8192;
	WRAM = (uint8*)FCEU_gmalloc(WRAMSIZE);
	SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
	AddExState(WRAM, WRAMSIZE, 0, "WRAM");

	AddExState(EXPREGS, 8, 0, "EXPR");
}

void BS400R_Init(CartInfo *info) {
	offset = 1;
	BSXXXXR_Init(info);
}

void BS4040R_Init(CartInfo *info) {
	offset = 6;
	BSXXXXR_Init(info);
}
