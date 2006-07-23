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

#include <string.h>

#include "types.h"
#include "x6502.h"
#include "fceu.h"
#include "sound.h"
//mbg merge 6/29/06
#include "debugger.h" //bbit edited: line added
#include "tracer.h" //bbit edited: line added
#include "memview.h" //bbit edited: line added
#include "cdlogger.h"

X6502 X;

//mbg merge 7/19/06 
//#ifdef FCEUDEF_DEBUGGER
//void (*X6502_Run)(int32 cycles);
//#endif

uint32 timestamp;
void FP_FASTAPASS(1) (*MapIRQHook)(int a);

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


//-------
//mbg merge 6/29/06
//bbit edited: a large portion of code was inserted from here on down
extern uint8 *XBuf;
//extern void FCEUD_BlitScreen(uint8 *XBuf);
void FCEUD_Update(uint8 *XBuf, int32 *Buffer, int Count); //mbg merge 6/30/06 - do this instead
static int indirectnext;

static INLINE void BreakHit() {
	DoDebug(1);
	FCEUI_SetEmulationPaused(1); //mbg merge 7/19/06 changed to use EmulationPaused()
	//if((!logtofile) && (logging))PauseLoggingSequence();
	UpdateLogWindow();
	if(hMemView)UpdateMemoryView(0);

	//mbg merge 6/30/06 - this architecture has changed
	FCEUD_Update(0,0,0);
	//FCEUD_BlitScreen(XBuf+8); //this looks odd, I know. but the pause routine is in here!!
	//if(logging)LogInstruction(); //logging might have been started while we were paused
}

static INLINE void LogCDVectors(int which){
	int i = 0xFFFA+(which*2);
	int j;
	j = GetPRGAddress(i);
	if(j == -1){
		return;
	}

	if(cdloggerdata[j] == 0){
	cdloggerdata[j] |= 0x0E; // we're in the last bank and recording it as data so 0x1110 or 0xE should be what we need
	datacount++;
	undefinedcount--;
	}
	j++;
	
	if(cdloggerdata[j] == 0){
	cdloggerdata[j] |= 0x0E; // we're in the last bank and recording it as data so 0x1110 or 0xE should be what we need
	datacount++;
	undefinedcount--;
	}

	return;
}



/*
	//very ineffecient, but this shouldn't get executed THAT much
	if(!(cdloggerdata[GetPRGAddress(0xFFFA)] & 2)){
		cdloggerdata[GetPRGAddress(0xFFFA)]|=2;
		codecount++;
		undefinedcount--;
	}
	if(!(cdloggerdata[GetPRGAddress(0xFFFB)] & 2)){
		cdloggerdata[GetPRGAddress(0xFFFB)]|=2;
		codecount++;
		undefinedcount--;
	}
	if(!(cdloggerdata[GetPRGAddress(0xFFFC)] & 2)){
		cdloggerdata[GetPRGAddress(0xFFFC)]|=2;
		codecount++;
		undefinedcount--;
	}
	if(!(cdloggerdata[GetPRGAddress(0xFFFD)] & 2)){
		cdloggerdata[GetPRGAddress(0xFFFD)]|=2;
		codecount++;
		undefinedcount--;
	}
	if(!(cdloggerdata[GetPRGAddress(0xFFFE)] & 2)){
		cdloggerdata[GetPRGAddress(0xFFFE)]|=2;
		codecount++;
		undefinedcount--;
	}
	if(!(cdloggerdata[GetPRGAddress(0xFFFF)] & 2)){
		cdloggerdata[GetPRGAddress(0xFFFF)]|=2;
		codecount++;
		undefinedcount--;
	}
	return;
}
*/
static INLINE void LogCDData(){
	int i, j;
	uint16 A=0; 
	uint8 opcode[3] = {0};

	j = GetPRGAddress(_PC);

	opcode[0] = GetMem(_PC);
	for (i = 1; i < opsize[opcode[0]]; i++) opcode[i] = GetMem(_PC+i);
	
	if(j != -1){
		for (i = 0; i < opsize[opcode[0]]; i++){
			if(cdloggerdata[j+i] & 1)continue; //this has been logged so skip
			cdloggerdata[j+i] |= 1;
			cdloggerdata[j+i] |=((_PC+i)>>11)&12;
			if(indirectnext)cdloggerdata[j+i] |= 0x10;
			codecount++; 
			if(!(cdloggerdata[j+i] & 0x42))undefinedcount--;
		}
	}
	indirectnext = 0;
	//log instruction jumped to in an indirect jump
	if(opcode[0] == 0x6c){
		indirectnext = 1;
	}

	switch (optype[opcode[0]]) {
		case 0: break;
		case 1:
			A = (opcode[1]+_X) & 0xFF;
			A = GetMem(A) | (GetMem(A+1))<<8;
			break;
		case 2: A = opcode[1]; break;
		case 3: A = opcode[1] | opcode[2]<<8; break;
		case 4: A = (GetMem(opcode[1]) | (GetMem(opcode[1]+1))<<8)+_Y; break;
		case 5: A = opcode[1]+_X; break;
		case 6: A = (opcode[1] | opcode[2]<<8)+_Y; break;
		case 7: A = (opcode[1] | opcode[2]<<8)+_X; break;
		case 8: A = opcode[1]+_Y; break;
	}

	//if(opbrktype[opcode[0]] != WP_R)return; //we only want reads

	if((j = GetPRGAddress(A)) == -1)return;
	//if(j == 0)BreakHit();


	if(cdloggerdata[j] & 2)return; 
	cdloggerdata[j] |= 2;
	cdloggerdata[j] |=((A/*+i*/)>>11)&12;
	if((optype[opcode[0]] == 1) || (optype[opcode[0]] == 4))cdloggerdata[j] |= 0x20;
	datacount++; 
	if(!(cdloggerdata[j+i] & 1))undefinedcount--;
	return;
}

// ################################## Start of SP CODE ###########################

// Returns the value of a given type or register

int getValue(int type)
{
	switch (type)
	{
		case 'A': return _A;
		case 'X': return _X;
		case 'Y': return _Y;
		case 'N': return _P & N_FLAG ? 1 : 0;
		case 'V': return _P & V_FLAG ? 1 : 0;
		case 'U': return _P & U_FLAG ? 1 : 0;
		case 'B': return _P & B_FLAG ? 1 : 0;
		case 'D': return _P & D_FLAG ? 1 : 0;
		case 'I': return _P & I_FLAG ? 1 : 0;
		case 'Z': return _P & Z_FLAG ? 1 : 0;
		case 'C': return _P & C_FLAG ? 1 : 0;
		case 'P': return _PC;
	}

	return 0;
}

// Evaluates a condition
int evaluate(Condition* c)
{
	int f = 0;

	int value1, value2;

	if (c->lhs)
	{
		value1 = evaluate(c->lhs);
	}
	else
	{
		switch(c->type1)
		{
			case TYPE_ADDR:
			case TYPE_NUM: value1 = c->value1; break;
			default: value1 = getValue(c->value1);
		}
	}

	if (c->type1 == TYPE_ADDR)
	{
		value1 = GetMem(value1);
	}

	f = value1;

	if (c->op)
	{
		if (c->rhs)
		{
			value2 = evaluate(c->rhs);
		}
		else
		{
			switch(c->type2)
			{
				case TYPE_ADDR:
				case TYPE_NUM: value2 = c->value2; break;
				default: value2 = getValue(c->type2);
			}
		}

		if (c->type2 == TYPE_ADDR)
		{
			value2 = GetMem(value2);
		}

		switch (c->op)
		{
			case OP_EQ: f = value1 == value2; break;
			case OP_NE: f = value1 != value2; break;
			case OP_GE: f = value1 >= value2; break;
			case OP_LE: f = value1 <= value2; break;
			case OP_G: f = value1 > value2; break;
			case OP_L: f = value1 < value2; break;
			case OP_MULT: f = value1 * value2; break;
			case OP_DIV: f = value1 / value2; break;
			case OP_PLUS: f = value1 + value2; break;
			case OP_MINUS: f = value1 - value2; break;
			case OP_OR: f = value1 || value2; break;
			case OP_AND: f = value1 && value2; break;
		}
	}

	return f;
}

int condition(watchpointinfo* wp)
{
	return wp->cond == 0 || evaluate(wp->cond);
}

// ################################## End of SP CODE ###########################

//extern int step;
//extern int stepout;
//extern int jsrcount;
void breakpoint() {
	int i;
	uint16 A=0;
	uint8 brk_type,opcode[3] = {0};

	opcode[0] = GetMem(_PC);
	
	if(badopbreak && (opsize[opcode[0]] == 0))BreakHit();

	if (stepout) {
		if (opcode[0] == 0x20) jsrcount++;
		else if (opcode[0] == 0x60) {
			if (jsrcount) jsrcount--;
			else {
				stepout = 0;
				step = 1;
				return;
			}
		}
	}
	if (step) {
		step = 0;
		BreakHit();
		return;
	}
	if ((watchpoint[64].address == _PC) && (watchpoint[64].flags)) {
		watchpoint[64].address = 0;
		watchpoint[64].flags = 0;
		BreakHit();
		return;
	}


	for (i = 1; i < opsize[opcode[0]]; i++) opcode[i] = GetMem(_PC+i);
	brk_type = opbrktype[opcode[0]] | WP_X;
	switch (optype[opcode[0]]) {
		case 0: /*A = _PC;*/ break;
		case 1:
			A = (opcode[1]+_X) & 0xFF;
			A = GetMem(A) | (GetMem(A+1))<<8;
			break;
		case 2: A = opcode[1]; break;
		case 3: A = opcode[1] | opcode[2]<<8; break;
		case 4: A = (GetMem(opcode[1]) | (GetMem(opcode[1]+1))<<8)+_Y; break;
		case 5: A = opcode[1]+_X; break;
		case 6: A = (opcode[1] | opcode[2]<<8)+_Y; break;
		case 7: A = (opcode[1] | opcode[2]<<8)+_X; break;
		case 8: A = opcode[1]+_Y; break;
	}

	for (i = 0; i < numWPs; i++) {
// ################################## Start of SP CODE ###########################
		if (condition(&watchpoint[i]))
		{
// ################################## End of SP CODE ###########################
			if (watchpoint[i].flags & BT_P) { //PPU Mem breaks
				if ((watchpoint[i].flags & WP_E) && (watchpoint[i].flags & brk_type) && ((A >= 0x2000) && (A < 0x4000)) && ((A&7) == 7)) {
					if (watchpoint[i].endaddress) {
						if ((watchpoint[i].address <= RefreshAddr) && (watchpoint[i].endaddress >= RefreshAddr)) BreakHit();
					}
					else if (watchpoint[i].address == RefreshAddr) BreakHit();
				}
			}
			else if (watchpoint[i].flags & BT_S) { //Sprite Mem breaks
				if ((watchpoint[i].flags & WP_E) && (watchpoint[i].flags & brk_type) && ((A >= 0x2000) && (A < 0x4000)) && ((A&7) == 4)) {
					if (watchpoint[i].endaddress) {
						if ((watchpoint[i].address <= PPU[3]) && (watchpoint[i].endaddress >= PPU[3])) BreakHit();
					}
					else if (watchpoint[i].address == PPU[3]) BreakHit();
				}
				else if ((watchpoint[i].flags & WP_E) && (watchpoint[i].flags & WP_W) && (A == 0x4014)) BreakHit(); //Sprite DMA! :P
			}
			else { //CPU mem breaks
				if ((watchpoint[i].flags & WP_E) && (watchpoint[i].flags & brk_type)) {
					if (watchpoint[i].endaddress) {
						if (((!(watchpoint[i].flags & WP_X)) && (watchpoint[i].address <= A) && (watchpoint[i].endaddress >= A)) ||
							((watchpoint[i].flags & WP_X) && (watchpoint[i].address <= _PC) && (watchpoint[i].endaddress >= _PC))) BreakHit();
					}
					else if (((!(watchpoint[i].flags & WP_X)) && (watchpoint[i].address == A)) ||
							((watchpoint[i].flags & WP_X) && (watchpoint[i].address == _PC))) BreakHit();
				}
			}
// ################################## Start of SP CODE ###########################
		}
// ################################## End of SP CODE ###########################
	}
}
//bbit edited: this is the end of the inserted code

#define ADDCYC(x) \
{     \
 int __x=x;       \
 _tcount+=__x;    \
 _count-=__x*48;  \
 timestamp+=__x;  \
}


//mbg 6/30/06 - hooked functions arent being used right now
////hooked memory read
//static INLINE uint8 RdMemHook(unsigned int A)
//{
// if(X.ReadHook)
//  return(_DB = X.ReadHook(&X,A) );
// else
//  return(_DB=ARead[A](A));
//}
//
////hooked memory write
//static INLINE void WrMemHook(unsigned int A, uint8 V)
//{
// if(X.WriteHook)
//  X.WriteHook(&X,A,V);
// else
//  BWrite[A](A,V);
//}

//#define RdRAMFast(A) (_DB=RAM[(A)])
//#define WrRAMFast(A,V) RAM[(A)]=(V)

//normal memory read
static INLINE uint8 RdMemNorm(unsigned int A)
{
 return(_DB=ARead[A](A));
}

//normal memory write
static INLINE void WrMemNorm(unsigned int A, uint8 V)
{
 BWrite[A](A,V);
}

static INLINE uint8 RdRAMNorm(unsigned int A) 
{
  //bbit edited: this was changed so cheat substituion would work
  return(_DB=ARead[A](A));
  // return(_DB=RAM[A]); 
}

static INLINE void WrRAMNorm(unsigned int A, uint8 V)
{
 RAM[A]=V;
}

uint8 FASTAPASS(1) X6502_DMR(uint32 A)
{
 ADDCYC(1);
 return(X.DB=ARead[A](A));
}

void FASTAPASS(2) X6502_DMW(uint32 A, uint8 V)
{
 ADDCYC(1);
 BWrite[A](A,V);
}

#define PUSH(V) \
{       \
 uint8 VTMP=V;  \
 WrRAM(0x100+_S,VTMP);  \
 _S--;  \
}       

#define POP() RdRAM(0x100+(++_S))

static uint8 ZNTable[256];
/* Some of these operations will only make sense if you know what the flag
   constants are. */

#define X_ZN(zort)      _P&=~(Z_FLAG|N_FLAG);_P|=ZNTable[zort]
#define X_ZNT(zort)  _P|=ZNTable[zort]

#define JR(cond);  \
{    \
 if(cond)  \
 {  \
  uint32 tmp;  \
  int32 disp;  \
  disp=(int8)RdMem(_PC);  \
  _PC++;  \
  ADDCYC(1);  \
  tmp=_PC;  \
  _PC+=disp;  \
  if((tmp^_PC)&0x100)  \
  ADDCYC(1);  \
 }  \
 else _PC++;  \
}


#define LDA     _A=x;X_ZN(_A)
#define LDX     _X=x;X_ZN(_X)
#define LDY  _Y=x;X_ZN(_Y)

/*  All of the freaky arithmetic operations. */
#define AND  _A&=x;X_ZN(_A)
#define BIT  _P&=~(Z_FLAG|V_FLAG|N_FLAG);_P|=ZNTable[x&_A]&Z_FLAG;_P|=x&(V_FLAG|N_FLAG)
#define EOR  _A^=x;X_ZN(_A)
#define ORA  _A|=x;X_ZN(_A)

#define ADC  {  \
        uint32 l=_A+x+(_P&1);  \
        _P&=~(Z_FLAG|C_FLAG|N_FLAG|V_FLAG);  \
        _P|=((((_A^x)&0x80)^0x80) & ((_A^l)&0x80))>>1;  \
        _P|=(l>>8)&C_FLAG;  \
        _A=l;  \
        X_ZNT(_A);  \
       }

#define SBC  {  \
        uint32 l=_A-x-((_P&1)^1);  \
        _P&=~(Z_FLAG|C_FLAG|N_FLAG|V_FLAG);  \
        _P|=((_A^l)&(_A^x)&0x80)>>1;  \
        _P|=((l>>8)&C_FLAG)^C_FLAG;  \
        _A=l;  \
        X_ZNT(_A);  \
       }

#define CMPL(a1,a2) {  \
         uint32 t=a1-a2;  \
         X_ZN(t&0xFF);  \
         _P&=~C_FLAG;  \
         _P|=((t>>8)&C_FLAG)^C_FLAG;  \
		    }

/* Special undocumented operation.  Very similar to CMP. */
#define AXS      {  \
                     uint32 t=(_A&_X)-x;    \
                     X_ZN(t&0xFF);      \
                     _P&=~C_FLAG;       \
         _P|=((t>>8)&C_FLAG)^C_FLAG;  \
         _X=t;  \
        }

#define CMP    CMPL(_A,x)
#define CPX    CMPL(_X,x)
#define CPY          CMPL(_Y,x)

/* The following operations modify the byte being worked on. */
#define DEC         x--;X_ZN(x)
#define INC    x++;X_ZN(x)

#define ASL  _P&=~C_FLAG;_P|=x>>7;x<<=1;X_ZN(x)
#define LSR  _P&=~(C_FLAG|N_FLAG|Z_FLAG);_P|=x&1;x>>=1;X_ZNT(x)

/* For undocumented instructions, maybe for other things later... */
#define LSRA  _P&=~(C_FLAG|N_FLAG|Z_FLAG);_P|=_A&1;_A>>=1;X_ZNT(_A)

#define ROL  {  \
     uint8 l=x>>7;  \
     x<<=1;  \
     x|=_P&C_FLAG;  \
     _P&=~(Z_FLAG|N_FLAG|C_FLAG);  \
     _P|=l;  \
     X_ZNT(x);  \
    }
#define ROR  {  \
     uint8 l=x&1;  \
     x>>=1;  \
     x|=(_P&C_FLAG)<<7;  \
     _P&=~(Z_FLAG|N_FLAG|C_FLAG);  \
     _P|=l;  \
     X_ZNT(x);  \
		}
		 
/* Icky icky thing for some undocumented instructions.  Can easily be
   broken if names of local variables are changed.
*/

/* Absolute */
#define GetAB(target)   \
{  \
 target=RdMem(_PC);  \
 _PC++;  \
 target|=RdMem(_PC)<<8;  \
 _PC++;  \
}

/* Absolute Indexed(for reads) */
#define GetABIRD(target, i)  \
{  \
 unsigned int tmp;  \
 GetAB(tmp);  \
 target=tmp;  \
 target+=i;  \
 if((target^tmp)&0x100)  \
 {  \
  target&=0xFFFF;  \
  RdMem(target^0x100);  \
  ADDCYC(1);  \
 }  \
}

/* Absolute Indexed(for writes and rmws) */
#define GetABIWR(target, i)  \
{  \
 unsigned int rt;  \
 GetAB(rt);  \
 target=rt;  \
 target+=i;  \
 target&=0xFFFF;  \
 RdMem((target&0x00FF)|(rt&0xFF00));  \
}

/* Zero Page */
#define GetZP(target)  \
{  \
 target=RdMem(_PC);   \
 _PC++;  \
}

/* Zero Page Indexed */
#define GetZPI(target,i)  \
{  \
 target=i+RdMem(_PC);  \
 _PC++;  \
}

/* Indexed Indirect */
#define GetIX(target)  \
{  \
 uint8 tmp;  \
 tmp=RdMem(_PC);  \
 _PC++;  \
 tmp+=_X;  \
 target=RdRAM(tmp);  \
 tmp++;    \
 target|=RdRAM(tmp)<<8;  \
}

/* Indirect Indexed(for reads) */
#define GetIYRD(target)  \
{  \
 unsigned int rt;  \
 uint8 tmp;  \
 tmp=RdMem(_PC);  \
 _PC++;  \
 rt=RdRAM(tmp);  \
 tmp++;  \
 rt|=RdRAM(tmp)<<8;  \
 target=rt;  \
 target+=_Y;  \
 if((target^rt)&0x100)  \
 {  \
  target&=0xFFFF;  \
  RdMem(target^0x100);  \
  ADDCYC(1);  \
 }  \
}

/* Indirect Indexed(for writes and rmws) */
#define GetIYWR(target)  \
{  \
 unsigned int rt;  \
 uint8 tmp;  \
 tmp=RdMem(_PC);  \
 _PC++;  \
 rt=RdRAM(tmp);  \
 tmp++;  \
 rt|=RdRAM(tmp)<<8;  \
 target=rt;  \
 target+=_Y;  \
 target&=0xFFFF; \
 RdMem((target&0x00FF)|(rt&0xFF00));  \
}

/* Now come the macros to wrap up all of the above stuff addressing mode functions
   and operation macros.  Note that operation macros will always operate(redundant
   redundant) on the variable "x".
*/

#define RMW_A(op) {uint8 x=_A; op; _A=x; break; } /* Meh... */
#define RMW_AB(op) {unsigned int A; uint8 x; GetAB(A); x=RdMem(A); WrMem(A,x); op; WrMem(A,x); break; }
#define RMW_ABI(reg,op) {unsigned int A; uint8 x; GetABIWR(A,reg); x=RdMem(A); WrMem(A,x); op; WrMem(A,x); break; }
#define RMW_ABX(op)  RMW_ABI(_X,op)
#define RMW_ABY(op)  RMW_ABI(_Y,op)
#define RMW_IX(op)  {unsigned int A; uint8 x; GetIX(A); x=RdMem(A); WrMem(A,x); op; WrMem(A,x); break; }
#define RMW_IY(op)  {unsigned int A; uint8 x; GetIYWR(A); x=RdMem(A); WrMem(A,x); op; WrMem(A,x); break; }
#define RMW_ZP(op)  {uint8 A; uint8 x; GetZP(A); x=RdRAM(A); op; WrRAM(A,x); break; }
#define RMW_ZPX(op) {uint8 A; uint8 x; GetZPI(A,_X); x=RdRAM(A); op; WrRAM(A,x); break;}

#define LD_IM(op)  {uint8 x; x=RdMem(_PC); _PC++; op; break;}
#define LD_ZP(op)  {uint8 A; uint8 x; GetZP(A); x=RdRAM(A); op; break;}
#define LD_ZPX(op)  {uint8 A; uint8 x; GetZPI(A,_X); x=RdRAM(A); op; break;}
#define LD_ZPY(op)  {uint8 A; uint8 x; GetZPI(A,_Y); x=RdRAM(A); op; break;}
#define LD_AB(op)  {unsigned int A; uint8 x; GetAB(A); x=RdMem(A); op; break; }
#define LD_ABI(reg,op)  {unsigned int A; uint8 x; GetABIRD(A,reg); x=RdMem(A); op; break;}
#define LD_ABX(op)  LD_ABI(_X,op)
#define LD_ABY(op)  LD_ABI(_Y,op)
#define LD_IX(op)  {unsigned int A; uint8 x; GetIX(A); x=RdMem(A); op; break;}
#define LD_IY(op)  {unsigned int A; uint8 x; GetIYRD(A); x=RdMem(A); op; break;}

#define ST_ZP(r)  {uint8 A; GetZP(A); WrRAM(A,r); break;}
#define ST_ZPX(r)  {uint8 A; GetZPI(A,_X); WrRAM(A,r); break;}
#define ST_ZPY(r)  {uint8 A; GetZPI(A,_Y); WrRAM(A,r); break;}
#define ST_AB(r)  {unsigned int A; GetAB(A); WrMem(A,r); break;}
#define ST_ABI(reg,r)  {unsigned int A; GetABIWR(A,reg); WrMem(A,r); break; }
#define ST_ABX(r)  ST_ABI(_X,r)
#define ST_ABY(r)  ST_ABI(_Y,r)
#define ST_IX(r)  {unsigned int A; GetIX(A); WrMem(A,r); break; }
#define ST_IY(r)  {unsigned int A; GetIYWR(A); WrMem(A,r); break; }

static uint8 CycTable[256] =
{                             
/*0x00*/ 7,6,2,8,3,3,5,5,3,2,2,2,4,4,6,6,
/*0x10*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
/*0x20*/ 6,6,2,8,3,3,5,5,4,2,2,2,4,4,6,6,
/*0x30*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
/*0x40*/ 6,6,2,8,3,3,5,5,3,2,2,2,3,4,6,6,
/*0x50*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
/*0x60*/ 6,6,2,8,3,3,5,5,4,2,2,2,5,4,6,6,
/*0x70*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
/*0x80*/ 2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
/*0x90*/ 2,6,2,6,4,4,4,4,2,5,2,5,5,5,5,5,
/*0xA0*/ 2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
/*0xB0*/ 2,5,2,5,4,4,4,4,2,4,2,4,4,4,4,4,
/*0xC0*/ 2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
/*0xD0*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
/*0xE0*/ 2,6,3,8,3,3,5,5,2,2,2,2,4,4,6,6,
/*0xF0*/ 2,5,2,8,4,4,6,6,2,4,2,7,4,4,7,7,
};

void FASTAPASS(1) X6502_IRQBegin(int w)
{
 _IRQlow|=w;
}

void FASTAPASS(1) X6502_IRQEnd(int w)
{
 _IRQlow&=~w;
}

void TriggerNMI(void)
{
 _IRQlow|=FCEU_IQNMI;
}

void TriggerNMI2(void)
{ 
 _IRQlow|=FCEU_IQNMI2;
}

#ifdef FCEUDEF_DEBUGGER
/* Called from debugger. */
void FCEUI_NMI(void)
{
 _IRQlow|=FCEU_IQNMI;
}

void FCEUI_IRQ(void)
{
 _IRQlow|=FCEU_IQTEMP;
}

void FCEUI_GetIVectors(uint16 *reset, uint16 *irq, uint16 *nmi)
{
 fceuindbg=1;

 *reset=RdMemNorm(0xFFFC);
 *reset|=RdMemNorm(0xFFFD)<<8;
 *nmi=RdMemNorm(0xFFFA);
 *nmi|=RdMemNorm(0xFFFB)<<8;
 *irq=RdMemNorm(0xFFFE);
 *irq|=RdMemNorm(0xFFFF)<<8;
 fceuindbg=0;
}
static int debugmode;
#endif

void X6502_Reset(void)
{
 _IRQlow=FCEU_IQRESET;
}
  
void X6502_Init(void)
{
	int x;

	memset((void *)&X,0,sizeof(X));
	for(x=0;x<256;x++)
	 if(!x) ZNTable[x]=Z_FLAG;
   else if(x&0x80) ZNTable[x]=N_FLAG;
	 else ZNTable[x]=0;
	#ifdef FCEUDEF_DEBUGGER
	X6502_Debug(0,0,0);
	#endif
}

void X6502_Power(void)
{
 _count=_tcount=_IRQlow=_PC=_A=_X=_Y=_S=_P=_PI=_DB=_jammed=0;
 timestamp=0;
 X6502_Reset();
}

//mbg 6/30/06 merge - this function reworked significantly for merge
void X6502_Run(int32 cycles)
{
  #define RdRAM RdRAMNorm
  #define WrRAM WrRAMNorm
  #define RdMem RdMemNorm
  #define WrMem WrMemNorm

  if(PAL)
   cycles*=15;    // 15*4=60
  else
   cycles*=16;    // 16*4=64

  _count+=cycles;

  while(_count>0)
  {
   int32 temp;
   uint8 b1;

   if(_IRQlow)
   {
    if(_IRQlow&FCEU_IQRESET)
    {
	if(loggingcodedata)LogCDVectors(0);
     _PC=RdMem(0xFFFC);
     _PC|=RdMem(0xFFFD)<<8;
     _jammed=0;
     _PI=_P=I_FLAG;
     _IRQlow&=~FCEU_IQRESET;
    }
    else if(_IRQlow&FCEU_IQNMI2)
     {
     _IRQlow&=~FCEU_IQNMI2;
     _IRQlow|=FCEU_IQNMI;
    }
    else if(_IRQlow&FCEU_IQNMI)
    {
     if(!_jammed)
     {
      ADDCYC(7);
      PUSH(_PC>>8);
      PUSH(_PC);
      PUSH((_P&~B_FLAG)|(U_FLAG));
      _P|=I_FLAG;
	  if(loggingcodedata)LogCDVectors(1);
      _PC=RdMem(0xFFFA);
      _PC|=RdMem(0xFFFB)<<8;
      _IRQlow&=~FCEU_IQNMI;
     }
    }
    else
    {
     if(!(_PI&I_FLAG) && !_jammed)
     {
      ADDCYC(7);
      PUSH(_PC>>8);
      PUSH(_PC);
      PUSH((_P&~B_FLAG)|(U_FLAG));
      _P|=I_FLAG;
	  if(loggingcodedata)LogCDVectors(1);
      _PC=RdMem(0xFFFE);
      _PC|=RdMem(0xFFFF)<<8;
     }
    }
    _IRQlow&=~(FCEU_IQTEMP);
    if(_count<=0)
    {
     _PI=_P;
     return;
     } //Should increase accuracy without a
              //major speed hit.
   }

   	//will probably cause a major speed decrease on low-end systems
    //bbit edited: this line added
	if (numWPs | step | stepout | watchpoint[64].flags | badopbreak) 
		breakpoint();
	if(loggingcodedata)LogCDData();
	//mbg 6/30/06 - this was commented out when i got here. i dont understand it anyway
 	//if(logging || (hMemView && (EditingMode == 2))) LogInstruction();
	if(logging) LogInstruction();
	//---

   _PI=_P;
   b1=RdMem(_PC);

   ADDCYC(CycTable[b1]);

   temp=_tcount;
   _tcount=0;
   if(MapIRQHook) MapIRQHook(temp);
   FCEU_SoundCPUHook(temp);
   _PC++;
   switch(b1)
   {
    #include "ops.h"
   }
  }

  #undef RdRAM
  #undef WrRAM
  #undef RdMem
  #undef WrMem
}


void X6502_Debug(void (*CPUHook)(X6502 *),
    uint8 (*ReadHook)(X6502 *, unsigned int),
    void (*WriteHook)(X6502 *, unsigned int, uint8))
{
 //mbg 6/30/06 - stubbed out because we don't support the hooked cpu
 //debugmode=(ReadHook || WriteHook || CPUHook)?1:0;
 //X.ReadHook=ReadHook;
 //X.WriteHook=WriteHook;
 //X.CPUHook=CPUHook;
}

//mbg 6/30/06 - the non-hooked cpu core and core switching has been removed to mimic XD.
//this could be put back in later
/*

//#ifdef FCEUDEF_DEBUGGER
//X6502 XSave;     // This is getting ugly. 
//#define RdMemHook(A)  ( X.ReadHook?(_DB=X.ReadHook(&X,A)):(_DB=ARead[A](A)) )
//#define WrMemHook(A,V)  { if(X.WriteHook) X.WriteHook(&X,A,V); else BWrite[A](A,V); }


#ifdef FCEUDEF_DEBUGGER
void (*X6502_Run)(int32 cycles);
#endif

#ifdef FCEUDEF_DEBUGGER
static void X6502_RunDebug(int32 cycles)
{
  #define RdRAM RdMemHook
  #define WrRAM WrMemHook
  #define RdMem RdMemHook
  #define WrMem WrMemHook

  if(PAL)
   cycles*=15;    // 15*4=60
  else
   cycles*=16;    // 16*4=64

  _count+=cycles;

  while(_count>0)
  {
   int32 temp;
   uint8 b1;

   if(_IRQlow)
   {
    if(_IRQlow&FCEU_IQRESET)
    {
     _PC=RdMem(0xFFFC);
     _PC|=RdMem(0xFFFD)<<8;
     _jammed=0;
     _PI=_P=I_FLAG;
     _IRQlow&=~FCEU_IQRESET;
    }
    else if(_IRQlow&FCEU_IQNMI2)
    {
     _IRQlow&=~FCEU_IQNMI2;
     _IRQlow|=FCEU_IQNMI;
    }
    else if(_IRQlow&FCEU_IQNMI)
    {
     if(!_jammed)
     {
      ADDCYC(7);
      PUSH(_PC>>8);
      PUSH(_PC);
      PUSH((_P&~B_FLAG)|(U_FLAG));
      _P|=I_FLAG;
	  if(loggingcodedata)LogCDVectors(1); //mbg 6/29/06 
      _PC=RdMem(0xFFFA);
      _PC|=RdMem(0xFFFB)<<8;
      _IRQlow&=~FCEU_IQNMI;
     }
    }
    else
    {
     if(!(_PI&I_FLAG) && !_jammed)
     {
      ADDCYC(7);
      PUSH(_PC>>8);
      PUSH(_PC);
      PUSH((_P&~B_FLAG)|(U_FLAG));
      _P|=I_FLAG;
	  if(loggingcodedata)LogCDVectors(1); //mbg 6/29/06 
      _PC=RdMem(0xFFFE);
      _PC|=RdMem(0xFFFF)<<8;
     }
    }
    _IRQlow&=~(FCEU_IQTEMP);
    if(_count<=0)
    {
     _PI=_P;
     return;
    } //Should increase accuracy without a
       //major speed hit.
   }

   //---
   //mbg merge 6/29/06
   	//will probably cause a major speed decrease on low-end systems
	if (numWPs | step | stepout | watchpoint[64].flags | badopbreak) breakpoint(); //bbit edited: this line added
	if(loggingcodedata)LogCDData();
 	if(logging
		//|| (hMemView && (EditingMode == 2))
		)LogInstruction();
	//---

   if(X.CPUHook) X.CPUHook(&X);
   //Ok, now the real fun starts.
   //Do the pre-exec voodoo.
   if(X.ReadHook || X.WriteHook)
   {
    uint32 tsave=timestamp;
    XSave=X;

    fceuindbg=1;
    X.preexec=1;
    b1=RdMem(_PC);
    _PC++;
    switch(b1)
    {
     #include "ops.h"
    }

    timestamp=tsave;

    //In case an NMI/IRQ/RESET was triggered by the debugger.
    //Should we also copy over the other hook variables?
    XSave.IRQlow=X.IRQlow;
    XSave.ReadHook=X.ReadHook;
    XSave.WriteHook=X.WriteHook;
    XSave.CPUHook=X.CPUHook;
    X=XSave;
    fceuindbg=0;
   }

   _PI=_P;
   b1=RdMem(_PC);
   ADDCYC(CycTable[b1]);

   temp=_tcount;
   _tcount=0;
   if(MapIRQHook) MapIRQHook(temp);

   FCEU_SoundCPUHook(temp);

   _PC++;
   switch(b1)
   {
    #include "ops.h"
   }
  }
  #undef RdRAM
  #undef WrRAM
  #undef RdMem
  #undef WrMem

}


static void X6502_RunNormal(int32 cycles)
#else
void X6502_Run(int32 cycles)
#endif
{
  #define RdRAM RdRAMFast
  #define WrRAM WrRAMFast
  #define RdMem RdMemNorm
  #define WrMem WrMemNorm

  //#if(defined(C80x86) && defined(__GNUC__))
  //// Gives a nice little speed boost.
  //register uint16 pbackus asm ("edi");
  //#else
  //uint16 pbackus;
  //#endif

  //pbackus=_PC;

  //#undef _PC
  //#define _PC pbackus

  if(PAL)
   cycles*=15;    // 15*4=60
  else
   cycles*=16;    // 16*4=64

  _count+=cycles;

  while(_count>0)
  {
   int32 temp;
   uint8 b1;

//   XI.PC=pbackus;
   if(_IRQlow)
   {
    if(_IRQlow&FCEU_IQRESET)
    {
     _PC=RdMem(0xFFFC);
     _PC|=RdMem(0xFFFD)<<8;
     _jammed=0;
     _PI=_P=I_FLAG;
     _IRQlow&=~FCEU_IQRESET;
    }
    else if(_IRQlow&FCEU_IQNMI2)
     {
     _IRQlow&=~FCEU_IQNMI2;
     _IRQlow|=FCEU_IQNMI;
    }
    else if(_IRQlow&FCEU_IQNMI)
    {
     if(!_jammed)
     {
      ADDCYC(7);
      PUSH(_PC>>8);
      PUSH(_PC);
      PUSH((_P&~B_FLAG)|(U_FLAG));
      _P|=I_FLAG;
	  if(loggingcodedata)LogCDVectors(1); //mbg 6/29/06 
      _PC=RdMem(0xFFFA);
      _PC|=RdMem(0xFFFB)<<8;
      _IRQlow&=~FCEU_IQNMI;
     }
    }
    else
    {
     if(!(_PI&I_FLAG) && !_jammed)
     {
      ADDCYC(7);
      PUSH(_PC>>8);
      PUSH(_PC);
      PUSH((_P&~B_FLAG)|(U_FLAG));
      _P|=I_FLAG;
	  if(loggingcodedata)LogCDVectors(1); //mbg 6/29/06 
      _PC=RdMem(0xFFFE);
      _PC|=RdMem(0xFFFF)<<8;
     }
    }
    _IRQlow&=~(FCEU_IQTEMP);
    if(_count<=0)
    {
     _PI=_P;
     //X.PC=pbackus;
     return;
     } //Should increase accuracy without a
              //major speed hit.
   }

   //---
   //mbg merge 6/29/06
   	//will probably cause a major speed decrease on low-end systems
	if (numWPs | step | stepout | watchpoint[64].flags | badopbreak) 
		breakpoint(); //bbit edited: this line added
	if(loggingcodedata)LogCDData();
 	if(logging
	// || (hMemView && (EditingMode == 2))
		)
		LogInstruction();
	//---

   _PI=_P;
   b1=RdMem(_PC);

   ADDCYC(CycTable[b1]);

   temp=_tcount;
   _tcount=0;
   if(MapIRQHook) MapIRQHook(temp);
   FCEU_SoundCPUHook(temp);
   //X.PC=pbackus;
   _PC++;
   switch(b1)
   {
    #include "ops.h"
   }
  }

  //#undef _PC
  //#define _PC X.PC
  //_PC=pbackus;
  //#undef RdRAM
  #undef WrRAM
}

#ifdef FCEUDEF_DEBUGGER
void X6502_Debug(void (*CPUHook)(X6502 *),
    uint8 (*ReadHook)(X6502 *, unsigned int),
    void (*WriteHook)(X6502 *, unsigned int, uint8))
{
 debugmode=(ReadHook || WriteHook || CPUHook)?1:0;
 X.ReadHook=ReadHook;
 X.WriteHook=WriteHook;
 X.CPUHook=CPUHook;

 if(!debugmode)
  X6502_Run=X6502_RunNormal;
 else
  X6502_Run=X6502_RunDebug;
}

#endif*/
