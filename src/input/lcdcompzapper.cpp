/* FCE Ultra - NES/Famicom Emulator
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

#include "share.h"

static uint32 lcdCompZapperStrobe[2];
static uint32 lcdCompZapperData[2];

static uint8 ReadLCDCompZapper(int w)
{
	return lcdCompZapperData[w];
}

static void StrobeLCDCompZapper(int w)
{
	lcdCompZapperStrobe[w] = 0;
}

void UpdateLCDCompZapper(int w, void* data, int arg)
{
	// In the '(*(uint32*)data)' variable, bit 0 holds the trigger value and bit 1 holds the light sense value.
	// Ultimately this needs to be converted from 0000 00lt to 000t l000 where l is the light bit and t
	// is the trigger bit.
	// l must be inverted because 0: detected; 1: not detected
	lcdCompZapperData[w] = ((((*(uint32*)data) & 1) << 4) | 
		                (((*(uint32*)data) & 2 ^ 2) << 2));
}

static INPUTC LCDCompZapperCtrl = { ReadLCDCompZapper,0,StrobeLCDCompZapper,UpdateLCDCompZapper,0,0 };

INPUTC* FCEU_InitLCDCompZapper(int w)
{
	lcdCompZapperStrobe[w] = lcdCompZapperData[w] = 0;
	return(&LCDCompZapperCtrl);
}
