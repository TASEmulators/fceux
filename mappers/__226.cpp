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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/*
#include "mapinc.h"

#define rg mapbyte1
static void DoPRG(void)
{
  int32 b=0x10|(((rg[0]>>1)&0xF) | ((rg[0]>>3)&0x10) | ((rg[1]&1)<<5));
  if(rg[0]&0x20)        // 16 KB
  {
   ROM_BANK16(0x8000,(b<<1)|(rg[0]&1));
   ROM_BANK16(0xC000,(b<<1)|(rg[0]&1));
  }
  else
   ROM_BANK32(b);
}

static DECLFW(Mapper226_write)
{
 rg[A&1]=V;
 DoPRG();
 if(A&1)
 {
  if(rg[1]&2)
   PPUCHRRAM=0;  // Write protected.
  else
   PPUCHRRAM=0xFF; // Not write protected.
 }
 else
  MIRROR_SET2((rg[0]>>6)&1);
}

static void M26Reset(void)
{
 rg[0]=rg[1]=0;
 DoPRG();
 PPUCHRRAM=0xFF;
 MIRROR_SET2(0);
}

static void M26Restore(int version)
{
 DoPRG();
 if(rg[1]&2)
  PPUCHRRAM=0;  // Write protected.
 else
  PPUCHRRAM=0xFF; // Not write protected.
 MIRROR_SET2((rg[0]>>6)&1);
}

void Mapper226_init(void)
{
  SetWriteHandler(0x8000,0xffff,Mapper226_write);
  MapperReset=M26Reset;
  GameStateRestore=M26Restore;
  M26Reset();
}
*/