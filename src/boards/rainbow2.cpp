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

#ifndef RAINBOW_DEBUG_MAPPER
#define RAINBOW_DEBUG_MAPPER 0
#endif

#if RAINBOW_DEBUG_MAPPER >= 1
#define UDBG(...) FCEU_printf(__VA_ARGS__)
#else
#define UDBG(...)
#endif

#define MAPPER_PLATFORM_PCB 0
#define MAPPER_PLATFORM_EMU 1
#define MAPPER_PLATFORM_WEB 2
#define MAPPER_VERSION_PROTOTYPE 0
#define MAPPER_VERSION      (MAPPER_PLATFORM_EMU << 5) | MAPPER_VERSION_PROTOTYPE

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

static uint8 RNBWHackNTbank[5];
static uint8 SPR_bank_offset;
static uint8 SPR_bank[64];

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
static uint8 reset_step = 0;

// Scanline IRQ
static bool S_IRQ_enable, S_IRQ_pending, S_IRQ_in_frame, S_IRQ_HBlank;
static uint8 S_IRQ_latch, S_IRQ_offset, S_IRQ_jitter_counter;

// additional flags to emulate hardware correctly
static float S_IRQ_dot_count;
static uint8 S_IRQ_ready, S_IRQ_last_SL_triggered;

// CPU Cycle IRQ
static bool C_IRQ_enable, C_IRQ_reset, C_IRQ_pending;
static int32 C_IRQLatch, C_IRQCount;

// ESP message IRQ
static uint8 ESP_IRQ_pending;

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

	{ &S_IRQ_enable, 1, "PPUE" },
	{ &S_IRQ_pending, 1, "PPUP" },
	{ &S_IRQ_in_frame, 1, "PPUF" },
	{ &S_IRQ_HBlank, 1, "PPUH" },
	{ &S_IRQ_latch, 1, "PPUL" },
	{ &S_IRQ_offset, 1, "PPUO" },
	{ &S_IRQ_dot_count, 4, "PPUD" },
	{ &S_IRQ_ready, 1, "PPUR" },
	{ &S_IRQ_last_SL_triggered, 1, "PPUT" },

	{ &C_IRQ_enable, 1, "CPUE" },
	{ &C_IRQ_pending, 1, "CPUP" },
	{ &C_IRQ_reset, 1, "CPUR" },
	{ &C_IRQLatch, 4, "CPUL" },
	{ &C_IRQCount, 4, "CPUC" },

	{ &ESP_IRQ_pending, 1, "ESPP" },

	{ 0 }
};

static void(*sfun[3]) (void);
static uint8 vpsg1[8];
static uint8 vpsg2[4];
static int32 cvbc[3];
static int32 vcount[3];
static int32 dcount[2];

static uint8 ZPCM[3];

static SFORMAT SStateRegs[] =
{
	{ vpsg1, 8, "PSG1" },	// SQ 1+2
	{ vpsg2, 4, "PSG2" },	// SAW
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
	if (!C_IRQ_pending & !S_IRQ_pending & !ESP_IRQ_pending)
	{
		X6502_IRQEnd(FCEU_IQEXT);
	}
}

static void Rainbow2IRQ(int a) {

	// Scanline IRQ
	int sl = newppu_get_scanline()-1;
	int dot = newppu_get_dot();

	S_IRQ_jitter_counter += a;

	if (dot >= 256)
		S_IRQ_HBlank = true;
	else
		S_IRQ_HBlank = false;

	if (!PPUON || sl >= 241)
	{
		// whenever rendering is off for any reason (vblank or forced disable)
		// the irq counter resets, as well as the inframe flag (easily verifiable from software)
		S_IRQ_in_frame = false; // in-frame flag cleared
		S_IRQ_pending = false; // pending IRQ flag cleared
		S_IRQ_ready = 0;
		S_IRQ_last_SL_triggered = 255;
		IRQEnd();
	}
	else
	{
		if (!S_IRQ_in_frame)
		{
			S_IRQ_in_frame = true;
			S_IRQ_pending = false;
			IRQEnd();
		}

		// this is kind of hacky but result is pretty close to hardware
		if ((sl == S_IRQ_latch) & (S_IRQ_ready == 0) & !S_IRQ_pending & (sl != S_IRQ_last_SL_triggered))
		{
			S_IRQ_ready = 1;
			S_IRQ_dot_count = (float)dot;
			if (PAL) S_IRQ_dot_count -= a * 3.2;
			else S_IRQ_dot_count -= (float)a * 3;
		}

		if ((S_IRQ_ready == 1) & !S_IRQ_pending)
		{
			if (PAL) S_IRQ_dot_count += (float)a * 3.2;
			else S_IRQ_dot_count += (float)a * 3;

			int dotTarget = (S_IRQ_offset * 2);
			if (PAL) dotTarget += 20;
			else dotTarget += 24;

			if (((int)S_IRQ_dot_count) >= dotTarget)
			{
				S_IRQ_ready = 2;
				S_IRQ_last_SL_triggered = S_IRQ_latch;
			}
		}

		if (S_IRQ_enable & !S_IRQ_pending & (S_IRQ_ready == 2))
		{
			X6502_IRQBegin(FCEU_IQEXT);
			S_IRQ_pending = true;
			S_IRQ_ready = 0;
			S_IRQ_jitter_counter = 0;
		}
	}

	// Cycle Counter IRQ
	if (C_IRQ_enable) {
		C_IRQCount -= a;
		if (C_IRQCount <= 0) {
			C_IRQ_enable = C_IRQ_reset;
			C_IRQ_pending = true;
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
			ESP_IRQ_pending = 1;
		}
		else
		{
			ESP_IRQ_pending = 0;
		}
	}

	IRQEnd();
}

static void Sync(void) {
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
	if (t_prg_rom_mode == PRG_ROM_MODE_4)
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

	if (chr_chip == CHR_CHIP_FPGA_RAM)
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
			setchr8r(cart_chr_map, chr[0] & 0x03ff);
			break;
		case CHR_MODE_1:
			setchr4r(cart_chr_map, 0x0000, chr[0] & 0x07ff);
			setchr4r(cart_chr_map, 0x1000, chr[1] & 0x07ff);
			break;
		case CHR_MODE_2:
			setchr2r(cart_chr_map, 0x0000, chr[0] & 0x0fff);
			setchr2r(cart_chr_map, 0x0800, chr[1] & 0x0fff);
			setchr2r(cart_chr_map, 0x1000, chr[2] & 0x0fff);
			setchr2r(cart_chr_map, 0x1800, chr[3] & 0x0fff);
			break;
		case CHR_MODE_3:
			for (uint8 i = 0; i < 8; i++) {
				setchr1r(cart_chr_map, i << 10, chr[i] & 0x1fff);
			}
			break;
		case CHR_MODE_4:
			for (uint8 i = 0; i < 16; i++) {
				setchr512r(cart_chr_map, i << 9, chr[i] & 0x3fff);
			}
			break;
		}
	}

	for (int i = 0; i < 4; i++)
	{
		uint8 cur_RNBWHackNT = RNBWHackNTbank[i];
		uint8 cur_NT_chip = RNBWHackNTcontrol[i] >> 6;
		//uint8 cur_NT_1K_dest = (RNBWHackNTcontrol[i] & 0x0C) >> 2;
		//uint8 cur_NT_ext_mode = RNBWHackNTcontrol[i] & 0x03;

		switch (cur_NT_chip)
		{
		case NT_CIRAM:
			vnapage[i] = NTARAM + 0x400 * (cur_RNBWHackNT & 0x01);
			break;
		case NT_CHR_RAM:
			vnapage[i] = CHRRAM + 0x400 * (cur_RNBWHackNT & 0x1f);
			break;
		case NT_FPGA_RAM:
			vnapage[i] = FPGA_RAM + 0x400 * (cur_RNBWHackNT & 0x03);
			break;
		case NT_CHR_ROM:
			vnapage[i] = CHR_FLASHROM + 0x400 * cur_RNBWHackNT;
			break;
		}
	}

	RNBWHackSplitNTARAMPtr = FPGA_RAM + 0x400 * (RNBWHackNTbank[4] & 0x03);

}

static DECLFW(RNBW_ExpAudioWr) {
	uint8 i;
	if (A >= 0x41A0 && A <= 0x41A2)
	{
		i = A & 3; // 0, 1, 2
		vpsg1[i] = V;
		if (sfun[0])
			sfun[0]();
	}
	else if (A >= 0x41A3 && A <= 0x41A5)
	{
		i = 4 | ((A - 3) & 3); // 4, 5, 6
		vpsg1[i] = V;
		if (sfun[1])
			sfun[1]();
	}
	else if (A >= 0x41A6 && A <= 0x41A8)
	{
		i = (A - 6) & 3; // 0, 1, 2
		vpsg2[i] = V;
		if (sfun[2])
			sfun[2]();
	}
}

static DECLFR(RNBW_0x4011Rd) {
	uint8 ZPCM_val = ((ZPCM[0] + ZPCM[1] + ZPCM[2]) & 0x3f) << 2;
	return ZPCM_val;
}

static DECLFR(RNBW_RstPowRd) {
	if (fceuindbg == 0)
	{
		if (A == 0xFFFC) reset_step = 1;
		else if ((A == 0xFFFD) & (reset_step == 1))
		{
			Rainbow2Reset();
			reset_step = 0;
		}
		else reset_step = 0;
	}
	return CartBR(A);
}

static DECLFR(RNBW_0x4100Rd) {
	switch (A)
	{
	case 0x4100: return (prg_ram_mode << 6) | prg_rom_mode;
	case 0x4120: return (chr_chip << 6) | (chr_spr_ext_mode << 5) | (RNBWHackSplitEnable << 4) | chr_mode;
	case 0x4151: {
		uint8 rv = (S_IRQ_HBlank ? 0x80 : 0) | (S_IRQ_in_frame ? 0x40 : 0 | (S_IRQ_pending ? 0x01 : 0));
		S_IRQ_pending = false;
		S_IRQ_ready = 0;
		IRQEnd();
		return rv;
	}
	case 0x4154: return S_IRQ_jitter_counter;
	case 0x4160: return MAPPER_VERSION;
	case 0x4161: return (S_IRQ_pending ? 0x80 : 0) | (C_IRQ_pending ? 0x40 : 0) | (ESP_IRQ_pending ? 0x01 : 0);

		// ESP - WiFi
	case 0x4190:
	{
		uint8 esp_enable_flag = esp_enable ? 0x01 : 0x00;
		uint8 irq_enable_flag = esp_irq_enable ? 0x02 : 0x00;
		UDBG("RAINBOW read flags %04x => %02xs\n", A, esp_enable_flag | irq_enable_flag);
		return esp_enable_flag | irq_enable_flag;
	}
	case 0x4191:
	{
		uint8 esp_message_received_flag = esp_message_received() ? 0x80 : 0;
		uint8 esp_rts_flag = esp->getDataReadyIO() ? 0x40 : 0x00;
		return esp_message_received_flag | esp_rts_flag;
	}
	case 0x4192: return esp_message_sent ? 0x80 : 0;
	case 0x4195:
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
		int bank = (A & 0x0f) - 5;
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
		int bank = (A & 0x0f) - 5;
		prg[bank] = (prg[bank] & 0xff00) | V;
		Sync();
		break;
	}
	// CHR banking / chip selector / vertical split-screen / sprite extended mode
	case 0x4120:
		chr_chip = (V & 0xC0) >> 6;
		chr_spr_ext_mode = (V & 0x20) >> 5;
		RNBWHackSplitEnable = (V & 0x10) >> 4;
		chr_mode = V & 0x07;
		Sync();
		break;
	case 0x4121:
		RNBWHackBGBankOffset = V & 0x1f;
		break;
	// Nametables bank
	case 0x4126: RNBWHackNTbank[0] = V; Sync(); break;
	case 0x4127: RNBWHackNTbank[1] = V; Sync(); break;
	case 0x4128: RNBWHackNTbank[2] = V; Sync(); break;
	case 0x4129: RNBWHackNTbank[3] = V; Sync(); break;
	case 0x412E: RNBWHackNTbank[4] = V; Sync(); break;
	// Nametables control
	case 0x412A: RNBWHackNTcontrol[0] = V; Sync(); break;
	case 0x412B: RNBWHackNTcontrol[1] = V; Sync(); break;
	case 0x412C: RNBWHackNTcontrol[2] = V; Sync(); break;
	case 0x412D: RNBWHackNTcontrol[3] = V; Sync(); break;
	case 0x412F: RNBWHackNTcontrol[4] = V; Sync(); break;
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
	case 0x4150: S_IRQ_latch = V; break;
	case 0x4151: S_IRQ_enable = true; break;
	case 0x4152: S_IRQ_pending = false; S_IRQ_enable = false; break;
	case 0x4153: S_IRQ_offset = V > 169 ? 169 : V; break;
	// CPU Cycle IRQ
	case 0x4158: C_IRQLatch &= 0xFF00; C_IRQLatch |= V; C_IRQCount = C_IRQLatch; break;
	case 0x4159: C_IRQLatch &= 0x00FF; C_IRQLatch |= V << 8; C_IRQCount = C_IRQLatch; break;
	case 0x415A:
		C_IRQ_enable = V & 0x01;
		C_IRQ_reset = (V & 0x02) >> 1;
		if (C_IRQ_enable)
			C_IRQCount = C_IRQLatch;
		break;
	case 0x415B:
		C_IRQ_pending = false;
		IRQEnd();
		break;
	// Window Mode
	case 0x4170:
		RNBWHackWindowXStartTile = V & 0x1f;
		break;
	case 0x4171:
		RNBWHackWindowXEndTile = V & 0x1f;
		break;
	case 0x4172:
		//RNBWHackWindowYStart = V; // TODO
		break;
	case 0x4173:
		//RNBWHackWindowYEnd = V; // TODO
		break;
	case 0x4174:
		RNBWHackSplitXScroll = V & 0x1f;
		break;
	case 0x4175:
		RNBWHackSplitYScroll = V;
		break;
	// ESP - WiFi
	case 0x4190:
		esp_enable = V & 0x01;
		esp_irq_enable = V & 0x02;
		break;
	case 0x4191:
		if (esp_enable) clear_esp_message_received();
		else FCEU_printf("RAINBOW warning: $4190.0 is not set\n");
		break;
	case 0x4192:
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
		else FCEU_printf("RAINBOW warning: $4190.0 is not set\n");
		break;
	case 0x4193:
		rx_address = V & 0x07;
		break;
	case 0x4194:
		tx_address = V & 0x07;
		break;
	case 0x4195:
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

	int xt = NTRefreshAddr & 31;
	int yt = (NTRefreshAddr >> 5) & 31;
	int ntnum = (NTRefreshAddr >> 10) & 3;

	bool split = false;
	int linetile;
	if(newppu)
	{
		if (RNBWHackSplitEnable & (
			((RNBWHackWindowXStartTile <= RNBWHackWindowXEndTile) & ((xt >= RNBWHackWindowXStartTile) & (xt <= RNBWHackWindowXEndTile))) |
			((RNBWHackWindowXStartTile > RNBWHackWindowXEndTile) & ((xt >= RNBWHackWindowXStartTile) | (xt <= RNBWHackWindowXEndTile)))
			)) {
			static const int kHack = -1; //dunno if theres science to this or if it just fixes SDF (cant be bothered to think about it)
			linetile = (newppu_get_scanline() + kHack + RNBWHackSplitYScroll) / 8;
			split = true;
		}
	}

	if (A < 0x2000) // pattern table / CHR data
	{
		if (ppuphase == PPUPHASE_OBJ && SpriteON) // sprite fetch
		{
			if (chr_spr_ext_mode)
				if (Sprite16)
					return RNBWHackVROMPtr[((SPR_bank[RNBWHackCurSprite] << 13) & RNBWHackVROMMask) + (A)]; // TODO: test + fix
				else
					return RNBWHackVROMPtr[((SPR_bank[RNBWHackCurSprite] << 12) & RNBWHackVROMMask) + (A)];
			else
			{
				return VPage[A >> 9][A]; // TODO: FIXME?
			}
		}

		if (ppuphase == PPUPHASE_BG && ScreenON) // tile fetch
		{

			if(split)
			{
				return RNBWHackVROMPtr[A & 0xFFF];
			}

			uint8 *tmp = FCEUPPU_GetCHR(A, NTRefreshAddr);
			if (tmp == NULL) return X.DB;
			else return *tmp;
		}

		// default
		return FFCEUX_PPURead_Default(A);
	}
	else if (A < 0x3F00)  // tiles / attributes
	{
		if ((A & 0x3FF) >= 0x3C0) { // attributes
			if (split) {
				//return 0x00;
				//uint8 NT_1K_dest = (RNBWHackNTcontrol[4] & 0x0C) >> 2;
				//uint8 NT_ext_mode = RNBWHackNTcontrol[4] & 0x03;

				//REF NT: return 0x2000 | (v << 0xB) | (h << 0xA) | (vt << 5) | ht;
				//REF AT: return 0x2000 | (v << 0xB) | (h << 0xA) | 0x3C0 | ((vt & 0x1C) << 1) | ((ht & 0x1C) >> 2);

				// Attributes
				//return FCEUPPU_GetAttr(ntnum, xt, yt);

				A &= ~(0x1C << 1); //mask off VT
				A |= (linetile & 0x1C) << 1; //mask on adjusted VT
				return FPGA_RAM[RNBWHackNTbank[4] * 0x400 + (A & 0x3ff)];
			} else {
				return FCEUPPU_GetAttr(ntnum, xt, yt);
			}
		} else { // tiles
			if(split) {
				// Tiles
				A &= ~((0x1F << 5) | (1 << 0xB)); //mask off VT and V
				A |= (linetile & 31) << 5; //mask on adjusted VT (V doesnt make any sense, I think)

				//uint8 *tmp = FCEUPPU_GetCHR(A, NTRefreshAddr);
				//if (tmp == NULL) return X.DB;
				//else return *tmp;

				return FPGA_RAM[RNBWHackNTbank[4] * 0x400 + (A & 0x3ff)];
			} else {
				return vnapage[(A >> 10) & 0x3][A & 0x3FF];
			}
		}
/*
		if(split)
		{
			uint8 NT_1K_dest = (RNBWHackNTcontrol[4] & 0x0C) >> 2;
			uint8 NT_ext_mode = RNBWHackNTcontrol[4] & 0x03;

			static const int kHack = -1; //dunno if theres science to this or if it just fixes SDF (cant be bothered to think about it)
			int linetile = (newppu_get_scanline() + kHack) / 8 + RNBWHackSplitYScroll;

			//REF NT: return 0x2000 | (v << 0xB) | (h << 0xA) | (vt << 5) | ht;
			//REF AT: return 0x2000 | (v << 0xB) | (h << 0xA) | 0x3C0 | ((vt & 0x1C) << 1) | ((ht & 0x1C) >> 2);

			// TODO: need to handle 8x8 attributes
			if((A & 0x3FF) >= 0x3C0)
			{
				// Attributes
				return FCEUPPU_GetAttr(ntnum, xt, yt);

				A &= ~(0x1C << 1); //mask off VT
				A |= (linetile & 0x1C) << 1; //mask on adjusted VT
				return FPGA_RAM[RNBWHackNTbank[4] * 0x400 + (A & 0x3ff)];
			}
			else
			{
				// Tiles
				//A &= ~((0x1F << 5) | (1 << 0xB)); //mask off VT and V
				//A |= (linetile & 31) << 5; //mask on adjusted VT (V doesnt make any sense, I think)

				uint8 *tmp = FCEUPPU_GetCHR(A, NTRefreshAddr);
				if (tmp == NULL) return X.DB;
				else return *tmp;

				return FPGA_RAM[RNBWHackNTbank[4] * 0x400 + (A & 0x3ff)];
				
			}
		}
*/
/*
		if ((A & 0x3FF) >= 0x3C0)
		{
			uint8 NT = (A >> 10) & 0x03;
			uint8 NT_1K_dest = (RNBWHackNTcontrol[NT] & 0x0C) >> 2;
			uint8 NT_ext_mode = RNBWHackNTcontrol[NT] & 0x03;

			// 8x8 attributes
			if (NT_ext_mode & 0x01)
			{
				uint8 byte = FPGA_RAM[NT_1K_dest * 0x400 + (NTRefreshAddr & 0x3ff)];
				//get attribute part and paste it 4x across the byte
				byte >>= 6;
				byte *= 0x55;
				return byte;
			}
		}
*/

		return vnapage[(A >> 10) & 0x3][A & 0x3FF];
	} else { // palette fetch
		return FFCEUX_PPURead_Default(A);
	}
}

uint8 Rainbow2FlashID(uint8 chip, uint32 A) {
	// Software ID mode is undefined by the datasheet for all but the lowest 2 addressable bytes,
	// but some tests of the chip currently being used found it repeats in 512-byte patterns.
	// http://forums.nesdev.com/viewtopic.php?p=178728#p178728

	uint32 flash_size;

	if (chip == CHIP_TYPE_PRG) flash_size = ROM_size * 16;
	else if (chip == CHIP_TYPE_CHR) flash_size = VROM_size * 8;
	else flash_size = ROM_size * 16; // Invalid chip number. Behavior to be verified, but don't let flash_size uninitialized.

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
			SetReadHandler(0x6000, 0xDFFF, Rainbow2FlashPrgID);
		else
			SetReadHandler(0x6000, 0xFFFF, Rainbow2FlashPrgID);
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
		SetReadHandler(0x6000, 0xFFFF, CartBR);
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
				else
					// Invalid chip number. Behavior to be verified, but don't let flash_size uninitialized.
					flash_size = ROM_size * 16;

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

static DECLFW(RNBW_0x6000Wr) {
	if ((A < 0x6000) || A > (0xFFFF))
		return;

	uint32 flash_addr = A;
	uint8 prg_idx;

	uint8 t_prg_rom_mode = bootrom ? PRG_ROM_MODE_3 : prg_rom_mode;

	if ((A >= 0x6000) & (A < 0x8000))
	{
		switch (prg_ram_mode)
		{
		case PRG_RAM_MODE_0:
			// 8K
			if (((prg[1] & 0x8000) >> 14) != 0)
				return CartBW(A, V); // FPGA_RAM or WRAM

			// PRG-ROM
			flash_addr &= 0x1FFF;
			flash_addr |= (prg[1] & 0x7FFF) << 13;

			break;
		case PRG_RAM_MODE_1:
			// 2 x 4K
			prg_idx = ((A >> 12) & 0x07) - 6;
			if (((prg[prg_idx] & 0x8000) >> 14) != 0)
				return CartBW(A, V); // FPGA_RAM or WRAM

			// PRG-ROM
			flash_addr &= 0x1FFF;
			flash_addr |= (prg[prg_idx] & 0x7FFF) << 13;

			break;
		}
	}
	else if ((A >= 0x8000) & (A <= 0xFFFF))
	{
		switch (t_prg_rom_mode)
		{
		case PRG_ROM_MODE_0:
			// 32K
			if ((prg[3] >> 14) != 0)
				return CartBW(A, V); // WRAM

			// PRG-ROM
			flash_addr &= 0x7FFF;
			flash_addr |= (prg[3] & 0x7FFF) << 15;

			break;
		case PRG_ROM_MODE_1:
			// 2 x 16K
			prg_idx = ((A >> 12) & 0x04) + 3;
			if ((prg[prg_idx] >> 14) != 0)
				return CartBW(A, V); // WRAM

			// PRG-ROM
			flash_addr &= 0x3FFF;
			flash_addr |= (prg[prg_idx] & 0x7FFF) << 14;

			break;
		case PRG_ROM_MODE_2:
			if ((A >= 0x8000) & (A <= 0xBFFF))
			{
				// 16K
				if ((prg[3] >> 14) != 0)
					return CartBW(A, V); // WRAM

				// PRG-ROM
				flash_addr &= 0x3FFF;
				flash_addr |= (prg[3] & 0x3F) << 14;
			}
			else if ((A >= 0xC000) & (A <= 0xFFFF))
			{
				// 2 x 8K
				prg_idx = ((A >> 12) & 0x06) + 3;
				if ((prg[prg_idx] >> 14) != 0)
					return CartBW(A, V); // WRAM

				// PRG-ROM
				flash_addr &= 0x1FFF;
				flash_addr |= (prg[prg_idx] & 0x7FFF) << 13;

			}
			break;
		case PRG_ROM_MODE_3:
			// 4 x 8K
			prg_idx = ((A >> 12) & 0x06) + 3;
			if ((prg[prg_idx] >> 14) != 0)
				return CartBW(A, V); // WRAM

			// PRG-ROM
			flash_addr &= 0x1FFF;
			flash_addr |= (prg[prg_idx] & 0x7FFF) << 13;

			break;
		case PRG_ROM_MODE_4:
			// 8 x 4K
			prg_idx = ((A >> 12) & 0x07) + 3;

			if ((prg[prg_idx] >> 14) != 0)
				return CartBW(A, V); // WRAM

			// PRG-ROM
			flash_addr &= 0x0FFF;
			flash_addr |= (prg[prg_idx] & 0x7FFF) << 13;
			break;

		default:
			return CartBW(A, V);
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
	// reset
	reset_step = 0;

	// PRG - 32K banks mapped to first PRG-ROM bank
	prg_rom_mode = PRG_ROM_MODE_0;
	prg[3] = 0;

	// CHR - 8K banks mapped to first bank of CHR-ROM
	// extended sprite mode disabled
	chr_chip = CHR_CHIP_ROM;
	chr_spr_ext_mode = 0;
	RNBWHackSplitEnable = 0;
	chr_mode = CHR_MODE_0;
	chr[0] = 0;

	// Nametables - CIRAM horizontal mirroring
	RNBWHackNTbank[0] = 0;
	RNBWHackNTbank[1] = 0;
	RNBWHackNTbank[2] = 1;
	RNBWHackNTbank[3] = 1;
	RNBWHackNTcontrol[0] = 0;
	RNBWHackNTcontrol[1] = 0;
	RNBWHackNTcontrol[2] = 0;
	RNBWHackNTcontrol[3] = 0;

	// Scanline IRQ
	S_IRQ_enable = false;
	S_IRQ_pending = false;
	S_IRQ_in_frame = false;
	S_IRQ_HBlank = false;
	S_IRQ_offset = 135;

	// CPU Cycle IRQ
	C_IRQ_enable = false;
	C_IRQ_pending = false;
	C_IRQ_reset = false;

	// Audio Output
	audio_output = 1;

	// ESP / WiFi
	esp_enable = false;
	//clear_esp_message_received();
	esp_message_sent = true;
	//rx_address = 0;
	//tx_address = 0;
	//rx_index = 0;
	ESP_IRQ_pending = 0;
	Sync();
}

static void Rainbow2Power(void) {

	// mapper init
	if (MiscROMS) bootrom = 1;
	else bootrom = 0;
	Rainbow2Reset();

	// reset-power vectors
	SetReadHandler(0xFFFC, 0xFFFF, RNBW_RstPowRd);

	//
	SetReadHandler(0x4800, 0xFFFB, CartBR);
	SetWriteHandler(0x4800, 0x5FFF, CartBW);

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

	// audio expansion (reads)
	SetReadHandler(0x4011, 0x4011, RNBW_0x4011Rd);

	// audio expansion registers (writes)
	SetWriteHandler(0x41A0, 0x41A8, RNBW_ExpAudioWr);

	// self-flashing
	flash_mode[CHIP_TYPE_PRG] = 0;
	flash_mode[CHIP_TYPE_CHR] = 0;
	flash_sequence[CHIP_TYPE_PRG] = 0;
	flash_sequence[CHIP_TYPE_CHR] = 0;
	flash_id[CHIP_TYPE_PRG] = false;
	flash_id[CHIP_TYPE_CHR] = false;
	SetWriteHandler(0x6000, 0xFFFF, RNBW_0x6000Wr);
	
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
	RNBWHackNTcontrol[4] = 0;
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
			{
				Wave[V >> 4] += amp;
			}
		}
		else
		{
			int32 thresh = (vpsg1[x << 2] >> 4) & 7;
			int32 freq = ((vpsg1[(x << 2) | 0x1] | ((vpsg1[(x << 2) | 0x2] & 15) << 8)) + 1) << 17;
			for (V = start; V < end; V++) {
				if (dcount[x] > thresh)
				{
					Wave[V >> 4] += amp;
				}
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
	int volume = vpsg1[x << 2] & 15;
	int duty = (vpsg1[x << 2] >> 4) & 7;
	bool ignore_duty = vpsg1[x << 2] & 0x80;
	int freq_lo = vpsg1[(x << 2) | 0x1];
	int freq_hi = vpsg1[(x << 2) | 0x2] & 15;
	bool enable = vpsg1[(x << 2) | 0x2] & 0x80;

	int32 V;
	int32 amp = (volume << 8) * 6 / 8;

	if (enable)
	{
		if (ignore_duty)
		{
			for (V = cvbc[x]; V < (int)SOUNDTS; V++)
			{
				if (audio_output & 0x03) WaveHi[V] += amp;
				ZPCM[x] = volume;
			}
		}
		else
		{
			int32 thresh = duty;
			for (V = cvbc[x]; V < (int)SOUNDTS; V++) {
				if (dcount[x] > thresh)
				{
					if (audio_output & 0x03) WaveHi[V] += amp;
					ZPCM[x] = volume;
				}
				vcount[x]--;
				if (vcount[x] <= 0)
				{
					vcount[x] = (freq_lo | (freq_hi << 8)) + 1;
					dcount[x] = (dcount[x] + 1) & 15;
				}
			}
		}
	}
	else
	{
		ZPCM[x] = 0;
	}
	cvbc[x] = SOUNDTS;

	return;
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

	if (vpsg2[2] & 0x80)
	{
		for (V = cvbc[2]; V < (int)SOUNDTS; V++) {
			ZPCM[2] = (phaseacc >> 3) & 0x1f;
			if (audio_output & 0x03) WaveHi[V] += (((phaseacc >> 3) & 0x1f) << 8) * 6 / 8;
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
	else
	{
		ZPCM[2] = 0;
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
		else if (info->wram_size < 0x8000)
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
		else if (info->vram_size < 0x8000)
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
