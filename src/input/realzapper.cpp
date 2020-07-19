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

static uint32 realZapperStrobe[2];
static uint32 realZapperData[2];

static uint8 ReadRealZapper(int w)
{
	return realZapperData[w];
}

static void StrobeRealZapper(int w)
{
	realZapperStrobe[w] = 0;
}

void UpdateRealZapper(int w, void* data, int arg)
{
	// In the '(*(uint32*)data)' variable, bit 0 holds the trigger value and bit 1 holds the light sense value.
	// Ultimately this needs to be converted from 0000 00lt to 000t l000 where l is the light bit and t
	// is the trigger bit.
	realZapperData[w] = ((((*(uint32*)data) & 1) << 4) | 
		                (((*(uint32*)data) & 2) << 2));
}

static INPUTC RealZapperCtrl = { ReadRealZapper,0,StrobeRealZapper,UpdateRealZapper,0,0 };

INPUTC* FCEU_InitRealZapper(int w)
{
	realZapperStrobe[w] = realZapperData[w] = 0;
	return(&RealZapperCtrl);
}
