/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2009 CaH4e3
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

static uint8 reg[7];
static uint8 *WRAM=NULL;
static uint32 WRAMSIZE;

static SFORMAT StateRegs[]=
{
  {reg, 2, "REG"},
  {0}
};

static uint8 bs_tbl[128] = {
  0x03, 0x13, 0x23, 0x33, 0x03, 0x13, 0x23, 0x33, 0x03, 0x13, 0x23, 0x33, 0x03, 0x13, 0x23, 0x33,
  0x45, 0x67, 0x45, 0x67, 0x45, 0x67, 0x45, 0x67, 0x45, 0x67, 0x45, 0x67, 0x45, 0x67, 0x45, 0x67,
  0x03, 0x13, 0x23, 0x33, 0x03, 0x13, 0x23, 0x33, 0x03, 0x13, 0x23, 0x33, 0x03, 0x13, 0x23, 0x33,
  0x47, 0x67, 0x47, 0x67, 0x47, 0x67, 0x47, 0x67, 0x47, 0x67, 0x47, 0x67, 0x47, 0x67, 0x47, 0x67,
  0x02, 0x12, 0x22, 0x32, 0x02, 0x12, 0x22, 0x32, 0x02, 0x12, 0x22, 0x32, 0x02, 0x12, 0x22, 0x32,
  0x45, 0x67, 0x45, 0x67, 0x45, 0x67, 0x45, 0x67, 0x45, 0x67, 0x45, 0x67, 0x45, 0x67, 0x45, 0x67,
  0x02, 0x12, 0x22, 0x32, 0x02, 0x12, 0x22, 0x32, 0x02, 0x12, 0x22, 0x32, 0x00, 0x10, 0x20, 0x30,
  0x47, 0x67, 0x47, 0x67, 0x47, 0x67, 0x47, 0x67, 0x47, 0x67, 0x47, 0x67, 0x47, 0x67, 0x47, 0x67,
};

static uint8 br_tbl[16] = {
  0x00, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x20, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02,
};

static void Sync(void)
{
//  setchr4(0x0000,(reg[0]&0x80) >> 7);
//  setchr4(0x1000,(reg[0]&0x80) >> 7);
  setchr8(0);
  setprg8r(0x10,0x6000,0);
  setprg16(0x8000,bs_tbl[reg[0]&0x7f]>>4);
  setprg16(0xc000,bs_tbl[reg[0]&0x7f]&0xf);
  setmirror(MI_V);
}

static DECLFW(UNLPEC586Write)
{
  reg[(A&0x700)>>8]=V;
  FCEU_printf("bs %04x %02x\n",A,V);
  Sync();
}

static DECLFR(UNLPEC586Read)
{
  FCEU_printf("read %04x\n",A);
  return (X.DB & 0xD8) | br_tbl[reg[4]>>4];
}

static void UNLPEC586Power(void)
{
  reg[0]=0x0E;
  Sync();
  setchr8(0);
  SetReadHandler(0x6000,0x7FFF,CartBR);
  SetWriteHandler(0x6000,0x7FFF,CartBW);
  SetReadHandler(0x8000,0xFFFF,CartBR);
  SetWriteHandler(0x5000,0x5fff,UNLPEC586Write);
  SetReadHandler(0x5000,0x5fff,UNLPEC586Read);
}

static void UNLPEC586IRQ(void)
{
//  if(reg[0]&0x80)
  {
    if(scanline==128) {
      setchr4(0x0000,1);
      setchr4(0x1000,0);
    }
    else {
      setchr4(0x0000,0);
      setchr4(0x1000,1);
    }
  }
}

static void UNLPEC586Close(void)
{
  if(WRAM)
    FCEU_gfree(WRAM);
  WRAM=NULL;
}

static void StateRestore(int version)
{
  Sync();
}

void UNLPEC586Init(CartInfo *info)
{
  info->Power=UNLPEC586Power;
  info->Close=UNLPEC586Close;
  GameHBIRQHook=UNLPEC586IRQ;
  GameStateRestore=StateRestore;

  WRAMSIZE=8192;
  WRAM=(uint8*)FCEU_gmalloc(WRAMSIZE);
  SetupCartPRGMapping(0x10,WRAM,WRAMSIZE,1);
  AddExState(WRAM, WRAMSIZE, 0, "WRAM");

  AddExState(&StateRegs, ~0, 0, 0);
}
