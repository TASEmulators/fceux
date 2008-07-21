#include "common.h"
#include "../../fceu.h" //mbg merge 7/18/06 added
#include "basicbot2.h"
#include "../../input.h" // qfox: fceu_botmode() fceu_setbotmode()

// static variables and functions are only used in this file
static HWND hwndBasicBot = 0; // GUI handle

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
	}
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
			BotCloseWindow();
			break;
		}
		case WM_COMMAND:
		{
			switch(HIWORD(wParam))
			{
				case BN_CLICKED: // mouse click on item in gui
				{
					break;
				}
				case GUI_BOT_TEST:
				{
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

static void BotCloseWindow() {
	if (hwndBasicBot) {
		DestroyWindow(hwndBasicBot);
		hwndBasicBot = 0;
	}
}

