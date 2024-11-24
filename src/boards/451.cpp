/* FCE Ultra - NES/Famicom Emulator
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

/* NES 2.0 Mapper 451 is used for the homebrew game Haratyler HP/MP. It is
 * basically a homebrew TLROM-like circuit board that implements the MMC3
 * register's in an unusual fashion, and saves the high score to flash ROM */

#include "mapinc.h"
#include "mmc3.h"
#include "../ines.h"

const int ROM_CHIP = 0x00;
const int CFI_CHIP = 0x10;
const int FLASH_CHIP = 0x11;

const int FLASH_SECTOR_SIZE = 64 * 1024;
const int magic_addr1 = 0x0555;
const int magic_addr2 = 0x02AA;

static uint8 flash_state, flash_id_mode;
static uint8 *flash_data;
static uint16 flash_buffer_a[10];
static uint8 flash_buffer_v[10];
static uint8 flash_id[2];

static DECLFW(M451FlashWrite)
{
	if (flash_state < sizeof(flash_buffer_a) / sizeof(flash_buffer_a[0])) {
		flash_buffer_a[flash_state] = (A & 0xFFF);
		flash_buffer_v[flash_state] = V;
		flash_state++;

		// enter flash ID mode
		if ((flash_state == 2) &&
			(flash_buffer_a[0] == magic_addr1) && (flash_buffer_v[0] == 0xAA) &&
			(flash_buffer_a[1] == magic_addr2) && (flash_buffer_v[1] == 0x55) &&
			(flash_buffer_a[1] == magic_addr1) && (flash_buffer_v[1] == 0x90)) {
			flash_id_mode = 0;
			flash_state = 0;
		}

		// erase sector
		if ((flash_state == 6) &&
			(flash_buffer_a[0] == magic_addr1) && (flash_buffer_v[0] == 0xAA) &&
			(flash_buffer_a[1] == magic_addr2) && (flash_buffer_v[1] == 0x55) &&
			(flash_buffer_a[2] == magic_addr1) && (flash_buffer_v[2] == 0x80) &&
			(flash_buffer_a[3] == magic_addr1) && (flash_buffer_v[3] == 0xAA) &&
			(flash_buffer_a[4] == magic_addr2) && (flash_buffer_v[4] == 0x55) &&
			(flash_buffer_v[5] == 0x30)) {
			int offset = &Page[A >> 11][A] - flash_data;
			int sector = offset / FLASH_SECTOR_SIZE;
			for (int i = sector * FLASH_SECTOR_SIZE; i < (sector + 1) * FLASH_SECTOR_SIZE; i++)
				flash_data[i % PRGsize[ROM_CHIP]] = 0xFF;
			FCEU_printf("Flash sector #%d is erased (0x%08x - 0x%08x).\n", sector, offset, offset + FLASH_SECTOR_SIZE);
		}

		// erase chip
		if ((flash_state == 6) &&
			(flash_buffer_a[0] == magic_addr1) && (flash_buffer_v[0] == 0xAA) &&
			(flash_buffer_a[1] == magic_addr2) && (flash_buffer_v[1] == 0x55) &&
			(flash_buffer_a[2] == magic_addr1) && (flash_buffer_v[2] == 0x80) &&
			(flash_buffer_a[3] == magic_addr1) && (flash_buffer_v[3] == 0xAA) &&
			(flash_buffer_a[4] == magic_addr2) && (flash_buffer_v[4] == 0x55) &&
			(flash_buffer_a[4] == magic_addr1) && (flash_buffer_v[4] == 0x10)) {
			memset(flash_data, 0xFF, PRGsize[ROM_CHIP]);
			FCEU_printf("Flash chip erased.\n");
			flash_state = 0;
		}

		// write byte
		if ((flash_state == 4) &&
			(flash_buffer_a[0] == magic_addr1) && (flash_buffer_v[0] == 0xAA) &&
			(flash_buffer_a[1] == magic_addr2) && (flash_buffer_v[1] == 0x55) &&
			(flash_buffer_a[2] == magic_addr1) && (flash_buffer_v[2] == 0xA0)) {
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
	if (((A & 0x00FF) != (magic_addr1 & 0x00FF)) && ((A & 0x00FF) != (magic_addr2 & 0x00FF))) {
		flash_state = 0;
	}

	// reset
	if (V == 0xF0) {
		flash_state = 0;
		flash_id_mode = 0;
	}

	FixMMC3PRG(MMC3_cmd);
}

static void M451FixPRG(uint32 A, uint8 V) {
	setprg8r(FLASH_CHIP, 0x8000, 0);
	setprg8r(FLASH_CHIP, 0xA000, 0x10 | ((EXPREGS[0] << 2) & 0x08) | (EXPREGS[0] & 0x01));
	setprg8r(FLASH_CHIP, 0xC000, 0x20 | ((EXPREGS[0] << 2) & 0x08) | (EXPREGS[0] & 0x01));
	setprg8r(FLASH_CHIP, 0xE000, 0x30);
}

static void M451FixCHR(uint32 A, uint8 V) {
	setchr8(EXPREGS[0] & 0x01);
}

static DECLFR(M451Read) {
	if (flash_state == 0x90) {
		return flash_id[A & 1];
	}
	return CartBR(A);
}

static DECLFW(M451Write) {
	M451FlashWrite(A, V);
	switch (A & 0xE000) {
    case 0xA000:
        MMC3_CMDWrite(0xA000, A & 0x01);
        break;
    case 0xC000:
        A &= 0xFF;
	    MMC3_IRQWrite(0xC000, A - 1);
	    MMC3_IRQWrite(0xC001, 0);
	    MMC3_IRQWrite(0xE000 + ((A == 0xFF) ? 0x00 : 0x01), 0x00);
        break;
    case 0xE000:
		EXPREGS[0] = A & 0x03;
		FixMMC3PRG(MMC3_cmd);
		FixMMC3CHR(MMC3_cmd);
        break;
	}
}

static void M451Power(void) {
	GenMMC3Power();
	SetReadHandler(0x8000, 0xFFFF, M451Read);
	SetWriteHandler(0x8000, 0xFFFF, M451Write);
}

static void StateRestore(int version) {
	FixMMC3PRG(MMC3_cmd);
    FixMMC3CHR(MMC3_cmd);
}

static void M451Close(void) {
	if(flash_data)
		FCEU_gfree(flash_data);
	flash_data = NULL;
}

static void M451FlashReset(void)
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

void Mapper451_Init(CartInfo *info) {
    GenMMC3_Init(info, 512, 16, 0, 0);
    pwrap = M451FixPRG;
    cwrap = M451FixCHR;

	info->Power = M451Power;
	info->Close = M451Close;
	GameStateRestore = StateRestore;

	flash_state = 0;
	flash_id_mode = 0;
	info->battery = 1;

	// Allocate memory for flash
	size_t flash_size = PRGsize[ROM_CHIP];
	flash_data = (uint8*)FCEU_gmalloc(flash_size);
	// Copy ROM to flash data
	for (size_t i = 0; i < flash_size; i++) {
		flash_data[i] = PRGptr[ROM_CHIP][i];
	}
	SetupCartPRGMapping(FLASH_CHIP, flash_data, flash_size, 1);
	info->addSaveGameBuf( flash_data, flash_size, M451FlashReset );

	flash_id[0] = 0x37;
	flash_id[1] = 0x86;
	SetupCartPRGMapping(CFI_CHIP, flash_id, sizeof(flash_id), 0);

	AddExState(flash_data, flash_size, 0, "FLSH");
	AddExState(&flash_state, sizeof(flash_state), 0, "FLST");
	AddExState(&flash_id_mode, sizeof(flash_id_mode), 0, "FLMD");
	AddExState(flash_buffer_a, sizeof(flash_buffer_a), 0, "FLBA");
	AddExState(flash_buffer_v, sizeof(flash_buffer_v), 0, "FLBV");
}
