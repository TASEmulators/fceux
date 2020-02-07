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

static uint32 vbrsb[2];
static uint32 vbrdata[2];

static uint8 ReadVB(int w)
{
	uint8 ret=0;
	ret |= (vbrdata[w]>>vbrsb[w])&1;
	if(vbrsb[w] >= 16)
	{
		ret|=0x1;
		vbrsb[w] = 16;
	}
	if(!fceuindbg)
		vbrsb[w]++;
	return ret;
}

static void StrobeVB(int w)
{
	vbrsb[w]=0;
}

void UpdateVB(int w, void *data, int arg)
{
	vbrdata[w]=0;
	for (int x=0;x<14;x++)
	{
		vbrdata[w]|=(((*(uint32 *)data)>>x)&1)<<x;
	}
	vbrdata[w]|=(1<<14); // fixed signature bit
}

static INPUTC VirtualBoyCtrl={ReadVB,0,StrobeVB,UpdateVB,0,0};

INPUTC *FCEU_InitVirtualBoy(int w)
{
	vbrsb[w]=vbrdata[w]=0;
	return(&VirtualBoyCtrl);
}
