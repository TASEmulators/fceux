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

static uint8 chrcmd[8], prg0, prg1, brk, mirr;
static uint8 reg[8], cmd;
static uint8 IRQCount,IRQLatch,IRQa;
static uint8 IRQReload;
static SFORMAT StateRegs[]=
{
  {reg, 8, "MMCREG"},
  {&cmd, 1, "MMCCMD"},
  {chrcmd, 8, "CHRCMD"},
  {&prg0, 1, "PRG0"},
  {&prg1, 1, "PRG1"},
  {&brk, 1, "BRK"},
  {&mirr, 1, "MIRR"},
  {&IRQReload, 1, "IRQR"},
  {&IRQCount, 1, "IRQC"},
  {&IRQLatch, 1, "IRQL"},
  {&IRQa, 1, "IRQA"},
  {0}
};

static void Sync(void)
{
//  if(brk&2)
//  {
//    setprg16(0x8000,~0);
//    setprg16(0xC000,~0); 
//  }
//  else
  {
    setprg8(0x8000,prg0);
    setprg8(0xA000,prg1);
  }
  int i;
  for(i=0; i<8; i++)
     setchr1(i<<10,chrcmd[i]);
  setmirror(mirr^1);
}

static void Sync2(void)
{
  setprg8(0x8000,reg[6]&0x3F);
  setprg8(0xA000,reg[7]&0x3F);
  setchr2(0x0000,reg[0]>>1);
  setchr2(0x8000,reg[1]>>1);
  setchr1(0x1000,reg[2]);
  setchr1(0x1400,reg[3]);
  setchr1(0x1800,reg[4]);
  setchr1(0x1C00,reg[5]);
  setmirror(mirr^1);
}

static DECLFW(UNLSL1632CMDWrite)
{
  FCEU_printf("bs %04x %02x\n",A,V);     
//  if((A&0xA131)==0xA131) brk=V;
  if((A&0xA131)==0xA131) brk=V;
  if(brk==2)
  {
  switch(A&0xE001)
  {
    case 0x8000: cmd=V&7; break;
    case 0x8001: reg[cmd]=V; Sync(); break;
    case 0xA000: mirr=V&1; break;
    case 0xC000: IRQLatch=V; break;
    case 0xC001: IRQReload=1; break;
    case 0xE000: X6502_IRQEnd(FCEU_IQEXT); IRQa=0; break;
    case 0xE001: IRQa=1; break;
  }
  Sync2();
  }
  else
  {
  switch(A&0xF003)
  {
    case 0x8000: prg0=V; break;
    case 0xA000: prg1=V; break;
    case 0x9000: mirr=V&1; break;
    case 0xB000: chrcmd[0]=(chrcmd[0]&0xF0)|(V&0x0F); break;
    case 0xB001: chrcmd[0]=(chrcmd[0]&0x0F)|(V<<4); break;
    case 0xB002: chrcmd[1]=(chrcmd[1]&0xF0)|(V&0x0F); break;
    case 0xB003: chrcmd[1]=(chrcmd[1]&0x0F)|(V<<4); break;
    case 0xC000: chrcmd[2]=(chrcmd[2]&0xF0)|(V&0x0F); break;
    case 0xC001: chrcmd[2]=(chrcmd[2]&0x0F)|(V<<4); break;
    case 0xC002: chrcmd[3]=(chrcmd[3]&0xF0)|(V&0x0F); break;
    case 0xC003: chrcmd[3]=(chrcmd[3]&0x0F)|(V<<4); break;
    case 0xD000: chrcmd[4]=(chrcmd[4]&0xF0)|(V&0x0F); break;
    case 0xD001: chrcmd[4]=(chrcmd[4]&0x0F)|(V<<4); break;
    case 0xD002: chrcmd[5]=(chrcmd[5]&0xF0)|(V&0x0F); break;
    case 0xD003: chrcmd[5]=(chrcmd[5]&0x0F)|(V<<4); break;
    case 0xE000: chrcmd[6]=(chrcmd[6]&0xF0)|(V&0x0F); break;
    case 0xE001: chrcmd[6]=(chrcmd[6]&0x0F)|(V<<4); break;
    case 0xE002: chrcmd[7]=(chrcmd[7]&0xF0)|(V&0x0F); break;
    case 0xE003: chrcmd[7]=(chrcmd[7]&0x0F)|(V<<4); break;
  }
  Sync();
  }
}

static void UNLSL1632IRQHook(void)
{
 int count = IRQCount;
 if((scanline==128)&&IRQa)X6502_IRQBegin(FCEU_IQEXT);
 if(!count || IRQReload)
 {
    IRQCount = IRQLatch;
    IRQReload = 0;
 }
 else
    IRQCount--;
 if(!IRQCount)
 {
    if(IRQa)
    {
       X6502_IRQBegin(FCEU_IQEXT);
    }
 }
}

static void StateRestore(int version)
{
  Sync();
}

static void UNLSL1632Power(void)
{
  setprg16(0xC000,~0);
  SetReadHandler(0x8000,0xFFFF,CartBR);
  SetWriteHandler(0x8000,0xFFFF,UNLSL1632CMDWrite);
}

void UNLSL1632_Init(CartInfo *info)
{
  info->Power=UNLSL1632Power;
  GameHBIRQHook2=UNLSL1632IRQHook;
  GameStateRestore=StateRestore;
  AddExState(&StateRegs, ~0, 0, 0);
}
