/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 CaH4e3
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

static uint8 prg_reg;
static uint8 chr_reg;

static uint8 sim0reg, sim0bit, sim0byte, sim0parity, sim0bcnt;
static uint8 sim0bitw, sim0bytew, sim0parityw, sim0bcntw;

static uint16 sim0data, sim0dataw, sim0iswrite;
static uint8 sim0array[128] =
{
  0x14, 0x55, 0x45, 0xd3, 0x18, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
  0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
  0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
  0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
  0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
  0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
  0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10,
  0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0xAA,
};
static uint8 sim0cmd[16];

static SFORMAT StateRegs[]=
{
  {&prg_reg, 1, "PREG"},
  {&chr_reg, 1, "CREG"},
  {0}
};

static void Sync(void)
{
  setprg32(0x8000, prg_reg);
  setchr8(chr_reg);
}

static void StateRestore(int version)
{
  Sync();
}

static DECLFW(M216WriteHi)
{
// FCEU_printf("%04x:%04x\n",A,V);
  prg_reg=A&1;
  chr_reg=(A&0x0E)>>1;
  Sync();
}

static DECLFW(M216Write5000)
{
  sim0reg=V;
  if(!sim0reg)
  {
    sim0bit=sim0byte=sim0parity=0;
    sim0bitw=sim0bytew=sim0parityw=0;
    sim0data=sim0array[0];
    sim0bcnt=0x7F;
  }
  if(sim0reg&0x20)
  {
    sim0bcnt=0x1E;
    sim0bcntw=4;
  }
  if(sim0reg&0x08)
  {
    uint8 sim0in=0;
    sim0iswrite=0x2C;
    if(sim0bitw<8)
    {
      sim0in=(V&0x10)>>4;
      sim0parityw+=sim0in;
      sim0dataw|=sim0in<<7;
      sim0dataw>>=1;
      sim0bitw++;
    }
    else if(sim0bitw==8)
    {
      sim0bitw++;
    }
    else if(sim0bitw==9)
    {
      sim0parityw=0;
      sim0bitw=0;
      if(sim0bytew==sim0bcntw)
      {
        sim0reg=0x60;
        sim0iswrite=0;
      }
      else
      {
        sim0cmd[sim0bytew++]=sim0dataw;
        sim0dataw=0;
      }
    }
  }
                 
  FCEU_printf("WRITE: %04x:%04x (PC=%02x cnt=%02x)\n",A,V,X.PC,sim0bcnt);
}

static DECLFR(M216Read5000)
{
  if(sim0reg&0x60)
  {
    if(sim0iswrite)
       sim0iswrite--;
    else
    sim0reg=(sim0reg^(sim0reg<<1))&0x40;
    FCEU_printf("READ: %04x PC=%04x reg=%02x\n",A,X.PC,sim0reg);
    return sim0reg;
  }
  else
  {
    uint8 sim0out=0;
    if(sim0bit<8)
    {
      sim0out=(sim0data&0x80)>>7;
      sim0parity+=sim0out;
      sim0out<<=6;
      sim0data<<=1;
      sim0bit++;
    }
    else if(sim0bit==8)
    {
      sim0bit++;
      sim0out=(sim0parity&1)<<6;
    }
    else if(sim0bit==9)
    {
      sim0parity=0;
        sim0bit=0;
      if(sim0byte==sim0bcnt)
      {
        sim0reg=0x40;
        sim0out=0x60;
      }
      else
      {
        sim0out=0;
        sim0data=sim0array[sim0byte++];
      }
    }
    FCEU_printf("READ: %04x PC=%04x out=%02x byte=%02x cnt=%02x bit=%02x\n",A,X.PC,sim0out,sim0byte,sim0bcnt,sim0bit);
    return sim0out;
  }
}

static void Power(void)
{
  prg_reg = 0;
  chr_reg = 0;
  Sync();
  SetReadHandler(0x8000,0xFFFF,CartBR);
  SetWriteHandler(0x8000,0xFFFF,M216WriteHi);
  SetWriteHandler(0x5000,0x5000,M216Write5000);
  SetReadHandler(0x5000,0x5000,M216Read5000);
}


void Mapper216_Init(CartInfo *info)
{
  info->Power=Power;
  GameStateRestore=StateRestore;
  AddExState(&StateRegs, ~0, 0, 0);
}
