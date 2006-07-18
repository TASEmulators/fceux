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

#include "mapinc.h"

static uint8 cmd;
static uint8 DRegs[8];
static SFORMAT StateRegs[]=
{
  {&cmd, 1, "CMD"},
  {DRegs, 8, "DREG"},
  {0}
};

static void Sync(void)
{
  setprg32(0x8000,(DRegs[0]<<4)|(DRegs[1]&0xF));
  setchr8(0);
}

static void StateRestore(int version)
{
  Sync();
}

static DECLFW(Write)
{
  switch (A&0x7300)
  {
    case 0x5100: DRegs[0]=V; Sync(); break;
    case 0x5000: DRegs[1]=V; Sync(); break;
  }
}

static DECLFW(Write2)
{
  switch (A&0x7300)
  {
    case 0x5200: DRegs[0]=V; Sync(); break;
    case 0x5000: DRegs[1]=V; Sync(); break;
  }
}

static uint8 WRAM[8192];
static DECLFR(AWRAM)
{
  return(WRAM[A-0x6000]);
}

static DECLFW(BWRAM)
{
  WRAM[A-0x6000]=V;
}

static void Power(void)
{
  memset(DRegs,0,8);
  DRegs[1]=0xFF;
  cmd=0;
  SetReadHandler(0x8000,0xFFFF,CartBR);
  SetWriteHandler(0x4020,0xFFFF,Write);
  SetReadHandler(0x6000,0x7FFF,AWRAM);
  SetWriteHandler(0x6000,0x7FFF,BWRAM);
  Sync();
}

static void M163HB(void)
{
  if(scanline==127&&DRegs[1]&0x80)
    setchr4(0x0000,1);
}


static void Power2(void)
{
  memset(DRegs,0,8);
  DRegs[1]=0xFF;
  cmd=0;
  SetReadHandler(0x8000,0xFFFF,CartBR);
  SetWriteHandler(0x4020,0xFFFF,Write2);
  SetReadHandler(0x6000,0x7FFF,AWRAM);
  SetWriteHandler(0x6000,0x7FFF,BWRAM);
  Sync();
}

void Mapper164_Init(CartInfo *info)
{
  info->Power=Power;
  GameStateRestore=StateRestore;
  AddExState(&StateRegs, ~0, 0, 0);
}

void Mapper163_Init(CartInfo *info)
{
  info->Power=Power2;
  GameHBIRQHook=M163HB;
  GameStateRestore=StateRestore;
  AddExState(&StateRegs, ~0, 0, 0);
}
