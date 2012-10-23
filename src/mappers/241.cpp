/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2003 Xodnizel
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

static DECLFW(M241wr)
{
//  FCEU_printf("Wr: $%04x:$%02x, $%04x\n",A,V,X.PC);
 if(A<0x8000)
 {
// printf("$%04x:$%02x, $%04x\n",A,V,X.PC);
 }
 else
  ROM_BANK32(V);
}

static DECLFR(M241rd)
{
 //DumpMem("out",0x8000,0xffff);
 //printf("Rd: $%04x, $%04x\n",A,X.PC);
 return(0x50);
}

void Mapper241_init(void)
{
 ROM_BANK32(0);
 SetWriteHandler(0x5000,0x5fff,M241wr);
 SetWriteHandler(0x8000,0xFFFF,M241wr);
 SetReadHandler(0x4020,0x5fff,M241rd);
}
