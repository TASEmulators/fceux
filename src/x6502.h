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

#ifndef _X6502H

#include "x6502struct.h"

extern X6502 X;
#define _PC        X.PC
#define _A         X.A
#define _X         X.X
#define _Y         X.Y
#define _S         X.S
#define _P         X.P
#define _PI        X.mooPI
#define _DB        X.DB
#define _count     X.count
#define _tcount    X.tcount
#define _IRQlow    X.IRQlow
#define _jammed    X.jammed


//the opsize table is used to quickly grab the instruction sizes (in bytes)
static const uint8 opsize[256] = {
/*0x00*/	1,2,0,0,0,2,2,0,1,2,1,0,0,3,3,0,
/*0x10*/	2,2,0,0,0,2,2,0,1,3,0,0,0,3,3,0,
/*0x20*/	3,2,0,0,2,2,2,0,1,2,1,0,3,3,3,0,
/*0x30*/	2,2,0,0,0,2,2,0,1,3,0,0,0,3,3,0,
/*0x40*/	1,2,0,0,0,2,2,0,1,2,1,0,3,3,3,0,
/*0x50*/	2,2,0,0,0,2,2,0,1,3,0,0,0,3,3,0,
/*0x60*/	1,2,0,0,0,2,2,0,1,2,1,0,3,3,3,0,
/*0x70*/	2,2,0,0,0,2,2,0,1,3,0,0,0,3,3,0,
/*0x80*/	0,2,0,0,2,2,2,0,1,0,1,0,3,3,3,0,
/*0x90*/	2,2,0,0,2,2,2,0,1,3,1,0,0,3,0,0,
/*0xA0*/	2,2,2,0,2,2,2,0,1,2,1,0,3,3,3,0,
/*0xB0*/	2,2,0,0,2,2,2,0,1,3,1,0,3,3,3,0,
/*0xC0*/	2,2,0,0,2,2,2,0,1,2,1,0,3,3,3,0,
/*0xD0*/	2,2,0,0,0,2,2,0,1,3,0,0,0,3,3,0,
/*0xE0*/	2,2,0,0,2,2,2,0,1,2,1,0,3,3,3,0,
/*0xF0*/	2,2,0,0,0,2,2,0,1,3,0,0,0,3,3,0
};


/*
the optype table is a quick way to grab the addressing mode for any 6502 opcode

  0 = Implied\Accumulator\Immediate\Branch\NULL
  1 = (Indirect,X)
  2 = Zero Page
  3 = Absolute
  4 = (Indirect),Y
  5 = Zero Page,X
  6 = Absolute,Y
  7 = Absolute,X
  8 = Zero Page,Y
*/
static const uint8 optype[256] = {
/*0x00*/	0,1,0,0,0,2,2,0,0,0,0,0,0,3,3,0,
/*0x10*/	0,4,0,0,0,5,5,0,0,6,0,0,0,7,7,0,
/*0x20*/	0,1,0,0,2,2,2,0,0,0,0,0,3,3,3,0,
/*0x30*/	0,4,0,0,0,5,5,0,0,6,0,0,0,7,7,0,
/*0x40*/	0,1,0,0,0,2,2,0,0,0,0,0,0,3,3,0,
/*0x50*/	0,4,0,0,0,5,5,0,0,6,0,0,0,7,7,0,
/*0x60*/	0,1,0,0,0,2,2,0,0,0,0,0,3,3,3,0,
/*0x70*/	0,4,0,0,0,5,5,0,0,6,0,0,0,7,7,0,
/*0x80*/	0,1,0,0,2,2,2,0,0,0,0,0,3,3,3,0,
/*0x90*/	0,4,0,0,5,5,8,0,0,6,0,0,0,7,0,0,
/*0xA0*/	0,1,0,0,2,2,2,0,0,0,0,0,3,3,3,0,
/*0xB0*/	0,4,0,0,5,5,8,0,0,6,0,0,7,7,6,0,
/*0xC0*/	0,1,0,0,2,2,2,0,0,0,0,0,3,3,3,0,
/*0xD0*/	0,4,0,0,0,5,5,0,0,6,0,0,0,7,7,0,
/*0xE0*/	0,1,0,0,2,2,2,0,0,0,0,0,3,3,3,0,
/*0xF0*/	0,4,0,0,0,5,5,0,0,6,0,0,0,7,7,0
};

//-----------
//mbg 6/30/06 - some of this was removed to mimic XD
//#ifdef FCEUDEF_DEBUGGER
void X6502_Debug(void (*CPUHook)(X6502 *),
    uint8 (*ReadHook)(X6502 *, unsigned int),
    void (*WriteHook)(X6502 *, unsigned int, uint8));

//extern void (*X6502_Run)(int32 cycles);
//#else
//void X6502_Run(int32 cycles);
//#endif
void X6502_RunDebug(int32 cycles);
#define X6502_Run(x) X6502_RunDebug(x)
//------------

extern uint32 timestamp;



#define N_FLAG  0x80
#define V_FLAG  0x40
#define U_FLAG  0x20
#define B_FLAG  0x10
#define D_FLAG  0x08
#define I_FLAG  0x04
#define Z_FLAG  0x02
#define C_FLAG  0x01

extern void (*MapIRQHook)(int a);

#define NTSC_CPU 1789772.7272727272727272
#define PAL_CPU  1662607.125

#define FCEU_IQEXT      0x001
#define FCEU_IQEXT2     0x002
/* ... */
#define FCEU_IQRESET    0x020
#define FCEU_IQNMI2  0x040  // Delayed NMI, gets converted to *_IQNMI
#define FCEU_IQNMI  0x080
#define FCEU_IQDPCM     0x100
#define FCEU_IQFCOUNT   0x200
#define FCEU_IQTEMP     0x800

void X6502_Init(void);
void X6502_Reset(void);
void X6502_Power(void);

void TriggerNMI(void);
void TriggerNMI2(void);

uint8 X6502_DMR(uint32 A);
void X6502_DMW(uint32 A, uint8 V);

void X6502_IRQBegin(int w);
void X6502_IRQEnd(int w);

#define _X6502H
#endif
