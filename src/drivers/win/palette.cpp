#include "../../version.h"
#include "common.h"
#include "../../palette.h"
#include "main.h"
#include "window.h"
#include "gui.h"

int cpalette_count = 0;
u8 cpalette[64*8*3] = {0};
extern int palnotch;
extern int palsaturation;
extern int palsharpness;
extern int palcontrast;
extern int palbrightness;
extern bool paldeemphswap;
HWND hWndPal = NULL;

bool SetPalette(const char* nameo)
{
	FILE *fp;
	if((fp = FCEUD_UTF8fopen(nameo, "rb")))
	{
		int readed = fread(cpalette, 1, sizeof(cpalette), fp);
		fclose(fp);
		cpalette_count = readed/3;
		FCEUI_SetUserPalette(cpalette,cpalette_count);
		eoptions |= EO_CPALETTE;
		return true;
	}
	else
	{
		FCEUD_PrintError("Error opening palette file!");
		return false;
	}
}

/**
* Prompts the user for a palette file and opens that file.
*
* @return Flag that indicates failure (0) or success (1)
**/
int LoadPaletteFile()
{
	const char filter[]="All usable files (*.pal)\0*.pal\0All Files (*.*)\0*.*\0\0";

	bool success = false;
	char nameo[2048];

	// Display open file dialog
	OPENFILENAME ofn;
	memset(&ofn, 0, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hInstance = fceu_hInstance;
	ofn.lpstrTitle = FCEU_NAME" Open Palette File...";
	ofn.lpstrFilter = filter;
	nameo[0] = 0;
	ofn.lpstrFile = nameo;
	ofn.nMaxFile = 256;
	ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
	ofn.lpstrInitialDir = 0;

	if(GetOpenFileName(&ofn))
	{
		success = SetPalette(nameo);
	}

	return success;
}
/**
* Notify the dialog to redraw the palette preview area
**/
void RedrawPalette(HWND hwnd)
{
	RECT rect;
	GetWindowRect(GetDlgItem(hwnd, IDC_PALETTE_PREVIEW), &rect);
	ScreenToClient(hwnd, (POINT*)&rect);
	ScreenToClient(hwnd, ((POINT*)&rect) + 1);
	InvalidateRect(hwnd, &rect, FALSE);
	UpdateWindow(hwnd);
}

/**
* Callback function for the palette configuration dialog.
**/
INT_PTR CALLBACK PaletteConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char text[40];
	switch(uMsg)
	{
		case WM_INITDIALOG:

			if(ntsccol_enable)
				CheckDlgButton(hwndDlg, CHECK_PALETTE_ENABLED, BST_CHECKED);

			if(paldeemphswap)
				CheckDlgButton(hwndDlg, CHECK_DEEMPH_SWAP, BST_CHECKED);

			if(force_grayscale)
				CheckDlgButton(hwndDlg, CHECK_PALETTE_GRAYSCALE, BST_CHECKED);

			if (eoptions & EO_CPALETTE)
				CheckDlgButton(hwndDlg, CHECK_PALETTE_CUSTOM, BST_CHECKED);

			SendDlgItemMessage(hwndDlg, CTL_TINT_TRACKBAR,       TBM_SETRANGE, 1, MAKELONG(0, 128));
			SendDlgItemMessage(hwndDlg, CTL_HUE_TRACKBAR,        TBM_SETRANGE, 1, MAKELONG(0, 128));
			SendDlgItemMessage(hwndDlg, CTL_PALNOTCH_TRACKBAR,   TBM_SETRANGE, 1, MAKELONG(0, 100));
			SendDlgItemMessage(hwndDlg, CTL_PALSAT_TRACKBAR,     TBM_SETRANGE, 1, MAKELONG(0, 200));
			SendDlgItemMessage(hwndDlg, CTL_PALSHARP_TRACKBAR,   TBM_SETRANGE, 1, MAKELONG(0,  50));
			SendDlgItemMessage(hwndDlg, CTL_PALCONTRAST_TRACKBAR,TBM_SETRANGE, 1, MAKELONG(0, 200));
			SendDlgItemMessage(hwndDlg, CTL_PALBRIGHT_TRACKBAR,  TBM_SETRANGE, 1, MAKELONG(0, 100));

			FCEUI_GetNTSCTH(&ntsctint, &ntschue);
			sprintf(text, "Notch: %d%%", palnotch);
			SendDlgItemMessage(hwndDlg, STATIC_NOTCHVALUE,   WM_SETTEXT, 0, (LPARAM) text);
			sprintf(text, "Saturation: %d%%", palsaturation);
			SendDlgItemMessage(hwndDlg, STATIC_SATVALUE,     WM_SETTEXT, 0, (LPARAM) text);
			sprintf(text, "Sharpness: %d%%", palsharpness);
			SendDlgItemMessage(hwndDlg, STATIC_SHARPVALUE,   WM_SETTEXT, 0, (LPARAM) text);
			sprintf(text, "Contrast: %d%%", palcontrast);
			SendDlgItemMessage(hwndDlg, STATIC_CONTRASTVALUE,WM_SETTEXT, 0, (LPARAM) text);
			sprintf(text, "Brightness: %d%%", palbrightness);
			SendDlgItemMessage(hwndDlg, STATIC_BRIGHTVALUE,  WM_SETTEXT, 0, (LPARAM) text);

			SendDlgItemMessage(hwndDlg, CTL_TINT_TRACKBAR,       TBM_SETPOS, 1, ntsctint);
			SendDlgItemMessage(hwndDlg, CTL_HUE_TRACKBAR,        TBM_SETPOS, 1, ntschue);
			SendDlgItemMessage(hwndDlg, CTL_PALNOTCH_TRACKBAR,   TBM_SETPOS, 1, palnotch);
			SendDlgItemMessage(hwndDlg, CTL_PALSAT_TRACKBAR,     TBM_SETPOS, 1, palsaturation);
			SendDlgItemMessage(hwndDlg, CTL_PALSHARP_TRACKBAR,   TBM_SETPOS, 1, palsharpness);
			SendDlgItemMessage(hwndDlg, CTL_PALCONTRAST_TRACKBAR,TBM_SETPOS, 1, palcontrast);
			SendDlgItemMessage(hwndDlg, CTL_PALBRIGHT_TRACKBAR,  TBM_SETPOS, 1, palbrightness);

			CenterWindowOnScreen(hwndDlg);

			EnableWindow(GetDlgItem(hwndDlg, 65463), ntsccol_enable);
			EnableWindow(GetDlgItem(hwndDlg, 64395), ntsccol_enable);
			EnableWindow(GetDlgItem(hwndDlg, CTL_HUE_TRACKBAR), ntsccol_enable);
			EnableWindow(GetDlgItem(hwndDlg, CTL_TINT_TRACKBAR), ntsccol_enable);
			break;
		case WM_HSCROLL:
			ntsctint      = SendDlgItemMessage(hwndDlg, CTL_TINT_TRACKBAR,       TBM_GETPOS, 0, (LPARAM)(LPSTR)0);
			ntschue       = SendDlgItemMessage(hwndDlg, CTL_HUE_TRACKBAR,        TBM_GETPOS, 0, (LPARAM)(LPSTR)0);
			palnotch      = SendDlgItemMessage(hwndDlg, CTL_PALNOTCH_TRACKBAR,   TBM_GETPOS, 0, (LPARAM)(LPSTR)0);
			palsaturation = SendDlgItemMessage(hwndDlg, CTL_PALSAT_TRACKBAR,     TBM_GETPOS, 0, (LPARAM)(LPSTR)0);
			palsharpness  = SendDlgItemMessage(hwndDlg, CTL_PALSHARP_TRACKBAR,   TBM_GETPOS, 0, (LPARAM)(LPSTR)0);
			palcontrast   = SendDlgItemMessage(hwndDlg, CTL_PALCONTRAST_TRACKBAR,TBM_GETPOS, 0, (LPARAM)(LPSTR)0);
			palbrightness = SendDlgItemMessage(hwndDlg, CTL_PALBRIGHT_TRACKBAR,  TBM_GETPOS, 0, (LPARAM)(LPSTR)0);
			FCEUI_SetNTSCTH(ntsccol_enable, ntsctint, ntschue);

			sprintf(text, "Notch: %d%%", palnotch);
			SendDlgItemMessage(hwndDlg, STATIC_NOTCHVALUE,   WM_SETTEXT, 0, (LPARAM) text);
			sprintf(text, "Saturation: %d%%", palsaturation);
			SendDlgItemMessage(hwndDlg, STATIC_SATVALUE,     WM_SETTEXT, 0, (LPARAM) text);
			sprintf(text, "Sharpness: %d%%", palsharpness);
			SendDlgItemMessage(hwndDlg, STATIC_SHARPVALUE,   WM_SETTEXT, 0, (LPARAM) text);
			sprintf(text, "Contrast: %d%%", palcontrast);
			SendDlgItemMessage(hwndDlg, STATIC_CONTRASTVALUE,WM_SETTEXT, 0, (LPARAM) text);
			sprintf(text, "Brightness: %d%%", palbrightness);
			SendDlgItemMessage(hwndDlg, STATIC_BRIGHTVALUE,  WM_SETTEXT, 0, (LPARAM) text);
			
			RedrawPalette(hwndDlg);
			break;
		case WM_PAINT:
		{
			if (!palo)
				break;

			RECT pvRect, wndRect;
			HWND pvHwnd = GetDlgItem(hwndDlg, IDC_PALETTE_PREVIEW);
			GetClientRect(pvHwnd, &pvRect);
			GetClientRect(hwndDlg, &wndRect);

			int col_width = (pvRect.right - pvRect.left) / 16;
			int col_height = (pvRect.bottom - pvRect.top) / 4;

			int width = col_width * 16;
			int height = col_height * 4;

			PAINTSTRUCT paint;
			HDC hdc = BeginPaint(hwndDlg, &paint);
			HDC memdc = CreateCompatibleDC(hdc);
			HBITMAP pvBitmap = CreateCompatibleBitmap(hdc, width, height);

			ClientToScreen(pvHwnd, (POINT*)&pvRect);
			ClientToScreen(pvHwnd, ((POINT*)&pvRect) + 1);
			ClientToScreen(hwndDlg, (POINT*)&wndRect);
			ClientToScreen(hwndDlg, ((POINT*)&wndRect) + 1);

			HGDIOBJ oldObj = SelectObject(memdc, pvBitmap);
			int i = 0;
			RECT rect = { 0, 0, col_width, col_height };
			while(i < 64)
			{
				COLORREF color;
				if (force_grayscale)
				{
					int gray = palo[i].r * 0.299 + palo[i].g * 0.587 + palo[i].b * 0.114;
					color = RGB(gray, gray, gray);
				}
				else
					color = RGB(palo[i].r, palo[i].g, palo[i].b);
				HBRUSH brush = CreateSolidBrush(color);
				FillRect(memdc, &rect, brush);
				DeleteObject(brush);
			
				if (++i % 16 == 0)
				{
					rect.left = 0;
					rect.right = col_width;
					rect.top += col_height;
					rect.bottom += col_height;
				}
				else
				{
					rect.left += col_width;
					rect.right += col_width;
				}
			}

			StretchBlt(hdc, pvRect.left - wndRect.left, pvRect.top - wndRect.top, pvRect.right - pvRect.left, pvRect.bottom - pvRect.top, memdc, 0, 0, width, height, SRCCOPY);

			EndPaint(hwndDlg, &paint);

			SelectObject(memdc, oldObj);
			DeleteDC(memdc);

			break;
		}
		case WM_CLOSE:
		case WM_QUIT:
			goto gornk;

		case WM_COMMAND:
			if(!(wParam>>16))
			{
				switch(wParam&0xFFFF)
				{
					case CHECK_PALETTE_ENABLED:
						ntsccol_enable ^= 1;
						FCEUI_SetNTSCTH(ntsccol_enable, ntsctint, ntschue); // it recalculates everything, use it for PAL block too!
						EnableWindow(GetDlgItem(hwndDlg, 65463), ntsccol_enable);
						EnableWindow(GetDlgItem(hwndDlg, 64395), ntsccol_enable);
						EnableWindow(GetDlgItem(hwndDlg, CTL_HUE_TRACKBAR), ntsccol_enable);
						EnableWindow(GetDlgItem(hwndDlg, CTL_TINT_TRACKBAR), ntsccol_enable);
						RedrawPalette(hwndDlg);
						break;

					case CHECK_PALETTE_GRAYSCALE:
						force_grayscale ^= 1;
						FCEUI_SetNTSCTH(ntsccol_enable, ntsctint, ntschue);
						RedrawPalette(hwndDlg);
						break;

					case CHECK_DEEMPH_SWAP:
						paldeemphswap ^= 1;
						FCEUI_SetNTSCTH(ntsccol_enable, ntsctint, ntschue);
						RedrawPalette(hwndDlg);
						break;

					case CHECK_PALETTE_CUSTOM:
					{
						if (eoptions & EO_CPALETTE)
						{
							//disable user palette
							FCEUI_SetUserPalette(0,0);
							eoptions &= ~EO_CPALETTE;
						} else
						{
							//switch to user palette (even if it isn't loaded yet!?)
							FCEUI_SetUserPalette(cpalette,64); //just have to guess the size I guess
							eoptions |= EO_CPALETTE;
						}
						RedrawPalette(hwndDlg);
						break;
					}

					case BTN_PALETTE_LOAD:
						if (LoadPaletteFile())
						{
							CheckDlgButton(hwndDlg, CHECK_PALETTE_CUSTOM, BST_CHECKED);
							RedrawPalette(hwndDlg);
						}
						break;

					case BTN_PALETTE_RESET:
						palnotch      = 90;
						palsaturation = 100;
						palsharpness  = 50;
						palcontrast   = 100;
						palbrightness = 50;

						sprintf(text, "Notch: %d%%", palnotch);
						SendDlgItemMessage(hwndDlg, STATIC_NOTCHVALUE, WM_SETTEXT, 0, (LPARAM) text);
						sprintf(text, "Saturation: %d%%", palsaturation);
						SendDlgItemMessage(hwndDlg, STATIC_SATVALUE, WM_SETTEXT, 0, (LPARAM) text);
						sprintf(text, "Sharpness: %d%%", palsharpness);
						SendDlgItemMessage(hwndDlg, STATIC_SHARPVALUE, WM_SETTEXT, 0, (LPARAM) text);
						sprintf(text, "Contrast: %d%%", palcontrast);
						SendDlgItemMessage(hwndDlg, STATIC_CONTRASTVALUE,WM_SETTEXT, 0, (LPARAM) text);
						sprintf(text, "Brightness: %d%%", palbrightness);
						SendDlgItemMessage(hwndDlg, STATIC_BRIGHTVALUE,  WM_SETTEXT, 0, (LPARAM) text);

						SendDlgItemMessage(hwndDlg, CTL_PALNOTCH_TRACKBAR,   TBM_SETPOS, 1, palnotch);
						SendDlgItemMessage(hwndDlg, CTL_PALSAT_TRACKBAR,     TBM_SETPOS, 1, palsaturation);
						SendDlgItemMessage(hwndDlg, CTL_PALSHARP_TRACKBAR,   TBM_SETPOS, 1, palsharpness);
						SendDlgItemMessage(hwndDlg, CTL_PALCONTRAST_TRACKBAR,TBM_SETPOS, 1, palcontrast);
						SendDlgItemMessage(hwndDlg, CTL_PALBRIGHT_TRACKBAR,  TBM_SETPOS, 1, palbrightness);

						FCEUI_SetNTSCTH(ntsccol_enable, ntsctint, ntschue);
						RedrawPalette(hwndDlg);
						break;

					case BUTTON_CLOSE:
gornk:
						DestroyWindow(hwndDlg);
						hWndPal = 0; // Yay for user race conditions.
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
	if(!hWndPal)
		hWndPal = CreateDialog(fceu_hInstance, "PALCONFIG", hAppWnd, PaletteConCallB);
	else
		SetFocus(hWndPal);
}

