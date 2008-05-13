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
#include "..\..\utils/xstring.h"
#include "debugger.h"
#include "..\..\x6502.h"
#include "..\..\fceu.h"
#include "..\..\debug.h"
#include "..\..\nsf.h"
#include "..\..\cart.h"
#include "..\..\ines.h"
#include "..\..\asm.h"
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

int childwnd;

extern readfunc ARead[0x10000];
int DbgPosX,DbgPosY;
int WP_edit=-1;
int ChangeWait=0,ChangeWait2=0;
uint8 debugger_open=0;
HWND hDebug;
static HFONT hFont,hNewFont;
static SCROLLINFO si;

#define START_OFFSET_HANDLE 200
#define END_OFFSET_HANDLE 201
#define READ_BREAKPOINT_HANDLE 102
#define WRITE_BREAKPOINT_HANDLE 103
#define EXECUTE_BREAKPOINT_HANDLE 104
#define CPU_BREAKPOINT_HANDLE 105
#define PPU_BREAKPOINT_HANDLE 106
#define SPRITE_BREAKPOINT_HANDLE 107

#define INVALID_START_OFFSET 1
#define INVALID_END_OFFSET 2

#define MAX_NAME_SIZE 200
#define MAX_CONDITION_SIZE 200
#define CONDITION_HANDLE 202
#define NAME_HANDLE 203

unsigned int NewBreakWindows(HWND hwndDlg, unsigned int num, bool enable)
{
	char startOffsetBuffer[5] = {0};
	char endOffsetBuffer[5] = {0};
	unsigned int type = 0;

	GetDlgItemText(hwndDlg, START_OFFSET_HANDLE, startOffsetBuffer, sizeof(startOffsetBuffer));
	GetDlgItemText(hwndDlg, END_OFFSET_HANDLE, endOffsetBuffer, sizeof(endOffsetBuffer));

	if (IsDlgButtonChecked(hwndDlg, CPU_BREAKPOINT_HANDLE))
	{
		type |= CPU_BREAKPOINT;
	}
	else if (IsDlgButtonChecked(hwndDlg, PPU_BREAKPOINT_HANDLE))
	{
		type |= PPU_BREAKPOINT;
	}
	else
	{
		type |= SPRITE_BREAKPOINT;
	}

	if (IsDlgButtonChecked(hwndDlg, READ_BREAKPOINT_HANDLE))
	{
		type |= READ_BREAKPOINT;
	}

	if (IsDlgButtonChecked(hwndDlg, WRITE_BREAKPOINT_HANDLE))
	{
		type |= WRITE_BREAKPOINT;
	}

	if (IsDlgButtonChecked(hwndDlg, EXECUTE_BREAKPOINT_HANDLE))
	{
		type |= EXECUTE_BREAKPOINT;
	}

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
	GetDlgItemText(hwndDlg, NAME_HANDLE, name, MAX_NAME_SIZE);
	
	char condition[MAX_CONDITION_SIZE] = {0};
	GetDlgItemText(hwndDlg, CONDITION_HANDLE, condition, MAX_CONDITION_SIZE);

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

BOOL CALLBACK AddbpCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char str[8]={0};
	int tmp;

	switch(uMsg) {
		case WM_INITDIALOG:
			CenterWindow(hwndDlg);
			SendDlgItemMessage(hwndDlg,200,EM_SETLIMITTEXT,4,0);
			SendDlgItemMessage(hwndDlg,201,EM_SETLIMITTEXT,4,0);
			if (WP_edit >= 0) {
				SetWindowText(hwndDlg,"Edit Breakpoint...");

				sprintf(str,"%04X",watchpoint[WP_edit].address);
				SetDlgItemText(hwndDlg,200,str);
				sprintf(str,"%04X",watchpoint[WP_edit].endaddress);
				if (strcmp(str,"0000") != 0) SetDlgItemText(hwndDlg,201,str);
				if (watchpoint[WP_edit].flags&WP_R) CheckDlgButton(hwndDlg, 102, BST_CHECKED);
				if (watchpoint[WP_edit].flags&WP_W) CheckDlgButton(hwndDlg, 103, BST_CHECKED);
				if (watchpoint[WP_edit].flags&WP_X) CheckDlgButton(hwndDlg, 104, BST_CHECKED);

				if (watchpoint[WP_edit].flags&BT_P) {
					CheckDlgButton(hwndDlg, 106, BST_CHECKED);
					EnableWindow(GetDlgItem(hwndDlg,104),FALSE);
				}
				else if (watchpoint[WP_edit].flags&BT_S) {
					CheckDlgButton(hwndDlg, 107, BST_CHECKED);
					EnableWindow(GetDlgItem(hwndDlg,104),FALSE);
				}
				else CheckDlgButton(hwndDlg, 105, BST_CHECKED);
				
// ################################## Start of SP CODE ###########################

				SendDlgItemMessage(hwndDlg,202,EM_SETLIMITTEXT,200,0);
				SendDlgItemMessage(hwndDlg,203,EM_SETLIMITTEXT,200,0);
				
				if (watchpoint[WP_edit].cond)
				{
					SetDlgItemText(hwndDlg, 202, watchpoint[WP_edit].condText);
				}
				else
				{
					SetDlgItemText(hwndDlg, 202, "");
				}
				
				if (watchpoint[WP_edit].desc)
				{
					SetDlgItemText(hwndDlg, 203, watchpoint[WP_edit].desc);
				}
				else
				{
					SetDlgItemText(hwndDlg, 203, "");
				}
				
// ################################## End of SP CODE ###########################
			}
			else CheckDlgButton(hwndDlg, 105, BST_CHECKED);
			break;
		case WM_CLOSE:
		case WM_QUIT:
			break;
		case WM_COMMAND:
			switch(HIWORD(wParam)) {
				case BN_CLICKED:
					switch(LOWORD(wParam)) {
						case 100:
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
						case 101:
							endaddbrk:
							EndDialog(hwndDlg,0);
							break;
						case 105: //CPU Mem
							EnableWindow(GetDlgItem(hwndDlg,104),TRUE);
							break;
						case 106: //PPU Mem
						case 107: //Sprtie Mem
							EnableWindow(GetDlgItem(hwndDlg,104),FALSE);
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
	int size,i,j;
	uint8 opcode[3];
	
// ################################## Start of SP CODE ###########################

	loadNameFiles();

// ################################## End of SP CODE ###########################

	si.nPos = addr;
	SetScrollInfo(GetDlgItem(hWnd,scrollid),SB_CTL,&si,TRUE);

	for (i = 0; i < 34; i++) {
		if (addr > 0xFFFF) break;
		if(addr >= 0x8000)
			sprintf(chr, "%02X:%04X", getBank(addr), addr);
		else sprintf(chr, "  :%04X", addr);
		
// ################################## Start of SP CODE ###########################

		symbDebugEnabled = IsDlgButtonChecked(hWnd, 208);
		
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
			for (j = 0; j < size; j++) {
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
	GetDlgItemText(hwndDlg,308,str,85);
	sscanf(str,"%2x,%2x,%2x,%2x,\r\n",&tmp);
	return tmp;
}
*/

void UpdateRegs(HWND hwndDlg) {
	X.A = GetEditHex(hwndDlg,304);
	X.X = GetEditHex(hwndDlg,305);
	X.Y = GetEditHex(hwndDlg,306);
	X.PC = GetEditHex(hwndDlg,307);
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
	//dont do anything if the debugger is not visible
	if(!hDebug)
		return;

	char str[256]={0},chr[8];
	int tmp,ret,i;

	Disassemble(hDebug, 300, 301, X.PC);

	sprintf(str, "%02X", X.A);
	SetDlgItemText(hDebug, 304, str);
	sprintf(str, "%02X", X.X);
	SetDlgItemText(hDebug, 305, str);
	sprintf(str, "%02X", X.Y);
	SetDlgItemText(hDebug, 306, str);
	sprintf(str, "%04X", (int)X.PC);
	SetDlgItemText(hDebug, 307, str);

	sprintf(str, "%04X", (int)RefreshAddr);
	SetDlgItemText(hDebug, 310, str);
	sprintf(str, "%02X", PPU[3]);
	SetDlgItemText(hDebug, 311, str);

	sprintf(str, "Scanline: %d", scanline);
	SetDlgItemText(hDebug, 501, str);

	tmp = X.S|0x0100;
	sprintf(str, "Stack $%04X", tmp);
	SetDlgItemText(hDebug, 403, str);
	tmp = ((tmp+1)|0x0100)&0x01FF;
	sprintf(str, "%02X", GetMem(tmp));
	for (i = 1; i < 28; i++) {
		tmp = ((tmp+1)|0x0100)&0x01FF;  //increment and fix pointer to $0100-$01FF range
		if ((i%4) == 0) sprintf(chr, ",\r\n%02X", GetMem(tmp));
		else sprintf(chr, ",%02X", GetMem(tmp));
		strcat(str,chr);
	}
	SetDlgItemText(hDebug, 308, str);

	GetDlgItemText(hDebug,309,str,5);
	if (((ret = sscanf(str,"%4X",&tmp)) == EOF) || (ret != 1)) tmp = 0;
	sprintf(str,"%04X",tmp);
	SetDlgItemText(hDebug,309,str);

	CheckDlgButton(hDebug, 200, BST_UNCHECKED);
	CheckDlgButton(hDebug, 201, BST_UNCHECKED);
	CheckDlgButton(hDebug, 202, BST_UNCHECKED);
	CheckDlgButton(hDebug, 203, BST_UNCHECKED);
	CheckDlgButton(hDebug, 204, BST_UNCHECKED);
	CheckDlgButton(hDebug, 205, BST_UNCHECKED);
	CheckDlgButton(hDebug, 206, BST_UNCHECKED);
	CheckDlgButton(hDebug, 207, BST_UNCHECKED);

	tmp = X.P;
	if (tmp & N_FLAG) CheckDlgButton(hDebug, 200, BST_CHECKED);
	if (tmp & V_FLAG) CheckDlgButton(hDebug, 201, BST_CHECKED);
	if (tmp & U_FLAG) CheckDlgButton(hDebug, 202, BST_CHECKED);
	if (tmp & B_FLAG) CheckDlgButton(hDebug, 203, BST_CHECKED);
	if (tmp & D_FLAG) CheckDlgButton(hDebug, 204, BST_CHECKED);
	if (tmp & I_FLAG) CheckDlgButton(hDebug, 205, BST_CHECKED);
	if (tmp & Z_FLAG) CheckDlgButton(hDebug, 206, BST_CHECKED);
	if (tmp & C_FLAG) CheckDlgButton(hDebug, 207, BST_CHECKED);
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
	
// ################################## Start of SP CODE ###########################
	if (watchpoint[num].desc)
	{
		strcat(str, " - ");
		strcat(str, watchpoint[num].desc);
	}
// ################################## End of SP CODE ###########################

	return str;
}

void AddBreakList() {
	SendDlgItemMessage(hDebug,302,LB_INSERTSTRING,-1,(LPARAM)(LPSTR)BreakToText(numWPs-1));
}

void EditBreakList() {
	if (WP_edit >= 0) {
		SendDlgItemMessage(hDebug,302,LB_DELETESTRING,WP_edit,0);
		SendDlgItemMessage(hDebug,302,LB_INSERTSTRING,WP_edit,(LPARAM)(LPSTR)BreakToText(WP_edit));
		SendDlgItemMessage(hDebug,302,LB_SETCURSEL,WP_edit,0);
	}
}

void FillBreakList(HWND hwndDlg) {
	int i;

	for (i = 0; i < numWPs; i++) {
		SendDlgItemMessage(hwndDlg,302,LB_INSERTSTRING,-1,(LPARAM)(LPSTR)BreakToText(i));
	}
}

void EnableBreak(int sel) {
	watchpoint[sel].flags^=WP_E;
	SendDlgItemMessage(hDebug,302,LB_DELETESTRING,sel,0);
	SendDlgItemMessage(hDebug,302,LB_INSERTSTRING,sel,(LPARAM)(LPSTR)BreakToText(sel));
	SendDlgItemMessage(hDebug,302,LB_SETCURSEL,sel,0);
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
	SendDlgItemMessage(hDebug,302,LB_DELETESTRING,sel,0);
	EnableWindow(GetDlgItem(hDebug,102),FALSE);
	EnableWindow(GetDlgItem(hDebug,103),FALSE);
}

void KillDebugger() {
	SendDlgItemMessage(hDebug,302,LB_RESETCONTENT,0,0);
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
			SendDlgItemMessage(hwndDlg,101,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,102,WM_SETFONT,(WPARAM)hNewFont,FALSE);

			//set limits
			SendDlgItemMessage(hwndDlg,100,CB_LIMITTEXT,20,0);

			SetDlgItemText(hwndDlg,101,DisassembleLine(iaPC));
			SetFocus(GetDlgItem(hwndDlg,100));

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
						case 201: //Apply
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
						case 202: //Save...
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
						case 203: //Undo
							if ((count = SendDlgItemMessage(hwndDlg,102,LB_GETCOUNT,0,0))) {
								SendDlgItemMessage(hwndDlg,102,LB_DELETESTRING,count-1,0);
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
								SetDlgItemText(hwndDlg,101,DisassembleLine(iaPC+count));
							}
							break;
						case 300: //Hidden default button
							count = 0;
							for (i = 0; i < patchlen; i++) count += opsize[patchdata[i][0]];
							GetDlgItemText(hwndDlg,100,str,21);
							if (!Assemble(patchdata[patchlen],(iaPC+count),str)) {
								count = iaPC;
								for (i = 0; i <= patchlen; i++) count += opsize[patchdata[i][0]];
								if (count > 0x10000) { //note: don't use 0xFFFF!
									MessageBox(hwndDlg, "Patch data cannot exceed address 0xFFFF", "Address error", MB_OK);
									break;
								}
								SetDlgItemText(hwndDlg,100,"");
								if (count < 0x10000) SetDlgItemText(hwndDlg,101,DisassembleLine(count));
								else SetDlgItemText(hwndDlg,101,"OVERFLOW");
								dasm = DisassembleData((count-opsize[patchdata[patchlen][0]]),patchdata[patchlen]);
								SendDlgItemMessage(hwndDlg,102,LB_INSERTSTRING,-1,(LPARAM)(LPSTR)dasm);
								AddAsmHistory(hwndDlg,100,dasm+16);
								SetWindowText(hwndDlg, "Inline Assembler");
								patchlen++;
							}
							else { //ERROR!
								SetWindowText(hwndDlg, "Inline Assembler  *Syntax Error*");
								MessageBeep(MB_ICONEXCLAMATION);
							}
							break;
					}
					SetFocus(GetDlgItem(hwndDlg,100)); //set focus to combo box after anything is pressed!
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
			SendDlgItemMessage(hwndDlg,102,EM_SETLIMITTEXT,6,0);
			SendDlgItemMessage(hwndDlg,109,EM_SETLIMITTEXT,30,0);
			UpdatePatcher(hwndDlg);

			if(iapoffset != -1){
				CheckDlgButton(hwndDlg, 101, BST_CHECKED);
				sprintf((char*)str,"%X",iapoffset); //mbg merge 7/18/06 added cast
				SetDlgItemText(hwndDlg,102,str);
			}

			SetFocus(GetDlgItem(hwndDlg,100));
			break;
		case WM_CLOSE:
		case WM_QUIT:
			EndDialog(hwndDlg,0);
			break;
		case WM_COMMAND:
			switch(HIWORD(wParam)) {
				case BN_CLICKED:
					switch(LOWORD(wParam)) {
						case 103: //todo: maybe get rid of this button and cause iapoffset to update every time you change the text
							if(IsDlgButtonChecked(hwndDlg,101) == BST_CHECKED){
								iapoffset = GetEditHex(hwndDlg,102);
							} else iapoffset = GetNesFileAddress(GetEditHex(hwndDlg,102));
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
						case 110:
							p = GetEditHexData(hwndDlg,109);
							i=0;
							c = GetNesPRGPointer(iapoffset-16);
							while(p[i] != -1){
								c[i] = p[i];
								i++;
							}
							UpdatePatcher(hwndDlg);
							break;
						case 111:
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
	DeleteObject(hNewFont);
	DestroyWindow(hDebug);
}

BOOL CALLBACK DebuggerCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	LOGFONT lf;
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

			SetWindowPos(hwndDlg,0,DbgPosX,DbgPosY,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER);

			si.cbSize = sizeof(SCROLLINFO);
			si.fMask = SIF_ALL;
			si.nMin = 0;
			si.nMax = 0x10000;
			si.nPos = 0;
			si.nPage = 20;
			SetScrollInfo(GetDlgItem(hwndDlg,301),SB_CTL,&si,TRUE);

			//setup font
			hFont = (HFONT)SendMessage(hwndDlg, WM_GETFONT, 0, 0);
			GetObject(hFont, sizeof(LOGFONT), &lf);
			strcpy(lf.lfFaceName,"Courier");
			hNewFont = CreateFontIndirect(&lf);

			SendDlgItemMessage(hwndDlg,300,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,304,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,305,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,306,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,307,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,308,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,309,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,310,WM_SETFONT,(WPARAM)hNewFont,FALSE);
			SendDlgItemMessage(hwndDlg,311,WM_SETFONT,(WPARAM)hNewFont,FALSE);

			//text limits
			SendDlgItemMessage(hwndDlg,304,EM_SETLIMITTEXT,2,0);
			SendDlgItemMessage(hwndDlg,305,EM_SETLIMITTEXT,2,0);
			SendDlgItemMessage(hwndDlg,306,EM_SETLIMITTEXT,2,0);
			SendDlgItemMessage(hwndDlg,307,EM_SETLIMITTEXT,4,0);
			SendDlgItemMessage(hwndDlg,308,EM_SETLIMITTEXT,83,0);
			SendDlgItemMessage(hwndDlg,309,EM_SETLIMITTEXT,4,0);
			SendDlgItemMessage(hwndDlg,310,EM_SETLIMITTEXT,4,0);
			SendDlgItemMessage(hwndDlg,311,EM_SETLIMITTEXT,2,0);

			//I'm lazy, disable the controls which I can't mess with right now
			SendDlgItemMessage(hwndDlg,310,EM_SETREADONLY,TRUE,0);
			SendDlgItemMessage(hwndDlg,311,EM_SETREADONLY,TRUE,0);

// ################################## Start of SP CODE ###########################

			SendDlgItemMessage(hwndDlg,312,EM_SETLIMITTEXT,4,0);
					
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
					CheckDlgButton(hwndDlg, 208, BST_CHECKED);
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
		case WM_CLOSE:
		case WM_QUIT:
			exitdebug:
			DebuggerExit();
			break;
		case WM_MOVING:
			break;
		case WM_MOVE:
			GetWindowRect(hwndDlg,&wrect);
			DbgPosX = wrect.left;
			DbgPosY = wrect.top;
			break;
		case WM_COMMAND:
			if ((HIWORD(wParam) == BN_CLICKED) && (LOWORD(wParam) == 100)) goto exitdebug;
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
					Disassemble(hDebug, 300, 301, si.nPos);
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
				Disassemble(hDebug, 300, 301, si.nPos);
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
						SetDlgItemText(hwndDlg,502,str);
					}
					else SetDlgItemText(hwndDlg,502,"");
				}
				else SetDlgItemText(hwndDlg,502,"");
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
							case 101: //Add
								childwnd = 1;
								if (DialogBox(fceu_hInstance,"ADDBP",hwndDlg,AddbpCallB)) AddBreakList();
								childwnd = 0;
								UpdateDebugger();
								break;
							case 102: //Delete
								DeleteBreak(SendDlgItemMessage(hwndDlg,302,LB_GETCURSEL,0,0));
								break;
							case 103: //Edit
								WP_edit = SendDlgItemMessage(hwndDlg,302,LB_GETCURSEL,0,0);
								if (DialogBox(fceu_hInstance,"ADDBP",hwndDlg,AddbpCallB)) EditBreakList();
								WP_edit = -1;
								UpdateDebugger();
								break;
							case 104: //Run
								//mbg merge 7/18/06 changed pausing check and set
								if (FCEUI_EmulationPaused()) {
									UpdateRegs(hwndDlg);
									FCEUI_ToggleEmulationPause(); 
								}
								break;
							case 105: //Step Into
								//mbg merge 7/19/06 also put the whole block inside the if (previously only updateregs was... was it a bug?)
								//mbg merge 7/18/06 changed pausing check and set
								if (FCEUI_EmulationPaused()) {
									UpdateRegs(hwndDlg);
									FCEUI_Debugger().step = true;
									FCEUI_SetEmulationPaused(0);
									UpdateDebugger();
								}
								break;
							case 106: //Step Out
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
							case 107: //Step Over
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
								}
								break;
							case 108: //Seek PC
								//mbg merge 7/18/06 changed pausing check
								if (FCEUI_EmulationPaused()) {
									UpdateRegs(hwndDlg);
									UpdateDebugger();
								}
								break;
							case 109: //Seek To:
								//mbg merge 7/18/06 changed pausing check
								if (FCEUI_EmulationPaused()) UpdateRegs(hwndDlg);
								GetDlgItemText(hwndDlg,309,str,5);
								if (((ret = sscanf(str,"%4X",&tmp)) == EOF) || (ret != 1)) tmp = 0;
								sprintf(str,"%04X",tmp);
								SetDlgItemText(hwndDlg,309,str);
								Disassemble(hDebug, 300, 301, tmp);
								break;

							case 110: //Break on bad opcode
								FCEUI_Debugger().badopbreak ^=1;
								break;
							case 200: X.P^=N_FLAG; UpdateDebugger(); break;
							case 201: X.P^=V_FLAG; UpdateDebugger(); break;
							case 202: X.P^=U_FLAG; UpdateDebugger(); break;
							case 203: X.P^=B_FLAG; UpdateDebugger(); break;
							case 204: X.P^=D_FLAG; UpdateDebugger(); break;
							case 205: X.P^=I_FLAG; UpdateDebugger(); break;
							case 206: X.P^=Z_FLAG; UpdateDebugger(); break;
							case 207: X.P^=C_FLAG; UpdateDebugger(); break;
// ################################## Start of SP CODE ###########################

							case 111: lastBank = loadedBank = -1; loadNameFiles(); UpdateDebugger(); break;
							case 112: AddDebuggerBookmark(hwndDlg); break;
							case 113: DeleteDebuggerBookmark(hwndDlg); break;
							case 208: UpdateDebugger(); break;
							
// ################################## End of SP CODE ###########################

							case 602: DoPatcher(-1,hwndDlg); break;
							//case 603: DoTracer(hwndDlg); break;
						}
						//UpdateDebugger();
						break;
					case LBN_DBLCLK:
						switch(LOWORD(wParam)) {
							case 302: EnableBreak(SendDlgItemMessage(hwndDlg,302,LB_GETCURSEL,0,0)); break;
// ################################## Start of SP CODE ###########################

							case 701: GoToDebuggerBookmark(hwndDlg); break;
							
// ################################## End of SP CODE ###########################
						}
						break;
					case LBN_SELCANCEL:
						switch(LOWORD(wParam)) {
							case 302:
								EnableWindow(GetDlgItem(hwndDlg,102),FALSE);
								EnableWindow(GetDlgItem(hwndDlg,103),FALSE);
								break;
						}
						break;
					case LBN_SELCHANGE:
						switch(LOWORD(wParam)) {
							case 302:
								EnableWindow(GetDlgItem(hwndDlg,102),TRUE);
								EnableWindow(GetDlgItem(hwndDlg,103),TRUE);
								break;
						}
						break;
				}
				break;/*
				default:
				if(
				(uMsg == 312) ||
				(uMsg == 309) ||
				(uMsg == 308) ||
				(uMsg == 307) ||
				(uMsg == 311) ||
				(uMsg == 71) ||
				(uMsg == 310) ||
				(uMsg == 20) ||
				(uMsg == 13) ||
				(uMsg == 133) ||
				(uMsg == 70) ||
				(uMsg == 24) ||
				(uMsg == 296) ||
				(uMsg == 295) ||
				(uMsg == 15) ||
				(uMsg == 272) ||
				(uMsg == 49) ||
				(uMsg == 3)
				)break;*/

					/*
				if(skipdebug)break;
				if(
				(uMsg == 32) ||
				(uMsg == 127))break;
				fp = fopen("debug.txt","a");
				u++;
				sprintf(str,"%d = %06d",u,uMsg);
				skipdebug=1;
				SetWindowText(hDebug,str);
				skipdebug=0;
				fprintf(fp,"%s\n",str);
				fclose(fp);
				break;*/
		}
	}


	return FALSE; //TRUE;
}

extern void iNESGI(int h);

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
		EnableWindow(GetDlgItem(hwndDlg,109),TRUE);
		EnableWindow(GetDlgItem(hwndDlg,110),TRUE);

		if(GetRomAddress(iapoffset) != -1)sprintf(str,"Current Data at NES ROM Address: %04X, .NES file Address: %04X",GetRomAddress(iapoffset),iapoffset);
		else sprintf(str,"Current Data at .NES file Address: %04X",iapoffset);

		SetDlgItemText(hwndDlg,104,str);

		sprintf(str,"%04X",GetRomAddress(iapoffset));
		SetDlgItemText(hwndDlg,107,str);

		if(GetRomAddress(iapoffset) != -1)SetDlgItemText(hwndDlg,107,DisassembleLine(GetRomAddress(iapoffset)));
		else SetDlgItemText(hwndDlg,107,"Not Currently Loaded in ROM for disassembly");

		p = GetNesPRGPointer(iapoffset-16);
		sprintf(str,"%02X %02X %02X %02X %02X %02X %02X %02X",
			p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
		SetDlgItemText(hwndDlg,105,str);

	} else {
		SetDlgItemText(hwndDlg,104,"No Offset Selected");
		SetDlgItemText(hwndDlg,105,"");
		SetDlgItemText(hwndDlg,107,"");
		EnableWindow(GetDlgItem(hwndDlg,109),FALSE);
		EnableWindow(GetDlgItem(hwndDlg,110),FALSE);
	}
	if(GameInfo->type != GIT_CART)EnableWindow(GetDlgItem(hwndDlg,111),FALSE);
	else EnableWindow(GetDlgItem(hwndDlg,111),TRUE);
}

void DoDebug(uint8 halt) {
	if (!debugger_open) hDebug = CreateDialog(fceu_hInstance,"DEBUGGER",NULL,DebuggerCallB);
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

