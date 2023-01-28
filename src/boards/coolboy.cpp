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
 * SMD132 and SMD133 ASICs, MMC3 clones that can address up to 32 MiB of PRG-ROM, 256 KiB of CHR-RAM, and 8 KiB of WRAM.
 *
 * COOLBOY cartridges use registers at address $6xxx
 * MINDKIDS cartridges use a solder pad labelled "5/6K" to select between $5000 and $6000
 *
 * $xxx0
 * 7  bit  0
 * ---- ----
 * ABCC DEEE
 * |||| ||||
 * |||| |+++-- PRG offset (PRG A19, A18, A17)
 * |||| +----- Alternate CHR A17
 * ||++------- PRG offset (PRG A24, A23), CHR offset (CHR A19, A18)
 * |+--------- PRG mask (PRG A17 from 0: MMC3; 1: offset)
 * +---------- CHR mask (CHR A17 from 0: MMC3; 1: alternate)
 *
 * $xxx1
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
 * $xxx2
 * 7  bit  0
 * ---- ----
 * xxxx MMMM
 *      ||||
 *      ++++-- CHR offset for GNROM mode (CHR A16, A15, A14, A13)
 *
 * $xxx3
 * 7  bit  0
 * ---- ----
 * NPZP QQRx
 * |||| |||
 * |||| +++--- PRG offset for GNROM mode (PRG A16, A15, A14)
 * |||+------- 1: GNROM mode; 0: MMC3 mode
 * ||||         (1: PRG A16...13 from QQ, L, R, CPU A14, A13 + CHR A16...10 from MMMM, PPU A12...10;
 * ||||          0: PRG A16...13 from MMC3 + CHR A16...A10 from MMC3 )
 * ||+-------- 1: Also enable PRG RAM in $5000-$5FFF
 * |+-+------- Banking mode
 * |+--------- "Weird MMC3 mode"
 * +---------- Lockout (prevent further writes to all registers but the one at $xxx2, only works in MMC3 mode)
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
#include "../ines.h"

const int ROM_CHIP = 0x00;
const int CFI_CHIP = 0x11;
const int FLASH_CHIP = 0x12;

const uint32 FLASH_SECTOR_SIZE = 128 * 1024;

extern uint8* WRAM;
static uint8* CFI = NULL;
static uint8* Flash = NULL;

static uint8 flash_save = 0;
static uint8 flash_state = 0;
static uint16 flash_buffer_a[10];
static uint8 flash_buffer_v[10];
static uint8 cfi_mode = 0;

static uint16 regs_base = 0;
static uint8 flag23 = 0;
static uint8 flag45 = 0;
static uint8 flag67 = 0;
static uint8 flag89 = 0;

// Macronix 256-mbit memory CFI data
const uint8 cfi_data[] =
{ 
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0x51, 0x52, 0x59, 0x02, 0x00, 0x40, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x27, 0x36, 0x00, 0x00, 0x03,
	0x06, 0x09, 0x13, 0x03, 0x05, 0x03, 0x02, 0x19,
	0x02, 0x00, 0x06, 0x00, 0x01, 0xFF, 0x00, 0x00,
	0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
	0x50, 0x52, 0x49, 0x31, 0x33, 0x14, 0x02, 0x01,
	0x00, 0x08, 0x00, 0x00, 0x02, 0x95, 0xA5, 0x05,
	0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
	0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
};

static void AA6023CW(uint32 A, uint8 V) {
	if (flag89) {
		/*
			$xxx0
			7  bit  0
			---- ----
			AB.C DEEE
			|| | ||||
			|| | |+++-- PRG offset (PRG A19, A18, A17)
			|| | +----- Alternate CHR A17
			|| +------- 1=Write-protect CHR-RAM
			|+--------- PRG mask (PRG A17 from 0: MMC3; 1: offset)
			+---------- CHR mask (CHR A17 from 0: MMC3; 1: alternate)
		*/
		if (EXPREGS[0] & 0b00010000)
			SetupCartCHRMapping(0, VROM, CHRsize[0], 0); // write-protect CHR-RAM
		else
			SetupCartCHRMapping(0, VROM, CHRsize[0], 1); // allow CHR writes
	}

	uint32 mask = 0xFF ^ (EXPREGS[0] & 0b10000000);
	if (EXPREGS[3] & 0b00010000) {
		if (EXPREGS[3] & 0b01000000) { // Weird mode
			int cbase = (MMC3_cmd & 0x80) << 5;
			switch (cbase ^ A) { // Don't even try do understand
			case 0x0400:
			case 0x0C00: V &= 0x7F; break;
			}
		}
		// Highest bit goes from MMC3 registers when EXPREGS[0]&0x80==0 or from EXPREGS[0]&0x08 otherwise
		setchr1(A,
			(V & 0x80 & mask) | ((((EXPREGS[0] & 0b00001000) << 4) & ~mask)) // 7th bit
			| ((EXPREGS[2] & 0x0F) << 3) // 6-3 bits
			| ((A >> 10) & 7) // 2-0 bits
			| ((EXPREGS[0] & 0b00110000) << 4) // There are some ROMs with 1 MiB CHR-ROM
		);
	}
	else {
		if (EXPREGS[3] & 0b01000000) { // Weird mode, again
			int cbase = (MMC3_cmd & 0x80) << 5;
			switch (cbase ^ A) { // Don't even try do understand
			case 0x0000: V = DRegBuf[0]; break;
			case 0x0800: V = DRegBuf[1]; break;
			case 0x0400:
			case 0x0C00: V = 0; break;
			}
		}
		// Simple MMC3 mode
		// Highest bit goes from MMC3 registers when EXPREGS[0]&0x80==0 or from EXPREGS[0]&0x08 otherwise
		setchr1(A, 
			(V & mask)
			| (((EXPREGS[0] & 0x08) << 4) & ~mask)
			| ((EXPREGS[0] & 0b00110000) << 4)); // There are some ROMs with 1 MiB CHR-ROM
	}
}

static void AA6023PW(uint32 A, uint8 V) {
	uint8 CREGS[] = {EXPREGS[0], EXPREGS[1], EXPREGS[2], EXPREGS[3]};
	// Submappers has scrambled bits
	if (flag23) {
		/*
			$xxx1
			7  bit  0
			---- ----
			GHIL JKKx
			|||| |||
			|||| +++--- PRG offset (in order: PRG A20, A21, A22)
			|||+------- GNROM mode bank PRG size (0: 32 KiB bank, PRG A14=CPU A14; 1: 16 KiB bank, PRG A14=offset A14)
			||+-------- PRG mask (PRG A20 from 0: offset; 1: MMC3)
			|+--------- PRG mask (PRG A19 from 0: offset; 1: MMC3)
			+---------- PRG mask (PRG A18 from 0: MMC3; 1: offset)
		*/
		CREGS[1] = (CREGS[1] & 0b11100101)
			| ((CREGS[1] & 0b00001000) << 1) // PRG A20
			| ((CREGS[1] & 0b00000010) << 2) // PRG A22
			| ((((CREGS[1] ^ 0b00010000) & 0b00010000) >> 3)); // GNROM mode bank PRG size
	}
	if (flag45) {
		/*
			$xxx0
			7  bit  0
			---- ----
			ABCC DEEE
			|||| ||||
			|||| |+++-- PRG offset (PRG A19, A18, A17)
			|||| +----- Alternate CHR A17
			||++------- PRG offset (PRG A21, A20)
			|+--------- PRG mask (PRG A17 from 0: MMC3; 1: offset)
			+---------- CHR mask (CHR A17 from 0: MMC3; 1: alternate)
			$xxx1
			7  bit  0
			---- ----
			GHIx xxLx
			|||    |
			|||    +--- GNROM mode bank PRG size (1: 32 KiB bank, PRG A14=CPU A14; 0: 16 KiB bank, PRG A14=offset A14)
			||+-------- PRG mask (PRG A20 from 0: offset; 1: MMC3)
			|+--------- PRG mask (PRG A19 from 0: offset; 1: MMC3)
			+---------- PRG mask (PRG A18 from 0: MMC3; 1: offset)
		*/
		CREGS[1] = (CREGS[1] & 0b11100011)
			| ((CREGS[0] & 0b00100000) >> 3) // PRG A21
			| (CREGS[0] & 0b00010000); // PRG A20
		CREGS[0] &= 0b11001111;
	}

	uint32 mask = ((0b00111111 | (CREGS[1] & 0b01000000) | ((CREGS[1] & 0b00100000) << 2)) ^ ((CREGS[0] & 0b01000000) >> 2)) ^ ((CREGS[1] & 0b10000000) >> 2);
	uint32 base = ((CREGS[0] & 0b00000111) >> 0) | ((CREGS[1] & 0b00010000) >> 1) | ((CREGS[1] & 0b00001100) << 2) | ((CREGS[0] & 0b00110000) << 2);

	if (flash_save && cfi_mode) {
		setprg32r(CFI_CHIP, 0x8000, 0);
		return;
	}

	int chip = !flash_save ? ROM_CHIP : FLASH_CHIP;
	// There are ROMs with multiple PRG ROM chips
	int chip_offset = 0;
	if (flag67 && EXPREGS[0] & 0b00001000) {
		chip_offset += ROM_size;
	}

	// Very weird mode
	// Last banks are first in this mode, ignored when MMC3_cmd&0x40
	if ((CREGS[3] & 0b01000000) && (V >= 0xFE) && !((MMC3_cmd & 0x40) != 0)) {
		switch (A & 0xE000) {
		case 0xC000:
		case 0xE000:
			V = 0;
			break;
		}
	}

	if (!(CREGS[3] & 0x10)) {
		// Regular MMC3 mode but can be extended to 2MiB
		setprg8r(chip, A, ((((base << 4) & ~mask)) | (V & mask)) + chip_offset);
	}
	else {
		// NROM mode
		mask &= 0xF0;
		uint8 emask;
		if (CREGS[1] & 0b00000010) // 32kb mode
			emask = (CREGS[3] & 0b00001100) | ((A & 0x4000) >> 13);
		else // 16kb mode
			emask = CREGS[3] & 0b00001110;
		setprg8r(chip, A, (
			((base << 4) & ~mask)	// 7-4 bits are from base
			| (V & mask)			// ... or from MM3 internal regs, depends on mask
			| emask					// 3-1 (or 3-2 when (EXPREGS[3]&0x0C is set) from EXPREGS[3]
			| ((A & 0x2000) >> 13)	// 0th just as is
			) + chip_offset);       // For multi-chip ROMs
	}
}

static DECLFW(AA6023WramWrite) {
	if (A001B & 0x80)
		CartBW(A, V);
}

static DECLFW(AA6023Write) {
	if (A >= 0x6000) {
		AA6023WramWrite(A, V);
	}

	// Deny any further writes when 7th bit is 1 AND 4th is 0
	if ((EXPREGS[3] & 0x90) != 0x80) {
		EXPREGS[A & 3] = V;
	}
	FixMMC3PRG(MMC3_cmd);
	FixMMC3CHR(MMC3_cmd);
}

static DECLFW(AA6023FlashWrite) {
	if (A < 0xC000)
		MMC3_CMDWrite(A, V);
	else
		MMC3_IRQWrite(A, V);

	if (!flash_save) return;
	if (flash_state < sizeof(flash_buffer_a) / sizeof(flash_buffer_a[0])) {
		flash_buffer_a[flash_state] = A & 0xFFF;
		flash_buffer_v[flash_state] = V;
		flash_state++;

		// enter CFI mode
		if ((flash_state == 1) &&
			(flash_buffer_a[0] == 0x0AAA) && (flash_buffer_v[0] == 0x98)) {
			cfi_mode = 1;
			flash_state = 0;
		}

		// erase sector
		if ((flash_state == 6) &&
			(flash_buffer_a[0] == 0x0AAA) && (flash_buffer_v[0] == 0xAA) &&
			(flash_buffer_a[1] == 0x0555) && (flash_buffer_v[1] == 0x55) &&
			(flash_buffer_a[2] == 0x0AAA) && (flash_buffer_v[2] == 0x80) &&
			(flash_buffer_a[3] == 0x0AAA) && (flash_buffer_v[3] == 0xAA) &&
			(flash_buffer_a[4] == 0x0555) && (flash_buffer_v[4] == 0x55) &&
			(flash_buffer_v[5] == 0x30)) {
			int offset = &Page[A >> 11][A] - Flash;
			int sector = offset / FLASH_SECTOR_SIZE;
			for (uint32 i = sector * FLASH_SECTOR_SIZE; i < (sector + 1) * FLASH_SECTOR_SIZE; i++)
				Flash[i % PRGsize[ROM_CHIP]] = 0xFF;
			FCEU_printf("Flash sector #%d is erased (0x%08x - 0x%08x).\n", sector, offset, offset + FLASH_SECTOR_SIZE);
			flash_state = 0;
		}

		// erase chip, lol
		if ((flash_state == 6) &&
			(flash_buffer_a[0] == 0x0AAA) && (flash_buffer_v[0] == 0xAA) &&
			(flash_buffer_a[1] == 0x0555) && (flash_buffer_v[1] == 0x55) &&
			(flash_buffer_a[2] == 0x0AAA) && (flash_buffer_v[2] == 0x80) &&
			(flash_buffer_a[3] == 0x0AAA) && (flash_buffer_v[3] == 0xAA) &&
			(flash_buffer_a[4] == 0x0555) && (flash_buffer_v[4] == 0x55) &&
			(flash_buffer_v[5] == 0x10)) {
			memset(Flash, 0xFF, PRGsize[ROM_CHIP]);
			FCEU_printf("Flash chip erased.\n");
			flash_state = 0;
		}

		// write byte
		if ((flash_state == 4) &&
			(flash_buffer_a[0] == 0x0AAA) && (flash_buffer_v[0] == 0xAA) &&
			(flash_buffer_a[1] == 0x0555) && (flash_buffer_v[1] == 0x55) &&
			(flash_buffer_a[2] == 0x0AAA) && (flash_buffer_v[2] == 0xA0)) {
			int offset = &Page[A >> 11][A] - Flash;
			if (CartBR(A) != 0xFF) {
				FCEU_PrintError("Error: can't write to 0x%08x, flash sector is not erased.\n", offset);
			}
			else {
				CartBW(A, V);
			}
			flash_state = 0;
		}
	}

	// not a command
	if (((A & 0xFFF) != 0x0AAA) && ((A & 0xFFF) != 0x0555)) {
		flash_state = 0;
	}

	// reset
	if (V == 0xF0) {
		flash_state = 0;
		cfi_mode = 0;
	}

	FixMMC3PRG(MMC3_cmd);
}

static void AA6023Reset(void) {
	MMC3RegReset();
	EXPREGS[0] = EXPREGS[1] = EXPREGS[2] = EXPREGS[3] = 0;
	flash_state = 0;
	cfi_mode = 0;
	FixMMC3PRG(MMC3_cmd);
	FixMMC3CHR(MMC3_cmd);
}

static void AA6023Power(void) {
	GenMMC3Power();
	EXPREGS[0] = EXPREGS[1] = EXPREGS[2] = EXPREGS[3] = 0;
	FixMMC3PRG(MMC3_cmd);
	FixMMC3CHR(MMC3_cmd);
	if (regs_base != 0x5000)
		SetWriteHandler(0x5000, 0x5fff, CartBW);            // some games access random unmapped areas and crashes because of KT-008 PCB hack in MMC3 source lol
	SetWriteHandler(0x6000, 0x7fff, AA6023WramWrite);
	SetWriteHandler(regs_base, regs_base + 0x0fff, AA6023Write);
	SetWriteHandler(0x8000, 0xFFFF, AA6023FlashWrite);
	SetReadHandler(0x8000, 0xFFFF, CartBR);
}

static void AA6023Restore(int version) {
	FixMMC3PRG(MMC3_cmd);
	FixMMC3CHR(MMC3_cmd);
}

static void AA6023Close(void) {
	if (WRAM)
		FCEU_gfree(WRAM);
	if (Flash)
		FCEU_gfree(Flash);
	if (CFI) 
		FCEU_gfree(CFI);
	WRAM = Flash = CFI = NULL;
}

void CommonInit(CartInfo* info, int submapper)
{	
	GenMMC3_Init(info, 2048, info->vram_size / 1024, !info->ines2 ? 8 : (info->wram_size + info->battery_wram_size) / 1024, info->battery);
	pwrap = AA6023PW;
	cwrap = AA6023CW;

	switch (submapper)
	{
	case 2:
		regs_base = 0x7000;
		break;
	case 0:
	case 4:
	case 6:
	case 8:
		regs_base = 0x6000;
		break;
	case 1:
	case 3:
	case 5:
	case 7:
	case 9:
		regs_base = 0x5000;
		break;
	default:
		FCEU_PrintError("Submapper #%d is not supported", submapper);
	}
	flag23 = (submapper == 2) || (submapper == 3);
	flag45 = (submapper == 4) || (submapper == 5);
	flag67 = (submapper == 6) || (submapper == 7);
	flag89 = (submapper == 8) || (submapper == 9);
	info->Power = AA6023Power;
	info->Reset = AA6023Reset;
	info->Close = AA6023Close;
	GameStateRestore = AA6023Restore;

	flash_save = info->battery;

	if (flash_save) {
		CFI = (uint8*)FCEU_gmalloc(sizeof(cfi_data) * 2);
		for (size_t i = 0; i < sizeof(cfi_data); i++) {
			CFI[i * 2] = CFI[i * 2 + 1] = cfi_data[i];
		}
		SetupCartPRGMapping(CFI_CHIP, CFI, sizeof(cfi_data) * 2, 0);
		Flash = (uint8*)FCEU_gmalloc(PRGsize[ROM_CHIP]);
		for (unsigned int i = 0; i < PRGsize[ROM_CHIP]; i++) {
			Flash[i] = PRGptr[ROM_CHIP][i % PRGsize[ROM_CHIP]];
		}
		SetupCartPRGMapping(FLASH_CHIP, Flash, PRGsize[ROM_CHIP], 1);
		info->SaveGame[1] = Flash;
		info->SaveGameLen[1] = PRGsize[ROM_CHIP];
	}

	AddExState(EXPREGS, 4, 0, "EXPR");
	if (flash_save)
	{
		AddExState(&flash_state, sizeof(flash_state), 0, "FLST");
		AddExState(flash_buffer_a, sizeof(flash_buffer_a), 0, "FLBA");
		AddExState(flash_buffer_v, sizeof(flash_buffer_v), 0, "FLBV");
		AddExState(&cfi_mode, sizeof(cfi_mode), 0, "CFIM");
		AddExState(Flash, PRGsize[ROM_CHIP], 0, "FLAS");
	}
}

// Registers at $6xxx
void COOLBOY_Init(CartInfo* info) {
	CommonInit(info, 0);
}

// Registers at $5xxx
void MINDKIDS_Init(CartInfo* info) {
	CommonInit(info, 1);
}

// For NES 2.0 loader
void AA6023_Init(CartInfo* info) {
	CommonInit(info, info->submapper);
}
