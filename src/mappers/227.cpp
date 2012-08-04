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

#include "mapinc.h"

//zero 14-apr-2012 - redid this entirely to match bizhawk

#define rg mapbyte1

static void DoSync(uint32 A)
{
	int S = A & 1;
	int M_horz = (A>>1)&1;
	int p = (A >> 2) & 0x1F;
	p += (A&0x100) ? 0x20 : 0;
	bool o = (A>>7)&1;
	bool L = (A>>9)&1;

	if (o && !S )	
	{
		ROM_BANK16(0x8000,p);
		ROM_BANK16(0xC000,p);
	}
	if (o && S )
	{
		ROM_BANK16(0x8000,p);
		ROM_BANK16(0xC000,p+1);
	}
	if (!o && !S && !L )
	{
		ROM_BANK16(0x8000,p);
		ROM_BANK16(0xC000,p&0x38);
	}
	if (!o && S && !L )
	{
		ROM_BANK16(0x8000,p&0x3E);
		ROM_BANK16(0xC000,p&0x38);
	}
	if (!o && !S && L)
	{
		ROM_BANK16(0x8000,p);
		ROM_BANK16(0xC000,p|7);
	}
	if (!o && S && L )
	{
		ROM_BANK16(0x8000,p&0x3E);
		ROM_BANK16(0xC000,p|7);
	}

 rg[0]=A;
 rg[1]=A>>8;

 MIRROR_SET((A>>1)&1);
}

static DECLFW(Mapper227_write)
{
 DoSync(A);
}

static void M227Reset(void)
{
 DoSync(0);
}

static void M227Restore(int version)
{
 DoSync(rg[0]|(rg[1]<<8));
}

void Mapper227_init(void)
{
  SetWriteHandler(0x8000,0xffff,Mapper227_write);
  MapperReset=M227Reset;
  GameStateRestore=M227Restore;
  M227Reset();
}
