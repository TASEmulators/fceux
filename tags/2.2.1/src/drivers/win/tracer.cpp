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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <algorithm>

#include "common.h"
#include "debugger.h"
#include "../../x6502.h"
#include "../../fceu.h"
#include "../../cart.h" //mbg merge 7/19/06 moved after fceu.h
#include "../../file.h"
#include "../../debug.h"
#include "../../asm.h"
#include "../../version.h"
#include "cdlogger.h"
#include "tracer.h"
#include "memview.h"
#include "main.h" //for GetRomName()

//Used to determine the current hotkey mapping for the pause key in order to display on the dialog
#include "mapinput.h"
#include "input.h"

using namespace std;

//#define LOG_SKIP_UNMAPPED 4
//#define LOG_ADD_PERIODS 8

// ################################## Start of SP CODE ###########################

#include "debuggersp.h"

extern Name* lastBankNames;
extern Name* loadedBankNames;
extern Name* ramBankNames;
extern int lastBank;
extern int loadedBank;
extern int myNumWPs;

// ################################## End of SP CODE ###########################

//int logaxy = 1, logopdata = 1; //deleteme
int logging_options = LOG_REGISTERS | LOG_PROCESSOR_STATUS | LOG_TO_THE_LEFT | LOG_MESSAGES | LOG_BREAKPOINTS | LOG_CODE_TABBING;
int log_update_window = 0;
//int tracer_open=0;
volatile int logtofile = 0, logging = 0;
HWND hTracer;
int log_optn_intlst[LOG_OPTION_SIZE]  = {3000000, 1000000, 300000, 100000, 30000, 10000, 3000, 1000, 300, 100};
char *log_optn_strlst[LOG_OPTION_SIZE] = {"3 000 000", "1 000 000", "300 000", "100 000", "30 000", "10 000", "3000", "1000", "300", "100"};
int log_lines_option = 5;	// 10000 lines by default
char *logfilename = 0;
int oldcodecount, olddatacount;

SCROLLINFO tracesi;
char **tracelogbuf;
int tracelogbufsize, tracelogbufpos;
int tracelogbufusedsize;

char str_axystate[LOG_AXYSTATE_MAX_LEN] = {0}, str_procstatus[LOG_PROCSTATUS_MAX_LEN] = {0};
char str_tabs[LOG_TABS_MASK+1] = {0}, str_address[LOG_ADDRESS_MAX_LEN] = {0}, str_data[LOG_DATA_MAX_LEN] = {0}, str_disassembly[LOG_DISASSEMBLY_MAX_LEN] = {0};
char str_temp[LOG_LINE_MAX_LEN] = {0};
char* tracer_decoration_name;
char* tracer_decoration_comment;
char str_decoration[NL_MAX_MULTILINE_COMMENT_LEN + 2] = {0};
char str_decoration_comment[NL_MAX_MULTILINE_COMMENT_LEN + 2] = {0};

bool log_old_emu_paused = true;		// thanks to this flag the window only updates once after the game is paused
extern bool JustFrameAdvanced;
extern int currFrameCounter;

FILE *LOG_FP;

int Tracer_wndx=0, Tracer_wndy=0;

void ShowLogDirDialog(void);
void BeginLoggingSequence(void);
void EndLoggingSequence(void);
void UpdateLogWindow(void);
void UpdateLogText(void);
void EnableTracerMenuItems(void);
int PromptForCDLogger(void);

BOOL CALLBACK TracerCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int i;
	switch(uMsg)
	{
		case WM_MOVE:
		{
			if (!IsIconic(hwndDlg))
			{
				RECT wrect;
				GetWindowRect(hwndDlg,&wrect);
				Tracer_wndx = wrect.left;
				Tracer_wndy = wrect.top;
				WindowBoundsCheckNoResize(Tracer_wndx,Tracer_wndy,wrect.right);
			}
			break;
		}
		case WM_INITDIALOG:
		{
			if (Tracer_wndx==-32000) Tracer_wndx=0; //Just in case
			if (Tracer_wndy==-32000) Tracer_wndy=0;
			SetWindowPos(hwndDlg,0,Tracer_wndx,Tracer_wndy,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER);
			hTracer = hwndDlg;

			// setup font
			SendDlgItemMessage(hwndDlg, IDC_TRACER_LOG, WM_SETFONT, (WPARAM)debugSystem->hFixedFont, FALSE);

			// calculate tracesi.nPage
			RECT wrect;
			GetClientRect(GetDlgItem(hwndDlg, IDC_TRACER_LOG), &wrect);
			tracesi.nPage = wrect.bottom / debugSystem->fixedFontHeight;

			//check the disabled radio button
			CheckRadioButton(hwndDlg,IDC_RADIO_LOG_LAST,IDC_RADIO_LOG_TO_FILE,IDC_RADIO_LOG_LAST);

			//EnableWindow(GetDlgItem(hwndDlg,IDC_SCRL_TRACER_LOG),FALSE);
			//fill in the options for the log size
			for(i = 0;i < LOG_OPTION_SIZE;i++)
			{
				SendDlgItemMessage(hwndDlg, IDC_TRACER_LOG_SIZE, CB_INSERTSTRING, -1, (LPARAM)(LPSTR)log_optn_strlst[i]);
			}
			SendDlgItemMessage(hwndDlg, IDC_TRACER_LOG_SIZE, CB_SETCURSEL, (WPARAM)log_lines_option, 0);
			SetDlgItemText(hwndDlg, IDC_TRACER_LOG, "Welcome to the Trace Logger.");
			logtofile = 0;

			CheckDlgButton(hwndDlg, IDC_CHECK_LOG_REGISTERS, (logging_options & LOG_REGISTERS) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_CHECK_LOG_PROCESSOR_STATUS, (logging_options & LOG_PROCESSOR_STATUS) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_CHECK_LOG_NEW_INSTRUCTIONS, (logging_options & LOG_NEW_INSTRUCTIONS) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_CHECK_LOG_NEW_DATA, (logging_options & LOG_NEW_DATA) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_CHECK_LOG_STATUSES_TO_THE_LEFT, (logging_options & LOG_TO_THE_LEFT) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_CHECK_LOG_FRAME_NUMBER, (logging_options & LOG_FRAME_NUMBER) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_CHECK_LOG_MESSAGES, (logging_options & LOG_MESSAGES) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_CHECK_LOG_BREAKPOINTS, (logging_options & LOG_BREAKPOINTS) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_CHECK_SYMBOLIC_TRACING, (logging_options & LOG_SYMBOLIC) ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_CHECK_CODE_TABBING, (logging_options & LOG_CODE_TABBING) ? BST_CHECKED : BST_UNCHECKED);
			
			EnableWindow(GetDlgItem(hwndDlg, IDC_TRACER_LOG_SIZE), TRUE);
			EnableWindow(GetDlgItem(hwndDlg, IDC_BTN_LOG_BROWSE), FALSE);
			CheckDlgButton(hwndDlg, IDC_CHECK_LOG_UPDATE_WINDOW, log_update_window ? BST_CHECKED : BST_UNCHECKED);
			EnableTracerMenuItems();
			break;
		}
		case WM_CLOSE:
		case WM_QUIT:
			if(logging)
				EndLoggingSequence();
			hTracer = 0;
			EndDialog(hwndDlg,0);
			break;
		case WM_COMMAND:
			switch(HIWORD(wParam)) {
				case BN_CLICKED:
					switch(LOWORD(wParam)) {
						case IDC_BTN_START_STOP_LOGGING:
							if(logging)EndLoggingSequence();
							else BeginLoggingSequence();
							EnableTracerMenuItems();
							break;
						case IDC_RADIO_LOG_LAST:
							logtofile = 0;
							EnableTracerMenuItems();
							break;
						case IDC_RADIO_LOG_TO_FILE:
							logtofile = 1;
							EnableTracerMenuItems();
							break;
						case IDC_CHECK_LOG_REGISTERS:
							logging_options ^= LOG_REGISTERS;
							CheckDlgButton(hwndDlg, IDC_CHECK_LOG_REGISTERS, (logging_options & LOG_REGISTERS) ? BST_CHECKED : BST_UNCHECKED);
							break;
						case IDC_CHECK_LOG_PROCESSOR_STATUS:
							logging_options ^= LOG_PROCESSOR_STATUS;
							CheckDlgButton(hwndDlg, IDC_CHECK_LOG_PROCESSOR_STATUS, (logging_options & LOG_PROCESSOR_STATUS) ? BST_CHECKED : BST_UNCHECKED);
							break;
						case IDC_CHECK_LOG_STATUSES_TO_THE_LEFT:
							logging_options ^= LOG_TO_THE_LEFT;
							CheckDlgButton(hwndDlg, IDC_CHECK_LOG_STATUSES_TO_THE_LEFT, (logging_options & LOG_TO_THE_LEFT) ? BST_CHECKED : BST_UNCHECKED);
							break;
						case IDC_CHECK_LOG_FRAME_NUMBER:
							logging_options ^= LOG_FRAME_NUMBER;
							CheckDlgButton(hwndDlg, IDC_CHECK_LOG_FRAME_NUMBER, (logging_options & LOG_FRAME_NUMBER) ? BST_CHECKED : BST_UNCHECKED);
							break;
						case IDC_CHECK_LOG_MESSAGES:
							logging_options ^= LOG_MESSAGES;
							CheckDlgButton(hwndDlg, IDC_CHECK_LOG_MESSAGES, (logging_options & LOG_MESSAGES) ? BST_CHECKED : BST_UNCHECKED);
							break;
						case IDC_CHECK_LOG_BREAKPOINTS:
							logging_options ^= LOG_BREAKPOINTS;
							CheckDlgButton(hwndDlg, IDC_CHECK_LOG_BREAKPOINTS, (logging_options & LOG_BREAKPOINTS) ? BST_CHECKED : BST_UNCHECKED);
							break;
						case IDC_CHECK_SYMBOLIC_TRACING:
							logging_options ^= LOG_SYMBOLIC;
							CheckDlgButton(hwndDlg, IDC_CHECK_SYMBOLIC_TRACING, (logging_options & LOG_SYMBOLIC) ? BST_CHECKED : BST_UNCHECKED);
							break;
						case IDC_CHECK_CODE_TABBING:
							logging_options ^= LOG_CODE_TABBING;
							CheckDlgButton(hwndDlg, IDC_CHECK_CODE_TABBING, (logging_options & LOG_CODE_TABBING) ? BST_CHECKED : BST_UNCHECKED);
							break;
						case IDC_CHECK_LOG_NEW_INSTRUCTIONS:
							logging_options ^= LOG_NEW_INSTRUCTIONS;
							if(logging && (!PromptForCDLogger()))
								logging_options &= ~LOG_NEW_INSTRUCTIONS; //turn it back off
							CheckDlgButton(hwndDlg, IDC_CHECK_LOG_NEW_INSTRUCTIONS, (logging_options & LOG_NEW_INSTRUCTIONS) ? BST_CHECKED : BST_UNCHECKED);
							//EnableTracerMenuItems();
							break;
						case IDC_CHECK_LOG_NEW_DATA:
							logging_options ^= LOG_NEW_DATA;
							if(logging && (!PromptForCDLogger()))
								logging_options &= ~LOG_NEW_DATA; //turn it back off
							CheckDlgButton(hwndDlg, IDC_CHECK_LOG_NEW_DATA, (logging_options & LOG_NEW_DATA) ? BST_CHECKED : BST_UNCHECKED);
							break;
						case IDC_CHECK_LOG_UPDATE_WINDOW:
						{
							//todo: if this gets unchecked then we need to clear out the window
							log_update_window ^= 1;
							if(!FCEUI_EmulationPaused() && !log_update_window)
							{
								// Assemble the message to pause the game.  Uses the current hotkey mapping dynamically
								string m1 = "Pause the game (press ";
								string m2 = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_PAUSE]);
								string m3 = " key or snap the Debugger) to update this window.\r\n";
								string pauseMessage = m1 + m2 + m3;
								SetDlgItemText(hTracer, IDC_TRACER_LOG, pauseMessage.c_str());
							}
							break;
						}
						case IDC_BTN_LOG_BROWSE:
							ShowLogDirDialog();
							break;
					}
					break;
			}
			break;
		case WM_MOVING:
			break;
	}


	switch(uMsg) {
		case WM_VSCROLL:
			if (lParam) {
				if ((!logging) || logtofile) break;

				if(!FCEUI_EmulationPaused() && !log_update_window) break; //mbg merge 7/19/06 changd to use EmulationPaused()

				GetScrollInfo((HWND)lParam,SB_CTL,&tracesi);
				switch(LOWORD(wParam))
				{
					case SB_ENDSCROLL:
					case SB_TOP:
					case SB_BOTTOM: break;
					case SB_LINEUP: tracesi.nPos--; break;
					case SB_LINEDOWN:tracesi.nPos++; break;
					case SB_PAGEUP: tracesi.nPos -= tracesi.nPage; break;
					case SB_PAGEDOWN: tracesi.nPos += tracesi.nPage; break;
					case SB_THUMBPOSITION: //break;
					case SB_THUMBTRACK: tracesi.nPos = tracesi.nTrackPos; break;
				}
				if ((tracesi.nPos + (int)tracesi.nPage) > tracesi.nMax)
					tracesi.nPos = tracesi.nMax - (int)tracesi.nPage;
				if (tracesi.nPos < tracesi.nMin)
					tracesi.nPos = tracesi.nMin;
				SetScrollInfo((HWND)lParam,SB_CTL,&tracesi,TRUE);
				UpdateLogText();
			}
			break;
	}
	return FALSE;
}

void BeginLoggingSequence(void)
{
	char str[2048], str2[100];
	int i, j;

	if(!PromptForCDLogger())return; //do nothing if user selected no and CD Logger is needed

	if(logtofile)
	{
		if(logfilename == NULL) ShowLogDirDialog();
		if (!logfilename) return;
		LOG_FP = fopen(logfilename,"w");
		if(LOG_FP == NULL){
			sprintf(str,"Error Opening File %s",logfilename);
			MessageBox(hTracer,str, "File Error", MB_OK);
			return;
		}
		fprintf(LOG_FP,FCEU_NAME_AND_VERSION" - Trace Log File\n"); //mbg merge 7/19/06 changed string
	} else
	{
		log_lines_option = SendDlgItemMessage(hTracer, IDC_TRACER_LOG_SIZE, CB_GETCURSEL, 0, 0);
		if (log_lines_option == CB_ERR)
			log_lines_option = 0;
		strcpy(str,"Allocating Memory...\r\n");
		SetDlgItemText(hTracer, IDC_TRACER_LOG, str);
		tracelogbufsize = j = log_optn_intlst[log_lines_option];
		tracelogbuf = (char**)malloc(j*sizeof(char *)); //mbg merge 7/19/06 added cast
		for(i = 0;i < j;i++)
		{
			tracelogbuf[i] = (char*)malloc(LOG_LINE_MAX_LEN); //mbg merge 7/19/06 added cast
			tracelogbuf[i][0] = 0;
		}
		sprintf(str2, "%d Bytes Allocated...\r\n", j * LOG_LINE_MAX_LEN);
		strcat(str,str2);
		// Assemble the message to pause the game.  Uses the current hotkey mapping dynamically
		string m1 = "Pause the game (press ";
		string m2 = GetKeyComboName(FCEUD_CommandMapping[EMUCMD_PAUSE]);
		string m3 = " key or snap the Debugger) to update this window.\r\n";
		string pauseMessage = m1 + m2 + m3;
		strcat(str, pauseMessage.c_str());
		SetDlgItemText(hTracer, IDC_TRACER_LOG, str);
		tracelogbufpos = tracelogbufusedsize = 0;
	}
	
	oldcodecount = codecount;
	olddatacount = datacount;

	logging=1;
	SetDlgItemText(hTracer, IDC_BTN_START_STOP_LOGGING,"Stop Logging");
	return;
}

//todo: really speed this up
void FCEUD_TraceInstruction(uint8 *opcode, int size)
{
	if (!logging)
		return;

	unsigned int addr = X.PC;
	uint8 tmp;
	static int unloggedlines;

	// if instruction executed from the RAM, skip this, log all instead
	// TODO: loops folding mame-lyke style
	if(GetPRGAddress(addr) != -1) {
		if(((logging_options & LOG_NEW_INSTRUCTIONS) && (oldcodecount != codecount)) ||
		   ((logging_options & LOG_NEW_DATA) && (olddatacount != datacount)))
		{
			//something new was logged
			oldcodecount = codecount;
			olddatacount = datacount;
			if(unloggedlines > 0)
			{
				sprintf(str_temp, "(%d lines skipped)", unloggedlines);
				OutputLogLine(str_temp);
				unloggedlines = 0;
			}
		} else
		{
			if((logging_options & LOG_NEW_INSTRUCTIONS) ||
				(logging_options & LOG_NEW_DATA))
			{
				if(FCEUI_GetLoggingCD())
					unloggedlines++;
				return;
			}
		}
	}

	if ((addr + size) > 0xFFFF)
	{
		sprintf(str_data, "%02X        ", opcode[0]);
		sprintf(str_disassembly, "OVERFLOW");
	} else
	{
		char* a = 0;
		switch (size)
		{
			case 0:
				sprintf(str_data, "%02X        ", opcode[0]);
				sprintf(str_disassembly,"UNDEFINED");
				break;
			case 1:
			{
				sprintf(str_data, "%02X        ", opcode[0]);
				a = Disassemble(addr + 1, opcode);
				// special case: an RTS opcode
				if (opcode[0] == 0x60)
				{
					// add the beginning address of the subroutine that we exit from
					unsigned int caller_addr = GetMem(((X.S) + 1)|0x0100) + (GetMem(((X.S) + 2)|0x0100) << 8) - 0x2;
					unsigned int call_addr = GetMem(caller_addr + 1) + (GetMem(caller_addr + 2) << 8);
					sprintf(str_decoration, " (from $%04X)", call_addr);
					strcat(a, str_decoration);
				}
				break;
			}
			case 2:
				sprintf(str_data, "%02X %02X     ", opcode[0],opcode[1]);
				a = Disassemble(addr + 2, opcode);
				break;
			case 3:
				sprintf(str_data, "%02X %02X %02X  ", opcode[0],opcode[1],opcode[2]);
				a = Disassemble(addr + 3, opcode);
				break;
		}

		if (a)
		{
			if (logging_options & LOG_SYMBOLIC)
			{
				// Insert Name and Comment lines if needed
				tracer_decoration_name = 0;
				tracer_decoration_comment = 0;
				decorateAddress(addr, &tracer_decoration_name, &tracer_decoration_comment);
				if (tracer_decoration_name)
				{
					strcpy(str_decoration, tracer_decoration_name);
					strcat(str_decoration, ": ");
					OutputLogLine(str_decoration, true);
				}
				if (tracer_decoration_comment)
				{
					// make a copy
					strcpy(str_decoration_comment, tracer_decoration_comment);
					strcat(str_decoration_comment, "\r\n");
					tracer_decoration_comment = str_decoration_comment;
					// divide the str_decoration_comment into strings (Comment1, Comment2, ...)
					char* end_pos = strstr(tracer_decoration_comment, "\r");
					while (end_pos)
					{
						end_pos[0] = 0;		// set \0 instead of \r
						strcpy(str_decoration, "; ");
						strcat(str_decoration, tracer_decoration_comment);
						OutputLogLine(str_decoration, true);
						end_pos += 2;
						tracer_decoration_comment = end_pos;
						end_pos = strstr(tracer_decoration_comment, "\r");
					}
				}
				replaceNames(ramBankNames, a);
				replaceNames(loadedBankNames, a);
				replaceNames(lastBankNames, a);
			}
			strcpy(str_disassembly, a);
		}
	}

	if (size == 1 && GetMem(addr) == 0x60)
	{
		// special case: an RTS opcode
		// add "----------" to emphasize the end of subroutine
		strcat(str_disassembly, " ");
		int i = strlen(str_disassembly);
		for (; i < (LOG_DISASSEMBLY_MAX_LEN - 2); ++i)
			str_disassembly[i] = '-';
		str_disassembly[i] = 0;
	}
	// stretch the disassembly string out if we have to output other stuff.
	if ((logging_options & (LOG_REGISTERS|LOG_PROCESSOR_STATUS)) && !(logging_options & LOG_TO_THE_LEFT))
	{
		for (int i = strlen(str_disassembly); i < (LOG_DISASSEMBLY_MAX_LEN - 1); ++i)
			str_disassembly[i] = ' ';
		str_disassembly[LOG_DISASSEMBLY_MAX_LEN - 1] = 0;
	}

	// Start filling the str_temp line: Frame number, AXYS state, Processor status, Tabs, Address, Data, Disassembly
	if (logging_options & LOG_FRAME_NUMBER)
	{
		sprintf(str_temp, "%06u: ", currFrameCounter);
	} else
	{
		str_temp[0] = 0;
	}

	if (logging_options & LOG_REGISTERS)
	{
		sprintf(str_axystate,"A:%02X X:%02X Y:%02X S:%02X ",(X.A),(X.X),(X.Y),(X.S));
	}
	
	if (logging_options & LOG_PROCESSOR_STATUS)
	{
		tmp = X.P^0xFF;
		sprintf(str_procstatus,"P:%c%c%c%c%c%c%c%c ",
			'N'|(tmp&0x80)>>2,
			'V'|(tmp&0x40)>>1,
			'U'|(tmp&0x20),
			'B'|(tmp&0x10)<<1,
			'D'|(tmp&0x08)<<2,
			'I'|(tmp&0x04)<<3,
			'Z'|(tmp&0x02)<<4,
			'C'|(tmp&0x01)<<5
			);
	}

	if (logging_options & LOG_TO_THE_LEFT)
	{
		if (logging_options & LOG_REGISTERS)
			strcat(str_temp, str_axystate);
		if (logging_options & LOG_PROCESSOR_STATUS)
			strcat(str_temp, str_procstatus);
	}

	if (logging_options & LOG_CODE_TABBING)
	{
		// add spaces at the beginning of the line according to stack pointer
		int spaces = (0xFF - X.S) & LOG_TABS_MASK;
		for (int i = 0; i < spaces; i++)
			str_tabs[i] = ' ';
		str_tabs[spaces] = 0;
		strcat(str_temp, str_tabs);
	} else if (logging_options & LOG_TO_THE_LEFT)
	{
		strcat(str_temp, " ");
	}

	sprintf(str_address, "$%04X:", addr);
	strcat(str_temp, str_address);
	strcat(str_temp, str_data);
	strcat(str_temp, str_disassembly);

	if (!(logging_options & LOG_TO_THE_LEFT))
	{
		if (logging_options & LOG_REGISTERS)
			strcat(str_temp, str_axystate);
		if (logging_options & LOG_PROCESSOR_STATUS)
			strcat(str_temp, str_procstatus);
	}

	OutputLogLine(str_temp);
	return;
}

void OutputLogLine(const char *str, bool add_newline)
{
	if(logtofile)
	{
		fputs(str, LOG_FP);
		if (add_newline)
			fputs("\n", LOG_FP);
		fflush(LOG_FP);
	} else
	{
		if (add_newline)
		{
			strncpy(tracelogbuf[tracelogbufpos], str, LOG_LINE_MAX_LEN - 3);
			tracelogbuf[tracelogbufpos][LOG_LINE_MAX_LEN - 3] = 0;
			strcat(tracelogbuf[tracelogbufpos], "\r\n");
		} else
		{
			strncpy(tracelogbuf[tracelogbufpos], str, LOG_LINE_MAX_LEN - 1);
			tracelogbuf[tracelogbufpos][LOG_LINE_MAX_LEN - 1] = 0;
		}
		tracelogbufpos++;
		if (tracelogbufusedsize < tracelogbufsize)
			tracelogbufusedsize++;
		tracelogbufpos %= tracelogbufsize;
	}
}

void EndLoggingSequence(void){
	int j, i;
	if(logtofile){
		fclose(LOG_FP);
	} else {
		j = tracelogbufsize;
		for(i = 0;i < j;i++){
			free(tracelogbuf[i]);
		}
		free(tracelogbuf);
		SetDlgItemText(hTracer, IDC_TRACER_LOG, "Welcome to the Trace Logger.");
	}
	logging=0;
	SetDlgItemText(hTracer, IDC_BTN_START_STOP_LOGGING,"Start Logging");

}

void UpdateLogWindow(void)
{
	//we don't want to continue if the trace logger isn't logging, or if its logging to a file.
	if ((!logging) || logtofile)
		return; 

	// only update the window when some emulation occured
	// and only update the window when emulator is paused or log_update_window=true
	bool emu_paused = (FCEUI_EmulationPaused() != 0);
	if ((!emu_paused && !log_update_window) || (log_old_emu_paused && !JustFrameAdvanced))	//mbg merge 7/19/06 changd to use EmulationPaused()
	{
		log_old_emu_paused = emu_paused;
		return;
	}
	log_old_emu_paused = emu_paused;

	tracesi.cbSize = sizeof(SCROLLINFO);
	tracesi.fMask = SIF_ALL;
	tracesi.nMin = 0;
	tracesi.nMax = tracelogbufusedsize;
	tracesi.nPos = tracesi.nMax - tracesi.nPage;
	if (tracesi.nPos < tracesi.nMin)
		tracesi.nPos = tracesi.nMin;
	SetScrollInfo(GetDlgItem(hTracer,IDC_SCRL_TRACER_LOG),SB_CTL,&tracesi,TRUE);

	if (logging_options & LOG_SYMBOLIC)
		loadNameFiles();

	UpdateLogText();

	return;
}

void UpdateLogText(void)
{
	int i, j;
	char str[3000];
	str[0] = 0;
	int last_line = tracesi.nPos + tracesi.nPage;
	if (last_line > tracesi.nMax)
		last_line = tracesi.nMax;

	for(i = tracesi.nPos; i < last_line; i++)
	{
		j = i;
		if(tracelogbufusedsize == tracelogbufsize)
		{
			j = (tracelogbufpos+i)%tracelogbufsize;
		}
		strcat(str,tracelogbuf[j]);
	}
	SetDlgItemText(hTracer, IDC_TRACER_LOG, str);
	sprintf(str,"nPage = %d, nPos = %d, nMax = %d, nMin = %d",tracesi.nPage,tracesi.nPos,tracesi.nMax,tracesi.nMin);
	SetDlgItemText(hTracer, IDC_TRACER_STATS, str);
	return;
}

void EnableTracerMenuItems(void){
	if(logging)
	{
		EnableWindow(GetDlgItem(hTracer,IDC_RADIO_LOG_LAST),FALSE);
		EnableWindow(GetDlgItem(hTracer,IDC_RADIO_LOG_TO_FILE),FALSE);
		EnableWindow(GetDlgItem(hTracer,IDC_TRACER_LOG_SIZE),FALSE);
		EnableWindow(GetDlgItem(hTracer,IDC_BTN_LOG_BROWSE),FALSE);
		return;
	}

	EnableWindow(GetDlgItem(hTracer,IDC_RADIO_LOG_LAST),TRUE);
	EnableWindow(GetDlgItem(hTracer,IDC_RADIO_LOG_TO_FILE),TRUE);
	EnableWindow(GetDlgItem(hTracer,IDC_TRACER_LOG_SIZE),TRUE);
	EnableWindow(GetDlgItem(hTracer,IDC_BTN_LOG_BROWSE),TRUE);
	EnableWindow(GetDlgItem(hTracer,IDC_CHECK_LOG_NEW_INSTRUCTIONS),TRUE);

	if(logtofile)
	{
		EnableWindow(GetDlgItem(hTracer,IDC_TRACER_LOG_SIZE),FALSE);
		EnableWindow(GetDlgItem(hTracer,IDC_BTN_LOG_BROWSE),TRUE);
		log_update_window = 0;
		EnableWindow(GetDlgItem(hTracer,IDC_CHECK_LOG_UPDATE_WINDOW),FALSE);
	} else
	{
		EnableWindow(GetDlgItem(hTracer,IDC_TRACER_LOG_SIZE),TRUE);
		EnableWindow(GetDlgItem(hTracer,IDC_BTN_LOG_BROWSE),FALSE);
		EnableWindow(GetDlgItem(hTracer,IDC_CHECK_LOG_UPDATE_WINDOW),TRUE);
	}

	return;
}

//this returns 1 if the CD logger is activated when needed, or 0 if the user selected no, not to activate it
int PromptForCDLogger(void)
{
	if ((logging_options & (LOG_NEW_INSTRUCTIONS|LOG_NEW_DATA)) && (!FCEUI_GetLoggingCD()))
	{
		if (MessageBox(hTracer,"In order for some of the features you have selected to take effect,\
 the Code/Data Logger must also be running.\
 Would you like to Start the Code/Data Logger Now?","Start Code/Data Logger?",
			MB_YESNO) == IDYES)
		{
			if (DoCDLogger())
			{
				FCEUI_SetLoggingCD(1);
				SetDlgItemText(hCDLogger, BTN_CDLOGGER_START_PAUSE, "Pause");
				return 1;
			}
			return 0; // CDLogger couldn't start, probably because the game is closed
		}
		return 0; // user selected no so 0 is returned
	}
	return 1;
}

void ShowLogDirDialog(void){
	const char filter[]="6502 Trace Log File (*.log)\0*.log;*.txt\0" "6502 Trace Log File (*.txt)\0*.log;*.txt\0All Files (*.*)\0*.*\0\0"; //'" "' used to prevent octal conversion on the numbers
	char nameo[2048];
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Log Trace As...";
	ofn.lpstrFilter=filter;
	strcpy(nameo,GetRomName());
	ofn.lpstrFile=nameo;
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
	ofn.hwndOwner = hTracer;
	if(GetSaveFileName(&ofn)){
		if (ofn.nFilterIndex == 1)
			AddExtensionIfMissing(nameo, sizeof(nameo), ".log");
		else if (ofn.nFilterIndex == 2)
			AddExtensionIfMissing(nameo, sizeof(nameo), ".txt");
		if(logfilename)
			free(logfilename);
		logfilename = (char*)malloc(strlen(nameo)+1); //mbg merge 7/19/06 added cast
		strcpy(logfilename,nameo);
	}
	return;
}

void DoTracer()
{
	if (!GameInfo) {
		FCEUD_PrintError("You must have a game loaded before you can use the Trace Logger.");
		return;
	}
	//if (GameInfo->type==GIT_NSF) { //todo: NSF support!
	//	FCEUD_PrintError("Sorry, you can't yet use the Trace Logger with NSFs.");
	//	return;
	//}

	if(!hTracer)
	{
		CreateDialog(fceu_hInstance,"TRACER",NULL,TracerCallB);
		//hTracer gets set in WM_INITDIALOG
	} else
	{
		ShowWindow(hTracer, SW_SHOWNORMAL);
		SetForegroundWindow(hTracer);
	}
}
