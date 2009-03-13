/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Ben Parnell
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

#include "common.h"
#include "../../utils/xstring.h"
#include "../../types.h"
#include "debugger.h"
#include "../../x6502.h"
#include "../../fceu.h"
#include "../../debug.h"
#include "../../nsf.h"
#include "../../cart.h"
#include "../../ines.h"
#include "../../asm.h"
#include "tracer.h"
#include "memview.h"
#include "cheat.h"
#include "gui.h"

// ################################## Start of SP CODE ###########################

#include "debuggersp.h"

extern Name* lastBankNames;
extern Name* loadedBankNames;
extern Name* ramBankNames;
extern int lastBank;
extern int loadedBank;
extern int myNumWPs;

// ################################## End of SP CODE ###########################

extern int vblankScanLines;
extern int vblankPixel;

int childwnd;

extern readfunc ARead[0x10000];
int DbgPosX,DbgPosY;
int DbgSizeX=-1,DbgSizeY=-1;
int WP_edit=-1;
int ChangeWait=0,ChangeWait2=0;
uint8 debugger_open=0;
HWND hDebug;
static HFONT hFont;
static SCROLLINFO si;

#define INVALID_START_OFFSET 1
#define INVALID_END_OFFSET 2

#define MAX_NAME_SIZE 200
#define MAX_CONDITION_SIZE 200

void RestoreSize(HWND hwndDlg)
{
	//As of the writing of this function (revision 1137) the Debugger default width = 821 and height of 523
	//If the dialog dimensions are changed those changes need to be reflected here.  - adelikat
	const int DEFAULT_WIDTH = 821;	//Original width
	const int DEFAULT_HEIGHT = 549;	//Original height
	
	SetWindowPos(hwndDlg,HWND_TOPMOST,DbgPosX,DbgPosY,DEFAULT_WIDTH,DEFAULT_HEIGHT,SWP_SHOWWINDOW);
}

unsigned int NewBreakWindows(HWND hwndDlg, unsigned int num, bool enable)
{
	char startOffsetBuffer[5] = {0};
	char endOffsetBuffer[5] = {0};
	unsigned int type = 0;

	GetDlgItemText(hwndDlg, IDC_ADDBP_ADDR_START, startOffsetBuffer, sizeof(startOffsetBuffer));
	GetDlgItemText(hwndDlg, IDC_ADDBP_ADDR_END, endOffsetBuffer, sizeof(endOffsetBuffer));

	if (IsDlgButtonChecked(hwndDlg, IDC_ADDBP_MEM_CPU))
		type |= BT_C;
	else if (IsDlgButtonChecked(hwndDlg, IDC_ADDBP_MEM_PPU))
		type |= BT_P;
	else
		type |= BT_S;

	if (IsDlgButtonChecked(hwndDlg, IDC_ADDBP_MODE_R))
		type |= WP_R;

	if (IsDlgButtonChecked(hwndDlg, IDC_ADDBP_MODE_W))
		type |= WP_W;

	if (IsDlgButtonChecked(hwndDlg, IDC_ADDBP_MODE_X))
		type |= WP_X;

	//this overrides all
	if (IsDlgButtonChecked(hwndDlg, IDC_ADDBP_MODE_F))
		type = WP_F;

	int start = offsetStringToInt(type, startOffsetBuffer);

	if (start == -1)
	{
		return INVALID_START_OFFSET;
	}

	int end = offsetStringToInt(type, endOffsetBuffer);

	if (*endOffsetBuffer && end == -1)
	{
		return INVALID_END_OFFSET;
	}

	// Handle breakpoint conditions
	char name[MAX_NAME_SIZE] = {0};
	GetDlgItemText(hwndDlg, IDC_ADDBP_NAME, name, MAX_NAME_SIZE);
	
	char condition[MAX_CONDITION_SIZE] = {0};
	GetDlgItemText(hwndDlg, IDC_ADDBP_CONDITION, condition, MAX_CONDITION_SIZE);

	return NewBreak(name, start, end, type, condition, num, enable);
}

/**
* Adds a new breakpoint to the breakpoint list
*
* @param hwndDlg Handle of the debugger window
* @return 0 (success), 1 (Too many breakpoints), 2 (???), 3 (Invalid breakpoint condition)
**/
unsigned int AddBreak(HWND hwndDlg)
{
	if (numWPs == MAXIMUM_NUMBER_OF_BREAKPOINTS)
	{
		return TOO_MANY_BREAKPOINTS;
	}

	unsigned val = NewBreakWindows(hwndDlg,numWPs,1);
	
	if (val == 1)
	{
		return 2;
	}
	else if (val == 2)
	{
		return INVALID_BREAKPOINT_CONDITION;
	}

	numWPs++;
	myNumWPs++;
	return 0;
}

static void UpdateDialog(HWND hwndDlg) {
	BOOL forbid = IsDlgButtonChecked(hwndDlg, IDC_ADDBP_MODE_F);
	BOOL enable = !forbid;
	EnableWindow(GetDlgItem(hwndDlg,IDC_ADDBP_MODE_R),enable);
	EnableWindow(GetDlgItem(hwndDlg,IDC_ADDBP_MODE_W),enable);
	EnableWindow(GetDlgItem(hwndDlg,IDC_ADDBP_MODE_X),enable);
	EnableWindow(GetDlgItem(hwndDlg,IDC_ADDBP_MEM_CPU),enable);
	EnableWindow(GetDlgItem(hwndDlg,IDC_ADDBP_MEM_PPU),enable);
	EnableWindow(GetDlgItem(hwndDlg,IDC_ADDBP_MEM_SPR),enable);	
	//nah.. lets leave these checked
	//CheckDlgButton(hwndDlg,IDC_ADDBP_MODE_R,BST_UNCHECKED);
	//CheckDlgButton(hwndDlg,IDC_ADDBP_MODE_W,BST_UNCHECKED);
	//CheckDlgButton(hwndDlg,IDC_ADDBP_MODE_X,BST_UNCHECKED);
	//CheckDlgButton(hwndDlg,IDC_ADDBP_MEM_CPU,BST_UNCHECKED);
	//CheckDlgButton(hwndDlg,IDC_ADDBP_MEM_PPU,BST_UNCHECKED);
	//CheckDlgButton(hwndDlg,IDC_ADDBP_MEM_SPR,BST_UNCHECKED);
}

BOOL CALLBACK AddbpCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char str[8]={0};
	int tmp;
				
	switch(uMsg) {
		case WM_INITDIALOG:
			CenterWindow(hwndDlg);
			SendDlgItemMessage(hwndDlg,IDC_ADDBP_ADDR_START,EM_SETLIMITTEXT,4,0);
			SendDlgItemMessage(hwndDlg,IDC_ADDBP_ADDR_END,EM_SETLIMITTEXT,4,0);
			if (WP_edit >= 0) {
				SetWindowText(hwndDlg,"Edit Breakpoint...");
						
				sprintf(str,"%04X",watchpoint[WP_edit].address);
				SetDlgItemText(hwndDlg,IDC_ADDBP_ADDR_START,str);
				sprintf(str,"%04X",watchpoint[WP_edit].endaddress);
				if (strcmp(str,"0000") != 0) SetDlgItemText(hwndDlg,IDC_ADDBP_ADDR_END,str);
				if (watchpoint[WP_edit].flags&WP_R) CheckDlgButton(hwndDlg, IDC_ADDBP_MODE_R, BST_CHECKED);
				if (watchpoint[WP_edit].flags&WP_W) CheckDlgButton(hwndDlg, IDC_ADDBP_MODE_W, BST_CHECKED);
				if (watchpoint[WP_edit].flags&WP_X) CheckDlgButton(hwndDlg, IDC_ADDBP_MODE_X, BST_CHECKED);
				if (watchpoint[WP_edit].flags&WP_F) CheckDlgButton(hwndDlg, IDC_ADDBP_MODE_F, BST_CHECKED);

				if (watchpoint[WP_edit].flags&BT_P) {
					CheckDlgButton(hwndDlg, IDC_ADDBP_MEM_PPU, BST_CHECKED);
					EnableWindow(GetDlgItem(hwndDlg,IDC_ADDBP_MODE_X),FALSE);
				}
				else if (watchpoint[WP_edit].flags&BT_S) {
					CheckDlgButton(hwndDlg, IDC_ADDBP_MEM_SPR, BST_CHECKED);
					EnableWindow(GetDlgItem(hwndDlg,IDC_ADDBP_MODE_X),FALSE);
				}
				else CheckDlgButton(hwndDlg, IDC_ADDBP_MEM_CPU, BST_CHECKED);

				UpdateDialog(hwndDlg);
				
// ################################## Start of SP CODE ###########################

				SendDlgItemMessage(hwndDlg,IDC_ADDBP_CONDITION,EM_SETLIMITTEXT,200,0);
				SendDlgItemMessage(hwndDlg,IDC_ADDBP_NAME,EM_SETLIMITTEXT,200,0);
				
				if (watchpoint[WP_edit].cond)
				{
					SetDlgItemText(hwndDlg, IDC_ADDBP_CONDITION, watchpoint[WP_edit].condText);
				}
				else
				{
					SetDlgItemText(hwndDlg, IDC_ADDBP_CONDITION, "");
				}
				
				if (watchpoint[WP_edit].desc)
				{
					SetDlgItemText(hwndDlg, IDC_ADDBP_NAME, watchpoint[WP_edit].desc);
				}
				else
				{
					SetDlgItemText(hwndDlg, IDC_ADDBP_NAME, "");
				}
				
// ################################## End of SP CODE ###########################
			}
			else CheckDlgButton(hwndDlg, IDC_ADDBP_MEM_CPU, BST_CHECKED);
			break;
		case WM_CLOSE:
		case WM_QUIT:
			break;
		case WM_COMMAND:
			switch(HIWORD(wParam)) {
				case BN_CLICKED:
					switch(LOWORD(wParam)) {
						case IDC_ADDBP_MODE_F: {
							UpdateDialog(hwndDlg);
							break;
						}

						case IDOK:
							if (WP_edit >= 0) {
								int tmp = NewBreakWindows(hwndDlg,WP_edit,(BOOL)(watchpoint[WP_edit].flags&WP_E));
								if (tmp == INVALID_BREAKPOINT_CONDITION)
								{
									MessageBox(hwndDlg, "Invalid breakpoint condition", "Error", MB_OK);
									break;
								}
								EndDialog(hwndDlg,1);
								break;
							}
							if ((tmp=AddBreak(hwndDlg)) == TOO_MANY_BREAKPOINTS) {
								MessageBox(hwndDlg, "Too many breakpoints, please delete one and try again", "Breakpoint Error", MB_OK);
								goto endaddbrk;
							}
							if (tmp == 2) goto endaddbrk;
							else if (tmp == INVALID_BREAKPOINT_CONDITION)
							{
								MessageBox(hwndDlg, "Invalid breakpoint condition", "Error", MB_OK);
								break;
							}
							EndDialog(hwndDlg,1);
							break;
						case IDCANCEL:
							endaddbrk:
							EndDialog(hwndDlg,0);
							break;
						case IDC_ADDBP_MEM_CPU:
							EnableWindow(GetDlgItem(hwndDlg,IDC_ADDBP_MODE_X),TRUE);
							break;
						case IDC_ADDBP_MEM_PPU:
						case IDC_ADDBP_MEM_SPR:
							EnableWindow(GetDlgItem(hwndDlg,IDC_ADDBP_MODE_X),FALSE);
							break;
					}
					break;
			}
        	break;
	}
	return FALSE; //TRUE;
}

void Disassemble(HWND hWnd, int id, int scrollid, unsigned int addr) {
// ################################## Start of SP CODE ###########################

// Changed 25 to 33 in this function for more disassembly lines
// Changed size of str (TODO: Better calculation of max size)
// Changed the buffer size of str to 35000

// ################################## End of SP CODE ###########################

	char str[35000]={0},chr[34]={0};
	int size;
	uint8 opcode[3];
	
// ################################## Start of SP CODE ###########################

	loadNameFiles();

// ################################## End of SP CODE ###########################

	si.nPos = addr;
	SetScrollInfo(GetDlgItem(hWnd,scrollid),SB_CTL,&si,TRUE);

	//figure out how many lines we can draw
	RECT rect;
	GetWindowRect(hWnd,&rect);
	int lines = (rect.bottom-rect.top)/13;


	for (int i = 0; i < lines; i++) {
		if (addr > 0xFFFF) break;
		if(addr >= 0x8000)
			sprintf(chr, "%02X:%04X", getBank(addr), addr);
		else sprintf(chr, "  :%04X", addr);
		
// ################################## Start of SP CODE ###########################

		symbDebugEnabled = IsDlgButtonChecked(hWnd, IDC_DEBUGGER_ENABLE_SYMBOLIC);
		
		decorateAddress(addr, str, chr, symbDebugEnabled);
		
// ################################## End of SP CODE ###########################

		if ((size = opsize[GetMem(addr)]) == 0) {
			sprintf(chr, "%02X        UNDEFINED", GetMem(addr++));
			strcat(str,chr);
		}
		else {
			char* a;
			if ((addr+size) > 0x10000) { //should this be 0xFFFF?
				while (addr < 0x10000) {
					sprintf(chr, "%02X        OVERFLOW\r\n", GetMem(addr++));
					strcat(str,chr);
				}
				break;
			}
			for (int j = 0; j < size; j++) {
				sprintf(chr, "%02X ", opcode[j] = GetMem(addr++));
				strcat(str,chr);
			}
			while (size < 3) {
				strcat(str,"   "); //pad output to align ASM
				size++;
			}
			
// ################################## Start of SP CODE ###########################

			a = Disassemble(addr,opcode);
			
			if (symbDebugEnabled)
			{
				replaceNames(ramBankNames, a);
				replaceNames(loadedBankNames, a);
				replaceNames(lastBankNames, a);
			}
			
// ################################## End of SP CODE ###########################

			strcat(strcat(str," "),a);
		}
		strcat(str,"\r\n");
	}
	SetDlgItemText(hWnd,id,str);
}

char *DisassembleLine(int addr) {
	static char str[64]={0},chr[25]={0};
	char *c;
	int size,j;
	uint8 opcode[3];

	sprintf(str, "%02X:%04X:", getBank(addr),addr);
	if ((size = opsize[GetMem(addr)]) == 0) {
		sprintf(chr, "%02X        UNDEFINED", GetMem(addr++));
		strcat(str,chr);
	}
	else {
		if ((addr+size) > 0x10000) {
			sprintf(chr, "%02X        OVERFLOW", GetMem(addr));
			strcat(str,chr);
		}
		else {
			for (j = 0; j < size; j++) {
				sprintf(chr, "%02X ", opcode[j] = GetMem(addr++));
				strcat(str,chr);
			}
			while (size < 3) {
				strcat(str,"   "); //pad output to align ASM
				size++;
			}
			strcat(strcat(str," "),Disassemble(addr,opcode));
		}
	}
	if ((c=strchr(str,'='))) *(c-1) = 0;
	if ((c=strchr(str,'@'))) *(c-1) = 0;
	return str;
}

char *DisassembleData(int addr, uint8 *opcode) {
	static char str[64]={0},chr[25]={0};
	char *c;
	int size,j;

	sprintf(str, "%02X:%04X:", getBank(addr), addr);
	if ((size = opsize[opcode[0]]) == 0) {
		sprintf(chr, "%02X        UNDEFINED", opcode[0]);
		strcat(str,chr);
	}
	else {
		if ((addr+size) > 0x10000) {
			sprintf(chr, "%02X        OVERFLOW", opcode[0]);
			strcat(str,chr);
		}
		else {
			for (j = 0; j < size; j++) {
				sprintf(chr, "%02X ", opcode[j]);
				addr++;
				strcat(str,chr);
			}
			while (size < 3) {
				strcat(str,"   "); //pad output to align ASM
				size++;
			}
			strcat(strcat(str," "),Disassemble(addr,opcode));
		}
	}
	if ((c=strchr(str,'='))) *(c-1) = 0;
	if ((c=strchr(str,'@'))) *(c-1) = 0;
	return str;
}


int GetEditHex(HWND hwndDlg, int id) {
	char str[9];
	int tmp;
	GetDlgItemText(hwndDlg,id,str,9);
	tmp = strtol(str,NULL,16);
	return tmp;
}

int *GetEditHexData(HWND hwndDlg, int id){
	static int data[31];
	char str[60];
	int i,j, k;

	GetDlgItemText(hwndDlg,id,str,60);
	memset(data,0,30*sizeof(int));
	j=0;
	for(i = 0;i < 60;i++){
		if(str[i] == 0)break;
		if((str[i] >= '0') && (str[i] <= '9'))j++;
		if((str[i] >= 'A') && (str[i] <= 'F'))j++;
		if((str[i] >= 'a') && (str[i] <= 'f'))j++;
	}

	j=j&1;
	for(i = 0;i < 60;i++){
		if(str[i] == 0)break;
		k = -1;
		if((str[i] >= '0') && (str[i] <= '9'))k=str[i]-'0';
		if((str[i] >= 'A') && (str[i] <= 'F'))k=(str[i]-'A')+10;
		if((str[i] >= 'a') && (str[i] <= 'f'))k=(str[i]-'a')+10;
		if(k != -1){
			if(j&1)data[j>>1] |= k;
			else data[j>>1] |= k<<4;
			j++;
		}
	}
	data[j>>1]=-1;
	return data;
}

/*
int GetEditStack(HWND hwndDlg) {
	char str[85];
	int tmp;
	GetDlgItemText(hwndDlg,IDC_DEBUGGER_STACK_CONTENTS,str,85);
	sscanf(str,"%2x,%2x,%2x,%2x,\r\n",&tmp);
	return tmp;
}
*/

void UpdateRegs(HWND hwndDlg) {
	X.A = GetEditHex(hwndDlg,IDC_DEBUGGER_VAL_A);
	X.X = GetEditHex(hwndDlg,IDC_DEBUGGER_VAL_X);
	X.Y = GetEditHex(hwndDlg,IDC_DEBUGGER_VAL_Y);
	X.PC = GetEditHex(hwndDlg,IDC_DEBUGGER_VAL_PC);
}

///indicates whether we're under the control of the debugger
bool inDebugger = false;

//this code enters the debugger.
void FCEUD_DebugBreakpoint() {
	UpdateDebugger();
	void win_debuggerLoop(); //HACK
	win_debuggerLoop();
}

void UpdateDebugger()
{
	//don't do anything if the debugger is not visible
	if(!hDebug)
		return;

	//but if the debugger IS visible, then focus it
	SetActiveWindow(hDebug);

	char str[256]={0},chr[8];
	int tmp,ret,i;

	Disassemble(hDebug, IDC_DEBUGGER_DISASSEMBLY, IDC_DEBUGGER_DISASSEMBLY_VSCR, X.PC);

	sprintf(str, "%02X", X.A);
	SetDlgItemText(hDebug, IDC_DEBUGGER_VAL_A, str);
	sprintf(str, "%02X", X.X);
	SetDlgItemText(hDebug, IDC_DEBUGGER_VAL_X, str);
	sprintf(str, "%02X", X.Y);
	SetDlgItemText(hDebug, IDC_DEBUGGER_VAL_Y, str);
	sprintf(str, "%04X", (int)X.PC);
	SetDlgItemText(hDebug, IDC_DEBUGGER_VAL_PC, str);

	sprintf(str, "%04X", (int)RefreshAddr);
	SetDlgItemText(hDebug, IDC_DEBUGGER_VAL_PPU, str);
	sprintf(str, "%02X", PPU[3]);
	SetDlgItemText(hDebug, IDC_DEBUGGER_VAL_SPR, str);

	extern int linestartts;
	#define GETLASTPIXEL    (PAL?((timestamp*48-linestartts)/15) : ((timestamp*48-linestartts)/16) )
	
	int ppupixel = GETLASTPIXEL;

	if (ppupixel>341)	//maximum number of pixels per scanline
		ppupixel = 0;	//Currently pixel display is borked until Run 128 lines is clicked, this keeps garbage from displaying

	//If not in the 0-239 pixel range, make special cases for display
	if (scanline == 240 && vblankScanLines < 22)
	{
		if (!vblankScanLines)								//Idle scanline (240)
			sprintf(str, "Idle %d\t       %d",scanline,vblankPixel);
		else if (scanline + vblankScanLines == 261)
			sprintf(str, "Prerender -1  %d", vblankPixel);	//Pre-render
		else
			sprintf(str, "Vblank %d    %d", scanline+vblankScanLines,vblankPixel);	//Vblank lines (241-260)
	}
	else	
		sprintf(str, "%d\t        %d", scanline,ppupixel);	//Scanlines 0 - 239
	
	SetDlgItemText(hDebug, IDC_DEBUGGER_VAL_SLINE, str);

	tmp = X.S|0x0100;
	sprintf(str, "Stack $%04X", tmp);
	SetDlgItemText(hDebug, IDC_DEBUGGER_VAL_S, str);
	tmp = ((tmp+1)|0x0100)&0x01FF;
	sprintf(str, "%02X", GetMem(tmp));
	for (i = 1; i < 28; i++) {
		tmp = ((tmp+1)|0x0100)&0x01FF;  //increment and fix pointer to $0100-$01FF range
		if ((i%4) == 0) sprintf(chr, ",\r\n%02X", GetMem(tmp));
		else sprintf(chr, ",%02X", GetMem(tmp));
		strcat(str,chr);
	}
	SetDlgItemText(hDebug, IDC_DEBUGGER_STACK_CONTENTS, str);

	GetDlgItemText(hDebug,IDC_DEBUGGER_VAL_PCSEEK,str,5);
	if (((ret = sscanf(str,"%4X",&tmp)) == EOF) || (ret != 1)) tmp = 0;
	sprintf(str,"%04X",tmp);
	SetDlgItemText(hDebug,IDC_DEBUGGER_VAL_PCSEEK,str);

	CheckDlgButton(hDebug, IDC_DEBUGGER_FLAG_N, BST_UNCHECKED);
	CheckDlgButton(hDebug, IDC_DEBUGGER_FLAG_V, BST_UNCHECKED);
	CheckDlgButton(hDebug, IDC_DEBUGGER_FLAG_U, BST_UNCHECKED);
	CheckDlgButton(hDebug, IDC_DEBUGGER_FLAG_B, BST_UNCHECKED);
	CheckDlgButton(hDebug, IDC_DEBUGGER_FLAG_D, BST_UNCHECKED);
	CheckDlgButton(hDebug, IDC_DEBUGGER_FLAG_I, BST_UNCHECKED);
	CheckDlgButton(hDebug, IDC_DEBUGGER_FLAG_Z, BST_UNCHECKED);
	CheckDlgButton(hDebug, IDC_DEBUGGER_FLAG_C, BST_UNCHECKED);

	tmp = X.P;
	if (tmp & N_FLAG) CheckDlgButton(hDebug, IDC_DEBUGGER_FLAG_N, BST_CHECKED);
	if (tmp & V_FLAG) CheckDlgButton(hDebug, IDC_DEBUGGER_FLAG_V, BST_CHECKED);
	if (tmp & U_FLAG) CheckDlgButton(hDebug, IDC_DEBUGGER_FLAG_U, BST_CHECKED);
	if (tmp & B_FLAG) CheckDlgButton(hDebug, IDC_DEBUGGER_FLAG_B, BST_CHECKED);
	if (tmp & D_FLAG) CheckDlgButton(hDebug, IDC_DEBUGGER_FLAG_D, BST_CHECKED);
	if (tmp & I_FLAG) CheckDlgButton(hDebug, IDC_DEBUGGER_FLAG_I, BST_CHECKED);
	if (tmp & Z_FLAG) CheckDlgButton(hDebug, IDC_DEBUGGER_FLAG_Z, BST_CHECKED);
	if (tmp & C_FLAG) CheckDlgButton(hDebug, IDC_DEBUGGER_FLAG_C, BST_CHECKED);
}

char *BreakToText(unsigned int num) {
	static char str[230],chr[8];

	sprintf(str, "$%04X", watchpoint[num].address);
	if (watchpoint[num].endaddress) {
		sprintf(chr, "-$%04X", watchpoint[num].endaddress);
		strcat(str,chr);
	}
	if (watchpoint[num].flags&WP_E) strcat(str,": E"); else strcat(str,": -");
	if (watchpoint[num].flags&BT_P) strcat(str,"P"); else if (watchpoint[num].flags&BT_S) strcat(str,"S"); else strcat(str,"C");
	if (watchpoint[num].flags&WP_R) strcat(str,"R"); else strcat(str,"-");
	if (watchpoint[num].flags&WP_W) strcat(str,"W"); else strcat(str,"-");
	if (watchpoint[num].flags&WP_X) strcat(str,"X"); else strcat(str,"-");
	if (watchpoint[num].flags&WP_F) strcat(str,"F"); else strcat(str,"-");
	
// ################################## Start of SP CODE ###########################
	if (watchpoint[num].desc)
	{
		strcat(str, " : ");
		strcat(str, watchpoint[num].desc);
	}
// ################################## End of SP CODE ###########################

	return str;
}

void AddBreakList() {
	SendDlgItemMessage(hDebug,IDC_DEBUGGER_BP_LIST,LB_INSERTSTRING,-1,(LPARAM)(LPSTR)BreakToText(numWPs-1));
}

void EditBreakList() {
	if (WP_edit >= 0) {
		SendDlgItemMessage(hDebug,IDC_DEBUGGER_BP_LIST,LB_DELETESTRING,WP_edit,0);
		SendDlgItemMessage(hDebug,IDC_DEBUGGER_BP_LIST,LB_INSERTSTRING,WP_edit,(LPARAM)(LPSTR)BreakToText(WP_edit));
		SendDlgItemMessage(hDebug,IDC_DEBUGGER_BP_LIST,LB_SETCURSEL,WP_edit,0);
	}
}

void FillBreakList(HWND hwndDlg) {
	int i;

	for (i = 0; i < numWPs; i++) {
		SendDlgItemMessage(hwndDlg,IDC_DEBUGGER_BP_LIST,LB_INSERTSTRING,-1,(LPARAM)(LPSTR)BreakToText(i));
	}
}

void EnableBreak(int sel) {
	watchpoint[sel].flags^=WP_E;
	SendDlgItemMessage(hDebug,IDC_DEBUGGER_BP_LIST,LB_DELETESTRING,sel,0);
	SendDlgItemMessage(hDebug,IDC_DEBUGGER_BP_LIST,LB_INSERTSTRING,sel,(LPARAM)(LPSTR)BreakToText(sel));
	SendDlgItemMessage(hDebug,IDC_DEBUGGER_BP_LIST,LB_SETCURSEL,sel,0);
}

void DeleteBreak(int sel) {
	int i;

	for (i = sel; i < numWPs; i++) {
		watchpoint[i].address = watchpoint[i+1].address;
		watchpoint[i].endaddress = watchpoint[i+1].endaddress;
		watchpoint[i].flags = watchpoint[i+1].flags;
// ################################## Start of SP CODE ###########################
		watchpoint[i].cond = watchpoint[i+1].cond;
		watchpoint[i].condText = watchpoint[i+1].condText;
		watchpoint[i].desc = watchpoint[i+1].desc;
// ################################## End of SP CODE ###########################
	}
	numWPs--;
// ################################## Start of SP CODE ###########################
	myNumWPs--;
// ################################## End of SP CODE ###########################
	SendDlgItemMessage(hDebug,IDC_DEBUGGER_BP_LIST,LB_DELETESTRING,sel,0);
	EnableWindow(GetDlgItem(hDebug,IDC_DEBUGGER_BP_DEL),FALSE);
	EnableWindow(GetDlgItem(hDebug,IDC_DEBUGGER_BP_EDIT),FALSE);
}

void KillDebugger() {
	SendDlgItemMessage(hDebug,IDC_DEBUGGER_BP_LIST,LB_RESETCONTENT,0,0);
	FCEUI_Debugger().reset();
	FCEUI_SetEmulationPaused(0); //mbg merge 7/18/06 changed from userpause
}


int AddAsmHistory(HWND hwndDlg, int id, char *str) {
	int index;
	index = SendDlgItemMessage(hwndDlg,id,CB_FINDSTRINGEXACT,-1,(LPARAM)(LPSTR)str);
	if (index == CB_ERR) {
		SendDlgItemMessage(hwndDlg,id,CB_INSERTSTRING,-1,(LPARAM)(LPSTR)str);
		return 0;
	}
	return 1;
}

BOOL CALLBACK AssemblerCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	int romaddr,count,i,j;
	char str[128],*dasm;
	static int patchlen,applied,saved,lastundo;
	static uint8 patchdata[64][3],undodata[64*3];
	uint8 *ptr;

	switch(uMsg) {
		case WM_INITDIALOG:
			CenterWindow(hwndDlg);

			//set font
			SendDlgItemMessage(hwndDlg,IDC_ASSEMBLER_DISASSEMBLY,WM_SETFONT,(WPARAM)debugSystem->hFixedFont,FALSE);
			SendDlgItemMessage(hwndDlg,IDC_ASSEMBLER_PATCH_DISASM,WM_SETFONT,(WPARAM)debugSystem->hFixedFont,FALSE);

			//set limits
			SendDlgItemMessage(hwndDlg,IDC_ASSEMBLER_HISTORY,CB_LIMITTEXT,20,0);

			SetDlgItemText(hwndDlg,IDC_ASSEMBLER_DISASSEMBLY,DisassembleLine(iaPC));
			SetFocus(GetDlgItem(hwndDlg,IDC_ASSEMBLER_HISTORY));

			patchlen = 0;
			applied = 0;
			saved = 0;
			lastundo = 0;
			break;
		case WM_CLOSE:
		case WM_QUIT:
			EndDialog(hwndDlg,0);
			break;
		case WM_COMMAND:
			switch (HIWORD(wParam)) {
				case BN_CLICKED:
					switch (LOWORD(wParam)) {
						case IDC_ASSEMBLER_APPLY:
							if (patchlen) {
								ptr = GetNesPRGPointer(GetNesFileAddress(iaPC)-16);
								count = 0;
								for (i = 0; i < patchlen; i++) {
									for (j = 0; j < opsize[patchdata[i][0]]; j++) {
										if (count == lastundo) undodata[lastundo++] = ptr[count];
										ptr[count++] = patchdata[i][j];
									}
								}
								SetWindowText(hwndDlg, "Inline Assembler  *Patches Applied*");
								//MessageBeep(MB_OK);
								applied = 1;
							}
							break;
						case IDC_ASSEMBLER_SAVE:
							if (applied) {
								count = romaddr = GetNesFileAddress(iaPC);
								for (i = 0; i < patchlen; i++) count += opsize[patchdata[i][0]];
								if (patchlen) sprintf(str,"Write patch data to file at addresses 0x%06X - 0x%06X?",romaddr,count-1);
								else sprintf(str,"Undo all previously applied patches?");
								if (MessageBox(hwndDlg, str, "Save changes to file?", MB_YESNO|MB_ICONINFORMATION) == IDYES) {
									if (iNesSave()) {
										saved = 1;
										applied = 0;
									}
									else MessageBox(hwndDlg, "Unable to save changes to file", "Error saving to file", MB_OK);
								}
							}
							break;
						case IDC_ASSEMBLER_UNDO:
							if ((count = SendDlgItemMessage(hwndDlg,IDC_ASSEMBLER_PATCH_DISASM,LB_GETCOUNT,0,0))) {
								SendDlgItemMessage(hwndDlg,IDC_ASSEMBLER_PATCH_DISASM,LB_DELETESTRING,count-1,0);
								patchlen--;
								count = 0;
								for (i = 0; i < patchlen; i++) count += opsize[patchdata[i][0]];
								if (count < lastundo) {
									ptr = GetNesPRGPointer(GetNesFileAddress(iaPC)-16);
									j = opsize[patchdata[patchlen][0]];
									for (i = count; i < (count+j); i++) {
										ptr[i] = undodata[i];
									}
									lastundo -= j;
									applied = 1;
								}
								SetDlgItemText(hwndDlg,IDC_ASSEMBLER_DISASSEMBLY,DisassembleLine(iaPC+count));
							}
							break;
						case IDC_ASSEMBLER_DEFPUSHBUTTON:
							count = 0;
							for (i = 0; i < patchlen; i++) count += opsize[patchdata[i][0]];
							GetDlgItemText(hwndDlg,IDC_ASSEMBLER_HISTORY,str,21);
							if (!Assemble(patchdata[patchlen],(iaPC+count),str)) {
								count = iaPC;
								for (i = 0; i <= patchlen; i++) count += opsize[patchdata[i][0]];
								if (count > 0x10000) { //note: don't use 0xFFFF!
									MessageBox(hwndDlg, "Patch data cannot exceed address 0xFFFF", "Address error", MB_OK);
									break;
								}
								SetDlgItemText(hwndDlg,IDC_ASSEMBLER_HISTORY,"");
								if (count < 0x10000) SetDlgItemText(hwndDlg,IDC_ASSEMBLER_DISASSEMBLY,DisassembleLine(count));
								else SetDlgItemText(hwndDlg,IDC_ASSEMBLER_DISASSEMBLY,"OVERFLOW");
								dasm = DisassembleData((count-opsize[patchdata[patchlen][0]]),patchdata[patchlen]);
								SendDlgItemMessage(hwndDlg,IDC_ASSEMBLER_PATCH_DISASM,LB_INSERTSTRING,-1,(LPARAM)(LPSTR)dasm);
								AddAsmHistory(hwndDlg,IDC_ASSEMBLER_HISTORY,dasm+16);
								SetWindowText(hwndDlg, "Inline Assembler");
								patchlen++;
							}
							else { //ERROR!
								SetWindowText(hwndDlg, "Inline Assembler  *Syntax Error*");
								MessageBeep(MB_ICONEXCLAMATION);
							}
							break;
					}
					SetFocus(GetDlgItem(hwndDlg,IDC_ASSEMBLER_HISTORY)); //set focus to combo box after anything is pressed!
					break;
			}
			break;
	}
	return FALSE;
}

BOOL CALLBACK PatcherCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	char str[64]; //mbg merge 7/18/06 changed from unsigned char
	uint8 *c;
	int i;
	int *p;

	switch(uMsg) {
		case WM_INITDIALOG:
			CenterWindow(hwndDlg);

			//set limits
			SendDlgItemMessage(hwndDlg,IDC_ROMPATCHER_OFFSET,EM_SETLIMITTEXT,6,0);
			SendDlgItemMessage(hwndDlg,IDC_ROMPATCHER_PATCH_DATA,EM_SETLIMITTEXT,30,0);
			UpdatePatcher(hwndDlg);

			if(iapoffset != -1){
				CheckDlgButton(hwndDlg, IDC_ROMPATCHER_DOTNES_OFFSET, BST_CHECKED);
				sprintf((char*)str,"%X",iapoffset); //mbg merge 7/18/06 added cast
				SetDlgItemText(hwndDlg,IDC_ROMPATCHER_OFFSET,str);
			}

			SetFocus(GetDlgItem(hwndDlg,IDC_ROMPATCHER_OFFSET_BOX));
			break;
		case WM_CLOSE:
		case WM_QUIT:
			EndDialog(hwndDlg,0);
			break;
		case WM_COMMAND:
			switch(HIWORD(wParam)) {
				case BN_CLICKED:
					switch(LOWORD(wParam)) {
						case IDC_ROMPATCHER_BTN_EDIT: //todo: maybe get rid of this button and cause iapoffset to update every time you change the text
							if(IsDlgButtonChecked(hwndDlg,IDC_ROMPATCHER_DOTNES_OFFSET) == BST_CHECKED){
								iapoffset = GetEditHex(hwndDlg,IDC_ROMPATCHER_OFFSET);
							} else iapoffset = GetNesFileAddress(GetEditHex(hwndDlg,IDC_ROMPATCHER_OFFSET));
							if((iapoffset < 16) && (iapoffset != -1)){
								MessageBox(hDebug, "Sorry, iNes Header editing isn't supported", "Error", MB_OK);
								iapoffset = -1;
							}
							if((iapoffset > PRGsize[0]) && (iapoffset != -1)){
								MessageBox(hDebug, "Error: .Nes offset outside of PRG rom", "Error", MB_OK);
								iapoffset = -1;
							}
							UpdatePatcher(hwndDlg);
							break;
						case IDC_ROMPATCHER_BTN_APPLY:
							p = GetEditHexData(hwndDlg,IDC_ROMPATCHER_PATCH_DATA);
							i=0;
							c = GetNesPRGPointer(iapoffset-16);
							while(p[i] != -1){
								c[i] = p[i];
								i++;
							}
							UpdatePatcher(hwndDlg);
							break;
						case IDC_ROMPATCHER_BTN_SAVE:
							if(!iNesSave())MessageBox(NULL,"Error Saving","Error",MB_OK);
							break;
					}
					break;
			}
			break;
	}
	return FALSE;
}

extern char *iNesShortFName();

void DebuggerExit()
{
	FCEUI_Debugger().badopbreak = 0;
	debugger_open = 0;
	DestroyWindow(hDebug);
}

static RECT currDebuggerRect;
static RECT newDebuggerRect;

//used to move all child items in the dialog when you resize (except for the dock fill controls which are resized)
BOOL CALLBACK DebuggerEnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	int dx = (newDebuggerRect.right-newDebuggerRect.left)-(currDebuggerRect.right-currDebuggerRect.left);	//Calculate & store difference in width of old size vs new size
	int dy = (newDebuggerRect.bottom-newDebuggerRect.top)-(currDebuggerRect.bottom-currDebuggerRect.top);	//ditto wtih height

	HWND editbox = GetDlgItem(hDebug,IDC_DEBUGGER_DISASSEMBLY);		//Get handle for Disassembly list box (large guy on the left)
	HWND icontray = GetDlgItem(hDebug,IDC_DEBUGGER_ICONTRAY);		//Get handle for IContray, vertical column to the left of disassembly
	HWND addrline = GetDlgItem(hDebug,IDC_DEBUGGER_ADDR_LINE);		//Get handle of address line (text area under the disassembly
	HWND vscr = GetDlgItem(hDebug,IDC_DEBUGGER_DISASSEMBLY_VSCR);	//Get handle for disassembly Vertical Scrollbar

	RECT crect;
	GetWindowRect(hwnd,&crect);					//Get rect of current child to be resized
	ScreenToClient(hDebug,(LPPOINT)&crect);		//Convert rect coordinates to client area coordinates
	ScreenToClient(hDebug,((LPPOINT)&crect)+1);

	if(hwnd == editbox) {
		crect.right += dx;
		crect.bottom += dy;
		SetWindowPos(hwnd,0,0,0,crect.right-crect.left,crect.bottom-crect.top,SWP_NOZORDER | SWP_NOMOVE);
		GetScrollInfo(GetDlgItem(hDebug,IDC_DEBUGGER_DISASSEMBLY_VSCR),SB_CTL,&si);
		Disassemble(hDebug, IDC_DEBUGGER_DISASSEMBLY, IDC_DEBUGGER_DISASSEMBLY_VSCR, si.nPos);
	} else if(hwnd == icontray) {
		crect.bottom += dy;
		SetWindowPos(hwnd,0,0,0,crect.right-crect.left,crect.bottom-crect.top,SWP_NOZORDER | SWP_NOMOVE);
	} else if(hwnd == addrline) {
		crect.top += dy;
		crect.right += dx;
		SetWindowPos(hwnd,0,crect.left,crect.top,crect.right-crect.left,crect.bottom-crect.top,SWP_NOZORDER);
	} else if(hwnd == vscr) {
		 crect.bottom += dy;
		 crect.left += dx;
		 crect.right += dx;
		 SetWindowPos(hwnd,0,crect.left,crect.top,crect.right-crect.left,crect.bottom-crect.top,SWP_NOZORDER);
	} else {
		crect.left += dx;
		//if (crect.left < 256) crect.left = 256;		//Limit how far left the remaining child windows will move
		SetWindowPos(hwnd,0,crect.left,crect.top,0,0,SWP_NOZORDER | SWP_NOSIZE);
	}
	return TRUE;
}

BOOL CALLBACK DebuggerCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	RECT wrect;
	char str[256]={0},*ptr,dotdot[4];
	int tmp,tmp2;
	int mouse_x,mouse_y;
	int ret,i;
	//FILE *fp;

	//these messages get handled at any time
	switch(uMsg) {
		case WM_INITDIALOG: {
			extern int loadDebugDataFailed;
			if (DbgPosX==-32000) DbgPosX=0; //Just in case
			if (DbgPosY==-32000) DbgPosY=0;
			SetWindowPos(hwndDlg,0,DbgPosX,DbgPosY,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER);

			GetWindowRect(hwndDlg,&currDebuggerRect);

			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_ALL;
			si.nMin = 0;
			si.nMax = 0x10000;
			si.nPos = 0;
			si.nPage = 20;
			SetScrollInfo(GetDlgItem(hwndDlg,IDC_DEBUGGER_DISASSEMBLY_VSCR),SB_CTL,&si,TRUE);

			//setup font
			SendDlgItemMessage(hwndDlg,IDC_DEBUGGER_DISASSEMBLY,WM_SETFONT,(WPARAM)debugSystem->hFixedFont,FALSE);
			SendDlgItemMessage(hwndDlg,IDC_DEBUGGER_VAL_A,WM_SETFONT,(WPARAM)debugSystem->hFixedFont,FALSE);
			SendDlgItemMessage(hwndDlg,IDC_DEBUGGER_VAL_X,WM_SETFONT,(WPARAM)debugSystem->hFixedFont,FALSE);
			SendDlgItemMessage(hwndDlg,IDC_DEBUGGER_VAL_Y,WM_SETFONT,(WPARAM)debugSystem->hFixedFont,FALSE);
			SendDlgItemMessage(hwndDlg,IDC_DEBUGGER_VAL_PC,WM_SETFONT,(WPARAM)debugSystem->hFixedFont,FALSE);
			SendDlgItemMessage(hwndDlg,IDC_DEBUGGER_STACK_CONTENTS,WM_SETFONT,(WPARAM)debugSystem->hFixedFont,FALSE);
			SendDlgItemMessage(hwndDlg,IDC_DEBUGGER_VAL_PCSEEK,WM_SETFONT,(WPARAM)debugSystem->hFixedFont,FALSE);
			SendDlgItemMessage(hwndDlg,IDC_DEBUGGER_VAL_PPU,WM_SETFONT,(WPARAM)debugSystem->hFixedFont,FALSE);
			SendDlgItemMessage(hwndDlg,IDC_DEBUGGER_VAL_SPR,WM_SETFONT,(WPARAM)debugSystem->hFixedFont,FALSE);

			//text limits
			SendDlgItemMessage(hwndDlg,IDC_DEBUGGER_VAL_A,EM_SETLIMITTEXT,2,0);
			SendDlgItemMessage(hwndDlg,IDC_DEBUGGER_VAL_X,EM_SETLIMITTEXT,2,0);
			SendDlgItemMessage(hwndDlg,IDC_DEBUGGER_VAL_Y,EM_SETLIMITTEXT,2,0);
			SendDlgItemMessage(hwndDlg,IDC_DEBUGGER_VAL_PC,EM_SETLIMITTEXT,4,0);
			SendDlgItemMessage(hwndDlg,IDC_DEBUGGER_STACK_CONTENTS,EM_SETLIMITTEXT,83,0);
			SendDlgItemMessage(hwndDlg,IDC_DEBUGGER_VAL_PCSEEK,EM_SETLIMITTEXT,4,0);
			SendDlgItemMessage(hwndDlg,IDC_DEBUGGER_VAL_PPU,EM_SETLIMITTEXT,4,0);
			SendDlgItemMessage(hwndDlg,IDC_DEBUGGER_VAL_SPR,EM_SETLIMITTEXT,2,0);

			//I'm lazy, disable the controls which I can't mess with right now
			SendDlgItemMessage(hwndDlg,IDC_DEBUGGER_VAL_PPU,EM_SETREADONLY,TRUE,0);
			SendDlgItemMessage(hwndDlg,IDC_DEBUGGER_VAL_SPR,EM_SETREADONLY,TRUE,0);

// ################################## Start of SP CODE ###########################

			SendDlgItemMessage(hwndDlg,IDC_DEBUGGER_BOOKMARK,EM_SETLIMITTEXT,4,0);
					
			if (!loadDebugDataFailed)
			{
				if (bookmarks)
				{
					unsigned int i;
					for (i=0;i<bookmarks;i++)
					{
						char buffer[5];
						sprintf(buffer, "%X", bookmarkData[i]);
						AddDebuggerBookmark2(hwndDlg, buffer);
					}
				}
				
				if (symbDebugEnabled)
				{
					CheckDlgButton(hwndDlg, IDC_DEBUGGER_ENABLE_SYMBOLIC, BST_CHECKED);
				}

				numWPs = myNumWPs;
			}
			else
			{
				bookmarks = 0;
			}
			
			debuggerWasActive = 1;
			
// ################################## End of SP CODE ###########################

			FCEUI_Debugger().badopbreak = false;
			debugger_open = 1;
			FillBreakList(hwndDlg);
			break;
		}

		case WM_SIZE: {
			if(wParam == SIZE_RESTORED)										//If dialog was resized
			{
				GetWindowRect(hwndDlg,&newDebuggerRect);					//Get new size
				//Force a minimum Dialog size-------------------------------
				if (newDebuggerRect.right - newDebuggerRect.left < 368 || newDebuggerRect.bottom - newDebuggerRect.top < 150)	//If either x or y is too small run the force size routine
				{
					if (newDebuggerRect.right - newDebuggerRect.left < 367)	//If width is too small reset to previous width
					{
						newDebuggerRect.right = currDebuggerRect.right;
						newDebuggerRect.left = currDebuggerRect.left;
						
					}
					if (newDebuggerRect.bottom - newDebuggerRect.top < 150)	//If heigth is too small reset to previous height
					{
						newDebuggerRect.top = currDebuggerRect.top;
						newDebuggerRect.bottom = currDebuggerRect.bottom;
					}
				SetWindowPos(hwndDlg,HWND_TOPMOST,newDebuggerRect.left,newDebuggerRect.top,(newDebuggerRect.right-newDebuggerRect.left),(newDebuggerRect.bottom-newDebuggerRect.top),SWP_SHOWWINDOW);
				}
				//Else run normal resizing procedure-------------------------
				else														
				{
					DbgSizeX = newDebuggerRect.right-newDebuggerRect.left;	//Store new size (this will be used to store in the .cfg file)	
					DbgSizeY = newDebuggerRect.bottom-newDebuggerRect.top;
					EnumChildWindows(hwndDlg,DebuggerEnumWindowsProc,0);	//Initiate callback for resizing child windows
					currDebuggerRect = newDebuggerRect;						//Store current debugger window size (for future calculations in EnumChildWindows
					InvalidateRect(hwndDlg,0,TRUE);
					UpdateWindow(hwndDlg);
				}
			}
			break;
		}


		case WM_CLOSE:
		case WM_QUIT:
			//exitdebug:
			DebuggerExit();
			break;
		case WM_MOVING:
			break;
		case WM_MOVE:
			GetWindowRect(hwndDlg,&wrect);
			DbgPosX = wrect.left;
			DbgPosY = wrect.top;
			break;
	}

	//these messages only get handled when a game is loaded
	if (GameInfo) {
		switch(uMsg) {
			case WM_VSCROLL:
				//mbg merge 7/18/06 changed pausing check
				if (FCEUI_EmulationPaused()) UpdateRegs(hwndDlg);
				if (lParam) {
					GetScrollInfo((HWND)lParam,SB_CTL,&si);
					switch(LOWORD(wParam)) {
						case SB_ENDSCROLL:
						case SB_TOP:
						case SB_BOTTOM: break;
						case SB_LINEUP: si.nPos--; break;
						case SB_LINEDOWN:
							if ((tmp=opsize[GetMem(si.nPos)])) si.nPos+=tmp;
							else si.nPos++;
							break;
						case SB_PAGEUP: si.nPos-=si.nPage; break;
						case SB_PAGEDOWN: si.nPos+=si.nPage; break;
						case SB_THUMBPOSITION: //break;
						case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
					}
					if (si.nPos < si.nMin) si.nPos = si.nMin;
					if ((si.nPos+(int)si.nPage) > si.nMax) si.nPos = si.nMax-si.nPage; //mbg merge 7/18/06 added cast
					SetScrollInfo((HWND)lParam,SB_CTL,&si,TRUE);
					Disassemble(hDebug, IDC_DEBUGGER_DISASSEMBLY, IDC_DEBUGGER_DISASSEMBLY_VSCR, si.nPos);
				}
				break;

			case WM_MOUSEWHEEL: //just handle page up/down and mousewheel messages together
				i = (short)HIWORD(wParam);///WHEEL_DELTA;
				GetScrollInfo((HWND)lParam,SB_CTL,&si);
				if(i < 0)si.nPos+=si.nPage;
				if(i > 0)si.nPos-=si.nPage;
				if (si.nPos < si.nMin) si.nPos = si.nMin;
				if ((si.nPos+(int)si.nPage) > si.nMax) si.nPos = si.nMax-si.nPage; //mbg merge 7/18/06 added cast
				SetScrollInfo((HWND)lParam,SB_CTL,&si,TRUE);
				Disassemble(hDebug, IDC_DEBUGGER_DISASSEMBLY, IDC_DEBUGGER_DISASSEMBLY_VSCR, si.nPos);
				break;

			case WM_KEYDOWN:
				MessageBox(hwndDlg,"Die!","I'm dead!",MB_YESNO|MB_ICONINFORMATION);
				//i=0;
				//if(uMsg == WM_KEYDOWN){
				//	if(wParam == VK_PRIOR) i = -1;
				//	if(wParam == VK_NEXT) i = 1;
				//}
				break;

			case WM_MOUSEMOVE:
				mouse_x = GET_X_LPARAM(lParam);
				mouse_y = GET_Y_LPARAM(lParam);
// ################################## Start of SP CODE ###########################

	// mouse_y < 538
	// > 33) tmp = 33

				if ((mouse_x > 8) && (mouse_x < 22) && (mouse_y > 10) && (mouse_y < 538)) {
					if ((tmp=((mouse_y - 10) / 13)) > 33) tmp = 33;
					
// ################################## End of SP CODE ###########################

					i = si.nPos;
					while (tmp > 0) {
						if ((tmp2=opsize[GetMem(i)]) == 0) tmp2++;
						if ((i+=tmp2) > 0xFFFF) {
							i = 0xFFFF;
							break;
						}
						tmp--;
					}
					if (i >= 0x8000) {
						dotdot[0] = 0;
						if (!(ptr = iNesShortFName())) ptr = "...";
						if (strlen(ptr) > 60) strcpy(dotdot,"...");
						if (GetNesFileAddress(i) == -1) sprintf(str,"CPU Address $%04X, Error retreiving ROM File Address!",i);
// ################################## Start of SP CODE ###########################
						else sprintf(str,"CPU Address %02X:%04X, Offset 0x%06X in file \"%.20s%s\" (NL file: %X)",getBank(i),i,GetNesFileAddress(i),ptr,dotdot,getBank(i));
// ################################## End of SP CODE ###########################
						SetDlgItemText(hwndDlg,IDC_DEBUGGER_ADDR_LINE,str);
					}
					else SetDlgItemText(hwndDlg,IDC_DEBUGGER_ADDR_LINE,"");
				}
				else SetDlgItemText(hwndDlg,IDC_DEBUGGER_ADDR_LINE,"");
				break;
			case WM_LBUTTONDOWN:
				mouse_x = GET_X_LPARAM(lParam);
				mouse_y = GET_Y_LPARAM(lParam);
// ################################## Start of SP CODE ###########################

	// mouse_y < 538
	// > 33) tmp = 33
				//mbg merge 7/18/06 changed pausing check
				if (FCEUI_EmulationPaused() && (mouse_x > 8) && (mouse_x < 22) && (mouse_y > 10) && (mouse_y < 538)) {
					if ((tmp=((mouse_y - 10) / 13)) > 33) tmp = 33;
// ################################## End of SP CODE ###########################
					i = si.nPos;
					while (tmp > 0) {
						if ((tmp2=opsize[GetMem(i)]) == 0) tmp2++;
						if ((i+=tmp2) > 0xFFFF) {
							i = 0xFFFF;
							break;
						}
						tmp--;
					}
					//DoPatcher(GetNesFileAddress(i),hwndDlg);
					iaPC=i;
					if (iaPC >= 0x8000) {
						DialogBox(fceu_hInstance,"ASSEMBLER",hwndDlg,AssemblerCallB);
						UpdateDebugger();
					}
				}
				break;
			case WM_RBUTTONDOWN:
				mouse_x = GET_X_LPARAM(lParam);
				mouse_y = GET_Y_LPARAM(lParam);
				//mbg merge 7/18/06 changed pausing check
				if (FCEUI_EmulationPaused() && (mouse_x > 8) && (mouse_x < 22) && (mouse_y > 10) && (mouse_y < 338)) 
				{
					if ((tmp=((mouse_y - 10) / 13)) > 24) tmp = 24;
					i = si.nPos;
					while (tmp > 0) {
						if ((tmp2=opsize[GetMem(i)]) == 0) tmp2++;
						if ((i+=tmp2) > 0xFFFF) {
							i = 0xFFFF;
							break;
						}
						tmp--;
					}
					ChangeMemViewFocus(2,GetNesFileAddress(i),-1);
				}
				break;
			case WM_MBUTTONDOWN:
				mouse_x = GET_X_LPARAM(lParam);
				mouse_y = GET_Y_LPARAM(lParam);
				//mbg merge 7/18/06 changed pausing check
				if (FCEUI_EmulationPaused() && (mouse_x > 8) && (mouse_x < 22) && (mouse_y > 10) && (mouse_y < 338)) 
				{
					if ((tmp=((mouse_y - 10) / 13)) > 24) tmp = 24;
					i = si.nPos;
					while (tmp > 0) {
						if ((tmp2=opsize[GetMem(i)]) == 0) tmp2++;
						if ((i+=tmp2) > 0xFFFF) {
							i = 0xFFFF;
							break;
						}
						tmp--;
					}
					SetGGConvFocus(i,GetMem(i));
				}
				break;
			case WM_INITMENUPOPUP:
			case WM_INITMENU:
				break;
			case WM_COMMAND:
				switch(HIWORD(wParam)) {
					case BN_CLICKED:
						switch(LOWORD(wParam)) {
							case IDC_DEBUGGER_RESTORESIZE: 
								RestoreSize(hwndDlg); 
								break;
							case IDC_DEBUGGER_BP_ADD:
								childwnd = 1;
								if (DialogBox(fceu_hInstance,"ADDBP",hwndDlg,AddbpCallB)) AddBreakList();
								childwnd = 0;
								UpdateDebugger();
								break;
							case IDC_DEBUGGER_BP_DEL:
								DeleteBreak(SendDlgItemMessage(hwndDlg,IDC_DEBUGGER_BP_LIST,LB_GETCURSEL,0,0));
								break;
							case IDC_DEBUGGER_BP_EDIT:
								WP_edit = SendDlgItemMessage(hwndDlg,IDC_DEBUGGER_BP_LIST,LB_GETCURSEL,0,0);
								if (DialogBox(fceu_hInstance,"ADDBP",hwndDlg,AddbpCallB)) EditBreakList();
								WP_edit = -1;
								UpdateDebugger();
								break;
							case IDC_DEBUGGER_RUN:
								//mbg merge 7/18/06 changed pausing check and set
								if (FCEUI_EmulationPaused()) {
									UpdateRegs(hwndDlg);
									FCEUI_ToggleEmulationPause(); 
								}
								break;
							case IDC_DEBUGGER_STEP_IN:
								//mbg merge 7/18/06 changed pausing check and set
								if (FCEUI_EmulationPaused()) {
									UpdateRegs(hwndDlg);
								}
								FCEUI_Debugger().step = true;
								FCEUI_SetEmulationPaused(0);
								UpdateDebugger();
								UpdateLogWindow();
								break;
							case IDC_DEBUGGER_RUN_LINE:
								if (FCEUI_EmulationPaused()) {
									UpdateRegs(hwndDlg);
								}
								FCEUI_Debugger().runline = true;
								{
									uint64 ts=timestampbase;
									ts+=timestamp;
									ts+=341/3;
									//if (scanline == 240) vblankScanLines++;
									//else vblankScanLines = 0;
									FCEUI_Debugger().runline_end_time=ts;
								}
								FCEUI_SetEmulationPaused(0);
								UpdateDebugger();
								UpdateLogWindow();
								break;
							case IDC_DEBUGGER_RUN_FRAME2:
								if (FCEUI_EmulationPaused()) {
									UpdateRegs(hwndDlg);
								}
								FCEUI_Debugger().runline = true;
								{
									uint64 ts=timestampbase;
									ts+=timestamp;
									ts+=128*341/3;
									FCEUI_Debugger().runline_end_time=ts;
									//if (scanline+128 >= 240 && scanline+128 <= 257) vblankScanLines = (scanline+128)-240;
									//else vblankScanLines = 0;
								}
								FCEUI_SetEmulationPaused(0);
								UpdateDebugger();
								UpdateLogWindow();
								break;
							case IDC_DEBUGGER_STEP_OUT:
								//mbg merge 7/18/06 changed pausing check and set
								if (FCEUI_EmulationPaused() > 0) {
									DebuggerState &dbgstate = FCEUI_Debugger();
									UpdateRegs(hwndDlg);
									if ((dbgstate.stepout) && (MessageBox(hwndDlg,"Step Out is currently in process. Cancel it and setup a new Step Out watch?","Step Out Already Active",MB_YESNO|MB_ICONINFORMATION) != IDYES)) break;
									if (GetMem(X.PC) == 0x20) dbgstate.jsrcount = 1;
									else dbgstate.jsrcount = 0;
									dbgstate.stepout = 1;
									FCEUI_SetEmulationPaused(0);
									//UpdateDebugger();
								}
								break;
							case IDC_DEBUGGER_STEP_OVER:
								//mbg merge 7/18/06 changed pausing check and set
								if (FCEUI_EmulationPaused()) {
									UpdateRegs(hwndDlg);
									if (GetMem(tmp=X.PC) == 0x20) {
										if ((watchpoint[64].flags) && (MessageBox(hwndDlg,"Step Over is currently in process. Cancel it and setup a new Step Over watch?","Step Over Already Active",MB_YESNO|MB_ICONINFORMATION) != IDYES)) break;
										watchpoint[64].address = (tmp+3);
										watchpoint[64].flags = WP_E|WP_X;
									}
									else FCEUI_Debugger().step = true;
									FCEUI_SetEmulationPaused(0);
								UpdateLogWindow();
								}
								break;
							case IDC_DEBUGGER_SEEK_PC:
								//mbg merge 7/18/06 changed pausing check
								if (FCEUI_EmulationPaused()) {
									UpdateRegs(hwndDlg);
									UpdateDebugger();
								}
								break;
							case IDC_DEBUGGER_SEEK_TO:
								//mbg merge 7/18/06 changed pausing check
								if (FCEUI_EmulationPaused()) UpdateRegs(hwndDlg);
								GetDlgItemText(hwndDlg,IDC_DEBUGGER_VAL_PCSEEK,str,5);
								if (((ret = sscanf(str,"%4X",&tmp)) == EOF) || (ret != 1)) tmp = 0;
								sprintf(str,"%04X",tmp);
								SetDlgItemText(hwndDlg,IDC_DEBUGGER_VAL_PCSEEK,str);
								Disassemble(hDebug, IDC_DEBUGGER_DISASSEMBLY, IDC_DEBUGGER_DISASSEMBLY_VSCR, tmp);
								break;

							case IDC_DEBUGGER_BREAK_ON_BAD_OP: //Break on bad opcode
								FCEUI_Debugger().badopbreak ^=1;
								break;
							case IDC_DEBUGGER_FLAG_N: X.P^=N_FLAG; UpdateDebugger(); break;
							case IDC_DEBUGGER_FLAG_V: X.P^=V_FLAG; UpdateDebugger(); break;
							case IDC_DEBUGGER_FLAG_U: X.P^=U_FLAG; UpdateDebugger(); break;
							case IDC_DEBUGGER_FLAG_B: X.P^=B_FLAG; UpdateDebugger(); break;
							case IDC_DEBUGGER_FLAG_D: X.P^=D_FLAG; UpdateDebugger(); break;
							case IDC_DEBUGGER_FLAG_I: X.P^=I_FLAG; UpdateDebugger(); break;
							case IDC_DEBUGGER_FLAG_Z: X.P^=Z_FLAG; UpdateDebugger(); break;
							case IDC_DEBUGGER_FLAG_C: X.P^=C_FLAG; UpdateDebugger(); break;
// ################################## Start of SP CODE ###########################

							case IDC_DEBUGGER_RELOAD_SYMS: lastBank = loadedBank = -1; loadNameFiles(); UpdateDebugger(); break;
							case IDC_DEBUGGER_BOOKMARK_ADD: AddDebuggerBookmark(hwndDlg); break;
							case IDC_DEBUGGER_BOOKMARK_DEL: DeleteDebuggerBookmark(hwndDlg); break;
							case IDC_DEBUGGER_ENABLE_SYMBOLIC: UpdateDebugger(); break;
							
// ################################## End of SP CODE ###########################
							
							case IDC_DEBUGGER_ROM_PATCHER: DoPatcher(-1,hwndDlg); break;
						}
						//UpdateDebugger();
						break;
					case LBN_DBLCLK:
						switch(LOWORD(wParam)) {
							case IDC_DEBUGGER_BP_LIST: EnableBreak(SendDlgItemMessage(hwndDlg,IDC_DEBUGGER_BP_LIST,LB_GETCURSEL,0,0)); break;
// ################################## Start of SP CODE ###########################

							case LIST_DEBUGGER_BOOKMARKS: GoToDebuggerBookmark(hwndDlg); break;
							
// ################################## End of SP CODE ###########################
						}
						break;
					case LBN_SELCANCEL:
						switch(LOWORD(wParam)) {
							case IDC_DEBUGGER_BP_LIST:
								EnableWindow(GetDlgItem(hwndDlg,IDC_DEBUGGER_BP_DEL),FALSE);
								EnableWindow(GetDlgItem(hwndDlg,IDC_DEBUGGER_BP_EDIT),FALSE);
								break;
						}
						break;
					case LBN_SELCHANGE:
						switch(LOWORD(wParam)) {
							case IDC_DEBUGGER_BP_LIST:
								EnableWindow(GetDlgItem(hwndDlg,IDC_DEBUGGER_BP_DEL),TRUE);
								EnableWindow(GetDlgItem(hwndDlg,IDC_DEBUGGER_BP_EDIT),TRUE);
								break;
						}
						break;
				}
				break;
		}
	}


	return FALSE; //TRUE;
}

extern void iNESGI(GI h);

void DoPatcher(int address,HWND hParent){
	iapoffset=address;
	if(GameInterface==iNESGI)DialogBox(fceu_hInstance,"ROMPATCHER",hParent,PatcherCallB);
	else MessageBox(hDebug, "Sorry, The Patcher only works on INES rom images", "Error", MB_OK);
	UpdateDebugger();
}

void UpdatePatcher(HWND hwndDlg){
	char str[75]; //mbg merge 7/18/06 changed from unsigned
	uint8 *p;
	if(iapoffset != -1){
		EnableWindow(GetDlgItem(hwndDlg,IDC_ROMPATCHER_PATCH_DATA),TRUE);
		EnableWindow(GetDlgItem(hwndDlg,IDC_ROMPATCHER_BTN_APPLY),TRUE);

		if(GetRomAddress(iapoffset) != -1)sprintf(str,"Current Data at NES ROM Address: %04X, .NES file Address: %04X",GetRomAddress(iapoffset),iapoffset);
		else sprintf(str,"Current Data at .NES file Address: %04X",iapoffset);

		SetDlgItemText(hwndDlg,IDC_ROMPATCHER_CURRENT_DATA_BOX,str);

		sprintf(str,"%04X",GetRomAddress(iapoffset));
		SetDlgItemText(hwndDlg,IDC_ROMPATCHER_DISASSEMBLY,str);

		if(GetRomAddress(iapoffset) != -1)SetDlgItemText(hwndDlg,IDC_ROMPATCHER_DISASSEMBLY,DisassembleLine(GetRomAddress(iapoffset)));
		else SetDlgItemText(hwndDlg,IDC_ROMPATCHER_DISASSEMBLY,"Not Currently Loaded in ROM for disassembly");

		p = GetNesPRGPointer(iapoffset-16);
		sprintf(str,"%02X %02X %02X %02X %02X %02X %02X %02X",
			p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
		SetDlgItemText(hwndDlg,IDC_ROMPATCHER_CURRENT_DATA,str);

	} else {
		SetDlgItemText(hwndDlg,IDC_ROMPATCHER_CURRENT_DATA_BOX,"No Offset Selected");
		SetDlgItemText(hwndDlg,IDC_ROMPATCHER_CURRENT_DATA,"");
		SetDlgItemText(hwndDlg,IDC_ROMPATCHER_DISASSEMBLY,"");
		EnableWindow(GetDlgItem(hwndDlg,IDC_ROMPATCHER_PATCH_DATA),FALSE);
		EnableWindow(GetDlgItem(hwndDlg,IDC_ROMPATCHER_BTN_APPLY),FALSE);
	}
	if(GameInfo->type != GIT_CART)EnableWindow(GetDlgItem(hwndDlg,IDC_ROMPATCHER_BTN_SAVE),FALSE);
	else EnableWindow(GetDlgItem(hwndDlg,IDC_ROMPATCHER_BTN_SAVE),TRUE);
}

void DoDebug(uint8 halt) {
	if (!debugger_open) {
		hDebug = CreateDialog(fceu_hInstance,"DEBUGGER",NULL,DebuggerCallB);
		if(DbgSizeX != -1 && DbgSizeY != -1)
			SetWindowPos(hDebug,0,0,0,DbgSizeX,DbgSizeY,SWP_NOMOVE|SWP_NOZORDER|SWP_NOOWNERZORDER);
	}
	if (hDebug) {
		SetWindowPos(hDebug,HWND_TOP,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);
		if (GameInfo) UpdateDebugger();
	}
}

//-----------------------------------------
DebugSystem* debugSystem;

DebugSystem::DebugSystem()
{
	hFixedFont = CreateFont(13,8, /*Height,Width*/
		0,0, /*escapement,orientation*/
		400,FALSE,FALSE,FALSE, /*weight, italic,, underline, strikeout*/
		ANSI_CHARSET,OUT_DEVICE_PRECIS,CLIP_MASK, /*charset, precision, clipping*/
		DEFAULT_QUALITY, DEFAULT_PITCH, /*quality, and pitch*/
		"Courier"); /*font name*/
}

DebugSystem::~DebugSystem()
{
	DeleteObject(hFixedFont);
}

