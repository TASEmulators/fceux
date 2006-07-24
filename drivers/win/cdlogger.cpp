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
#include "..\..\fceu.h"
#include "..\..\cart.h" //mbg merge 7/18/06 moved beneath fceu.h
#include "..\..\x6502.h"
#include "..\..\debug.h"
#include "debugger.h"
#include "tracer.h"
#include "cdlogger.h"

#define INESPRIV
#include "..\..\ines.h"

void LoadCDLogFile();
void SaveCDLogFileAs();
void SaveCDLogFile();
void SaveStrippedRom();

extern iNES_HEADER head; //defined in ines.c
extern uint8 *trainerpoo;
//extern uint8 *ROM;
//extern uint8 *VROM;

//volatile int loggingcodedata;
//int cdlogger_open;

HWND hCDLogger;
char loadedcdfile[1024];


BOOL CALLBACK CDLoggerCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_INITDIALOG:
			hCDLogger = hwndDlg;
			codecount = datacount = 0;
			undefinedcount = PRGsize[0];
			cdloggerdata = (unsigned char*)malloc(PRGsize[0]); //mbg merge 7/18/06 added cast
			ZeroMemory(cdloggerdata,PRGsize[0]);
			break;
		case WM_CLOSE:
		case WM_QUIT:
								if((logging) && (logging_options & LOG_NEW_INSTRUCTIONS)){
									StopSound();
									MessageBox(hCDLogger,
"The Trace logger is currently using this for some of its features.\
 Please turn the trace logger off and try again.","Unable to Pause Code/Data Logger",
MB_OK);
									break;
								}
			FCEUI_SetLoggingCD(0);
			free(cdloggerdata);
			cdloggerdata=0;
			hCDLogger = 0;
			EndDialog(hwndDlg,0);
			break;
		case WM_COMMAND:
			switch(HIWORD(wParam)) {
				case BN_CLICKED:
					switch(LOWORD(wParam)) {
						case 103:
							codecount = datacount = 0;
							undefinedcount = PRGsize[0];
							ZeroMemory(cdloggerdata,PRGsize[0]);
							UpdateCDLogger();
							break;
						case 104:
							LoadCDLogFile();
							break;
						case 105:
							if(FCEUI_GetLoggingCD()){
								if((logging) && (logging_options & LOG_NEW_INSTRUCTIONS)){
									MessageBox(hCDLogger,
"The Trace logger is currently using this for some of its features.\
 Please turn the trace logger off and try again.","Unable to Pause Code/Data Logger",
MB_OK);
									break;
								}
								FCEUI_SetLoggingCD(0);
								EnableTracerMenuItems();
								SetDlgItemText(hCDLogger, 105, "Start");
							}
							else{
								FCEUI_SetLoggingCD(1);
								EnableTracerMenuItems();
								SetDlgItemText(hCDLogger, 105, "Pause");
							}
							break;
						case 106:
							SaveCDLogFileAs();
							break;
						case 107:
							SaveCDLogFile();
							break;
						case 108:
							SaveStrippedRom();
							break;
					}
					break;
			}
			break;
		case WM_MOVING:
			StopSound();
			break;
	}
	return FALSE;
}

void LoadCDLogFile(){
	FILE *FP;
	int i, j;
	const char filter[]="Code Data Log File(*.CDL)\0*.cdl\0";
	char nameo[2048]; //todo: possibly no need for this? can lpstrfilter point to loadedcdfile instead?
	OPENFILENAME ofn;
	StopSound();
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Load Code Data Log File...";
	ofn.lpstrFilter=filter;
	nameo[0]=0;
	ofn.lpstrFile=nameo;
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
	ofn.hwndOwner = hCDLogger;
	if(!GetOpenFileName(&ofn))return;
	strcpy(loadedcdfile,nameo);
	//FCEUD_PrintError(loadedcdfile);
	
	//fseek(FP,0,SEEK_SET);
	//codecount = datacount = 0;
	//undefinedcount = PRGsize[0];

	//cdloggerdata = malloc(PRGsize[0]);
	

	FP = fopen(loadedcdfile,"rb");
	if(FP == NULL){
		FCEUD_PrintError("Error Opening File");
		return;
	}

	//codecount = datacount = 0;
	//undefinedcount = PRGsize[0];

	for(i = 0;i < (int)PRGsize[0];i++){
		j = fgetc(FP);
		if(j == EOF)break;
		if((j & 1) && !(cdloggerdata[i] & 1))codecount++; //if the new byte has something logged and
		if((j & 2) && !(cdloggerdata[i] & 2))datacount++; //and the old one doesn't. Then increment
		if((j & 3) && !(cdloggerdata[i] & 3))undefinedcount--; //the appropriate counter.
		cdloggerdata[i] |= j;
	}
	fclose(FP);
	UpdateCDLogger();
	return;
}

void SaveCDLogFileAs(){
	const char filter[]="Code Data Log File(*.CDL)\0*.cdl\0";
	char nameo[2048]; //todo: possibly no need for this? can lpstrfilter point to loadedcdfile instead?
	OPENFILENAME ofn;
	StopSound();
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Save Code Data Log File As...";
	ofn.lpstrFilter=filter;
	nameo[0]=0;
	ofn.lpstrFile=nameo;
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
	ofn.hwndOwner = hCDLogger;
	if(!GetSaveFileName(&ofn))return;
	strcpy(loadedcdfile,nameo);
	SaveCDLogFile();
	return;
}

void SaveCDLogFile(){ //todo make this button work before you've saved as
	FILE *FP;
	//if(loadedcdfile[0] == 0)SaveCDLogFileAs();
	//FCEUD_PrintError(loadedcdfile);

	FP = fopen(loadedcdfile,"wb");
	if(FP == NULL){
		FCEUD_PrintError("Error Opening File");
		return;
	}
	fwrite(cdloggerdata,PRGsize[0],1,FP);
	fclose(FP);
	return;
}

void DoCDLogger(){
	if (!GI) {
		FCEUD_PrintError("You must have a game loaded before you can use the Code Data Logger.");
		return;
	}
	if (GI->type==GIT_NSF) { //todo: NSF support!
		FCEUD_PrintError("Sorry, you can't yet use the Code Data Logger with NSFs.");
		return;
	}

	if(!hCDLogger){
		CreateDialog(fceu_hInstance,"CDLOGGER",NULL,CDLoggerCallB);
	}	else {
		SetWindowPos(hCDLogger,HWND_TOP,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);
	}
}

void UpdateCDLogger(){
	char str[50];
	float fcodecount = codecount, fdatacount = datacount,
		fundefinedcount = undefinedcount, fromsize = PRGsize[0];
	if(!hCDLogger)return;
	sprintf(str,"0x%06x %.2f%%",codecount,fcodecount/fromsize*100);
	SetDlgItemText(hCDLogger,100,str);
	sprintf(str,"0x%06x %.2f%%",datacount,fdatacount/fromsize*100);
	SetDlgItemText(hCDLogger,101,str);
	sprintf(str,"0x%06x %.2f%%",undefinedcount,fundefinedcount/fromsize*100);
	SetDlgItemText(hCDLogger,102,str);
	return;
}

void LogPCM(int romaddress){
	int i = GetPRGAddress(romaddress);
	if(i == -1)return;
	if(cdloggerdata[i] & 0x40)return;
	cdloggerdata[i] |= 0x40;

	if(!(cdloggerdata[i] & 2)){
		datacount++;
		cdloggerdata[i] |= 2;
		if(!(cdloggerdata[i] & 1))undefinedcount--;
	}
	return;
}

void SaveStrippedRom(){ //this is based off of iNesSave()
	//todo: make this support nsfs
	const char filter[]="Stripped iNes Rom file(*.NES)\0*.nes\0";
	char sromfilename[MAX_PATH];
	FILE *fp;
	OPENFILENAME ofn;
	int i;
	if(codecount == 0){
		MessageBox(NULL, "Unable to Generate Stripped Rom. Get Something Logged and try again.", "Error", MB_OK);
		return;
	}
	StopSound();
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Save Stripped Rom File As...";
	ofn.lpstrFilter=filter;
	sromfilename[0]=0;
	ofn.lpstrFile=sromfilename;
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
	ofn.hwndOwner = hCDLogger;
	if(!GetSaveFileName(&ofn))return;

	fp = fopen(sromfilename,"wb");

	if(fwrite(&head,1,16,fp)!=16)return;

	if(head.ROM_type&4) 	/* Trainer */
	{
 	 fwrite(trainerpoo,512,1,fp);
	}

	for(i = 0;i < head.ROM_size*0x4000;i++){
		if(cdloggerdata[i] & 3)fputc(ROM[i],fp);
		else fputc(0,fp);
	}
	//fwrite(ROM,0x4000,head.ROM_size,fp);

	if(head.VROM_size)fwrite(VROM,0x2000,head.VROM_size,fp);
	fclose(fp);
}
