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
 * CAI Shogakko no Sansu
 */

#include "mapinc.h"

static uint8 *CHRRAM=NULL;
static uint8 SWRAM[4096];

static uint8 regs[16];
static uint8 WRAM[4096];
static SFORMAT StateRegs[]=
{
  {&regs, 16, "REGS"},
  {WRAM, 4096, "WRAM"},
  {0}
};

static void Sync(void)
{
  if(regs[5]&0x40)
  {
    setchr4r(0,0x1000,regs[5]&0x3F);
  }
  else
  {
    setchr4r(0x10,0x0000,regs[5]);
    setchr4r(0x10,0x1000,regs[5]^1);
  }
  setprg8r((regs[2]>>6)&1,0x8000,(regs[2]&0x3F));
  setprg8r((regs[3]>>6)&1,0xA000,(regs[3]&0x3F));
  setprg8r((regs[4]>>6)&1,0xC000,(regs[4]&0x3F));
  setprg8r(1,0xE000,~0);
  setmirror((regs[0xA]&3));
}

static DECLFW(M190Write)
{
// FCEU_printf("write %04x:%04x %d, %d\n",A,V,scanline,timestamp);
  regs[(A&0x0F00)>>8]=V;
  Sync();
}

static DECLFR(M190Read)
{
// FCEU_printf("read %04x:%04x %d, %d\n",A,regs[(A&0x0F00)>>8],scanline,timestamp);
  return regs[(A&0x0F00)>>8];
}

static DECLFR(AWRAM)
{
  return(WRAM[A-0x7000]);
}
static DECLFW(BWRAM)
{
  WRAM[A-0x7000]=V;
}

static DECLFR(ASWRAM)
{
  return(SWRAM[A-0x6000]);
}
static DECLFW(BSWRAM)
{
  SWRAM[A-0x6000]=V;
}

static void M190Power(void)
{
  setvram8(CHRRAM);
  SetReadHandler(0x8000,0xFFFF,CartBR);
  SetWriteHandler(0x8000,0xFFFF,M190Write);
// SetReadHandler(0xDA00,0xDA00,M190Read);
// SetReadHandler(0xDB00,0xDB00,M190Read);
  SetReadHandler(0xDC00,0xDC00,M190Read);
  SetReadHandler(0xDD00,0xDD00,M190Read);
  SetReadHandler(0x7000,0x7FFF,AWRAM);
  SetWriteHandler(0x7000,0x7FFF,BWRAM);
  SetReadHandler(0x6000,0x6FFF,ASWRAM);
  SetWriteHandler(0x6000,0x6FFF,BSWRAM);
  Sync();
}

static void M190Close(void)
{
  if(CHRRAM)
    FCEU_gfree(CHRRAM);
  CHRRAM=NULL;
}

static void StateRestore(int version)
{
  Sync();
}

void Mapper190_Init(CartInfo *info)
{
  info->Power=M190Power;
  info->Close=M190Close;
  if(info->battery)
  {
    info->SaveGame[0]=SWRAM;
    info->SaveGameLen[0]=4096;
  }
  GameStateRestore=StateRestore;
  CHRRAM=(uint8*)FCEU_gmalloc(8192);
  SetupCartCHRMapping(0x10,CHRRAM,8192,1);
  AddExState(CHRRAM, 8192, 0, "CHRRAM");
  AddExState(&StateRegs, ~0, 0, 0);
}
