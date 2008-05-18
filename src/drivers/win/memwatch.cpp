/* FCE Ultra - NES/Famicom Emulator
*
* Copyright notice for this file:
*  Copyright (C) 2006 Luke Gustafson
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
#include "memwatch.h"
#include "..\..\debug.h"
#include "debugger.h"

static HDC hdc;
static HWND hwndMemWatch=0;
static char addresses[24][16];
static char labels[24][24];
static int NeedsInit = 1;
char *MemWatchDir = 0;
char memwLastFilename[2048];
bool fileChanged = false;
bool MemWatchLoadOnStart = false;  //If load memw on fceu start TODO: receive it from config file, if not in config, set to false
bool loadFileonStart = false;  //If load last file on memw start TODO: receive from config file, if not in config, set to false
static HMENU memwmenu = 0;
//char RecentMemwDirs[5][48];  //Recent directories

//mbg 5/12/08
//for the curious, I tested U16ToHexStr and it was 10x faster than printf.
//so the author of these dedicated functions is not insane, and I will leave them.

static char *U8ToStr(uint8 a)
{
	static char TempArray[8];
	TempArray[0] = '0' + a/100;
	TempArray[1] = '0' + (a%100)/10;
	TempArray[2] = '0' + (a%10);
	TempArray[3] = 0;
	return TempArray;
}


//I don't trust scanf for speed
static uint16 FastStrToU16(char* s, bool& valid)
{
	int i;
	uint16 v=0;
	for(i=0; i < 4; i++)
	{
		if(s[i] == 0) return v;
		v<<=4;
		if(s[i] >= '0' && s[i] <= '9')
		{
			v+=s[i]-'0';
		}
		else if(s[i] >= 'a' && s[i] <= 'f')
		{
			v+=s[i]-'a'+10;
		}
		else if(s[i] >= 'A' && s[i] <= 'F')
		{
			v+=s[i]-'A'+10;
		}
		else
		{
			valid = false;
			return 0xFFFF;
		}
	}
	valid = true;
	return v;
}

static char *U16ToDecStr(uint16 a)
{
	static char TempArray[8];
	TempArray[0] = '0' + a/10000;
	TempArray[1] = '0' + (a%10000)/1000;
	TempArray[2] = '0' + (a%1000)/100;
	TempArray[3] = '0' + (a%100)/10;
	TempArray[4] = '0' + (a%10);
	TempArray[5] = 0;
	return TempArray;
}


static char *U16ToHexStr(uint16 a)
{
	static char TempArray[8];
	TempArray[0] = a/4096 > 9?'A'+a/4096-10:'0' + a/4096;
	TempArray[1] = (a%4096)/256 > 9?'A'+(a%4096)/256 - 10:'0' + (a%4096)/256;
	TempArray[2] = (a%256)/16 > 9?'A'+(a%256)/16 - 10:'0' + (a%256)/16;
	TempArray[3] = a%16 > 9?'A'+(a%16) - 10:'0' + (a%16);
	TempArray[4] = 0;
	return TempArray;
}

static char *U8ToHexStr(uint8 a)
{
	static char TempArray[8];
	TempArray[0] = a/16 > 9?'A'+a/16 - 10:'0' + a/16;
	TempArray[1] = a%16 > 9?'A'+(a%16) - 10:'0' + (a%16);
	TempArray[2] = 0;
	return TempArray;
}


static const int MW_ADDR_Lookup[] = {
	MW_ADDR00,MW_ADDR01,MW_ADDR02,MW_ADDR03,
	MW_ADDR04,MW_ADDR05,MW_ADDR06,MW_ADDR07,
	MW_ADDR08,MW_ADDR09,MW_ADDR10,MW_ADDR11,
	MW_ADDR12,MW_ADDR13,MW_ADDR14,MW_ADDR15,
	MW_ADDR16,MW_ADDR17,MW_ADDR18,MW_ADDR19,
	MW_ADDR20,MW_ADDR21,MW_ADDR22,MW_ADDR23
};

static const int MWNUM = ARRAY_SIZE(MW_ADDR_Lookup);

static int yPositions[MWNUM];
static int xPositions[MWNUM];

static struct MWRec
{
	static int findIndex(WORD ctl)
	{
		for(int i=0;i<MWNUM;i++)
			if(MW_ADDR_Lookup[i] == ctl)
				return i;
		return -1;
	}

	void parse(WORD ctl)
	{
		char TempArray[16];
		GetDlgItemText(hwndMemWatch,ctl,TempArray,16);
		TempArray[15]=0;

		valid = hex = twobytes = false;
		switch(TempArray[0])
		{
			case 0:
				break;
			case '!':
				twobytes=true;
				addr=FastStrToU16(TempArray+1,valid);
				break;
			case 'x':
				hex = true;
				valid = true;
				addr=FastStrToU16(TempArray+1,valid);
				break;
			case 'X':
				hex = twobytes = true;
				valid = true;
				addr = FastStrToU16(TempArray+1,valid);
				break;
			default:
				valid = true;
				addr=FastStrToU16(TempArray,valid);
				break;
			}
	}
	bool valid, twobytes, hex;
	uint16 addr;
} mwrecs[MWNUM];

//Update the values in the Memory Watch window
void UpdateMemWatch()
{
	if(hwndMemWatch)
	{
		SetTextColor(hdc,RGB(0,0,0));
		SetBkColor(hdc,GetSysColor(COLOR_3DFACE));

		for(int i = 0; i < MWNUM; i++)
		{
			MWRec& mwrec = mwrecs[i];

			char* text;
			if(mwrec.valid && GameInfo)
			{
				if(mwrec.hex)
				{
					if(mwrec.twobytes)
					{
						text = U16ToHexStr(GetMem(mwrec.addr)+(GetMem(mwrec.addr+1)<<8));
					}
					else
					{
						text = U8ToHexStr(GetMem(mwrec.addr));
					}
				}
				else
				{
					if(mwrec.twobytes)
					{
						text = U16ToDecStr(GetMem(mwrec.addr)+(GetMem(mwrec.addr+1)<<8));
					}
					else
					{
						text = U8ToStr(GetMem(mwrec.addr));
					}
				}
			}
			else
			{
				text = "-";
			}

			MoveToEx(hdc,xPositions[i],yPositions[i],NULL);
			TextOut(hdc,0,0,text,strlen(text));
		}
	}
}

//Save labels/addresses so next time dialog is opened,
//you don't lose what you've entered.
static void SaveStrings()
{
	int i;
	for(i=0;i<24;i++)
	{
		GetDlgItemText(hwndMemWatch,1001+i*3,addresses[i],16);
		GetDlgItemText(hwndMemWatch,1000+i*3,labels[i],24);
	}
}

//replaces spaces with a dummy character
static void TakeOutSpaces(int i)
{
	int j;
	for(j=0;j<16;j++)
	{
		if(addresses[i][j] == ' ') addresses[i][j] = '|';
		if(labels[i][j] == ' ') labels[i][j] = '|';
	}
	for(;j<24;j++)
	{
		if(labels[i][j] == ' ') labels[i][j] = '|';
	}
}

//replaces dummy characters with spaces
static void PutInSpaces(int i)
{
	int j;
	for(j=0;j<16;j++)
	{
		if(addresses[i][j] == '|') addresses[i][j] = ' ';
		if(labels[i][j] == '|') labels[i][j] = ' ';
	}
	for(;j<24;j++)
	{
		if(labels[i][j] == '|') labels[i][j] = ' ';
	}
}


bool iftextchanged()
{
//Decides if any edit box has anything	
	int i,j;
	for(i=0;i<24;i++)
	{
		for(j=0;j<16;j++)
		{
			if(addresses[i][j] != NULL || labels [i][j] != NULL)
				return true;
		}
	}
return false;
}

//Saves all the addresses and labels to disk
static void SaveMemWatch()
{
	const char filter[]="Memory address list(*.txt)\0*.txt\0";
	 
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Save Memory Watch As...";
	ofn.lpstrFilter=filter;
	memwLastFilename[0]=0;
	ofn.lpstrFile=memwLastFilename;
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
	ofn.lpstrInitialDir=FCEU_GetPath(FCEUMKF_MEMW);
	if(GetSaveFileName(&ofn))
	{
		int i;

		//Save the directory
		if(ofn.nFileOffset < 1024)
		{
			free(MemWatchDir);
			MemWatchDir=(char*)malloc(strlen(ofn.lpstrFile)+1);
			strcpy(MemWatchDir,ofn.lpstrFile);
			MemWatchDir[ofn.nFileOffset]=0;
		}

		//quick get length of memwLastFilename
		for(i=0;i<2048;i++)
		{
			if(memwLastFilename[i] == 0)
			{
				break;
			}
		}

		//add .txt if memwLastFilename doesn't have it
		if((i < 4 || memwLastFilename[i-4] != '.') && i < 2040)
		{
			memwLastFilename[i] = '.';
			memwLastFilename[i+1] = 't';
			memwLastFilename[i+2] = 'x';
			memwLastFilename[i+3] = 't';
			memwLastFilename[i+4] = 0;
		}
		
		SaveStrings();
		FILE *fp=FCEUD_UTF8fopen(memwLastFilename,"w");
		for(i=0;i<24;i++)
		{
			//Use dummy strings to fill empty slots
			if(labels[i][0] == 0)
			{
				labels[i][0] = '|';
				labels[i][1] = 0;
			}
			if(addresses[i][0] == 0)
			{
				addresses[i][0] = '|';
				addresses[i][1] = 0;
			}
			//spaces can be a problem for scanf so get rid of them
			TakeOutSpaces(i);
			fprintf(fp, "%s %s\n", addresses[i], labels[i]);
			PutInSpaces(i);
		}
		fileChanged=false;
		fclose(fp);
	}
}

static void QuickSaveMemWatch() //Save rather than Save as
{
	if (fileChanged==false) //Checks if current watch has been changed, if so quick save has no effect.
		return;	
	if (memwLastFilename[0]!=NULL) // Check if there is there something to save else default to save as
	{
		SaveStrings();
		FILE *fp=FCEUD_UTF8fopen(memwLastFilename,"w");
		for(int i=0;i<24;i++)
		{
			//Use dummy strings to fill empty slots
			if(labels[i][0] == 0)
			{
				labels[i][0] = '|';
				labels[i][1] = 0;
			}
			if(addresses[i][0] == 0)
			{
				addresses[i][0] = '|';
				addresses[i][1] = 0;
			}
			//spaces can be a problem for scanf so get rid of them
			TakeOutSpaces(i);
			fprintf(fp, "%s %s\n", addresses[i], labels[i]);
			PutInSpaces(i);
			
		}
		fileChanged = false;
		fclose (fp);
	}
	else
		SaveMemWatch();
}

//Loads a previously saved file
static void LoadMemWatch()
{
	const char filter[]="Memory address list(*.txt)\0*.txt\0";
	char watchfcontents[2048]; 
	
	//Now clear last file used variable (memwLastFilename)
	//This might be unecessary
		for (int i=0;i<2048;i++)
		{
			memwLastFilename[i] = NULL;
		}
	
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Load Memory Watch...";
	ofn.lpstrFilter=filter;
	memwLastFilename[0]=0;
	ofn.lpstrFile=memwLastFilename;
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
	ofn.lpstrInitialDir=FCEU_GetPath(FCEUMKF_MEMW);

	if(GetOpenFileName(&ofn))
	{
		int i,j;

		//Save the directory
		if(ofn.nFileOffset < 1024)
		{
			free(MemWatchDir);
			MemWatchDir=(char*)malloc(strlen(ofn.lpstrFile)+1);
			strcpy(MemWatchDir,ofn.lpstrFile);
			MemWatchDir[ofn.nFileOffset]=0;
		}

		FILE *fp=FCEUD_UTF8fopen(memwLastFilename,"r");
		for(i=0;i<24;i++)
		{
			fscanf(fp, "%s ", watchfcontents); //Reads contents of newly opened file
			for(j = 0; j < 16; j++)
				addresses[i][j] = watchfcontents[j];
			fscanf(fp, "%s\n", watchfcontents);
			for(j = 0; j < 24; j++)
				labels[i][j] = watchfcontents[j];

			//Replace dummy strings with empty strings
			if(addresses[i][0] == '|')
			{
				addresses[i][0] = 0;
			}
			if(labels[i][0] == '|')
			{
				labels[i][0] = 0;
			}
			PutInSpaces(i);

			addresses[i][15] = 0;
			labels[i][23] = 0; //just in case

			SetDlgItemText(hwndMemWatch,1002+i*3,(LPTSTR) "---");
			SetDlgItemText(hwndMemWatch,1001+i*3,(LPTSTR) addresses[i]);
			SetDlgItemText(hwndMemWatch,1000+i*3,(LPTSTR) labels[i]);
		}
		fclose(fp);
	}
fileChanged = false;
}

void CloseMemoryWatch()
{
			SaveStrings();
			
			//TODO: save window x,y and last file opened to some variables then save them to config when fceu closes
			if (fileChanged==true)
			{
				if(MessageBox(hwndMemWatch, "Save Changes?", "Save", MB_YESNO)==IDYES)
					{
						SaveMemWatch();
					}
			}
	//Save MemwLastFile, window x,y, MemWatchLoadOnStart, LoadFileonStart, RecentMemwDirs		
	DestroyWindow(hwndMemWatch);
	hwndMemWatch=0;

}


void ClearAllText()
{
	if (fileChanged==true) //If contents have changed
		{
			if(MessageBox(hwndMemWatch, "Save Changes?", "Save", MB_YESNO)==IDYES)
			{
				SaveMemWatch();

			}
		}
					
					int i;
					for(i=0;i<24;i++)
					{
						addresses[i][0] = 0;
						labels[i][0] = 0;
						SetDlgItemText(hwndMemWatch,1001+i*3,(LPTSTR) addresses[i]);
						SetDlgItemText(hwndMemWatch,1000+i*3,(LPTSTR) labels[i]);
					}
					//Now clear last file used variable (memwLastFilename)
					for (int i=0;i<2048;i++)
					{
						memwLastFilename[i] = NULL;
					}
fileChanged = false;
				
}

static BOOL CALLBACK MemWatchCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	
	const int kLabelControls[] = {MW_ValueLabel1,MW_ValueLabel2};

	switch(uMsg)
	{
	case WM_INITDIALOG:
		hdc = GetDC(hwndDlg);
		SelectObject (hdc, debugSystem->hFixedFont);
		SetTextAlign(hdc,TA_UPDATECP | TA_TOP | TA_LEFT);

		//find the positions where we should draw string values
		for(int i=0;i<MWNUM;i++) {
			int col=0;
			if(i>=MWNUM/2)
				col=1;
			RECT r;
			GetWindowRect(GetDlgItem(hwndDlg,MW_ADDR_Lookup[i]),&r);
			ScreenToClient(hwndDlg,(LPPOINT)&r);
			ScreenToClient(hwndDlg,(LPPOINT)&r.right);
			yPositions[i] = r.top;
			yPositions[i] += ((r.bottom-r.top)-debugSystem->fixedFontHeight)/2; //vertically center
			GetWindowRect(GetDlgItem(hwndDlg,kLabelControls[col]),&r);
			ScreenToClient(hwndDlg,(LPPOINT)&r);
			xPositions[i] = r.left;
		}
		break;

	case WM_PAINT: {
		PAINTSTRUCT ps;
		BeginPaint(hwndDlg, &ps);
		EndPaint(hwndDlg, &ps);
		UpdateMemWatch();
		CheckMenuItem(memwmenu, MEMW_OPTIONS_LOADSTART, MemWatchLoadOnStart ? MF_CHECKED : MF_UNCHECKED);
		break;
	}
	case WM_CLOSE:
	case WM_QUIT:
		CloseMemoryWatch();
		//DeleteObject(hdc);
		break;
	case WM_COMMAND:
		
		//Menu Items
		switch(wParam)
		{	
		case MEMW_FILE_CLOSE:  
			CloseMemoryWatch();
			break;
								 
		case MEMW_FILE_OPEN:
			LoadMemWatch();
			break;
		
		case MEMW_FILE_SAVE:
			QuickSaveMemWatch();
			break;

		case MEMW_FILE_SAVEAS:
			SaveMemWatch();
			break;
		
		case MEMW_FILE_NEW:
			ClearAllText();
			break;
		
		case MEMW_FILE_RECENT:
			break;

		case MEMW_OPTIONS_LOADSTART: //Load on Start up
				MemWatchLoadOnStart ^= 1;
				CheckMenuItem(memwmenu, MEMW_OPTIONS_LOADSTART, MemWatchLoadOnStart ? MF_CHECKED : MF_UNCHECKED);
			break;

		case MEMW_OPTIONS_LOADLASTFILE: //Load last file when opening memwatch
			
			if (loadFileonStart==false)
				{
				CheckMenuItem(memwmenu,MEMW_OPTIONS_LOADLASTFILE,MF_CHECKED);
				loadFileonStart=true;
				}
			else
				{
				CheckMenuItem(memwmenu,MEMW_OPTIONS_LOADLASTFILE,MF_UNCHECKED);
				loadFileonStart=false;
				}
				
			break;

		case MEMW_HELP_WCOMMANDS:
			break;

		case MEMW_HELP_ABOUT:
			break;

		default:
		break;
		}

		switch(HIWORD(wParam))
		{
		
		case EN_CHANGE:
			{
				fileChanged = iftextchanged();			
				//the contents of an address box changed. re-parse it.
				//first, find which address changed
				int changed = MWRec::findIndex(LOWORD(wParam));
				if(changed==-1) break;
				mwrecs[changed].parse(LOWORD(wParam));
				break;
			}
		
		case BN_CLICKED:
			
			switch(LOWORD(wParam))
			{
			case 101: //Save button clicked
				//StopSound(); //mbg 5/7/08
				//SaveMemWatch();  //5/13/08 Buttons removed (I didn't remove the code so it would be easy to add them back one day)
				break;			
			case 102: //Load button clicked
				//StopSound(); //mbg 5/7/08
				//LoadMemWatch();  //5/13/08 Buttons removed
				break;
			case 103: //Clear button clicked
				//ClearAllText();  //5/13/08 Buttons removed
				break;
			default:
				break;
			} 
		}
		
		if(!(wParam>>16)) //Close button clicked
		{
			switch(wParam&0xFFFF)
			{
			case 1:
				//CloseMemoryWatch();  //5/13/08 Buttons removed
				break;
			}
		}
		
		break;
	default:
		break;
		
	}
	return 0;
}

//Open the Memory Watch dialog
void CreateMemWatch()
//void CreateMemWatch(HWND parent) - adelikat: old code made use of this but parent was only used in a line commented out.  Taking out for my own needs :P

{
	if(NeedsInit) //Clear the strings
	{
		NeedsInit = 0;
		int i,j;
		for(i=0;i<24;i++)
		{
			for(j=0;j<24;j++)
			{
				addresses[i][j] = 0;
				labels[i][j] = 0;
			}
		}
	}

	if(hwndMemWatch) //If already open, give focus
	{
		SetFocus(hwndMemWatch);
		return;
	}

	//Create
	//hwndMemWatch=CreateDialog(fceu_hInstance,"MEMWATCH",parent,MemWatchCallB);
	hwndMemWatch=CreateDialog(fceu_hInstance,"MEMWATCH",NULL,MemWatchCallB);
	memwmenu=GetMenu(hwndMemWatch);
	UpdateMemWatch();

	//Initialize values to previous entered addresses/labels
	
	{
		int i;
		for(i = 0; i < 24; i++)
		{
			SetDlgItemText(hwndMemWatch,1002+i*3,(LPTSTR) "---");
			SetDlgItemText(hwndMemWatch,1001+i*3,(LPTSTR) addresses[i]);
			SetDlgItemText(hwndMemWatch,1000+i*3,(LPTSTR) labels[i]);
		}
	}
}
void AddMemWatch(char memaddress[32])
{
 char TempArray[32];
	int i;
 CreateMemWatch();
	for(i = 0; i < 24; i++)
	{
	 GetDlgItemText(hwndMemWatch,1001+i*3,TempArray,32);
	 if (TempArray[0] == 0)
	 {
		 SetDlgItemText(hwndMemWatch,1001+i*3,memaddress);
		 break;
		}
	}
}

