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
#include "../../fceu.h"
#include "../../cart.h" //mbg merge 7/18/06 moved beneath fceu.h
#include "../../x6502.h"
#include "../../debug.h"
#include "debugger.h"
#include "tracer.h"
#include "cdlogger.h"
#include "main.h" //for GetRomName()

#define INESPRIV
#include "../../ines.h"

#include "../../nsf.h"

using namespace std;

void LoadCDLogFile();
void SaveCDLogFileAs();
void SaveCDLogFile();
void SaveStrippedRom();

extern iNES_HEADER head; //defined in ines.c
extern uint8 *trainerpoo;

int CDLogger_wndx=0, CDLogger_wndy=0;

//extern uint8 *ROM;
//extern uint8 *VROM;

extern uint8 *NSFDATA;
extern int NSFMaxBank;
static uint8 NSFLoadLow;
static uint8 NSFLoadHigh;

//volatile int loggingcodedata;
//int cdlogger_open;

HWND hCDLogger;
char loadedcdfile[1024];

//Prototypes
void LoadCDLog (const char* nameo);

BOOL CALLBACK CDLoggerCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch(uMsg) {
		case WM_DROPFILES:
			{
				UINT len;
				char *ftmp;

				len=DragQueryFile((HDROP)wParam,0,0,0)+1; 
				if((ftmp=(char*)malloc(len))) 
				{
					DragQueryFile((HDROP)wParam,0,ftmp,len); 
					string fileDropped = ftmp;
					//adelikat:  Drag and Drop only checks file extension, the internal functions are responsible for file error checking
					//-------------------------------------------------------
					//Check if .tbl
					//-------------------------------------------------------
					if (!(fileDropped.find(".cdl") == string::npos) && (fileDropped.find(".cdl") == fileDropped.length()-4))
					{
						LoadCDLog(fileDropped.c_str());
					}
					else
					{
						std::string str = "Could not open " + fileDropped;
						MessageBox(hwndDlg, str.c_str(), "File error", 0);
					}
				}            
			}

			break;

		case WM_MOVE: {
			RECT wrect;
			GetWindowRect(hwndDlg,&wrect);
			CDLogger_wndx = wrect.left;
			CDLogger_wndy = wrect.top;
			break;
		};
		case WM_INITDIALOG:
			if (CDLogger_wndx==-32000) CDLogger_wndx=0; //Just in case
			if (CDLogger_wndy==-32000) CDLogger_wndy=0;
			SetWindowPos(hwndDlg,0,CDLogger_wndx,CDLogger_wndy,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER);
			hCDLogger = hwndDlg;
			codecount = datacount = 0;
			undefinedcount = PRGsize[0];
			cdloggerdata = (unsigned char*)malloc(PRGsize[0]); //mbg merge 7/18/06 added cast
			ZeroMemory(cdloggerdata,PRGsize[0]);
			break;
		case WM_CLOSE:
		case WM_QUIT:
								if((logging) && (logging_options & LOG_NEW_INSTRUCTIONS)){
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
						case BTN_CDLOGGER_RESET:
							codecount = datacount = 0;
							undefinedcount = PRGsize[0];
							ZeroMemory(cdloggerdata,PRGsize[0]);
							UpdateCDLogger();
							break;
						case BTN_CDLOGGER_LOAD:
							LoadCDLogFile();
							break;
						case BTN_CDLOGGER_START_PAUSE:
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
								SetDlgItemText(hCDLogger, BTN_CDLOGGER_START_PAUSE, "Start");
							}
							else{
								FCEUI_SetLoggingCD(1);
								EnableTracerMenuItems();
								SetDlgItemText(hCDLogger, BTN_CDLOGGER_START_PAUSE, "Pause");
							}
							break;
						case BTN_CDLOGGER_SAVE_AS:
							SaveCDLogFileAs();
							break;
						case BTN_CDLOGGER_SAVE:
							SaveCDLogFile();
							break;
						case BTN_CDLOGGER_SAVE_STRIPPED:
							SaveStrippedRom();
							break;
					}
					break;
			}
			break;
		case WM_MOVING:
			break;
	}
	return FALSE;
}

void LoadCDLog (const char* nameo)
{
	FILE *FP;
	int i,j;

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

void LoadCDLogFile(){
	const char filter[]="Code Data Log File(*.CDL)\0*.cdl\0";
	char nameo[2048];
	OPENFILENAME ofn;
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
	LoadCDLog(nameo);
}

void SaveCDLogFileAs(){
	const char filter[]="Code Data Log File(*.CDL)\0*.cdl\0";
	char nameo[2048]; 
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Save Code Data Log File As...";
	ofn.lpstrFilter=filter;
	strcpy(nameo,GetRomName());
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
	if (!GameInfo) {
		FCEUD_PrintError("You must have a game loaded before you can use the Code Data Logger.");
		return;
	}
	//if (GameInfo->type==GIT_NSF) { //todo: NSF support!
	//	FCEUD_PrintError("Sorry, you can't yet use the Code Data Logger with NSFs.");
	//	return;
	//}

	if(!hCDLogger){
		CreateDialog(fceu_hInstance,"CDLOGGER",NULL,CDLoggerCallB);
	}	else {
		SetWindowPos(hCDLogger,HWND_TOP,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);
	}
}

void UpdateCDLogger(){
	if(!hCDLogger)return;
	
	char str[50];
	float fcodecount = codecount, fdatacount = datacount,
		fundefinedcount = undefinedcount, fromsize = PRGsize[0];
	
	sprintf(str,"0x%06x %.2f%%",codecount,fcodecount/fromsize*100);
	SetDlgItemText(hCDLogger,LBL_CDLOGGER_CODECOUNT,str);
	sprintf(str,"0x%06x %.2f%%",datacount,fdatacount/fromsize*100);
	SetDlgItemText(hCDLogger,LBL_CDLOGGER_DATACOUNT,str);
	sprintf(str,"0x%06x %.2f%%",undefinedcount,fundefinedcount/fromsize*100);
	SetDlgItemText(hCDLogger,LBL_CDLOGGER_UNDEFCOUNT,str);
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
	const char NESfilter[]="Stripped iNes Rom file(*.NES)\0*.nes\0";
	const char NSFfilter[]="Stripped NSF file(*.NSF)\0*.nsf\0";
	char sromfilename[MAX_PATH];
	FILE *fp;
	OPENFILENAME ofn;
	int i;

	if (GameInfo->type==GIT_NSF) {
		MessageBox(NULL, "Sorry, you're not allowed to save optimized NSFs yet. Please don't optimize individual banks, as there are still some issues with several NSFs to be fixed, and it is easier to fix those issues with as much of the bank data intact as possible.", "Disallowed", MB_OK);
		return;
	}

	if(codecount == 0){
		MessageBox(NULL, "Unable to Generate Stripped Rom. Get Something Logged and try again.", "Error", MB_OK);
		return;
	}
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Save Stripped File As...";
	if (GameInfo->type==GIT_NSF) {
		ofn.lpstrFilter=NSFfilter;
	}
	else {
		ofn.lpstrFilter=NESfilter;
	}
	strcpy(sromfilename,GetRomName());
	ofn.lpstrFile=sromfilename;
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
	ofn.hwndOwner = hCDLogger;
	if(!GetSaveFileName(&ofn))return;

	fp = fopen(sromfilename,"wb");

if(GameInfo->type==GIT_NSF)
{
    //Not used because if bankswitching, the addresses involved
    //could still end up being used through writes
    //static uint16 LoadAddr;
    //LoadAddr=NSFHeader.LoadAddressLow;
    //LoadAddr|=(NSFHeader.LoadAddressHigh&0x7F)<<8;
    
    //Simple store/restore for writing a working NSF header
    NSFLoadLow = NSFHeader.LoadAddressLow;
    NSFLoadHigh = NSFHeader.LoadAddressHigh;
    NSFHeader.LoadAddressLow=0;
    NSFHeader.LoadAddressHigh&=0xF0;
    fwrite(&NSFHeader,1,0x8,fp);
    NSFHeader.LoadAddressLow = NSFLoadLow;
    NSFHeader.LoadAddressHigh = NSFLoadHigh;
    
	fseek(fp,0x8,SEEK_SET);
	for(i = 0;i < ((NSFMaxBank+1)*4096);i++){
		if(cdloggerdata[i] & 3)fputc(NSFDATA[i],fp);
		else fputc(0,fp);
	}

}
else
{
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
}
	fclose(fp);
}

