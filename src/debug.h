#ifndef _DEBUG_H_
#define _DEBUG_H_

#include "conddebug.h"

//watchpoint stuffs
#define WP_E       0x01  //watchpoint, enable
#define WP_W       0x02  //watchpoint, write
#define WP_R       0x04  //watchpoint, read
#define WP_X       0x08  //watchpoint, execute

#define BT_C       0x00  //break type, cpu mem
#define BT_P       0x10  //break type, ppu mem
#define BT_S       0x20  //break type, sprite mem

//opbrktype is used to grab the breakpoint type that each instruction will cause.
//WP_X is not used because ALL opcodes will have the execute bit set.
static const uint8 opbrktype[256] = {
	      /*0,    1, 2, 3,    4,    5,         6, 7, 8,    9, A, B,    C,    D,         E, F*/
/*0x00*/	0, WP_R, 0, 0,    0, WP_R, WP_R|WP_W, 0, 0,    0, 0, 0,    0, WP_R, WP_R|WP_W, 0,
/*0x10*/	0, WP_R, 0, 0,    0, WP_R, WP_R|WP_W, 0, 0, WP_R, 0, 0,    0, WP_R, WP_R|WP_W, 0,
/*0x20*/	0, WP_R, 0, 0, WP_R, WP_R, WP_R|WP_W, 0, 0,    0, 0, 0, WP_R, WP_R, WP_R|WP_W, 0,
/*0x30*/	0, WP_R, 0, 0,    0, WP_R, WP_R|WP_W, 0, 0, WP_R, 0, 0,    0, WP_R, WP_R|WP_W, 0,
/*0x40*/	0, WP_R, 0, 0,    0, WP_R, WP_R|WP_W, 0, 0,    0, 0, 0,    0, WP_R, WP_R|WP_W, 0,
/*0x50*/	0, WP_R, 0, 0,    0, WP_R, WP_R|WP_W, 0, 0, WP_R, 0, 0,    0, WP_R, WP_R|WP_W, 0,
/*0x60*/	0, WP_R, 0, 0,    0, WP_R, WP_R|WP_W, 0, 0,    0, 0, 0, WP_R, WP_R, WP_R|WP_W, 0,
/*0x70*/	0, WP_R, 0, 0,    0, WP_R, WP_R|WP_W, 0, 0, WP_R, 0, 0,    0, WP_R, WP_R|WP_W, 0,
/*0x80*/	0, WP_W, 0, 0, WP_W, WP_W,      WP_W, 0, 0,    0, 0, 0, WP_W, WP_W,      WP_W, 0,
/*0x90*/	0, WP_W, 0, 0, WP_W, WP_W,      WP_W, 0, 0, WP_W, 0, 0,    0, WP_W,         0, 0,
/*0xA0*/	0, WP_R, 0, 0, WP_R, WP_R,      WP_R, 0, 0,    0, 0, 0, WP_R, WP_R,      WP_R, 0,
/*0xB0*/	0, WP_R, 0, 0, WP_R, WP_R,      WP_R, 0, 0, WP_R, 0, 0, WP_R, WP_R,      WP_R, 0,
/*0xC0*/	0, WP_R, 0, 0, WP_R, WP_R, WP_R|WP_W, 0, 0,    0, 0, 0, WP_R, WP_R, WP_R|WP_W, 0,
/*0xD0*/	0, WP_R, 0, 0,    0, WP_R, WP_R|WP_W, 0, 0, WP_R, 0, 0,    0, WP_R, WP_R|WP_W, 0,
/*0xE0*/	0, WP_R, 0, 0, WP_R, WP_R, WP_R|WP_W, 0, 0,    0, 0, 0, WP_R, WP_R, WP_R|WP_W, 0,
/*0xF0*/	0, WP_R, 0, 0,    0, WP_R, WP_R|WP_W, 0, 0, WP_R, 0, 0,    0, WP_R, WP_R|WP_W, 0
};

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

#endif
