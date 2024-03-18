/// \file
/// \brief Implements core debugging facilities
#include "types.h"
#include "x6502.h"
#include "fceu.h"
#include "cart.h"
#include "ines.h"
#include "debug.h"
#include "debugsymboltable.h"
#include "driver.h"
#include "ppu.h"

#include "x6502abbrev.h"

#include <cstdlib>
#include <cstring>

unsigned int debuggerPageSize = 14;
int vblankScanLines = 0;	//Used to calculate scanlines 240-261 (vblank)
int vblankPixel = 0;		//Used to calculate the pixels in vblank


struct TraceInstructionCallback
{
	void (*func)(uint8 *opcode, int size) = nullptr;
	TraceInstructionCallback* next = nullptr;
};
static TraceInstructionCallback* traceInstructionCB = nullptr;

int offsetStringToInt(unsigned int type, const char* offsetBuffer, bool *conversionOk)
{
	int offset = -1;

	if (conversionOk)
	{
		*conversionOk = false;
	}

	if (sscanf(offsetBuffer,"%7X",(unsigned int *)&offset) == EOF)
	{
		return -1;
	}

	if (type & BT_P)
	{
		if (conversionOk)
		{
			*conversionOk = (offset >= 0) && (offset < 0x4000);
		}
		return offset & 0x3FFF;
	}
	else if (type & BT_S)
	{
		if (conversionOk)
		{
			*conversionOk = (offset >= 0) && (offset < 0x100);
		}
		return offset & 0x00FF;
	}
	else if (type & BT_R)
	{
		if (conversionOk)
		{
			*conversionOk = (offset >= 0);
		}
		return offset;
	}
	else // BT_C
	{
		auto sym = debugSymbolTable.getSymbolAtAnyBank(offsetBuffer);

		if (sym)
		{
			if (conversionOk)
			{
				*conversionOk = true;
			}
			return sym->offset() & 0xFFFF;
		}

		int type = GIT_CART;

		if (GameInfo)
		{
			type = GameInfo->type;
		}
		if (type == GIT_NSF) { //NSF Breakpoint keywords
			if (strcmp(offsetBuffer,"LOAD") == 0) offset = (NSFHeader.LoadAddressLow | (NSFHeader.LoadAddressHigh<<8));
			else if (strcmp(offsetBuffer,"INIT") == 0) offset = (NSFHeader.InitAddressLow | (NSFHeader.InitAddressHigh<<8));
			else if (strcmp(offsetBuffer,"PLAY") == 0) offset = (NSFHeader.PlayAddressLow | (NSFHeader.PlayAddressHigh<<8));
		}
		else if (type == GIT_FDS) { //FDS Breakpoint keywords
			if (strcmp(offsetBuffer,"NMI1") == 0) offset = (GetMem(0xDFF6) | (GetMem(0xDFF7)<<8));
			else if (strcmp(offsetBuffer,"NMI2") == 0) offset = (GetMem(0xDFF8) | (GetMem(0xDFF9)<<8));
			else if (strcmp(offsetBuffer,"NMI3") == 0) offset = (GetMem(0xDFFA) | (GetMem(0xDFFB)<<8));
			else if (strcmp(offsetBuffer,"RST") == 0) offset = (GetMem(0xDFFC) | (GetMem(0xDFFD)<<8));
			else if ((strcmp(offsetBuffer,"IRQ") == 0) || (strcmp(offsetBuffer,"BRK") == 0)) offset = (GetMem(0xDFFE) | (GetMem(0xDFFF)<<8));
		}
		else { //NES Breakpoint keywords
			if ((strcmp(offsetBuffer,"NMI") == 0) || (strcmp(offsetBuffer,"VBL") == 0)) offset = (GetMem(0xFFFA) | (GetMem(0xFFFB)<<8));
			else if (strcmp(offsetBuffer,"RST") == 0) offset = (GetMem(0xFFFC) | (GetMem(0xFFFD)<<8));
			else if ((strcmp(offsetBuffer,"IRQ") == 0) || (strcmp(offsetBuffer,"BRK") == 0)) offset = (GetMem(0xFFFE) | (GetMem(0xFFFF)<<8));
		}

		if (conversionOk)
		{
			*conversionOk = (offset >= 0) && (offset < 0x10000);
		}
	}

	return offset & 0xFFFF;
}

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
		case 'S': return _S;
	}

	return 0;
}


/**
* Checks whether a breakpoint condition is syntactically valid
* and creates a breakpoint condition object if everything's OK.
*
* @param condition Condition to parse
* @param num Number of the breakpoint in the BP list the condition belongs to
* @return 0 in case of an error; 2 if everything went fine
**/
int checkCondition(const char* condition, int num)
{
	const char* b = condition;

	// Check if the condition isn't just all spaces.

	int onlySpaces = 1;

	while (*b)
	{
		if (*b != ' ')
		{
			onlySpaces = 0;
			break;
		}

		++b;
	}


	// If there's an actual condition create the BP condition object now

	if (*condition && !onlySpaces)
	{
		Condition* c = generateCondition(condition);

		// Remove the old breakpoint condition before adding a new condition.
		if (watchpoint[num].cond)
		{
			delete watchpoint[num].cond;
			free(watchpoint[num].condText);
			watchpoint[num].cond = 0;
			watchpoint[num].condText = 0;
		}

		// If the creation of the BP condition object was succesful
		// the condition is apparently valid. It can be added to the
		// breakpoint now.

		if (c)
		{
			watchpoint[num].cond = c;
			watchpoint[num].condText = (char*)malloc(strlen(condition) + 1);
			if (!watchpoint[num].condText)
				return 0;
			strcpy(watchpoint[num].condText, condition);
		}
		else
		{
			watchpoint[num].cond = 0;
		}

		return watchpoint[num].cond == 0 ? 2 : 0;
	}
	else
	{
		// Remove the old breakpoint condition
		if (watchpoint[num].cond)
		{
			delete watchpoint[num].cond;
			free(watchpoint[num].condText);
			watchpoint[num].cond = 0;
			watchpoint[num].condText = 0;
		}
		return 0;
	}
}

/**
* Adds a new breakpoint.
*
* @param hwndDlg Handle of the debugger window
* @param num Number of the breakpoint
* @param
**/
unsigned int NewBreak(const char* name, int start, int end, unsigned int type, const char* condition, unsigned int num, bool enable)
{
	// Finally add breakpoint to the list
	watchpoint[num].address = start;
	watchpoint[num].endaddress = 0;

	// Optional end address found
	if (end != -1)
	{
		watchpoint[num].endaddress = end;
	}

	// Get the breakpoint flags
	watchpoint[num].flags = 0;
	if (enable) watchpoint[num].flags|=WP_E;
	if (type & WP_R) watchpoint[num].flags|=WP_R;
	if (type & WP_F) watchpoint[num].flags|=WP_F;
	if (type & WP_W) watchpoint[num].flags|=WP_W;
	if (type & WP_X) watchpoint[num].flags|=WP_X;
	if (type & BT_P) {
		watchpoint[num].flags|=BT_P;
		watchpoint[num].flags&=~WP_X; //disable execute flag!
	}
	if (type & BT_S) {
		watchpoint[num].flags|=BT_S;
		watchpoint[num].flags&=~WP_X; //disable execute flag!
	}
	if (type & BT_R) {
		watchpoint[num].flags|=BT_R;
	}

	if (watchpoint[num].desc)
		free(watchpoint[num].desc);

	watchpoint[num].desc = (char*)malloc(strlen(name) + 1);
	strcpy(watchpoint[num].desc, name);

	return checkCondition(condition, num);
}

int GetPRGAddress(int A){
	int result;
	if(A > 0xFFFF)
		return -1;
	if (GameInfo->type == GIT_FDS) {
		if (A < 0xE000) {
			result = &Page[A >> 11][A] - PRGptr[1];
			if ((result > (int)PRGsize[1]) || (result < 0))
				return -1;
			else
				return result;
		} else {
			result = &Page[A >> 11][A] - PRGptr[0];
			if ((result > (int)PRGsize[0]) || (result < 0))
				return -1;
			else
				return result + PRGsize[1];
		}
	} else {
		result = &Page[A >> 11][A] - PRGptr[0];
		if ((result > (int)PRGsize[0]) || (result < 0))
			return -1;
		else
			return result;
	}
}

/**
* Returns the bank for a given offset.
* Technically speaking this function does not calculate the actual bank
* where the offset resides but the 0x4000 bytes large chunk of the ROM of the offset.
*
* @param offs The offset
* @return The bank of that offset or -1 if the offset is not part of the ROM.
**/
int getBank(int offs)
{
	//NSF data is easy to overflow the return on.
	//Anything over FFFFF will kill it.

	//GetNesFileAddress doesn't work well with Unif files
	int addr = GetNesFileAddress(offs)-NES_HEADER_SIZE;

	if (GameInfo && GameInfo->type==GIT_NSF)
		return addr != -1 ? addr / 0x1000 : -1;
	return addr != -1 ? addr / (1<<debuggerPageSize) : -1; //formerly, dividing by 0x4000
}

int GetNesFileAddress(int A){
	int result;
	if((A < 0x6000) || (A > 0xFFFF))return -1;
	result = &Page[A>>11][A]-PRGptr[0];
	if((result > (int)(PRGsize[0])) || (result < 0))return -1;
	else return result+NES_HEADER_SIZE; //16 bytes for the header remember
}

int GetRomAddress(int A){
	int i;
	uint8 *p = GetNesPRGPointer(A-=NES_HEADER_SIZE);
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
	if ((A >= 0x2000) && (A < 0x4000)) // PPU regs and their mirrors
		switch (A&7) {
			case 0: return PPU[0];
			case 1: return PPU[1];
			case 2: return PPU[2]|(PPUGenLatch&0x1F);
			case 3: return PPU[3];
			case 4: return SPRAM[PPU[3]];
			case 5: return XOffset;
			case 6: return FCEUPPU_PeekAddress() & 0xFF;
			case 7: return VRAMBuffer;
		}
	// feos: added more registers
	else if ((A >= 0x4000) && (A < 0x4010))
		return PSG[A&15];
	else if ((A >= 0x4010) && (A < 0x4018))
		switch(A&7) {
			case 0: return DMCFormat;
			case 1: return RawDALatch;
			case 2: return DMCAddressLatch;
			case 3: return DMCSizeLatch;
			case 4: return SpriteDMA;
			case 5: return EnabledChannels;
			case 6: return RawReg4016;
			case 7: return IRQFrameMode;
		}		
	else if ((A >= 0x4018) && (A < 0x5000))	// AnS: changed the range, so MMC5 ExRAM can be watched in the Hexeditor
		return 0xFF;
	if (GameInfo) {							//adelikat: 11/17/09: Prevent crash if this is called with no game loaded.
		uint32 ret;
		fceuindbg=1;
		ret = ARead[A](A);
		fceuindbg=0;
		return ret;
	} else return 0;
}

uint8 GetPPUMem(uint8 A) {
	uint16 tmp = FCEUPPU_PeekAddress() & 0x3FFF;

	if (tmp<0x2000) return VPage[tmp>>10][tmp];
	if (tmp>=0x3F00) return PALRAM[tmp&0x1F];
	return vnapage[(tmp>>10)&0x3][tmp&0x3FF];
}

//---------------------

uint8 evaluateWrite(uint8 opcode, uint16 address)
{
	// predicts value written by this opcode
	switch (opwrite[opcode])
	{
		default:
		case  0: return 0; // no write
		case  1: return _A; // STA, PHA
		case  2: return _X; // STX
		case  3: return _Y; // STY
		case  4: return _P; // PHP
		case  5: return GetMem(address) << 1; // ASL (SLO)
		case  6: return GetMem(address) >> 1; // LSR (SRE)
		case  7: return (GetMem(address) << 1) | (_P & 1); // ROL (RLA)
		case  8: return (GetMem(address) >> 1) | ((_P & 1) << 7); // ROL (RRA)
		case  9: return GetMem(address) + 1; // INC (ISC)
		case 10: return GetMem(address) - 1; // DEC (DCP)
		case 11: return _A & _X; // (SAX)
		case 12: return _A&_X&(((address-_Y)>>8)+1); // (AHX)
		case 13: return _Y&(((address-_X)>>8)+1); // (SHY)
		case 14: return _X&(((address-_Y)>>8)+1); // (SHX)
		case 15: return _S& (((address-_Y)>>8)+1); // (TAS)
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
			case TYPE_ADDR: // This is intended to not break, and use the TYPE_NUM code
			case TYPE_NUM: value1 = c->value1; break;
			default: value1 = getValue(c->value1); break;
		}
	}

	switch(c->type1)
	{
		case TYPE_ADDR: value1 = GetMem(value1); break;
		case TYPE_PC_BANK: value1 = getBank(_PC); break;
		case TYPE_DATA_BANK: value1 = getBank(debugLastAddress); break;
		case TYPE_VALUE_READ: value1 = GetMem(debugLastAddress); break;
		case TYPE_VALUE_WRITE: value1 = evaluateWrite(debugLastOpcode, debugLastAddress); break;
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
				case TYPE_ADDR: // This is intended to not break, and use the TYPE_NUM code
				case TYPE_NUM: value2 = c->value2; break;
				default: value2 = getValue(c->type2); break;
			}
		}

	switch(c->type2)
	{
		case TYPE_ADDR: value2 = GetMem(value2); break;
		case TYPE_PC_BANK: value2 = getBank(_PC); break;
		case TYPE_DATA_BANK: value2 = getBank(debugLastAddress); break;
		case TYPE_VALUE_READ: value2 = GetMem(debugLastAddress); break;
		case TYPE_VALUE_WRITE: value2 = evaluateWrite(debugLastOpcode, debugLastAddress); break;
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
			case OP_DIV: f = (value2==0) ? 0 : (value1 / value2); break;
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


//---------------------

volatile int codecount = 0, datacount = 0, undefinedcount = 0;
unsigned char *cdloggerdata = NULL;
unsigned int cdloggerdataSize = 0;
static int indirectnext = 0;

int debug_loggingCD = 0;

//called by the cpu to perform logging if CDLogging is enabled
void LogCDVectors(int which){
	int j;
	j = GetPRGAddress(which);
	if(j == -1) return;

	if(!(cdloggerdata[j] & 2)){
		cdloggerdata[j] |= 0x0E; // we're in the last bank and recording it as data so 0x1110 or 0xE should be what we need
		datacount++;
		if(!(cdloggerdata[j] & 1))undefinedcount--;
	}
	j++;

	if(!(cdloggerdata[j] & 2)){
		cdloggerdata[j] |= 0x0E;
		datacount++;
		if(!(cdloggerdata[j] & 1))undefinedcount--;
	}
}

bool break_on_unlogged_code = false;
bool break_on_unlogged_data = false;

void LogCDData(uint8 *opcode, uint16 A, int size)
{
	int i, j;
	uint8 memop = 0;
	bool newCodeHit = false, newDataHit = false;

	if ((j = GetPRGAddress(_PC)) != -1)
	{
		for (i = 0; i < size; i++)
		{
			if (cdloggerdata[j+i] & 1) continue; //this has been logged so skip
			cdloggerdata[j+i] |= 1;
			cdloggerdata[j+i] |= ((_PC + i) >> 11) & 0x0c;
			cdloggerdata[j+i] |= ((_PC & 0x8000) >> 8) ^ 0x80;	// 19/07/14 used last reserved bit, if bit 7 is 1, then code is running from lowe area (6000)
			if (indirectnext)cdloggerdata[j+i] |= 0x10;
			codecount++;
			if (!(cdloggerdata[j+i] & 2))undefinedcount--;
			newCodeHit = true;
		}
	}

	//log instruction jumped to in an indirect jump
	if(opcode[0] == 0x6c)
		indirectnext = 1;
	else
		indirectnext = 0;

	switch (optype[opcode[0]]) {
		case 1:
		case 4: memop = 0x20; break;
	}

	if ((j = GetPRGAddress(A)) != -1)
	{
		if (opwrite[opcode[0]] == 0)
		{
			if (!(cdloggerdata[j] & 2))
			{
				cdloggerdata[j] |= 2;
				cdloggerdata[j] |= (A >> 11) & 0x0c;
				cdloggerdata[j] |= memop;
				cdloggerdata[j] |= ((A & 0x8000) >> 8) ^ 0x80;	
				datacount++;
				if (!(cdloggerdata[j] & 1))undefinedcount--;
				newDataHit = true;
			}
		}
		else
		{
			if (cdloggerdata[j] & 1)
			{
				codecount--;
			}
			if (cdloggerdata[j] & 2)
			{
				datacount--;
			}
			if ((cdloggerdata[j] & 3) != 0) undefinedcount++;
			cdloggerdata[j] = 0;
		}
	}

	if ( break_on_unlogged_code && newCodeHit )
	{
		BreakHit( BREAK_TYPE_UNLOGGED_CODE );
	}
	else if ( break_on_unlogged_data && newDataHit )
	{
		BreakHit( BREAK_TYPE_UNLOGGED_DATA );
	}
}

//-----------debugger stuff

watchpointinfo watchpoint[65]; //64 watchpoints, + 1 reserved for step over
int iaPC;
uint32 iapoffset; //mbg merge 7/18/06 changed from int
int u; //deleteme
int skipdebug; //deleteme
int numWPs;

bool break_asap = false;
// for CPU cycles and Instructions counters
uint64 total_cycles_base = 0;
uint64 delta_cycles_base = 0;
bool break_on_cycles = false;
uint64 break_cycles_limit = 0;
uint64 total_instructions = 0;
uint64 delta_instructions = 0;
bool break_on_instructions = false;
uint64 break_instructions_limit = 0;

static DebuggerState dbgstate;

DebuggerState &FCEUI_Debugger() { return dbgstate; }

void ResetDebugStatisticsCounters()
{
	ResetCyclesCounter();
	ResetInstructionsCounter();
}
void ResetCyclesCounter()
{
	total_cycles_base = delta_cycles_base = timestampbase + (uint64)timestamp;
}
void ResetInstructionsCounter()
{
	total_instructions = delta_instructions = 0;
}
void ResetDebugStatisticsDeltaCounters()
{
	delta_cycles_base = timestampbase + (uint64)timestamp;
	delta_instructions = 0;
}
void IncrementInstructionsCounters()
{
	total_instructions++;
	delta_instructions++;
}

bool CondForbidTest(int bp_num) {
	if (bp_num >= 0 && !condition(&watchpoint[bp_num]))
	{
		return false;	// condition rejected
	}

	//check to see whether we fall in any forbid zone
	for (int i = 0; i < numWPs; i++)
	{
		watchpointinfo& wp = watchpoint[i];
		if (!(wp.flags & WP_F) || !(wp.flags & WP_E))
			continue;

		if (condition(&wp))
		{
			if (wp.endaddress) {
				if ((wp.address <= _PC) && (wp.endaddress >= _PC))
					return false;	// forbid
			}
			else {
				if (wp.address == _PC)
					return false;	// forbid
			}
		}
	}
	return true;
}

void BreakHit(int bp_num)
{
	FCEUI_SetEmulationPaused(EMULATIONPAUSED_PAUSED); //mbg merge 7/19/06 changed to use EmulationPaused()

//#ifdef WIN32
	FCEUD_DebugBreakpoint(bp_num);
//#endif
}

int StackAddrBackup;
uint16 StackNextIgnorePC = 0xFFFF;

///fires a breakpoint
static void breakpoint(uint8 *opcode, uint16 A, int size) {
	int i, romAddrPC;
	unsigned int j;
	uint8 brk_type;
	uint8 stackop=0;
	uint8 stackopstartaddr=0,stackopendaddr=0;

	debugLastAddress = A;
	debugLastOpcode = opcode[0];

	if (break_asap)
	{
		break_asap = false;
		BreakHit(BREAK_TYPE_LUA);
	}

	if (break_on_cycles && ((timestampbase + (uint64)timestamp - total_cycles_base) > break_cycles_limit))
		BreakHit(BREAK_TYPE_CYCLES_EXCEED);
	if (break_on_instructions && (total_instructions > break_instructions_limit))
		BreakHit(BREAK_TYPE_INSTRUCTIONS_EXCEED);

	//if the current instruction is bad, and we are breaking on bad opcodes, then hit the breakpoint
	if(dbgstate.badopbreak && (size == 0))
		BreakHit(BREAK_TYPE_BADOP);

	//if we're stepping out, track the nest level
	if (dbgstate.stepout) {
		if (opcode[0] == 0x20) dbgstate.jsrcount++;
		else if (opcode[0] == 0x60) {
			if (dbgstate.jsrcount)
				dbgstate.jsrcount--;
			else {
				dbgstate.stepout = false;
				dbgstate.step = true;
				return;
			}
		}
	}

	//if we're stepping, then we'll always want to break
	if (dbgstate.step) {
		dbgstate.step = false;
		BreakHit(BREAK_TYPE_STEP);
		return;
	}

	//if we're running for a scanline, we want to check if we've hit the cycle limit
	if (dbgstate.runline) {
		uint64 ts = timestampbase;
		ts+=timestamp;
		int diff = dbgstate.runline_end_time-ts;
		if (diff<=0)
		{
			dbgstate.runline=false;
			BreakHit(BREAK_TYPE_STEP);
			return;
		}
	}

	//check the step over address and break if we've hit it
	if ((watchpoint[64].address == _PC) && (watchpoint[64].flags)) {
		watchpoint[64].address = 0;
		watchpoint[64].flags = 0;
		BreakHit(BREAK_TYPE_STEP);
		return;
	}

	romAddrPC = GetNesFileAddress(_PC);

	brk_type = opbrktype[opcode[0]] | WP_X;

	switch (opcode[0]) {
		//Push Ops
		case 0x08: //Fall to next
		case 0x48: debugLastAddress=stackopstartaddr=stackopendaddr=X.S-1; stackop=WP_W; StackAddrBackup = X.S; StackNextIgnorePC=_PC+1; break;
		//Pull Ops
		case 0x28: //Fall to next
		case 0x68: debugLastAddress=stackopstartaddr=stackopendaddr=X.S+1; stackop=WP_R; StackAddrBackup = X.S; StackNextIgnorePC=_PC+1; break;
		//JSR (Includes return address - 1)
		case 0x20: stackopstartaddr=stackopendaddr=X.S-1; stackop=WP_W; StackAddrBackup = X.S; StackNextIgnorePC=(opcode[1]|opcode[2]<<8); break;
		//RTI (Includes processor status, and exact return address)
		case 0x40: stackopstartaddr=X.S+1; stackopendaddr=X.S+3; stackop=WP_R; StackAddrBackup = X.S; StackNextIgnorePC=(GetMem(X.S+2|0x0100)|GetMem(X.S+3|0x0100)<<8); break;
		//RTS (Includes return address - 1)
		case 0x60: stackopstartaddr=X.S+1; stackopendaddr=X.S+2; stackop=WP_R; StackAddrBackup = X.S; StackNextIgnorePC=(GetMem(stackopstartaddr|0x0100)|GetMem(stackopendaddr|0x0100)<<8)+1; break;
		default: break;
	}

#define BREAKHIT(x) { if (CondForbidTest(x)) { breakHit = (x); goto STOPCHECKING; } }
	int breakHit = -1;
	for (i = 0; i < numWPs; i++)
	{
		if ((watchpoint[i].flags & WP_E))
		{
			if (watchpoint[i].flags & BT_P)
			{
				// PPU Mem breaks
				if ((watchpoint[i].flags & brk_type) && ((A >= 0x2000) && (A < 0x4000)) && ((A&7) == 7))
				{
					const uint32 PPUAddr = FCEUPPU_PeekAddress();
					if (watchpoint[i].endaddress)
					{
						if ((watchpoint[i].address <= PPUAddr) && (watchpoint[i].endaddress >= PPUAddr))
							BREAKHIT(i);
					} else
					{
						if (watchpoint[i].address == PPUAddr)
							BREAKHIT(i);
					}
				}
			} else if (watchpoint[i].flags & BT_S)
			{
				// Sprite Mem breaks
				if ((watchpoint[i].flags & brk_type) && ((A >= 0x2000) && (A < 0x4000)) && ((A&7) == 4))
				{
					if (watchpoint[i].endaddress)
					{
						if ((watchpoint[i].address <= PPU[3]) && (watchpoint[i].endaddress >= PPU[3]))
							BREAKHIT(i);
					} else
					{
						if (watchpoint[i].address == PPU[3])
						BREAKHIT(i);
					}
				} else if ((watchpoint[i].flags & WP_W) && (A == 0x4014))
				{
					// Sprite DMA! :P
					BREAKHIT(i);
				}
			} else
			{
				// CPU mem breaks
				if ((watchpoint[i].flags & brk_type))
				{
					if (watchpoint[i].endaddress)
					{
						if (((watchpoint[i].flags & (WP_R | WP_W)) && (watchpoint[i].address <= A) && (watchpoint[i].endaddress >= A)) ||
							((watchpoint[i].flags & WP_X) && (watchpoint[i].address <= _PC) && (watchpoint[i].endaddress >= _PC)))
							BREAKHIT(i);
					}
					else
					{
						if (watchpoint[i].flags & BT_R)
						{
							if ( (watchpoint[i].flags & WP_X) && (watchpoint[i].address == static_cast<unsigned int>(romAddrPC)) )
							{
								BREAKHIT(i);
							}
							//else if ( (watchpoint[i].flags & WP_R) && (watchpoint[i].address == A) )
							//{
							//	BREAKHIT(i);
							//}	
						}
						else
						{
							if ( (watchpoint[i].flags & (WP_R | WP_W)) && (watchpoint[i].address == A)) 
							{
								BREAKHIT(i);
							}
							else if ( (watchpoint[i].flags & WP_X) && (watchpoint[i].address == _PC) )
							{
								BREAKHIT(i);
							}
						}
					}
				} else
				{
					// brk_type independant coding
					if (stackop > 0)
					{
						// Announced stack mem breaks
						// PHA, PLA, PHP, and PLP affect the stack data.
						// TXS and TSX only deal with the pointer.
						if (watchpoint[i].flags & stackop)
						{
							for (j = (stackopstartaddr|0x0100); j <= (static_cast<unsigned int>(stackopendaddr)|0x0100); j++)
							{
								if (watchpoint[i].endaddress)
								{
									if ((watchpoint[i].address <= j) && (watchpoint[i].endaddress >= j))
										BREAKHIT(i);
								} else
								{
									if (watchpoint[i].address == j)
										BREAKHIT(i);
								}
							}
						}
					}
					if (StackNextIgnorePC == _PC)
					{
						// Used to make it ignore the unannounced stack code one time
						StackNextIgnorePC = 0xFFFF;
					} else
					{
						if (StackAddrBackup != -1 && (X.S < StackAddrBackup) && (stackop==0))
						{
							// Unannounced stack mem breaks
							// Pushes to stack
							if (watchpoint[i].flags & WP_W)
							{
								for (j = (X.S|0x0100); j < (static_cast<unsigned int>(StackAddrBackup)|0x0100); j++)
								{
									if (watchpoint[i].endaddress)
									{
										if ((watchpoint[i].address <= j) && (watchpoint[i].endaddress >= j))
											BREAKHIT(i);
									} else
									{
										if (watchpoint[i].address == j)
											BREAKHIT(i);
									}
								}
							}
						} else if (StackAddrBackup != -1 && (StackAddrBackup < X.S) && (stackop==0))
						{
							// Pulls from stack
							if (watchpoint[i].flags & WP_R)
							{
								for (j = (StackAddrBackup|0x0100); j < (static_cast<unsigned int>(X.S)|0x0100); j++)
								{
									if (watchpoint[i].endaddress)
									{
										if ((watchpoint[i].address <= j) && (watchpoint[i].endaddress >= j))
											BREAKHIT(i);
									} else
									{
										if (watchpoint[i].address == j)
											BREAKHIT(i);
									}
								}
							}
						}
					}

				}
			}
		}
	} //loop across all breakpoints

STOPCHECKING:
	
	//Update the stack address with the current one, now that changes have registered.
	//ZEROMUS THINKS IT MAKES MORE SENSE HERE
	StackAddrBackup = X.S;

	if(breakHit != -1)
		BreakHit(i);

	////Update the stack address with the current one, now that changes have registered.
	//StackAddrBackup = X.S;
}
//bbit edited: this is the end of the inserted code

void DebugCycle()
{
	uint8 opcode[3] = {0};
	uint16 A = 0, tmp;
	int size;

	if (scanline == 240)
	{
		vblankScanLines = (PAL?int((double)timestamp / ((double)341 / (double)3.2)):timestamp / 114);	//114 approximates the number of timestamps per scanline during vblank.  Approx 2508. NTSC: (341 / 3.0) PAL: (341 / 3.2). Uses (3.? * cpu_cycles) / 341.0, and assumes 1 cpu cycle.
		if (vblankScanLines) vblankPixel = 341 / vblankScanLines;	//341 pixels per scanline
		//FCEUI_printf("vbPixel = %d",vblankPixel);					     //Debug
		//FCEUI_printf("ts: %d line: %d\n", timestamp, vblankScanLines); //Debug
	}
	else
		vblankScanLines = 0;

	if (GameInfo->type==GIT_NSF)
	{
		if ((_PC >= 0x3801) && (_PC <= 0x3824)) return;
	}

	opcode[0] = GetMem(_PC);
	size = opsize[opcode[0]];
	switch (size)
	{
		default:
		case 1: break;
		case 2:
			opcode[1] = GetMem(_PC + 1);
			break;
		case 0: // illegal instructions may have operands
		case 3:
			opcode[1] = GetMem(_PC + 1);
			opcode[2] = GetMem(_PC + 2);
			break;
	}

	switch (optype[opcode[0]])
	{
		case 0: break;
		case 1:
			tmp = (opcode[1] + _X) & 0xFF;
			A = GetMem(tmp);
			tmp = (opcode[1] + _X + 1) & 0xFF;
			A |= (GetMem(tmp) << 8);
			break;
		case 2: A = opcode[1]; break;
		case 3: A = opcode[1] | (opcode[2] << 8); break;
		case 4: A = (GetMem(opcode[1]) | (GetMem((opcode[1] + 1) & 0xFF) << 8)) + _Y; break;
		case 5: A = opcode[1] + _X; break;
		case 6: A = (opcode[1] | (opcode[2] << 8)) + _Y; break;
		case 7: A = (opcode[1] | (opcode[2] << 8)) + _X; break;
		case 8: A = opcode[1] + _Y; break;
	}

	if (numWPs || dbgstate.step || dbgstate.runline || dbgstate.stepout || watchpoint[64].flags || dbgstate.badopbreak || break_on_cycles || break_on_instructions || break_asap)
		breakpoint(opcode, A, size);

	if(debug_loggingCD)
		LogCDData(opcode, A, size);

#ifdef __WIN_DRIVER__
	FCEUD_TraceInstruction(opcode, size);
#else
	// Use callback pointer that can be null checked, this saves on the overhead
	// of calling a function for every instruction when we aren't tracing.
	if (traceInstructionCB != nullptr)
	{
		auto* cb = traceInstructionCB;
		while (cb != nullptr)
		{
			cb->func(opcode, size);
			cb = cb->next;
		}
	}
#endif
}

void* FCEUI_TraceInstructionRegister( void (*func)(uint8*,int) )
{
	TraceInstructionCallback* cb = nullptr;

	if (traceInstructionCB == nullptr)
	{
		cb = traceInstructionCB = new TraceInstructionCallback();
		cb->func = func;
	}
	else
	{
		cb = traceInstructionCB;

		while (cb != nullptr)
		{
			if (cb->func == func)
			{
				// This function has already been registered, don't double add.
				return nullptr;
			}
			if (cb->next == nullptr)
			{
				auto* newCB = new TraceInstructionCallback();
				newCB->func = func;
				cb->next = newCB;
				return newCB;
			}
			cb = cb->next;
		}
	}
	return cb;
}

bool FCEUI_TraceInstructionUnregisterHandle( void* handle )
{
	TraceInstructionCallback* cb, *cb_prev, *cb_handle;

	cb_handle = static_cast<TraceInstructionCallback*>(handle);
	cb_prev = nullptr;
	cb = traceInstructionCB;

	while (cb != nullptr)
	{
		if (cb == cb_handle)
		{	// Match we are going to remove from list and delete
			if (cb_prev != nullptr)
			{
				cb_prev = cb->next;
			}
			else
			{
				traceInstructionCB = cb->next;
			}
			delete cb;
			return true;
		}
		cb_prev = cb;
		cb = cb->next;
	}
	return false;
}

