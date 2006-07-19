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
#include "..\..\types.h"
#include "..\..\memview.h"
#include "..\..\debugger.h"
#include "..\..\cdlogger.h"
#include "..\..\fceu.h"
#include "..\..\cheat.h"
#include "..\..\cart.h"
#include "..\..\ines.h"
#include "cheat.h"
#include "memviewsp.h"
#include <assert.h>
//#include "string.h"

#define MODE_NES_MEMORY   0
#define MODE_NES_PPU      1
#define MODE_NES_FILE     2


// This defines all of our right click popup menus
struct
{
     int   minaddress;  //The minimum address where this popup will appear
     int   maxaddress;  //The maximum address where this popup will appear
	 int   editingmode; //The editing mode which this popup appears in
	 int   id;          //The menu ID for this popup
	 char  *text;    //the text for the menu item (some of these need to be dynamic)
}
popupmenu[] =
{
	{0,0x2000,0,1,"Freeze/Unfreeze This Address"},
	{0x6000,0x7FFF,0,1,"Freeze/Unfreeze This Address"},
	{0,0xFFFF,0,2,"Add Debugger Read Breakpoint"},
	{0,0x3FFF,1,2,"Add Debugger Read Breakpoint"},
	{0,0xFFFF,0,3,"Add Debugger Write Breakpoint"},
	{0,0x3FFF,1,3,"Add Debugger Write Breakpoint"},
	{0,0xFFFF,0,4,"Add Debugger Execute Breakpoint"},
	{0x8000,0xFFFF,0,5,"Go Here In Rom File"},
	{0x8000,0xFFFF,0,6,"Create Game Genie Code At This Address"},
	//{0,0xFFFFFF,2,7,"Create Game Genie Code At This Address"}
// ################################## Start of SP CODE ###########################
	{0, 0xFFFF, 0, 20, "Add / Remove bookmark"},
// ################################## End of SP CODE ###########################
} ;

#define POPUPNUM (sizeof popupmenu / sizeof popupmenu[0])

int LoadTableFile();
void UnloadTableFile();
void InputData(char *input);
int GetMemViewData(int i);
void UpdateCaption();
int UpdateCheatColorCallB(char *name, uint32 a, uint8 v, int compare,int s,int type, void *data); //mbg merge 6/29/06 - added arg
int DeleteCheatCallB(char *name, uint32 a, uint8 v, int compare,int s,int type); //mbg merge 6/29/06 - added arg
// ################################## Start of SP CODE ###########################
void FreezeRam(int address, int mode, int final);
// ################################## End of SP CODE ###########################
int GetHexScreenCoordx(int offset);
int GetHexScreenCoordy(int offset);
int GetAddyFromCoord(int x,int y);
void AutoScrollFromCoord(int x,int y);
LRESULT CALLBACK MemViewCallB(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
BOOL CALLBACK MemFindCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
void FindNext();
void OpenFindDialog();

HWND hMemView, hMemFind;
HDC mDC;
//int tempdummy;
//char dummystr[100];
HFONT hMemFont;
int CurOffset;
int MemFontHeight;
int MemFontWidth;
int ClientHeight;
int NoColors;
int EditingMode;
int EditingText;
int AddyWasText; //used by the GetAddyFromCoord() function.
int TableFileLoaded;

char chartable[256];

//SCROLLINFO memsi;
//HBITMAP HDataBmp;
//HGDIOBJ HDataObj;
HDC HDataDC;
int CursorX=2, CursorY=9;
int CursorStartAddy, CursorEndAddy=-1;
int CursorDragPoint;//, CursorShiftPoint = -1;
//int CursorStartNibble=1, CursorEndNibble; //1 means that only half of the byte is selected
int TempData=-1;
int DataAmount;
int MaxSize;

COLORREF *BGColorList;
COLORREF *TextColorList;
int *OldValues; //this will be used for a speed hack
int OldCurOffset;

int lbuttondown, lbuttondownx, lbuttondowny;
int mousex, mousey;

int FindAsText;
int FindDirectionUp;
char FindTextBox[60];

extern iNES_HEADER head;

//undo structure
struct UNDOSTRUCT {
	int addr;
	int size;
	unsigned char *data;
	UNDOSTRUCT *last; //mbg merge 7/18/06 removed struct qualifier
};

struct UNDOSTRUCT *undo_list=0;

void ApplyPatch(int addr,int size, uint8* data){
	UNDOSTRUCT *tmp=(UNDOSTRUCT*)malloc(sizeof(UNDOSTRUCT)); //mbg merge 7/18/06 removed struct qualifiers and added cast

	int i;

	//while(tmp != 0){tmp=tmp->next;x++;};
	//tmp = malloc(sizeof(struct UNDOSTRUCT));
	//sprintf(str,"%d",x);
	//MessageBox(hMemView,str,"info", MB_OK);
	tmp->addr = addr;
	tmp->size = size;
	tmp->data = (uint8*)malloc(sizeof(uint8)*size);
	tmp->last=undo_list;

	for(i = 0;i < size;i++){
		tmp->data[i] = GetFileData(addr+i);
		WriteFileData(addr+i,data[i]);
	}

	undo_list=tmp;

	//UpdateColorTable();
	return;
}

void UndoLastPatch(){
	struct UNDOSTRUCT *tmp=undo_list;
	int i;
	if(undo_list == 0)return;
	//while(tmp->next != 0){tmp=tmp->next;}; //traverse to the one before the last one

	for(i = 0;i < tmp->size;i++){
		WriteFileData(tmp->addr+i,tmp->data[i]);
	}
	
	undo_list=undo_list->last;
	
	ChangeMemViewFocus(2,tmp->addr, -1); //move to the focus to where we are undoing at.
	
	free(tmp->data);
	free(tmp);
	return;
}

void FlushUndoBuffer(){
	struct UNDOSTRUCT *tmp;
	while(undo_list!= 0){
		tmp=undo_list;
		undo_list=undo_list->last;
		free(tmp->data);
		free(tmp);
	}
	UpdateColorTable();
	return;
}


int GetFileData(int offset){
	if(offset < 16) return *((unsigned char *)&head+offset);
	if(offset < 16+PRGsize[0])return PRGptr[0][offset-16];
	if(offset < 16+PRGsize[0]+CHRsize[0])return CHRptr[0][offset-16-PRGsize[0]];
	return -1;
}

int WriteFileData(int addr,int data){
	if (addr < 16)MessageBox(hMemView,"Sorry", "Go bug bbit if you really want to edit the header.", MB_OK);
	if((addr >= 16) && (addr < PRGsize[0]+16)) *(uint8 *)(GetNesPRGPointer(addr-16)) = data;
	if((addr >= PRGsize[0]+16) && (addr < CHRsize[0]+PRGsize[0]+16)) *(uint8 *)(GetNesCHRPointer(addr-16-PRGsize[0])) = data;

	return 0;
}

int GetRomFileSize(){ //todo: fix or remove this?
	return 0;
}

//should return -1, otherwise returns the line number it had the error on
int LoadTableFile(){
	char str[50];
	FILE *FP;
	int i, line, charcode1, charcode2;

	const char filter[]="Table Files (*.TBL)\0*.tbl\0";
	char nameo[2048]; //todo: possibly no need for this? can lpstrfilter point to loadedcdfile instead?
	OPENFILENAME ofn;
	StopSound();
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Load Table File...";
	ofn.lpstrFilter=filter;
	nameo[0]=0;
	ofn.lpstrFile=nameo;
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
	ofn.hwndOwner = hCDLogger;
	if(!GetOpenFileName(&ofn))return -1;

	for(i = 0;i < 256;i++){
		chartable[i] = 0;
	}

	FP = fopen(nameo,"r");
	line = 0;
	while((fgets(str, 45, FP)) != NULL){/* get one line from the file */
		line++;

		if(strlen(str) < 3)continue;

		charcode1 = charcode2 = -1;

		if((str[0] >= 'a') && (str[0] <= 'f')) charcode1 = str[0]-('a'-0xA);
		if((str[0] >= 'A') && (str[0] <= 'F')) charcode1 = str[0]-('A'-0xA);
		if((str[0] >= '0') && (str[0] <= '9')) charcode1 = str[0]-'0';

		if((str[1] >= 'a') && (str[1] <= 'f')) charcode2 = str[1]-('a'-0xA);
		if((str[1] >= 'A') && (str[1] <= 'F')) charcode2 = str[1]-('A'-0xA);
		if((str[1] >= '0') && (str[1] <= '9')) charcode2 = str[1]-'0';
		
		if(charcode1 == -1){
			UnloadTableFile();
			fclose(FP);
			return line; //we have an error getting the first input
		}

		if(charcode2 != -1) charcode1 = (charcode1<<4)|charcode2;

		for(i = 0;i < (int)strlen(str);i++)if(str[i] == '=')break;

		if(i == strlen(str)){
			UnloadTableFile();
			fclose(FP);
			return line; //error no '=' found
		}

		i++;
			//ORing i with 32 just converts it to lowercase if it isn't
		if(((str[i]|32) == 'r') && ((str[i+1]|32) == 'e') && ((str[i+2]|32) == 't'))
			charcode2 = 0x0D;
		else charcode2 = str[i];

		chartable[charcode1] = charcode2;
	}
	TableFileLoaded = 1;
	fclose(FP);
	return -1;

}

void UnloadTableFile(){
	int i, j;
	for(i = 0;i < 256;i++){
		j = i;
		if(j < 0x20)j = 0x2E;
		if(j > 0x7e)j = 0x2E;
		chartable[i] = j;
	}
	TableFileLoaded = 0;
	return;
}
void UpdateMemoryView(int draw_all){
	int i, j;
	//LPVOID lpMsgBuf;
	//int curlength;
	char str[100];
	char str2[100];
	if (!hMemView) return;

/*
	if(draw_all){
		for(i = CurOffset;i < CurOffset+DataAmount;i+=16){
			MoveToEx(HDataDC,0,MemFontHeight*((i-CurOffset)/16),NULL);
			sprintf(str,"%06X: ",i);
			for(j = 0;j < 16;j++){
				sprintf(str2,"%02X ",GetMem(i+j));
				strcat(str,str2);
			}
			strcat(str," : ");
			k = strlen(str);
			for(j = 0;j < 16;j++){
				str[k+j] = GetMem(i+j);
				if(str[k+j] < 0x20)str[k+j] = 0x2E;
				if(str[k+j] > 0x7e)str[k+j] = 0x2E;
			}
			str[k+16] = 0;
			TextOut(HDataDC,0,0,str,strlen(str));
		}
	} else {*/
	for(i = CurOffset;i < CurOffset+DataAmount;i+=16){
		if((OldCurOffset != CurOffset) || draw_all){
			MoveToEx(HDataDC,0,MemFontHeight*((i-CurOffset)/16),NULL);
			SetTextColor(HDataDC,RGB(0,0,0));
			SetBkColor(HDataDC,RGB(255,255,255));
			sprintf(str,"%06X: ",i);
			TextOut(HDataDC,0,0,str,strlen(str));
		}
		for(j = 0;j < 16;j++){
			if((CursorEndAddy == -1) && (CursorStartAddy == i+j)){ //print up single highlighted text
				sprintf(str,"%02X",GetMemViewData(CursorStartAddy));
				OldValues[i+j-CurOffset] = -1; //set it to redraw this one next time
				MoveToEx(HDataDC,8*MemFontWidth+(j*3*MemFontWidth),MemFontHeight*((i-CurOffset)/16),NULL);
				if(TempData != -1){
					sprintf(str2,"%X",TempData);
					SetBkColor(HDataDC,RGB(255,255,255));
					SetTextColor(HDataDC,RGB(255,0,0));
					TextOut(HDataDC,0,0,str2,1);
					SetTextColor(HDataDC,RGB(255,255,255));
					SetBkColor(HDataDC,RGB(0,0,0));
					TextOut(HDataDC,0,0,&str[1],1);
				} else {
					SetTextColor(HDataDC,RGB(255,255,255));
					SetBkColor(HDataDC,RGB(0,0,0));
					TextOut(HDataDC,0,0,str,1);
					SetTextColor(HDataDC,RGB(0,0,0));
					SetBkColor(HDataDC,RGB(255,255,255));
					TextOut(HDataDC,0,0,&str[1],1);
				}
				TextOut(HDataDC,0,0," ",1);

				SetTextColor(HDataDC,RGB(255,255,255));
				SetBkColor(HDataDC,RGB(0,0,0));
				MoveToEx(HDataDC,(59+j)*MemFontWidth,MemFontHeight*((i-CurOffset)/16),NULL); //todo: try moving this above the for loop
				str[0] = chartable[GetMemViewData(i+j)];
				if(str[0] < 0x20)str[0] = 0x2E;
				if(str[0] > 0x7e)str[0] = 0x2E;
				str[1] = 0;
				TextOut(HDataDC,0,0,str,1);

				continue;
			}
			if((OldValues[i+j-CurOffset] != GetMemViewData(i+j)) || draw_all){
				MoveToEx(HDataDC,8*MemFontWidth+(j*3*MemFontWidth),MemFontHeight*((i-CurOffset)/16),NULL);
				SetTextColor(HDataDC,TextColorList[i+j-CurOffset]);//(8+j*3)*MemFontWidth
				SetBkColor(HDataDC,BGColorList[i+j-CurOffset]);
				sprintf(str,"%02X ",GetMemViewData(i+j));
				TextOut(HDataDC,0,0,str,strlen(str));

				MoveToEx(HDataDC,(59+j)*MemFontWidth,MemFontHeight*((i-CurOffset)/16),NULL); //todo: try moving this above the for loop
				str[0] = chartable[GetMemViewData(i+j)];
				if(str[0] < 0x20)str[0] = 0x2E;
				if(str[0] > 0x7e)str[0] = 0x2E;
				str[1] = 0;
				TextOut(HDataDC,0,0,str,1);
				if(CursorStartAddy != i+j)OldValues[i+j-CurOffset] = GetMemViewData(i+j);
			}
		}
		if(draw_all){
			MoveToEx(HDataDC,56*MemFontWidth,MemFontHeight*((i-CurOffset)/16),NULL);
			SetTextColor(HDataDC,RGB(0,0,0));
			SetBkColor(HDataDC,RGB(255,255,255));
			TextOut(HDataDC,0,0," : ",3);
		}/*
		for(j = 0;j < 16;j++){
			if((OldValues[i+j-CurOffset] != GetMem(i+j)) || draw_all){
				MoveToEx(HDataDC,(59+j)*MemFontWidth,MemFontHeight*((i-CurOffset)/16),NULL); //todo: try moving this above the for loop
				SetTextColor(HDataDC,TextColorList[i+j-CurOffset]);
				SetBkColor(HDataDC,BGColorList[i+j-CurOffset]);
				str[0] = GetMem(i+j);
				if(str[0] < 0x20)str[0] = 0x2E;
				if(str[0] > 0x7e)str[0] = 0x2E;
				str[1] = 0;
				TextOut(HDataDC,0,0,str,1);
				if(CursorStartAddy != i+j)OldValues[i+j-CurOffset] = GetMem(i+j);
			}
		}*/
	}
//	}

	SetTextColor(HDataDC,RGB(0,0,0));
	SetBkColor(HDataDC,RGB(255,255,255));

	MoveToEx(HDataDC,0,0,NULL);	
	OldCurOffset = CurOffset;
	return;
}

void UpdateCaption(){
	char str[100];
	char EditString[3][20] = {"RAM","PPU Memory","ROM"};

	if(CursorEndAddy == -1){
		sprintf(str,"Hex Editor - Editing %s Offset 0x%06x",EditString[EditingMode],CursorStartAddy);
	} else {
		sprintf(str,"Hex Editor - Editing %s Offset 0x%06x - 0x%06x, 0x%x bytes selected ",
			EditString[EditingMode],CursorStartAddy,CursorEndAddy,CursorEndAddy-CursorStartAddy+1);
	}
	SetWindowText(hMemView,str);
	return;
}

int GetMemViewData(int i){
	if(EditingMode == 0)return GetMem(i);
	if(EditingMode == 1){
		i &= 0x3FFF;
		if(i < 0x2000)return VPage[(i)>>10][(i)];
		if(i < 0x3F00)return vnapage[(i>>10)&0x3][i&0x3FF];
		return PALRAM[i&0x1F];
	}
	if(EditingMode == 2){ //todo: use getfiledata() here
		if(i < 16) return *((unsigned char *)&head+i);
		if(i < 16+PRGsize[0])return PRGptr[0][i-16];
		if(i < 16+PRGsize[0]+CHRsize[0])return CHRptr[0][i-16-PRGsize[0]];
	}
	return 0;
}

void UpdateColorTable(){
	UNDOSTRUCT *tmp; //mbg merge 7/18/06 removed struct qualifier
	int i,j;
	if(!hMemView)return;
	for(i = 0;i < DataAmount;i++){
		if((i+CurOffset >= CursorStartAddy) && (i+CurOffset <= CursorEndAddy)){
			BGColorList[i] = RGB(0,0,0);
			TextColorList[i] = RGB(255,255,255);
			continue;
		}

		BGColorList[i] = RGB(255,255,255);
		TextColorList[i] = RGB(0,0,0);
	}

	//mbg merge 6/29/06 - added argument
	if(EditingMode == 0)FCEUI_ListCheats(UpdateCheatColorCallB,0);
	
// ################################## Start of SP CODE ###########################

	for (j=0;j<nextBookmark;j++)
	{
		if((hexBookmarks[j].address >= CurOffset) && (hexBookmarks[j].address < CurOffset+DataAmount))
			TextColorList[hexBookmarks[j].address - CurOffset] = RGB(0,0xCC,0);
	}

// ################################## End of SP CODE ###########################

	if(EditingMode == 2){
		if(cdloggerdata) {
			for(i = 0;i < DataAmount;i++){
				if((CurOffset+i >= 16) && (CurOffset+i < 16+PRGsize[0])) {
					if((cdloggerdata[i+CurOffset-16]&3) == 3)TextColorList[i]=RGB(0,192,0);
					if((cdloggerdata[i+CurOffset-16]&3) == 1)TextColorList[i]=RGB(192,192,0);
					if((cdloggerdata[i+CurOffset-16]&3) == 2)TextColorList[i]=RGB(0,0,192);
				}
			}
		}

		tmp=undo_list;
		while(tmp!= 0){
			//if((tmp->addr < CurOffset+DataAmount) && (tmp->addr+tmp->size > CurOffset))
				for(i = tmp->addr;i < tmp->addr+tmp->size;i++){
					if((i > CurOffset) && (i < CurOffset+DataAmount))
						TextColorList[i-CurOffset] = RGB(255,0,0);
				}
			tmp=tmp->last;
		}
	}

	UpdateMemoryView(1); //anytime the colors change, the memory viewer needs to be completely redrawn
}

//mbg merge 6/29/06 - added argument
int UpdateCheatColorCallB(char *name, uint32 a, uint8 v, int compare,int s,int type, void *data) {

	if((a >= CurOffset) && (a < CurOffset+DataAmount)){
	if(s)TextColorList[a-CurOffset] = RGB(0,0,255);
	}
	return 1;
}

int addrtodelete;    // This is a very ugly hackish method of doing this
int cheatwasdeleted; // but it works and that is all that matters here.
int DeleteCheatCallB(char *name, uint32 a, uint8 v, int compare,int s,int type, void *data){  //mbg merge 6/29/06 - added arg
	if(cheatwasdeleted == -1)return 1;
	cheatwasdeleted++;
	if(a == addrtodelete){
		FCEUI_DelCheat(cheatwasdeleted-1);
		cheatwasdeleted = -1;
		return 0;
	}
	return 1;
}

// ################################## Start of SP CODE ###########################

void dumpToFile(const char* buffer, unsigned int size)
{
	char name[257] = {0};
	
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Save to file ...";
	ofn.lpstrFilter="All files (*.*)\0*.*\0";
	ofn.lpstrFile=name;
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_HIDEREADONLY;

	if (GetOpenFileName(&ofn))
	{
		FILE* memfile = fopen(ofn.lpstrFile, "wb");
		
		if (!memfile || fwrite(buffer, 1, size, memfile) != size)
		{
			MessageBox(0, "Saving failed", "Error", 0);
		}

		if (memfile)
			fclose(memfile);
	}
}

void FreezeRam(int address, int mode, int final){
	// mode: -1 == Unfreeze; 0 == Toggle; 1 == Freeze
// ################################## End of SP CODE ###########################
	if((address < 0x2000) || ((address >= 0x6000) && (address <= 0x7FFF))){
		addrtodelete = address;
		cheatwasdeleted = 0;
		
// ################################## Start of SP CODE ###########################
		if (mode == 0 || mode == -1)
		{
			//mbg merge 6/29/06 - added argument
			FCEUI_ListCheats(DeleteCheatCallB,0);
			if(mode == 0 && cheatwasdeleted != -1)FCEUI_AddCheat("",address,GetMem(address),-1,1);
		}
		else
		{
			//mbg merge 6/29/06 - added argument
			FCEUI_ListCheats(DeleteCheatCallB,0);
			FCEUI_AddCheat("",address,GetMem(address),-1,1);
		}
// ################################## End of SP CODE ###########################
		
		/*if (final)
		{
			if(hCheat)RedoCheatsLB(hCheat);
			UpdateColorTable();
		}*/
		//mbg merge 6/29/06 - WTF
	}
}

//input is expected to be an ASCII string
void InputData(char *input){
	//CursorEndAddy = -1;
	int addr, i, j, datasize = 0;
	unsigned char *data;
	char inputc;
	//char str[100];
	//mbg merge 7/18/06 added cast:
	data = (uint8*)malloc(strlen(input)); //it can't be larger than the input string, so use that as the size

	for(i = 0;input[i] != 0;i++){
		if(!EditingText){
			inputc = -1;
			if((input[i] >= 'a') && (input[i] <= 'f')) inputc = input[i]-('a'-0xA);
			if((input[i] >= 'A') && (input[i] <= 'F')) inputc = input[i]-('A'-0xA);
			if((input[i] >= '0') && (input[i] <= '9')) inputc = input[i]-'0';
			if(inputc == -1)continue;

			if(TempData != -1){
				data[datasize++] = inputc|(TempData<<4);
				TempData = -1;
			} else {
				TempData = inputc;
			}
		} else {
			for(j = 0;j < 256;j++)if(chartable[j] == input[i])break;
			if(j == 256)continue;
			data[datasize++] = j;
		}
	}

	if(datasize+CursorStartAddy >= MaxSize){ //too big
		datasize = MaxSize-CursorStartAddy;
		//free(data);
		//return;
	}
	
	//its possible for this loop not to get executed at all
//	for(addr = CursorStartAddy;addr < datasize+CursorStartAddy;addr++){
	//sprintf(str,"datasize = %d",datasize);
	//MessageBox(hMemView,str, "debug", MB_OK);

	for(i = 0;i < datasize;i++){
		addr = CursorStartAddy+i;

		if(EditingMode == 0)BWrite[addr](addr,data[i]);
		if(EditingMode == 1){
			addr &= 0x3FFF;
			if(addr < 0x2000)VPage[addr>>10][addr] = data[i]; //todo: detect if this is vrom and turn it red if so
			if((addr > 0x2000) && (addr < 0x3F00))vnapage[(addr>>10)&0x3][addr&0x3FF] = data[i]; //todo: this causes 0x3000-0x3f00 to mirror 0x2000-0x2f00, is this correct?
			if((addr > 0x3F00) && (addr < 0x3FFF))PALRAM[addr&0x1F] = data[i];
		}
		if(EditingMode == 2){
			ApplyPatch(addr,datasize,data);
			break;
		}
	}
	CursorStartAddy+=datasize;
	CursorEndAddy=-1;
	if(CursorStartAddy >= MaxSize)CursorStartAddy = MaxSize-1;

	free(data);
	ChangeMemViewFocus(EditingMode,CursorStartAddy,-1);
	UpdateColorTable();
	return;
}
/*
	if(!EditingText){
		if((input >= 'a') && (input <= 'f')) input-=('a'-0xA);
		if((input >= 'A') && (input <= 'F')) input-=('A'-0xA);
		if((input >= '0') && (input <= '9')) input-='0';
		if(input > 0xF)return;

		if(TempData != -1){
			addr = CursorStartAddy;
			data = input|(TempData<<4);
			if(EditingMode == 0)BWrite[addr](addr,data);
			if(EditingMode == 1){
				addr &= 0x3FFF;
				if(addr < 0x2000)VPage[addr>>10][addr] = data; //todo: detect if this is vrom and turn it red if so
				if((addr > 0x2000) && (addr < 0x3F00))vnapage[(addr>>10)&0x3][addr&0x3FF] = data; //todo: this causes 0x3000-0x3f00 to mirror 0x2000-0x2f00, is this correct?
				if((addr > 0x3F00) && (addr < 0x3FFF))PALRAM[addr&0x1F] = data;
			}
			if(EditingMode == 2)ApplyPatch(addr,1,(uint8 *)&data);
			CursorStartAddy++;
			TempData = -1;
		} else {
			TempData = input;
		}
	} else {
		for(i = 0;i < 256;i++)if(chartable[i] == input)break;
		if(i == 256)return;

		addr = CursorStartAddy;
		data = i;
		if(EditingMode == 0)BWrite[addr](addr,data);
		if(EditingMode == 2)ApplyPatch(addr,1,(uint8 *)&data);
		CursorStartAddy++;
	}
	*/


void ChangeMemViewFocus(int newEditingMode, int StartOffset,int EndOffset){
	SCROLLINFO si;
	
	if (GI->type==GIT_NSF) {
		FCEUD_PrintError("Sorry, you can't yet use the Memory Viewer with NSFs.");
		return;
	}

	if(!hMemView)DoMemView();
	if(EditingMode != newEditingMode)
		MemViewCallB(hMemView,WM_COMMAND,300+newEditingMode,0); //let the window handler change this for us

	if((EndOffset == StartOffset) || (EndOffset == -1)){
		CursorEndAddy = -1;
		CursorStartAddy = StartOffset;
	} else {
		CursorStartAddy = min(StartOffset,EndOffset);
		CursorEndAddy = max(StartOffset,EndOffset);
	}


	if(min(StartOffset,EndOffset) >= MaxSize)return; //this should never happen

	if(StartOffset < CurOffset){
		CurOffset = (StartOffset/16)*16;
	}

	if(StartOffset >= CurOffset+DataAmount){
		CurOffset = ((StartOffset/16)*16)-DataAmount+0x10;
		if(CurOffset < 0)CurOffset = 0;
	}

	SetFocus(hMemView);	
	ZeroMemory(&si, sizeof(SCROLLINFO));
	si.fMask = SIF_POS;
	si.cbSize = sizeof(SCROLLINFO);
	si.nPos = CurOffset/16;
	SetScrollInfo(hMemView,SB_VERT,&si,TRUE);
	UpdateCaption();
	UpdateColorTable();
	return;
}


int GetHexScreenCoordx(int offset){
	return (8*MemFontWidth)+((offset%16)*3*MemFontWidth); //todo: add Curoffset to this and to below function
}

int GetHexScreenCoordy(int offset){
	return (offset/16)*MemFontHeight;
}

//0000E0: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  : ................

//if the mouse is in the text field, this function will set AddyWasText to 1 otherwise it is 0
//if the mouse wasn't in any range, this function returns -1
int GetAddyFromCoord(int x,int y){
	if(y < 0)y = 0;
	if(x < 8*MemFontWidth)x = 8*MemFontWidth+1;

	if(y > DataAmount*MemFontHeight) return -1;

	if(x < 55*MemFontWidth){
		AddyWasText = 0;
		return ((y/MemFontHeight)*16)+((x-(8*MemFontWidth))/(3*MemFontWidth))+CurOffset;
	}

	if((x > 59*MemFontWidth) && (x < 75*MemFontWidth)){
		AddyWasText = 1;
		return ((y/MemFontHeight)*16)+((x-(59*MemFontWidth))/(MemFontWidth))+CurOffset;
	}
	
	return -1;
}

void AutoScrollFromCoord(int x,int y){
	SCROLLINFO si;
	if(y < 0){
		ZeroMemory(&si, sizeof(SCROLLINFO));
		si.fMask = SIF_ALL;
		si.cbSize = sizeof(SCROLLINFO);
		GetScrollInfo(hMemView,SB_VERT,&si);
		si.nPos += y / 16;
		if (si.nPos < si.nMin) si.nPos = si.nMin;
		if ((si.nPos+si.nPage) > si.nMax) si.nPos = si.nMax-si.nPage;
		CurOffset = si.nPos*16;
		SetScrollInfo(hMemView,SB_VERT,&si,TRUE);
		return;
	}

	if(y > ClientHeight){
		ZeroMemory(&si, sizeof(SCROLLINFO));
		si.fMask = SIF_ALL;
		si.cbSize = sizeof(SCROLLINFO);
		GetScrollInfo(hMemView,SB_VERT,&si);
		si.nPos -= (ClientHeight-y) / 16;
		if (si.nPos < si.nMin) si.nPos = si.nMin;
		if ((si.nPos+si.nPage) > si.nMax) si.nPos = si.nMax-si.nPage;
		CurOffset = si.nPos*16;
		SetScrollInfo(hMemView,SB_VERT,&si,TRUE);
		return;
	}
}

void KillMemView(){
	DeleteObject(hMemFont);
	ReleaseDC(hMemView,mDC);
	DestroyWindow(hMemView);
	UnregisterClass("MEMVIEW",fceu_hInstance);
	hMemView = 0;
	return;
}

LRESULT CALLBACK MemViewCallB(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam){
	HDC          hdc;
	HGLOBAL      hGlobal ;
	PTSTR        pGlobal ;
	HMENU        hMenu;
	MENUITEMINFO MenuInfo;
	POINT        point;
	PAINTSTRUCT ps ;
	TEXTMETRIC tm;
	SCROLLINFO si;
	int x, y, i, j;

	char c[2];
	char str[100];
// ################################## Start of SP CODE ###########################
			extern int debuggerWasActive;
// ################################## End of SP CODE ###########################

	switch (message) {
		case WM_ENTERMENULOOP:StopSound();return 0;
		case WM_INITMENUPOPUP:
			if(undo_list != 0)EnableMenuItem(GetMenu(hMemView),200,MF_BYCOMMAND | MF_ENABLED);
			else EnableMenuItem(GetMenu(hMemView),200,MF_BYCOMMAND | MF_GRAYED);
			
			if(TableFileLoaded)EnableMenuItem(GetMenu(hMemView),103,MF_BYCOMMAND | MF_ENABLED);
			else EnableMenuItem(GetMenu(hMemView),103,MF_BYCOMMAND | MF_GRAYED);

		return 0;

		case WM_CREATE:
// ################################## Start of SP CODE ###########################
			debuggerWasActive = 1;
// ################################## End of SP CODE ###########################
			mDC = GetDC(hwnd);
			HDataDC = mDC;//deleteme
			hMemFont = CreateFont(13,8, /*Height,Width*/
				0,0, /*escapement,orientation*/
				400,FALSE,FALSE,FALSE, /*weight, italic,, underline, strikeout*/
				ANSI_CHARSET,OUT_DEVICE_PRECIS,CLIP_MASK, /*charset, precision, clipping*/
				DEFAULT_QUALITY, DEFAULT_PITCH, /*quality, and pitch*/
				"Courier"); /*font name*/
			SelectObject (HDataDC, hMemFont);
			SetTextAlign(HDataDC,TA_UPDATECP | TA_TOP | TA_LEFT);

			GetTextMetrics (HDataDC, &tm) ;
			MemFontWidth = 8;
			MemFontHeight = 13;

			MaxSize = 0x10000;
			//Allocate Memory for color lists
			DataAmount = 0x100;
			//mbg merge 7/18/06 added casts:
			TextColorList = (COLORREF*)malloc(DataAmount*sizeof(COLORREF));
			BGColorList = (COLORREF*)malloc(DataAmount*sizeof(COLORREF));
			OldValues = (int*)malloc(DataAmount*sizeof(int));
			EditingText = EditingMode = CurOffset = 0;
			
			//set the default table
			UnloadTableFile();
			UpdateColorTable(); //draw it
			updateBookmarkMenus(GetSubMenu(GetMenu(hwnd), 3));
			return 0;
		case WM_PAINT:
			hdc = BeginPaint(hwnd, &ps);
			EndPaint(hwnd, &ps);
			UpdateMemoryView(1);
			return 0;
		case WM_VSCROLL:
			
				StopSound();
				ZeroMemory(&si, sizeof(SCROLLINFO));
				si.fMask = SIF_ALL;
				si.cbSize = sizeof(SCROLLINFO);
				GetScrollInfo(hwnd,SB_VERT,&si);
				switch(LOWORD(wParam)) {
					case SB_ENDSCROLL:
					case SB_TOP:
					case SB_BOTTOM: break;
					case SB_LINEUP: si.nPos--; break;
					case SB_LINEDOWN:si.nPos++; break;
					case SB_PAGEUP: si.nPos-=si.nPage; break;
					case SB_PAGEDOWN: si.nPos+=si.nPage; break;
					case SB_THUMBPOSITION: //break;
					case SB_THUMBTRACK: si.nPos = si.nTrackPos; break;
				}
				if (si.nPos < si.nMin) si.nPos = si.nMin;
				if ((si.nPos+(int)si.nPage) > si.nMax) si.nPos = si.nMax-si.nPage; //mbg merge 7/18/06 added cast
				CurOffset = si.nPos*16;
				SetScrollInfo(hwnd,SB_VERT,&si,TRUE);
				UpdateColorTable();
			return 0;
		case WM_CHAR:
				if(GetKeyState(VK_CONTROL) & 0x8000)return 0; //prevents input when pressing ctrl+c
				c[0] = (char)(wParam&0xFF);
				c[1] = 0;
				//sprintf(str,"c[0] = %c c[1] = %c",c[0],c[1]);
				//MessageBox(hMemView,str, "debug", MB_OK);
				InputData(c);
				UpdateColorTable();
				UpdateCaption();
			return 0;

		case WM_KEYDOWN:
				//if((wParam >= 0x30) && (wParam <= 0x39))InputData(wParam-0x30);
				//if((wParam >= 0x41) && (wParam <= 0x46))InputData(wParam-0x41+0xA);
				/*if(!((GetKeyState(VK_LSHIFT) & 0x8000) || (GetKeyState(VK_RSHIFT) & 0x8000))){
					//MessageBox(hMemView,"nobody", "mouse wheel dance!", MB_OK);
					CursorShiftPoint = -1;
				}
				if(((GetKeyState(VK_LSHIFT) & 0x8000) || (GetKeyState(VK_RSHIFT) & 0x8000)) &&
					(CursorShiftPoint == -1)){
					CursorShiftPoint = CursorStartAddy;
					//MessageBox(hMemView,"somebody", "mouse wheel dance!", MB_OK);
				}*/

				if(GetKeyState(VK_CONTROL) & 0x8000){
					
// ################################## Start of SP CODE ###########################

					if (wParam >= '0' && wParam <= '9')
					{
						int newValue = handleBookmarkMenu(wParam - '0');
						
						if (newValue != -1)
						{
							CurOffset = newValue;
							CursorEndAddy = -1;
							CursorStartAddy = hexBookmarks[wParam - '0'].address;
							UpdateColorTable();
						}
					}
					
// ################################## End of SP CODE ###########################

					switch(wParam){
						case 0x43: //Ctrl+C
							MemViewCallB(hMemView,WM_COMMAND,201,0); //recursion at work
							return 0;
						case 0x56: //Ctrl+V
							MemViewCallB(hMemView,WM_COMMAND,202,0);
							return 0;
						case 0x5a: //Ctrl+Z
							UndoLastPatch();
					}
				}

				//if(CursorShiftPoint == -1){
					if(wParam == VK_LEFT)CursorStartAddy--;
					if(wParam == VK_RIGHT)CursorStartAddy++;
					if(wParam == VK_UP)CursorStartAddy-=16;
					if(wParam == VK_DOWN)CursorStartAddy+=16;
				/*} else {
					if(wParam == VK_LEFT)CursorShiftPoint--;
					if(wParam == VK_RIGHT)CursorShiftPoint++;
					if(wParam == VK_UP)CursorShiftPoint-=16;
					if(wParam == VK_DOWN)CursorShiftPoint+=16;
					if(CursorShiftPoint < CursorStartAddy){
						if(CursorEndAddy == -1)CursorEndAddy = CursorStartAddy;
						CursorStartAddy = CursorShiftPoint;
					}
					//if(CursorShiftPoint > CursorEndAddy)CursorEndAddy = CursorShiftPoint;
				}*/

				//if(CursorStartAddy == CursorEndAddy)CursorEndAddy = -1;
				if(CursorStartAddy < 0)CursorStartAddy = 0;
				if(CursorStartAddy >= MaxSize)CursorStartAddy = MaxSize-1; //todo: fix this up when I add support for editing more stuff

				if((wParam == VK_DOWN) || (wParam == VK_UP) ||
					(wParam == VK_RIGHT) || (wParam == VK_LEFT)){
					CursorEndAddy = -1;
					TempData = -1;
					if(CursorStartAddy < CurOffset) CurOffset = (CursorStartAddy/16)*16;
					if(CursorStartAddy > CurOffset+DataAmount-0x10)CurOffset = ((CursorStartAddy-DataAmount+0x10)/16)*16;
				}
				
				if(wParam == VK_PRIOR)CurOffset-=DataAmount;
				if(wParam == VK_NEXT)CurOffset+=DataAmount;
				if(CurOffset < 0)CurOffset = 0;
				if(CurOffset >= MaxSize)CurOffset = MaxSize-1;
			/*
				if((wParam == VK_PRIOR) || (wParam == VK_NEXT)){
					ZeroMemory(&si, sizeof(SCROLLINFO));
					si.fMask = SIF_ALL;
					si.cbSize = sizeof(SCROLLINFO);
					GetScrollInfo(hwnd,SB_VERT,&si);
					if(wParam == VK_PRIOR)si.nPos-=si.nPage;
					if(wParam == VK_NEXT)si.nPos+=si.nPage;
					if (si.nPos < si.nMin) si.nPos = si.nMin;
					if ((si.nPos+si.nPage) > si.nMax) si.nPos = si.nMax-si.nPage;
					CurOffset = si.nPos*16;
				}
				*/
				
				//This updates the scroll bar to curoffset
				ZeroMemory(&si, sizeof(SCROLLINFO));
				si.fMask = SIF_POS;
				si.cbSize = sizeof(SCROLLINFO);
				si.nPos = CurOffset/16;
				SetScrollInfo(hwnd,SB_VERT,&si,TRUE);
				UpdateColorTable();
				UpdateCaption();
			return 0;
/*		case WM_KEYUP:
				if((wParam == VK_LSHIFT) || (wParam == VK_RSHIFT)){
					CursorShiftPoint = -1;
				}
				return 0;*/
		case WM_LBUTTONDOWN:
				//CursorShiftPoint = -1;
				SetCapture(hwnd);
				lbuttondown = 1;
				x = GET_X_LPARAM(lParam);
				y = GET_Y_LPARAM(lParam);
				if((i = GetAddyFromCoord(x,y)) == -1)return 0;
				EditingText = AddyWasText;
				lbuttondownx = x;
				lbuttondowny = y;
				CursorStartAddy = CursorDragPoint = i;
				CursorEndAddy = -1;
				UpdateCaption();
				UpdateColorTable();
			return 0;
		case WM_MOUSEMOVE:
				mousex = x = GET_X_LPARAM(lParam); 
				mousey = y = GET_Y_LPARAM(lParam); 
				if(lbuttondown){
					AutoScrollFromCoord(x,y);
					i = GetAddyFromCoord(x,y);
					if (i >= MaxSize)i = MaxSize-1;
					EditingText = AddyWasText;
					if(i != -1){
						CursorStartAddy = min(i,CursorDragPoint);
						CursorEndAddy = max(i,CursorDragPoint);
						if(CursorEndAddy == CursorStartAddy)CursorEndAddy = -1;
					}

					UpdateCaption();
					UpdateColorTable();
				}
				//sprintf(str,"%d %d",mousex, mousey);
				//SetWindowText(hMemView,str);
			return 0;
		case WM_LBUTTONUP:
				lbuttondown = 0;
				if(CursorEndAddy == CursorStartAddy)CursorEndAddy = -1;
				if((CursorEndAddy < CursorStartAddy) && (CursorEndAddy != -1)){ //this reverses them if they're not right
					i = CursorStartAddy;
					CursorStartAddy = CursorEndAddy;
					CursorEndAddy = i;
				}
				UpdateCaption();
				UpdateColorTable();
				ReleaseCapture();
			return 0;
		case WM_CONTEXTMENU:
				point.x = x = GET_X_LPARAM(lParam);
				point.y = y = GET_Y_LPARAM(lParam);
				ScreenToClient(hMemView,&point);
				mousex = point.x;
				mousey = point.y;
				j = GetAddyFromCoord(mousex,mousey);
				//sprintf(str,"x = %d, y = %d, j = %d",mousex,mousey,j);
				//MessageBox(hMemView,str, "mouse wheel dance!", MB_OK);
				hMenu = CreatePopupMenu();
				for(i = 0;i < POPUPNUM;i++){
					if((j >= popupmenu[i].minaddress) && (j <= popupmenu[i].maxaddress)
					&& (EditingMode == popupmenu[i].editingmode)){
						memset(&MenuInfo,0,sizeof(MENUITEMINFO));
						switch(popupmenu[i].id){ //this will set the text for the menu dynamically based on the id
// ################################## Start of SP CODE ###########################
							case 1:
							{
								HMENU sub = CreatePopupMenu();
								AppendMenu(hMenu, MF_POPUP | MF_STRING, (UINT)sub, "Freeze / Unfreeze Address");
								AppendMenu(sub, MF_STRING, 1, "Toggle state");
								AppendMenu(sub, MF_STRING, 50, "Freeze");
								AppendMenu(sub, MF_STRING, 51, "Unfreeze");
								AppendMenu(sub, MF_SEPARATOR, 52, "-");
								AppendMenu(sub, MF_STRING, 53, "Unfreeze all");
								
								continue;
							}
// ################################## End of SP CODE ###########################
							case 2 : //We want this to give the address to add the read breakpoint for
								if((j <= CursorEndAddy) && (j >= CursorStartAddy))
									sprintf(str,"Add Read Breakpoint For Address 0x%04X-0x%04X",CursorStartAddy,CursorEndAddy);
								else
									sprintf(str,"Add Read Breakpoint For Address 0x%04X",j);
								popupmenu[i].text = str;
							break;

							case 3 :
								if((j <= CursorEndAddy) && (j >= CursorStartAddy))
									sprintf(str,"Add Write Breakpoint For Address 0x%04X-0x%04X",CursorStartAddy,CursorEndAddy);
								else
									sprintf(str,"Add Write Breakpoint For Address 0x%04X",j);
								popupmenu[i].text = str;
							break;
							case 4 :
								if((j <= CursorEndAddy) && (j >= CursorStartAddy))
									sprintf(str,"Add Execute Breakpoint For Address 0x%04X-0x%04X",CursorStartAddy,CursorEndAddy);
								else
									sprintf(str,"Add Execute Breakpoint For Address 0x%04X",j);
								popupmenu[i].text = str;
							break;
						}
						MenuInfo.cbSize = sizeof(MENUITEMINFO);
						MenuInfo.fMask = MIIM_TYPE | MIIM_ID | MIIM_DATA;
						MenuInfo.fType = MF_STRING;
						MenuInfo.dwTypeData = popupmenu[i].text;
						MenuInfo.cch = strlen(popupmenu[i].text);
						MenuInfo.wID = popupmenu[i].id;
						InsertMenuItem(hMenu,i+1,1,&MenuInfo);
					}
				}
				//InsertMenu(hMenu, 1, MF_STRING, 892, "Test");
				if(i != 0)i = TrackPopupMenuEx(hMenu, TPM_RETURNCMD, x, y, hMemView, NULL);
				switch(i){
					case 1 : //1 = Freeze Ram Address
// ################################## Start of SP CODE ###########################
					{
						int n;
						for (n=CursorStartAddy;(CursorEndAddy == -1 && n == CursorStartAddy) || n<=CursorEndAddy;n++)
						{
							FreezeRam(n, 0, n == CursorEndAddy);
						}
						break;
					}
					case 50:
					{
						int n;
						for (n=CursorStartAddy;(CursorEndAddy == -1 && n == CursorStartAddy) || n<=CursorEndAddy;n++)
						{
							FreezeRam(n, 1, n == CursorEndAddy);
						}
						break;
					}
					case 51:
					{
						int n;
						for (n=CursorStartAddy;(CursorEndAddy == -1 && n == CursorStartAddy) || n<=CursorEndAddy;n++)
						{
							FreezeRam(n, -1, n == CursorEndAddy);
						}
						break;
					}
					case 53:
					{
						int n;
						for (n=0;n<0x2000;n++)
						{
							FreezeRam(n, -1, 0);
						}
						for (n=0x6000;n<0x8000;n++)
						{
							FreezeRam(n, -1, n == 0x7FFF);
						}
						break;
					}
// ################################## End of SP CODE ###########################
					break;

					case 2 : //2 = Add Read Breakpoint
						watchpoint[numWPs].flags = WP_E | WP_R;
						if(EditingMode == 1)watchpoint[numWPs].flags |= BT_P;
							if((j <= CursorEndAddy) && (j >= CursorStartAddy)){
								watchpoint[numWPs].address = CursorStartAddy;
								watchpoint[numWPs].endaddress = CursorEndAddy;
							}			
							else{
								watchpoint[numWPs].address = j;
								watchpoint[numWPs].endaddress = 0;
							}
						numWPs++;
// ################################## Start of SP CODE ###########################
						{ extern int myNumWPs;
						myNumWPs++; }
// ################################## End of SP CODE ###########################
						if(hDebug)AddBreakList();
						else DoDebug(0);
					break;

					case 3 : //3 = Add Write Breakpoint
						watchpoint[numWPs].flags = WP_E | WP_W;
						if(EditingMode == 1)watchpoint[numWPs].flags |= BT_P;
							if((j <= CursorEndAddy) && (j >= CursorStartAddy)){
								watchpoint[numWPs].address = CursorStartAddy;
								watchpoint[numWPs].endaddress = CursorEndAddy;
							}			
							else{
								watchpoint[numWPs].address = j;
								watchpoint[numWPs].endaddress = 0;
							}
						numWPs++;
// ################################## Start of SP CODE ###########################
						{ extern int myNumWPs;
						myNumWPs++; }
// ################################## End of SP CODE ###########################
						if(hDebug)AddBreakList();
						else DoDebug(0);
					break;
					case 4 : //4 = Add Execute Breakpoint
						watchpoint[numWPs].flags = WP_E | WP_X;
							if((j <= CursorEndAddy) && (j >= CursorStartAddy)){
								watchpoint[numWPs].address = CursorStartAddy;
								watchpoint[numWPs].endaddress = CursorEndAddy;
							}			
							else{
								watchpoint[numWPs].address = j;
								watchpoint[numWPs].endaddress = 0;
							}
						numWPs++;
// ################################## Start of SP CODE ###########################
						{ extern int myNumWPs;
						myNumWPs++; }
// ################################## End of SP CODE ###########################
						if(hDebug)AddBreakList();
						else DoDebug(0);
					break;
					case 5 : //5 = Go Here In Rom File
						ChangeMemViewFocus(2,GetNesFileAddress(j),-1);
					break;
					case 6 : //6 = Create GG Code
						SetGGConvFocus(j,GetMem(j));
					break;
// ################################## Start of SP CODE ###########################
					case 20:
					{
						if (toggleBookmark(hwnd, CursorStartAddy))
						{
							MessageBox(hDebug, "Can't set more than 64 breakpoints", "Error", MB_OK | MB_ICONERROR);
						}
						else
						{
							updateBookmarkMenus(GetSubMenu(GetMenu(hwnd), 3));
							UpdateColorTable();
						}
					}
					break;
// ################################## End of SP CODE ###########################
				}
//6 = Create GG Code

			return 0;
		case WM_MBUTTONDOWN:
				x = GET_X_LPARAM(lParam);
				y = GET_Y_LPARAM(lParam);
				i = GetAddyFromCoord(x,y);
				if(i == -1)return 0;
// ################################## Start of SP CODE ###########################
				FreezeRam(i, 0, 1);
// ################################## End of SP CODE ###########################
			return 0;
		case WM_MOUSEWHEEL:
				i = (short)HIWORD(wParam);///WHEEL_DELTA;
				ZeroMemory(&si, sizeof(SCROLLINFO));
				si.fMask = SIF_ALL;
				si.cbSize = sizeof(SCROLLINFO);
				GetScrollInfo(hwnd,SB_VERT,&si);
				if(i < 0)si.nPos+=si.nPage;
				if(i > 0)si.nPos-=si.nPage;
				if (si.nPos < si.nMin) si.nPos = si.nMin;
				if ((si.nPos+(int)si.nPage) > si.nMax) si.nPos = si.nMax-si.nPage; //added cast
				CurOffset = si.nPos*16;
				SetScrollInfo(hwnd,SB_VERT,&si,TRUE);
				UpdateColorTable();
			return 0;

		case WM_SIZE:
			StopSound();
			ClientHeight = HIWORD (lParam) ;
			if(DataAmount != ((ClientHeight/MemFontHeight)*16)){
				DataAmount = ((ClientHeight/MemFontHeight)*16);
				if(DataAmount+CurOffset > MaxSize)CurOffset = MaxSize-DataAmount;
				//mbg merge 7/18/06 added casts:
				TextColorList = (COLORREF*)realloc(TextColorList,DataAmount*sizeof(COLORREF));
				BGColorList = (COLORREF*)realloc(BGColorList,DataAmount*sizeof(COLORREF));
				OldValues = (int*)realloc(OldValues,(DataAmount)*sizeof(int)); 
				for(i = 0;i < DataAmount;i++)OldValues[i] = -1;
			}
			//Set vertical scroll bar range and page size
			ZeroMemory(&si, sizeof(SCROLLINFO));
			si.cbSize = sizeof (si) ;
			si.fMask  = (SIF_RANGE|SIF_PAGE) ;
			si.nMin   = 0 ;
			si.nMax   = MaxSize/16 ;
			si.nPage  = ClientHeight/MemFontHeight;
			SetScrollInfo (hwnd, SB_VERT, &si, TRUE);
			UpdateColorTable();
			return 0 ;

		case WM_COMMAND:
			StopSound();
			
// ################################## Start of SP CODE ###########################
			if (wParam >= 30 && wParam <= 39)
			{
				int newValue = handleBookmarkMenu(wParam - 30);
				
				if (newValue != -1)
				{
					CurOffset = newValue;
					CursorEndAddy = -1;
					CursorStartAddy = hexBookmarks[wParam - 30].address;
					UpdateColorTable();
				}
			}
			else if (wParam == 400)
			{
				removeAllBookmarks(GetSubMenu(GetMenu(hwnd), 3));
				UpdateColorTable();
			}
			else if (wParam == 600)
			{
				MessageBox(0, "", "", 0);
			}
// ################################## End of SP CODE ###########################
			
			switch(wParam)
			{
				case 100:
					FlushUndoBuffer();
					iNesSave();
					UpdateColorTable();
				return 0;

				case 101:
				return 0;

				case 102:
					if((i = LoadTableFile()) != -1){
						sprintf(str,"Error Loading Table File At Line %d",i);
						MessageBox(hMemView,str,"error", MB_OK);
					}
					UpdateColorTable();
				return 0;

				case 103:
					UnloadTableFile();
					UpdateColorTable();
				return 0;
				
// ################################## Start of SP CODE ###########################
				case 104:
					{
						char bar[0x800];
						unsigned int i;
						for (i=0;i<sizeof(bar);i++) bar[i] = GetMem(i);
							
						dumpToFile(bar, sizeof(bar));
						return 0;
					}
				case 105:
					{
						char bar[0x4000];
						unsigned int i;
						for (i=0;i<sizeof(bar);i++)
						{
//							bar[i] = GetPPUMem(i);
							i &= 0x3FFF;
							if(i < 0x2000) bar[i] = VPage[(i)>>10][(i)];
							else if(i < 0x3F00) bar[i] = vnapage[(i>>10)&0x3][i&0x3FF];
							else bar[i] = PALRAM[i&0x1F];
						}
						dumpToFile(bar, sizeof(bar));
						return 0;
					}
// ################################## End of SP CODE ###########################

				case 200: //undo
					UndoLastPatch();
				return 0;

				case 201: //copy
						if(CursorEndAddy == -1)i = 1;
						else i = CursorEndAddy-CursorStartAddy+1;

						hGlobal = GlobalAlloc (GHND, 
							(i*2)+1); //i*2 is two characters per byte, plus terminating null

						pGlobal = (char*)GlobalLock (hGlobal) ; //mbg merge 7/18/06 added cast
						if(!EditingText){
							for(j = 0;j < i;j++){
								str[0] = 0;
								sprintf(str,"%02X",GetMemViewData(j+CursorStartAddy));
								strcat(pGlobal,str);
							}
						} else {
							for(j = 0;j < i;j++){
								str[0] = 0;
								sprintf(str,"%c",chartable[GetMemViewData(j+CursorStartAddy)]);
								strcat(pGlobal,str);
							}
						}
						GlobalUnlock (hGlobal);
						OpenClipboard (hwnd) ;
						EmptyClipboard () ;
						SetClipboardData (CF_TEXT, hGlobal) ;
						CloseClipboard () ;
				return 0;

				case 202: //paste
							
							OpenClipboard(hwnd);
							hGlobal = GetClipboardData(CF_TEXT);
							if(hGlobal == NULL){
								CloseClipboard();
								return 0;
							}
							pGlobal = (char*)GlobalLock (hGlobal) ; //mbg merge 7/18/06 added cast
							//for(i = 0;pGlobal[i] != 0;i++){
							InputData(pGlobal);
							//}
							GlobalUnlock (hGlobal);
							CloseClipboard();
							return 0;
				return 0;

				case 203: //find
							OpenFindDialog();
				return 0;


				case 300:
				case 301:
				case 302:
					EditingMode = wParam-300;
					for(i = 0;i < 3;i++){
						if(EditingMode == i)CheckMenuItem(GetMenu(hMemView),300+i,MF_CHECKED);
						else CheckMenuItem(GetMenu(hMemView),300+i,MF_UNCHECKED);
					}
					if(EditingMode == 0)MaxSize = 0x10000;
					if(EditingMode == 1)MaxSize = 0x4000;
					if(EditingMode == 2)MaxSize = 16+CHRsize[0]+PRGsize[0]; //todo: add trainer size
					if(DataAmount+CurOffset > MaxSize)CurOffset = MaxSize-DataAmount;
					if(CursorEndAddy > MaxSize)CursorEndAddy = -1;
					if(CursorStartAddy > MaxSize)CursorStartAddy= MaxSize-1;

					//Set vertical scroll bar range and page size
					ZeroMemory(&si, sizeof(SCROLLINFO));
					si.cbSize = sizeof (si) ;
					si.fMask  = (SIF_RANGE|SIF_PAGE) ;
					si.nMin   = 0 ;
					si.nMax   = MaxSize/16 ;
					si.nPage  = ClientHeight/MemFontHeight;
					SetScrollInfo (hwnd, SB_VERT, &si, TRUE);

					for(i = 0;i < DataAmount;i++)OldValues[i] = -1;
					
					UpdateColorTable();
					return 0;
			}

		case WM_MOVE:
			StopSound();
			return 0;

		case WM_DESTROY :
			KillMemView();
			//ReleaseDC (hwnd, mDC) ;
			//DestroyWindow(hMemView);
			//UnregisterClass("MEMVIEW",fceu_hInstance);
			//hMemView = 0;
			return 0;
	 }
	return DefWindowProc (hwnd, message, wParam, lParam) ;
}



void DoMemView() {
	WNDCLASSEX     wndclass ;
	//static RECT al;

	if (!GI) {
		FCEUD_PrintError("You must have a game loaded before you can use the Memory Viewer.");
		return;
	}
	if (GI->type==GIT_NSF) {
		FCEUD_PrintError("Sorry, you can't yet use the Memory Viewer with NSFs.");
		return;
	}

	if (!hMemView){
		memset(&wndclass,0,sizeof(wndclass));
		wndclass.cbSize=sizeof(WNDCLASSEX);
		wndclass.style         = CS_HREDRAW | CS_VREDRAW ;
		wndclass.lpfnWndProc   = MemViewCallB ;
		wndclass.cbClsExtra    = 0 ;
		wndclass.cbWndExtra    = 0 ;
		wndclass.hInstance     = fceu_hInstance;
		wndclass.hIcon         = LoadIcon(fceu_hInstance, "FCEUXD_ICON");
		wndclass.hIconSm       = LoadIcon(fceu_hInstance, "FCEUXD_ICON");
		wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
		wndclass.hbrBackground = (HBRUSH) GetStockObject(WHITE_BRUSH) ;
		wndclass.lpszMenuName  = "MEMVIEWMENU" ; //TODO: add a menu
		wndclass.lpszClassName = "MEMVIEW" ;

		if(!RegisterClassEx(&wndclass)) {FCEUD_PrintError("Error Registering MEMVIEW Window Class."); return;}

		hMemView = CreateWindowEx(0,"MEMVIEW","Memory Editor",
                        //WS_OVERLAPPEDWINDOW|WS_CLIPSIBLINGS,  /* Style */
						WS_SYSMENU|WS_THICKFRAME|WS_VSCROLL,
                        CW_USEDEFAULT,CW_USEDEFAULT,625,242,  /* X,Y ; Width, Height */
                        NULL,NULL,fceu_hInstance,NULL ); 
		ShowWindow (hMemView, SW_SHOW) ;
		UpdateCaption();
		//hMemView = CreateDialog(fceu_hInstance,"MEMVIEW",NULL,MemViewCallB);
	}
	if (hMemView) {
		SetWindowPos(hMemView,HWND_TOP,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);
		//UpdateMemView(0);
		//MemViewDoBlit();
	}
}

BOOL CALLBACK MemFindCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {

	switch(uMsg) {
		case WM_INITDIALOG:
			if(FindDirectionUp) CheckDlgButton(hwndDlg, 1003, BST_CHECKED);
			else CheckDlgButton(hwndDlg, 1004, BST_CHECKED);

			if(FindAsText) CheckDlgButton(hwndDlg, 1002, BST_CHECKED);
			else CheckDlgButton(hwndDlg, 1001, BST_CHECKED);
			
			if(FindTextBox[0])SetDlgItemText(hwndDlg,1000,FindTextBox);

			SendDlgItemMessage(hwndDlg,1000,EM_SETLIMITTEXT,59,0);
			break;
		case WM_CREATE:

			break;
		case WM_PAINT:
			break;
		case WM_CLOSE:
		case WM_QUIT:
			GetDlgItemText(hMemFind,1000,FindTextBox,59);
			DestroyWindow(hMemFind);
			hMemFind = 0;
			break;
		case WM_MOVING:
			break;
		case WM_MOVE:
			break;
		case WM_RBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
			break;
		case WM_MOUSEMOVE:
			break;

		case WM_COMMAND:
			switch(HIWORD(wParam)) {
				case BN_CLICKED:
					switch(LOWORD(wParam)) {
						case 1001 :
							FindAsText=0;
						break;
						case 1002 :
							FindAsText=1;
						break;

						case 1003 :
							FindDirectionUp = 1;
						break;
						case 1004 :
							FindDirectionUp = 0;
						break;
						case 1005 :
							FindNext();
						break;
					}
					break;
			}
			break;
		case WM_HSCROLL:
			break;
	}
	return FALSE;
}
void FindNext(){
	char str[60];
	unsigned char data[60];
	int datasize = 0, i, j, inputc = -1, found;

	if(hMemFind) GetDlgItemText(hMemFind,1000,str,59);
	else strcpy(str,FindTextBox);

	for(i = 0;str[i] != 0;i++){
		if(!FindAsText){
			if(inputc == -1){
				if((str[i] >= 'a') && (str[i] <= 'f')) inputc = str[i]-('a'-0xA);
				if((str[i] >= 'A') && (str[i] <= 'F')) inputc = str[i]-('A'-0xA);
				if((str[i] >= '0') && (str[i] <= '9')) inputc = str[i]-'0';
			} else {
				if((str[i] >= 'a') && (str[i] <= 'f')) inputc = (inputc<<4)|(str[i]-('a'-0xA));
				if((str[i] >= 'A') && (str[i] <= 'F')) inputc = (inputc<<4)|(str[i]-('A'-0xA));
				if((str[i] >= '0') && (str[i] <= '9')) inputc = (inputc<<4)|(str[i]-'0');
				
				if(((str[i] >= 'a') && (str[i] <= 'f')) ||
				((str[i] >= 'A') && (str[i] <= 'F')) ||
				((str[i] >= '0') && (str[i] <= '9'))){
					data[datasize++] = inputc;
					inputc = -1;
				}
			}
		} else {
			for(j = 0;j < 256;j++)if(chartable[j] == str[i])break;
			if(j == 256)continue;
			data[datasize++] = j;
		}
	}
	
	if(datasize < 1){
		MessageBox(hMemView,"Invalid String","Error", MB_OK);
		return;
	}
	if(!FindDirectionUp){
		for(i = CursorStartAddy+1;i+datasize < MaxSize;i++){
			found = 1;
			for(j = 0;j < datasize;j++){
				if(GetMemViewData(i+j) != data[j])found = 0;
			}
			if(found == 1){
				ChangeMemViewFocus(EditingMode,i, i+datasize-1);
				return;
			}
		}
		for(i = 0;i < CursorStartAddy;i++){
			found = 1;
			for(j = 0;j < datasize;j++){
				if(GetMemViewData(i+j) != data[j])found = 0;
			}
			if(found == 1){
				ChangeMemViewFocus(EditingMode,i, i+datasize-1);
				return;
			}
		}
	} else { //FindDirection is up
		for(i = CursorStartAddy-1;i > 0;i--){
			found = 1;
			for(j = 0;j < datasize;j++){
				if(GetMemViewData(i+j) != data[j])found = 0;
			}
			if(found == 1){
				ChangeMemViewFocus(EditingMode,i, i+datasize-1);
				return;
			}
		}
		for(i = MaxSize-datasize;i > CursorStartAddy;i--){
			found = 1;
			for(j = 0;j < datasize;j++){
				if(GetMemViewData(i+j) != data[j])found = 0;
			}
			if(found == 1){
				ChangeMemViewFocus(EditingMode,i, i+datasize-1);
				return;
			}
		}
	}


	MessageBox(hMemView,"String Not Found","Error", MB_OK);
	return;
}


void OpenFindDialog(){
	if((!hMemView) || (hMemFind))return;
	hMemFind = CreateDialog(fceu_hInstance,"MEMVIEWFIND",hMemView,MemFindCallB);
	return;
}
