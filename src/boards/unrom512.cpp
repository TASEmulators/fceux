/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2014 CaitSith2, 2022 Cluster
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

/*
 * Roms still using NES 1.0 format should be loaded as 8K CHR RAM.
 * Roms defined under NES 2.0 should use the VRAM size field, defining 7, 8 or 9, based on how much VRAM should be present.
 * UNIF doesn't have this problem, because unique board names can define this information.
 * The UNIF names are UNROM-512-8K, UNROM-512-16K and UNROM-512-32K
 *
 * The battery flag in the NES header enables flash,  Mirrror mode 2 Enables MI_0 and MI_1 mode.
 * Known games to use this board are: 
 *    Battle Kid 2: Mountain of Torment (512K PRG, 8K CHR RAM, Horizontal Mirroring, Flash disabled)
 *    Study Hall (128K PRG (in 512K flash chip), 8K CHR RAM, Horizontal Mirroring, Flash enabled)
 *    Nix: The Paradox Relic (512 PRG, 8K CHR RAM, Vertical Mirroring, Flash enabled)
 * Although Xmas 2013 uses a different board, where LEDs can be controlled (with writes to the $8000-BFFF space),
 * it otherwise functions identically.
*/

#include "mapinc.h"
#include "../ines.h"

const int ROM_CHIP = 0x00;
const int CFI_CHIP = 0x10;
const int FLASH_CHIP = 0x11;

const int FLASH_SECTOR_SIZE = 4 * 1024;

static uint8 flash_save, flash_state, flash_id_mode, latche, bus_conflict;
static uint16 latcha;
static uint8 *flash_data;
static uint16 flash_buffer_a[10];
static uint8 flash_buffer_v[10];
static uint8 flash_id[2];

static void UNROM512_Sync() {
	int chip;
	if (flash_save)
		chip = !flash_id_mode ? FLASH_CHIP : CFI_CHIP;
	else
		chip = ROM_CHIP;
	setprg16r(chip, 0x8000, latche);
	setprg16r(chip, 0xc000, ~0);
	setchr8(latche >> 5);
	setmirror(MI_0 + ((latche >> 7) & 1));
}

static void StateRestore(int version) {
	UNROM512_Sync();
}

static DECLFW(UNROM512FlashWrite)
{
	if (flash_state < sizeof(flash_buffer_a) / sizeof(flash_buffer_a[0])) {
		flash_buffer_a[flash_state] = (A & 0x3FFF) | ((latche & 1) << 14);
		flash_buffer_v[flash_state] = V;
		flash_state++;

		// enter flash ID mode
		if ((flash_state == 2) &&
			(flash_buffer_a[0] == 0x5555) && (flash_buffer_v[0] == 0xAA) &&
			(flash_buffer_a[1] == 0x2AAA) && (flash_buffer_v[1] == 0x55) &&
			(flash_buffer_a[1] == 0x5555) && (flash_buffer_v[1] == 0x90)) {
			flash_id_mode = 0;
			flash_state = 0;
		}

		// erase sector
		if ((flash_state == 6) &&
			(flash_buffer_a[0] == 0x5555) && (flash_buffer_v[0] == 0xAA) &&
			(flash_buffer_a[1] == 0x2AAA) && (flash_buffer_v[1] == 0x55) &&
			(flash_buffer_a[2] == 0x5555) && (flash_buffer_v[2] == 0x80) &&
			(flash_buffer_a[3] == 0x5555) && (flash_buffer_v[3] == 0xAA) &&
			(flash_buffer_a[4] == 0x2AAA) && (flash_buffer_v[4] == 0x55) &&
			(flash_buffer_v[5] == 0x30)) {
			int offset = &Page[A >> 11][A] - flash_data;
			int sector = offset / FLASH_SECTOR_SIZE;
			for (int i = sector * FLASH_SECTOR_SIZE; i < (sector + 1) * FLASH_SECTOR_SIZE; i++)
				flash_data[i % PRGsize[ROM_CHIP]] = 0xFF;
			FCEU_printf("Flash sector #%d is erased (0x%08x - 0x%08x).\n", sector, offset, offset + FLASH_SECTOR_SIZE);
		}

		// erase chip
		if ((flash_state == 6) &&
			(flash_buffer_a[0] == 0x5555) && (flash_buffer_v[0] == 0xAA) &&
			(flash_buffer_a[1] == 0x2AAA) && (flash_buffer_v[1] == 0x55) &&
			(flash_buffer_a[2] == 0x5555) && (flash_buffer_v[2] == 0x80) &&
			(flash_buffer_a[3] == 0x5555) && (flash_buffer_v[3] == 0xAA) &&
			(flash_buffer_a[4] == 0x2AAA) && (flash_buffer_v[4] == 0x55) &&
			(flash_buffer_a[4] == 0x5555) && (flash_buffer_v[4] == 0x10)) {
			memset(flash_data, 0xFF, PRGsize[ROM_CHIP]);
			FCEU_printf("Flash chip erased.\n");
			flash_state = 0;
		}

		// write byte
		if ((flash_state == 4) &&
			(flash_buffer_a[0] == 0x5555) && (flash_buffer_v[0] == 0xAA) &&
			(flash_buffer_a[1] == 0x2AAA) && (flash_buffer_v[1] == 0x55) &&
			(flash_buffer_a[2] == 0x5555) && (flash_buffer_v[2] == 0xA0)) {
			int offset = &Page[A >> 11][A] - flash_data;
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
		flash_id_mode = 0;
	}

	UNROM512_Sync();
}

static DECLFW(UNROM512HLatchWrite)
{
	if (bus_conflict)
		latche = V & CartBR(A);
	else
		latche = V;
	latcha = A;
	UNROM512_Sync();
}

static void UNROM512LatchPower(void) {
	latche = 0;
	UNROM512_Sync();
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	if(!flash_save)
		SetWriteHandler(0x8000, 0xFFFF, UNROM512HLatchWrite);
	else
	{
		SetWriteHandler(0x8000,0xBFFF,UNROM512FlashWrite);
		SetWriteHandler(0xC000,0xFFFF,UNROM512HLatchWrite);
	}
}

static void UNROM512LatchClose(void) {
	if(flash_data)
		FCEU_gfree(flash_data);
	flash_data = NULL;
}

static void UNROM512_FlashReset(void)
{
	if (flash_data)
	{
		size_t flash_size = PRGsize[ROM_CHIP];
		// Copy ROM to flash data
		for (size_t i = 0; i < flash_size; i++) {
			flash_data[i] = PRGptr[ROM_CHIP][i];
		}
	}
}

void UNROM512_Init(CartInfo *info) {
	info->Power = UNROM512LatchPower;
	info->Close = UNROM512LatchClose;
	GameStateRestore = StateRestore;

	flash_state = 0;
	flash_id_mode = 0;
	flash_save = info->battery;
	bus_conflict = !info->battery; // Is it required by any game?

	int mirror = (head.ROM_type & 1) | ((head.ROM_type & 8) >> 2);
	switch (mirror)
	{
	case 0: // hard horizontal, internal
		SetupCartMirroring(MI_H, 1, NULL);
		break;
	case 1: // hard vertical, internal
		SetupCartMirroring(MI_V, 1, NULL);
		break;
	case 2: // switchable 1-screen, internal (flags: 4-screen + horizontal)
		SetupCartMirroring(MI_0, 0, NULL);
		break;
	case 3: // hard four screen, last 8k of 32k RAM (flags: 4-screen + vertical)
		SetupCartMirroring(   4, 1, VROM + (info->vram_size - 8192));
		break;
	}

	if(flash_save)
	{
		// Allocate memory for flash
		size_t flash_size = PRGsize[ROM_CHIP];
		flash_data = (uint8*)FCEU_gmalloc(flash_size);
		// Copy ROM to flash data
		for (size_t i = 0; i < flash_size; i++) {
			flash_data[i] = PRGptr[ROM_CHIP][i];
		}
		SetupCartPRGMapping(FLASH_CHIP, flash_data, flash_size, 1);
		info->addSaveGameBuf( flash_data, flash_size, UNROM512_FlashReset );

		flash_id[0] = 0xBF;
		flash_id[1] = 0xB5 + (ROM_size >> 4);
		SetupCartPRGMapping(CFI_CHIP, flash_id, sizeof(flash_id), 0);

		AddExState(flash_data, flash_size, 0, "FLSH");
		AddExState(&flash_state, sizeof(flash_state), 0, "FLST");
		AddExState(&flash_id_mode, sizeof(flash_id_mode), 0, "FLMD");
		AddExState(flash_buffer_a, sizeof(flash_buffer_a), 0, "FLBA");
		AddExState(flash_buffer_v, sizeof(flash_buffer_v), 0, "FLBV");
	}
	AddExState(&latcha, sizeof(latcha), 0, "LATA");
	AddExState(&latche, sizeof(latche), 0, "LATC");
	AddExState(&bus_conflict, sizeof(bus_conflict), 0, "BUSC");
}
