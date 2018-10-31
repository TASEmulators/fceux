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
// 512k PRG-ROM in 32k pages (flashable if battery backed is specified)
// 32k CHR-RAM used as:
//     2 x 8k pattern pages
//     2 x 8k nametable pages
//
// Notes:
// - CHR-RAM for nametables maps to $3000-3FFF as well, but FCEUX internally mirrors to 4k?

#include "mapinc.h"
#include "../ines.h"

static uint8 reg;
static uint8 *CHRRAM = NULL;
const uint32 CHRRAMSIZE = 1024 * 32;

static bool flash = false;
static uint8 flash_mode;
static uint8 flash_sequence;
static uint8 flash_id;
static uint8 *FLASHROM = NULL;
const uint32 FLASHROMSIZE = 1024 * 512;


static SFORMAT StateRegs[] =
{
	{ &reg, 1, "REG" },
	{ 0 }
};

static SFORMAT FlashRegs[] =
{
	{ &flash_mode, 1, "FMOD" },
	{ &flash_sequence, 1, "FSEQ" },
	{ &flash_id, 1, "FMID" },
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

	uint32 prg_chip = flash ? 0x10 : 0;
	setprg32r(prg_chip,0x8000,prg);
}

static DECLFW(M111Write) {
	if ((A >= 0x5000 && A <= 0x5FFF) || (A >= 0x7000 && A <= 0x7FFF))
	{
		reg = V;
		Sync();
	}
}

static DECLFR(M111FlashID)
{
	// Software ID mode is undefined by the datasheet for all but the lowest 2 addressable bytes,
	// but some tests of the chip currently being used found it repeats in 512-byte patterns.
	// http://forums.nesdev.com/viewtopic.php?p=178728#p178728

	uint32 aid = A & 0x1FF;
	switch (aid)
	{
	case 0:  return 0xBF;
	case 1:  return 0xB7;
	default: return 0xFF;
	}
}

void M111FlashIDEnter()
{
	if (flash_id) return;
	flash_id = 1;
	SetReadHandler(0x8000,0xFFFF,M111FlashID);
}

void M111FlashIDExit()
{
	if (!flash_id) return;
	flash_id = 0;
	SetReadHandler(0x8000,0xFFFF,CartBR);
}

static DECLFW(M111Flash) {
	if (A < 0x8000 || A > 0xFFFF) return;

	uint32 flash_addr = ((reg & 0x0F) << 15) | (A & 0x7FFF);
	uint32 command_addr = flash_addr & 0x7FFF;

	enum
	{
		FLASH_MODE_READY = 0,
		FLASH_MODE_COMMAND,
		FLASH_MODE_BYTE_WRITE,
		FLASH_MODE_ERASE,
	};

	switch (flash_mode)
	{
	default:
	case FLASH_MODE_READY:
		if (command_addr == 0x5555 && V == 0xAA)
		{
			flash_mode = FLASH_MODE_COMMAND;
			flash_sequence = 0;
		}
		else if (V == 0xF0)
		{
			M111FlashIDExit();
		}
		break;
	case FLASH_MODE_COMMAND:
		if (flash_sequence == 0)
		{
			if (command_addr == 0x2AAA && V == 0x55)
				flash_sequence = 1;
			else
				flash_mode = FLASH_MODE_READY;
		}
		else if (flash_sequence == 1)
		{
			if (command_addr == 0x5555)
			{
				flash_sequence = 0;
				switch (V)
				{
				default:   flash_mode = FLASH_MODE_READY; break;
				case 0xA0: flash_mode = FLASH_MODE_BYTE_WRITE; break;
				case 0x80: flash_mode = FLASH_MODE_ERASE; break;
				case 0x90: M111FlashIDEnter(); flash_mode = FLASH_MODE_READY; break;
				case 0xF0: M111FlashIDExit(); flash_mode = FLASH_MODE_READY; break;
				}
			}
			else
				flash_mode = FLASH_MODE_READY;
		}
		else
			flash_mode = FLASH_MODE_READY; // should be unreachable
		break;
	case FLASH_MODE_BYTE_WRITE:
		FLASHROM[flash_addr] &= V;
		flash_mode = FLASH_MODE_READY;
		break;
	case FLASH_MODE_ERASE:
		if (flash_sequence == 0)
		{
			if (command_addr == 0x5555 && V == 0xAA)
				flash_sequence = 1;
			else
				flash_mode = FLASH_MODE_READY;
		}
		else if (flash_sequence == 1)
		{
			if (command_addr == 0x2AAA && V == 0x55)
				flash_sequence = 2;
			else
				flash_mode = FLASH_MODE_READY;
		}
		else if (flash_sequence == 2)
		{
			if (command_addr == 0x5555 && V == 0x10) // erase chip
			{
				memset(FLASHROM, 0xFF, FLASHROMSIZE);
			}
			else if (V == 0x30) // erase 4k sector
			{
				uint32 sector = flash_addr & 0x7F000;
				memset(FLASHROM + sector, 0xFF, 1024 * 4);
			}
			flash_mode = FLASH_MODE_READY;
		}
		else
			flash_mode = FLASH_MODE_READY; // should be unreachable
		break;
	}
}

static void M111Power(void) {
	reg = 0xFF;
	Sync();
	SetReadHandler(0x8000, 0xffff, CartBR);
	SetWriteHandler(0x5000, 0x5fff, M111Write);
	SetWriteHandler(0x7000, 0x7fff, M111Write);

	if (flash)
	{
		flash_mode = 0;
		flash_sequence = 0;
		flash_id = false;
		SetWriteHandler(0x8000, 0xFFFF, M111Flash);
	}
}

static void M111Close(void) {
	if (CHRRAM)
		FCEU_gfree(CHRRAM);
	CHRRAM = NULL;

	if (FLASHROM)
		FCEU_gfree(FLASHROM);
	FLASHROM = NULL;
}

static void StateRestore(int version) {
	Sync();
}

void Mapper111_Init(CartInfo *info) {
	info->Power = M111Power;
	info->Close = M111Close;

	CHRRAM = (uint8*)FCEU_gmalloc(CHRRAMSIZE);
	SetupCartCHRMapping(0x10, CHRRAM, CHRRAMSIZE, 1);

	GameStateRestore = StateRestore;
	AddExState(&StateRegs, ~0, 0, 0);
	AddExState(CHRRAM, CHRRAMSIZE, 0, "CRAM");

	flash = (info->battery != 0);
	if (flash)
	{
		FLASHROM = (uint8*)FCEU_gmalloc(FLASHROMSIZE);
		info->SaveGame[0] = FLASHROM;
		info->SaveGameLen[0] = FLASHROMSIZE;
		AddExState(FLASHROM, FLASHROMSIZE, 0, "FROM");
		AddExState(&FlashRegs, ~0, 0, 0);

		// copy PRG ROM into FLASHROM, use it instead of PRG ROM
		const uint32 PRGSIZE = ROM_size * 16 * 1024;
		for (uint32 w=0, r=0; w<FLASHROMSIZE; ++w)
		{
			FLASHROM[w] = ROM[r];
			++r;
			if (r >= PRGSIZE) r = 0;
		}
		SetupCartPRGMapping(0x10, FLASHROM, FLASHROMSIZE, 0);
	}
}
