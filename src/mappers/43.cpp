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
/*
#include "mapinc.h"



static DECLFW(Mapper43_write)
{
 //printf("$%04x:$%02x\n",A,V);
 if((A&0x8122)==0x8122)
 {
  X6502_IRQEnd(FCEU_IQEXT);
  if(V&2) IRQa=1;
  else
   IRQCount=IRQa=0;
 }
}

static DECLFW(M43Low)
{
 int transo[8]={4,3,4,4,4,7,5,6};
// int transo[8]={4,3,2,3,4,7,5,6};
 A&=0xF0FF;
 if(A==0x4022)
  setprg8(0x6000,transo[V&7]);
 //printf("$%04x:$%02x\n",A,V);
}

static void FP_FASTAPASS(1) M43Ho(int a)
{
 IRQCount+=a;
 if(IRQa)
  if(IRQCount>=4096)
  {
   X6502_IRQBegin(FCEU_IQEXT);
  }
}

//static DECLFR(boo)
//{
// printf("$%04x\n",A);
// return( ROM[0x2000*8 +0x1000 +(A-0x5000)]);
//}

void Mapper43_init(void)
{
 setprg4(0x5000,16);
 setprg8(0x6000,2);
 setprg8(0x8000,1);
 setprg8(0xa000,0);
 setprg8(0xc000,4);
 setprg8(0xe000,9);
 SetWriteHandler(0x8000,0xffff,Mapper43_write);
 SetWriteHandler(0x4020,0x7fff,M43Low);
 //SetReadHandler(0x5000,0x5fff,boo);
 SetReadHandler(0x6000,0xffff,CartBR);
 MapIRQHook=M43Ho;
}
*/