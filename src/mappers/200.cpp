/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2003 CaH4e3
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

static DECLFW(Mapper200_write)
{
//  FCEU_printf("%04x, %02x\n",A,V);
  ROM_BANK16(0x8000,A&0x07);
  ROM_BANK16(0xC000,A&0x07);
  VROM_BANK8(A&0x07);
  MIRROR_SET((A&0x08)>>3);
}

void Mapper200_init(void)
{
  setprg16(0x8000,~0);
  setprg16(0xC000,~0);
  SetWriteHandler(0x8000,0xffff,Mapper200_write);
}

