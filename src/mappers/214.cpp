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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "mapinc.h"

static DECLFW(Mapper214_write)
{
// FCEU_printf("%02x:%02x\n",A,V);
 ROM_BANK16(0x8000,(A>>2)&3);
 ROM_BANK16(0xC000,(A>>2)&3);
 VROM_BANK8(A&3);
}

void Mapper214_init(void)
{
 ROM_BANK16(0x8000,0);
 ROM_BANK16(0xC000,0);
 VROM_BANK8(0);
 SetWriteHandler(0x8000,0xFFFF,Mapper214_write);
}
