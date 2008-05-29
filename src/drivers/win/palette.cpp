#include "common.h"
#include "main.h"
#include "window.h"
#include "gui.h"

uint8 cpalette[192];

/**
* Prompts the user for a palette file and opens that file.
*
* @return Flag that indicates failure (0) or success (1)
**/
int LoadPaletteFile()
{
	const char filter[]="All usable files(*.pal)\0*.pal\0All files (*.*)\0*.*\0";

	FILE *fp;
	char nameo[2048];

	// Display open file dialog
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hInstance = fceu_hInstance;
	ofn.lpstrTitle = "FCE Ultra Open Palette File...";
	ofn.lpstrFilter = filter;
	nameo[0] = 0;
	ofn.lpstrFile = nameo;
	ofn.nMaxFile = 256;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrInitialDir = 0;

	if(GetOpenFileName(&ofn))
	{
		if((fp = FCEUD_UTF8fopen(nameo, "rb")))
		{
			fread(cpalette, 1, 192, fp);
			fclose(fp);

			FCEUI_SetPaletteArray(cpalette);
			
			eoptions |= EO_CPALETTE;
			return 1;
		}
		else
		{
			FCEUD_PrintError("Error opening palette file!");
		}
	}

	return 0;
}

/**
* Callback function for the palette configuration dialog.
**/
BOOL CALLBACK PaletteConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch(uMsg)
	{
		case WM_INITDIALOG:

			if(ntsccol)
			{
				CheckDlgButton(hwndDlg, 100, BST_CHECKED);
			}

			SendDlgItemMessage(hwndDlg, 500, TBM_SETRANGE, 1, MAKELONG(0, 128));
			SendDlgItemMessage(hwndDlg, 501, TBM_SETRANGE, 1, MAKELONG(0, 128));

			FCEUI_GetNTSCTH(&ntsctint, &ntschue);

			SendDlgItemMessage(hwndDlg, 500, TBM_SETPOS, 1, ntsctint);
			SendDlgItemMessage(hwndDlg, 501, TBM_SETPOS, 1, ntschue);

			EnableWindow(GetDlgItem(hwndDlg, 201), (eoptions & EO_CPALETTE) ? 1 : 0);

			CenterWindowOnScreen(hwndDlg);

			break;

		case WM_HSCROLL:
			ntsctint = SendDlgItemMessage(hwndDlg, 500, TBM_GETPOS, 0, (LPARAM)(LPSTR)0);
			ntschue = SendDlgItemMessage(hwndDlg, 501, TBM_GETPOS, 0, (LPARAM)(LPSTR)0);
			FCEUI_SetNTSCTH(ntsccol, ntsctint, ntschue);
			break;

		case WM_CLOSE:
		case WM_QUIT:
			goto gornk;

		case WM_COMMAND:
			if(!(wParam>>16))
			{
				switch(wParam&0xFFFF)
				{
					case 100:
						ntsccol ^= 1;
						FCEUI_SetNTSCTH(ntsccol, ntsctint, ntschue);
						break;

					case 200:
						if(LoadPaletteFile())
						{
							EnableWindow(GetDlgItem(hwndDlg, 201), 1);
						}
						break;

					case 201:
						FCEUI_SetPaletteArray(0);
						eoptions &= ~EO_CPALETTE;
						EnableWindow(GetDlgItem(hwndDlg, 201), 0);
						break;

					case 1:
gornk:
						DestroyWindow(hwndDlg);
						pwindow = 0; // Yay for user race conditions.
						break;
				}
			}
	}

	return 0;
}

/**
* Shows palette configuration dialog.
**/
void ConfigPalette()
{
	if(!pwindow)
	{
		pwindow=CreateDialog(fceu_hInstance, "PALCONFIG" ,0 , PaletteConCallB);
	}
	else
	{
		SetFocus(pwindow);
	}
}

