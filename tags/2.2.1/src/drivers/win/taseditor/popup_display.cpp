/* ---------------------------------------------------------------------------------
Implementation file of POPUP_DISPLAY class
Copyright (c) 2011-2012 AnS

(The MIT License)
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
------------------------------------------------------------------------------------
Popup display - Manager of popup windows
[Singleton]

* implements all operations with popup windows: initialization, redrawing, centering, screenshot decompression and conversion
* regularly inspects changes of Bookmarks Manager and shows/updates/hides popup windows
* on demand: updates contents of popup windows
* stores resources: coordinates and appearance of popup windows, timings of fade in/out
------------------------------------------------------------------------------------ */

#include "taseditor_project.h"
#include "zlib.h"

extern TASEDITOR_CONFIG taseditor_config;
extern TASEDITOR_WINDOW taseditor_window;
extern BOOKMARKS bookmarks;
extern BRANCHES branches;
extern PIANO_ROLL piano_roll;
extern MARKERS_MANAGER markers_manager;
extern PLAYBACK playback;

LRESULT CALLBACK ScrBmpWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY MarkerNoteDescrWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

// resources
char szClassName[] = "ScrBmp";
char szClassName2[] = "MarketNoteDescr";

POPUP_DISPLAY::POPUP_DISPLAY()
{
	hwndScrBmp = 0;
	hwndMarkerNoteDescr = 0;
	// create BITMAPINFO
	scr_bmi = (LPBITMAPINFO)malloc(sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));	// 256 color in palette
	scr_bmi->bmiHeader.biSize = sizeof(scr_bmi->bmiHeader);
	scr_bmi->bmiHeader.biWidth = SCREENSHOT_WIDTH;
	scr_bmi->bmiHeader.biHeight = -SCREENSHOT_HEIGHT;		// negative value = top-down bmp
	scr_bmi->bmiHeader.biPlanes = 1;
	scr_bmi->bmiHeader.biBitCount = 8;
	scr_bmi->bmiHeader.biCompression = BI_RGB;
	scr_bmi->bmiHeader.biSizeImage = 0;

	// register SCREENSHOT_DISPLAY window class
	wincl1.hInstance = fceu_hInstance;
	wincl1.lpszClassName = szClassName;
	wincl1.lpfnWndProc = ScrBmpWndProc;
	wincl1.style = CS_DBLCLKS;
	wincl1.cbSize = sizeof(WNDCLASSEX);
	wincl1.hIcon = 0;
	wincl1.hIconSm = 0;
	wincl1.hCursor = 0;
	wincl1.lpszMenuName = 0;
	wincl1.cbClsExtra = 0;
	wincl1.cbWndExtra = 0;
	wincl1.hbrBackground = 0;
	if (!RegisterClassEx(&wincl1))
		FCEU_printf("Error registering SCREENSHOT_DISPLAY window class\n");

	// register MARKER_NOTE_DESCRIPTION window class
	wincl2.hInstance = fceu_hInstance;
	wincl2.lpszClassName = szClassName2;
	wincl2.lpfnWndProc = MarkerNoteDescrWndProc;
	wincl2.style = CS_DBLCLKS;
	wincl2.cbSize = sizeof(WNDCLASSEX);
	wincl2.hIcon = 0;
	wincl2.hIconSm = 0;
	wincl2.hCursor = 0;
	wincl2.lpszMenuName = 0;
	wincl2.cbClsExtra = 0;
	wincl2.cbWndExtra = 0;
	wincl2.hbrBackground = 0;
	if (!RegisterClassEx(&wincl2))
		FCEU_printf("Error registering MARKER_NOTE_DESCRIPTION window class\n");

	// create blendfunction
	blend.BlendOp = AC_SRC_OVER;
	blend.BlendFlags = 0;
	blend.AlphaFormat = 0;
	blend.SourceConstantAlpha = 255;
}

void POPUP_DISPLAY::init()
{
	free();
	// fill scr_bmp palette with current palette colors
	extern PALETTEENTRY *color_palette;
	for (int i = 0; i < 256; ++i)
	{
		scr_bmi->bmiColors[i].rgbRed = color_palette[i].peRed;
		scr_bmi->bmiColors[i].rgbGreen = color_palette[i].peGreen;
		scr_bmi->bmiColors[i].rgbBlue = color_palette[i].peBlue;
	}
	HDC win_hdc = GetWindowDC(piano_roll.hwndList);
	scr_bmp = CreateDIBSection(win_hdc, scr_bmi, DIB_RGB_COLORS, (void**)&scr_ptr, 0, 0);
	// calculate coordinates of popup windows (relative to TAS Editor window)
	ParentWindowMoved();
}
void POPUP_DISPLAY::free()
{
	reset();
	if (scr_bmp)
	{
		DeleteObject(scr_bmp);
		scr_bmp = 0;
	}
}
void POPUP_DISPLAY::reset()
{
	screenshot_currently_shown = ITEM_UNDER_MOUSE_NONE;
	next_update_time = scr_bmp_phase = 0;
	if (hwndScrBmp)
	{
		DestroyWindow(hwndScrBmp);
		hwndScrBmp = 0;
	}
	if (hwndMarkerNoteDescr)
	{
		DestroyWindow(hwndMarkerNoteDescr);
		hwndMarkerNoteDescr = 0;
	}
}

void POPUP_DISPLAY::update()
{
	// once per 40 milliseconds update popup windows alpha
	if (clock() > next_update_time)
	{
		next_update_time = clock() + DISPLAY_UPDATE_TICK;
		if (branches.IsSafeToShowBranchesData() && bookmarks.item_under_mouse >= 0 && bookmarks.item_under_mouse < TOTAL_BOOKMARKS && bookmarks.bookmarks_array[bookmarks.item_under_mouse].not_empty)
		{
			if (taseditor_config.show_branch_screenshots && !hwndScrBmp)
			{
				// create window
				hwndScrBmp = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT, szClassName, szClassName, WS_POPUP, taseditor_config.wndx + scr_bmp_x, taseditor_config.wndy + scr_bmp_y, SCREENSHOT_WIDTH, SCREENSHOT_HEIGHT, taseditor_window.hwndTasEditor, NULL, fceu_hInstance, NULL);
				RedrawScreenshotBitmap();
				ShowWindow(hwndScrBmp, SW_SHOWNA);
			}
			if (taseditor_config.show_branch_descr && !hwndMarkerNoteDescr)
			{
				RECT wrect;
				GetWindowRect(playback.hwndPlaybackMarkerEdit, &wrect);
				descr_x = scr_bmp_x + (SCREENSHOT_WIDTH - (wrect.right - wrect.left)) / 2;
				hwndMarkerNoteDescr = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT, szClassName2, szClassName2, WS_POPUP, taseditor_config.wndx + descr_x, taseditor_config.wndy + descr_y, wrect.right - wrect.left, wrect.bottom - wrect.top, taseditor_window.hwndTasEditor, NULL, fceu_hInstance, NULL);
				ChangeDescrText();
				ShowWindow(hwndMarkerNoteDescr, SW_SHOWNA);
			}
			// change screenshot_bitmap pic and description text if needed
			if (screenshot_currently_shown != bookmarks.item_under_mouse)
			{
				if (taseditor_config.show_branch_screenshots)
					ChangeScreenshotBitmap();
				if (taseditor_config.show_branch_descr)
					ChangeDescrText();
				screenshot_currently_shown = bookmarks.item_under_mouse;
			}
			if (scr_bmp_phase < SCR_BMP_PHASE_MAX)
			{
				scr_bmp_phase++;
				// update alpha
				int phase_alpha = scr_bmp_phase;
				if (phase_alpha > SCR_BMP_PHASE_ALPHA_MAX) phase_alpha = SCR_BMP_PHASE_ALPHA_MAX;
				if (hwndScrBmp)
				{
					SetLayeredWindowAttributes(hwndScrBmp, 0, (255 * phase_alpha) / SCR_BMP_PHASE_ALPHA_MAX, LWA_ALPHA);
					UpdateLayeredWindow(hwndScrBmp, 0, 0, 0, 0, 0, 0, &blend, ULW_ALPHA);
				}
				if (hwndMarkerNoteDescr)
				{
					SetLayeredWindowAttributes(hwndMarkerNoteDescr, 0, (255 * phase_alpha) / SCR_BMP_PHASE_ALPHA_MAX, LWA_ALPHA);
					UpdateLayeredWindow(hwndMarkerNoteDescr, 0, 0, 0, 0, 0, 0, &blend, ULW_ALPHA);
				}
			}
		} else
		{
			// fade and finally hide screenshot
			if (scr_bmp_phase > 0)
				scr_bmp_phase--;
			if (scr_bmp_phase > 0)
			{
				// update alpha
				int phase_alpha = scr_bmp_phase;
				if (phase_alpha > SCR_BMP_PHASE_ALPHA_MAX)
					phase_alpha = SCR_BMP_PHASE_ALPHA_MAX;
				else if (phase_alpha < 0)
					phase_alpha = 0;
				if (hwndScrBmp)
				{
					SetLayeredWindowAttributes(hwndScrBmp, 0, (255 * phase_alpha) / SCR_BMP_PHASE_ALPHA_MAX, LWA_ALPHA);
					UpdateLayeredWindow(hwndScrBmp, 0, 0, 0, 0, 0, 0, &blend, ULW_ALPHA);
				}
				if (hwndMarkerNoteDescr)
				{
					SetLayeredWindowAttributes(hwndMarkerNoteDescr, 0, (255 * phase_alpha) / SCR_BMP_PHASE_ALPHA_MAX, LWA_ALPHA);
					UpdateLayeredWindow(hwndMarkerNoteDescr, 0, 0, 0, 0, 0, 0, &blend, ULW_ALPHA);
				}
			} else
			{
				// destroy popup windows
				scr_bmp_phase = 0;
				if (hwndScrBmp)
				{
					DestroyWindow(hwndScrBmp);
					hwndScrBmp = 0;
				}
				if (hwndMarkerNoteDescr)
				{
					DestroyWindow(hwndMarkerNoteDescr);
					hwndMarkerNoteDescr = 0;
				}
				// immediately redraw the window below those
				UpdateWindow(taseditor_window.hwndTasEditor);
			}
		}
	}
}

void POPUP_DISPLAY::ChangeScreenshotBitmap()
{
	// uncompress
	uLongf destlen = SCREENSHOT_SIZE;
	int e = uncompress(&scr_ptr[0], &destlen, &bookmarks.bookmarks_array[bookmarks.item_under_mouse].saved_screenshot[0], bookmarks.bookmarks_array[bookmarks.item_under_mouse].saved_screenshot.size());
	if (e != Z_OK && e != Z_BUF_ERROR)
	{
		// error decompressing
		FCEU_printf("Error decompressing screenshot %d\n", bookmarks.item_under_mouse);
		// at least fill bitmap with zeros
		memset(&scr_ptr[0], 0, SCREENSHOT_SIZE);
	}
	RedrawScreenshotBitmap();
}
void POPUP_DISPLAY::RedrawScreenshotBitmap()
{
	HBITMAP temp_bmp = (HBITMAP)SendMessage(scr_bmp_pic, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)scr_bmp);
	if (temp_bmp && temp_bmp != scr_bmp)
		DeleteObject(temp_bmp);
}
void POPUP_DISPLAY::ChangeDescrText()
{
	// retrieve info from the pointed bookmark's Markers
	int frame = bookmarks.bookmarks_array[bookmarks.item_under_mouse].snapshot.keyframe;
	int marker_id = markers_manager.GetMarkerUp(bookmarks.bookmarks_array[bookmarks.item_under_mouse].snapshot.markers, frame);
	char new_text[MAX_NOTE_LEN];
	strcpy(new_text, markers_manager.GetNote(bookmarks.bookmarks_array[bookmarks.item_under_mouse].snapshot.markers, marker_id).c_str());
	SetWindowText(marker_note_descr, new_text);
}

void POPUP_DISPLAY::ParentWindowMoved()
{
	// calculate new positions relative to IDC_BOOKMARKS_BOX
	RECT temp_rect, parent_rect;
	GetWindowRect(taseditor_window.hwndTasEditor, &parent_rect);
	GetWindowRect(GetDlgItem(taseditor_window.hwndTasEditor, IDC_BOOKMARKS_BOX), &temp_rect);
	scr_bmp_x = temp_rect.left - SCREENSHOT_WIDTH - SCR_BMP_DX - parent_rect.left;
	scr_bmp_y = (temp_rect.bottom - SCREENSHOT_HEIGHT) - parent_rect.top;
	RECT wrect;
	GetWindowRect(playback.hwndPlaybackMarkerEdit, &wrect);
	descr_x = scr_bmp_x + (SCREENSHOT_WIDTH - (wrect.right - wrect.left)) / 2;
	descr_y = scr_bmp_y + SCREENSHOT_HEIGHT + SCR_BMP_DESCR_GAP;
	// if popup windows are currently shown, update their positions
	if (hwndScrBmp)
		SetWindowPos(hwndScrBmp, 0, taseditor_config.wndx + scr_bmp_x, taseditor_config.wndy + scr_bmp_y, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
	if (hwndMarkerNoteDescr)
		SetWindowPos(hwndMarkerNoteDescr, 0, taseditor_config.wndx + descr_x, taseditor_config.wndy + descr_y, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
}
// ----------------------------------------------------------------------------------------
LRESULT APIENTRY ScrBmpWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	extern POPUP_DISPLAY popup_display;
	switch(message)
	{
		case WM_CREATE:
		{
			// create static bitmap placeholder
			popup_display.scr_bmp_pic = CreateWindow(WC_STATIC, NULL, SS_BITMAP | WS_CHILD | WS_VISIBLE, 0, 0, 255, 255, hwnd, NULL, NULL, NULL);
			return 0;
		}
		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
	}
}
LRESULT APIENTRY MarkerNoteDescrWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	extern POPUP_DISPLAY popup_display;
	switch(message)
	{
		case WM_CREATE:
		{
			// create static text field
			RECT wrect;
			GetWindowRect(playback.hwndPlaybackMarkerEdit, &wrect);
			popup_display.marker_note_descr = CreateWindow(WC_STATIC, NULL, WS_CHILD | WS_VISIBLE | SS_CENTER | SS_ENDELLIPSIS | SS_SUNKEN, 1, 1, wrect.right - wrect.left - 2, wrect.bottom - wrect.top - 2, hwnd, NULL, NULL, NULL);
			SendMessage(popup_display.marker_note_descr, WM_SETFONT, (WPARAM)piano_roll.hMarkersEditFont, 0);
			return 0;
		}
		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
	}
}


