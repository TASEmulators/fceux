//Implementation file of POPUP_DISPLAY class
#include "taseditor_project.h"
#include "zlib.h"

extern TASEDITOR_CONFIG taseditor_config;
extern TASEDITOR_WINDOW taseditor_window;
extern MARKERS current_markers;
extern BOOKMARKS bookmarks;
extern TASEDITOR_LIST list;

LRESULT CALLBACK ScrBmpWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT APIENTRY MarkerNoteTooltipWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

// resources
char szClassName[] = "ScrBmp";
char szClassName2[] = "MarketNoteTooltip";

POPUP_DISPLAY::POPUP_DISPLAY()
{
	// create BITMAPINFO
	scr_bmi = (LPBITMAPINFO)malloc(sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));	// 256 color in palette
	scr_bmi->bmiHeader.biSize = sizeof(scr_bmi->bmiHeader);
	scr_bmi->bmiHeader.biWidth = SCREENSHOT_WIDTH;
	scr_bmi->bmiHeader.biHeight = -SCREENSHOT_HEIGHT;		// negative value = top-down bmp
	scr_bmi->bmiHeader.biPlanes = 1;
	scr_bmi->bmiHeader.biBitCount = 8;
	scr_bmi->bmiHeader.biCompression = BI_RGB;
	scr_bmi->bmiHeader.biSizeImage = 0;

	// register MarketNoteTooltip window class
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
	if(!RegisterClassEx(&wincl1))
		FCEU_printf("Error registering SCREENSHOT_DISPLAY window class\n");

	// register ScrBmp window class
	wincl2.hInstance = fceu_hInstance;
	wincl2.lpszClassName = szClassName2;
	wincl2.lpfnWndProc = MarkerNoteTooltipWndProc;
	wincl2.style = CS_DBLCLKS;
	wincl2.cbSize = sizeof(WNDCLASSEX);
	wincl2.hIcon = 0;
	wincl2.hIconSm = 0;
	wincl2.hCursor = 0;
	wincl2.lpszMenuName = 0;
	wincl2.cbClsExtra = 0;
	wincl2.cbWndExtra = 0;
	wincl2.hbrBackground = 0;
	if(!RegisterClassEx(&wincl2))
		FCEU_printf("Error registering MARKER_NOTE_TOOLTIP window class\n");

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
	HDC win_hdc = GetWindowDC(list.hwndList);
	scr_bmp = CreateDIBSection(win_hdc, scr_bmi, DIB_RGB_COLORS, (void**)&scr_ptr, 0, 0);
	// calculate coordinates (relative to IDC_BOOKMARKS_BOX)
	RECT temp_rect, parent_rect;
	GetWindowRect(taseditor_window.hwndTasEditor, &parent_rect);
	GetWindowRect(GetDlgItem(taseditor_window.hwndTasEditor, IDC_BOOKMARKS_BOX), &temp_rect);
	scr_bmp_x = temp_rect.left - SCREENSHOT_WIDTH - SCR_BMP_DX - parent_rect.left;
	//scr_bmp_y = ((temp_rect.bottom + temp_rect.top - (SCREENSHOT_HEIGHT + SCR_BMP_TOOLTIP_GAP + MARKER_NOTE_TOOLTIP_HEIGHT)) / 2) - parent_rect.top;
	scr_bmp_y = (temp_rect.bottom - SCREENSHOT_HEIGHT) - parent_rect.top;
	tooltip_x = scr_bmp_x + SCREENSHOT_WIDTH - MARKER_NOTE_TOOLTIP_WIDTH;
	//tooltip_y = scr_bmp_y + SCREENSHOT_HEIGHT + SCR_BMP_TOOLTIP_GAP;
	tooltip_y = scr_bmp_y + SCREENSHOT_HEIGHT + SCR_BMP_TOOLTIP_GAP;
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
	if (hwndMarkerNoteTooltip)
	{
		DestroyWindow(hwndMarkerNoteTooltip);
		hwndMarkerNoteTooltip = 0;
	}
}

void POPUP_DISPLAY::update()
{
	// once per 40 milliseconds update screenshot_bitmap alpha
	if (clock() > next_update_time)
	{
		next_update_time = clock() + DISPLAY_UPDATE_TICK;
		if (bookmarks.item_under_mouse >= 0 && bookmarks.item_under_mouse < TOTAL_BOOKMARKS)
		{
			if (taseditor_config.show_branch_screenshots && !hwndScrBmp)
			{
				// create window
				hwndScrBmp = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT, szClassName, szClassName, WS_POPUP, taseditor_config.wndx + scr_bmp_x, taseditor_config.wndy + scr_bmp_y, SCREENSHOT_WIDTH, SCREENSHOT_HEIGHT, taseditor_window.hwndTasEditor, NULL, fceu_hInstance, NULL);
				RedrawScreenshotBitmap();
				ShowWindow(hwndScrBmp, SW_SHOWNA);
			}
			if (taseditor_config.show_branch_tooltips && !hwndMarkerNoteTooltip)
			{
				hwndMarkerNoteTooltip = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT, szClassName2, szClassName2, WS_POPUP, taseditor_config.wndx + tooltip_x, taseditor_config.wndy + tooltip_y, MARKER_NOTE_TOOLTIP_WIDTH, MARKER_NOTE_TOOLTIP_HEIGHT, taseditor_window.hwndTasEditor, NULL, fceu_hInstance, NULL);
				ChangeTooltipText();
				ShowWindow(hwndMarkerNoteTooltip, SW_SHOWNA);
			}
			// change screenshot_bitmap pic and tooltip text if needed
			if (screenshot_currently_shown != bookmarks.item_under_mouse)
			{
				if (taseditor_config.show_branch_screenshots)
					ChangeScreenshotBitmap();
				if (taseditor_config.show_branch_tooltips)
					ChangeTooltipText();
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
				if (hwndMarkerNoteTooltip)
				{
					SetLayeredWindowAttributes(hwndMarkerNoteTooltip, 0, (255 * phase_alpha) / SCR_BMP_PHASE_ALPHA_MAX, LWA_ALPHA);
					UpdateLayeredWindow(hwndMarkerNoteTooltip, 0, 0, 0, 0, 0, 0, &blend, ULW_ALPHA);
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
				if (phase_alpha > SCR_BMP_PHASE_ALPHA_MAX) phase_alpha = SCR_BMP_PHASE_ALPHA_MAX;
				if (hwndScrBmp)
				{
					SetLayeredWindowAttributes(hwndScrBmp, 0, (255 * phase_alpha) / SCR_BMP_PHASE_ALPHA_MAX, LWA_ALPHA);
					UpdateLayeredWindow(hwndScrBmp, 0, 0, 0, 0, 0, 0, &blend, ULW_ALPHA);
				}
				if (hwndMarkerNoteTooltip)
				{
					SetLayeredWindowAttributes(hwndMarkerNoteTooltip, 0, (255 * phase_alpha) / SCR_BMP_PHASE_ALPHA_MAX, LWA_ALPHA);
					UpdateLayeredWindow(hwndMarkerNoteTooltip, 0, 0, 0, 0, 0, 0, &blend, ULW_ALPHA);
				}
			} else
			{
				// destroy screenshot bitmap window
				scr_bmp_phase = 0;
				if (hwndScrBmp)
				{
					DestroyWindow(hwndScrBmp);
					hwndScrBmp = 0;
				}
				if (hwndMarkerNoteTooltip)
				{
					DestroyWindow(hwndMarkerNoteTooltip);
					hwndMarkerNoteTooltip = 0;
				}
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
void POPUP_DISPLAY::ChangeTooltipText()
{
	// retrieve info from the pointed bookmark's markers
	int frame = bookmarks.bookmarks_array[bookmarks.item_under_mouse].snapshot.jump_frame;
	int marker_id = bookmarks.bookmarks_array[bookmarks.item_under_mouse].snapshot.my_markers.GetMarkerUp(frame);
	char new_text[MAX_NOTE_LEN];
	strcpy(new_text, bookmarks.bookmarks_array[bookmarks.item_under_mouse].snapshot.my_markers.GetNote(marker_id).c_str());
	SetWindowText(marker_note_tooltip, new_text);
}

void POPUP_DISPLAY::ParentWindowMoved()
{
	if (hwndScrBmp)
		SetWindowPos(hwndScrBmp, 0, taseditor_config.wndx + scr_bmp_x, taseditor_config.wndy + scr_bmp_y, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
	if (hwndMarkerNoteTooltip)
		SetWindowPos(hwndMarkerNoteTooltip, 0, taseditor_config.wndx + tooltip_x, taseditor_config.wndy + tooltip_y, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
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
LRESULT APIENTRY MarkerNoteTooltipWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	extern POPUP_DISPLAY popup_display;
	switch(message)
	{
		case WM_CREATE:
		{
			// create static text field
			popup_display.marker_note_tooltip = CreateWindow(WC_STATIC, NULL, WS_CHILD | WS_VISIBLE | SS_CENTER | SS_SUNKEN, 1, 1, MARKER_NOTE_TOOLTIP_WIDTH - 2, MARKER_NOTE_TOOLTIP_HEIGHT - 2, hwnd, NULL, NULL, NULL);
			return 0;
		}
		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
	}
}


