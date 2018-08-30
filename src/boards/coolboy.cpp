/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2018 CaH4e3, Cluster
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
 * CoolBoy 400-in-1 FK23C-mimic mapper 16Mb/32Mb PROM + 128K/256K CHR RAM, optional SRAM, optional battery
 * only MMC3 mode
 *
 * $6000
 * 7  bit  0
 * ---- ----
 * ABCC DEEE
 * |||| ||||
 * |||| |+++-- PRG offset (PRG A19, A18, A17)
 * |||| +----- Alternate CHR A17
 * ||++------- PRG offset (PRG A24, A23)
 * |+--------- PRG mask (PRG A17 from 0: MMC3; 1: offset)
 * +---------- CHR mask (CHR A17 from 0: MMC3; 1: alternate)
 *
 * $6001
 *
 * 7  bit  0
 * ---- ----
 * GHIJ KKLx
 * |||| |||
 * |||| ||+--- GNROM mode bank PRG size (1: 32 KiB bank, PRG A14=CPU A14; 0: 16 KiB bank, PRG A14=offset A14)
 * |||+-++---- PRG offset (in order: PRG A20, A22, A21)
 * ||+-------- PRG mask (PRG A20 from 0: offset; 1: MMC3)
 * |+--------- PRG mask (PRG A19 from 0: offset; 1: MMC3)
 * +---------- PRG mask (PRG A18 from 0: MMC3; 1: offset)
 *
 * $6002
 * 7  bit  0
 * ---- ----
 * xxxx MMMM
 *      ||||
 *      ++++-- CHR offset for GNROM mode (CHR A16, A15, A14, A13)
 *
 * $6003
 * 7  bit  0
 * ---- ----
 * NPxP QQRx
 * || | |||
 * || | +++--- PRG offset for GNROM mode (PRG A16, A15, A14)
 * || +------- 1: GNROM mode; 0: MMC3 mode
 * || |         (1: PRG A16...13 from QQ, L, R, CPU A14, A13 + CHR A16...10 from MMMM, PPU A12...10;
 * || |          0: PRG A16...13 from MMC3 + CHR A16...A10 from MMC3 )
 * |+-+------- Banking mode
 * |+--------- "Weird MMC3 mode"
 * +---------- Lockout (prevent further writes to these four registers, only works in MMC3 mode)
 *
 * There is also alternative version from MINDKIDS,
 * the only difference is register addresses - 500x instead of 600x
 *
 * Also some new cartridges from MINDKIDS have /WE and /OE pins connected to mapper,
 * which allows you to rewrite flash memory without soldering.
 * This also allows console to write data to the cartridge.
 * This behavior is not emulated.
 * No cart has been discovered so far that makes use of this feature, but this can be used for homebrew.
 *
 */

#include "mapinc.h"
#include "mmc3.h"

static void COOLBOYCW(uint32 A, uint8 V) {
	uint32 mask = 0xFF ^ (EXPREGS[0] & 0x80);
	if (EXPREGS[3] & 0x10) {
		if (EXPREGS[3] & 0x40) { // Weird mode
			int cbase = (MMC3_cmd & 0x80) << 5;
			switch (cbase ^ A) { // Don't even try do understand
			case 0x0400:
			case 0x0C00: V &= 0x7F; break;
			}
		}
		// Highest bit goes from MMC3 registers when EXPREGS[3]&0x80==0 or from EXPREGS[0]&0x08 otherwise
		setchr1(A,
			(V & 0x80 & mask) | ((((EXPREGS[0] & 0x08) << 4) & ~mask)) // 7th bit
			| ((EXPREGS[2] & 0x0F) << 3) // 6-3 bits
			| ((A >> 10) & 7) // 2-0 bits
		);
	} else {
		if (EXPREGS[3] & 0x40) { // Weird mode, again
			int cbase = (MMC3_cmd & 0x80) << 5;
			switch (cbase ^ A) { // Don't even try do understand
			case 0x0000: V = DRegBuf[0]; break;
			case 0x0800: V = DRegBuf[1]; break;
			case 0x0400:
			case 0x0C00: V = 0; break;
			}
		}
		// Simple MMC3 mode
		// Highest bit goes from MMC3 registers when EXPREGS[3]&0x80==0 or from EXPREGS[0]&0x08 otherwise
		setchr1(A, (V & mask) | (((EXPREGS[0] & 0x08) << 4) & ~mask));
	}
}

static void COOLBOYPW(uint32 A, uint8 V) {
	uint32 mask = ((0x3F | (EXPREGS[1] & 0x40) | ((EXPREGS[1] & 0x20) << 2)) ^ ((EXPREGS[0] & 0x40) >> 2)) ^ ((EXPREGS[1] & 0x80) >> 2);
	uint32 base = ((EXPREGS[0] & 0x07) >> 0) | ((EXPREGS[1] & 0x10) >> 1) | ((EXPREGS[1] & 0x0C) << 2) | ((EXPREGS[0] & 0x30) << 2);

	// Very weird mode
	// Last banks are first in this mode, ignored when MMC3_cmd&0x40
	if ((EXPREGS[3] & 0x40) && (V >= 0xFE) && !((MMC3_cmd & 0x40) != 0)) {
		switch (A & 0xE000) {
		case 0xC000:
		case 0xE000:
			V = 0;
			break;
		}
	}

	// Regular MMC3 mode, internal ROM size can be up to 2048kb!
	if (!(EXPREGS[3] & 0x10))
		setprg8(A, (((base << 4) & ~mask)) | (V & mask));
	else { // NROM mode
		mask &= 0xF0;
		uint8 emask;
		if ((((EXPREGS[1] & 2) != 0))) // 32kb mode
			emask = (EXPREGS[3] & 0x0C) | ((A & 0x4000) >> 13);
		else // 16kb mode
			emask = EXPREGS[3] & 0x0E;
		setprg8(A, ((base << 4) & ~mask) // 7-4 bits are from base (see below)
			| (V & mask)                   // ... or from MM3 internal regs, depends on mask
			| emask                        // 3-1 (or 3-2 when (EXPREGS[3]&0x0C is set) from EXPREGS[3]
			| ((A & 0x2000) >> 13));       // 0th just as is
	}
}

static DECLFW(COOLBOYWrite) {
	if(A001B & 0x80)
		CartBW(A,V);

	// Deny any further writes when 7th bit is 1 AND 4th is 0
	if ((EXPREGS[3] & 0x90) != 0x80) {
		EXPREGS[A & 3] = V;
		FixMMC3PRG(MMC3_cmd);
		FixMMC3CHR(MMC3_cmd);
	}
}

static void COOLBOYReset(void) {
	MMC3RegReset();
	EXPREGS[0] = EXPREGS[1] = EXPREGS[2] = EXPREGS[3] = 0;
	FixMMC3PRG(MMC3_cmd);
	FixMMC3CHR(MMC3_cmd);
}

static void COOLBOYPower(void) {
	GenMMC3Power();
	EXPREGS[0] = EXPREGS[1] = EXPREGS[2] = EXPREGS[3] = 0;
	FixMMC3PRG(MMC3_cmd);
	FixMMC3CHR(MMC3_cmd);
	SetWriteHandler(0x5000, 0x5fff, CartBW);            // some games access random unmapped areas and crashes because of KT-008 PCB hack in MMC3 source lol
	SetWriteHandler(0x6000, 0x6fff, COOLBOYWrite);
}

static void MINDKIDSPower(void) {
	GenMMC3Power();
	EXPREGS[0] = EXPREGS[1] = EXPREGS[2] = EXPREGS[3] = 0;
	FixMMC3PRG(MMC3_cmd);
	FixMMC3CHR(MMC3_cmd);
	SetWriteHandler(0x5000, 0x5fff, COOLBOYWrite);
}

void COOLBOY_Init(CartInfo *info) {
	GenMMC3_Init(info, 2048, 256, 8, 1);
	pwrap = COOLBOYPW;
	cwrap = COOLBOYCW;
	info->Power = COOLBOYPower;
	info->Reset = COOLBOYReset;
	AddExState(EXPREGS, 4, 0, "EXPR");
}

void MINDKIDS_Init(CartInfo *info) {
	GenMMC3_Init(info, 2048, 256, 8, 1);
	pwrap = COOLBOYPW;
	cwrap = COOLBOYCW;
	info->Power = MINDKIDSPower;
	info->Reset = COOLBOYReset;
	AddExState(EXPREGS, 4, 0, "EXPR");
}
