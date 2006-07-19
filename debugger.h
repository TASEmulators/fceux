#ifndef DEBUGGER_H
#define DEBUGGER_H

//#define GetMem(x) (((x < 0x2000) || (x >= 0x4020))?ARead[x](x):0xFF)
#include <windows.h>
#include "conddebug.h"

//watchpoint stuffs
#define WP_E       0x01  //watchpoint, enable
#define WP_W       0x02  //watchpoint, write
#define WP_R       0x04  //watchpoint, read
#define WP_X       0x08  //watchpoint, execute

#define BT_C       0x00  //break type, cpu mem
#define BT_P       0x10  //break type, ppu mem
#define BT_S       0x20  //break type, sprite mem

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

extern uint8 *vnapage[4],*VPage[8];
extern uint8 PPU[4],PALRAM[0x20],SPRAM[0x100],VRAMBuffer,PPUGenLatch,XOffset;
extern uint32 RefreshAddr;
//extern volatile int userpause; //mbg merge 7/18/06 removed for merging
extern int scanline; //current scanline! :D
extern int badopbreak;
extern HWND hDebug;

extern int step,stepout,jsrcount;
extern int childwnd,numWPs; //mbg merge 7/18/06 had to make extern

BOOL CenterWindow(HWND hwndDlg);
void DoPatcher(int address,HWND hParent);
void UpdatePatcher(HWND hwndDlg);
int GetNesFileAddress(int A);
int GetPRGAddress(int A);
int GetRomAddress(int A);
int GetEditHex(HWND hwndDlg, int id);
uint8 *GetNesPRGPointer(int A);
uint8 *GetNesCHRPointer(int A);
extern void AddBreakList();

char *BinToASM(int addr, uint8 *opcode);

void UpdateDebugger();
void DoDebug(uint8 halt);
void KillDebugger();
uint8 GetMem(uint16 A);
uint8 GetPPUMem(uint8 A);


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

#endif
