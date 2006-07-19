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
#include "../../fceu.h" //mbg merge 7/18/06 added
#include "memwatch.h"

static HWND hwndMemWatch=0;
static char addresses[24][16];
static char labels[24][24];
static int NeedsInit = 1;
char *MemWatchDir = 0;

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
static uint16 FastStrToU16(char* s)
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
			return 0xFFFF;
		}
	}
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

//sorry this is hard to read... but I don't trust printf for speed!
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

//Update the values in the Memory Watch window
void UpdateMemWatch()
{
	if(hwndMemWatch)
	{
		char TempArray[16];
		int i, twobytes, hex;
		uint16 a;
		for(i = 0; i < 24; i++)
		{
			GetDlgItemText(hwndMemWatch,1001+i*3,TempArray,16);
			twobytes=0;
			hex = 0;
			TempArray[15]=0;
			switch(TempArray[0])
			{
			case 0:
				SetDlgItemText(hwndMemWatch,1002+i*3,(LPTSTR)"---");
				continue;
			case '!':
				twobytes=1;
				a=FastStrToU16(TempArray+1);
				break;
			case 'x':
				hex = 1;
				a=FastStrToU16(TempArray+1);
				break;
			case 'X':
				hex = twobytes = 1;
				a = FastStrToU16(TempArray+1);
				break;
			default:
				a=FastStrToU16(TempArray);
				break;
			}
			if(a != 0xFFFF)
			{
				if(hex == 0)
				{
					if(twobytes == 0)
					{
						SetDlgItemText(hwndMemWatch,1002+i*3,(LPTSTR)U8ToStr(ARead[a](a)));
					}
					else
					{
						SetDlgItemText(hwndMemWatch,1002+i*3,(LPTSTR)U16ToDecStr(ARead[a](a)+256*ARead[a+1](a+1)));
					}
				}
				else
				{
					if(twobytes == 0)
					{
						SetDlgItemText(hwndMemWatch,1002+i*3,(LPTSTR)U8ToHexStr(ARead[a](a)));
					}
					else
					{
						SetDlgItemText(hwndMemWatch,1002+i*3,(LPTSTR)U16ToHexStr(ARead[a](a)+256*ARead[a+1](a+1)));
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
		int i; //mbg merge 7/18/06 removed unused decl for j

		//Save the directory
		if(ofn.nFileOffset < 1024)
		{
			free(MemWatchDir);
			MemWatchDir=(char*)malloc(strlen(ofn.lpstrFile)+1); //mbg merge 7/18/06 added cast
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
			MemWatchDir=(char*)malloc(strlen(ofn.lpstrFile)+1); //mbg merge 7/18/06 added cast
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
	DSMFix(uMsg);
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
		case BN_CLICKED:
			switch(LOWORD(wParam))
			{
			case 101: //Save button clicked
				StopSound();
				SaveMemWatch();
				break;			
			case 102: //Load button clicked
				StopSound();
				LoadMemWatch();
				break;
			case 103: //Clear button clicked
				StopSound();
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

