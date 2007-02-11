#include "common.h"
#include "main.h"
#include "gui.h"

/**
* Processes information from the GUI options dialog after
* the dialog was closed.
*
* @param hwndDlg Handle of the dialog window.
**/
void CloseGuiDialog(HWND hwndDlg)
{
	if(IsDlgButtonChecked(hwndDlg, CB_LOAD_FILE_OPEN) == BST_CHECKED)
	{
		eoptions |= EO_FOAFTERSTART;
	}
	else
	{
		eoptions &= ~EO_FOAFTERSTART;
	}

	if(IsDlgButtonChecked(hwndDlg, CB_AUTO_HIDE_MENU) == BST_CHECKED)
	{
		eoptions |= EO_HIDEMENU;
	}
	else
	{
		eoptions &= ~EO_HIDEMENU;
	}

	goptions &= ~(GOO_CONFIRMEXIT | GOO_DISABLESS);

	if(IsDlgButtonChecked(hwndDlg, CB_ASK_EXIT)==BST_CHECKED)
	{
		goptions |= GOO_CONFIRMEXIT;
	}

	if(IsDlgButtonChecked(hwndDlg, CB_DISABLE_SCREEN_SAVER)==BST_CHECKED)
	{
		goptions |= GOO_DISABLESS;
	}

	EndDialog(hwndDlg,0);
}

/**
* Message loop of the GUI configuration dialog.
**/
BOOL CALLBACK GUIConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:

			if(eoptions & EO_FOAFTERSTART)
			{
				CheckDlgButton(hwndDlg, CB_LOAD_FILE_OPEN, BST_CHECKED);
			}

			if(eoptions&EO_HIDEMENU)
			{
				CheckDlgButton(hwndDlg, CB_AUTO_HIDE_MENU, BST_CHECKED);
			}

			if(goptions & GOO_CONFIRMEXIT)
			{
				CheckDlgButton(hwndDlg, CB_ASK_EXIT, BST_CHECKED);
			}

			if(goptions & GOO_DISABLESS)
			{
				CheckDlgButton(hwndDlg, CB_DISABLE_SCREEN_SAVER, BST_CHECKED);
			}

			CenterWindowOnScreen(hwndDlg);

			break;

		case WM_CLOSE:
		case WM_QUIT:
			CloseGuiDialog(hwndDlg);

		case WM_COMMAND:
			if(!(wParam >> 16))
			{
				switch(wParam & 0xFFFF)
				{
					case BUTTON_CLOSE:
						CloseGuiDialog(hwndDlg);
				}
			}
	}

	return 0;
}

/**
* Shows the GUI configuration dialog.
**/
void ConfigGUI()
{
	DialogBox(fceu_hInstance, "GUICONFIG", hAppWnd, GUIConCallB);  
}

