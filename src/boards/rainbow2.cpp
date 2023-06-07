/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2021-2022 Broke Studio
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

 // mapper 682 - Rainbow2 board v1.0 revA and v1.1 by Broke Studio
 //
 // documentation available here: https://github.com/BrokeStudio/rainbow-lib

#include "mapinc.h"
#include "../ines.h"
#include "rainbow_esp.h"

#undef RAINBOW_DEBUG
 //define RAINBOW_DEBUG

#ifdef RAINBOW_DEBUG
#define UDBG(...) FCEU_printf(__VA_ARGS__)
#else
#define UDBG(...)
#endif

#define MAPPER_VERSION		0b01000000

#define MIRR_VERTICAL		0b00 // VRAM
#define MIRR_HORIZONTAL		0b01 // VRAM
#define MIRR_ONE_SCREEN		0b10 // VRAM [+ CHR-RAM]
#define MIRR_FOUR_SCREEN	0b11 // CHR-RAM

#define PRG_ROM_MODE_0		0b000	// 32K
#define PRG_ROM_MODE_1		0b001	// 16K + 16K
#define PRG_ROM_MODE_2		0b010	// 16K + 8K + 8K
#define PRG_ROM_MODE_3		0b011	// 8K + 8K + 8K + 8K
#define PRG_ROM_MODE_4		0b100	// 4K + 4K + 4K + 4K + 4K + 4K + 4K + 4K

#define PRG_RAM_MODE_0		0b0	// 8K
#define PRG_RAM_MODE_1		0b1	// 4K

#define CHR_CHIP_ROM		0b00  // CHR-ROM
#define CHR_CHIP_RAM		0b01  // CHR-RAM
#define CHR_CHIP_FPGA_RAM	0b10  // FPGA-RAM

#define CHR_MODE_0			0b000 // 8K mode
#define CHR_MODE_1			0b001 // 4K mode
#define CHR_MODE_2			0b010 // 2K mode
#define CHR_MODE_3			0b011 // 1K mode
#define CHR_MODE_4			0b100 // 512B mode

#define CHIP_TYPE_PRG		0
#define CHIP_TYPE_CHR		1

#define NT_CIRAM			0b00
#define NT_CHR_RAM			0b01
#define NT_FPGA_RAM			0b10
#define NT_CHR_ROM			0b11

#define SpriteON			(PPU[1] & 0x10)	//Show Sprite
#define ScreenON			(PPU[1] & 0x08)	//Show screen
#define PPUON				(PPU[1] & 0x18)	//PPU should operate
#define Sprite16			(PPU[0] & 0x20)	//Sprites 8x16/8x8

static void Rainbow2Reset(void);

static uint8 prg_rom_mode, prg_ram_mode, bootrom;
static uint16 prg[11]; // 0: $5000, 1: $6000, 2: $7000, 3: $8000, 4: $9000, etc

static uint8 chr_chip, chr_spr_ext_mode, chr_mode;
static uint16 chr[16];

static uint8 NT_bank[4];
static uint8 SPR_bank_offset;
static uint16 SPR_bank[64];

static uint8 audio_output;
static uint8 rx_address, tx_address, rx_index;

static uint32 PRGSIZE = 0;
static uint32 CHRSIZE = 0;

static uint8 *WRAM = NULL;
static int WRAMSIZE = 0; // max 512 KiB

static uint8 *FPGA_RAM = NULL;
const uint32 FPGA_RAMSIZE = 8 * 1024;

static uint8 *DUMMY_CHRRAM = NULL;
const uint32 DUMMY_CHRRAMSIZE = 4 * 1024;

static uint8 *DUMMY_CHRROM = NULL;
const uint32 DUMMY_CHRROMSIZE = 8192 * 1024; // max 8192 MiB

static uint8 *CHRRAM = NULL;
static int CHRRAMSIZE = 0; // max 512 KiB

extern uint8 *ExtraNTARAM;

static uint8 RNBWbattery = 0;

// Scanline IRQ
static uint8 S_IRQcontrol, S_IRQlatch, S_IRQoffset; // matches hardware register
// additional flags to emulate hardware correctly
static float S_IRQdotcount;
static uint8 S_IRQready, S_IRQlastSLtriggered;

// CPU Cycle IRQ
static bool C_IRQe, C_IRQr, C_IRQp;
static int32 C_IRQLatch, C_IRQCount;

// ESP message IRQ
static uint8 ESP_IRQp;

static uint8 flash_mode[2];
static uint8 flash_sequence[2];
static uint8 flash_id[2];

static uint8 *PRG_FLASHROM = NULL;
const uint32 PRG_FLASHROMSIZE = 8192 * 1024; // max 8MiB

static uint8 *CHR_FLASHROM = NULL;
const uint32 CHR_FLASHROMSIZE = 8192 * 1024; // max 8MiB

const bool S29AL008_TOP = false;
const bool S29AL016_TOP = true;
const bool S29JL032_TOP = false;
const bool S29GL064S_TOP = false;

static SFORMAT FlashRegs[] =
{
	{ &flash_mode, 2, "FMOD" },
	{ &flash_sequence, 2, "FSEQ" },
	{ &flash_id, 2, "FMID" },
	{ 0 }
};

static SFORMAT Rainbow2StateRegs[] =
{
	{ prg, 11, "PRG" },
	{ chr, 16, "CHR" },

	{ &S_IRQcontrol, 1, "SICO" },
	{ &S_IRQlatch, 1, "SILA" },
	{ &S_IRQoffset, 1, "SIOF" },
	{ &S_IRQdotcount, 4, "SIDC" },
	{ &S_IRQready, 1, "SIRE" },
	{ &S_IRQlastSLtriggered, 1, "SITR" },

	{ &C_IRQe, 1, "CPUA" },
	{ &C_IRQp, 1, "CPUP" },
	{ &C_IRQr, 1, "CPUR" },
	{ &C_IRQLatch, 4, "CPUL" },
	{ &C_IRQCount, 4, "CPUC" },

	{ &ESP_IRQp, 1, "ESPP" },

	{ 0 }
};

static void(*sfun[3]) (void);
static uint8 vpsg1[8];
static uint8 vpsg2[4];
static int32 cvbc[3];
static int32 vcount[3];
static int32 dcount[2];

static SFORMAT SStateRegs[] =
{
	{ vpsg1, 8, "PSG1" },
	{ vpsg2, 4, "PSG2" },
	{ 0 }
};

static EspFirmware *esp = NULL;
static bool esp_enable;
static bool esp_irq_enable;
static bool has_esp_message_received;
static bool esp_message_sent;

static void esp_check_new_message() {
	// get new message if needed
	if (esp_enable && esp->getDataReadyIO() && has_esp_message_received == false)
	{
		uint8 message_length = esp->tx();
		FPGA_RAM[0x1800 + (rx_address << 8)] = message_length;
		for (uint8 i = 0; i < message_length; i++)
		{
			FPGA_RAM[0x1800 + (rx_address << 8) + 1 + i] = esp->tx();
		}
		rx_index = 0;
		has_esp_message_received = true;
	}
}

static bool esp_message_received() {
	esp_check_new_message();
	return has_esp_message_received;
}

static void clear_esp_message_received() {
	has_esp_message_received = false;
}

static void IRQEnd() {
	if (!C_IRQp && !(S_IRQcontrol & 0x80) && !ESP_IRQp)
	{
		X6502_IRQEnd(FCEU_IQEXT);
	}
}

static void Rainbow2IRQ(int a) {

	// Scanline IRQ
	int sl = newppu_get_scanline();
	int dot = newppu_get_dot();
	int ppuon = (PPU[1] & 0x18);

	if (!ppuon || sl >= 241)
	{
		// whenever rendering is off for any reason (vblank or forced disable)
		// the irq counter resets, as well as the inframe flag (easily verifiable from software)
		S_IRQcontrol &= ~0x40; // in-frame flag cleared
		S_IRQcontrol &= ~0x80; // pending IRQ flag cleared
		S_IRQready = 0;
		S_IRQlastSLtriggered = 255;
		IRQEnd();
	}
	else
	{
		if (!(S_IRQcontrol & 0x40))
		{
			S_IRQcontrol |= 0x40;
			S_IRQcontrol &= ~0x80;
			IRQEnd();
		}

		// this is kind of hacky but result is pretty close to hardware
		if ((sl == S_IRQlatch) & !S_IRQready & !(S_IRQcontrol & 0x80) & (sl != S_IRQlastSLtriggered))
		{
			S_IRQready = 1;
			S_IRQdotcount = (float)dot;
			if (PAL) S_IRQdotcount -= a * 3.2;
			else S_IRQdotcount -= (float)a * 3;
		}

		if ((S_IRQready == 1) & !(S_IRQcontrol & 0x80))
		{
			if (PAL) S_IRQdotcount += (float)a * 3.2;
			else S_IRQdotcount += (float)a * 3;

			int dotTarget = (S_IRQoffset * 2);
			if (PAL) dotTarget += 20;
			else dotTarget += 24;

			if (((int)S_IRQdotcount) >= dotTarget)
			{
				S_IRQready = 2;
				S_IRQlastSLtriggered = S_IRQlatch;
			}
		}

		if ((S_IRQcontrol & 0x01) && !(S_IRQcontrol & 0x80) && (S_IRQready == 2))
		{
			X6502_IRQBegin(FCEU_IQEXT);
			S_IRQcontrol |= 0x80;
			S_IRQready = 0;
		}
	}

	// Cycle Counter IRQ
	if (C_IRQe) {
		C_IRQCount -= a;
		if (C_IRQCount <= 0) {
			C_IRQe = C_IRQr;
			C_IRQp = true;
			C_IRQCount = C_IRQLatch;
			X6502_IRQBegin(FCEU_IQEXT);
		}
	}

	// ESP / new message IRQ
	if (esp_irq_enable)
	{
		if (esp_message_received())
		{
			X6502_IRQBegin(FCEU_IQEXT);
			ESP_IRQp = 1;
		}
		else
		{
			ESP_IRQp = 0;
		}
	}

	IRQEnd();
}

static void Sync(void) {
	static uint8 *address;
	//uint32 start;
	//uint32 offset;
	uint8 cart_chr_map;

	// $8000-$ffff
	uint8 t_prg_rom_mode = bootrom ? PRG_ROM_MODE_3 : prg_rom_mode;

	// 32K
	if (t_prg_rom_mode == PRG_ROM_MODE_0)
	{
		// PRG-ROM
		if (!(prg[3] & 0x8000))
			setprg32r(0x11, 0x8000, prg[3] & 0x7fff);
		// WRAM
		if (prg[3] & 0x8000)
			setprg32r(0x10, 0x8000, 0);
	}

	// 16K + 16K
	if (t_prg_rom_mode == PRG_ROM_MODE_1)
	{
		// PRG-ROM
		if (!(prg[3] & 0x8000))
			setprg16r(0x11, 0x8000, prg[3] & 0x7fff);
		// WRAM
		if (prg[3] & 0x8000)
			setprg16r(0x10, 0x8000, prg[3] & 0x01);

		// PRG-ROM
		if (!(prg[7] & 0x8000))
			setprg16r(0x11, 0xC000, prg[7] & 0x7fff);
		// WRAM
		if (prg[7] & 0x8000)
			setprg16r(0x10, 0xC000, prg[7] & 0x01);
	}

	// 16K + 8K + 8K
	if (t_prg_rom_mode == PRG_ROM_MODE_2)
	{
		// PRG-ROM
		if (!(prg[3] & 0x8000))
			setprg16r(0x11, 0x8000, prg[3] & 0x7fff);
		// WRAM
		if (prg[3] & 0x8000)
			setprg16r(0x10, 0x8000, prg[3] & 0x01);

		// PRG-ROM
		if (!(prg[7] & 0x8000))
			setprg8r(0x11, 0xc000, prg[7] & 0x7fff);
		// WRAM
		if (prg[7] & 0x8000)
			setprg8r(0x10, 0xc000, prg[7] & 0x03);

		// PRG-ROM
		if (!(prg[9] & 0x8000))
			setprg8r(0x11, 0xe000, prg[9] & 0x7fff);
		// WRAM
		if (prg[9] & 0x8000)
			setprg8r(0x10, 0xe000, prg[9] & 0x03);
	}

	// 8K + 8K + 8K + 8K
	if (t_prg_rom_mode == PRG_ROM_MODE_3)
	{
		for (uint8 i = 0; i < 4; i++)
		{
			// PRG-ROM
			if (i == 3 && bootrom)
				setprg8r(0x13, 0xe000, 1);
			else
				if (!(prg[3 + i * 2] & 0x8000))
					setprg8r(0x11, 0x8000 + 0x2000 * i, prg[3 + i * 2] & 0x7fff);
			// WRAM
			if (prg[3 + i * 2] & 0x8000)
				setprg8r(0x10, 0x8000 + 0x2000 * i, prg[3 + i * 2] & 0x03);
		}
	}

	// 4K + 4K + 4K + 4K + 4K + 4K + 4K + 4K
	if (prg_rom_mode == PRG_ROM_MODE_4)
	{
		for (uint8 i = 0; i < 8; i++)
		{
			// PRG-ROM
			if (!(prg[3 + i] & 0x8000))
				setprg4r(0x11, 0x8000 + 0x1000 * i, prg[3 + i] & 0x7fff);
			// WRAM
			if (prg[3 + i] & 0x8000)
				setprg4r(0x10, 0x8000 + 0x1000 * i, prg[3 + i] & 0x07);
		}
	}

	// $6000-$7fff

	// 8K
	if (prg_ram_mode == PRG_RAM_MODE_0)
	{
		// WRAM
		if (((prg[1] & 0xC000) >> 14) == 2)
			setprg8r(0x10, 0x6000, prg[1] & 0x3f);
		// FPGA-RAM
		if (((prg[1] & 0xC000) >> 14) == 3)
			setprg8r(0x12, 0x6000, 0);
		// PRG-ROM
		if (((prg[1] & 0x8000) >> 15) == 0)
			setprg8r(0x11, 0x6000, prg[1] & 0x7fff);
	}

	// 4K
	if (prg_ram_mode == PRG_RAM_MODE_1)
	{
		// WRAM
		if (((prg[1] & 0xC000) >> 14) == 2)
			setprg4r(0x10, 0x6000, prg[1] & 0x7f);
		// FPGA-RAM
		if (((prg[1] & 0xC000) >> 14) == 3)
			setprg4r(0x12, 0x6000, prg[1] & 0x01);
		// PRG-ROM
		if (((prg[1] & 0x8000) >> 15) == 0)
			setprg4r(0x11, 0x6000, prg[1] & 0x7fff);

		// WRAM
		if (((prg[2] & 0xC000) >> 14) == 2)
			setprg4r(0x10, 0x7000, prg[2] & 0x7f);
		// FPGA-RAM
		if (((prg[2] & 0xC000) >> 14) == 3)
			setprg4r(0x12, 0x7000, prg[2] & 0x01);
		// PRG-ROM
		if (((prg[2] & 0xC000) >> 15) == 0)
			setprg4r(0x11, 0x7000, prg[2] & 0x7fff);
	}

	// $5000-$5fff - 4K FPGA-RAM
	if (bootrom)
		setprg4r(0x13, 0x5000, 1);
	else
		setprg4r(0x12, 0x5000, prg[0] & 0x01);

	// $4800-$4fff
	setprg2r(0x12, 0x4800, 3);

	// CHR
	switch (chr_chip)
	{
		case CHR_CHIP_ROM:
			RNBWHackVROMPtr = CHR_FLASHROM;
			RNBWHackVROMMask = CHR_FLASHROMSIZE - 1;
			break;
		case CHR_CHIP_RAM:
			RNBWHackVROMPtr = CHRRAM;
			RNBWHackVROMMask = CHRRAMSIZE - 1;
			break;
		case CHR_CHIP_FPGA_RAM:
			RNBWHackVROMPtr = FPGA_RAM;
			RNBWHackVROMMask = FPGA_RAMSIZE - 1;
			break;
	}

	if(chr_chip == CHR_CHIP_FPGA_RAM)
	{
		setchr4r(0x12, 0x0000, 0);
		setchr4r(0x12, 0x1000, 0);
	}
	else
	{
		cart_chr_map = chr_chip + 0x10;
		switch (chr_mode)
		{
		case CHR_MODE_0:
			setchr8r(cart_chr_map, chr[0] & 0xffff);
			break;
		case CHR_MODE_1:
			setchr4r(cart_chr_map, 0x0000, chr[0] & 0xffff);
			setchr4r(cart_chr_map, 0x1000, chr[1] & 0xffff);
			break;
		case CHR_MODE_2:
			setchr2r(cart_chr_map, 0x0000, chr[0] & 0xffff);
			setchr2r(cart_chr_map, 0x0800, chr[1] & 0xffff);
			setchr2r(cart_chr_map, 0x1000, chr[2] & 0xffff);
			setchr2r(cart_chr_map, 0x1800, chr[3] & 0xffff);
			break;
		case CHR_MODE_3:
			for (uint8 i = 0; i < 8; i++) {
				setchr1r(cart_chr_map, i << 10, chr[i] & 0xffff);
			}
			break;
		case CHR_MODE_4:
			for (uint8 i = 0; i < 16; i++) {
				setchr512r(cart_chr_map, i << 9, chr[i] & 0xffff);
			}
			break;
		}
	}

	for (int i = 0; i < 4; i++)
	{
		uint8 cur_NT_bank = NT_bank[i];
		uint8 cur_NT_chip = RNBWHackNTcontrol[i] >> 6;
		//uint8 cur_NT_1K_dest = (RNBWHackNTcontrol[i] & 0x0C) >> 2;
		//uint8 cur_NT_ext_mode = RNBWHackNTcontrol[i] & 0x03;

		switch (cur_NT_chip)
		{
		case NT_CIRAM:
			vnapage[i] = NTARAM + 0x400 * (cur_NT_bank & 0x01);
			break;
		case NT_CHR_RAM:
			vnapage[i] = CHRRAM + 0x400 * (cur_NT_bank & 0x1f);
			break;
		case NT_FPGA_RAM:
			vnapage[i] = FPGA_RAM + 0x400 * (cur_NT_bank & 0x03);
			break;
		case NT_CHR_ROM:
			vnapage[i] = CHR_FLASHROM + 0x400 * cur_NT_bank;
			break;
		}
	}

}

static DECLFW(RNBW_ExpAudioWr) {
	if (A >= 0x41A0 && A <= 0x41A2)
	{
		vpsg1[A & 3] = V;
		if (sfun[0])
			sfun[0]();
	}
	else if (A >= 0x41A3 && A <= 0x41A5)
	{
		vpsg1[4 | ((A - 3) & 3)] = V;
		if (sfun[1])
			sfun[1]();
	}
	else if (A >= 0x41A6 && A <= 0x41A8)
	{
		vpsg2[(A - 6) & 3] = V;
		if (sfun[2])
			sfun[2]();
	}
}

static DECLFW(WRAM_0x6000Wr)
{
	// $6000-$7fff

	// 8K
	if (prg_ram_mode == PRG_RAM_MODE_0)
	{
		if (((prg[1] & 0xC000) >> 14) == 3)
			CartBW(A, V); // FPGA_RAM
		else if ((((prg[1] & 0xC000) >> 14) == 2) && WRAM)
			CartBW(A, V); // WRAM
		else if (((prg[1] & 0x8000) >> 15) == 0)
			CartBW(A, V); // PRG-ROM
	}

	// 4K
	if (prg_ram_mode == PRG_RAM_MODE_1)
	{
		if ((A >= 0x6000) & (A < 0x7000))
		{
			if (((prg[1] & 0xC000) >> 14) == 3)
				CartBW(A, V); // FPGA_RAM
			else if ((((prg[1] & 0xC000) >> 14) == 2) && WRAM)
				CartBW(A, V); // WRAM
			else  if (((prg[1] & 0x8000) >> 15) == 0)
				CartBW(A, V); // PRG-ROM
		}

		if ((A >= 0x7000) & (A < 0x8000))
		{
			if (((prg[2] & 0xC000) >> 14) == 3)
				CartBW(A, V); // FPGA_RAM
			else if ((((prg[2] & 0xC000) >> 14) == 2) && WRAM)
				CartBW(A, V); // WRAM
			else if (((prg[2] & 0x8000) >> 15) == 0)
				CartBW(A, V); // PRG-ROM
		}
	}
}

static DECLFR(WRAM_0x6000Rd)
{
	// $6000-$7fff

	// 8K
	if (prg_ram_mode == PRG_RAM_MODE_0)
	{
		if (((prg[1] & 0xC000) >> 14) == 3)
			return CartBR(A); // FPGA_RAM
		else if ((((prg[1] & 0xC000) >> 14) == 2) && WRAM)
			return CartBR(A); // WRAM
		else if (((prg[1] & 0x8000) >> 15) == 0)
			return CartBR(A); // PRG-ROM
	}

	// 4K
	if (prg_ram_mode == PRG_RAM_MODE_1)
	{
		if ((A >= 0x6000) & (A < 0x7000))
		{
			if (((prg[1] & 0xC000) >> 14) == 3)
				return CartBR(A); // FPGA_RAM
			else if ((((prg[1] & 0xC000) >> 14) == 2) && WRAM)
				return CartBR(A); // WRAM
			else if (((prg[1] & 0x8000) >> 15) == 0)
				return CartBR(A); // PRG-ROM
		}

		if ((A >= 0x7000) & (A < 0x8000))
		{
			if (((prg[2] & 0xC000) >> 14) == 3)
				return CartBR(A); // FPGA_RAM
			else if ((((prg[2] & 0xC000) >> 14) == 2) && WRAM)
				return CartBR(A); // WRAM
			else if (((prg[2] & 0x8000) >> 15) == 0)
				return CartBR(A); // PRG-ROM
		}
	}
	
	return X.DB;
}
static DECLFW(WRAM_0x5000Wr)
{
	CartBW(A, V);
}
static DECLFR(WRAM_0x5000Rd)
{
	return CartBR(A);
}
static DECLFW(FPGA_0x4800Wr)
{
	CartBW(A, V);
	//FPGA_RAM[(A & 0x7ff) + 0x1800] = V;
}
static DECLFR(FPGA_0x4800Rd)
{
	return CartBR(A);
	//return FPGA_RAM[(A & 0x7ff) + 0x1800];
}

static DECLFR(RNBW_0x4100Rd) {
	switch (A)
	{
	case 0x4100: return (prg_ram_mode << 6) | prg_rom_mode;
	case 0x4120: return (chr_chip << 6) | (chr_spr_ext_mode << 5) | chr_mode;
	case 0x4151: {
		S_IRQcontrol &= ~0x80;
		S_IRQready = 0;
		IRQEnd();
		return CartBR(A);
	}
	case 0x4160: return MAPPER_VERSION;

		// ESP - WiFi
	case 0x4170:
	{
		uint8 esp_enable_flag = esp_enable ? 0x01 : 0x00;
		uint8 irq_enable_flag = esp_irq_enable ? 0x02 : 0x00;
		UDBG("RAINBOW read flags %04x => %02xs\n", A, esp_enable_flag | irq_enable_flag);
		return esp_enable_flag | irq_enable_flag;
	}
	case 0x4171:
	{
		uint8 esp_message_received_flag = esp_message_received() ? 0x80 : 0;
		uint8 esp_rts_flag = esp->getDataReadyIO() ? 0x40 : 0x00;
		return esp_message_received_flag | esp_rts_flag;
	}
	case 0x4172: return esp_message_sent ? 0x80 : 0;
	case 0x4175:
	{
		uint8 retval = FPGA_RAM[0x1800 + (rx_address << 8) + rx_index];
		rx_index++;
		return retval;
	}
	default:
		return CartBR(A);
	}
}

static DECLFW(RNBW_0x4100Wr) {
	switch (A)
	{
		// Mapper configuration
	case 0x4100:
		prg_rom_mode = V & 0x07;
		prg_ram_mode = (V & 0x80) >> 7;
		Sync();
		break;
		// PRG banking
	case 0x4106:
	case 0x4107:
	case 0x4108:
	case 0x4109:
	case 0x410A:
	case 0x410B:
	case 0x410C:
	case 0x410D:
	case 0x410E:
	case 0x410F:
	{
		int bank = ( A & 0x0f ) - 5;
		prg[bank] = (prg[bank] & 0x00ff) | (V << 8);
		Sync();
		break;
	}
	case 0x4115: prg[0] = V & 0x01; Sync(); break;
	case 0x4116:
	case 0x4117:
	case 0x4118:
	case 0x4119:
	case 0x411A:
	case 0x411B:
	case 0x411C:
	case 0x411D:
	case 0x411E:
	case 0x411F:
	{
		int bank = ( A & 0x0f ) - 5;
		prg[bank] = (prg[bank] & 0xff00) | V;
		Sync();
		break;
	}
		// CHR banking / chip selector / sprite extended mode
	case 0x4120:
		chr_chip = (V & 0xC0) >> 6;
		chr_spr_ext_mode = (V & 0x20) >> 5;
		chr_mode = V & 0x07;
		Sync();
		break;
	case 0x4121:
		RNBWHackBGBankOffset = V & 0x1f;
		break;
		// Nametables bank
	case 0x4126: NT_bank[0] = V; Sync(); break;
	case 0x4127: NT_bank[1] = V; Sync(); break;
	case 0x4128: NT_bank[2] = V; Sync(); break;
	case 0x4129: NT_bank[3] = V; Sync(); break;
		// Nametables control
	case 0x412A: RNBWHackNTcontrol[0] = V; Sync(); break;
	case 0x412B: RNBWHackNTcontrol[1] = V; Sync(); break;
	case 0x412C: RNBWHackNTcontrol[2] = V; Sync(); break;
	case 0x412D: RNBWHackNTcontrol[3] = V; Sync(); break;
		// CHR banking
	case 0x4130:
	case 0x4131:
	case 0x4132:
	case 0x4133:
	case 0x4134:
	case 0x4135:
	case 0x4136:
	case 0x4137:
	case 0x4138:
	case 0x4139:
	case 0x413A:
	case 0x413B:
	case 0x413C:
	case 0x413D:
	case 0x413E:
	case 0x413F:
	{
		int bank = A & 0x0f;
		chr[bank] = (chr[bank] & 0x00ff) | (V << 8);
		Sync();
		break;
	}
	case 0x4140:
	case 0x4141:
	case 0x4142:
	case 0x4143:
	case 0x4144:
	case 0x4145:
	case 0x4146:
	case 0x4147:
	case 0x4148:
	case 0x4149:
	case 0x414A:
	case 0x414B:
	case 0x414C:
	case 0x414D:
	case 0x414E:
	case 0x414F:
	{
		int bank = A & 0x0f;
		chr[bank] = (chr[bank] & 0xff00) | V;
		Sync();
		break;
	}
		// Scanline IRQ
	case 0x4150: S_IRQlatch = V; break;
	case 0x4151: S_IRQcontrol |= 1; break;
	case 0x4152: S_IRQcontrol &= 0x7E; break;
	case 0x4153: S_IRQoffset = V > 169 ? 169 : V; break;
		// CPU Cycle IRQ
	case 0x4158: C_IRQLatch &= 0xFF00; C_IRQLatch |= V; C_IRQCount = C_IRQLatch; break;
	case 0x4159: C_IRQLatch &= 0x00FF; C_IRQLatch |= V << 8; C_IRQCount = C_IRQLatch; break;
	case 0x415A:
		C_IRQe = V & 0x01;
		C_IRQr = (V & 0x02) >> 1;
		if (C_IRQe)
			C_IRQCount = C_IRQLatch;
		break;
	case 0x415B:
		C_IRQp = false;
		IRQEnd();
		break;
		// ESP - WiFi
	case 0x4170:
		esp_enable = V & 0x01;
		esp_irq_enable = V & 0x02;
		break;
	case 0x4171:
		if (esp_enable) clear_esp_message_received();
		else FCEU_printf("RAINBOW warning: $4170.0 is not set\n");
		break;
	case 0x4172:
		if (esp_enable)
		{
			esp_message_sent = false;
			uint8 message_length = FPGA_RAM[0x1800 + (tx_address << 8)];
			esp->rx(message_length);
			for (uint8 i = 0; i < message_length; i++)
			{
				esp->rx(FPGA_RAM[0x1800 + (tx_address << 8) + 1 + i]);
			}
			esp_message_sent = true;
		}
		else FCEU_printf("RAINBOW warning: $4170.0 is not set\n");
		break;
	case 0x4173:
		rx_address = V & 0x07;
		break;
	case 0x4174:
		tx_address = V & 0x07;
		break;
	case 0x4175:
		rx_index = V;
		break;
	case 0x41FF:
		bootrom = V & 0x01;
		if (bootrom == 0) Rainbow2Reset(); // a bit hacky but does the job for testing
		Sync();
		break;
	case 0x4240:
		SPR_bank_offset = V;
		break;
	case 0x41A9:
		audio_output = V & 0x07;
		break;
	}

	// $4200-$423F Sprite 4K bank lower bits
	if ((A >= 0x4200) & (A <= 0x423f))
		SPR_bank[A & 0x3f] = V;
}

extern uint32 NTRefreshAddr;
uint8 FASTCALL Rainbow2PPURead(uint32 A) {
/*
	// if CHR-RAM, check if CHR-RAM exists, if not return data bus cache
	if (chr_chip == CHR_CHIP_RAM && CHRRAM == NULL)
	{
		if (PPU_hook) PPU_hook(A);
		return X.DB;
	}

	// if CHR-ROM, check if CHR-ROM exists, if not return data bus cache
	if (chr_chip == CHR_CHIP_ROM && CHR_FLASHROM == NULL)
	{
		if (PPU_hook) PPU_hook(A);
		return X.DB;
	}
*/
	if (A < 0x2000)
	{
		if (ppuphase == PPUPHASE_OBJ && ScreenON)
		{
			if (chr_spr_ext_mode)
				if(Sprite16)
					return RNBWHackVROMPtr[(SPR_bank[RNBWHackCurSprite] << 13) + (A)];
				else
					return RNBWHackVROMPtr[(SPR_bank[RNBWHackCurSprite] << 12) + (A)];
			else
			{
				return VPage[A >> 9][A]; // FIXME
			}
		}

		if (ppuphase == PPUPHASE_BG && ScreenON)
			return *FCEUPPU_GetCHR(A, NTRefreshAddr);

		// default
		return FFCEUX_PPURead_Default(A);
	}
	else
	{
		uint8 NT = (A >> 10) & 0x03;
		uint8 NT_1K_dest = (RNBWHackNTcontrol[NT] & 0x0C) >> 2;
		uint8 NT_ext_mode = RNBWHackNTcontrol[NT] & 0x03;
		if(( A & 0x3FF ) >= 0x3C0)
		{
			// Attributes
			if (NT_ext_mode & 0x01)
			{
				uint8 byte = FPGA_RAM[NT_1K_dest * 0x400 + (NTRefreshAddr & 0x3ff)];
				//get attribute part and paste it 4x across the byte
				byte >>= 6;
				byte *= 0x55;
				return byte;
			}
		}
		else
		{

		}
		return vnapage[(A >> 10) & 0x3][A & 0x3FF];
	}
}

uint8 Rainbow2FlashID(uint8 chip, uint32 A) {
	// Software ID mode is undefined by the datasheet for all but the lowest 2 addressable bytes,
	// but some tests of the chip currently being used found it repeats in 512-byte patterns.
	// http://forums.nesdev.com/viewtopic.php?p=178728#p178728

	uint32 flash_size;

	if (chip == CHIP_TYPE_PRG) flash_size = ROM_size * 16;
	else if (chip == CHIP_TYPE_CHR) flash_size = VROM_size * 8;

	uint32 aid = A & 0x1FF;
	switch (aid)
	{
	case 0x00:  return 0x01; // 0x01 = Cypress
	case 0x02:
	{
		switch (flash_size)
		{
		case 1024: return S29AL008_TOP ? 0xDA : 0x5B;	// S29AL008: 0xDA = top boot block | 0x5B = bottom boot block
		case 2048: return S29AL016_TOP ? 0xC4 : 0x49;	// S29AL016: 0xC4 = top boot block | 0x49 = bottom boot block
		case 4096: return 0x7E; // S29JL032
		case 8192: return 0x7E;	// S29GL064S
		default: return 0xFF;
		}
	}
	case 0x1C:
	{
		switch (flash_size)
		{
		case 4096: return 0x0A; // S29JL032
		case 8192: return 0x10;	// S29GL064S
		default: return 0xFF;
		}
	}
	case 0x1E:
	{
		switch (flash_size)
		{
		case 4096: return S29JL032_TOP ? 0x01 : 0x00;	// S29JL032: 0x00 = bottom boot block | 0x01 = top boot block
		case 8192: return S29GL064S_TOP ? 0x01 : 0x00;	// S29GL064S: 0x00 = bottom boot block | 0x01 = top boot block
		default: return 0xFF;
		}
	}
	default: return 0xFF;
	}
}

uint8 FASTCALL Rainbow2FlashChrID(uint32 A) {
	return Rainbow2FlashID(CHIP_TYPE_CHR, A);
}

static DECLFR(Rainbow2FlashPrgID)
{
	return Rainbow2FlashID(CHIP_TYPE_PRG, A);
}

void Rainbow2FlashIDEnter(uint8 chip)
{
	switch (chip)
	{
	case CHIP_TYPE_PRG:
		if (flash_id[chip])
			return;
		flash_id[chip] = 1;
		if (bootrom)
			SetReadHandler(0x8000, 0xDFFF, Rainbow2FlashPrgID);
		else
			SetReadHandler(0x8000, 0xFFFF, Rainbow2FlashPrgID);
		break;
	case CHIP_TYPE_CHR:
		if (CHR_FLASHROM == NULL)
			return;
		if (flash_id[chip])
			return;
		flash_id[chip] = 1;
		FFCEUX_PPURead = Rainbow2FlashChrID;
		break;
	default:
		return;
	}
}

void Rainbow2FlashIDExit(uint8 chip)
{
	switch (chip)
	{
	case CHIP_TYPE_PRG:
		if (!flash_id[chip])
			return;
		flash_id[chip] = 0;
		SetReadHandler(0x8000, 0xFFFF, CartBR);
		break;
	case CHIP_TYPE_CHR:
		if (!flash_id[chip])
			return;
		flash_id[chip] = 0;
		FFCEUX_PPURead = Rainbow2PPURead;
		break;
	default:
		return;
	}
}

void Rainbow2Flash(uint8 chip, uint32 flash_addr, uint8 V) {

	uint32 command_addr = flash_addr & 0x0FFF;

	enum
	{
		flash_mode_READY = 0,
		flash_mode_COMMAND,
		flash_mode_UNLOCK_BYPASS,
		flash_mode_BYTE_WRITE,
		flash_mode_ERASE,
	};

	switch (flash_mode[chip])
	{
	default:
	case flash_mode_READY:
		if (command_addr == 0x0AAA && V == 0xAA)
		{
			flash_mode[chip] = flash_mode_COMMAND;
			flash_sequence[chip] = 0;
		}
		else if (V == 0xF0)
		{
			Rainbow2FlashIDExit(chip);
		}
		break;
	case flash_mode_COMMAND:
		if (flash_sequence[chip] == 0)
		{
			if (command_addr == 0x0555 && V == 0x55)
			{
				flash_sequence[chip] = 1;
			}
			else
			{
				flash_mode[chip] = flash_mode_READY;
			}
		}
		else if (flash_sequence[chip] == 1)
		{
			if (command_addr == 0x0AAA)
			{
				flash_sequence[chip] = 0;
				switch (V)
				{
				default:   flash_mode[chip] = flash_mode_READY; break;
				case 0x20: flash_mode[chip] = flash_mode_UNLOCK_BYPASS; break;
				case 0xA0: flash_mode[chip] = flash_mode_BYTE_WRITE; break;
				case 0x80: flash_mode[chip] = flash_mode_ERASE; break;
				case 0x90: Rainbow2FlashIDEnter(chip); flash_mode[chip] = flash_mode_READY; break;
				case 0xF0: Rainbow2FlashIDExit(chip); flash_mode[chip] = flash_mode_READY; break;
				}
			}
			else
				flash_mode[chip] = flash_mode_READY;
		}
		else
			flash_mode[chip] = flash_mode_READY; // should be unreachable
		break;
	case flash_mode_UNLOCK_BYPASS:
		if (flash_sequence[chip] == 0)
		{
			switch (V)
			{
			case 0xA0: flash_sequence[chip] = 1; break;
			case 0x90: flash_sequence[chip] = 2; break;
			}
		}
		else if (flash_sequence[chip] == 1)
		{
			flash_sequence[chip] = 0;
			if (chip == CHIP_TYPE_PRG)
			{
				if (PRG_FLASHROM[flash_addr] == 0xff) PRG_FLASHROM[flash_addr] &= V;
			}
			else if (chip == CHIP_TYPE_CHR)
			{
				if (CHR_FLASHROM[flash_addr] == 0xff) CHR_FLASHROM[flash_addr] &= V;
			}
		}
		else if (flash_sequence[chip] == 2)
		{
			if (V == 0x00)
			{
				flash_sequence[chip] = 0;
				flash_mode[chip] = flash_mode_READY;
			}
		}
		break;
	case flash_mode_BYTE_WRITE:
		if (chip == CHIP_TYPE_PRG)
		{
			if (PRG_FLASHROM[flash_addr] == 0xff) PRG_FLASHROM[flash_addr] &= V;
		}
		else if (chip == CHIP_TYPE_CHR)
		{
			if (CHR_FLASHROM[flash_addr] == 0xff) CHR_FLASHROM[flash_addr] &= V;
		}
		flash_mode[chip] = flash_mode_READY;
		break;
	case flash_mode_ERASE:
		if (flash_sequence[chip] == 0)
		{
			if (command_addr == 0x0AAA && V == 0xAA)
				flash_sequence[chip] = 1;
			else
				flash_mode[chip] = flash_mode_READY;
		}
		else if (flash_sequence[chip] == 1)
		{
			if (command_addr == 0x0555 && V == 0x55)
				flash_sequence[chip] = 2;
			else
				flash_mode[chip] = flash_mode_READY;
		}
		else if (flash_sequence[chip] == 2)
		{
			if (command_addr == 0x0AAA && V == 0x10) // erase chip
			{
				if (chip == CHIP_TYPE_PRG)
					memset(PRG_FLASHROM, 0xFF, PRG_FLASHROMSIZE);
				else if (chip == CHIP_TYPE_CHR)
					memset(CHR_FLASHROM, 0xFF, CHR_FLASHROMSIZE);

			}
			else if (V == 0x30) // erase sectors
			{
				uint32 sector_offset = (flash_addr & 0xFF0000);
				uint8 sector_index = sector_offset >> 16;
				uint8 sector_size = 64;
				uint32 flash_size;

				if (chip == CHIP_TYPE_PRG)
					flash_size = ROM_size * 16;
				else if (chip == CHIP_TYPE_CHR)
					flash_size = VROM_size * 8;

				switch (flash_size)
				{
				case 1024: // S29AL008
					if (S29AL008_TOP && sector_index == 15)
					{
							 if (flash_addr >= 0xF0000 && flash_addr <= 0xF7FFF) { sector_offset = 0xF0000; sector_size = 32; }
						else if (flash_addr >= 0xF8000 && flash_addr <= 0xF9FFF) { sector_offset = 0xF8000; sector_size = 8; }
						else if (flash_addr >= 0xFA000 && flash_addr <= 0xFBFFF) { sector_offset = 0xFA000; sector_size = 8; }
						else if (flash_addr >= 0xFC000 && flash_addr <= 0xFFFFF) { sector_offset = 0xFC000; sector_size = 16; }
					}
					else if (!S29AL008_TOP && sector_index == 0)
					{
							 if (flash_addr >= 0x00000 && flash_addr <= 0x03FFF) { sector_offset = 0x00000; sector_size = 16; }
						else if (flash_addr >= 0x04000 && flash_addr <= 0x05FFF) { sector_offset = 0x04000; sector_size = 8; }
						else if (flash_addr >= 0x06000 && flash_addr <= 0x07FFF) { sector_offset = 0x06000; sector_size = 8; }
						else if (flash_addr >= 0x08000 && flash_addr <= 0x0FFFF) { sector_offset = 0x08000; sector_size = 32; }
					}
					break;
				case 2048: // S29AL016
					if (S29AL016_TOP && sector_index == 31)
					{
							 if (flash_addr >= 0x1F0000 && flash_addr <= 0x1F7FFF) { sector_offset = 0xF0000; sector_size = 32; }
						else if (flash_addr >= 0x1F8000 && flash_addr <= 0x1F9FFF) { sector_offset = 0xF8000; sector_size = 8; }
						else if (flash_addr >= 0x1FA000 && flash_addr <= 0x1FBFFF) { sector_offset = 0xFA000; sector_size = 8; }
						else if (flash_addr >= 0x1FC000 && flash_addr <= 0x1FFFFF) { sector_offset = 0xFC000; sector_size = 16; }
					}
					else if (!S29AL016_TOP && sector_index == 0)
					{
							 if (flash_addr >= 0x00000 && flash_addr <= 0x03FFF) { sector_offset = 0x00000; sector_size = 16; }
						else if (flash_addr >= 0x04000 && flash_addr <= 0x05FFF) { sector_offset = 0x04000; sector_size = 8; }
						else if (flash_addr >= 0x06000 && flash_addr <= 0x07FFF) { sector_offset = 0x06000; sector_size = 8; }
						else if (flash_addr >= 0x08000 && flash_addr <= 0x0FFFF) { sector_offset = 0x08000; sector_size = 32; }
					}
					break;
				case 4096: // S29JL032
					if (S29JL032_TOP && sector_index == 63)
					{
							 if (flash_addr >= 0x3F0000 && flash_addr <= 0x3F1FFF) { sector_offset = 0x3F0000; sector_size = 8; }
						else if (flash_addr >= 0x3F2000 && flash_addr <= 0x3F3FFF) { sector_offset = 0x3F2000; sector_size = 8; }
						else if (flash_addr >= 0x3F4000 && flash_addr <= 0x3F5FFF) { sector_offset = 0x3F4000; sector_size = 8; }
						else if (flash_addr >= 0x3F6000 && flash_addr <= 0x3F7FFF) { sector_offset = 0x3F6000; sector_size = 8; }
						else if (flash_addr >= 0x3F8000 && flash_addr <= 0x3F9FFF) { sector_offset = 0x3F8000; sector_size = 8; }
						else if (flash_addr >= 0x3FA000 && flash_addr <= 0x3FBFFF) { sector_offset = 0x3FA000; sector_size = 8; }
						else if (flash_addr >= 0x3FC000 && flash_addr <= 0x3FDFFF) { sector_offset = 0x3FC000; sector_size = 8; }
						else if (flash_addr >= 0x3FE000 && flash_addr <= 0x3FFFFF) { sector_offset = 0x3FE000; sector_size = 8; }

					}
					else if (!S29JL032_TOP && sector_index == 0)
					{
							 if (flash_addr >= 0x000000 && flash_addr <= 0x001FFF) { sector_offset = 0x000000; sector_size = 8; }
						else if (flash_addr >= 0x002000 && flash_addr <= 0x003FFF) { sector_offset = 0x002000; sector_size = 8; }
						else if (flash_addr >= 0x004000 && flash_addr <= 0x005FFF) { sector_offset = 0x004000; sector_size = 8; }
						else if (flash_addr >= 0x006000 && flash_addr <= 0x007FFF) { sector_offset = 0x006000; sector_size = 8; }
						else if (flash_addr >= 0x008000 && flash_addr <= 0x009FFF) { sector_offset = 0x008000; sector_size = 8; }
						else if (flash_addr >= 0x00A000 && flash_addr <= 0x00BFFF) { sector_offset = 0x00A000; sector_size = 8; }
						else if (flash_addr >= 0x00C000 && flash_addr <= 0x00DFFF) { sector_offset = 0x00C000; sector_size = 8; }
						else if (flash_addr >= 0x00E000 && flash_addr <= 0x00FFFF) { sector_offset = 0x00E000; sector_size = 8; }
					}
					break;
				case 8192: // S29GL064S
					if (!S29GL064S_TOP && sector_index == 0)
					{
							 if (flash_addr >= 0x000000 && flash_addr <= 0x001FFF) { sector_offset = 0x000000; sector_size = 8; }
						else if (flash_addr >= 0x002000 && flash_addr <= 0x003FFF) { sector_offset = 0x002000; sector_size = 8; }
						else if (flash_addr >= 0x004000 && flash_addr <= 0x005FFF) { sector_offset = 0x004000; sector_size = 8; }
						else if (flash_addr >= 0x006000 && flash_addr <= 0x007FFF) { sector_offset = 0x006000; sector_size = 8; }
						else if (flash_addr >= 0x008000 && flash_addr <= 0x009FFF) { sector_offset = 0x008000; sector_size = 8; }
						else if (flash_addr >= 0x00A000 && flash_addr <= 0x00BFFF) { sector_offset = 0x00A000; sector_size = 8; }
						else if (flash_addr >= 0x00C000 && flash_addr <= 0x00DFFF) { sector_offset = 0x00C000; sector_size = 8; }
						else if (flash_addr >= 0x00E000 && flash_addr <= 0x00FFFF) { sector_offset = 0x00E000; sector_size = 8; }
					}
					else if (S29GL064S_TOP && sector_index == 127)
					{
							 if (flash_addr >= 0x7F0000 && flash_addr <= 0x7F1FFF) { sector_offset = 0x7F0000; sector_size = 8; }
						else if (flash_addr >= 0x7F2000 && flash_addr <= 0x7F3FFF) { sector_offset = 0x7F2000; sector_size = 8; }
						else if (flash_addr >= 0x7F4000 && flash_addr <= 0x7F5FFF) { sector_offset = 0x7F4000; sector_size = 8; }
						else if (flash_addr >= 0x7F6000 && flash_addr <= 0x7F7FFF) { sector_offset = 0x7F6000; sector_size = 8; }
						else if (flash_addr >= 0x7F8000 && flash_addr <= 0x7F9FFF) { sector_offset = 0x7F8000; sector_size = 8; }
						else if (flash_addr >= 0x7FA000 && flash_addr <= 0x7FBFFF) { sector_offset = 0x7FA000; sector_size = 8; }
						else if (flash_addr >= 0x7FC000 && flash_addr <= 0x7FDFFF) { sector_offset = 0x7FC000; sector_size = 8; }
						else if (flash_addr >= 0x7FE000 && flash_addr <= 0x7FFFFF) { sector_offset = 0x7FE000; sector_size = 8; }
					}
					break;
				}

				if (chip == CHIP_TYPE_PRG)
					memset(PRG_FLASHROM + sector_offset, 0xFF, 1024 * sector_size);
				else if (chip == CHIP_TYPE_CHR)
					memset(CHR_FLASHROM + sector_offset, 0xFF, 1024 * sector_size);

			}
			flash_mode[chip] = flash_mode_READY;
		}
		else
			flash_mode[chip] = flash_mode_READY; // should be unreachable
		break;
	}
}

static DECLFW(RNBW_0x8000Wr) {
	if ((A < 0x6000) || A > (0xFFFF))
		return;

	uint32 flash_addr = A;

	uint8 t_prg_rom_mode = bootrom ? PRG_ROM_MODE_3 : prg_rom_mode;

	if ((A >= 0x6000) & (A < 0x8000))
	{
		switch (prg_ram_mode)
		{
		case PRG_RAM_MODE_0:
			if ((A >= 0x6000) & (A <= 0x7FFF) & !(prg[1] & 0x8000))
			{
				flash_addr &= 0x1FFF;
				flash_addr |= (prg[1] & 0x7FFF) << 13;
			}
			else
			{
				return;
			}
			break;
		case PRG_RAM_MODE_1:
			flash_addr &= 0x0FFF;
			if ((A >= 0x6000) & (A <= 0x6FFF) & !(prg[1] & 0x8000))
			{
				flash_addr |= (prg[1] & 0x7FFF) << 12;
			}
			else if ((A >= 0x7000) & (A <= 0x7FFF) & !(prg[2] & 0x8000))
			{
				flash_addr |= (prg[2] & 0x7FFF) << 12;
			}
			else
			{
				return;
			}
			break;
		}
	}
	else if ((A >= 0x8000) & (A <= 0xFFFF))
	{
		switch (t_prg_rom_mode)
		{
		case PRG_ROM_MODE_0:
			flash_addr &= 0x7FFF;
			flash_addr |= (prg[3] & 0x7FFF) << 15;
			break;
		case PRG_ROM_MODE_1:
			flash_addr &= 0x3FFF;
			if ((A >= 0x8000) & (A <= 0xBFFF))
			{
				flash_addr |= (prg[3] & 0x7FFF) << 14;
			}
			else if ((A >= 0xC000) & (A <= 0xFFFF))
			{
				flash_addr |= (prg[7] & 0x7FFF) << 14;
			}
			break;
		case PRG_ROM_MODE_2:
			if ((A >= 0x8000) & (A <= 0xBFFF))
			{
				flash_addr &= 0x3FFF;
				flash_addr |= (prg[3] & 0x3F) << 14;
			}
			else if ((A >= 0xC000) & (A <= 0xDFFF))
			{
				flash_addr &= 0x1FFF;
				flash_addr |= (prg[7] & 0x7FFF) << 13;
			}
			else if ((A >= 0xE000) & (A <= 0xFFFF))
			{
				flash_addr &= 0x1FFF;
				flash_addr |= (prg[9] & 0x7FFF) << 13;
			}
			break;
		case PRG_ROM_MODE_3:
			flash_addr &= 0x1FFF;
			if ((A >= 0x8000) & (A <= 0x9FFF))
			{
				flash_addr |= (prg[3] & 0x7FFF) << 13;
			}
			else if ((A >= 0xA000) & (A <= 0xBFFF))
			{
				flash_addr |= (prg[5] & 0x7FFF) << 13;
			}
			else if ((A >= 0xC000) & (A <= 0xDFFF))
			{
				flash_addr |= (prg[7] & 0x7FFF) << 13;
			}
			else if ((A >= 0xE000) & (A <= 0xFFFF))
			{
				flash_addr |= (prg[9] & 0x7FFF) << 13;
			}
			break;
		case PRG_ROM_MODE_4:
			flash_addr &= 0x0FFF;
			if ((A >= 0x8000) & (A <= 0x8FFF))
			{
				flash_addr |= (prg[3] & 0x7FFF) << 12;
			}
			else if ((A >= 0x9000) & (A <= 0x9FFF))
			{
				flash_addr |= (prg[4] & 0x7FFF) << 12;
			}
			else if ((A >= 0xA000) & (A <= 0xAFFF))
			{
				flash_addr |= (prg[5] & 0x7FFF) << 12;
			}
			else if ((A >= 0xB000) & (A <= 0xBFFF))
			{
				flash_addr |= (prg[6] & 0x7FFF) << 12;
			}
			else if ((A >= 0xC000) & (A <= 0xCFFF))
			{
				flash_addr |= (prg[7] & 0x7FFF) << 12;
			}
			else if ((A >= 0xD000) & (A <= 0xDFFF))
			{
				flash_addr |= (prg[8] & 0x7FFF) << 12;
			}
			else if ((A >= 0xE000) & (A <= 0xEFFF))
			{
				flash_addr |= (prg[9] & 0x7FFF) << 12;
			}
			else if ((A >= 0xF000) & (A <= 0xFFFF))
			{
				flash_addr |= (prg[10] & 0x7FFF) << 12;
			}
			break;

		default:
			return;
		}
	}
	Rainbow2Flash(CHIP_TYPE_PRG, flash_addr, V);
}

static void Rainbow2PPUWrite(uint32 A, uint8 V) {
/*
	// if CHR-RAM but CHR-RAM does not exist then return
	if (chr_chip == CHR_CHIP_RAM && CHRRAM == NULL)
		return;

	// if CHR-ROM but CHR-ROM does not exist then return
	if (chr_chip == CHR_CHIP_ROM && CHR_FLASHROM == NULL)
		return;
	else*/
	{
		uint32 flash_addr = A;
		if (A < 0x2000)
		{
			switch (chr_mode)
			{
				// NOTE: plus les bons modes ?
			case CHR_MODE_0:
				flash_addr &= 0x1FFF;
				flash_addr |= (chr[A >> 13] & 0xffff) << 13;
				break;
			case CHR_MODE_1:
				flash_addr &= 0xFFF;
				flash_addr |= (chr[A >> 12] & 0xffff) << 12;
				break;
			case CHR_MODE_2:
				flash_addr &= 0x7FF;
				flash_addr |= (chr[A >> 11] & 0xffff) << 11;
				break;
			case CHR_MODE_3:
				flash_addr &= 0x3FF;
				flash_addr |= (chr[A >> 10] & 0xffff) << 10;
				break;
			case CHR_MODE_4:
				flash_addr &= 0x1FF;
				flash_addr |= (chr[A >> 9] & 0xffff) << 9;
				break;
			}
			Rainbow2Flash(CHIP_TYPE_CHR, flash_addr, V);
		}
	}
	FFCEUX_PPUWrite_Default(A, V);
}

static void Rainbow2Reset(void) {
	// PRG - 32K banks mapped to first PRG-ROM bank
	prg_rom_mode = PRG_ROM_MODE_0;
	prg[3] = 0;

	// CHR - 8K banks mapped to first bank of CHR-ROM
	// extended sprite mode disabled
	chr_chip = CHR_CHIP_ROM;
	chr_spr_ext_mode = 0;
	chr_mode = CHR_MODE_0;
	chr[0] = 0;

	// Nametables - CIRAM horizontal mirroring
	NT_bank[0] = 0;
	NT_bank[1] = 0;
	NT_bank[2] = 1;
	NT_bank[3] = 1;
	RNBWHackNTcontrol[0] = 0;
	RNBWHackNTcontrol[1] = 0;
	RNBWHackNTcontrol[2] = 0;
	RNBWHackNTcontrol[3] = 0;

	// Scanline IRQ
	S_IRQcontrol = 0;
	S_IRQoffset = 135;

	// CPU Cycle IRQ
	C_IRQe = C_IRQp = C_IRQr = 0;

	// Audio Output
	audio_output = 1;

	// ESP / WiFi
	esp_enable = false;
	//clear_esp_message_received();
	esp_message_sent = true;
	//rx_address = 0;
	//tx_address = 0;
	//rx_index = 0;
	ESP_IRQp = 0;
	Sync();
}

static void Rainbow2Power(void) {

	// mapper init
	if(MiscROMS) bootrom = 1;
	else bootrom = 0;
	Rainbow2Reset();	

	SetReadHandler(0x4800, 0xFFFF, CartBR);
	SetWriteHandler(0x4800, 0x7FFF, CartBW);
/*
	if (WRAM)
		FCEU_CheatAddRAM(WRAMSIZE >> 10, 0x6000, WRAM);
	FCEU_CheatAddRAM(FPGA_RAMSIZE >> 10, 0x5000, FPGA_RAM);
	FCEU_CheatAddRAM(0x1800 >> 10, 0x4800, FPGA_RAM);
*/
	// mapper registers (writes)
	SetWriteHandler(0x4100, 0x47ff, RNBW_0x4100Wr);

	// mapper registers (reads)
	SetReadHandler(0x4100, 0x47ff, RNBW_0x4100Rd);

	// audio expansion registers (writes)
	SetWriteHandler(0x41A0, 0x41A8, RNBW_ExpAudioWr);

	// FPGA WRAM @ $4800-$4fff (reads/writes)
	SetWriteHandler(0x4800, 0x4fff, FPGA_0x4800Wr);
	SetReadHandler(0x4800, 0x4fff, FPGA_0x4800Rd);

	// FPGA WRAM @ $5000-$5fff (reads/writes)
	SetWriteHandler(0x5000, 0x5fff, WRAM_0x5000Wr);
	SetReadHandler(0x5000, 0x5fff, WRAM_0x5000Rd);

	// WRAM @ $6000-$7fff (reads/writes)
	SetWriteHandler(0x6000, 0x7fff, WRAM_0x6000Wr);
	SetReadHandler(0x6000, 0x7fff, WRAM_0x6000Rd);

	// self-flashing
	flash_mode[CHIP_TYPE_PRG] = 0;
	flash_mode[CHIP_TYPE_CHR] = 0;
	flash_sequence[CHIP_TYPE_PRG] = 0;
	flash_sequence[CHIP_TYPE_CHR] = 0;
	flash_id[CHIP_TYPE_PRG] = false;
	flash_id[CHIP_TYPE_CHR] = false;
	SetWriteHandler(0x8000, 0xFFFF, RNBW_0x8000Wr);

	// fill WRAM/FPGA_RAM/CHRRAM/DUMMY_CHRRAM/DUMMY_CHRROM with random values
	if (WRAM && !RNBWbattery)
		FCEU_MemoryRand(WRAM, WRAMSIZE, false);

	if (FPGA_RAM && !RNBWbattery)
		FCEU_MemoryRand(FPGA_RAM, FPGA_RAMSIZE, false);

	if (CHRRAM && !RNBWbattery)
		FCEU_MemoryRand(CHRRAM, CHRRAMSIZE, false);

	if (DUMMY_CHRRAM)
		FCEU_MemoryRand(DUMMY_CHRRAM, DUMMY_CHRRAMSIZE, false);

	if (DUMMY_CHRROM)
		FCEU_MemoryRand(DUMMY_CHRROM, DUMMY_CHRROMSIZE, false);

	// init FPGA RAM with bootrom CHR data
	for (size_t i = 0; i < 0x1000; i++)
	{
		FPGA_RAM[i] = bootrom_chr[i];
	}

	// init FPGA RAM with bootrom PRG data
	if (MiscROMS)
	{
		// init FPGA_RAM with MISC ROM data
		for (size_t i = 0; i < 0xE00; i++)
		{
			FPGA_RAM[0x1000 + i] = MiscROMS[i];
		}
	}

	// ESP firmware
	esp = new BrokeStudioFirmware;
	//esp_enable = false;
	//esp_irq_enable = false;
	clear_esp_message_received();
	//esp_message_sent = true;
	//rx_address = 0;
	//tx_address = 0;
	//rx_index = 0;
}

static void Rainbow2Close(void)
{
	if (WRAM)
	{
		FCEU_gfree(WRAM);
		WRAM = NULL;
	}

	if (FPGA_RAM)
	{
		FCEU_gfree(FPGA_RAM);
		FPGA_RAM = NULL;
	}

	if (CHRRAM)
		ExtraNTARAM = NULL;

	if (DUMMY_CHRRAM)
	{
		FCEU_gfree(DUMMY_CHRRAM);
		DUMMY_CHRRAM = NULL;
		ExtraNTARAM = NULL;
	}

	if (PRG_FLASHROM)
	{
		FCEU_gfree(PRG_FLASHROM);
		PRG_FLASHROM = NULL;
	}

	if (CHR_FLASHROM)
	{
		FCEU_gfree(CHR_FLASHROM);
		CHR_FLASHROM = NULL;
	}

	if (esp)
	{
		esp_enable = false;
		delete esp;
		esp = NULL;
	}

	RNBWHack = 0;
	RNBWHackNTcontrol[0] = 0;
	RNBWHackNTcontrol[1] = 0;
	RNBWHackNTcontrol[2] = 0;
	RNBWHackNTcontrol[3] = 0;
	RNBWHackVROMPtr = NULL;
	RNBWHackExNTARAMPtr = NULL;
}

static void StateRestore(int version) {
	Sync();
}

// audio expansion

static void DoSQV1(void);
static void DoSQV2(void);
static void DoSawV(void);

static INLINE void DoSQV(int x) {
	int32 V;
	int32 amp = (((vpsg1[x << 2] & 15) << 8) * 6 / 8) >> 4;
	int32 start, end;

	start = cvbc[x];
	end = (SOUNDTS << 16) / soundtsinc;
	if (end <= start)
		return;
	cvbc[x] = end;

	if ((vpsg1[(x << 2) | 0x2] & 0x80) && (audio_output & 0x03))
	{
		if (vpsg1[x << 2] & 0x80)
		{
			for (V = start; V < end; V++)
				Wave[V >> 4] += amp;
		}
		else
		{
			int32 thresh = (vpsg1[x << 2] >> 4) & 7;
			int32 freq = ((vpsg1[(x << 2) | 0x1] | ((vpsg1[(x << 2) | 0x2] & 15) << 8)) + 1) << 17;
			for (V = start; V < end; V++) {
				if (dcount[x] > thresh)
					Wave[V >> 4] += amp;
				vcount[x] -= nesincsize;
				while (vcount[x] <= 0) {
					vcount[x] += freq;
					dcount[x] = (dcount[x] + 1) & 15;
				}
			}
		}
	}
}

static void DoSQV1(void) {
	DoSQV(0);
}

static void DoSQV2(void) {
	DoSQV(1);
}

static void DoSawV(void) {
	int V;
	int32 start, end;

	start = cvbc[2];
	end = (SOUNDTS << 16) / soundtsinc;
	if (end <= start)
		return;
	cvbc[2] = end;

	if ((vpsg2[2] & 0x80) && (audio_output & 0x03))
	{
		static int32 saw1phaseacc = 0;
		uint32 freq3;
		static uint8 b3 = 0;
		static int32 phaseacc = 0;
		static uint32 duff = 0;

		freq3 = (vpsg2[1] + ((vpsg2[2] & 15) << 8) + 1);

		for (V = start; V < end; V++) {
			saw1phaseacc -= nesincsize;
			if (saw1phaseacc <= 0)
			{
				int32 t;
			rea:
				t = freq3;
				t <<= 18;
				saw1phaseacc += t;
				phaseacc += vpsg2[0] & 0x3f;
				b3++;
				if (b3 == 7)
				{
					b3 = 0;
					phaseacc = 0;
				}
				if (saw1phaseacc <= 0)
					goto rea;
				duff = (((phaseacc >> 3) & 0x1f) << 4) * 6 / 8;
			}
			Wave[V >> 4] += duff;
		}
	}
}

static INLINE void DoSQVHQ(int x) {
	int32 V;
	int32 amp = ((vpsg1[x << 2] & 15) << 8) * 6 / 8;

	if ((vpsg1[(x << 2) | 0x2] & 0x80) && (audio_output & 0x03))
	{
		if (vpsg1[x << 2] & 0x80)
		{
			for (V = cvbc[x]; V < (int)SOUNDTS; V++)
				WaveHi[V] += amp;
		}
		else
		{
			int32 thresh = (vpsg1[x << 2] >> 4) & 7;
			for (V = cvbc[x]; V < (int)SOUNDTS; V++) {
				if (dcount[x] > thresh)
					WaveHi[V] += amp;
				vcount[x]--;
				if (vcount[x] <= 0)
				{
					vcount[x] = (vpsg1[(x << 2) | 0x1] | ((vpsg1[(x << 2) | 0x2] & 15) << 8)) + 1;
					dcount[x] = (dcount[x] + 1) & 15;
				}
			}
		}
	}
	cvbc[x] = SOUNDTS;
}

static void DoSQV1HQ(void) {
	DoSQVHQ(0);
}

static void DoSQV2HQ(void) {
	DoSQVHQ(1);
}

static void DoSawVHQ(void) {
	static uint8 b3 = 0;
	static int32 phaseacc = 0;
	int32 V;

	if ((vpsg2[2] & 0x80) && (audio_output & 0x03))
	{
		for (V = cvbc[2]; V < (int)SOUNDTS; V++) {
			WaveHi[V] += (((phaseacc >> 3) & 0x1f) << 8) * 6 / 8;
			vcount[2]--;
			if (vcount[2] <= 0)
			{
				vcount[2] = (vpsg2[1] + ((vpsg2[2] & 15) << 8) + 1) << 1;
				phaseacc += vpsg2[0] & 0x3f;
				b3++;
				if (b3 == 7) {
					b3 = 0;
					phaseacc = 0;
				}
			}
		}
	}
	cvbc[2] = SOUNDTS;
}

void Rainbow2Sound(int Count) {
	int x;

	DoSQV1();
	DoSQV2();
	DoSawV();
	for (x = 0; x < 3; x++)
		cvbc[x] = Count;
}

void Rainbow2SoundHQ(void) {
	DoSQV1HQ();
	DoSQV2HQ();
	DoSawVHQ();
}

void Rainbow2SyncHQ(int32 ts) {
	int x;
	for (x = 0; x < 3; x++) cvbc[x] = ts;
}

static void Rainbow2ESI(void) {
	GameExpSound.RChange = Rainbow2ESI;
	GameExpSound.Fill = Rainbow2Sound;
	GameExpSound.HiFill = Rainbow2SoundHQ;
	GameExpSound.HiSync = Rainbow2SyncHQ;

	memset(cvbc, 0, sizeof(cvbc));
	memset(vcount, 0, sizeof(vcount));
	memset(dcount, 0, sizeof(dcount));
	if (FSettings.SndRate)
	{
		if (FSettings.soundq >= 1)
		{
			sfun[0] = DoSQV1HQ;
			sfun[1] = DoSQV2HQ;
			sfun[2] = DoSawVHQ;
		}
		else
		{
			sfun[0] = DoSQV1;
			sfun[1] = DoSQV2;
			sfun[2] = DoSawV;
		}
	}
	else
		memset(sfun, 0, sizeof(sfun));
	AddExState(&SStateRegs, ~0, 0, 0);
}

#if 0
// Let's disable NSF support for now since I don't have an NSF ROM file to test it yet.

// NSF Init

void NSFRainbow2_Init(void) {
	Rainbow2ESI();
	SetWriteHandler(0x8000, 0xbfff, RNBW_ExpAudioWr);
}
#endif

// mapper init

void RAINBOW2_Init(CartInfo *info) {
	info->Power = Rainbow2Power;
	info->Reset = Rainbow2Reset;
	info->Close = Rainbow2Close;

	RNBWbattery = info->battery;

	// WRAM
	if (info->wram_size != 0)
	{
		if (info->wram_size > 0x80000)
		{
			WRAMSIZE = 0x80000; // maximum is 512KiB
		}
		else if (info->wram_size > 0x80000)
		{
			WRAMSIZE = 0x8000;  // minimum is 32Kib
		}
		else
		{
			WRAMSIZE = info->wram_size & 0xF8000; // we need this to match the hardware as close as possible
		}

		if (WRAMSIZE != 0)
		{
			WRAM = (uint8*)FCEU_gmalloc(WRAMSIZE);
			SetupCartPRGMapping(0x10, WRAM, WRAMSIZE, 1);
			AddExState(WRAM, WRAMSIZE, 0, "WRAM");
			info->wram_size = WRAMSIZE;

			if (RNBWbattery)
			{
				info->addSaveGameBuf(WRAM, WRAMSIZE);
			}
		}
		else
		{
			info->wram_size = 0;
		}
	}

	// FPGA_RAM
	FPGA_RAM = (uint8*)FCEU_gmalloc(FPGA_RAMSIZE);
	SetupCartPRGMapping(0x12, FPGA_RAM, FPGA_RAMSIZE, 1);
	SetupCartCHRMapping(0x12, FPGA_RAM, FPGA_RAMSIZE, 1);
	AddExState(FPGA_RAM, FPGA_RAMSIZE, 0, "FPGA_RAM");
	if (info->battery)
	{
		info->addSaveGameBuf(FPGA_RAM, FPGA_RAMSIZE);
	}

	// PRG FLASH ROM
	PRG_FLASHROM = (uint8*)FCEU_gmalloc(PRG_FLASHROMSIZE);
	AddExState(PRG_FLASHROM, PRG_FLASHROMSIZE, 0, "PFROM");
	if (info->battery)
	{
		info->addSaveGameBuf(PRG_FLASHROM, PRG_FLASHROMSIZE);
	}

	// copy PRG ROM into PRG_FLASHROM, use it instead of PRG ROM
	PRGSIZE = ROM_size * 16 * 1024;
	for (uint32 w = 0, r = 0; w < PRG_FLASHROMSIZE; ++w)
	{
		PRG_FLASHROM[w] = ROM[r];
		++r;
		if (r >= PRGSIZE)
			r = 0;
	}
	SetupCartPRGMapping(0x11, PRG_FLASHROM, PRG_FLASHROMSIZE, 0);

	// CHR-RAM
	if (info->vram_size != 0)
	{
		if (info->vram_size > 0x80000)
		{
			CHRRAMSIZE = 0x80000; // maximum is 512KiB
		}
		else if (info->vram_size > 0x80000)
		{
			CHRRAMSIZE = 0x8000;  // minimum is 32Kib
		}
		else
		{
			CHRRAMSIZE = info->vram_size & 0xF8000; // we need this to match the hardware as close as possible
		}

		if (CHRRAMSIZE != 0)
		{
			CHRRAMSIZE = info->vram_size;
			CHRRAM = (uint8*)FCEU_gmalloc(CHRRAMSIZE);
			SetupCartCHRMapping(0x11, CHRRAM, CHRRAMSIZE, 1);
			AddExState(CHRRAM, CHRRAMSIZE, 0, "CRAM");
			ExtraNTARAM = CHRRAM + 30 * 1024;
			AddExState(ExtraNTARAM, 2048, 0, "EXNR");
			info->vram_size = CHRRAMSIZE;
		}
		else
		{
			info->vram_size = 0;
		}
	}
	else
	{
		// create dummy CHR-RAM to avoid crash when trying to use CHR-RAM for nametables
		// when no CHR-RAM is specified in the ROM header
		DUMMY_CHRRAM = (uint8*)FCEU_gmalloc(DUMMY_CHRRAMSIZE);
		ExtraNTARAM = DUMMY_CHRRAM;
		AddExState(ExtraNTARAM, DUMMY_CHRRAMSIZE, 0, "EXNR");
	}

	// CHR FLASHROM
	if (VROM_size != 0)
	{
		CHR_FLASHROM = (uint8*)FCEU_gmalloc(CHR_FLASHROMSIZE);
		AddExState(CHR_FLASHROM, CHR_FLASHROMSIZE, 0, "CFROM");
		if (info->battery)
		{
			info->addSaveGameBuf(CHR_FLASHROM, CHR_FLASHROMSIZE);
		}

		// copy CHR ROM into CHR_FLASHROM, use it instead of CHR ROM
		CHRSIZE = VROM_size * 8 * 1024;
		for (uint32 w = 0, r = 0; w < CHR_FLASHROMSIZE; ++w)
		{
			CHR_FLASHROM[w] = VROM[r];
			++r;
			if (r >= CHRSIZE)
				r = 0;
		}
		SetupCartCHRMapping(0x10, CHR_FLASHROM, CHR_FLASHROMSIZE, 0);
	}
		else
	{
		// create dummy CHR-ROM to avoid crash when trying to use CHR-ROM for pattern tables
		// when no CHR-ROM is specified in the ROM header
		DUMMY_CHRROM = (uint8*)FCEU_gmalloc(DUMMY_CHRROMSIZE);
		SetupCartCHRMapping(0x10, DUMMY_CHRROM, DUMMY_CHRROMSIZE, 0);
	}

	// BOOTROM
	if (info->misc_roms != 0 && MiscROMS)
	{
		SetupCartPRGMapping(0x13, MiscROMS, MiscROMS_size, 0);
	}

	FFCEUX_PPURead = Rainbow2PPURead;
	FFCEUX_PPUWrite = Rainbow2PPUWrite;

	//GameHBIRQHook = Rainbow2hb;
	MapIRQHook = Rainbow2IRQ;
	RNBWHack = 1;
	RNBWHackNTcontrol[0] = 0;
	RNBWHackNTcontrol[1] = 0;
	RNBWHackNTcontrol[2] = 0;
	RNBWHackNTcontrol[3] = 0;
	RNBWHackVROMPtr = FPGA_RAM;
	RNBWHackVROMMask = FPGA_RAMSIZE - 1;
	RNBWHackExNTARAMPtr = FPGA_RAM;
	Rainbow2ESI();
	GameStateRestore = StateRestore;

	AddExState(&FlashRegs, ~0, 0, 0);
	AddExState(&Rainbow2StateRegs, ~0, 0, 0);
}
