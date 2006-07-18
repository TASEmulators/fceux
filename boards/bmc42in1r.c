/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2005 CaH4e3
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
 *
 * BMC 42-in-1 reset switch
 */

#include "mapinc.h"

static uint8 hrd_sw;
static uint8 latche;
static SFORMAT StateRegs[]=
{
  {&latche, 1, "LATCHE"},
  {&hrd_sw, 1, "HRDSW"},
  {0}
};

static void Sync(void)
{
  if(!(latche&0x20))
    setprg32r(hrd_sw,0x8000,(latche>>1)&0x0f);
  else
  {
    setprg16r(hrd_sw,0x8000,latche&0x1f);
    setprg16r(hrd_sw,0xC000,latche&0x1f);
  }
  switch((latche>>6)&3)
  {
    case 0: setmirrorw(0,0,0,1); break;
    case 1: setmirror(MI_V); break;
    case 2: setmirror(MI_H); break;
    case 3: setmirror(MI_1); break;
  }
}

static DECLFW(BMC42in1rWrite)
{
  latche=V;
  Sync();
}

static void BMC42in1rReset(void)
{
  hrd_sw^=1;
  Sync();
}

static void BMC42in1rPower(void)
{
  latche=0x00;
  hrd_sw=0;
  setchr8(0);
  Sync();
  SetWriteHandler(0x8000,0xFFFF,BMC42in1rWrite);
  SetReadHandler(0x8000,0xFFFF,CartBR);
}

static void StateRestore(int version)
{
  Sync();
}

void BMC42in1r_Init(CartInfo *info)
{
  info->Power=BMC42in1rPower;
  info->Reset=BMC42in1rReset;
  AddExState(&StateRegs, ~0, 0, 0);
  GameStateRestore=StateRestore;
}


