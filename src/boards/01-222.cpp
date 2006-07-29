/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2006 CaH4e3
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

#include "mapinc.h"

static uint8 reg[4];
static SFORMAT StateRegs[]=
{
  {reg, 4, "REGS"},
  {0}
};

static void Sync(void)
{
  setprg32(0x8000,(reg[2]>>2)&1);
  setchr8(reg[2]&3);
}

static DECLFW(UNL22211WriteLo)
{
//  FCEU_printf("bs %04x %02x\n",A,V);
  reg[A&3]=V;
}

static DECLFW(UNL22211WriteHi)
{
//  FCEU_printf("bs %04x %02x\n",A,V);
  Sync();
}

static DECLFR(UNL22211ReadLo)
{
  if(reg[3])
    return reg[2];
  else
    return X.DB;
}

static void UNL22211Power(void)
{
  Sync();
  SetReadHandler(0x8000,0xFFFF,CartBR);
  SetReadHandler(0x4100,0x4100,UNL22211ReadLo);
  SetWriteHandler(0x4100,0x4103,UNL22211WriteLo);
  SetWriteHandler(0x8000,0xFFFF,UNL22211WriteHi);
}

static void StateRestore(int version)
{
  Sync();
}

void UNL22211_Init(CartInfo *info)
{
  info->Power=UNL22211Power;
  GameStateRestore=StateRestore;
  AddExState(&StateRegs, ~0, 0, 0);
}
