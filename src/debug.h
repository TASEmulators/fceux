#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "conddebug.h"
#include "git.h"
#include "nsf.h"

//watchpoint stuffs
#define WP_E       0x01  //watchpoint, enable
#define WP_W       0x02  //watchpoint, write
#define WP_R       0x04  //watchpoint, read
#define WP_X       0x08  //watchpoint, execute
#define WP_F       0x10  //watchpoint, forbid

#define BT_C       0x00  //break type, cpu mem
#define BT_P       0x20  //break type, ppu mem
#define BT_S       0x40  //break type, sprite mem

//opbrktype is used to grab the breakpoint type that each instruction will cause.
extern const uint8 opbrktype[256];

typedef struct {
	uint16 address;
	uint16 endaddress;
	uint8 flags;
// ################################## Start of SP CODE ###########################

	Condition* cond;
	char* condText;
	char* desc;

// ################################## End of SP CODE ###########################
} watchpointinfo;

//mbg merge 7/18/06 had to make this extern
extern watchpointinfo watchpoint[65]; //64 watchpoints, + 1 reserved for step over

int GetNesFileAddress(int A);
int GetPRGAddress(int A);
int GetRomAddress(int A);
//int GetEditHex(HWND hwndDlg, int id);
uint8 *GetNesPRGPointer(int A);
uint8 *GetNesCHRPointer(int A);
void KillDebugger();
uint8 GetMem(uint16 A);
uint8 GetPPUMem(uint8 A);

//---------CDLogger
void LogCDVectors(int which);
void LogCDData();
extern volatile int codecount, datacount, undefinedcount;
extern unsigned char *cdloggerdata;

extern int debug_loggingCD;
static INLINE void FCEUI_SetLoggingCD(int val) { debug_loggingCD = val; }
static INLINE int FCEUI_GetLoggingCD() { return debug_loggingCD; }
//-------

//-------tracing
//we're letting the win32 driver handle this ittself for now
//extern int debug_tracing;
//static INLINE void FCEUI_SetTracing(int val) { debug_tracing = val; }
//static INLINE int FCEUI_GetTracing() { return debug_tracing; }
//---------

//--------debugger
extern int iaPC;
extern uint32 iapoffset; //mbg merge 7/18/06 changed from int
void DebugCycle();
//-------------

//internal variables that debuggers will want access to
extern uint8 *vnapage[4],*VPage[8];
extern uint8 PPU[4],PALRAM[0x20],SPRAM[0x100],VRAMBuffer,PPUGenLatch,XOffset;
extern uint32 RefreshAddr;

extern int debug_loggingCD;
extern int numWPs; 

///encapsulates the operational state of the debugger core
class DebuggerState {
public:
	///indicates whether the debugger is stepping through a single instruction
	bool step;
	///indicates whether the debugger is stepping out of a function call
	bool stepout;
	///indicates whether the debugger should break on bad opcodes
	bool badopbreak;
	///counts the nest level of the call stack while stepping out
	int jsrcount;

	///resets the debugger state to an empty, non-debugging state
	void reset() {
		numWPs = 0;
		step = false;
		stepout = false;
		jsrcount = 0;
	}
};

extern NSF_HEADER NSFHeader;

///retrieves the core's DebuggerState
DebuggerState &FCEUI_Debugger();

//#define CPU_BREAKPOINT 1
//#define PPU_BREAKPOINT 2
//#define SPRITE_BREAKPOINT 4
//#define READ_BREAKPOINT 8
//#define WRITE_BREAKPOINT 16
//#define EXECUTE_BREAKPOINT 32

int offsetStringToInt(unsigned int type, const char* offsetBuffer);
unsigned int NewBreak(const char* name, int start, int end, unsigned int type, const char* condition, unsigned int num, bool enable);

#endif
