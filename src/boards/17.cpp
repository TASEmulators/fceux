/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2012 CaH4e3
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

static uint8 preg[4], creg[8];
static uint8 IRQa, mirr;
static int32 IRQCount, IRQLatch;

static SFORMAT StateRegs[]=
{
  {preg, 4, "PREG"},
  {creg, 8, "CREG"},
  {&mirr, 1, "MIRR"},
  {&IRQa, 1, "IRQA"},
  {&IRQCount, 4, "IRQC"},
  {&IRQLatch, 4, "IRQL"},
  {0}
};

static void Sync(void)
{
  int i;
  for(i=0; i<8; i++) setchr1(i<<10,creg[i]);
  setprg8(0x8000,preg[0]);
  setprg8(0xA000,preg[1]);
  setprg8(0xC000,preg[2]);
  setprg8(0xE000,preg[3]);
  switch(mirr) {
    case 0: setmirror(MI_0); break;
    case 1: setmirror(MI_1); break;
    case 2: setmirror(MI_H); break;
    case 3: setmirror(MI_V); break;
  }
}

static DECLFW(M17WriteMirr)
{
  mirr = ((A << 1) & 2)|((V >> 4) & 1);
  Sync();
}

static DECLFW(M17WriteIRQ)
{
  switch(A) {
    case 0x4501: IRQa=0; X6502_IRQEnd(FCEU_IQEXT); break;
    case 0x4502: IRQCount&=0xFF00; IRQCount|=V; break;
    case 0x4503: IRQCount&=0x00FF; IRQCount|=V<<8; IRQa=1; break;
  }
}

static DECLFW(M17WritePrg)
{
  preg[A & 3] = V;
  Sync();
}

static DECLFW(M17WriteChr)
{
  creg[A & 7] = V;
  Sync();
}

static void M17Power(void)
{
  preg[3] = ~0;
  Sync();
  SetReadHandler(0x8000,0xFFFF,CartBR);
  SetWriteHandler(0x42FE,0x42FF,M17WriteMirr);
  SetWriteHandler(0x4500,0x4503,M17WriteIRQ);
  SetWriteHandler(0x4504,0x4507,M17WritePrg);
  SetWriteHandler(0x4510,0x4517,M17WriteChr);
}

static void M17IRQHook(int a)
{
  if(IRQa)
  {
    IRQCount+=a;
    if(IRQCount>=0x10000)
    {
      X6502_IRQBegin(FCEU_IQEXT);
      IRQa=0;
      IRQCount=0;
    }
  }
}

static void StateRestore(int version)
{
  Sync();
}

void Mapper17_Init(CartInfo *info)
{
  info->Power=M17Power;
  MapIRQHook=M17IRQHook;
  GameStateRestore=StateRestore;

  AddExState(&StateRegs, ~0, 0, 0);
}

