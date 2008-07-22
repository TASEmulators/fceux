#include "common.h"
#include "../../fceu.h" // BotInput
#include "basicbot2.h"
#include "../../input.h" // fceu_botmode() fceu_setbotmode() BOTMODES
#include <time.h> // random seed

// Static variables and functions are only used in this file
static HWND hwndBasicBot = 0; // GUI handle
// GUI values
static char * inputStrings[16]; // from gui
static int inputNumbers[16]; // temp: the values
static char * romString;
static char * commentString;
static char * scoreString[3][2]; // score[n][title/value]

// put all the inputs into an array for easy iterative access
// (indices are synced with the order of BotInput[1] inputs)
static int inputs[] = {
	BOT_TF_A_1,
	BOT_TF_B_1,
	BOT_TF_SELECT_1,
	BOT_TF_START_1,
	BOT_TF_UP_1,
	BOT_TF_DOWN_1,
	BOT_TF_LEFT_1,
	BOT_TF_RIGHT_1,
	BOT_TF_A_2,
	BOT_TF_B_2,
	BOT_TF_SELECT_2,
	BOT_TF_START_2,
	BOT_TF_UP_2,
	BOT_TF_DOWN_2,
	BOT_TF_LEFT_2,
	BOT_TF_RIGHT_2
};

void BotCreateBasicBot() {
	if(hwndBasicBot) {
		// If already open, give focus
		SetFocus(hwndBasicBot);
	}
	else {
		// create window
		hwndBasicBot = CreateDialog(fceu_hInstance,"BASICBOT2",NULL,WindowCallback);
		// set title
		SetWindowText(hwndBasicBot,"anything?");
		// create menu
		HMENU hmenu = LoadMenu(fceu_hInstance,"BASICBOTMENU");
		// add menu to window
		SetMenu(hwndBasicBot, hmenu);
		// initialize the random generator
		srand( (unsigned)time( NULL ) );
		// set all inputs to 0
		GetAllInputs();
	}
	FCEU_SetBotMode(BOTMODE_NEWBOT);
}
static void BotCloseBasicBot() {
	if (hwndBasicBot) {
		DestroyWindow(hwndBasicBot);
		hwndBasicBot = 0;
	}
	FCEU_SetBotMode(BOTMODE_OFF);
}


static BOOL CALLBACK WindowCallback(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
		case WM_INITDIALOG:
		{
			// cant change gui controls in this event
			// seems to be a inherited "feature", for backwards
			// compatibility.
			SetWindowPos(
				hwndDlg,						// window handle
				0,								// z-index (ignored)
				0,0,							// x,y
				0,0,						// width,height (ignored, uses default)
				SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER	// flags
			);
			break;
		}
		case WM_MOVE:
		{
			break;
		};
		case WM_CLOSE:
		case WM_QUIT:
		{
			BotCloseBasicBot();
			break;
		}
		case WM_COMMAND:
		{
			switch(HIWORD(wParam))
			{
				case BN_CLICKED: // mouse click on item in gui
				{
					switch(LOWORD(wParam))
					{
						case BOT_BUTTON_UPDATE:
						{
							GetAllInputs();
							break;
						}
						case BOT_BUTTON_RUN:
						{
							break;
						}
						case BOT_BUTTON_TEST:
						{
							break;
						}
						case BOT_MENU_CLOSE:
						{
							BotCloseBasicBot();
							break;
						}
					}
					break;
				}
				default:
					break;
				break; // WM_COMMAND
			}
		}
		default:
			break;
	}
	return 0;
}

// Called from main emulator loop
void BasicBotGetInput() {
	if (FCEU_BotMode() != BOTMODE_NEWBOT) {
		return;
	}
	BotInput[0] = 1; // number of frames on the buffer (starts at BotInput[1])
	BotInput[1] = 0; // reset first (and only) frame
	for (int i=0;i<16;i++) {
		if ((int)(((double)rand()/(double)RAND_MAX)*1000) < inputNumbers[i]) {
			// Button flags:
			//	button - player 1 - player 2
			//	A		 	1			 9
			//	B		 	2			10
			//	select	 	3			11
			//	start	 	4			12
			//	up		 	5			13
			//	down	 	6			14
			//	left	 	7			15
			//	right	 	8			16
			//	The input code will read the value of BotInput[1]
			//	If flag 17 is set, it will load a savestate, else
			//	it takes this input and puts the lower byte in 1
			//	and the upper byte in 2.
			BotInput[1] |= 1 << i;
		}
	}
}


/**
 * Get the length of a textfield ("edit control") or textarea
 * It's more of a macro really.
 * Returns 0 when an error occurs, the length of current 
 * contents in any other case (check LastError in case of 0)
 */
static unsigned int FieldLength(HWND winhandle,int controlid) {
	HWND hwndItem = GetDlgItem(winhandle,controlid);
	return SendMessage(hwndItem,WM_GETTEXTLENGTH,0,0);
}

/**
 * wrapper functions
 **/
static char * GetText(int controlid) {
	unsigned int count = FieldLength(hwndBasicBot,controlid);
	char *t = new char[count+1];
	GetDlgItemTextA(hwndBasicBot,controlid,t,count+1);
	return t;
}
static UINT GetInt(int controlid) {
	BOOL * x = new BOOL();
	return GetDlgItemInt(hwndBasicBot,controlid, x, true);
}
static void SetInt(int controlid, int value) {
	SetDlgItemInt(hwndBasicBot, controlid, value, true);
}
static void SetText(int controlid, char * str) {
	SetDlgItemTextA(hwndBasicBot, controlid, str);
}

// get data from all the inputs from the gui and store it
static void GetAllInputs() {
	// for all 8 buttons, twice
	for (int i=0; i<16; ++i) {
		inputStrings[i] = GetText(inputs[i]);
		inputNumbers[i] = GetInt(inputs[i]);
		SetInt(inputs[i], inputNumbers[i]);
	}
	romString = GetText(BOT_TF_ROM);
	commentString = GetText(BOT_TF_COMMENT);
	scoreString[0][0] = GetText(BOT_TF_SCORE1_DESC);
	scoreString[0][1] = GetText(BOT_TF_SCORE1_VALUE);
	scoreString[1][0] = GetText(BOT_TF_SCORE2_DESC);
	scoreString[1][1] = GetText(BOT_TF_SCORE2_VALUE);
	scoreString[2][0] = GetText(BOT_TF_SCORE3_DESC);
	scoreString[2][1] = GetText(BOT_TF_SCORE3_VALUE);
}

