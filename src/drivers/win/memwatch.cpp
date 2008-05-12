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

static HWND hwndMemWatch=0;
static char addresses[24][16];
static char labels[24][24];
static int NeedsInit = 1;
char *MemWatchDir = 0;

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
		for(int i = 0; i < MWNUM; i++)
		{
			MWRec& mwrec = mwrecs[i];
			uint16& a = mwrec.addr;
			bool& hex = mwrec.hex;
			bool& twobytes = mwrec.twobytes;
			bool& valid = mwrec.valid;

			if(mwrec.valid)
			{
				if(mwrec.hex)
				{
					if(mwrec.twobytes)
					{
						SetDlgItemText(hwndMemWatch,1002+i*3,(LPTSTR)U8ToStr(GetMem(mwrec.addr)));
					}
					else
					{
						SetDlgItemText(hwndMemWatch,1002+i*3,(LPTSTR)U16ToDecStr(GetMem(mwrec.addr)+(GetMem(mwrec.addr+1)<<8)));
					}
				}
				else
				{
					if(mwrec.twobytes)
					{
						SetDlgItemText(hwndMemWatch,1002+i*3,(LPTSTR)U8ToHexStr(GetMem(mwrec.addr)));
					}
					else
					{
						SetDlgItemText(hwndMemWatch,1002+i*3,(LPTSTR)U16ToHexStr(GetMem(mwrec.addr)+(GetMem(mwrec.addr+1)<<8)));
					}
				}
			}
			else
			{
				SetDlgItemText(hwndMemWatch,1002+i*3,(LPTSTR)"---");
			}
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

//Saves all the addresses and labels to disk
static void SaveMemWatch()
{
	const char filter[]="Memory address list(*.txt)\0*.txt\0";
	char nameo[2048];
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Save Memory Watch As...";
	ofn.lpstrFilter=filter;
	nameo[0]=0;
	ofn.lpstrFile=nameo;
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
	ofn.lpstrInitialDir=MemWatchDir;
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

		//quick get length of nameo
		for(i=0;i<2048;i++)
		{
			if(nameo[i] == 0)
			{
				break;
			}
		}

		//add .txt if nameo doesn't have it
		if((i < 4 || nameo[i-4] != '.') && i < 2040)
		{
			nameo[i] = '.';
			nameo[i+1] = 't';
			nameo[i+2] = 'x';
			nameo[i+3] = 't';
			nameo[i+4] = 0;
		}

		SaveStrings();
		FILE *fp=FCEUD_UTF8fopen(nameo,"w");
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
		fclose(fp);
	}
}

//Loads a previously saved file
static void LoadMemWatch()
{
	const char filter[]="Memory address list(*.txt)\0*.txt\0";
	char nameo[2048];
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Load Memory Watch...";
	ofn.lpstrFilter=filter;
	nameo[0]=0;
	ofn.lpstrFile=nameo;
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
	ofn.lpstrInitialDir=MemWatchDir;

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

		FILE *fp=FCEUD_UTF8fopen(nameo,"r");
		for(i=0;i<24;i++)
		{
			fscanf(fp, "%s ", nameo); //re-using nameo--bady style :P
			for(j = 0; j < 16; j++)
				addresses[i][j] = nameo[j];
			fscanf(fp, "%s\n", nameo);
			for(j = 0; j < 24; j++)
				labels[i][j] = nameo[j];

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
}


static BOOL CALLBACK MemWatchCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	//mbg 5/7/08 - wtf?
	//DSMFix(uMsg);
	switch(uMsg)
	{
	case WM_INITDIALOG:
		break;
	case WM_CLOSE:
	case WM_QUIT:
		SaveStrings();
		DestroyWindow(hwndMemWatch);
		hwndMemWatch=0;
		break;
	case WM_COMMAND:
		switch(HIWORD(wParam))
		{
		
		case EN_CHANGE:
			{
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
				SaveMemWatch();
				break;			
			case 102: //Load button clicked
				//StopSound(); //mbg 5/7/08
				LoadMemWatch();
				break;
			case 103: //Clear button clicked
				//StopSound(); //mbg 5/7/08
				if(MessageBox(hwndMemWatch, "Clear all text?", "Confirm clear", MB_YESNO)==IDYES)
				{
					int i;
					for(i=0;i<24;i++)
					{
						addresses[i][0] = 0;
						labels[i][0] = 0;
						SetDlgItemText(hwndMemWatch,1001+i*3,(LPTSTR) addresses[i]);
						SetDlgItemText(hwndMemWatch,1000+i*3,(LPTSTR) labels[i]);
					}
				}
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
				SaveStrings();
				DestroyWindow(hwndMemWatch);
				hwndMemWatch=0;
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
void CreateMemWatch(HWND parent)
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

