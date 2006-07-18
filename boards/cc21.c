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
 */

#include "mapinc.h"

static uint16 addrlatche;

//------------------ UNLCC21 ---------------------------

static void UNLCC21Sync(void)
{
  setchr8(addrlatche&1);
  setmirror(MI_0+((addrlatche&2)>>1));
}

static DECLFW(UNLCC21Write)
{
  addrlatche=A;
  UNLCC21Sync();
}

static void UNLCC21Power(void)
{
  setprg32(0x8000,0);
  SetReadHandler(0x8000,0xFFFF,CartBR);
  SetWriteHandler(0x8000,0xffff,UNLCC21Write);
}

static void UNLCC21Restore(int version)
{
  UNLCC21Sync();
}

void UNLCC21_Init(CartInfo *info)
{
  info->Power=UNLCC21Power;
  GameStateRestore=UNLCC21Restore;
  AddExState(&addrlatche, 2, 0, "ALATC");
}
