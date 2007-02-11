#include "common.h"
#include "main.h"
#include "window.h"

/**
* Processes information from the Directories selection dialog after
* the dialog was closed.
*
* @param hwndDlg Handle of the dialog window.
**/
void CloseDirectoriesDialog(HWND hwndDlg)
{
	int x;

	// Update the information from the screenshot naming checkbox
	if(IsDlgButtonChecked(hwndDlg, CHECK_SCREENSHOT_NAMES) == BST_CHECKED)
	{
		eoptions |= EO_SNAPNAME;
	}
	else
	{
		eoptions &= ~EO_SNAPNAME;
	}

	RemoveDirs();   // Remove empty directories.

	// Read the information from the edit fields and update the
	// necessary variables.
	for(x = 0; x < NUMBER_OF_DIRECTORIES; x++)
	{
		LONG len;
		len = SendDlgItemMessage(hwndDlg, EDIT_CHEATS + x, WM_GETTEXTLENGTH, 0, 0);

		if(len <= 0)
		{
			if(directory_names[x])
			{
				free(directory_names[x]);
			}

			directory_names[x] = 0;
			continue;
		}

		len++; // Add 1 for null character.

		if( !(directory_names[x] = (char*)malloc(len))) //mbg merge 7/17/06 added cast
		{
			continue;
		}

		if(!GetDlgItemText(hwndDlg, EDIT_CHEATS + x, directory_names[x], len))
		{
			free(directory_names[x]);
			directory_names[x]=0;
			continue;
		}

	}

	CreateDirs();   // Create needed directories.
	SetDirs();      // Set the directories in the core.

	EndDialog(hwndDlg, 0);
}

/**
* Callback function for the directories configuration dialog.
**/
static BOOL CALLBACK DirConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int x;

	switch(uMsg)
	{
		case WM_INITDIALOG:

			SetDlgItemText(hwndDlg,65508,"The settings in the \"Individual Directory Overrides\" group will override the settings in the \"Base Directory Override\" group.  To delete an override, delete the text from the text edit control.  Note that the directory the configuration file is in cannot be overridden");

			// Initialize the directories textboxes
			for(x = 0; x < NUMBER_OF_DIRECTORIES; x++)
			{
				SetDlgItemText(hwndDlg, EDIT_CHEATS + x, directory_names[x]);
			}

			// Check the screenshot naming checkbox if necessary
			if(eoptions & EO_SNAPNAME)
			{
				CheckDlgButton(hwndDlg, CHECK_SCREENSHOT_NAMES, BST_CHECKED);
			}
			break;

		case WM_CLOSE:
		case WM_QUIT:
			CloseDirectoriesDialog(hwndDlg);
			break;

		case WM_COMMAND:
			if( !(wParam >> 16) )
			{
				if( (wParam & 0xFFFF) >= BUTTON_CHEATS && (wParam & 0xFFFF) <= BUTTON_CHEATS + NUMBER_OF_DIRECTORIES)
				{
					// If a directory selection button was pressed, ask the
					// user for a directory.

					static char *helpert[6] = {
						"Cheats",
						"Miscellaneous",
						"Nonvolatile Game Data",
						"Save States",
						"Screen Snapshots",
						"Base Directory"
					};

					char name[MAX_PATH];

					if(BrowseForFolder(hwndDlg, helpert[ ( (wParam & 0xFFFF) - BUTTON_CHEATS)], name))
					{
						SetDlgItemText(hwndDlg, EDIT_CHEATS + ((wParam & 0xFFFF) - BUTTON_CHEATS), name);
					}
				}
				else switch(wParam & 0xFFFF)
				{
					case CLOSE_BUTTON:
						CloseDirectoriesDialog(hwndDlg);
						break;
				}
			}

	}

	return 0;
}

/**
* Shows the dialog for configuring the standard directories.
**/
void ConfigDirectories()
{
	DialogBox(fceu_hInstance, "DIRCONFIG", hAppWnd, DirConCallB);
}

