/* FCE Ultra - NES/Famicom Emulator
*
* Copyright notice for this file:
*  Copyright (C) 2022 Cluster
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
* Very complicated homebrew multicart mapper with.
* The code is so obscured and weird because it's ported from Verilog CPLD source code:
* https://github.com/ClusterM/coolgirl-famicom-multicart/blob/master/CoolGirl_mappers.vh
*
* Range: $5000-$5FFF
*
* Mask: $5007
*
* All registers are $00 on power-on and reset.
*
* $5xx0
* 7  bit  0
* ---- ----
* PPPP PPPP
* |||| ||||
* ++++-++++-- PRG base offset (A29-A22)
*
* $5xx1
* 7  bit  0
* ---- ----
* PPPP PPPP
* |||| ||||
* ++++-++++-- PRG base offset (A21-A14)
*
* $5xx2
* 7  bit  0
* ---- ----
* AMMM MMMM
* |||| ||||
* |+++-++++-- PRG mask (A20-A14, inverted+anded with PRG address)
* +---------- CHR mask (A18, inverted+anded with CHR address)
*
* $5xx3
* 7  bit  0
* ---- ----
* BBBC CCCC
* |||| ||||
* |||+-++++-- CHR bank A (bits 7-3)
* +++-------- PRG banking mode (see below)
*
* $5xx4
* 7  bit  0
* ---- ----
* DDDE EEEE
* |||| ||||
* |||+-++++-- CHR mask (A17-A13, inverted+anded with CHR address)
* +++-------- CHR banking mode (see below)
*
* $5xx5
* 7  bit  0
* ---- ----
* CDDE EEWW
* |||| ||||
* |||| ||++-- 8KiB WRAM page at $6000-$7FFF
* |+++-++---- PRG bank A (bits 5-1)
* +---------- CHR bank A (bit 8)
*
* $5xx6
* 7  bit  0
* ---- ----
* FFFM MMMM
* |||| ||||
* |||+ ++++-- Mapper code (bits 4-0, see below)
* +++-------- Flags 2-0, functionality depends on selected mapper
*
* $5xx7
* 7  bit  0
* ---- ----
* LMTR RSNO
* |||| |||+-- Enable WRAM (read and write) at $6000-$7FFF
* |||| ||+--- Allow writes to CHR RAM
* |||| |+---- Allow writes to flash chip
* |||+-+----- Mirroring (00=vertical, 01=horizontal, 10=1Sa, 11=1Sb)
* ||+-------- Enable four-screen mode
* |+-- ------ Mapper code (bit 5, see below)
* +---------- Lockout bit (prevent further writes to all registers)
*
*/

#include "mapinc.h"

const uint32 SAVE_FLASH_SIZE = 1024 * 1024 * 8;
const uint32 FLASH_SECTOR_SIZE = 128 * 1024;
const int ROM_CHIP = 0x00;
const int WRAM_CHIP = 0x10;
const int FLASH_CHIP = 0x11;
const int CFI_CHIP = 0x13;

static int CHR_SIZE = 0;
static uint32 WRAM_SIZE = 0;
static uint8* WRAM = NULL;
static uint8* SAVE_FLASH = NULL;
static uint8* CFI;

static uint8 sram_enabled = 0;
static uint8 sram_page = 0;		// [1:0]
static uint8 can_write_chr = 0;
static uint8 map_rom_on_6000 = 0;
static uint8 flags = 0;			// [2:0]
static uint8 mapper = 0;		// [5:0]
static uint8 can_write_flash = 0;
static uint8 mirroring = 0;		// [1:0]
static uint8 four_screen = 0;
static uint8 lockout = 0;

static uint32 prg_base = 0;		// [26:14]
static uint32 prg_mask = 0b11111000 << 14; // 11111000, 128KB		// [20:14]
static uint8 prg_mode = 0;		// [2:0]
static uint8 prg_bank_6000 = 0;	// [7:0]
static uint8 prg_bank_a = 0;	// [7:0]
static uint8 prg_bank_b = 1;	// [7:0]
static uint8 prg_bank_c = ~1;	// [7:0]
static uint8 prg_bank_d = ~0;	// [7:0]

static uint32 chr_mask = 0;		// [18:13]
static uint8 chr_mode = 0;		// [2:0]
static uint16 chr_bank_a = 0;	// [8:0]
static uint16 chr_bank_b = 1;	// [8:0]
static uint16 chr_bank_c = 2;	// [8:0]
static uint16 chr_bank_d = 3;	// [8:0]
static uint16 chr_bank_e = 4;	// [8:0]
static uint16 chr_bank_f = 5;	// [8:0]
static uint16 chr_bank_g = 6;	// [8:0]
static uint16 chr_bank_h = 7;	// [8:0]

static uint8 TKSMIR[8];

static uint32 prg_bank_6000_mapped = 0;
static uint32 prg_bank_a_mapped = 0;
static uint32 prg_bank_b_mapped = 0;
static uint32 prg_bank_c_mapped = 0;
static uint32 prg_bank_d_mapped = 0;

// for MMC2/MMC4
static uint8 ppu_latch0 = 0;
static uint8 ppu_latch1 = 0;
// for MMC1
static uint64 lreset = 0;
static uint8 mmc1_load_register = 0;	// [5:0]
// for MMC3
static uint8 mmc3_internal = 0;			// [2:0]
// for mapper #69
static uint8 mapper69_internal = 0;		// [3:0]
// for mapper #112
static uint8 mapper112_internal = 0;	// [2:0]
// for mapper #163
static uint8 mapper_163_latch = 0;
static uint8 mapper163_r0 = 0;			// [7:0]
static uint8 mapper163_r1 = 0;			// [7:0]
static uint8 mapper163_r2 = 0;			// [7:0]
static uint8 mapper163_r3 = 0;			// [7:0]
static uint8 mapper163_r4 = 0;			// [7:0]
static uint8 mapper163_r5 = 0;			// [7:0]

// For mapper #90
static uint8 mul1 = 0;
static uint8 mul2 = 0;

// for MMC3 scanline-based interrupts, counts A12 rises after long A12 falls
static uint8 mmc3_irq_enabled = 0;				// register to enable/disable counter
static uint8 mmc3_irq_latch = 0;				// [7:0], stores counter reload latch value
static uint8 mmc3_irq_counter = 0;				// [7:0], counter itself (downcounting)
static uint8 mmc3_irq_reload = 0;				// flag to reload counter from latch
// for MMC5 scanline-based interrupts, counts dummy PPU reads
static uint8 mmc5_irq_enabled = 0;				// register to enable/disable counter
static uint8 mmc5_irq_line = 0;					// [7:0], scanline on which IRQ will be triggered
static uint8 mmc5_irq_out = 0;					// stores 1 when IRQ is triggered
// for mapper #18
static uint16 mapper18_irq_value = 0;			// [15:0], counter itself (downcounting)
static uint8 mapper18_irq_control = 0;			// [3:0], IRQ settings
static uint16 mapper18_irq_latch = 0;			// [15:0], stores counter reload latch value
// for mapper #65
static uint8 mapper65_irq_enabled = 0;			// register to enable/disable IRQ
static uint16 mapper65_irq_value = 0;			// [15:0], counter itself (downcounting)
static uint16 mapper65_irq_latch = 0;			// [15:0], stores counter reload latch value 
// reg mapper65_irq_out = 0;
// for Sunsoft FME-7
static uint8 mapper69_irq_enabled = 0;			// register to enable/disable IRQ
static uint8 mapper69_counter_enabled = 0;			// register to enable/disable counter
static uint16 mapper69_irq_value = 0;				// counter itself (downcounting)
// for VRC4 CPU-based interrupts
static uint8 vrc4_irq_value = 0;				// [7:0], counter itself (upcounting)
static uint8 vrc4_irq_control = 0;				// [2:0]� IRQ settings
static uint8 vrc4_irq_latch = 0;				// [7:0], stores counter reload latch value
static uint8 vrc4_irq_prescaler = 0;			// [6:0], prescaler counter for VRC4
static uint8 vrc4_irq_prescaler_counter = 0;	// prescaler cicles counter for VRC4
// for VRC3 CPU-based interrupts
static uint16 vrc3_irq_value = 0;				// [15:0], counter itself (upcounting)
static uint8 vrc3_irq_control = 0;				// [3:0], IRQ settings
static uint16 vrc3_irq_latch = 0;				// [15:0], stores counter reload latch value
// for mapper #42 (only Baby Mario)
static uint8 mapper42_irq_enabled = 0;			// register to enable/disable counter
static uint16 mapper42_irq_value = 0;			// [14:0], counter itself (upcounting)
// for mapper #83
static uint8 mapper83_irq_enabled_latch = 0;
static uint8 mapper83_irq_enabled = 0;
static uint16 mapper83_irq_counter = 0;
// for mapper #90
static uint8 mapper90_xor = 0;
// for mapper #67
static uint8 mapper67_irq_enabled = 0;
static uint8 mapper67_irq_latch = 0;
static uint16 mapper67_irq_counter = 0;

static uint8 flash_state = 0;
static uint16 flash_buffer_a[10];
static uint8 flash_buffer_v[10];
static uint8 cfi_mode = 0;

// Micron 4-gbit memory CFI data
static const uint8 cfi_data[] =
{ 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x51, 0x52, 0x59, 0x02, 0x00, 0x40, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x27, 0x36, 0x00, 0x00, 0x06,
  0x06, 0x09, 0x13, 0x03, 0x05, 0x03, 0x02, 0x1E,
  0x02, 0x00, 0x06, 0x00, 0x01, 0xFF, 0x03, 0x00,
  0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF,
  0x50, 0x52, 0x49, 0x31, 0x33, 0x14, 0x02, 0x01,
  0x00, 0x08, 0x00, 0x00, 0x02, 0xB5, 0xC5, 0x05,
  0x01, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
  0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

#define SET_BITS(target, target_bits, source, source_bits) target = set_bits(target, target_bits, get_bits(source, source_bits))

static inline uint8 string_to_bits(const char* bitsstr, int* bits)
{
	uint8 bit1, bit2, count = 0;
	for (int i = 0; i < 32; i++)
		bits[i] = -1;
	while (*bitsstr)
	{
		bit1 = 0;
		bit2 = 0;
		if (isdigit(*bitsstr))
		{
			while (isdigit(*bitsstr))
			{
				bit1 *= 10;
				bit1 += *bitsstr - '0';
				bitsstr++;
			}
			if (*bitsstr == ':')
			{
				bitsstr++;
				while (isdigit(*bitsstr))
				{
					bit2 *= 10;
					bit2 += *bitsstr - '0';
					bitsstr++;
				}
				if (bit2 < bit1)
					for (int i = bit1; i >= bit2; i--)
					{
						bits[count] = i;
						count++;
					}
				else
					for (int i = bit1; i <= bit2; i++)
					{
						bits[count] = i;
						count++;
					}
			}
			else {
				bits[count] = bit1;
				count++;
			}
		}
		else {
			bitsstr++;
		}
	}
	return count;
}

static inline uint32 get_bits(const uint32 V, const char* bitsstr)
{
	uint32 result = 0;
	int bits[32];
	string_to_bits(bitsstr, bits);
	for (int i = 0; bits[i] >= 0; i++)
	{
		result <<= 1;
		result |= (V >> bits[i]) & 1;
	}
	return result;
}

static inline uint32 set_bits(uint32 V, const char* bitsstr, const uint32 new_bits)
{
	int bits[32];
	uint8 count = string_to_bits(bitsstr, bits);
	for (int i = 0; i < count; i++)
	{
		if ((new_bits >> (count - i - 1)) & 1)
			V |= 1 << bits[i];
		else
			V &= ~(1 << bits[i]);
	}
	return V;
}

static void COOLGIRL_Sync_PRG(void) {
	prg_bank_6000_mapped = (prg_base >> 13) | (prg_bank_6000 & ((~(prg_mask >> 13) & 0xFE) | 1));
	prg_bank_a_mapped = (prg_base >> 13) | (prg_bank_a & ((~(prg_mask >> 13) & 0xFE) | 1));
	prg_bank_b_mapped = (prg_base >> 13) | (prg_bank_b & ((~(prg_mask >> 13) & 0xFE) | 1));
	prg_bank_c_mapped = (prg_base >> 13) | (prg_bank_c & ((~(prg_mask >> 13) & 0xFE) | 1));
	prg_bank_d_mapped = (prg_base >> 13) | (prg_bank_d & ((~(prg_mask >> 13) & 0xFE) | 1));
	uint8 REG_A_CHIP = (SAVE_FLASH != NULL && prg_bank_a_mapped >= 0x20000 - SAVE_FLASH_SIZE / 1024 / 8) ? FLASH_CHIP : ROM_CHIP;
	uint8 REG_B_CHIP = (SAVE_FLASH != NULL && prg_bank_b_mapped >= 0x20000 - SAVE_FLASH_SIZE / 1024 / 8) ? FLASH_CHIP : ROM_CHIP;
	uint8 REG_C_CHIP = (SAVE_FLASH != NULL && prg_bank_c_mapped >= 0x20000 - SAVE_FLASH_SIZE / 1024 / 8) ? FLASH_CHIP : ROM_CHIP;
	uint8 REG_D_CHIP = (SAVE_FLASH != NULL && prg_bank_d_mapped >= 0x20000 - SAVE_FLASH_SIZE / 1024 / 8) ? FLASH_CHIP : ROM_CHIP;

	if (!cfi_mode || !SAVE_FLASH)
	{
		switch (prg_mode & 7)
		{
		default:
		case 0:
			setprg16r(REG_A_CHIP, 0x8000, prg_bank_a_mapped >> 1);
			setprg16r(REG_C_CHIP, 0xC000, prg_bank_c_mapped >> 1);
			break;
		case 1:
			setprg16r(REG_C_CHIP, 0x8000, prg_bank_c_mapped >> 1);
			setprg16r(REG_A_CHIP, 0xC000, prg_bank_a_mapped >> 1);
			break;
		case 4:
			setprg8r(REG_A_CHIP, 0x8000, prg_bank_a_mapped);
			setprg8r(REG_B_CHIP, 0xA000, prg_bank_b_mapped);
			setprg8r(REG_C_CHIP, 0xC000, prg_bank_c_mapped);
			setprg8r(REG_D_CHIP, 0xE000, prg_bank_d_mapped);
			break;
		case 5:
			setprg8r(REG_C_CHIP, 0x8000, prg_bank_c_mapped);
			setprg8r(REG_B_CHIP, 0xA000, prg_bank_b_mapped);
			setprg8r(REG_A_CHIP, 0xC000, prg_bank_a_mapped);
			setprg8r(REG_D_CHIP, 0xE000, prg_bank_d_mapped);
			break;
		case 6:
			setprg32r(REG_A_CHIP, 0x8000, prg_bank_b_mapped >> 2);
			break;
		case 7:
			setprg32r(REG_A_CHIP, 0x8000, prg_bank_a_mapped >> 2);
			break;
		}
	}
	else {
		setprg32r(CFI_CHIP, 0x8000, 0);
	}

	if (!map_rom_on_6000 && WRAM)
		setprg8r(WRAM_CHIP, 0x6000, sram_page); // Select SRAM page
	else if (map_rom_on_6000)
		setprg8(0x6000, prg_bank_6000_mapped); // Map ROM on $6000-$7FFF
}

static void COOLGIRL_Sync_CHR(void) {
	// calculate CHR shift
	// wire shift_chr_right = ENABLE_MAPPER_021_022_023_025 && ENABLE_MAPPER_022 && (mapper == 6'b011000) && flags[1];
	int chr_shift_right = ((mapper == 0b011000) && (flags & 0b010)) ? 1 : 0;
	int chr_shift_left = 0;

	// enable or disable writes to CHR RAM, setup CHR mask
	SetupCartCHRMapping(0, UNIFchrrama, ((((~(chr_mask >> 13) & 0x3F) + 1) * 0x2000 - 1) & (CHR_SIZE - 1)) + 1, can_write_chr);

	switch (chr_mode & 7)
	{
	default:
	case 0:
		setchr8(chr_bank_a >> 3 >> chr_shift_right << chr_shift_left);
		break;
	case 1:
		setchr4(0x0000, mapper_163_latch >> chr_shift_right << chr_shift_left);
		setchr4(0x1000, mapper_163_latch >> chr_shift_right << chr_shift_left);
		break;
	case 2:
		setchr2(0x0000, chr_bank_a >> 1 >> chr_shift_right << chr_shift_left);
		TKSMIR[0] = TKSMIR[1] = chr_bank_a;
		setchr2(0x0800, chr_bank_c >> 1 >> chr_shift_right << chr_shift_left);
		TKSMIR[2] = TKSMIR[3] = chr_bank_c;
		setchr1(0x1000, chr_bank_e >> chr_shift_right << chr_shift_left);
		TKSMIR[4] = chr_bank_e;
		setchr1(0x1400, chr_bank_f >> chr_shift_right << chr_shift_left);
		TKSMIR[5] = chr_bank_f;
		setchr1(0x1800, chr_bank_g >> chr_shift_right << chr_shift_left);
		TKSMIR[6] = chr_bank_g;
		setchr1(0x1C00, chr_bank_h >> chr_shift_right << chr_shift_left);
		TKSMIR[7] = chr_bank_h;
		break;
	case 3:
		setchr1(0x0000, chr_bank_e >> chr_shift_right << chr_shift_left);
		TKSMIR[0] = chr_bank_e;
		setchr1(0x0400, chr_bank_f >> chr_shift_right << chr_shift_left);
		TKSMIR[1] = chr_bank_f;
		setchr1(0x0800, chr_bank_g >> chr_shift_right << chr_shift_left);
		TKSMIR[2] = chr_bank_g;
		setchr1(0x0C00, chr_bank_h >> chr_shift_right << chr_shift_left);
		TKSMIR[3] = chr_bank_h;
		setchr2(0x1000, chr_bank_a >> 1 >> chr_shift_right << chr_shift_left);
		TKSMIR[4] = TKSMIR[5] = chr_bank_a;
		setchr2(0x1800, chr_bank_c >> 1 >> chr_shift_right << chr_shift_left);
		TKSMIR[6] = TKSMIR[7] = chr_bank_c;
		break;
	case 4:
		setchr4(0x0000, chr_bank_a >> 2 >> chr_shift_right << chr_shift_left);
		setchr4(0x1000, chr_bank_e >> 2 >> chr_shift_right << chr_shift_left);
		break;
	case 5:
		if (!ppu_latch0)
			setchr4(0x0000, chr_bank_a >> 2 >> chr_shift_right << chr_shift_left);
		else
			setchr4(0x0000, chr_bank_b >> 2 >> chr_shift_right << chr_shift_left);
		if (!ppu_latch1)
			setchr4(0x1000, chr_bank_e >> 2 >> chr_shift_right << chr_shift_left);
		else
			setchr4(0x1000, chr_bank_f >> 2 >> chr_shift_right << chr_shift_left);
		break;
	case 6:
		setchr2(0x0000, chr_bank_a >> 1 >> chr_shift_right << chr_shift_left);
		setchr2(0x0800, chr_bank_c >> 1 >> chr_shift_right << chr_shift_left);
		setchr2(0x1000, chr_bank_e >> 1 >> chr_shift_right << chr_shift_left);
		setchr2(0x1800, chr_bank_g >> 1 >> chr_shift_right << chr_shift_left);
		break;
	case 7:
		setchr1(0x0000, chr_bank_a >> chr_shift_right << chr_shift_left);
		setchr1(0x0400, chr_bank_b >> chr_shift_right << chr_shift_left);
		setchr1(0x0800, chr_bank_c >> chr_shift_right << chr_shift_left);
		setchr1(0x0C00, chr_bank_d >> chr_shift_right << chr_shift_left);
		setchr1(0x1000, chr_bank_e >> chr_shift_right << chr_shift_left);
		setchr1(0x1400, chr_bank_f >> chr_shift_right << chr_shift_left);
		setchr1(0x1800, chr_bank_g >> chr_shift_right << chr_shift_left);
		setchr1(0x1C00, chr_bank_h >> chr_shift_right << chr_shift_left);
		break;
	}
}

static void COOLGIRL_Sync_Mirroring(void) {
	if (!four_screen)
	{
		if (!((mapper == 0b010100) && (flags & 1))) // Mapper #189?
			setmirror((mirroring < 2) ? (mirroring ^ 1) : mirroring);
	}
	else { // four screen mode
		vnapage[0] = UNIFchrrama + 0x3F000;
		vnapage[1] = UNIFchrrama + 0x3F400;
		vnapage[2] = UNIFchrrama + 0x3F800;
		vnapage[3] = UNIFchrrama + 0x3FC00;
	}
}

static void COOLGIRL_Sync(void) {
	COOLGIRL_Sync_PRG();
	COOLGIRL_Sync_CHR();
	COOLGIRL_Sync_Mirroring();
}

static DECLFW(COOLGIRL_Flash_Write) {
	if (flash_state < sizeof(flash_buffer_a) / sizeof(flash_buffer_a[0]))
	{
		flash_buffer_a[flash_state] = A & 0xFFF;
		flash_buffer_v[flash_state] = V;
		flash_state++;

		// enter CFI mode
		if ((flash_state == 1) &&
			(flash_buffer_a[0] == 0x0AAA) && (flash_buffer_v[0] == 0x98))
		{
			cfi_mode = 1;
			flash_state = 0;
		}

		// sector erase
		if ((flash_state == 6) &&
			(flash_buffer_a[0] == 0x0AAA) && (flash_buffer_v[0] == 0xAA) &&
			(flash_buffer_a[1] == 0x0555) && (flash_buffer_v[1] == 0x55) &&
			(flash_buffer_a[2] == 0x0AAA) && (flash_buffer_v[2] == 0x80) &&
			(flash_buffer_a[3] == 0x0AAA) && (flash_buffer_v[3] == 0xAA) &&
			(flash_buffer_a[4] == 0x0555) && (flash_buffer_v[4] == 0x55) &&
			(flash_buffer_v[5] == 0x30))
		{
			int sector = prg_bank_a_mapped * 0x2000 / FLASH_SECTOR_SIZE;
			uint32 sector_address = sector * FLASH_SECTOR_SIZE;
			for (uint32 i = sector_address; i < sector_address + FLASH_SECTOR_SIZE; i++)
				SAVE_FLASH[i % SAVE_FLASH_SIZE] = 0xFF;
			flash_state = 0;
		}

		// write byte
		if ((flash_state == 4) &&
			(flash_buffer_a[0] == 0x0AAA) && (flash_buffer_v[0] == 0xAA) &&
			(flash_buffer_a[1] == 0x0555) && (flash_buffer_v[1] == 0x55) &&
			(flash_buffer_a[2] == 0x0AAA) && (flash_buffer_v[2] == 0xA0))
		{
			//int sector = prg_bank_a_mapped * 0x2000 / FLASH_SECTOR_SIZE;
			uint32 flash_addr = prg_bank_a_mapped * 0x2000 + (A % 0x8000);
			if (SAVE_FLASH[flash_addr % SAVE_FLASH_SIZE] != 0xFF) {
				FCEU_PrintError("Error: can't write to 0x%08x, flash sector is not erased.\n", flash_addr);
			}
			else {
				SAVE_FLASH[flash_addr % SAVE_FLASH_SIZE] = V;
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

	COOLGIRL_Sync_PRG();
}

static DECLFW(COOLGIRL_WRITE) {
	if (sram_enabled && A >= 0x6000 && A < 0x8000 && !map_rom_on_6000)
		CartBW(A, V); // SRAM is enabled and writable
	if (SAVE_FLASH && can_write_flash && A >= 0x8000) // writing flash
		COOLGIRL_Flash_Write(A, V);

	// block two writes in a row
	if ((timestampbase + timestamp) < (lreset + 2))	return;
	lreset = timestampbase + timestamp;

	if (A >= 0x5000 && A < 0x6000 && !lockout)
	{
		switch (A & 7)
		{
		case 0:
			// {prg_base[26:22]} = cpu_data_in[4:0];
			// use bits 29-27 to simulate flash memory
			SET_BITS(prg_base, "29:22", V, "7:0");
			break;
		case 1:
			// prg_base[21:14] = cpu_data_in[7:0];
			SET_BITS(prg_base, "21:14", V, "7:0");
			break;
		case 2:
			// {chr_mask[18], prg_mask[20:14]} = cpu_data_in[7:0];
			SET_BITS(chr_mask, "18", V, "7");
			SET_BITS(prg_mask, "20:14", V, "6:0");
			break;
		case 3:
			// {prg_mode[2:0], chr_bank_a[7:3]} = cpu_data_in[7:0];
			SET_BITS(prg_mode, "2:0", V, "7:5");
			SET_BITS(chr_bank_a, "7:3", V, "4:0");
			break;
		case 4:
			// {chr_mode[2:0], chr_mask[17:13]} = cpu_data_in[7:0];
			SET_BITS(chr_mode, "2:0", V, "7:5");
			SET_BITS(chr_mask, "17:13", V, "4:0");
			break;
		case 5:
			// {chr_bank_a[8], prg_bank_a[5:1], sram_page[1:0]} = cpu_data_in[7:0];
			SET_BITS(chr_bank_a, "8", V, "7");
			SET_BITS(prg_bank_a, "5:1", V, "6:2");
			SET_BITS(sram_page, "1:0", V, "1:0");
			break;
		case 6:
			// {flags[2:0], mapper[4:0]} = cpu_data_in[7:0];
			SET_BITS(flags, "2:0", V, "7:5");
			SET_BITS(mapper, "4:0", V, "4:0");
			break;
		case 7:
			// {lockout, mapper[5], four_screen, mirroring[1:0], prg_write_enabled, chr_write_enabled, sram_enabled} = cpu_data_in[7:0];
			SET_BITS(lockout, "0", V, "7");
			SET_BITS(mapper, "5", V, "6");
			SET_BITS(four_screen, "0", V, "5");
			SET_BITS(mirroring, "1:0", V, "4:3");
			SET_BITS(can_write_flash, "0", V, "2");
			SET_BITS(can_write_chr, "0", V, "1");
			SET_BITS(sram_enabled, "0", V, "0");
			if (mapper == 0b010001) prg_bank_b = ~2; // if (USE_MAPPER_009_010 && mapper == 6'b010001) prg_bank_b = 8'b11111101;
			if (mapper == 0b010111) map_rom_on_6000 = 1; // if (ENABLE_MAPPER_042 && (mapper == 6'b010111)) map_rom_on_6000 <= 1;
			if (mapper == 0b001110) prg_bank_b = 1; // if (USE_MAPPER_065 && mapper == 6'b001110) prg_bank_b = 1;
			break;
		}
	}

	if (A < 0x8000) // $0000-$7FFF
	{
		// Mapper #163
		if (mapper == 0b000110)
		{
			if (A == 0x5101) // if (cpu_addr_in[14:0] == 15'h5101)
			{
				if (mapper163_r4 && !V) // if ((mapper163_r4 != 0) && (cpu_data_in == 0))
					mapper163_r5 ^= 1; // mapper163_r5[0] = ~mapper163_r5[0];
				mapper163_r4 = V;
			}
			else if (A == 0x5100 && V == 6) // if ((cpu_addr_in[14:0] == 15'h5100) && (cpu_data_in == 6))
			{
				SET_BITS(prg_mode, "0", 0, "0"); // prg_mode[0] = 0;
				prg_bank_b = 0b1100; // prb_bank_b = 4'b1100;
			}
			else {
				if (get_bits(A, "14:12") == 0b101) // if (cpu_addr_in[14:12] == 3'b101) begin
				{
					switch (get_bits(A, "9:8")) // case (cpu_addr_in[9:8])
					{
					case 2:
						SET_BITS(prg_mode, "0", 1, "0"); // prg_mode[0] = 1;
						SET_BITS(prg_bank_a, "7:6", V, "1:0"); // prg_bank_a[7:6] = cpu_data_in[1:0];
						mapper163_r0 = V;
						break;
					case 0:
						SET_BITS(prg_mode, "0", 1, "0"); // prg_mode[0] = 1;
						SET_BITS(prg_bank_a, "5:2", V, "3:0"); // prg_bank_a[5:2] = cpu_data_in[3:0];
						SET_BITS(chr_mode, "0", V, "7"); // chr_mode[0] = cpu_data_in[7];
						mapper163_r1 = V;
						break;
					case 3:
						mapper163_r2 = V; // mapper163_r2 = cpu_data_in;
						break;
					case 1:
						mapper163_r3 = V; // mapper163_r3 = cpu_data_in;
						break;
					}
				}
			}
		}

		// Mapper #87
		if (mapper == 0b001100)
		{
			if (get_bits(A, "14:13") == 0b11) // if (cpu_addr_in[14:13] == 2'b11) // $6000-$7FFF
			{
				// chr_bank_a[4:3] = {cpu_data_in[0], cpu_data_in[1]};
				SET_BITS(chr_bank_a, "4:3", V, "0,1");
			}
		}

		// Mapper #90 - JY
		/*
		if (mapper == 0b001101)
		{
			switch (A)
			{
			case 0x5800: mul1 = V; break;
			case 0x5801: mul2 = V; break;
			}
		}
		*/

		// MMC5 (not really)
		if (mapper == 0b001111)
		{
			// case (cpu_addr_in[14:0])
			switch (get_bits(A, "14:0"))
			{
			case 0x5105:
				if (V == 0xFF)
				{
					four_screen = 1;
				}
				else {
					four_screen = 0;
					// case ({cpu_data_in[4], cpu_data_in[2]})
					switch (get_bits(V, "4,2"))
					{
					case 0b00: // 2'b00: mirroring = 2'b10;
						mirroring = 0b10; break;
					case 0b01: // 2'b01: mirroring = 2'b00;
						mirroring = 0b00; break;
					case 0b10: // 2'b10: mirroring = 2'b01;
						mirroring = 0b01; break;
					case 0b11: // 2'b11: mirroring = 2'b11;
						mirroring = 0b11; break;
					}
				}
				break;
			case 0x5115:
				// prg_bank_a[4:0] = { cpu_data_in[4:1], 1'b0};
				SET_BITS(prg_bank_a, "4:1", V, "4:1");
				SET_BITS(prg_bank_a, "0", 0, "0");
				// prg_bank_b[4:0] = { cpu_data_in[4:1], 1'b1};
				SET_BITS(prg_bank_b, "4:1", V, "4:1");
				SET_BITS(prg_bank_b, "0", 1, "0");
				break;
			case 0x5116:
				// prg_bank_c[4:0] = cpu_data_in[4:0];
				SET_BITS(prg_bank_c, "4:0", V, "4:0");
				break;
			case 0x5117:
				// prg_bank_d[4:0] = cpu_data_in[4:0];
				SET_BITS(prg_bank_d, "4:0", V, "4:0");
				break;
			case 0x5120:
				// chr_bank_a[7:0] = cpu_data_in[7:0];
				SET_BITS(chr_bank_a, "7:0", V, "7:0");
				break;
			case 0x5121:
				// chr_bank_b[7:0] = cpu_data_in[7:0];
				SET_BITS(chr_bank_b, "7:0", V, "7:0");
				break;
			case 0x5122:
				// chr_bank_c[7:0] = cpu_data_in[7:0];
				SET_BITS(chr_bank_c, "7:0", V, "7:0");
				break;
			case 0x5123:
				// chr_bank_d[7:0] = cpu_data_in[7:0];
				SET_BITS(chr_bank_d, "7:0", V, "7:0");
				break;
			case 0x5128:
				// chr_bank_e[7:0] = cpu_data_in[7:0];
				SET_BITS(chr_bank_e, "7:0", V, "7:0");
				break;
			case 0x5129:
				// chr_bank_f[7:0] = cpu_data_in[7:0];
				SET_BITS(chr_bank_f, "7:0", V, "7:0");
				break;
			case 0x512A:
				// chr_bank_g[7:0] = cpu_data_in[7:0];
				SET_BITS(chr_bank_g, "7:0", V, "7:0");
				break;
			case 0x512B:
				// chr_bank_h[7:0] = cpu_data_in[7:0];
				SET_BITS(chr_bank_h, "7:0", V, "7:0");
				break;
			case 0x5203:
				// mmc5_irq_ack = 1;
				X6502_IRQEnd(FCEU_IQEXT);
				mmc5_irq_out = 0;
				// mmc5_irq_line[7:0] = cpu_data_in[7:0];
				SET_BITS(mmc5_irq_line, "7:0", V, "7:0");
				break;
			case 0x5204:
				// mmc5_irq_ack = 1;
				X6502_IRQEnd(FCEU_IQEXT);
				mmc5_irq_out = 0;
				// mmc5_irq_enabled = cpu_data_in[7];
				mmc5_irq_enabled = get_bits(V, "7");
				break;
			}
		}

		// Mapper #189
		if ((mapper == 0b010100) && (flags & 2))
		{
			if (A >= 0x4120) // if (cpu_addr_in[14:0] >= 15'h4120) // $4120-$7FFF
			{
				// prg_bank_a[5:2] = cpu_data_in[3:0] | cpu_data_in[7:4];
				prg_bank_a = set_bits(prg_bank_a, "5:2",
					get_bits(V, "7:4") | get_bits(V, "3:0"));
			}
		}

		// Mappers #79 and #146 - NINA-03/06 and Sachen 3015: (flag0 = 1)
		if (mapper == 0b011011)
		{
			// if ({cpu_addr_in[14:13], cpu_addr_in[8]} == 3'b101)
			if (get_bits(A, "14:13,8") == 0b101)
			{
				//chr_bank_a[5:3] = cpu_data_in[2:0];
				SET_BITS(chr_bank_a, "5:3", V, "2:0");
				//prg_bank_a[2] = cpu_data_in[3];
				SET_BITS(prg_bank_a, "2", V, "3");
			}
		}

		// Mapper #133
		if (mapper == 0b011100)
		{
			// if ({cpu_addr_in[14:13], cpu_addr_in[8]} == 3'b101)
			if (get_bits(A, "14:13,8") == 0b101)
			{
				//chr_bank_a[4:3] = cpu_data_in[1:0];
				SET_BITS(chr_bank_a, "4:3", V, "1:0");
				//prg_bank_a[2] = cpu_data_in[2];
				SET_BITS(prg_bank_a, "2", V, "2");
			}
		}

		// Mapper #184
		if (mapper == 0b011111)
		{
			if (get_bits(A, "14:13") == 0b11) // if (cpu_addr_in[14:13] == 2'b11)
			{
				// chr_bank_a[4:2] = cpu_data_in[2:0];
				SET_BITS(chr_bank_a, "4:2", V, "2:0");
				// chr_bank_e[4:2] = {1'b1, cpu_data_in[5:4]};
				SET_BITS(chr_bank_e, "3:2", V, "5:4");
				SET_BITS(chr_bank_e, "4", 1, "0");
			}
		}

		// Mapper #38
		if (mapper == 0b100000)
		{
			// if (cpu_addr_in[14:12] == 3'b111)
			if (get_bits(A, "14:12") == 0b111)
			{
				// prg_bank_a[3:2] = cpu_data_in[1:0];
				SET_BITS(prg_bank_a, "3:2", V, "1:0");
				// chr_bank_a[4:3] = cpu_data_in[3:2];
				SET_BITS(chr_bank_a, "4:3", V, "3:2");
			}
		}
	}
	else // $8000-$FFFF
	{
		// Mapper #2 - UxROM
		// flag0 - mapper #71 - for Fire Hawk only.
		// other mapper-#71 games are UxROM
		if (mapper == 0b000001)
		{
			// if (!ENABLE_MAPPER_071 | ~flags[0] | (cpu_addr_in[14:12] != 3'b001))
			if (!(flags & 1) || (get_bits(A, "14:12") != 0b001))
			{
				// prg_bank_a[UxROM_BITSIZE+1:1] = cpu_data_in[UxROM_BITSIZE:0];
				// UxROM_BITSIZE = 4
				SET_BITS(prg_bank_a, "5:1", V, "4:0");
				// if (ENABLE_MAPPER_030 && flags[1])
				if (flags & 2)
				{
					// One screen mirroring select, CHR RAM bank, PRG ROM bank
					// mirroring[1:0] = { 1'b1, cpu_data_in[7]};
					SET_BITS(mirroring, "1", 1, "0");
					SET_BITS(mirroring, "0", V, "7");
					// chr_bank_a[1:0] = cpu_data_in[6:5];
					SET_BITS(chr_bank_a, "1:0", V, "6:5");
				}
			}
			else {
				// CodeMasters, blah. Mirroring control used only by Fire Hawk
				// mirroring[1:0] = {1'b1, cpu_data_in[4]};
				SET_BITS(mirroring, "1", 1, "0");
				SET_BITS(mirroring, "0", V, "4");
			}
		}

		// Mapper #3 - CNROM
		if (mapper == 0b000010)
		{
			// chr_bank_a[7:3] = cpu_data_in[4:0];
			SET_BITS(chr_bank_a, "7:3", V, "4:0");
		}

		// Mapper #78 - Holy Diver
		if (mapper == 0b000011)
		{
			// prg_bank_a[3:1] = cpu_data_in[2:0];
			SET_BITS(prg_bank_a, "3:1", V, "2:0");
			// chr_bank_a[6:3] = cpu_data_in[7:4];
			SET_BITS(chr_bank_a, "6:3", V, "7:4");
			// mirroring = { 1'b0, ~cpu_data_in[3]};
			mirroring = get_bits(V, "3") ^ 1;
		}

		// Mapper #97 - Irem's TAM-S1
		if (mapper == 0b000100)
		{
			// prg_bank_a[5:1] = cpu_data_in[4:0];
			SET_BITS(prg_bank_a, "5:1", V, "4:0");
			// mirroring = { 1'b0, ~cpu_data_in[7]};
			mirroring = get_bits(V, "7") ^ 1;
		}

		// Mapper #93 - Sunsoft-2
		if (mapper == 0b000101)
		{
			// prg_bank_a[3:1] = { cpu_data_in[6:4] };
			SET_BITS(prg_bank_a, "3:1", V, "6:4");
			// chr_write_enabled = cpu_data_in[0];
			can_write_chr = V & 1;
		}

		// Mapper #18
		if (mapper == 0b000111)
		{
			// case ({cpu_addr_in[14:12], cpu_addr_in[1:0]})
			switch (get_bits(A, "14:12,1:0"))
			{
			case 0b00000: // 5'b00000: prg_bank_a[3:0] = cpu_data_in[3:0]; // $8000
				SET_BITS(prg_bank_a, "3:0", V, "3:0"); break;
			case 0b00001: // 5'b00001: prg_bank_a[7:4] = cpu_data_in[3:0]; // $8001
				SET_BITS(prg_bank_a, "7:4", V, "3:0"); break;
			case 0b00010: // 5'b00010: prg_bank_b[3:0] = cpu_data_in[3:0]; // $8002
				SET_BITS(prg_bank_b, "3:0", V, "3:0"); break;
			case 0b00011: // 5'b00011: prg_bank_b[7:4] = cpu_data_in[3:0]; // $8003
				SET_BITS(prg_bank_b, "7:4", V, "3:0"); break;
			case 0b00100: // 5'b00100: prg_bank_c[3:0] = cpu_data_in[3:0]; // $9000
				SET_BITS(prg_bank_c, "3:0", V, "3:0"); break;
			case 0b00101: // 5'b00101: prg_bank_c[7:4] = cpu_data_in[3:0]; // $9001
				SET_BITS(prg_bank_c, "7:4", V, "3:0"); break;
			case 0b00110:
				break;
			case 0b00111:
				break;
			case 0b01000: // 5'b01000: chr_bank_a[3:0] = cpu_data_in[3:0]; // $A000
				SET_BITS(chr_bank_a, "3:0", V, "3:0"); break;
			case 0b01001: // 5'b01001: chr_bank_a[7:4] = cpu_data_in[3:0]; // $A001
				SET_BITS(chr_bank_a, "7:4", V, "3:0"); break;
			case 0b01010: // 5'b01010: chr_bank_b[3:0] = cpu_data_in[3:0]; // $A002
				SET_BITS(chr_bank_b, "3:0", V, "3:0"); break;
			case 0b01011: // 5'b01011: chr_bank_b[7:4] = cpu_data_in[3:0]; // $A003
				SET_BITS(chr_bank_b, "7:4", V, "3:0"); break;
			case 0b01100: // 5'b01100: chr_bank_c[3:0] = cpu_data_in[3:0]; // $B000
				SET_BITS(chr_bank_c, "3:0", V, "3:0"); break;
			case 0b01101: // 5'b01101: chr_bank_c[7:4] = cpu_data_in[3:0]; // $B001
				SET_BITS(chr_bank_c, "7:4", V, "3:0"); break;
			case 0b01110: // 5'b01110: chr_bank_d[3:0] = cpu_data_in[3:0]; // $B002
				SET_BITS(chr_bank_d, "3:0", V, "3:0"); break;
			case 0b01111: // 5'b01111: chr_bank_d[7:4] = cpu_data_in[3:0]; // $B003
				SET_BITS(chr_bank_d, "7:4", V, "3:0"); break;
			case 0b10000: // 5'b10000: chr_bank_e[3:0] = cpu_data_in[3:0]; // $C000
				SET_BITS(chr_bank_e, "3:0", V, "3:0"); break;
			case 0b10001: // 5'b10001: chr_bank_e[7:4] = cpu_data_in[3:0]; // $C001
				SET_BITS(chr_bank_e, "7:4", V, "3:0"); break;
			case 0b10010: // 5'b10010: chr_bank_f[3:0] = cpu_data_in[3:0]; // $C002
				SET_BITS(chr_bank_f, "3:0", V, "3:0"); break;
			case 0b10011: // 5'b10011: chr_bank_f[7:4] = cpu_data_in[3:0]; // $C003
				SET_BITS(chr_bank_f, "7:4", V, "3:0"); break;
			case 0b10100: // 5'b10100: chr_bank_g[3:0] = cpu_data_in[3:0]; // $D000
				SET_BITS(chr_bank_g, "3:0", V, "3:0"); break;
			case 0b10101: // 5'b10101: chr_bank_g[7:4] = cpu_data_in[3:0]; // $D001
				SET_BITS(chr_bank_g, "7:4", V, "3:0"); break;
			case 0b10110: // 5'b10110: chr_bank_h[3:0] = cpu_data_in[3:0]; // $D002
				SET_BITS(chr_bank_h, "3:0", V, "3:0"); break;
			case 0b10111: // 5'b10111: chr_bank_h[7:4] = cpu_data_in[3:0]; // $D003
				SET_BITS(chr_bank_h, "7:4", V, "3:0"); break;
			case 0b11000: // 5'b11000: mapper18_irq_latch[3:0] = cpu_data_in[3:0]; // $E000
				SET_BITS(mapper18_irq_latch, "3:0", V, "3:0"); break;
			case 0b11001: // 5'b11001: mapper18_irq_latch[7:4] = cpu_data_in[3:0]; // $E001
				SET_BITS(mapper18_irq_latch, "7:4", V, "3:0"); break;
			case 0b11010: // 5'b11010: mapper18_irq_latch[11:8] = cpu_data_in[3:0]; // $E002
				SET_BITS(mapper18_irq_latch, "11:8", V, "3:0"); break;
			case 0b11011: // 5'b11011: mapper18_irq_latch[15:12] = cpu_data_in[3:0]; // $E003
				SET_BITS(mapper18_irq_latch, "15:12", V, "3:0"); break;
			case 0b11100: // 5'b11100: begin // $F000
				// mapper18_irq_out = 0; // ack
				X6502_IRQEnd(FCEU_IQEXT);
				// mapper18_irq_value[15:0] = mapper18_irq_latch[15:0];
				mapper18_irq_value = mapper18_irq_latch; break; // irq_cpu_out = 0;
			case 0b11101: // 5'b11101: begin // $F001
				X6502_IRQEnd(FCEU_IQEXT); // irq_cpu_control[3:0] = cpu_data_in[3:0];
				SET_BITS(mapper18_irq_control, "3:0", V, "3:0"); break;
			case 0b11110: // 5'b11110
				switch (get_bits(V, "1:0")) // case (cpu_data_in[1:0])
				{
				case 0b00: mirroring = 0b01; break; //2'b00: mirroring = 2'b01; // Horz
				case 0b01: mirroring = 0b00; break;	//2'b01: mirroring = 2'b00; // Vert
				case 0b10: mirroring = 0b10; break;	//2'b10: mirroring = 2'b10; // 1SsA
				case 0b11: mirroring = 0b11; break;	//2'b11: mirroring = 2'b11; // 1SsB
				}
			case 0b11111:
				break; // sound
			}
		}

		// Mapper #7 - AxROM, mapper #241 - BNROM
		if (mapper == 0b001000)
		{
			// AxROM_BxROM_BITSIZE = 3
			//prg_bank_a[AxROM_BxROM_BITSIZE + 2:2] = cpu_data_in[AxROM_BxROM_BITSIZE:0];
			SET_BITS(prg_bank_a, "5:2", V, "3:0");
			//if (!ENABLE_MAPPER_034_241_BxROM || !flags[0]) // BxROM?
			//	mirroring = { 1'b1, cpu_data_in[4]};
			if (!(flags & 1))
				mirroring = (1 << 1) | get_bits(V, "4");
		}

		// Mapper #228 - Cheetahmen II				
		if (mapper == 0b001001)
		{
			// prg_bank_a[5:2] = cpu_addr_in[10:7];
			SET_BITS(prg_bank_a, "5:2", A, "10:7");
			// chr_bank_a[7:3] = { cpu_addr_in[2:0], cpu_data_in[1:0] }; // only 256k, sorry
			SET_BITS(chr_bank_a, "7:5", A, "2:0");
			SET_BITS(chr_bank_a, "4:3", V, "1:0");
			// mirroring = { 1'b0, cpu_addr_in[13]};
			mirroring = get_bits(A, "13");
		}

		// Mapper #11 - ColorDreams
		if (mapper == 0b001010)
		{
			// prg_bank_a[3:2] = cpu_data_in[1:0];
			SET_BITS(prg_bank_a, "3:2", V, "1:0");
			// chr_bank_a[6:3] = cpu_data_in[7:4];
			SET_BITS(chr_bank_a, "6:3", V, "7:4");
		}

		// Mapper #66 - GxROM
		if (mapper == 0b001011)
		{
			// prg_bank_a[3:2] = cpu_data_in[5:4];
			SET_BITS(prg_bank_a, "3:2", V, "5:4");
			// chr_bank_a[4:3] = cpu_data_in[1:0];
			SET_BITS(chr_bank_a, "4:3", V, "1:0");
		}

		// Mapper #90 - JY
		if (mapper == 0b001101)
		{
			// if (cpu_addr_in[14:12] == 3'b000) // $800x
			if (get_bits(A, "14:12") == 0b000)
			{
				// case (cpu_addr_in[1:0])
				switch (get_bits(A, "1:0"))
				{
					// 2'b00: prg_bank_a[5:0] = cpu_data_in[5:0];
				case 0b00: SET_BITS(prg_bank_a, "5:0", V, "5:0"); break;
					// 2'b01: prg_bank_b[5:0] = cpu_data_in[5:0];
				case 0b01: SET_BITS(prg_bank_b, "5:0", V, "5:0"); break;
					// 2'b10: prg_bank_c[5:0] = cpu_data_in[5:0];
				case 0b10: SET_BITS(prg_bank_c, "5:0", V, "5:0"); break;
					// 2'b11: prg_bank_d[5:0] = cpu_data_in[5:0];
				case 0b11: SET_BITS(prg_bank_d, "5:0", V, "5:0"); break;
				}
			}

			// if (cpu_addr_in[14:12] == 3'b001) // $900x
			if (get_bits(A, "14:12") == 0b001)
			{
				// case (cpu_addr_in[2:0])
				switch (get_bits(A, "2:0"))
				{
				case 0b000: // 3'b000: chr_bank_a[7:0] = cpu_data_in[7:0]; // $9000
					SET_BITS(chr_bank_a, "7:0", V, "7:0"); break;
				case 0b001: // 3'b001: chr_bank_b[7:0] = cpu_data_in[7:0]; // $9001
					SET_BITS(chr_bank_b, "7:0", V, "7:0"); break;
				case 0b010: // 3'b010: chr_bank_c[7:0] = cpu_data_in[7:0]; // $9002
					SET_BITS(chr_bank_c, "7:0", V, "7:0"); break;
				case 0b011: // 3'b011: chr_bank_d[7:0] = cpu_data_in[7:0]; // $9003
					SET_BITS(chr_bank_d, "7:0", V, "7:0"); break;
				case 0b100: // 3'b100: chr_bank_e[7:0] = cpu_data_in[7:0]; // $9004
					SET_BITS(chr_bank_e, "7:0", V, "7:0"); break;
				case 0b101: // 3'b101: chr_bank_f[7:0] = cpu_data_in[7:0]; // $9005
					SET_BITS(chr_bank_f, "7:0", V, "7:0"); break;
				case 0b110: // 3'b110: chr_bank_g[7:0] = cpu_data_in[7:0]; // $9006
					SET_BITS(chr_bank_g, "7:0", V, "7:0"); break;
				case 0b111: // 3'b111: chr_bank_h[7:0] = cpu_data_in[7:0]; // $9007
					SET_BITS(chr_bank_h, "7:0", V, "7:0"); break;
				}
			}

			// if ({cpu_addr_in[14:12], cpu_addr_in[1:0]} == 5'b10101) // $D001
			if (get_bits(A, "14:12,1:0") == 0b10101)
			{
				// mirroring = cpu_data_in[1:0];
				SET_BITS(mirroring, "1:0", V, "1:0");
			}

			// use MMC3's IRQs
			// if (cpu_addr_in[14:12] == 3'b100) // $C00x
			if (get_bits(A, "14:12") == 0b100)
			{
				// case (cpu_addr_in[2:0])
				switch (get_bits(A, "2:0"))
				{
				case 0b000:
					// 3'b000: mmc3_irq_enabled = cpu_data_in[0];
					if (V & 1)
					{
						mmc3_irq_enabled = 1;
					}
					else {
						X6502_IRQEnd(FCEU_IQEXT);
						mmc3_irq_enabled = 0;
					}
					break;
				case 0b001:
					break; // who cares about this shit?
				case 0b010:
					// 3'b010: mmc3_irq_enabled = 0;
					mmc3_irq_enabled = 0;
					X6502_IRQEnd(FCEU_IQEXT);
					break;
				case 0b011:
					// 3'b011: mmc3_irq_enabled = 1;
					mmc3_irq_enabled = 1;
					break;
				case 0b100:
					break; // prescaler? who cares?
				case 0b101:
					// mmc3_irq_latch = cpu_data_in ^ mapper90_xor;
					mmc3_irq_latch = V ^ mapper90_xor;
					mmc3_irq_reload = 1;
					break;
				case 0b110:
					// mapper90_xor = cpu_data_in;
					mapper90_xor = V;
					break;
				case 0b111:
					break; // meh
				}
			}
		}

		// Mapper #65 - Irem's H3001
		if (mapper == 0b001110)
		{
			// case ({cpu_addr_in[14:12], cpu_addr_in[2:0]})
			switch (get_bits(A, "14:12,2:0"))
			{
			case 0b000000:
				// 6'b000000: prg_bank_a[5:0] = cpu_data_in[5:0]; // $8000
				SET_BITS(prg_bank_a, "5:0", V, "5:0");
				break;
			case 0b001001:
				// 6'b001001: mirroring = {1'b0, cpu_data_in[7]}; // $9001, mirroring
				mirroring = get_bits(V, "7");
				break;
			case 0b001011:
				// mapper65_irq_out = 0; // ack
				X6502_IRQEnd(FCEU_IQEXT); // irq_cpu_out = 0;
				// mapper65_irq_enabled = cpu_data_in[7]; // $9003, enable IRQ
				mapper65_irq_enabled = get_bits(V, "7");
				break;
			case 0b001100:
				X6502_IRQEnd(FCEU_IQEXT); // mapper65_irq_out = 0; // ack
				mapper65_irq_value = mapper65_irq_latch; // $9004, IRQ reload
				break;
			case 0b001101: // mapper65_irq_latch[15:8] = cpu_data_in; // $9005, IRQ high value
				SET_BITS(mapper65_irq_latch, "15:8", V, "7:0"); break;
			case 0b001110: // mapper65_irq_latch[7:0] = cpu_data_in; // $9006, IRQ low value
				SET_BITS(mapper65_irq_latch, "7:0", V, "7:0"); break;
			case 0b010000: // prg_bank_b[5:0] = cpu_data_in[5:0]; // $A000
				prg_bank_b = (prg_bank_b & 0b11000000) | (V & 0b00111111); break;
			case 0b011000: // chr_bank_a[7:0] = cpu_data_in; // $B000
				SET_BITS(chr_bank_a, "7:0", V, "7:0"); break;
			case 0b011001: // chr_bank_b[7:0] = cpu_data_in[7:0]; // $B001
				SET_BITS(chr_bank_b, "7:0", V, "7:0"); break;
			case 0b011010: // chr_bank_c[7:0] = cpu_data_in[7:0]; // $B002
				SET_BITS(chr_bank_c, "7:0", V, "7:0"); break;
			case 0b011011: // chr_bank_d[7:0] = cpu_data_in[7:0]; // $B003
				SET_BITS(chr_bank_d, "7:0", V, "7:0"); break;
			case 0b011100: // chr_bank_e[7:0] = cpu_data_in[7:0]; // $B004
				SET_BITS(chr_bank_e, "7:0", V, "7:0"); break;
			case 0b011101: // chr_bank_f[7:0] = cpu_data_in[7:0]; // $B005
				SET_BITS(chr_bank_f, "7:0", V, "7:0"); break;
			case 0b011110: // chr_bank_g[7:0] = cpu_data_in[7:0]; // $B006
				SET_BITS(chr_bank_g, "7:0", V, "7:0"); break;
			case 0b011111: // chr_bank_h[7:0] = cpu_data_in[7:0]; // $B007
				SET_BITS(chr_bank_h, "7:0", V, "7:0"); break;
			case 0b100000: // 6'b100000: prg_bank_c[5:0] = cpu_data_in[5:0]; // $C000
				SET_BITS(prg_bank_c, "5:0", V, "5:0"); break;
			}
		}

		// Mapper #1 - MMC1
		/*
		flag0 - 16KB of SRAM (SOROM)
		*/
		if (mapper == 0b010000)
		{
			if (V & 0x80) // reset
			{
				mmc1_load_register = set_bits(mmc1_load_register, "5:0", 0b100000); // mmc1_load_register[5:0] = 6'b100000;
				prg_mode = 0; // 0x4000 (A) + fixed last (C)
				prg_bank_c = set_bits(prg_bank_c, "4:0", 0b11110); // prg_bank_c[4:0] = 5'b11110;
			}
			else {
				// mmc1_load_register[5:0] = { cpu_data_in[0], mmc1_load_register[5:1] };
				SET_BITS(mmc1_load_register, "4:0", mmc1_load_register, "5:1");
				SET_BITS(mmc1_load_register, "5", V, "0");
				// if (mmc1_load_register[0] == 1)
				if (mmc1_load_register & 1)
				{
					switch ((A >> 13) & 3)
					{
					case 0b00: // 2'b00: begin // $8000-$9FFF
						if (get_bits(mmc1_load_register, "4:3") == 0b11) // if (mmc1_load_register[4:3] == 2'b11)
						{
							prg_mode = 0; // prg_mode = 3'b000;   // 0x4000 (A) + fixed last (C)
							prg_bank_c = set_bits(prg_bank_c, "4:1", 0b1111); // prg_bank_c[4:1] = 4'b1111;
						}
						//else if (mmc1_load_register[4:3] == 2'b10)
						else if (get_bits(mmc1_load_register, "4:3") == 0b10)
						{
							prg_mode = 0b001; // prg_mode = 3'b001;   // fixed first (C) + 0x4000 (A)
							prg_bank_c = set_bits(prg_bank_c, "4:1", 0b0000); // prg_bank_c[4:0] = 4'b0000;
						}
						else
							prg_mode = 0b111; // prg_mode = 3'b111;   // 0x8000 (A)
						if (get_bits(mmc1_load_register, "5"))
							chr_mode = 0b100;
						else
							chr_mode = 0b000;
						mirroring = set_bits(mirroring, "1:0", get_bits(mmc1_load_register, "2:1") ^ 0b10);
						break;
					case 0b01: // 2'b01
						SET_BITS(chr_bank_a, "6:2", mmc1_load_register, "5:1"); // chr_bank_a[6:2] = mmc1_load_register[5:1];
						if (flags & 1) // (flags[0]) - 16KB of SRAM
						{
							// PRG RAM page #2 is battery backed
							sram_page = 2 | get_bits(mmc1_load_register, "4") ^ 1; // sram_page <= {1'b1, ~mmc1_load_register[4]};
						}
						SET_BITS(prg_bank_a, "5", mmc1_load_register, "5"); // prg_bank_a[5] = mmc1_load_register[5]; // for SUROM, 512k PRG support
						SET_BITS(prg_bank_c, "5", mmc1_load_register, "5"); // prg_bank_c[5] = mmc1_load_register[5]; // for SUROM, 512k PRG support
						break;
					case 0b10: // 2'b10: chr_bank_e[6:2] = mmc1_load_register[5:1]; // $C000-$DFFF
						SET_BITS(chr_bank_e, "6:2", mmc1_load_register, "5:1");
						break;
					case 0b11: // 2'b11
						// prg_bank_a[4:1] = mmc1_load_register[4:1];
						SET_BITS(prg_bank_a, "4:1", mmc1_load_register, "4:1");
						// sram_enabled = ~mmc1_load_register[5];
						sram_enabled = get_bits(mmc1_load_register, "5") ^ 1;
						break;
					}
					mmc1_load_register = 0b100000; // mmc1_load_register[5:0] = 6'b100000;
				}
			}
		}

		// Mapper #9 and #10 - MMC2 and MMC4
		// flag0 - 0=MMC2, 1=MMC4
		if (mapper == 0b010001)
		{
			switch ((A >> 12) & 7)
			{
			case 2:  // $A000-$AFFF
				if (!(flags & 1)) {
					// MMC2
					SET_BITS(prg_bank_a, "3:0", V, "3:0");
				}
				else {
					// MMC4
					SET_BITS(prg_bank_a, "4:1", V, "3:0"); // prg_bank_a[4:0] = { cpu_data_in[3:0], 1'b0};
					prg_bank_a &= ~1;
				}
				break;
			case 3: // $B000-$BFFF
				SET_BITS(chr_bank_a, "6:2", V, "4:0"); // chr_bank_a[6:2] = cpu_data_in[4:0];
				break;
			case 4: // $C000-$CFFF
				SET_BITS(chr_bank_b, "6:2", V, "4:0"); // chr_bank_b[6:2] = cpu_data_in[4:0];
				break;
			case 5: // $D000-$DFFF
				SET_BITS(chr_bank_e, "6:2", V, "4:0"); // chr_bank_e[6:2] = cpu_data_in[4:0];
				break;
			case 6: // $E000-$EFFF
				SET_BITS(chr_bank_f, "6:2", V, "4:0"); // chr_bank_f[6:2] = cpu_data_in[4:0];
				break;
			case 7: // $F000-$FFFF
				mirroring = V & 1;
				break;
			}
		}

		// Mapper #152
		if (mapper == 0b010010)
		{
			SET_BITS(chr_bank_a, "6:3", V, "3:0"); // chr_bank_a[6:3] = cpu_data_in[3:0];
			SET_BITS(prg_bank_a, "3:1", V, "6:4"); // prg_bank_a[3:1] = cpu_data_in[6:4];
			if (flags & 1)
				mirroring = 2 | get_bits(V, "7"); // mirroring = {1'b1, cpu_data_in[7]}; // Mapper #152
			else
				SET_BITS(prg_bank_a, "4", V, "7"); //prg_bank_a[4] <= cpu_data_in[7]; // Mapper #70
		}

		// Mapper #73 - VRC3
		if (mapper == 0b010011)
		{
			switch (get_bits(A, "14:12")) // case (cpu_addr_in[14:12])
			{
			case 0b000: // 3'b000: vrc3_irq_latch[3:0] = cpu_data_in[3:0]; // $8000-$8FFF
				SET_BITS(vrc3_irq_latch, "3:0", V, "3:0");
				break;
			case 0b001: // 3'b001: vrc3_irq_latch[7:4] = cpu_data_in[3:0]; // $9000-$9FFF
				SET_BITS(vrc3_irq_latch, "7:4", V, "3:0");
				break;
			case 0b010: // 3'b010: vrc3_irq_latch[11:8] = cpu_data_in[3:0]; // $A000-$AFFF
				SET_BITS(vrc3_irq_latch, "11:8", V, "3:0");
				break;
			case 0b011: // 3'b011: vrc3_irq_latch[15:12] = cpu_data_in[3:0]; // $B000-$BFFF
				SET_BITS(vrc3_irq_latch, "15:12", V, "3:0");
				break;
			case 0b100: // // $C000-$CFFF
				X6502_IRQEnd(FCEU_IQEXT); // vrc3_irq_out = 0; // ack
				SET_BITS(vrc3_irq_control, "2:0", V, "2:0"); // vrc3_irq_control[2:0] = cpu_data_in[2:0]; // mode, enabled, enabled after ack
				if (vrc3_irq_control & 2) // if (vrc3_irq_control[1]) // if E is set
					vrc3_irq_value = vrc3_irq_latch; // vrc3_irq_value[15:0] = vrc3_irq_latch[15:0]; // reload with latch
				break;
			case 0b101: // // $D000-$DFFF
				X6502_IRQEnd(FCEU_IQEXT); // vrc3_irq_out = 0; // ack
				SET_BITS(vrc3_irq_control, "1", vrc3_irq_control, "0"); // vrc3_irq_control[1] = vrc3_irq_control[0];
				break;
			case 0b110: // $E000-$EFFF
				break;
			case 0b111: // 3'b111: prg_bank_a[3:1] = cpu_data_in[2:0]; // $F000-$FFFF
				SET_BITS(prg_bank_a, "3:1", V, "2:0");
				break;
			}
		}

		// Mapper #4 - MMC3/MMC6
		/*
		flag0 - TxSROM
		flag1 - mapper #189
		*/
		if (mapper == 0b010100)
		{
			// case ({cpu_addr_in[14:13], cpu_addr_in[0]})
			switch (get_bits(A, "14:13,0"))
			{
			case 0b000: // $8000-$9FFE, even
				SET_BITS(mmc3_internal, "2:0", V, "2:0"); // mmc3_internal[2:0] = cpu_data_in[2:0];
				if (!(flags & 2) && !(flags & 4)) // if ((!USE_MAPPER_189 | ~flags[1]) & (!USE_MAPPER_206 | ~flags[2]))
				{
					if (get_bits(V, "6")) // if (cpu_data_in[6])
						prg_mode = 0b101;
					else
						prg_mode = 0b100;
				}
				if (!(flags & 4)) // if (!USE_MAPPER_206 | ~flags[2]) // disabled for mapper #206
				{
					if (V & 0x80) // if (cpu_data_in[7])
						chr_mode = 0b011;
					else
						chr_mode = 0b010;
				}
				break;
			case 0b001: // $8001-$9FFF, odd
				switch (get_bits(mmc3_internal, "2:0"))
				{
				case 0b000: // 3'b000: chr_bank_a[7:0] = cpu_data_in[7:0];
					SET_BITS(chr_bank_a, "7:0", V, "7:0"); break;
				case 0b001: // 3'b001: chr_bank_c[7:0] = cpu_data_in[7:0];
					SET_BITS(chr_bank_c, "7:0", V, "7:0"); break;
				case 0b010: // 3'b010: chr_bank_e[7:0] = cpu_data_in[7:0];
					SET_BITS(chr_bank_e, "7:0", V, "7:0"); break;
				case 0b011: // 3'b011: chr_bank_f[7:0] = cpu_data_in[7:0];
					SET_BITS(chr_bank_f, "7:0", V, "7:0"); break;
				case 0b100: // 3'b100: chr_bank_g[7:0] = cpu_data_in[7:0];
					SET_BITS(chr_bank_g, "7:0", V, "7:0"); break;
				case 0b101: // 3'b101: chr_bank_h[7:0] = cpu_data_in[7:0];
					SET_BITS(chr_bank_h, "7:0", V, "7:0"); break;
				case 0b110: // 3'b110: if (!ENABLE_MAPPER_189 | ~flags[1]) prg_bank_a[(MMC3_BITSIZE-1):0] = cpu_data_in[(MMC3_BITSIZE-1):0];
					if (!(flags & 2))
						SET_BITS(prg_bank_a, "7:0", V, "7:0");
					break;
				case 0b111: // 3'b111: if (!ENABLE_MAPPER_189 | ~flags[1]) prg_bank_b[(MMC3_BITSIZE-1):0] = cpu_data_in[(MMC3_BITSIZE-1):0];
					if (!(flags & 2))
						SET_BITS(prg_bank_b, "7:0", V, "7:0");
					break;
				}
				break;
			case 0b010: // $A000-$BFFE, even (mirroring)
				// if (!ENABLE_MAPPER_206 | ~flags[2]) // disabled for mapper #206
				if (!(flags & 4))
					mirroring = V & 1; // mirroring = {1'b0, cpu_data_in[0]};
				break;
			case 0b011: // RAM protect... no
				break;
			case 0b100: // 3'b100: mmc3_irq_latch = cpu_data_in; // $C000-$DFFE, even (IRQ latch)
				mmc3_irq_latch = V; break;
			case 0b101: // 3'b101: mmc3_irq_reload = 1; // $C001-$DFFF, odd
				mmc3_irq_reload = 1; break;
			case 0b110: // 3'b110: mmc3_irq_enabled = 0; // $E000-$FFFE, even
				X6502_IRQEnd(FCEU_IQEXT);
				mmc3_irq_enabled = 0;
				break;
			case 0b111: // $E001-$FFFF, odd	
				if (!(flags & 4)) // if (!USE_MAPPER_206 | ~flags[2]) // disabled for mapper #206
					mmc3_irq_enabled = 1; // mmc3_irq_enabled = 1; 
				break;
			}
		}

		// Mapper #112
		if (mapper == 0b010101)
		{
			switch (get_bits(A, "14:13"))
			{
			case 0b00: // $8000-$9FFF
				SET_BITS(mapper112_internal, "2:0", V, "2:0"); // mapper112_internal[2:0] = cpu_data_in[2:0];
				break;
			case 0b01: // $A000-BFFF
				switch (get_bits(mapper112_internal, "2:0"))
				{
				case 0b000: // 3'b000: prg_bank_a[5:0] = cpu_data_in[5:0];
					SET_BITS(prg_bank_a, "5:0", V, "5:0"); break;
				case 0b001: // 3'b001: prg_bank_b[5:0] = cpu_data_in[5:0];
					SET_BITS(prg_bank_b, "5:0", V, "5:0"); break;
				case 0b010: // 3'b010: chr_bank_a[7:0] = cpu_data_in[7:0];
					SET_BITS(chr_bank_a, "7:0", V, "7:0"); break;
				case 0b011: // 3'b011: chr_bank_c[7:0] = cpu_data_in[7:0];
					SET_BITS(chr_bank_c, "7:0", V, "7:0"); break;
				case 0b100: // 3'b100: chr_bank_e[7:0] = cpu_data_in[7:0];
					SET_BITS(chr_bank_e, "7:0", V, "7:0"); break;
				case 0b101: // 3'b101: chr_bank_f[7:0] = cpu_data_in[7:0];
					SET_BITS(chr_bank_f, "7:0", V, "7:0"); break;
				case 0b110: // 3'b110: chr_bank_g[7:0] = cpu_data_in[7:0];
					SET_BITS(chr_bank_g, "7:0", V, "7:0"); break;
				case 0b111: // 3'b111: chr_bank_h[7:0] = cpu_data_in[7:0];
					SET_BITS(chr_bank_h, "7:0", V, "7:0"); break;
				}
				break;
			case 0b10: // $C000-$DFFF 
				break;
			case 0b11: // 2'b11: mirroring = {1'b0, cpu_data_in[0]}; // $E000-$FFFF
				mirroring = V & 1; break;
			}
		}

		// Mappers #33 + #48 - Taito
		// flag0=0 - #33, flag0=1 - #48
		if (mapper == 0b010110)
		{
			// case ({cpu_addr_in[14:13], cpu_addr_in[1:0]})
			switch (get_bits(A, "14:13,1:0"))
			{
			case 0b0000:
				SET_BITS(prg_bank_a, "5:0", V, "5:0"); // prg_bank_a[5:0] = cpu_data_in[5:0]; // $8000, PRG Reg 0 (8k @ $8000)
				if (!(flags & 1)) // if (~flags[0]) // 33
					mirroring = get_bits(V, "6"); // mirroring = {1'b0, cpu_data_in[6]};
				break;
			case 0b0001: // 4'b0001: prg_bank_b[5:0] = cpu_data_in[5:0]; // $8001, PRG Reg 1 (8k @ $A000)
				SET_BITS(prg_bank_b, "5:0", V, "5:0"); break;
			case 0b0010: // 4'b0010: chr_bank_a[7:1] = cpu_data_in[6:0]; // $8002, CHR Reg 0 (2k @ $0000)
				SET_BITS(chr_bank_a, "7:1", V, "6:0"); break;
			case 0b0011: // 4'b0011: chr_bank_c[7:1] = cpu_data_in[6:0]; // $8003, CHR Reg 1 (2k @ $0800)
				SET_BITS(chr_bank_c, "7:1", V, "6:0"); break;
			case 0b0100: // 4'b0100: chr_bank_e[7:0] = cpu_data_in[7:0]; // $A000, CHR Reg 2 (1k @ $1000)
				SET_BITS(chr_bank_e, "7:0", V, "7:0"); break;
			case 0b0101: // 4'b0101: chr_bank_f[7:0] = cpu_data_in[7:0]; // $A001, CHR Reg 2 (1k @ $1400)
				SET_BITS(chr_bank_f, "7:0", V, "7:0"); break;
			case 0b0110: // 4'b0110: chr_bank_g[7:0] = cpu_data_in[7:0]; // $A002, CHR Reg 2 (1k @ $1800)
				SET_BITS(chr_bank_g, "7:0", V, "7:0"); break;
			case 0b0111: // 4'b0111: chr_bank_h[7:0] = cpu_data_in[7:0]; // $A003, CHR Reg 2 (1k @ $1C00)
				SET_BITS(chr_bank_h, "7:0", V, "7:0"); break;
			case 0b1100: // 4'b1100: if (flags[0]) mirroring = {1'b0, cpu_data_in[6]};	// $E000, mirroring, for mapper #48
				if (flags & 1) // 48
					mirroring = get_bits(V, "6"); // mirroring = cpu_data_in[6];
				break;
			case 0b1000: // 4'b1000: irq_scanline_latch = ~cpu_data_in; // $C000, IRQ latch
				mmc3_irq_latch = set_bits(mmc3_irq_latch, "7:0", get_bits(V, "7:0") ^ 0b11111111);
				break;
			case 0b1001: // 4'b1001: mmc3_irq_reload = 1; // $C001, IRQ reload
				mmc3_irq_reload = 1; break;
			case 0b1010: // 4'b1010: mmc3_irq_enabled = 1; // $C002, IRQ enable
				mmc3_irq_enabled = 1; break;
			case 0b1011: // 4'b1011: mmc3_irq_enabled = 0; // $C003, IRQ disable & ack
				mmc3_irq_enabled = 0;
				X6502_IRQEnd(FCEU_IQEXT); // irq_cpu_out = 0; // ack
				break;
			}
		}

		// Mapper #42
		if (mapper == 0b010111)
		{
			// case ({cpu_addr_in[14], cpu_addr_in[1:0]})
			switch (get_bits(A, "14,1:0"))
			{
			case 0: // 3'b000: chr_bank_a[7:3] = cpu_data_in[4:0]; // $8000, CHR Reg (8k @ $8000)
				SET_BITS(chr_bank_a, "7:3", V, "4:0");
				break;
			case 4: // 3'b100: prg_bank_6000[3:0] = cpu_data_in[3:0]; // $E000, PRG Reg (8k @ $6000)
				SET_BITS(prg_bank_6000, "3:0", V, "3:0");
				break;
			case 5: // 3'b101: mirroring = {1'b0, cpu_data_in[3]}; // Mirroring
				mirroring = get_bits(V, "3");
				break;
			case 6: // 3'b110: ... // IRQ
				mapper42_irq_enabled = get_bits(V, "1"); // mapper42_irq_enabled = cpu_data_in[1];
				if (!mapper42_irq_enabled) // if (!mapper42_irq_enabled)
				{
					X6502_IRQEnd(FCEU_IQEXT); // irq_cpu_out = 0;
					mapper42_irq_value = 0; // mapper42_irq_value = 0;
				}
				break;
			}
		}

		// Mapper #23 - VRC2/4
		/*
		flag0 - switches A0 and A1 lines. 0=A0,A1 like VRC2b (mapper #23), 1=A1,A0 like VRC2a(#22), VRC2c(#25)
		flag1 - divides CHR bank select by two (mapper #22, VRC2a)
		*/
		if (mapper == 0b011000)
		{
			uint8 vrc_2b_hi =
				(flags & 5) == 0 ? // (!flags[0] && !flags[2]) ?
				(get_bits(A, "7") | get_bits(A, "2")) // | cpu_addr_in[7] | cpu_addr_in[2]) // mapper #21
				: (flags & 5) == 1 ? //: (flags[0] && !flags[2]) ?
				get_bits(A, "0") // (cpu_addr_in[0]) // mapper #22
				: (flags & 5) == 4 ? // : (!flags[0] && flags[2]) ?
				(get_bits(A, "5") | get_bits(A, "3") | get_bits(A, "1")) // (cpu_addr_in[5] | cpu_addr_in[3] | cpu_addr_in[1]) // mapper #23
				: (get_bits(A, "2") | get_bits(A, "0")); // : (cpu_addr_in[2] | cpu_addr_in[0]); // mapper #25
			uint8 vrc_2b_low =
				(flags & 5) == 0 ? // (!flags[0] && !flags[2]) ?
				(get_bits(A, "6") | get_bits(A, "1")) // (cpu_addr_in[6] | cpu_addr_in[1]) // mapper #21
				: (flags & 5) == 1 ? // : (flags[0] && !flags[2]) ?
				get_bits(A, "1") // (cpu_addr_in[1]) // mapper #22
				: (flags & 5) == 4 ?// : (!flags[0] && flags[2]) ?
				(get_bits(A, "4") | get_bits(A, "2") | get_bits(A, "0")) // (cpu_addr_in[4] | cpu_addr_in[2] | cpu_addr_in[0]) // mapper #23
				: (get_bits(A, "3") | get_bits(A, "1")); // : (cpu_addr_in[3] | cpu_addr_in[1]); // mapper #25

			// case ({ cpu_addr_in[14:12], vrc_2b_hi, vrc_2b_low })
			switch ((get_bits(A, "14:12") << 2) | (vrc_2b_hi << 1) | vrc_2b_low)
			{
			case 0b00000: // $8000-$8003, PRG0
			case 0b00001:
			case 0b00010:
			case 0b00011:
				SET_BITS(prg_bank_a, "4:0", V, "4:0"); // prg_bank_a[4:0] = cpu_data_in[4:0];
				break;
			case 0b00100: // $9000-$9001, mirroring
			case 0b00101:
				// VRC2 - using games are usually well - behaved and only write 0 or 1 to this register,
				// but Wai Wai World in one instance writes $FF instead
				if (V != 0b11111111) // if (cpu_data_in != 8'b11111111) mirroring = cpu_data_in[1:0]; // $9000-$9001, mirroring
					SET_BITS(mirroring, "1:0", V, "1:0");
				break;
			case 0b00110: // $9002-$9004, PRG swap
			case 0b00111:
				SET_BITS(prg_mode, "0", V, "1"); // prg_mode[0] = cpu_data_in[1];
				break;
			case 0b01000:	// $A000-$A003, PRG1
			case 0b01001:
			case 0b01010:
			case 0b01011:
				SET_BITS(prg_bank_b, "4:0", V, "4:0"); // prg_bank_b[4:0] = cpu_data_in[4:0];
				break;
			case 0b01100: // 5'b01100: chr_bank_a[3:0] = cpu_data_in[3:0];  // $B000, CHR0 low
				SET_BITS(chr_bank_a, "3:0", V, "3:0"); break;
			case 0b01101: // 5'b01101: chr_bank_a[7:4] = cpu_data_in[3:0];  // $B001, CHR0 hi
				SET_BITS(chr_bank_a, "7:4", V, "3:0"); break;
			case 0b01110: // 5'b01110: chr_bank_b[3:0] = cpu_data_in[3:0];  // $B002, CHR1 low
				SET_BITS(chr_bank_b, "3:0", V, "3:0"); break;
			case 0b01111: // 5'b01111: chr_bank_b[7:4] = cpu_data_in[3:0];  // $B003, CHR1 hi
				SET_BITS(chr_bank_b, "7:4", V, "3:0"); break;
			case 0b10000: // 5'b10000: chr_bank_c[3:0] = cpu_data_in[3:0];  // $C000, CHR2 low
				SET_BITS(chr_bank_c, "3:0", V, "3:0"); break;
			case 0b10001: // 5'b10001: chr_bank_c[7:4] = cpu_data_in[3:0];  // $C001, CHR2 hi
				SET_BITS(chr_bank_c, "7:4", V, "3:0"); break;
			case 0b10010: // 5'b10010: chr_bank_d[3:0] = cpu_data_in[3:0];  // $C002, CHR3 low
				SET_BITS(chr_bank_d, "3:0", V, "3:0"); break;
			case 0b10011: // 5'b10011: chr_bank_d[7:4] = cpu_data_in[3:0];  // $C003, CHR3 hi
				SET_BITS(chr_bank_d, "7:4", V, "3:0"); break;
			case 0b10100: // 5'b10100: chr_bank_e[3:0] = cpu_data_in[3:0];  // $D000, CHR4 low
				SET_BITS(chr_bank_e, "3:0", V, "3:0"); break;
			case 0b10101: // 5'b10101: chr_bank_e[7:4] = cpu_data_in[3:0];  // $D001, CHR4 hi
				SET_BITS(chr_bank_e, "7:4", V, "3:0"); break;
			case 0b10110: // 5'b10110: chr_bank_f[3:0] = cpu_data_in[3:0];  // $D002, CHR5 low
				SET_BITS(chr_bank_f, "3:0", V, "3:0"); break;
			case 0b10111: // 5'b10111: chr_bank_f[7:4] = cpu_data_in[3:0];  // $D003, CHR5 hi
				SET_BITS(chr_bank_f, "7:4", V, "3:0"); break;
			case 0b11000: // 5'b11000: chr_bank_g[3:0] = cpu_data_in[3:0];  // $E000, CHR6 low
				SET_BITS(chr_bank_g, "3:0", V, "3:0"); break;
			case 0b11001: // 5'b11001: chr_bank_g[7:4] = cpu_data_in[3:0];  // $E001, CHR6 hi
				SET_BITS(chr_bank_g, "7:4", V, "3:0"); break;
			case 0b11010: // 5'b11010: chr_bank_h[3:0] = cpu_data_in[3:0];  // $E002, CHR7 low
				SET_BITS(chr_bank_h, "3:0", V, "3:0"); break;
			case 0b11011: // 5'b11011: chr_bank_h[7:4] = cpu_data_in[3:0];  // $E003, CHR7 hi
				SET_BITS(chr_bank_h, "7:4", V, "3:0"); break;
			}

			// if (cpu_addr_in[14:12] == 3'b111)
			if (get_bits(A, "14:12") == 0b111)
			{
				// case (vrc_2b_hi, vrc_2b_low})
				switch ((vrc_2b_hi << 1) | vrc_2b_low)
				{
				case 0b00: // 2'b00: vrc4_irq_latch[3:0] = cpu_data_in[3:0];  // IRQ latch low
					SET_BITS(vrc4_irq_latch, "3:0", V, "3:0"); break;
				case 0b01: // 2'b01: vrc4_irq_latch[7:4] = cpu_data_in[3:0];  // IRQ latch hi
					SET_BITS(vrc4_irq_latch, "7:4", V, "3:0"); break;
				case 0b10: // 2'b10 // IRQ control
					X6502_IRQEnd(FCEU_IQEXT); // vrc4_irq_out = 0; // ack
					SET_BITS(vrc4_irq_control, "2:0", V, "2:0"); // vrc4_irq_control[2:0] = cpu_data_in[2:0]; // mode, enabled, enabled after ack
					if (vrc4_irq_control & 2) // if (vrc4_irq_control[1]) begin // if E is set
					{
						vrc4_irq_prescaler_counter = 0; // vrc4_irq_prescaler_counter[1:0] = 2'b00; // reset prescaler
						vrc4_irq_prescaler = 0; // vrc4_irq_prescaler[6:0] = 7'b0000000;
						SET_BITS(vrc4_irq_value, "7:0", vrc4_irq_latch, "7:0"); // vrc4_irq_value[7:0] = vrc4_irq_latch[7:0]; // reload with latch
					}
					break;
				case 0b11: // 2'b11 // IRQ ack
					X6502_IRQEnd(FCEU_IQEXT); // vrc4_irq_out = 0;
					SET_BITS(vrc4_irq_control, "1", vrc4_irq_control, "0"); // vrc4_irq_control[1] = vrc4_irq_control[0];
					break;
				}
			}
		}

		// Mapper #69 - Sunsoft FME-7
		if (mapper == 0b011001)
		{
			// if (cpu_addr_in[14:13] == 2'b00) mapper69_internal[3:0] = cpu_data_in[3:0];
			if (get_bits(A, "14:13") == 0b00) SET_BITS(mapper69_internal, "3:0", V, "3:0");
			// if (cpu_addr_in[14:13] == 2'b01)
			if (get_bits(A, "14:13") == 0b01)
			{
				switch (get_bits(mapper69_internal, "3:0")) // case (mapper69_internal[3:0])
				{
				case 0b0000: // 4'b0000: chr_bank_a[7:0] = cpu_data_in[7:0]; // CHR0
					SET_BITS(chr_bank_a, "7:0", V, "7:0"); break;
				case 0b0001: // 4'b0001: chr_bank_b[7:0] = cpu_data_in[7:0]; // CHR1
					SET_BITS(chr_bank_b, "7:0", V, "7:0"); break;
				case 0b0010: // 4'b0010: chr_bank_c[7:0] = cpu_data_in[7:0]; // CHR2
					SET_BITS(chr_bank_c, "7:0", V, "7:0"); break;
				case 0b0011: // 4'b0011: chr_bank_d[7:0] = cpu_data_in[7:0]; // CHR3
					SET_BITS(chr_bank_d, "7:0", V, "7:0"); break;
				case 0b0100: // 4'b0100: chr_bank_e[7:0] = cpu_data_in[7:0]; // CHR4
					SET_BITS(chr_bank_e, "7:0", V, "7:0"); break;
				case 0b0101: // 4'b0101: chr_bank_f[7:0] = cpu_data_in[7:0]; // CHR5
					SET_BITS(chr_bank_f, "7:0", V, "7:0"); break;
				case 0b0110: // 4'b0110: chr_bank_g[7:0] = cpu_data_in[7:0]; // CHR6
					SET_BITS(chr_bank_g, "7:0", V, "7:0"); break;
				case 0b0111: // 4'b0111: chr_bank_h[7:0] = cpu_data_in[7:0]; // CHR7
					SET_BITS(chr_bank_h, "7:0", V, "7:0"); break;
				case 0b1000: // 4'b1000: {sram_enabled, map_rom_on_6000, prg_bank_6000} = {cpu_data_in[7], ~cpu_data_in[6], cpu_data_in[5:0]}; // PRG0
					sram_enabled = (V >> 7) & 1;
					map_rom_on_6000 = ((V >> 6) & 1) ^ 1;
					prg_bank_6000 = V & 0x3F;
					break;
				case 0b1001: // 4'b1001: prg_bank_a[5:0] = cpu_data_in[5:0]; // PRG1
					SET_BITS(prg_bank_a, "5:0", V, "5:0"); break;
				case 0b1010: // 4'b1010: prg_bank_b[5:0] = cpu_data_in[5:0]; // PRG2
					SET_BITS(prg_bank_b, "5:0", V, "5:0"); break;
				case 0b1011: // 4'b1011: prg_bank_c[5:0] = cpu_data_in[5:0]; // PRG3
					SET_BITS(prg_bank_c, "5:0", V, "5:0"); break;
				case 0b1100: // 4'b1100: mirroring[1:0] = cpu_data_in[1:0]; // mirroring
					SET_BITS(mirroring, "1:0", V, "1:0"); break;
				case 0b1101: // 4'b1101
					X6502_IRQEnd(FCEU_IQEXT); // fme7_irq_out = 0; // ack
					mapper69_counter_enabled = get_bits(V, "7"); // fme7_counter_enabled = cpu_data_in[7];
					mapper69_irq_enabled = get_bits(V, "0"); // fme7_irq_enabled = cpu_data_in[0];
					break;
				case 0b1110: // 4'b1110: fme7_irq_value[7:0] = cpu_data_in[7:0]; // IRQ low
					SET_BITS(mapper69_irq_value, "7:0", V, "7:0"); break;
				case 0b1111: // fme7_irq_value[15:8] = cpu_data_in[7:0]; // IRQ high
					SET_BITS(mapper69_irq_value, "15:8", V, "7:0"); break;
				}
			}
		}

		// Mapper #32 - Irem's G-101
		if (mapper == 0b011010)
		{
			switch (get_bits(A, "14:12"))
			{
			case 0b000: // 2'b00: prg_bank_a[5:0] = cpu_data_in[5:0]; // $8000-$8FFF, PRG0
				SET_BITS(prg_bank_a, "5:0", V, "5:0");
				break;
			case 0b001: // 2'b01: {prg_mode[0], mirroring} = {cpu_data_in[1], 1'b0, cpu_data_in[0]}; // $9000-$9FFF, PRG mode, mirroring
				SET_BITS(prg_mode, "0", V, "1");
				mirroring = V & 1;
				break;
			case 0b010: // 2'b10: prg_bank_b[5:0] = cpu_data_in[5:0]; // $A000-$AFFF, PRG1
				SET_BITS(prg_bank_b, "5:0", V, "5:0");
				break;
			case 0b011: // $B000-$BFFF, CHR regs
				switch (get_bits(A, "2:0"))
				{
				case 0b000: // 3'b000: chr_bank_a[7:0] = cpu_data_in[7:0];
					SET_BITS(chr_bank_a, "7:0", V, "7:0"); break;
				case 0b001: // 3'b001: chr_bank_b[7:0] = cpu_data_in[7:0];
					SET_BITS(chr_bank_b, "7:0", V, "7:0"); break;
				case 0b010: // 3'b010: chr_bank_c[7:0] = cpu_data_in[7:0];
					SET_BITS(chr_bank_c, "7:0", V, "7:0"); break;
				case 0b011: // 3'b011: chr_bank_d[7:0] = cpu_data_in[7:0];
					SET_BITS(chr_bank_d, "7:0", V, "7:0"); break;
				case 0b100: // 3'b100: chr_bank_e[7:0] = cpu_data_in[7:0];
					SET_BITS(chr_bank_e, "7:0", V, "7:0"); break;
				case 0b101: // 3'b101: chr_bank_f[7:0] = cpu_data_in[7:0];
					SET_BITS(chr_bank_f, "7:0", V, "7:0"); break;
				case 0b110: // 3'b110: chr_bank_g[7:0] = cpu_data_in[7:0];
					SET_BITS(chr_bank_g, "7:0", V, "7:0"); break;
				case 0b111: // 3'b111: chr_bank_h[7:0] = cpu_data_in[7:0];
					SET_BITS(chr_bank_h, "7:0", V, "7:0"); break;
				}
				break;
			}
		}

		// Mapper #36 is assigned to TXC's PCB 01-22000-400
		if (mapper == 0b011101)
		{
			if (get_bits(A, "14:1") != 0b11111111111111) // (cpu_addr_in[14:1] != 14'b11111111111111)
			{
				// prg_bank_a[5:2] = cpu_data_in[7:4];
				SET_BITS(prg_bank_a, "5:2", V, "7:4");
				// chr_bank_a[6:3] = cpu_data_in[3:0];
				SET_BITS(chr_bank_a, "6:3", V, "3:0");
			}
		}

		// Mapper #70
		if (mapper == 0b011110)
		{
			// prg_bank_a[4:1] = cpu_data_in[7:4];
			SET_BITS(prg_bank_a, "4:1", V, "7:4");
			// chr_bank_a[6:3] = cpu_data_in[3:0];
			SET_BITS(chr_bank_a, "6:3", V, "3:0");
		}

		// Mapper #75 - VRC1
		if (mapper == 0b100010)
		{
			// case (cpu_addr_in[14:12])
			switch (get_bits(A, "14:12"))
			{
			case 0b000: // 3'b000: prg_bank_a[3:0] = cpu_data_in[3:0]; // $8000-$8FFF
				SET_BITS(prg_bank_a, "3:0", V, "3:0"); break;
			case 0b001:
				mirroring = V & 1; // mirroring = {1'b0, cpu_data_in[0]};
				SET_BITS(chr_bank_a, "6", V, "1"); // chr_bank_a[6] = cpu_data_in[1];
				SET_BITS(chr_bank_e, "6", V, "2"); // chr_bank_e[6] = cpu_data_in[2];
				break;
			case 0b010: // 3'b010: prg_bank_b[3:0] = cpu_data_in[3:0]; // $A000-$AFFF
				SET_BITS(prg_bank_b, "3:0", V, "3:0"); break;
			case 0b100: // 3'b100: prg_bank_c[3:0] = cpu_data_in[3:0]; // $C000-$CFFF
				SET_BITS(prg_bank_c, "3:0", V, "3:0"); break;
			case 0b110: // 3'b110: �hr_bank_a[5:2] = cpu_data_in[3:0]; // $E000-$EFFF
				SET_BITS(chr_bank_a, "5:2", V, "3:0"); break;
			case 0b111: // 3'b111: chr_bank_e[5:2] = cpu_data_in[3:0]; // $F000-$FFFF
				SET_BITS(chr_bank_e, "5:2", V, "3:0"); break;
			}
		}

		// Mapper #83 - Cony/Yoko
		if (mapper == 0b100011)
		{
			// case (cpu_addr_in[9:8])
			switch (get_bits(A, "9:8"))
			{
			case 0b00: // $80xx
				SET_BITS(prg_bank_a, "4:1", V, "3:0");
				break;
			case 0b01: // $81xx
				mirroring = get_bits(V, "1:0"); // mirroring <= cpu_data_in[1:0];
				SET_BITS(prg_mode, "2", V, "4"); // prg_mode[2] <= cpu_data_in[4];
				map_rom_on_6000 = get_bits(V, "5"); // map_rom_on_6000 <= cpu_data_in[5];
				mapper83_irq_enabled_latch = get_bits(V, "7"); // mapper83_irq_enabled_latch <= cpu_data_in[7];
				break;
			case 0b10: // 82xx
				if (!get_bits(A, "0")) // if (!cpu_addr_in[0])
				{
					X6502_IRQEnd(FCEU_IQEXT); // mapper83_irq_out <= 0;
					SET_BITS(mapper83_irq_counter, "7:0", V, "7:0"); // mapper83_irq_counter[7:0] <= cpu_data_in[7:0];
				}
				else {
					mapper83_irq_enabled = mapper83_irq_enabled_latch; //mapper83_irq_enabled <= mapper83_irq_enabled_latch;
					SET_BITS(mapper83_irq_counter, "15:8", V, "7:0"); // mapper83_irq_counter[15:8] <= cpu_data_in[7:0];
				}
				break;
			case 0b11:
				if (!get_bits(A, "4")) // if (!cpu_addr_in[4])
				{
					switch (get_bits(A, "1:0")) // case (cpu_addr_in[1:0])
					{
					case 0b00: SET_BITS(prg_bank_a, "7:0", V, "7:0"); break; // 2'b00: prg_bank_a[7:0] <= cpu_data_in[7:0];
					case 0b01: SET_BITS(prg_bank_b, "7:0", V, "7:0"); break; // 2'b01: prg_bank_b[7:0] <= cpu_data_in[7:0];
					case 0b10: SET_BITS(prg_bank_b, "7:0", V, "7:0"); break; // 2'b10: prg_bank_c[7:0] <= cpu_data_in[7:0];
					case 0b11: SET_BITS(prg_bank_6000, "7:0", V, "7:0"); break; //2'b11: prg_bank_6000[7:0] <= cpu_data_in[7:0];
					}
				}
				else {
					if (!(flags & 0b100))
					{
						switch (get_bits(A, "2:0")) // case (cpu_addr_in[2:0])
						{
						case 0b000: SET_BITS(chr_bank_a, "7:0", V, "7:0"); break; // 3'b000: chr_bank_a[7:0] <= cpu_data_in[7:0];
						case 0b001: SET_BITS(chr_bank_b, "7:0", V, "7:0"); break; // 3'b001: chr_bank_b[7:0] <= cpu_data_in[7:0];
						case 0b010: SET_BITS(chr_bank_c, "7:0", V, "7:0"); break; // 3'b010: chr_bank_c[7:0] <= cpu_data_in[7:0];
						case 0b011: SET_BITS(chr_bank_d, "7:0", V, "7:0"); break; // 3'b011: chr_bank_d[7:0] <= cpu_data_in[7:0];
						case 0b100: SET_BITS(chr_bank_e, "7:0", V, "7:0"); break; // 3'b100: chr_bank_e[7:0] <= cpu_data_in[7:0];
						case 0b101: SET_BITS(chr_bank_f, "7:0", V, "7:0"); break; // 3'b101: chr_bank_f[7:0] <= cpu_data_in[7:0];
						case 0b110: SET_BITS(chr_bank_g, "7:0", V, "7:0"); break; // 3'b110: chr_bank_g[7:0] <= cpu_data_in[7:0];
						case 0b111: SET_BITS(chr_bank_h, "7:0", V, "7:0"); break; // 3'b111: chr_bank_h[7:0] <= cpu_data_in[7:0];
						}
					}
					else {
						switch (get_bits(A, "2:0")) // case (cpu_addr_in[2:0])
						{
						case 0b000:
							SET_BITS(chr_bank_a, "8:1", V, "7:0"); break; // 3'b000: chr_bank_a[8:1] <= cpu_data_in[7:0];
						case 0b001:
							SET_BITS(chr_bank_c, "8:1", V, "7:0"); break; // 3'b001: chr_bank_c[8:1] <= cpu_data_in[7:0];
						case 0b110:
							SET_BITS(chr_bank_e, "8:1", V, "7:0"); break; // 3'b110: chr_bank_e[8:1] <= cpu_data_in[7:0];
						case 0b111:
							SET_BITS(chr_bank_g, "8:1", V, "7:0"); break; // 3'b111: chr_bank_g[8:1] <= cpu_data_in[7:0];
						}
					}
				}
				break;
			}
		}

		// Mapper #67 - Sunsoft-3
		if (mapper == 0b100100)
		{
			if (get_bits(A, "11")) // if (cpu_addr_in[11])
			{
				switch (get_bits(A, "14:12")) // case (cpu_addr_in[14:12])
				{
				case 0b000: // 3'b000: chr_bank_a[6:1] <= cpu_data_in[5:0]; // $8800
					SET_BITS(chr_bank_a, "6:1", V, "5:0"); break;
				case 0b001: // 3'b001: chr_bank_c[6:1] <= cpu_data_in[5:0]; // $9800
					SET_BITS(chr_bank_c, "6:1", V, "5:0"); break;
				case 0b010: // 3'b010: chr_bank_e[6:1] <= cpu_data_in[5:0]; // $A800
					SET_BITS(chr_bank_e, "6:1", V, "5:0"); break;
				case 0b011: // 3'b011: chr_bank_g[6:1] <= cpu_data_in[5:0]; // $B800
					SET_BITS(chr_bank_g, "6:1", V, "5:0"); break;
				case 0b100: // 3'b100: begin // $C800, IRQ load
					mapper67_irq_latch = ~mapper67_irq_latch;
					if (mapper67_irq_latch)
						SET_BITS(mapper67_irq_counter, "15:8", V, "7:0"); // mapper67_irq_counter[15:8] <= cpu_data_in[7:0];
					else
						SET_BITS(mapper67_irq_counter, "7:0", V, "7:0"); // mapper67_irq_counter[7:0] <= cpu_data_in[7:0];
					break;
				case 0b101: // 3'b101: begin // $D800, IRQ enable
					mapper67_irq_latch = 0; // mapper67_irq_latch <= 0;
					SET_BITS(mapper67_irq_enabled, "0", V, "4"); // mapper67_irq_enabled <= cpu_data_in[4];
					break;
				case 0b110: // 3'b110: mirroring[1:0] <= cpu_data_in[1:0];  // $E800
					SET_BITS(mirroring, "1:0", V, "1:0");
					break;
				case 0b111: // 3'b111: prg_bank_a[4:1] <= cpu_data_in[3:0]; // $F800
					SET_BITS(prg_bank_a, "4:1", V, "3:0");
					break;
				}
			}
			else {
				// Interrupt Acknowledge ($8000)
				X6502_IRQEnd(FCEU_IQEXT); // mapper67_irq_out <= 0;
			}
		}

		// Mapper #89 - Sunsoft-2 chip on the Sunsoft-3 board
		if (mapper == 0b100101)
		{
			// prg_bank_a[3:1] <= cpu_data_in[6:4];
			SET_BITS(prg_bank_a, "3:1", V, "6:4");
			// chr_bank_a[6:3] <= {cpu_data_in[7], cpu_data_in[2:0]};
			SET_BITS(chr_bank_a, "6:3", V, "7,2:0");
			// mirroring[1:0] <= {1'b1, cpu_data_in[3]};
			SET_BITS(mirroring, "1", 1, "0");
			SET_BITS(mirroring, "0", V, "3");
		}
	}

	COOLGIRL_Sync();
}

static DECLFR(MAFRAM) {
	if ((mapper == 0b000000) && (A >= 0x5000) && (A < 0x6000))
		return 0;

	// Mapper #163
	//	(USE_MAPPER_163 && (mapper == 5'b00110) && m2 && romsel && cpu_rw_in && ((cpu_addr_in[14:8] & 7'h77) == 7'h51)) ?  
	if ((mapper == 0b000110) && ((A & 0x7700) == 0x5100))
		return mapper163_r2 | mapper163_r0 | mapper163_r1 | ~mapper163_r3; // {1'b1, r2 | r0 | r1 | ~r3}				
	// (USE_MAPPER_163 && (mapper == 5'b00110) && m2 && romsel && cpu_rw_in && ((cpu_addr_in[14:8] & 7'h77) == 7'h55)) ? 
	if ((mapper == 0b000110) && ((A & 0x7700) == 0x5500))
		return (mapper163_r5 & 1) ? mapper163_r2 : mapper163_r1; // {1'b1, r5[0] ? r2 : r1}

	// MMC5
	if ((mapper == 0b001111) && (A == 0x5204))
	{
		int ppuon = (PPU[1] & 0x18);
		uint8 r = (!ppuon || scanline + 1 >= 241) ? 0 : 1;
		uint8 p = mmc5_irq_out;
		X6502_IRQEnd(FCEU_IQEXT);
		mmc5_irq_out = 0;
		return (p << 7) | (r << 6);
	}

	// Mapper #36 is assigned to TXC's PCB 01-22000-400
	if ((mapper == 0b011101) && ((A & 0xE100) == 0x4100)) // (USE_MAPPER_036 && mapper == 5'b11101 && {cpu_addr_in[14:13], cpu_addr_in[8]} == 3'b101) ?
	{
		return (prg_bank_a & 0x0C) << 2; // {1'b1, 2'b00, prg_bank_a[3:2], 4'b00}
	}

	// Mapper #83 - Cony/Yoko
	if ((mapper == 0b100011) && ((A & 0x7000) == 0x5000)) return flags & 3;

	// Mapper #90 - JY
	if ((mapper == 0b001101) && (A == 0x5800)) return (mul1 * mul2) & 0xFF;
	if ((mapper == 0b001101) && (A == 0x5801)) return ((mul1 * mul2) >> 8) & 0xFF;

	if (sram_enabled && !map_rom_on_6000 && (A >= 0x6000) && (A < 0x8000))
		return CartBR(A);          // SRAM
	if (map_rom_on_6000 && (A >= 0x6000) && (A < 0x8000))
		return CartBR(A);          // PRG

	return X.DB; // Open bus
}

static void COOLGIRL_ScanlineCounter(void) {
	// for MMC3 and MMC3-based
	if (mmc3_irq_reload || !mmc3_irq_counter)
	{
		mmc3_irq_counter = mmc3_irq_latch;
		mmc3_irq_reload = 0;
	}
	else
		mmc3_irq_counter--;
	if (!mmc3_irq_counter && mmc3_irq_enabled)
		X6502_IRQBegin(FCEU_IQEXT);

	// for MMC5
	if (mmc5_irq_line == scanline + 1)
	{
		if (mmc5_irq_enabled)
		{
			X6502_IRQBegin(FCEU_IQEXT);
			mmc5_irq_out = 1;
		}
	}

	// for mapper #163
	if (scanline == 239)
	{
		mapper_163_latch = 0;
		COOLGIRL_Sync_CHR();
	}
	else if (scanline == 127)
	{
		mapper_163_latch = 1;
		COOLGIRL_Sync_CHR();
	}
}

static void COOLGIRL_CpuCounter(int a) {
	while (a--)
	{
		// Mapper #23 - VRC4
		if (vrc4_irq_control & 2) // if (ENABLE_MAPPER_021_022_023_025 & ENABLE_VRC4_INTERRUPTS & (vrc4_irq_control[1]))
		{
			vrc4_irq_prescaler++; // vrc4_irq_prescaler = vrc4_irq_prescaler + 1'b1; // count prescaler
			// if ((vrc4_irq_prescaler_counter[1] == 0 && vrc4_irq_prescaler == 114)
			//   || (vrc4_irq_prescaler_counter[1] == 1 && vrc4_irq_prescaler == 113)) // 114, 114, 113
			if ((!(vrc4_irq_prescaler_counter & 2) && vrc4_irq_prescaler == 114) || ((vrc4_irq_prescaler_counter & 2) && vrc4_irq_prescaler == 113))
			{
				vrc4_irq_prescaler = 0; // vrc4_irq_prescaler = 0;
				vrc4_irq_prescaler_counter++; // vrc4_irq_prescaler_counter = vrc4_irq_prescaler_counter + 1'b1;
				if (vrc4_irq_prescaler_counter == 0b11) vrc4_irq_prescaler_counter = 0; // if (vrc4_irq_prescaler_counter == 2'b11) vrc4_irq_prescaler_counter =  2'b00;
				vrc4_irq_value++; // {carry, vrc4_irq_value[7:0]} = vrc4_irq_value[7:0] + 1'b1;
				if (vrc4_irq_value == 0) // f (carry)
				{
					X6502_IRQBegin(FCEU_IQEXT);
					vrc4_irq_value = vrc4_irq_latch; // irq_cpu_value[7:0] = vrc4_irq_latch[7:0];
				}
			}
		}

		// Mapper #73 - VRC3
		if (vrc3_irq_control & 2)
		{
			if (vrc3_irq_control & 4) // if (vrc3_irq_control[2])
			{  // 8-bit mode
				//vrc3_irq_value = set_bits(vrc3_irq_value, "7:0", get_bits(vrc3_irq_value, "7:0") + 1); // vrc3_irq_value[7:0] = vrc3_irq_value[7:0] + 1'b1;
				vrc3_irq_value = (vrc3_irq_value & 0xFF00) | ((vrc3_irq_value + 1) & 0xFF);
				//if (get_bits(vrc3_irq_value, "7:0") == 0) // if (vrc3_irq_value[7:0] == 0)
				if ((vrc3_irq_value & 0xFF) == 0)
				{
					X6502_IRQBegin(FCEU_IQEXT); // vrc3_irq_out = 1;
					//SET_BITS(vrc3_irq_value, "7:0", vrc3_irq_latch, "7:0"); // vrc3_irq_value[7:0] = vrc3_irq_latch[7:0];
					vrc3_irq_value = (vrc3_irq_value & 0xFF00) | (vrc3_irq_latch & 0xFF);
				}
			}
			else { // 16-bit
				//vrc3_irq_value = set_bits(vrc3_irq_value, "15:0", get_bits(vrc3_irq_value, "15:0") + 1); // vrc3_irq_value[15:0] = vrc3_irq_value[15:0] + 1'b1;
				vrc3_irq_value += 1;
				if (vrc3_irq_value == 0) // if (vrc3_irq_value[15:0] == 0)
				{
					X6502_IRQBegin(FCEU_IQEXT); // vrc3_irq_out = 1;
					//SET_BITS(vrc3_irq_value, "15:0", vrc3_irq_latch, "15:0"); // vrc3_irq_value[15:0] = vrc3_irq_latch[15:0];
					vrc3_irq_value = vrc3_irq_latch;
				}
			}
		}

		// Mapper #69 - Sunsoft FME-7
		// if (ENABLE_MAPPER_069 & fme7_counter_enabled)
		if (mapper69_counter_enabled)
		{
			// {carry, fme7_irq_value[15:0]} = {0'b0, fme7_irq_value[15:0]} - 1'b1;
			mapper69_irq_value--;
			// if (fme7_irq_enabled && carry) fme7_irq_out = 1;
			if (mapper69_irq_value == 0xFFFF) X6502_IRQBegin(FCEU_IQEXT);
		}

		// Mapper #18
		if (mapper18_irq_control & 1) // if (mapper18_irq_control[0])
		{
			uint8 carry;
			//carry = get_bits(mapper18_irq_value, "3:0") - 1;
			carry = (mapper18_irq_value & 0x0F) - 1;
			//SET_BITS(mapper18_irq_value, "3:0", carry, "3:0");
			mapper18_irq_value = (mapper18_irq_value & 0xFFF0) | (carry & 0x0F);
			carry = (carry >> 4) & 1;
			// if (mapper18_irq_control[3] == 1'b0)
			if (!(mapper18_irq_control & 0b1000))
			{
				//carry = get_bits(mapper18_irq_value, "7:4") - carry;
				carry = ((mapper18_irq_value >> 4) & 0x0F) - carry;
				//SET_BITS(mapper18_irq_value, "7:4", carry, "3:0");
				mapper18_irq_value = (mapper18_irq_value & 0xFF0F) | ((carry & 0x0F) << 4);
				carry = (carry >> 4) & 1;
			}
			// if (mapper18_irq_control[3:2] == 2'b00)
			if (!(mapper18_irq_control & 0b1100))
			{
				//carry = get_bits(mapper18_irq_value, "11:8") - carry;
				carry = ((mapper18_irq_value >> 8) & 0x0F) - carry;
				//SET_BITS(mapper18_irq_value, "11:8", carry, "3:0");
				mapper18_irq_value = (mapper18_irq_value & 0xF0FF) | ((carry & 0x0F) << 8);
				carry = (carry >> 4) & 1;
			}
			// if (mapper18_irq_control[3:1] == 3'b000)
			if (!(mapper18_irq_control & 0b1110))
			{
				//carry = get_bits(mapper18_irq_value, "15:12") - carry;
				carry = ((mapper18_irq_value >> 12) & 0x0F) - carry;
				//SET_BITS(mapper18_irq_value, "15:12", carry, "3:0");
				mapper18_irq_value = (mapper18_irq_value & 0x0FFF) | ((carry & 0x0F) << 12);
				carry = (carry >> 4) & 1;
			}
			if (carry)
				X6502_IRQBegin(FCEU_IQEXT);
		}

		// Mapper #65 - Irem's H3001
		// if (mapper65_irq_enabled)
		if (mapper65_irq_enabled)
		{
			if (mapper65_irq_value != 0) // if (mapper65_irq_value[15:0] != 0)
			{
				mapper65_irq_value--; // mapper65_irq_value[15:0] = mapper65_irq_value[15:0] - 1;
				if (!mapper65_irq_value)
					X6502_IRQBegin(FCEU_IQEXT); // if (mapper65_irq_value[15:0] == 0) irq_cpu_out = 1;
			}
		}

		// Mapper #42
		if (mapper42_irq_enabled)
		{
			//mapper42_irq_value = set_bits(mapper42_irq_value, "14:0", get_bits(mapper42_irq_value, "14:0") + 1);
			//if (get_bits(mapper42_irq_value, "14:13") == 0b11)
			mapper42_irq_value++;
			if (mapper42_irq_value >> 15) mapper42_irq_value = 0;
			if (((mapper42_irq_value >> 13) & 0b11) == 0b11)
				X6502_IRQBegin(FCEU_IQEXT);
			else
				X6502_IRQEnd(FCEU_IQEXT);
		}

		// Mapper #83 - Cony/Yoko
		if (mapper83_irq_enabled)
		{
			if (mapper83_irq_counter == 0) // if (mapper83_irq_counter == 0)
				X6502_IRQBegin(FCEU_IQEXT); // mapper83_irq_out <= 1;
			mapper83_irq_counter--; // mapper83_irq_counter = mapper83_irq_counter - 1'b1;
		}

		// Mapper #67 - Sunsoft-3
		if (mapper67_irq_enabled)
		{
			mapper67_irq_counter--; // mapper67_irq_counter = mapper67_irq_counter - 1'b1;
			if (mapper67_irq_counter == 0xFFFF) // if (mapper67_irq_counter == 16'hFFFF)
			{
				X6502_IRQBegin(FCEU_IQEXT); // mapper67_irq_out <= 1; // fire IRQ
				mapper67_irq_enabled = 0; // mapper67_irq_enabled <= 0; // disable IRQ
			}
		}
	}
}

static void COOLGIRL_PPUHook(uint32 A) {
	// For TxROM
	if ((mapper == 0b010100) && (flags & 1))
	{
		setmirror(MI_0 + (TKSMIR[(A & 0x1FFF) >> 10] >> 7));
	}

	// Mapper #9 and #10 - MMC2 and MMC4
	if (mapper == 0b010001)
	{
		if ((A >> 4) == 0xFD) // if (ppu_addr_in[13:3] == 11'b00111111011) ppu_latch0 = 0;
		{
			ppu_latch0 = 0;
			COOLGIRL_Sync_CHR();
		}
		if ((A >> 4) == 0xFE) // if (ppu_addr_in[13:3] == 11'b00111111101) ppu_latch0 = 1;
		{
			ppu_latch0 = 1;
			COOLGIRL_Sync_CHR();
		}
		if ((A >> 4) == 0x1FD) // if (ppu_addr_in[13:3] == 11'b01111111011) ppu_latch1 = 0;
		{
			ppu_latch1 = 0;
			COOLGIRL_Sync_CHR();
		}
		if ((A >> 4) == 0x1FE) // if (ppu_addr_in[13:3] == 11'b01111111101) ppu_latch1 = 1;
		{
			ppu_latch1 = 1;
			COOLGIRL_Sync_CHR();
		}
	}
}

static void COOLGIRL_Reset(void) {
	sram_enabled = 0;
	sram_page = 0;
	can_write_chr = 0;
	map_rom_on_6000 = 0;
	flags = 0;
	mapper = 0;
	can_write_flash = 0;
	mirroring = 0;
	four_screen = 0;
	lockout = 0;
	prg_base = 0;
	prg_mask = 0b11111000 << 14;
	prg_mode = 0;
	prg_bank_6000 = 0;
	prg_bank_a = 0;
	prg_bank_b = 1;
	prg_bank_c = ~1;
	prg_bank_d = ~0;
	chr_mask = 0;
	chr_mode = 0;
	chr_bank_a = 0;
	chr_bank_b = 1;
	chr_bank_c = 2;
	chr_bank_d = 3;
	chr_bank_e = 4;
	chr_bank_f = 5;
	chr_bank_g = 6;
	chr_bank_h = 7;
	ppu_latch0 = 0;
	ppu_latch1 = 0;
	lreset = 0;
	mmc1_load_register = 0;
	mmc3_internal = 0;
	mapper69_internal = 0;
	mapper112_internal = 0;
	mapper_163_latch = 0;
	mapper163_r0 = 0;
	mapper163_r1 = 0;
	mapper163_r2 = 0;
	mapper163_r3 = 0;
	mapper163_r4 = 0;
	mapper163_r5 = 0;
	mul1 = 0;
	mul2 = 0;
	mmc3_irq_enabled = 0;
	mmc3_irq_latch = 0;
	mmc3_irq_counter = 0;
	mmc3_irq_reload = 0;
	mmc5_irq_enabled = 0;
	mmc5_irq_line = 0;
	mmc5_irq_out = 0;
	mapper18_irq_value = 0;
	mapper18_irq_control = 0;
	mapper18_irq_latch = 0;
	mapper65_irq_enabled = 0;
	mapper65_irq_value = 0;
	mapper65_irq_latch = 0;
	mapper69_irq_enabled = 0;
	mapper69_counter_enabled = 0;
	mapper69_irq_value = 0;
	vrc4_irq_value = 0;
	vrc4_irq_control = 0;
	vrc4_irq_latch = 0;
	vrc4_irq_prescaler = 0;
	vrc4_irq_prescaler_counter = 0;
	vrc3_irq_value = 0;
	vrc3_irq_control = 0;
	vrc3_irq_latch = 0;
	mapper42_irq_enabled = 0;
	mapper42_irq_value = 0;
	mapper90_xor = 0;
	flash_state = 0;
	cfi_mode = 0;
	COOLGIRL_Sync();
}

static void COOLGIRL_Power(void) {
	FCEU_CheatAddRAM(32, 0x6000, WRAM);
	SetReadHandler(0x4020, 0x7FFF, MAFRAM);
	SetReadHandler(0x8000, 0xFFFF, CartBR);
	SetWriteHandler(0x4020, 0xFFFF, COOLGIRL_WRITE);
	GameHBIRQHook = COOLGIRL_ScanlineCounter;
	MapIRQHook = COOLGIRL_CpuCounter;
	PPU_hook = COOLGIRL_PPUHook;
	COOLGIRL_Reset();
}

static void COOLGIRL_Close(void) {
	if (WRAM)
		FCEU_gfree(WRAM);
	if (SAVE_FLASH)
		FCEU_gfree(SAVE_FLASH);
	if (CFI)
		FCEU_gfree(CFI);
	WRAM = SAVE_FLASH = CFI = NULL;
}

static void COOLGIRL_Restore(int version) {
	COOLGIRL_Sync();
	lreset = 0;
}

#define ExState(var, varname) AddExState(&var, sizeof(var), 0, varname)

void COOLGIRL_Init(CartInfo* info) {
	CHR_SIZE = info->vram_size ? info->vram_size /* NES 2.0 */ : 256 * 1024 /* UNIF, fixed */;

	WRAM_SIZE = info->ines2 ? (info->wram_size + info->battery_wram_size) : (32 * 1024);
	if (WRAM_SIZE > 0) {
		WRAM = (uint8*)FCEU_gmalloc(WRAM_SIZE);
		memset(WRAM, 0, WRAM_SIZE);
		SetupCartPRGMapping(WRAM_CHIP, WRAM, WRAM_SIZE, 1);
		AddExState(WRAM, 32 * 1024, 0, "SRAM");
		if (info->battery)
		{
			info->addSaveGameBuf(WRAM, 32 * 1024);
		}
	}

	if (info->battery)
	{
		SAVE_FLASH = (uint8*)FCEU_gmalloc(SAVE_FLASH_SIZE);
		SetupCartPRGMapping(FLASH_CHIP, SAVE_FLASH, SAVE_FLASH_SIZE, 1);
		AddExState(SAVE_FLASH, SAVE_FLASH_SIZE, 0, "SAVF");
		info->addSaveGameBuf(SAVE_FLASH, SAVE_FLASH_SIZE);
	}

	CFI = (uint8*)FCEU_gmalloc(sizeof(cfi_data) * 2);
	for (size_t i = 0; i < sizeof(cfi_data); i++)
	{
		CFI[i * 2] = CFI[i * 2 + 1] = cfi_data[i];
	}
	SetupCartPRGMapping(CFI_CHIP, CFI, sizeof(cfi_data) * 2, 0);

	ExState(sram_enabled, "SREN");
	ExState(sram_page, "SRPG");
	ExState(can_write_chr, "SRWR");
	ExState(map_rom_on_6000, "MR6K");
	ExState(flags, "FLGS");
	ExState(mapper, "MPPR");
	ExState(can_write_flash, "FLWR");
	ExState(mirroring, "MIRR");
	ExState(four_screen, "4SCR");
	ExState(lockout, "LOCK");
	ExState(prg_base, "PBAS");
	ExState(prg_mask, "PMSK");
	ExState(prg_mode, "PMOD");
	ExState(prg_bank_6000, "P6BN");
	ExState(prg_bank_a, "PABN");
	ExState(prg_bank_b, "PBBN");
	ExState(prg_bank_c, "PCBN");
	ExState(prg_bank_d, "PDBN");
	ExState(chr_mask, "CMSK");
	ExState(chr_mode, "CMOD");
	ExState(chr_bank_a, "CABN");
	ExState(chr_bank_b, "CBBN");
	ExState(chr_bank_c, "CCBN");
	ExState(chr_bank_d, "CDBN");
	ExState(chr_bank_e, "CEBN");
	ExState(chr_bank_f, "CFBN");
	ExState(chr_bank_g, "CGBN");
	ExState(chr_bank_h, "CHBN");
	ExState(ppu_latch0, "PPU0");
	ExState(ppu_latch1, "PPU1");
	ExState(lreset, "LRST");
	ExState(mmc1_load_register, "M01R");
	ExState(mmc3_internal, "M01I");
	ExState(mapper69_internal, "M69I");
	ExState(mapper112_internal, "112I");
	ExState(mapper_163_latch, "163L");
	ExState(mapper163_r0, "1630");
	ExState(mapper163_r1, "1631");
	ExState(mapper163_r2, "1632");
	ExState(mapper163_r3, "1633");
	ExState(mapper163_r4, "1634");
	ExState(mapper163_r5, "1635");
	ExState(mul1, "MUL1");
	ExState(mul2, "MUL2");
	ExState(mmc3_irq_enabled, "M4IE");
	ExState(mmc3_irq_latch, "M4IL");
	ExState(mmc3_irq_counter, "M4IC");
	ExState(mmc3_irq_reload, "M4IR");
	ExState(mmc5_irq_enabled, "M5IE");
	ExState(mmc5_irq_line, "M5IL");
	ExState(mmc5_irq_out, "M5IO");
	ExState(mapper18_irq_value, "18IV");
	ExState(mapper18_irq_control, "18IC");
	ExState(mapper18_irq_latch, "18IL");
	ExState(mapper65_irq_enabled, "65IE");
	ExState(mapper65_irq_value, "65IV");
	ExState(mapper65_irq_latch, "65IL");
	ExState(mapper69_irq_enabled, "69IE");
	ExState(mapper69_counter_enabled, "69CE");
	ExState(mapper69_irq_value, "69IV");
	ExState(vrc4_irq_value, "V4IV");
	ExState(vrc4_irq_control, "V4IC");
	ExState(vrc4_irq_latch, "V4IL");
	ExState(vrc4_irq_prescaler, "V4PP");
	ExState(vrc4_irq_prescaler_counter, "V4PC");
	ExState(vrc3_irq_value, "V3IV");
	ExState(vrc3_irq_control, "V3IC");
	ExState(vrc3_irq_latch, "V3IL");
	ExState(mapper42_irq_enabled, "42IE");
	ExState(mapper42_irq_value, "42IV");
	ExState(mapper83_irq_enabled_latch, "M83L");
	ExState(mapper83_irq_enabled, "M83I");
	ExState(mapper83_irq_counter, "M83C");
	ExState(mapper90_xor, "90XR");
	ExState(mapper67_irq_enabled, "67IE");
	ExState(mapper67_irq_latch, "67IL");
	ExState(mapper67_irq_counter, "67IC");
	ExState(flash_state, "FLST");
	ExState(flash_buffer_a, "FLBA");
	ExState(flash_buffer_v, "FLBV");
	ExState(cfi_mode, "CFIM");

	info->Power = COOLGIRL_Power;
	info->Reset = COOLGIRL_Reset;
	info->Close = COOLGIRL_Close;
	GameStateRestore = COOLGIRL_Restore;
}
