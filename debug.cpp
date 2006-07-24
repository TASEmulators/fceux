#include "types.h"
#include "x6502.h"
#include "fceu.h"
#include "cart.h"
#include "ines.h"
#include "debug.h"
#include "driver.h"
#include "ppu.h"

#include <stdlib.h>

int GetPRGAddress(int A){
	unsigned int result;
	if((A < 0x8000) || (A > 0xFFFF))return -1;
	result = &Page[A>>11][A]-PRGptr[0];
	if((result > PRGsize[0]) || (result < 0))return -1;
	else return result;
}

int GetNesFileAddress(int A){
	unsigned int result;
	if((A < 0x8000) || (A > 0xFFFF))return -1;
	result = &Page[A>>11][A]-PRGptr[0];
	if((result > PRGsize[0]) || (result < 0))return -1;
	else return result+16; //16 bytes for the header remember
}

int GetRomAddress(int A){
	int i;
	uint8 *p = GetNesPRGPointer(A-=16);
	for(i = 16;i < 32;i++){
		if((&Page[i][i<<11] <= p) && (&Page[i][(i+1)<<11] > p))break;
	}
	if(i == 32)return -1; //not found

	return (i<<11) + (p-&Page[i][i<<11]);
}

uint8 *GetNesPRGPointer(int A){
	return PRGptr[0]+A;
}

uint8 *GetNesCHRPointer(int A){
	return CHRptr[0]+A;
}

uint8 GetMem(uint16 A) {
	if ((A >= 0x2000) && (A < 0x4000)) {
		switch (A&7) {
			case 0: return PPU[0];
			case 1: return PPU[1];
			case 2: return PPU[2]|(PPUGenLatch&0x1F);
			case 3: return PPU[3];
			case 4: return SPRAM[PPU[3]];
			case 5: return XOffset;
			case 6: return RefreshAddr&0xFF;
			case 7: return VRAMBuffer;
		}
	}
	else if ((A >= 0x4000) && (A < 0x6000)) return 0xFF; //fix me
	return ARead[A](A);
}

uint8 GetPPUMem(uint8 A) {
	uint16 tmp=RefreshAddr&0x3FFF;

	if (tmp<0x2000) return VPage[tmp>>10][tmp];
	if (tmp>=0x3F00) return PALRAM[tmp&0x1F];
	return vnapage[(tmp>>10)&0x3][tmp&0x3FF];
}


//---------------------

volatile int codecount, datacount, undefinedcount;
//HWND hCDLogger=0;
unsigned char *cdloggerdata;
char *cdlogfilename;
//char loadedcdfile[MAX_PATH];
static int indirectnext;

int debug_loggingCD;
//void FCEUI_SetLoggingCD(int val) { debug_loggingCD = val; }
//int FCEUI_GetLoggingCD(int val) { return debug_loggingCD = val; }

//called by the cpu to perform logging if CDLogging is enabled
void LogCDVectors(int which){
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

void LogCDData(){
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

//-----------debugger stuff

int badopbreak;
int iaPC;
uint32 iapoffset; //mbg merge 7/18/06 changed from int
int step,stepout,jsrcount;
int u; //deleteme
int skipdebug; //deleteme
int numWPs; 

void BreakHit() {
	FCEUI_SetEmulationPaused(1); //mbg merge 7/19/06 changed to use EmulationPaused()
	
	//MBG TODO - was this commented out before the gnu refactoring?
	//if((!logtofile) && (logging))PauseLoggingSequence();
	
	FCEUD_DebugBreakpoint();
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


void DebugCycle() {
	if (numWPs | step | stepout | watchpoint[64].flags | badopbreak) 
		breakpoint();
	if(debug_loggingCD) LogCDData();
	//mbg 6/30/06 - this was commented out when i got here. i dont understand it anyway
 	//if(logging || (hMemView && (EditingMode == 2))) LogInstruction();
	//if(logging) LogInstruction();
}
