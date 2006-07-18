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

static void Sync(void)
{
  if(addrlatche&0x40)
  {
    setprg16(0x8000,addrlatche&7);
    setprg16(0xC000,addrlatche&7);
  }
  else
    setprg32(0x8000,(addrlatche>>1)&3);
  setchr8((addrlatche>>3)&7);
  setmirror(((addrlatche&0x80)>>7)^1);
}

static DECLFW(BMCGK192Write)
{
  addrlatche=A;
  Sync();
}

static void BMCGK192Reset(void)
{
  setprg32(0x8000,0);
}

static void BMCGK192Power(void)
{
  setprg32(0x8000,0);
  SetReadHandler(0x8000,0xFFFF,CartBR);
  SetWriteHandler(0x8000,0xffff,BMCGK192Write);
}

static void StateRestore(int version)
{
  Sync();
}

void Mapper58_Init(CartInfo *info)
{
  info->Power=BMCGK192Power;
  info->Reset=BMCGK192Reset;
  GameStateRestore=StateRestore;
  AddExState(&addrlatche, 2, 0, "ALATC");
}
