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
#include "debugger.h"
#include "..\..\x6502.h"
#include "..\..\fceu.h"
#include "..\..\cart.h" //mbg merge 7/19/06 moved after fceu.h
#include "cdlogger.h"
#include "..\..\file.h"
#include "..\..\debug.h"
#include "tracer.h"
#include "..\..\memview.h"

//#define LOG_SKIP_UNMAPPED 4
//#define LOG_ADD_PERIODS 8

//int logaxy = 1, logopdata = 1; //deleteme
int logging_options = -1;
int log_update_window = 0;
//int tracer_open=0;
volatile int logtofile = 0, logging = 0;
HWND hTracer;
HFONT hlogFont, hlogNewFont;
int log_optn_intlst[LOG_OPTION_SIZE]  = {10000,8000,6000,4000,2000,1000,700,400,200,100};
char *log_optn_strlst[LOG_OPTION_SIZE] = {"10000","8000","6000","4000","2000","1000","700","400","200","100"};
char *logfilename = 0;
int oldcodecount, olddatacount;

SCROLLINFO tracesi;
char **tracelogbuf;
int tracelogbufsize, tracelogbufpos;
int tracelogbufusedsize;

FILE *LOG_FP;

void ShowLogDirDialog(void);
void BeginLoggingSequence(void);
void LogInstruction(void);
void OutputLogLine(char *str);
void EndLoggingSequence(void);
//void PauseLoggingSequence(void);
void UpdateLogWindow(void);
void UpdateLogText(void);
void EnableTracerMenuItems(void);
int PromptForCDLogger(void);

BOOL CALLBACK TracerCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	int i;
	LOGFONT lf;
	switch(uMsg) {
		case WM_INITDIALOG:
			//CenterWindow(hwndDlg);
			hTracer = hwndDlg;

			//setup font
			hlogFont = (HFONT)SendMessage(hwndDlg, WM_GETFONT, 0, 0);
			GetObject(hlogFont, sizeof(LOGFONT), &lf);
			strcpy(lf.lfFaceName,"Courier");
			hlogNewFont = CreateFontIndirect(&lf);
			SendDlgItemMessage(hwndDlg,100,WM_SETFONT,(WPARAM)hlogNewFont,FALSE);
			
			//check the disabled radio button
			CheckRadioButton(hwndDlg,103,104,103);

			//EnableWindow(GetDlgItem(hwndDlg,101),FALSE);
			//fill in the options for the log size
			for(i = 0;i < LOG_OPTION_SIZE;i++){
				SendDlgItemMessage(hwndDlg,105,CB_INSERTSTRING,-1,(LPARAM)(LPSTR)log_optn_strlst[i]);
			}
			SendDlgItemMessage(hwndDlg,105,CB_SETCURSEL,0,0);
			SetDlgItemText(hwndDlg, 100, "Welcome to the Trace Logger.");
			logtofile = 0;

			if(logging_options == -1){
				logging_options = (LOG_REGISTERS | LOG_PROCESSOR_STATUS);
				CheckDlgButton(hwndDlg, 110, BST_CHECKED);
				CheckDlgButton(hwndDlg, 111, BST_CHECKED);
			} else{
				if(logging_options&LOG_REGISTERS)CheckDlgButton(hwndDlg, 110, BST_CHECKED);
				if(logging_options&LOG_PROCESSOR_STATUS)CheckDlgButton(hwndDlg, 111, BST_CHECKED);				
			}
			EnableWindow(GetDlgItem(hwndDlg,105),TRUE);
			EnableWindow(GetDlgItem(hwndDlg,112),FALSE);

			if(log_update_window)CheckDlgButton(hwndDlg, 116, BST_CHECKED);

			EnableTracerMenuItems();
			break;
		case WM_CLOSE:
		case WM_QUIT:
			if(logging)EndLoggingSequence();
			DeleteObject(hlogNewFont);
			logging_options &= 3;
			hTracer = 0;
			EndDialog(hwndDlg,0);
			break;
		case WM_COMMAND:
			switch(HIWORD(wParam)) {
				case BN_CLICKED:
					switch(LOWORD(wParam)) {
						case 102: //start logging
							if(logging)EndLoggingSequence();
							else BeginLoggingSequence();
							EnableTracerMenuItems();
							break;
						case 103:
							logtofile = 0;
							EnableTracerMenuItems();
							break;
						case 104:
							logtofile = 1;
							EnableTracerMenuItems();
							break;
						case 110:
							logging_options ^= LOG_REGISTERS;
							break;
						case 111:
							logging_options ^= LOG_PROCESSOR_STATUS;
							break; 
						case 114:
							logging_options ^= LOG_NEW_INSTRUCTIONS;
							if(logging && (!PromptForCDLogger())){
								logging_options ^= LOG_NEW_INSTRUCTIONS; //turn it back off
								CheckDlgButton(hTracer, 114, BST_UNCHECKED);
							}
							//EnableTracerMenuItems();
							break;
						case 115:
							logging_options ^= LOG_NEW_DATA;
							if(logging && (!PromptForCDLogger())){
								logging_options ^= LOG_NEW_DATA; //turn it back off
								CheckDlgButton(hTracer, 115, BST_UNCHECKED);
							}
							break;
						case 116:
							//todo: if this gets unchecked then we need to clear out the window
							log_update_window ^= 1;
							if(!FCEUI_EmulationPaused() && !log_update_window) //mbg merge 7/19/06 changed to use EmulationPaused()
								SetDlgItemText(hTracer, 100, "Press F2 to pause the game, or snap \r\nthe debugger to update this window.\r\n");
							//PauseLoggingSequence();
							break;
						case 112:
							ShowLogDirDialog();
							break;
					}
					break;
			}
			break;
		case WM_MOVING:
			StopSound();
			break;
	}


	//if (logging && !logtofile) {
		switch(uMsg) {
			case WM_VSCROLL:
				if (lParam) {
					//if ((!logging) || (logging && logtofile))break;
					//if(!(logging && !logtofile))break;
					if ((!logging) || logtofile) break;

					if(!FCEUI_EmulationPaused() && !log_update_window) break; //mbg merge 7/19/06 changd to use EmulationPaused()

					StopSound();
					GetScrollInfo((HWND)lParam,SB_CTL,&tracesi);
					switch(LOWORD(wParam)) {
						case SB_ENDSCROLL:
						case SB_TOP:
						case SB_BOTTOM: break;
						case SB_LINEUP: tracesi.nPos--; break;
						case SB_LINEDOWN:tracesi.nPos++; break;
						case SB_PAGEUP: tracesi.nPos-=tracesi.nPage; break;
						case SB_PAGEDOWN: tracesi.nPos+=tracesi.nPage; break;
						case SB_THUMBPOSITION: //break;
						case SB_THUMBTRACK: tracesi.nPos = tracesi.nTrackPos; break;
					}
					if(tracesi.nPos < tracesi.nMin) tracesi.nPos = tracesi.nMin;
					if((tracesi.nPos+(int)tracesi.nPage) > tracesi.nMax) tracesi.nPos = tracesi.nMax-(int)tracesi.nPage; //mbg merge 7/19/06 added casts
					if(tracesi.nPos < 0)tracesi.nPos = 0; //this fixes a bad crash that occurs if you scroll up when only a few isntructions were logged
					SetScrollInfo((HWND)lParam,SB_CTL,&tracesi,TRUE);
					UpdateLogText();
				}
				break;
		}
	//}
	return FALSE;
}

void BeginLoggingSequence(void){
	char str[2048], str2[100];
	int i, j;

	if(!PromptForCDLogger())return; //do nothing if user selected no and CD Logger is needed

	if(logtofile){
		LOG_FP = fopen(logfilename,"w");
		if(LOG_FP == NULL){
			sprintf(str,"Error Opening File %s",logfilename);
			MessageBox(hTracer,str, "File Error", MB_OK);
			return;
		}
		fprintf(LOG_FP,FCEU_NAME_AND_VERSION" - Trace Log File\n"); //mbg merge 7/19/06 changed string
	} else {
		strcpy(str,"Allocating Memory...\r\n");
		SetDlgItemText(hTracer, 100, str);
		tracelogbufsize = j = log_optn_intlst[SendDlgItemMessage(hTracer,105,CB_GETCURSEL,0,0)];
		tracelogbuf = (char**)malloc(j*sizeof(char *)); //mbg merge 7/19/06 added cast
		for(i = 0;i < j;i++){
			tracelogbuf[i] = (char*)malloc(80); //mbg merge 7/19/06 added cast
			tracelogbuf[i][0] = 0;
		}
		sprintf(str2,"%d Bytes Allocated...\r\n",j*80);
		strcat(str,str2);
		strcat(str,"Press F2 to pause the game, or snap \r\nthe debugger to update this window.\r\n");
		SetDlgItemText(hTracer, 100, str);
		tracelogbufpos = tracelogbufusedsize = 0;
	}
	
	oldcodecount = codecount;
	olddatacount = datacount;

	logging=1;
	SetDlgItemText(hTracer, 102,"Stop Logging");
	return;
}
/*
void LogInstructionOld(){
	char str[96], chr[32];
	int addr=X.PC;
	int size, j;
	uint8 opcode[3];

	sprintf(str, "$%04X:", addr);
	if ((size = opsize[GetMem(addr)]) == 0) {
		sprintf(chr, "%02X        UNDEFINED", GetMem(addr)); addr++;
		strcat(str,chr);
	}
	else {
		if ((addr+size) > 0xFFFF) {
			sprintf(chr, "%02X        OVERFLOW", GetMem(addr));
			strcat(str,chr);
			goto done;
		}
		for (j = 0; j < size; j++) {
			sprintf(chr, "%02X ", opcode[j] = GetMem(addr)); addr++;
			strcat(str,chr);
		}
		while (size < 3) {
			strcat(str,"   "); //pad output to align ASM
			size++;
		}
		strcat(strcat(str," "),BinToASM(addr,opcode));
	}
done:
	strcat(str,"\n");
	if(logtofile){
		fprintf(LOG_FP,str);
	}
}*/

//todo: really speed this up
void FCEUD_TraceInstruction(){
	if(!logging) return;

	char address[7], data[11], disassembly[28], axystate[16], procstatus[12];
	char str[96];
	int addr=X.PC;
	int size, j;
	uint8 opcode[3], tmp;
	static int unloggedlines;

	if(((logging_options & LOG_NEW_INSTRUCTIONS) && (oldcodecount != codecount)) ||
	   ((logging_options & LOG_NEW_DATA) && (olddatacount != datacount))){//something new was logged
		oldcodecount = codecount;
		olddatacount = datacount;
		if(unloggedlines > 0){
			sprintf(str,"(%d lines skipped)",unloggedlines);
			OutputLogLine(str);
			unloggedlines = 0;
		}
	} else {
		if((logging_options & LOG_NEW_INSTRUCTIONS) ||
			(logging_options & LOG_NEW_DATA)){
			if(FCEUI_GetLoggingCD())unloggedlines++;
			return;
			}
	}

	sprintf(address, "$%04X:", addr);
	axystate[0] = str[0] = 0;
	size = opsize[GetMem(addr)];

	if ((addr+size) > 0xFFFF){
		sprintf(data, "%02X        ", GetMem(addr&0xFFFF));
		sprintf(disassembly,"OVERFLOW");
	} else {
	switch(size){
			case 0:
				sprintf(data, "%02X        ", GetMem(addr));
				sprintf(disassembly,"UNDEFINED");
				break;
			case 1:
				opcode[0]=GetMem(addr++);
				sprintf(data, "%02X        ", opcode[0]);
				strcpy(disassembly,BinToASM(addr,opcode));
				break;
			case 2:
				opcode[0]=GetMem(addr++);
				opcode[1]=GetMem(addr++);
				sprintf(data, "%02X %02X     ", opcode[0],opcode[1]);
				strcpy(disassembly,BinToASM(addr,opcode));
				break;
			case 3:
				opcode[0]=GetMem(addr++);
				opcode[1]=GetMem(addr++);
				opcode[2]=GetMem(addr++);
				sprintf(data, "%02X %02X %02X  ", opcode[0],opcode[1],opcode[2]);
				strcpy(disassembly,BinToASM(addr,opcode));
				break;
		}
	}
	//stretch the disassembly string out if we have to output other stuff.
	if(logging_options & (LOG_REGISTERS|LOG_PROCESSOR_STATUS)){
		for(j = strlen(disassembly);j < 27;j++)disassembly[j] = ' ';
		disassembly[27] = 0;
	}

	if(logging_options & LOG_REGISTERS){
		sprintf(axystate,"A:%02X X:%02X Y:%02X",(X.A),(X.X),(X.Y));
	}
	
	if(logging_options & LOG_PROCESSOR_STATUS){
		tmp = X.P^0xFF;
		sprintf(procstatus,"P:%c%c%c%c%c%c%c%c",
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


	strcat(str,address);
	strcat(str,data);
	strcat(str,disassembly);
	if(logging_options & LOG_REGISTERS)strcat(str,axystate);
	if((logging_options & LOG_REGISTERS) && (logging_options & LOG_PROCESSOR_STATUS))strcat(str," ");
	if(logging_options & LOG_PROCESSOR_STATUS)strcat(str,procstatus);

	OutputLogLine(str);

	return;
}

void OutputLogLine(char *str){
	if(logtofile){
		strcat(str,"\n");
		fputs(str,LOG_FP);
		fflush(LOG_FP);
	}else{
		strcat(str,"\r\n");
		if(strlen(str) < 80)strcpy(tracelogbuf[tracelogbufpos],str);
		tracelogbufpos++;
		if(tracelogbufusedsize < tracelogbufsize)tracelogbufusedsize++;
		tracelogbufpos%=tracelogbufsize;
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
		SetDlgItemText(hTracer, 100, "Welcome to the Trace Logger.");
	}
	logging=0;
	SetDlgItemText(hTracer, 102,"Start Logging");

}

//void PauseLoggingSequence(void){
void UpdateLogWindow(void){

	//if((tracelogbufpos == 0) || (tracelogbuf[tracelogbufpos][0]))
	//	tracesi.nMax = tracelogbufsize;
	//else tracesi.nMax = tracelogbufpos;

	//todo: fix this up.
	//if(!log_update_window && !userpause){
	//	SetDlgItemText(hTracer, 100, "Press F2 to pause the game, or snap \r\nthe debugger to update this window.\r\n");
	//	return;
	//}
	
	//
	
	//we don't want to continue if the trace logger isn't logging, or if its logging to  a file.
	if((!logging) || logtofile)return; 

	//if the game isn't paused, and the option to update the log window while running isn't checked, then halt here.
	if(!FCEUI_EmulationPaused() && !log_update_window){ //mbg merge 7/19/06 changd to use EmulationPaused()
		return;
	}

	tracesi.cbSize = sizeof(SCROLLINFO);
	tracesi.fMask = SIF_ALL;
	tracesi.nPage = 21;
	tracesi.nMin = 0;
	tracesi.nMax = tracelogbufusedsize; //todo: try -2
	tracesi.nPos = tracesi.nMax-tracesi.nPage;
	if (tracesi.nPos < tracesi.nMin) tracesi.nPos = tracesi.nMin;
	SetScrollInfo(GetDlgItem(hTracer,101),SB_CTL,&tracesi,TRUE);
	UpdateLogText();

	return;
}

void UpdateLogText(void){
	int i, j;
	char str[2048];
	str[0] = 0;
	/*
	for(i = 21;i > 0;i--){
		j = tracelogbufpos-i-1;
		if((tracelogbufusedsize == tracelogbufsize) && (j < 0))j = tracelogbufsize+j;	
		if(j >= 0)strcat(str,tracelogbuf[j]);
	}
	*/

	for(i = tracesi.nPos;i < min(tracesi.nMax,tracesi.nPos+21);i++){
		j = i;
		if(tracelogbufusedsize == tracelogbufsize){
			j = (tracelogbufpos+i)%tracelogbufsize;
		}
		strcat(str,tracelogbuf[j]);
	}
	SetDlgItemText(hTracer, 100, str);
	sprintf(str,"nPage = %d, nPos = %d, nMax = %d, nMin = %d",tracesi.nPage,tracesi.nPos,tracesi.nMax,tracesi.nMin);
	SetDlgItemText(hTracer, 1002, str);
	return;
}

void EnableTracerMenuItems(void){

	//if(logging_options & LOG_NEW_INSTRUCTIONS){
		//EnableWindow(GetDlgItem(hTracer,115),TRUE);
	//} else {
	//	CheckDlgButton(hTracer, 115, BST_UNCHECKED);
		//EnableWindow(GetDlgItem(hTracer,115),FALSE);
	//}

	if(logging){
		EnableWindow(GetDlgItem(hTracer,103),FALSE);
		EnableWindow(GetDlgItem(hTracer,104),FALSE);
		EnableWindow(GetDlgItem(hTracer,105),FALSE);
		//EnableWindow(GetDlgItem(hTracer,110),FALSE);
		//EnableWindow(GetDlgItem(hTracer,111),FALSE);
		EnableWindow(GetDlgItem(hTracer,112),FALSE);
		//EnableWindow(GetDlgItem(hTracer,114),FALSE);
		//EnableWindow(GetDlgItem(hTracer,115),FALSE);
		return;
	}

	EnableWindow(GetDlgItem(hTracer,103),TRUE);
	EnableWindow(GetDlgItem(hTracer,104),TRUE);
	EnableWindow(GetDlgItem(hTracer,105),TRUE);
	//EnableWindow(GetDlgItem(hTracer,110),TRUE);
	//EnableWindow(GetDlgItem(hTracer,111),TRUE); //uncomment me
	EnableWindow(GetDlgItem(hTracer,112),TRUE);
	EnableWindow(GetDlgItem(hTracer,114),TRUE);

	if(logtofile){
		EnableWindow(GetDlgItem(hTracer,105),FALSE);
		EnableWindow(GetDlgItem(hTracer,112),TRUE);
		CheckDlgButton(hTracer, 116, BST_UNCHECKED);
		log_update_window = 0;
		EnableWindow(GetDlgItem(hTracer,116),FALSE);
	} else{
		EnableWindow(GetDlgItem(hTracer,105),TRUE);
		EnableWindow(GetDlgItem(hTracer,112),FALSE);
		EnableWindow(GetDlgItem(hTracer,116),TRUE);
	}

/*
	if(FCEUI_GetLoggingCD()){
			EnableWindow(GetDlgItem(hTracer,114),TRUE);
		if(logging_options & LOG_NEW_INSTRUCTIONS){
			EnableWindow(GetDlgItem(hTracer,115),TRUE);
		} else EnableWindow(GetDlgItem(hTracer,115),FALSE);
	}
	else{
			EnableWindow(GetDlgItem(hTracer,114),FALSE);
			EnableWindow(GetDlgItem(hTracer,115),FALSE);
			CheckDlgButton(hTracer, 114, BST_UNCHECKED);
			CheckDlgButton(hTracer, 115, BST_UNCHECKED);
			logging_options &= 3;
	}
*/
	return;
}

//this returns 1 if the CD logger is activated when needed, or 0 if the user selected no, not to activate it
int PromptForCDLogger(void){
	if((logging_options & (LOG_NEW_INSTRUCTIONS|LOG_NEW_DATA)) && (!FCEUI_GetLoggingCD())){
		StopSound();
		if(MessageBox(hTracer,"In order for some of the features you have selected to take effect,\
 the Code/Data Logger must also be running.\
 Would you like to Start the Code/Data Logger Now?","Start Code/Data Logger?",
			MB_YESNO) == IDYES){
				DoCDLogger();
				FCEUI_SetLoggingCD(1);
				//EnableTracerMenuItems();
				SetDlgItemText(hCDLogger, 105, "Pause");
				return 1;
			}
		return 0; //user selected no so 0 is returned
	}
	return 1;
}

void ShowLogDirDialog(void){
 const char filter[]="6502 Trace Log File(*.log,*.txt)\0*.log;*.txt\0";
 char nameo[2048];
 OPENFILENAME ofn;
 StopSound();
 memset(&ofn,0,sizeof(ofn));
 ofn.lStructSize=sizeof(ofn);
 ofn.hInstance=fceu_hInstance;
 ofn.lpstrTitle="Log Trace As...";
 ofn.lpstrFilter=filter;
 nameo[0]=0;
 ofn.lpstrFile=nameo;
 ofn.nMaxFile=256;
 ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
 ofn.hwndOwner = hTracer;
 GetSaveFileName(&ofn);
 if(nameo[0]){
	if(logfilename)free(logfilename);
	logfilename = (char*)malloc(strlen(nameo)+1); //mbg merge 7/19/06 added cast
	strcpy(logfilename,nameo);
 }
 return;
}

void DoTracer(){

	if (!GI) {
		FCEUD_PrintError("You must have a game loaded before you can use the Trace Logger.");
		return;
	}
	if (GI->type==GIT_NSF) { //todo: NSF support!
		FCEUD_PrintError("Sorry, you can't yet use the Trace Logger with NSFs.");
		return;
	}

	if(!hTracer){
		CreateDialog(fceu_hInstance,"TRACER",NULL,TracerCallB);
		//hTracer gets set in WM_INITDIALOG
	} else {
		SetWindowPos(hTracer,HWND_TOP,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);
	}
}
