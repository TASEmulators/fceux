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

#include "basicbot.h"

static HWND hwndBasicBot=0;
static int BotRunning = 0;
static uint32 rand1=1, rand2=0x1337EA75, rand3=0x0BADF00D;
static char Formula[24][1024];
static int BotCounter[256];
static int EditPlayer = 1, BotFrame, BotLoopVar = 0;
static int BotAttempts, BotFrames, BotBestScore[4], NewAttempt;
static int CurrentAttempt[1024], BestAttempt[1024], AttemptPointer;
static int DoAddCounter = 1;
char *BasicBotDir = 0;

static void SeedRandom(uint32 seed)
{
	rand1 = seed;
	if(rand1 == 0) rand1 = 1; //I forget if this is necessary.
}

//Simulation-quality random numbers: taus88 algorithm
static unsigned int NextRandom(void)
{
	uint32 b;
	b = (((rand1 << 13) ^ rand1) >> 19);
	rand1 = (((rand1 & 4294967294U) << 12) ^ b);
	b = (((rand2 << 2) ^ rand2) >> 25);
	rand2 = (((rand2 & 4294967288U) << 4) ^ b);
	b = (((rand3 << 3) ^ rand3) >> 11);
	rand3 = (((rand3 & 4294967280U) << 17) ^ b);
	return (rand1 ^ rand2 ^ rand3);
}

//Should do something to notify user of an error...  eh
//void BotError()
//{
//
//}

//recursively evaluate the formula
//horribly hacky, not meant to be a real expression evaluator!
int EvaluateFormula(char * formula, char **nextposition)
{
	char *currentchar = formula;
	int value=0;
	int hexmode = 0;
	while(1)
	{
		switch(*currentchar)
		{
		case 0:
			*nextposition = currentchar;
			return value;
		case '(':
			value = EvaluateFormula(currentchar + 1, &currentchar);
			break;
		case ')':
		case ':':
			*nextposition = currentchar + 1;
			return value;
		case 'x':
			hexmode=1;
			currentchar++;
			break;
		case '+':
			return (value + EvaluateFormula(currentchar+1, nextposition));
		case '-':
			return (value - EvaluateFormula(currentchar+1, nextposition));
		case '*':
			return (value * EvaluateFormula(currentchar+1, nextposition));
		case '/':
			return (value / EvaluateFormula(currentchar+1, nextposition));
		case '%':
			return (value % EvaluateFormula(currentchar+1, nextposition));
		case '&':
			return (value & EvaluateFormula(currentchar+1, nextposition));
		case '|':
			return (value | EvaluateFormula(currentchar+1, nextposition));
		case '^':
			return (value ^ EvaluateFormula(currentchar+1, nextposition));
		case '=':
			return (value == EvaluateFormula(currentchar+1, nextposition) ? 1000: 0);
		case '>':
			if(*(currentchar + 1) == '=')
				return (value >= EvaluateFormula(currentchar+2, nextposition) ? 1000: 0);
			else
				return (value > EvaluateFormula(currentchar+1, nextposition) ? 1000: 0);
		case '<':
			if(*(currentchar + 1) == '=')
				return (value <= EvaluateFormula(currentchar+2, nextposition) ? 1000: 0);
			else
				return (value < EvaluateFormula(currentchar+1, nextposition) ? 1000: 0);
		case '!':
			if(*(currentchar + 1) == '=')
			{
				return (value != EvaluateFormula(currentchar+2, nextposition) ? 1000: 0);
			}
			else
			{
				//BotError();
				currentchar++;
			}
		case '?':
			{
				int temp,temp2=DoAddCounter;
				if(value)
				{
					temp = EvaluateFormula(currentchar+1,&currentchar);
					DoAddCounter = 0;
					EvaluateFormula(currentchar,&currentchar);
					DoAddCounter = temp2;
					value= temp;
				}
				else
				{
					DoAddCounter = 0;
					EvaluateFormula(currentchar+1,&currentchar);
					DoAddCounter = temp2;
					temp = EvaluateFormula(currentchar,&currentchar);
					value= temp;
				}
			}
		case ' ':
			currentchar++;
			break;
		case 'a':
			if(*(currentchar+1) == 't'
				&&*(currentchar+2) == 't'
				&&*(currentchar+3) == 'e'
				&&*(currentchar+4) == 'm'
				&&*(currentchar+5) == 'p'
				&&*(currentchar+6) == 't')
			{
				currentchar+=7;
				value = BotAttempts;
			}
			else if(*(currentchar+1) == 'd'
				&&*(currentchar+2) == 'd'
				&&*(currentchar+3) == 'c'
				&&*(currentchar+4) == 'o'
				&&*(currentchar+5) == 'u'
				&&*(currentchar+6) == 'n'
				&&*(currentchar+7) == 't'
				&&*(currentchar+8) == 'e'
				&&*(currentchar+9) == 'r'
				&&*(currentchar+10)== '(')
			{
				currentchar += 11;
				if(DoAddCounter)
				{
					value = (BotCounter[(EvaluateFormula(currentchar, &currentchar) & 0x7FFFFFFF) % 256] += value);
				}
			}
			else if(*(currentchar+1) == 'b'
				&&*(currentchar+2) == 's'
				&&*(currentchar+3) == '(')
			{
				currentchar += 4;
				value = EvaluateFormula(currentchar, &currentchar);
				if(value < 0) value = -value;
			}
			else if(*(currentchar+1) == 'c'
				&&*(currentchar+2) == '(')
			{
				currentchar += 3;
				if(DoAddCounter)
				{
					value = (BotCounter[(EvaluateFormula(currentchar, &currentchar) & 0x7FFFFFFF) % 256] += value);
				}
			}
			else
			{
				value = 1;
				currentchar++;
			}
			break;
		case 'b':
			if(*(currentchar+1) == 'u'
				&&*(currentchar+2) == 't'
				&&*(currentchar+3) == 't'
				&&*(currentchar+4) == 'o'
				&&*(currentchar+5) == 'n')
			{
				currentchar += 6;
				value &= BotInput[1];
			}
			else
			{
				value = 2;
				currentchar++;
			}
			break;
		case 'c':
			if(*(currentchar+1) == 'o'
				&& *(currentchar+2) == 'u'
				&& *(currentchar+3) == 'n'
				&& *(currentchar+4) == 't'
				&& *(currentchar+5) == 'e'
				&& *(currentchar+6) == 'r'
				&& *(currentchar+7) == '(')
			{
				currentchar += 8;
				value = BotCounter[(EvaluateFormula(currentchar, &currentchar) & 0x7FFFFFFF) % 256];
			}
			else if(*(currentchar+1) == '(')
			{
				currentchar += 2;
				value = BotCounter[(EvaluateFormula(currentchar, &currentchar) & 0x7FFFFFFF) % 256];
			}
			else
			{
				//BotError();
				currentchar++;
			}
			break;
		case 'd':
			if(*(currentchar+1) == 'o'
				&&*(currentchar+2) == 'w'
				&&*(currentchar+3) == 'n')
			{
				currentchar += 4;
				value = 32;
			}
			else
			{
				currentchar++;
			}
			break;
		case 'f':
			if(*(currentchar+1) == 'r'
				&& *(currentchar+2) == 'a'
				&& *(currentchar+3) == 'm'
				&& *(currentchar+4) == 'e')
			{
				currentchar+= 5;
				value = BotFrame;
			}
			else
			{
				//BotError();
				currentchar++;
			}
			break;
		case 'i':
			value = BotLoopVar;
			currentchar++;
			break;
		case 'l':
			if(*(currentchar+1) == 'e'
				&&*(currentchar+2) == 'f'
				&&*(currentchar+3) == 't')
			{
				currentchar += 4;
				value = 64;
			}
			else if(*(currentchar+1) == 'o'
				&&*(currentchar+2) == 'o'
				&&*(currentchar+3) == 'p'
				&&*(currentchar+4) == '(')
			{
				if(value > 0)
				{
					currentchar += 5;
					if(BotLoopVar == 0)
					{
						for(BotLoopVar = 1; BotLoopVar < value && BotLoopVar < 65536; BotLoopVar++)
						{
							EvaluateFormula(currentchar, nextposition);
						}
						BotLoopVar = 0;
						EvaluateFormula(currentchar, &currentchar);
					}
					else
					{
						//BotError();
					}
				}
				else
				{
					int temp = DoAddCounter;
					currentchar += 5;
					DoAddCounter = 0;
					EvaluateFormula(currentchar, &currentchar);
					DoAddCounter = temp;
				}
			}
			else
			{
				currentchar++;
			}
			break;
		case 'm':
			if(*(currentchar+1) == 'e'
				&&*(currentchar+2) == 'm'
				&&*(currentchar+3) == '(')
			{
				currentchar += 4;
				int temp = (EvaluateFormula(currentchar, &currentchar) & 0x7FFFFFFF) % 65536;
				value = ARead[temp](temp);
			}
			else
			{
				currentchar++;
			}
			break;
		case 'r':
			if(*(currentchar+1) == 'a'
				&&*(currentchar+2) == 'm'
				&&*(currentchar+3) == '(')
			{
				currentchar += 4;
				value = RAM[(EvaluateFormula(currentchar, &currentchar) & 0x7FFFFFFF) % 2048];
			}
			else if(*(currentchar+1) == 'i'
				&&*(currentchar+2) == 'g'
				&&*(currentchar+3) == 'h'
				&&*(currentchar+4) == 't')
			{
				currentchar += 5;
				value = 128;
			}
			else if(*(currentchar+1) == 'e'
				&&*(currentchar+2) == 's'
				&&*(currentchar+3) == 'e'
				&&*(currentchar+4) == 't'
				&&*(currentchar+5) == 'c'
				&&*(currentchar+6) == 'o'
				&&*(currentchar+7) == 'u'
				&&*(currentchar+8) == 'n'
				&&*(currentchar+9) == 't'
				&&*(currentchar+10) == 'e'
				&&*(currentchar+11) == 'r'
				&&*(currentchar+12) == '(')
			{
				currentchar += 13;
				if(DoAddCounter) BotCounter[(EvaluateFormula(currentchar, &currentchar) & 0x7FFFFFFF) % 256] = 0;
				value = 0;
			}
			else if(*(currentchar+1) == 'c'
				&&*(currentchar+2) == '(')
			{
				currentchar += 3;
				if(DoAddCounter) BotCounter[(EvaluateFormula(currentchar, &currentchar) & 0x7FFFFFFF) % 256] = 0;
				value = 0;
			}
			else
			{
				currentchar++;
			}
			break;
		case 's':
			if(*(currentchar+1) == 't'
				&&*(currentchar+2) == 'a'
				&&*(currentchar+3) == 'r'
				&&*(currentchar+4) == 't')
			{
				currentchar += 5;
				value = 8;
			}
			else if(*(currentchar+1) == 'e'
				&&*(currentchar+2) == 'l'
				&&*(currentchar+3) == 'e'
				&&*(currentchar+4) == 'c'
				&&*(currentchar+5) == 't')
			{
				currentchar += 6;
				value = 4;
			}
			else if(*(currentchar+1) == 'e'
				&&*(currentchar+2) == 't'
				&&*(currentchar+3) == 'c'
				&&*(currentchar+4) == 'o'
				&&*(currentchar+5) == 'u'
				&&*(currentchar+6) == 'n'
				&&*(currentchar+7) == 't'
				&&*(currentchar+8) == 'e'
				&&*(currentchar+9) == 'r'
				&&*(currentchar+10) == '(')
			{
				currentchar += 11;
				if(DoAddCounter) BotCounter[(EvaluateFormula(currentchar, &currentchar) & 0x7FFFFFFF) % 256] = value;
			}
			else
			{
				currentchar++;
			}
			break;
		case 'u':
			if(*(currentchar+1) == 'p')
			{
				currentchar += 2;
				value = 16;
			}
			else
			{
				currentchar++;
			}
			break;
		case '0':
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
			value *= hexmode?16:10;
			value += *currentchar - '0';
			currentchar++;
			break;
		case 'A':
		case 'B':
		case 'C':
		case 'D':
		case 'E':
		case 'F':
			if(hexmode)
			{
				value *= 16;
				value += *currentchar - 'A' + 10;
			}
			else
			{
			//	BotError();
			}
			currentchar++;
			break;
		default:
			currentchar++;
			break;
		}
	}
	return 0;
}

//Update BasicBot input
void UpdateBasicBot()
{
	if(hwndBasicBot && BotRunning)
	{
		int i,j,k;
		char *dummy = "dum";
		
		if(BotInput[0])
			return;
		
		//Create the bot output...
		BotInput[0] = 1;
		if(NewAttempt)
		{
			BotInput[1] = 65536;
			NewAttempt = 0;
			AttemptPointer = 0;
			BotFrame = 0;
			memset(BotCounter, 0, 1024);
		}
		else
		{
			//Check if goal is met
			if(BotFrame > 1020 || (int) (NextRandom() % 1000) < EvaluateFormula(Formula[20], &dummy))
			{
				int currentscore[4];
				int better = 0;

				NewAttempt = 1;

				//Check if better score
				for(i=0; i < 4; i++)
				{
					currentscore[i] = EvaluateFormula(Formula[16+i], &dummy);
				}
				for(i=0; i < 4; i++)
				{
					if(currentscore[i] > BotBestScore[i])
					{
						better = 1;
						break;
					}
					else if(currentscore[i] < BotBestScore[i])
					{
						break;
					}
				}
				//Update best
				if(better)
				{
					char tempstring[4096];
					char symbols[16] = "ABET^v<>ABET^v<>";
					int seenplayer2=0;
					for(i = 0; i < 4; i++)
					{
						BotBestScore[i] = currentscore[i];
					}
					for(i = 0; i < AttemptPointer; i++)
					{
						BestAttempt[i] = CurrentAttempt[i];
					}
					for(; i < 1024; i++)
					{
						BestAttempt[i] = -1;
					}
					sprintf(tempstring, "%d %d %d %d (%d attempt)", BotBestScore[0], BotBestScore[1], BotBestScore[2], BotBestScore[3], BotAttempts);
					SetDlgItemText(hwndBasicBot,1018,tempstring);
					memset(tempstring, 0, 4096);
					k = 0;

					//Make output as text
					for(i = 0; i < 1024 && BestAttempt[i] != -1 && k < 4095; i++)
					{
						tempstring[k] = '0' + ((i/10) % 10);
						k++;
						tempstring[k] = '0' + (i%10);
						k++;
						tempstring[k] = '.';
						k++;
						seenplayer2=0;
						for(j = 0; j < 16; j++)
						{
							if(BestAttempt[i] & (1 << j))
							{
								if(j >= 8 && seenplayer2==0)
								{
									seenplayer2=1;
									tempstring[k]=',';
									k++;
								}
								tempstring[k] = symbols[j];
								k++;
							}
						}
						tempstring[k] = ' ';
						k++;
					}
					SetDlgItemText(hwndBasicBot,1019,tempstring);
				}
			}
			else //Goal not met
			{
				//Generate 1 frame of output
				BotInput[1] = 0;
				
				for(i=0;i<16;i++)
				{
					if((int) (NextRandom() % 1000) < EvaluateFormula(Formula[i], &dummy))
					{
						BotInput[1] |= 1 << i;
					}
				}

				//Save what we've done
				if(AttemptPointer < 1024)
				{
					CurrentAttempt[AttemptPointer] = BotInput[1];
					AttemptPointer++;
				}
				
				//Run extra commands
				EvaluateFormula(Formula[21],&dummy);
				EvaluateFormula(Formula[22],&dummy);
			}
		}

		//Update statistics
		if(BotInput[1] == 65536)
		{
			BotAttempts++;
		}
		else
		{
			BotFrames++;
			BotFrame++;
		}
		if(BotFrames % 100 == 0)
		{
			SetDlgItemInt(hwndBasicBot,1037,BotAttempts,FALSE);
			SetDlgItemInt(hwndBasicBot,1017,BotFrames,FALSE);
		}
	}
}

static void SaveBasicBot()
{
	const char filter[]="Basic Bot(*.bot)\0*.bot\0";
	char nameo[2048];
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Save Basic Bot As...";
	ofn.lpstrFilter=filter;
	nameo[0]=0;
	ofn.lpstrFile=nameo;
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
	ofn.lpstrInitialDir=BasicBotDir;
	if(GetSaveFileName(&ofn))
	{
		int i,j;

		//Save the directory
		if(ofn.nFileOffset < 1024)
		{
			free(BasicBotDir);
			BasicBotDir=malloc(strlen(ofn.lpstrFile)+1);
			strcpy(BasicBotDir,ofn.lpstrFile);
			BasicBotDir[ofn.nFileOffset]=0;
		}

		//quick get length of nameo
		for(i=0;i<2048;i++)
		{
			if(nameo[i] == 0)
			{
				break;
			}
		}

		//add .bot if nameo doesn't have it
		if((i < 4 || nameo[i-4] != '.') && i < 2040)
		{
			nameo[i] = '.';
			nameo[i+1] = 'b';
			nameo[i+2] = 'o';
			nameo[i+3] = 't';
			nameo[i+4] = 0;
		}

		FILE *fp=FCEUD_UTF8fopen(nameo,"wb");
		fputc('b',fp);
		fputc('o',fp);
		fputc('t',fp);
		fputc(0,fp);
		for(i=0;i<23;i++)
		{
			for(j=0;j<1024;j++)
			{
				fputc(Formula[i][j],fp);
			}
		}
		fclose(fp);
	}
}

//Loads a previously saved file
static void LoadBasicBot()
{
	const char filter[]="Basic Bot (*.bot)\0*.bot\0";
	char nameo[2048];
	OPENFILENAME ofn;
	memset(&ofn,0,sizeof(ofn));
	ofn.lStructSize=sizeof(ofn);
	ofn.hInstance=fceu_hInstance;
	ofn.lpstrTitle="Load Basic Bot...";
	ofn.lpstrFilter=filter;
	nameo[0]=0;
	ofn.lpstrFile=nameo;
	ofn.nMaxFile=256;
	ofn.Flags=OFN_EXPLORER|OFN_FILEMUSTEXIST|OFN_HIDEREADONLY;
	ofn.lpstrInitialDir=BasicBotDir;

	if(GetOpenFileName(&ofn))
	{
		int i,j;
		
		//Save the directory
		if(ofn.nFileOffset < 1024)
		{
			free(BasicBotDir);
			BasicBotDir=malloc(strlen(ofn.lpstrFile)+1);
			strcpy(BasicBotDir,ofn.lpstrFile);
			BasicBotDir[ofn.nFileOffset]=0;
		}

		FILE *fp=FCEUD_UTF8fopen(nameo,"rb");
		if(fgetc(fp) != 'b'
			|| fgetc(fp) != 'o'
			|| fgetc(fp) != 't'
			|| fgetc(fp) != 0)
		{
			fclose(fp);
			return;
		}
		
		for(i=0;i<23;i++)
		{
			for(j=0;j<1024;j++)
			{
				Formula[i][j] = fgetc(fp);
				if(Formula[i][j] & 128)
				{
					Formula[i][j] = 0;
				}
			}
		}
		fclose(fp);
	}
}


static BOOL CALLBACK BasicBotCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int i;
	DSMFix(uMsg);
	switch(uMsg)
	{
	case WM_INITDIALOG:
		break;
	case WM_CLOSE:
	case WM_QUIT:
		for(i = 0; i < 8; i++)
		{
			GetDlgItemText(hwndBasicBot,1000+i,Formula[i + (EditPlayer?0:8)],1024);
		}
		for(i = 0; i < 4; i++)
		{
			GetDlgItemText(hwndBasicBot,1010+i,Formula[16+i],1024);
		}
		GetDlgItemText(hwndBasicBot, 1009, Formula[20],1024);
		GetDlgItemText(hwndBasicBot, 1020, Formula[21],1024);
		GetDlgItemText(hwndBasicBot, 1022, Formula[22],1024);
		DestroyWindow(hwndBasicBot);
		hwndBasicBot=0;
		break;
	case WM_COMMAND:
		switch(HIWORD(wParam))
		{
		case BN_CLICKED:
			switch(LOWORD(wParam))
			{
			case 1008: //change player button clicked
				EditPlayer ^= 1;
				SetDlgItemText(hwndBasicBot,1008,(LPTSTR)(EditPlayer?"Edit Player 2":"Edit Player 1"));
				SetDlgItemText(hwndBasicBot,2000,(LPTSTR)(EditPlayer?"Player 1":"Player 2"));
				for(i = 0; i < 8; i++)
				{
					GetDlgItemText(hwndBasicBot,1000+i,Formula[i + (EditPlayer?8:0)],1024);
					SetDlgItemText(hwndBasicBot,1000+i,Formula[i + (EditPlayer?0:8)]);
				}
				break;
			case 1014: //Save button clicked
				for(i = 0; i < 8; i++)
				{
					GetDlgItemText(hwndBasicBot,1000+i,Formula[i + (EditPlayer?0:8)],1024);
				}
				for(i = 0; i < 4; i++)
				{
					GetDlgItemText(hwndBasicBot,1010+i,Formula[16+i],1024);
				}
				GetDlgItemText(hwndBasicBot, 1009, Formula[20],1024);
				GetDlgItemText(hwndBasicBot, 1020, Formula[21],1024);
				GetDlgItemText(hwndBasicBot, 1022, Formula[22],1024);
				StopSound();
				SaveBasicBot();
				break;			
			case 1015: //Load button clicked
				StopSound();
				LoadBasicBot();
				for(i = 0; i < 8; i++)
				{
					SetDlgItemText(hwndBasicBot,1000+i,Formula[i + (EditPlayer?0:8)]);
				}
				for(i = 0; i < 4; i++)
				{
					SetDlgItemText(hwndBasicBot,1010+i,Formula[16+i]);
				}
				SetDlgItemText(hwndBasicBot, 1009, Formula[20]);
				SetDlgItemText(hwndBasicBot, 1020, Formula[21]);
				SetDlgItemText(hwndBasicBot, 1022, Formula[22]);
				break;
			case 1021: //Play best button clicked
				if(BotRunning == 0 && BotInput[0] == 0)
				{
					for(i = 0; i < 1022 && BestAttempt[i] != -1; i++)
					{
						BotInput[i+2] = BestAttempt[i];
					}
					BotInput[0] = i+1;
					BotInput[1] = 65536;
				}
				break;
			case 1016: //Run button clicked
				BotRunning ^= 1;
				if(BotRunning)
				{
					BotAttempts = BotFrames = 0;
					BotBestScore[0] = BotBestScore[1] = BotBestScore[2] = BotBestScore[3] = -999999999;
					NewAttempt = 1;
				}
				for(i = 0; i < 8; i++)
				{
					GetDlgItemText(hwndBasicBot,1000+i,Formula[i + (EditPlayer?0:8)],1024);
				}
				for(i = 0; i < 4; i++)
				{
					GetDlgItemText(hwndBasicBot,1010+i,Formula[16+i],1024);
				}
				GetDlgItemText(hwndBasicBot, 1009, Formula[20],1024);
				GetDlgItemText(hwndBasicBot, 1020, Formula[21],1024);
				GetDlgItemText(hwndBasicBot, 1022, Formula[22],1024);
				SetDlgItemText(hwndBasicBot,1016,(LPTSTR)(BotRunning?"Stop":"Run!"));
				break;
			case 1036: //Clear button clicked
				StopSound();
				if(MessageBox(hwndBasicBot, "Clear all text?", "Confirm clear", MB_YESNO)==IDYES)
				{
					memset(Formula,0, 24*1024);
					for(i = 0; i < 24; i++)
					{
						Formula[i][0] = '0';
					}
					Formula[20][0] = 'f';
					Formula[20][1] = 'r';
					Formula[20][2] = 'a';
					Formula[20][3] = 'm';
					Formula[20][4] = 'e';
					Formula[20][5] = '=';
					Formula[20][6] = '1';
					for(i = 0; i < 8; i++)
					{
						SetDlgItemText(hwndBasicBot,1000+i,Formula[i+(EditPlayer?0:8)]);
					}
					for(i = 0; i < 4; i++)
					{
						SetDlgItemText(hwndBasicBot,1010+i,Formula[16+i]);
					}
					SetDlgItemText(hwndBasicBot, 1009, Formula[20]);
					SetDlgItemText(hwndBasicBot, 1020, Formula[21]);
					SetDlgItemText(hwndBasicBot, 1022, Formula[22]);
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
				for(i = 0; i < 8; i++)
				{
					GetDlgItemText(hwndBasicBot,1000+i,Formula[i + (EditPlayer?0:8)],1024);
				}
				for(i = 0; i < 4; i++)
				{
					GetDlgItemText(hwndBasicBot,1010+i,Formula[16+i],1024);
				}
				GetDlgItemText(hwndBasicBot, 1009, Formula[20],1024);
				GetDlgItemText(hwndBasicBot, 1020, Formula[21],1024);
				GetDlgItemText(hwndBasicBot, 1022, Formula[22],1024);
				DestroyWindow(hwndBasicBot);
				hwndBasicBot=0;
				break;
			}
		}
		break;
	default:
		break;
	}
	return 0;
}

void CreateBasicBot()
{
	static int BotNeedsInit = 1;
	int i;
	if(BotNeedsInit) 
	{
		BotNeedsInit = 0;
		SeedRandom(rand());
		memset(Formula,0, 24*1024);
		for(i = 0; i < 24; i++)
		{
			Formula[i][0] = '0';
		}
		Formula[20][0] = 'f';
		Formula[20][1] = 'r';
		Formula[20][2] = 'a';
		Formula[20][3] = 'm';
		Formula[20][4] = 'e';
		Formula[20][5] = '=';
		Formula[20][6] = '1';
	}

	if(hwndBasicBot) //If already open, give focus
	{
		SetFocus(hwndBasicBot);
		return;
	}
	
	//Create
	hwndBasicBot=CreateDialog(fceu_hInstance,"BASICBOT",NULL,BasicBotCallB);
	EditPlayer = 1;
	BotRunning = 0;
	for(i = 0; i < 8; i++)
	{
		SetDlgItemText(hwndBasicBot,1000+i,Formula[i]);
	}
	for(i = 0; i < 4; i++)
	{
		SetDlgItemText(hwndBasicBot,1010+i,Formula[16+i]);
	}
	SetDlgItemText(hwndBasicBot, 1009, Formula[20]);
	SetDlgItemText(hwndBasicBot, 1020, Formula[21]);
	SetDlgItemText(hwndBasicBot, 1022, Formula[22]);
}

