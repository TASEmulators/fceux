//Implementation file of SCREENSHOT_DISPLAY class

#include "taseditproj.h"
#include "zlib.h"

extern HWND hwndTasEdit;
extern int TasEdit_wndx, TasEdit_wndy;
extern bool TASEdit_show_branch_screenshots;

extern BOOKMARKS bookmarks;
extern TASEDIT_LIST tasedit_list;

LRESULT CALLBACK ScrBmpWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

// resources
char szClassName[] = "ScrBmp";

SCREENSHOT_DISPLAY::SCREENSHOT_DISPLAY()
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

	// register ScrBmp window class
	wincl.hInstance = fceu_hInstance;
	wincl.lpszClassName = szClassName;
	wincl.lpfnWndProc = ScrBmpWndProc;
	wincl.style = CS_DBLCLKS;
	wincl.cbSize = sizeof(WNDCLASSEX);
	wincl.hIcon = 0;
	wincl.hIconSm = 0;
	wincl.hCursor = 0;
	wincl.lpszMenuName = 0;
	wincl.cbClsExtra = 0;
	wincl.cbWndExtra = 0;
	wincl.hbrBackground = 0;
	if(!RegisterClassEx(&wincl))
		FCEU_printf("Error registering SCR_DISPLAY window class\n");
	// create blendfunction
	blend.BlendOp = AC_SRC_OVER;
	blend.BlendFlags = 0;
	blend.AlphaFormat = 0;
	blend.SourceConstantAlpha = 255;
}

void SCREENSHOT_DISPLAY::init()
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
	HDC win_hdc = GetWindowDC(tasedit_list.hwndList);
	scr_bmp = CreateDIBSection(win_hdc, scr_bmi, DIB_RGB_COLORS, (void**)&scr_ptr, 0, 0);
	// calculate coordinates (relative to IDC_BOOKMARKS_BOX top-left corner)
	RECT temp_rect, parent_rect;
	GetWindowRect(hwndTasEdit, &parent_rect);
	GetWindowRect(GetDlgItem(hwndTasEdit, IDC_BOOKMARKS_BOX), &temp_rect);
	scr_bmp_x = temp_rect.left - SCREENSHOT_WIDTH - SCR_BMP_DX - parent_rect.left;
	scr_bmp_y = ((temp_rect.bottom + temp_rect.top - SCREENSHOT_HEIGHT) / 2) - parent_rect.top;
}
void SCREENSHOT_DISPLAY::free()
{
	reset();
	if (scr_bmp)
	{
		DeleteObject(scr_bmp);
		scr_bmp = 0;
	}
}
void SCREENSHOT_DISPLAY::reset()
{
	screenshot_currently_shown = ITEM_UNDER_MOUSE_NONE;
	next_update_time = scr_bmp_phase = 0;
	if (hwndScrBmp)
	{
		DestroyWindow(hwndScrBmp);
		hwndScrBmp = 0;
	}
}

void SCREENSHOT_DISPLAY::update()
{
	// once per 40 milliseconds update screenshot_bitmap alpha
	if (clock() > next_update_time)
	{
		next_update_time = clock() + DISPLAY_UPDATE_TICK;
		if (bookmarks.item_under_mouse >= 0 && bookmarks.item_under_mouse < TOTAL_BOOKMARKS && TASEdit_show_branch_screenshots)
		{
			if (!hwndScrBmp)
			{
				// create window
				hwndScrBmp = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT, szClassName, szClassName, WS_POPUP, TasEdit_wndx + scr_bmp_x, TasEdit_wndy + scr_bmp_y, SCREENSHOT_WIDTH, SCREENSHOT_HEIGHT, hwndTasEdit, NULL, fceu_hInstance, NULL);
				RedrawScreenshotBitmap();
				ShowWindow(hwndScrBmp, SW_SHOWNA);
			}
			// change screenshot_bitmap pic if needed
			if (bookmarks.item_under_mouse != screenshot_currently_shown)
			{
				if (bookmarks.bookmarks_array[bookmarks.item_under_mouse].not_empty)
					ChangeScreenshotBitmap();
			}
			if (scr_bmp_phase < SCR_BMP_PHASE_MAX)
			{
				scr_bmp_phase++;
				// update alpha
				int phase_alpha = scr_bmp_phase;
				if (phase_alpha > SCR_BMP_PHASE_ALPHA_MAX) phase_alpha = SCR_BMP_PHASE_ALPHA_MAX;
				SetLayeredWindowAttributes(hwndScrBmp, 0, (255 * phase_alpha) / SCR_BMP_PHASE_ALPHA_MAX, LWA_ALPHA);
				UpdateLayeredWindow(hwndScrBmp, 0, 0, 0, 0, 0, 0, &blend, ULW_ALPHA);
			}
		} else
		{
			// fade and finally hide screenshot
			if (scr_bmp_phase > 0)
				scr_bmp_phase--;
			if (scr_bmp_phase > 0)
			{
				if (hwndScrBmp)
				{
					// update alpha
					int phase_alpha = scr_bmp_phase;
					if (phase_alpha > SCR_BMP_PHASE_ALPHA_MAX) phase_alpha = SCR_BMP_PHASE_ALPHA_MAX;
					SetLayeredWindowAttributes(hwndScrBmp, 0, (255 * phase_alpha) / SCR_BMP_PHASE_ALPHA_MAX, LWA_ALPHA);
					UpdateLayeredWindow(hwndScrBmp, 0, 0, 0, 0, 0, 0, &blend, ULW_ALPHA);
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
			}
		}
	}
}

void SCREENSHOT_DISPLAY::ChangeScreenshotBitmap()
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
	screenshot_currently_shown = bookmarks.item_under_mouse;
	RedrawScreenshotBitmap();
}
void SCREENSHOT_DISPLAY::RedrawScreenshotBitmap()
{
	HBITMAP temp_bmp = (HBITMAP)SendMessage(scr_bmp_pic, STM_SETIMAGE, IMAGE_BITMAP, (LPARAM)scr_bmp);
	if (temp_bmp && temp_bmp != scr_bmp)
		DeleteObject(temp_bmp);
}

void SCREENSHOT_DISPLAY::ParentWindowMoved()
{
	if (hwndScrBmp)
		SetWindowPos(hwndScrBmp, 0, TasEdit_wndx + scr_bmp_x, TasEdit_wndy + scr_bmp_y, 0, 0, SWP_NOSIZE|SWP_NOZORDER|SWP_NOACTIVATE);
}
// ----------------------------------------------------------------------------------------
LRESULT APIENTRY ScrBmpWndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	extern SCREENSHOT_DISPLAY screenshot_display;
	switch(message)
	{
		case WM_CREATE:
		{
			// create static bitmap placeholder
			screenshot_display.scr_bmp_pic = CreateWindow(WC_STATIC, NULL, SS_BITMAP | WS_CHILD | WS_VISIBLE, 0, 0, 255, 255, hwnd, NULL, NULL, NULL);
			return 0;
		}
		default:
			return DefWindowProc(hwnd, message, wParam, lParam);
	}
}


