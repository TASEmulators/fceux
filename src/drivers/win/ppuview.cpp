/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Ben Parnell
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "common.h"
#include "ppuview.h"
#include "../../debug.h"
#include "../../palette.h"
#include "../../fceu.h"
#include "../../cart.h"

HWND hPPUView;

extern uint8 *VPage[8];
extern uint8 PALRAM[0x20];
extern uint8 UPALRAM[3];

int PPUViewPosX, PPUViewPosY;
bool PPUView_maskUnusedGraphics = true;
bool PPUView_invertTheMask = false;
int PPUView_sprite16Mode = 0;

static uint8 pallast[32+3] = { 0 }; // palette cache for change comparison
static uint8 palcache[36] = { 0 }; //palette cache for drawing
uint8 chrcache0[0x1000], chrcache1[0x1000], logcache0[0x1000], logcache1[0x1000]; //cache CHR, fixes a refresh problem when right-clicking
uint8 *pattern0, *pattern1; //pattern table bitmap arrays
uint8 *ppuv_palette;
static int pindex0 = 0, pindex1 = 0;
int PPUViewScanline = 0, PPUViewer = 0;
int PPUViewSkip;
int PPUViewRefresh = 0;
int mouse_x, mouse_y;

#define PATTERNWIDTH          128
#define PATTERNHEIGHT         128
#define PATTERNBITWIDTH       PATTERNWIDTH*3
#define PATTERNDESTX_BASE     7
#define PATTERNDESTY_BASE     18
#define ZOOM                  2

#define PALETTEWIDTH          32*4*4
#define PALETTEHEIGHT         32*2
#define PALETTEBITWIDTH       PALETTEWIDTH*3
#define PALETTEDESTX_BASE     7
#define PALETTEDESTY_BASE     18

#define TBM_SETPOS            (WM_USER+5)
#define TBM_SETRANGE          (WM_USER+6)
#define TBM_GETPOS            (WM_USER)

int patternDestX = PATTERNDESTX_BASE;
int patternDestY = PATTERNDESTY_BASE;
int paletteDestX = PALETTEDESTX_BASE;
int paletteDestY = PALETTEDESTY_BASE;

BITMAPINFO bmInfo;
HDC pDC,TmpDC0,TmpDC1;
HBITMAP TmpBmp0,TmpBmp1;
HGDIOBJ TmpObj0,TmpObj1;

BITMAPINFO bmInfo2;
HDC TmpDC2,TmpDC3;
HBITMAP TmpBmp2,TmpBmp3;
HGDIOBJ TmpObj2,TmpObj3;


void PPUViewDoBlit()
{
	if (!hPPUView)
		return;
	if (PPUViewSkip < PPUViewRefresh)
	{
		PPUViewSkip++;
		return;
	}
	PPUViewSkip = 0;

	StretchBlt(pDC, patternDestX, patternDestY, PATTERNWIDTH * ZOOM, PATTERNHEIGHT * ZOOM, TmpDC0, 0, PATTERNHEIGHT - 1, PATTERNWIDTH, -PATTERNHEIGHT, SRCCOPY);
	StretchBlt(pDC, patternDestX + (PATTERNWIDTH * ZOOM) + 1, patternDestY, PATTERNWIDTH * ZOOM, PATTERNHEIGHT * ZOOM, TmpDC1, 0, PATTERNHEIGHT - 1, PATTERNWIDTH, -PATTERNHEIGHT, SRCCOPY);
	StretchBlt(pDC, paletteDestX, paletteDestY, PALETTEWIDTH, PALETTEHEIGHT, TmpDC2, 0, PALETTEHEIGHT - 1, PALETTEWIDTH, -PALETTEHEIGHT, SRCCOPY);
}

//---------CDLogger VROM
extern unsigned char *cdloggervdata;
extern unsigned int cdloggerVideoDataSize;

void DrawPatternTable(uint8 *bitmap, uint8 *table, uint8 *log, uint8 pal)
{
	int i,j,k,x,y,index=0;
	int p=0,tmp;
	uint8 chr0,chr1,logs,shift;
	uint8 *pbitmap = bitmap;

	pal <<= 2;
	for (i = 0; i < (16 >> PPUView_sprite16Mode); i++)		//Columns
	{
		for (j = 0; j < 16; j++)	//Rows
		{
			//-----------------------------------------------
			for (k = 0; k < (PPUView_sprite16Mode + 1); k++) {
				for (y = 0; y < 8; y++)
				{
					chr0 = table[index];
					chr1 = table[index + 8];
					logs = log[index] & log[index + 8];
					tmp = 7;
					shift=(PPUView_maskUnusedGraphics && debug_loggingCD && (((logs & 3) != 0) == PPUView_invertTheMask))?3:0;
					for (x = 0; x < 8; x++)
					{
						p  =  (chr0 >> tmp) & 1;
						p |= ((chr1 >> tmp) & 1) << 1;
						p = palcache[p | pal];
						tmp--;
						*(uint8*)(pbitmap++) = palo[p].b >> shift;
						*(uint8*)(pbitmap++) = palo[p].g >> shift;
						*(uint8*)(pbitmap++) = palo[p].r >> shift;
					}
					index++;
					pbitmap += (PATTERNBITWIDTH-24);
				}
				index+=8;
			}
			pbitmap -= ((PATTERNBITWIDTH<<(3+PPUView_sprite16Mode))-24);
			//------------------------------------------------
		}
		pbitmap += (PATTERNBITWIDTH*((8<<PPUView_sprite16Mode)-1));
	}
}

void FCEUD_UpdatePPUView(int scanline, int refreshchr)
{
	if(!PPUViewer) return;
	if(scanline != -1 && scanline != PPUViewScanline) return;

	int x,y,i;
	uint8 *pbitmap = ppuv_palette;

	if(!hPPUView) return;
	if(PPUViewSkip < PPUViewRefresh) return;

	if(refreshchr)
	{
		for (i = 0, x=0x1000; i < 0x1000; i++, x++)
		{
			chrcache0[i] = VPage[i>>10][i];
			chrcache1[i] = VPage[x>>10][x];
			if (debug_loggingCD) {
				if (cdloggerVideoDataSize)
				{
					int addr;
					addr = &VPage[i >> 10][i] - CHRptr[0];
					if ((addr >= 0) && (addr < (int)cdloggerVideoDataSize))
						logcache0[i] = cdloggervdata[addr];
					addr = &VPage[x >> 10][x] - CHRptr[0];
					if ((addr >= 0) && (addr < (int)cdloggerVideoDataSize))
						logcache1[i] = cdloggervdata[addr];
				} else {
					logcache0[i] = cdloggervdata[i];
					logcache1[i] = cdloggervdata[x];
				}
			}
		}
	}

	// update palette only if required
	if ((memcmp(pallast, PALRAM, 32) != 0) || (memcmp(pallast+32, UPALRAM, 3) != 0))
	{
		memcpy(pallast, PALRAM, 32);
		memcpy(pallast+32, UPALRAM, 3);

		// cache palette content
		memcpy(palcache,PALRAM,32);
		palcache[0x10] = palcache[0x00];
		palcache[0x04] = palcache[0x14] = UPALRAM[0];
		palcache[0x08] = palcache[0x18] = UPALRAM[1];
		palcache[0x0C] = palcache[0x1C] = UPALRAM[2];

		//draw palettes
		for (y = 0; y < PALETTEHEIGHT; y++)
		{
			for (x = 0; x < PALETTEWIDTH; x++)
			{
				i = (((y>>5)<<4)+(x>>5));
				*(uint8*)(pbitmap++) = palo[palcache[i]].b;
				*(uint8*)(pbitmap++) = palo[palcache[i]].g;
				*(uint8*)(pbitmap++) = palo[palcache[i]].r;
			}
		}

		//draw line seperators on palette
		pbitmap = (ppuv_palette+PALETTEBITWIDTH*31);
		for (x = 0; x < PALETTEWIDTH*2; x++)
		{
				*(uint8*)(pbitmap++) = 0;
				*(uint8*)(pbitmap++) = 0;
				*(uint8*)(pbitmap++) = 0;
		}
		pbitmap = (ppuv_palette-3);
		for (y = 0; y < 64*3; y++)
		{
			if (!(y%3)) pbitmap += (32*4*3);
			for (x = 0; x < 6; x++)
			{
				*(uint8*)(pbitmap++) = 0;
			}
			pbitmap += ((32*4*3)-6);
		}
	}

	DrawPatternTable(pattern0,chrcache0,logcache0,pindex0);
	DrawPatternTable(pattern1,chrcache1,logcache1,pindex1);

	//PPUViewDoBlit();
}

static void CalculateBitmapPositions(HWND hwndDlg)
{
	// calculate bitmaps positions relative to their groupboxes
	RECT rect;
	POINT pt;
	GetWindowRect(GetDlgItem(hwndDlg, GRP_PPUVIEW_TABLES), &rect);
	pt.x = rect.left;
	pt.y = rect.top;
	ScreenToClient(hwndDlg, &pt);
	patternDestX = pt.x + PATTERNDESTX_BASE;
	patternDestY = pt.y + PATTERNDESTY_BASE;
	GetWindowRect(GetDlgItem(hwndDlg, LBL_PPUVIEW_PALETTES), &rect);
	pt.x = rect.left;
	pt.y = rect.top;
	ScreenToClient(hwndDlg, &pt);
	paletteDestX = pt.x + PALETTEDESTX_BASE;
	paletteDestY = pt.y + PALETTEDESTY_BASE;
}

static BOOL CALLBACK EnsurePixelSizeEnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	const int shift = lParam;
	HWND chrbox = GetDlgItem(hPPUView, GRP_PPUVIEW_TABLES);
	HWND palbox = GetDlgItem(hPPUView, LBL_PPUVIEW_PALETTES);

	if (hwnd != chrbox && hwnd != palbox)
	{
		RECT rect;
		GetWindowRect(hwnd, &rect);
		ScreenToClient(hPPUView,(LPPOINT)&rect);
		ScreenToClient(hPPUView,((LPPOINT)&rect)+1);
		SetWindowPos(hwnd,0,rect.left,rect.top+shift,0,0,SWP_NOZORDER | SWP_NOSIZE);
	}
	return TRUE;
}

static void EnsurePixelSize()
{
	// DPI varies, so the pixel size of the window may be too small to fit the viewer.
	// This expands the window and moves its controls around if necessary.

	if (!hPPUView) return;
	HWND chrbox = GetDlgItem(hPPUView, GRP_PPUVIEW_TABLES);
	HWND palbox = GetDlgItem(hPPUView, LBL_PPUVIEW_PALETTES);
	HWND chrlab = GetDlgItem(hPPUView, LBL_PPUVIEW_TILE1);

	RECT crect, prect, lrect;
	GetWindowRect(chrbox,&crect);
	GetWindowRect(palbox,&prect);
	GetWindowRect(chrlab,&lrect);

	const int MIN_CHR_W = (PATTERNWIDTH * ZOOM * 2) + 1 + (PATTERNDESTX_BASE * 2);
	const int MIN_CHR_H = (PATTERNHEIGHT * ZOOM)        + (PATTERNDESTY_BASE + 8);
	const int MIN_PAL_W = PALETTEWIDTH                  + (PALETTEDESTX_BASE * 2);
	const int MIN_PAL_H = PALETTEHEIGHT                 + (PALETTEDESTY_BASE + 8);

	const int chr_w = crect.right - crect.left;
	const int chr_h = lrect.top - crect.top; // measure CHR height against "Tile:" label
	const int pal_w = prect.right - prect.left;
	const int pal_h = prect.bottom - prect.top;

	int chr_w_add = 0;
	int chr_h_add = 0;
	int pal_w_add = 0;
	int pal_h_add = 0;

	if (chr_w < MIN_CHR_W) chr_w_add = MIN_CHR_W - chr_w;
	if (chr_h < MIN_CHR_H) chr_h_add = MIN_CHR_H - chr_h;
	if (pal_w < MIN_PAL_W) pal_w_add = MIN_PAL_W - pal_w;
	if (pal_h < MIN_PAL_H) pal_h_add = MIN_PAL_H - pal_h;

	const int all_w_add = (pal_w_add > chr_w_add) ? pal_w_add : chr_w_add;
	const int all_h_add = chr_h_add + pal_h_add;

	if (all_h_add <= 0 && all_w_add <= 0) return;

	// expand parent window
	RECT wrect;
	GetWindowRect(hPPUView,&wrect);
	int ww = (wrect.right - wrect.left) + all_w_add;
	int wh = (wrect.bottom - wrect.top) + all_h_add;
	SetWindowPos(hPPUView,0,0,0,ww,wh,SWP_NOZORDER | SWP_NOMOVE);

	if (all_w_add > 0 || chr_h_add > 0)
	{
		// expand CHR box
		SetWindowPos(chrbox,0,0,0,
			chr_w + all_w_add,
			(crect.bottom - crect.top) + chr_h_add,
			SWP_NOZORDER | SWP_NOMOVE);
	}

	if (all_w_add > 0 || pal_h_add > 0)
	{
		// expand and move palette box
		ScreenToClient(hPPUView,(LPPOINT)&prect);
		ScreenToClient(hPPUView,(LPPOINT)&prect+1);
		SetWindowPos(palbox,0,
			prect.left,
			prect.top + chr_h_add,
			pal_w + all_w_add,
			pal_h + pal_h_add,
			SWP_NOZORDER);
	}

	// move children
	if (chr_h_add > 0)
	{
		EnumChildWindows(hPPUView, EnsurePixelSizeEnumWindowsProc, chr_h_add);
	}

	CalculateBitmapPositions(hPPUView);
	RedrawWindow(hPPUView,0,0,RDW_ERASE | RDW_INVALIDATE);
}

void KillPPUView()
{
	//GDI cleanup
	DeleteObject(TmpBmp0);
	SelectObject(TmpDC0,TmpObj0);
	DeleteDC(TmpDC0);
	DeleteObject(TmpBmp1);
	SelectObject(TmpDC1,TmpObj1);
	DeleteDC(TmpDC1);
	DeleteObject(TmpBmp2);
	SelectObject(TmpDC2,TmpObj2);
	DeleteDC(TmpDC2);
	ReleaseDC(hPPUView,pDC);

	DestroyWindow(hPPUView);
	hPPUView=NULL;
	PPUViewer=0;
	PPUViewSkip=0;
}

INT_PTR CALLBACK PPUViewCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	RECT wrect;
	char str[20];

	switch(uMsg)
	{
		case WM_INITDIALOG:
		{
			if (PPUViewPosX==-32000) PPUViewPosX=0; //Just in case
			if (PPUViewPosY==-32000) PPUViewPosY=0;
			SetWindowPos(hwndDlg,0,PPUViewPosX,PPUViewPosY,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER);

			CalculateBitmapPositions(hwndDlg);

			//prepare the bitmap attributes
			//pattern tables
			memset(&bmInfo.bmiHeader,0,sizeof(BITMAPINFOHEADER));
			bmInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmInfo.bmiHeader.biWidth = PATTERNWIDTH;
			bmInfo.bmiHeader.biHeight = PATTERNHEIGHT;
			bmInfo.bmiHeader.biPlanes = 1;
			bmInfo.bmiHeader.biBitCount = 24;

			//palettes
			memset(&bmInfo2.bmiHeader,0,sizeof(BITMAPINFOHEADER));
			bmInfo2.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmInfo2.bmiHeader.biWidth = PALETTEWIDTH;
			bmInfo2.bmiHeader.biHeight = PALETTEHEIGHT;
			bmInfo2.bmiHeader.biPlanes = 1;
			bmInfo2.bmiHeader.biBitCount = 24;

			//create memory dcs
			pDC = GetDC(hwndDlg); // GetDC(GetDlgItem(hwndDlg,GRP_PPUVIEW_TABLES));
			TmpDC0 = CreateCompatibleDC(pDC); //pattern table 0
			TmpDC1 = CreateCompatibleDC(pDC); //pattern table 1
			TmpDC2 = CreateCompatibleDC(pDC); //palettes

			//create bitmaps and select them into the memory dc's
			TmpBmp0 = CreateDIBSection(pDC,&bmInfo,DIB_RGB_COLORS,(void**)&pattern0,0,0);
			TmpObj0 = SelectObject(TmpDC0,TmpBmp0);
			TmpBmp1 = CreateDIBSection(pDC,&bmInfo,DIB_RGB_COLORS,(void**)&pattern1,0,0);
			TmpObj1 = SelectObject(TmpDC1,TmpBmp1);
			TmpBmp2 = CreateDIBSection(pDC,&bmInfo2,DIB_RGB_COLORS,(void**)&ppuv_palette,0,0);
			TmpObj2 = SelectObject(TmpDC2,TmpBmp2);

			//Refresh Trackbar
			SendDlgItemMessage(hwndDlg,CTL_PPUVIEW_TRACKBAR,TBM_SETRANGE,0,(LPARAM)MAKELONG(0,25));
			SendDlgItemMessage(hwndDlg,CTL_PPUVIEW_TRACKBAR,TBM_SETPOS,1,PPUViewRefresh);

			CheckDlgButton(hwndDlg, IDC_MASK_UNUSED_GRAPHICS, PPUView_maskUnusedGraphics ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_INVERT_THE_MASK, PPUView_invertTheMask ? BST_CHECKED : BST_UNCHECKED);

			CheckDlgButton(hwndDlg, IDC_SPRITE16_MODE, PPUView_sprite16Mode ? BST_CHECKED : BST_UNCHECKED);

			EnableWindow(GetDlgItem(hwndDlg, IDC_INVERT_THE_MASK), PPUView_maskUnusedGraphics ?  true : false);

			//Set Text Limit
			SendDlgItemMessage(hwndDlg,IDC_PPUVIEW_SCANLINE,EM_SETLIMITTEXT,3,0);

			//force redraw the first time the PPU Viewer is opened
			PPUViewSkip=100;

			//clear cache
			memset(pallast,0,32+3);
			memset(palcache,0,36);
			memset(chrcache0,0,0x1000);
			memset(chrcache1,0,0x1000);
			memset(logcache0,0,0x1000);
			memset(logcache1,0,0x1000);

			// forced palette (e.g. for debugging CHR when palettes are all-black)
			palcache[(8*4)+0] = 0x0F;
			palcache[(8*4)+1] = 0x00;
			palcache[(8*4)+2] = 0x10;
			palcache[(8*4)+3] = 0x20;

			PPUViewer=1;
			break;
		}
		case WM_PAINT:
				PPUViewDoBlit();
				break;
		case WM_CLOSE:
		case WM_QUIT:
				KillPPUView();
				break;
		case WM_MOVING:
				break;
		case WM_MOVE:
				if (!IsIconic(hwndDlg)) {
				GetWindowRect(hwndDlg,&wrect);
				PPUViewPosX = wrect.left;
				PPUViewPosY = wrect.top;

				#ifdef WIN32
				WindowBoundsCheckNoResize(PPUViewPosX,PPUViewPosY,wrect.right);
				#endif
				}
				break;
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONDOWN:
		{
			// redraw now
			PPUViewSkip = PPUViewRefresh;
			FCEUD_UpdatePPUView(-1, 0);
			PPUViewDoBlit();
			break;
		}
		case WM_RBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
		{
			mouse_x = GET_X_LPARAM(lParam);
			mouse_y = GET_Y_LPARAM(lParam);
			if(((mouse_x >= patternDestX) && (mouse_x < (patternDestX + (PATTERNWIDTH * ZOOM)))) && (mouse_y >= patternDestY) && (mouse_y < (patternDestY + (PATTERNHEIGHT * ZOOM))))
			{
				if (pindex0 == 8)
					pindex0 = 0;
				else
					pindex0++;
			} else if(((mouse_x >= patternDestX + (PATTERNWIDTH * ZOOM) + 1) && (mouse_x < (patternDestX + (PATTERNWIDTH * ZOOM) * 2 + 1))) && (mouse_y >= patternDestY) && (mouse_y < (patternDestY + (PATTERNHEIGHT * ZOOM))))
			{
				if (pindex1 == 8)
					pindex1 = 0;
				else
					pindex1++;
			}
			// redraw now
			PPUViewSkip = PPUViewRefresh;
			FCEUD_UpdatePPUView(-1, 0);
			PPUViewDoBlit();
			break;
		}
		case WM_MOUSEMOVE:
				mouse_x = GET_X_LPARAM(lParam);
				mouse_y = GET_Y_LPARAM(lParam);
				if (((mouse_x >= patternDestX) && (mouse_x < (patternDestX + (PATTERNWIDTH * ZOOM)))) && (mouse_y >= patternDestY) && (mouse_y < (patternDestY + (PATTERNHEIGHT * ZOOM))))
				{
					int A = (mouse_x - patternDestX) / (8 * ZOOM);
					int B = (mouse_y - patternDestY) / (8 * ZOOM);
					if(PPUView_sprite16Mode) {
						A *= 2;
						mouse_x = (A & 0xE) + (B & 1);
						mouse_y = (B & 0xE) + ((A >> 4) & 1);
					} else {
						mouse_x = A;
						mouse_y = B;
					}
					sprintf(str,"Tile: $%X%X",mouse_y,mouse_x);
					SetDlgItemText(hwndDlg,LBL_PPUVIEW_TILE1,str);
					SetDlgItemText(hwndDlg,LBL_PPUVIEW_TILE2,"Tile:");
					SetDlgItemText(hwndDlg,LBL_PPUVIEW_PALETTES,"Palettes");
				} else if (((mouse_x >= patternDestX + (PATTERNWIDTH * ZOOM) + 1) && (mouse_x < (patternDestX + (PATTERNWIDTH * ZOOM) * 2 + 1))) && (mouse_y >= patternDestY) && (mouse_y < (patternDestY + (PATTERNHEIGHT * ZOOM))))
				{
					int A = (mouse_x - (patternDestX + (PATTERNWIDTH * ZOOM) + 1)) / (8 * ZOOM);
					int B = (mouse_y - patternDestY) / (8 * ZOOM);
					if(PPUView_sprite16Mode) {
						A *= 2;
						mouse_x = (A & 0xE) + (B & 1);
						mouse_y = (B & 0xE) + ((A >> 4) & 1);
					} else {
						mouse_x = A;
						mouse_y = B;
					}
					sprintf(str,"Tile: $%X%X",mouse_y,mouse_x);
					SetDlgItemText(hwndDlg,LBL_PPUVIEW_TILE2,str);
					SetDlgItemText(hwndDlg,LBL_PPUVIEW_TILE1,"Tile:");
					SetDlgItemText(hwndDlg,LBL_PPUVIEW_PALETTES,"Palettes");
				}
				else if(((mouse_x >= paletteDestX) && (mouse_x < (paletteDestX + PALETTEWIDTH))) && (mouse_y >= paletteDestY) && (mouse_y < (paletteDestY + PALETTEHEIGHT)))
				{
					mouse_x = (mouse_x - paletteDestX) / 32;
					mouse_y = (mouse_y - paletteDestY) / 32;
					int ix = (mouse_y<<4)|mouse_x;
					sprintf(str,"Palette: $%02X",palcache[ix]);
					SetDlgItemText(hwndDlg,LBL_PPUVIEW_TILE1,"Tile:");
					SetDlgItemText(hwndDlg,LBL_PPUVIEW_TILE2,"Tile:");
					SetDlgItemText(hwndDlg,LBL_PPUVIEW_PALETTES,str);
				} else
				{
					SetDlgItemText(hwndDlg,LBL_PPUVIEW_TILE1,"Tile:");
					SetDlgItemText(hwndDlg,LBL_PPUVIEW_TILE2,"Tile:");
					SetDlgItemText(hwndDlg,LBL_PPUVIEW_PALETTES,"Palettes");
				}
				break;
		case WM_NCACTIVATE:
				sprintf(str,"%d",PPUViewScanline);
				SetDlgItemText(hwndDlg,IDC_PPUVIEW_SCANLINE,str);
				break;
		case WM_COMMAND:
		{
			switch(HIWORD(wParam))
			{
				case EN_UPDATE:
				{
					GetDlgItemText(hwndDlg,IDC_PPUVIEW_SCANLINE,str,4);
					sscanf(str,"%d",&PPUViewScanline);
					if(PPUViewScanline > 239) PPUViewScanline = 239;
					break;
				}
				case BN_CLICKED:
				{
					switch(LOWORD(wParam))
					{
						case IDC_MASK_UNUSED_GRAPHICS:
						{
							PPUView_maskUnusedGraphics ^= 1;
							CheckDlgButton(hwndDlg, IDC_MASK_UNUSED_GRAPHICS, PPUView_maskUnusedGraphics ? BST_CHECKED : BST_UNCHECKED);
							EnableWindow(GetDlgItem(hwndDlg, IDC_INVERT_THE_MASK), PPUView_maskUnusedGraphics ?  true : false);
							// redraw now
							PPUViewSkip = PPUViewRefresh;
							FCEUD_UpdatePPUView(-1, 0);
							PPUViewDoBlit();
							break;
						}
						case IDC_INVERT_THE_MASK:
						{
							PPUView_invertTheMask ^= 1;
							CheckDlgButton(hwndDlg, IDC_INVERT_THE_MASK, PPUView_invertTheMask ? BST_CHECKED : BST_UNCHECKED);
							// redraw now
							PPUViewSkip = PPUViewRefresh;
							FCEUD_UpdatePPUView(-1, 0);
							PPUViewDoBlit();
							break;
						}
						case IDC_SPRITE16_MODE:
						{
							PPUView_sprite16Mode ^= 1;
							CheckDlgButton(hwndDlg, IDC_SPRITE16_MODE, PPUView_sprite16Mode ? BST_CHECKED : BST_UNCHECKED);
							// redraw now
							PPUViewSkip = PPUViewRefresh;
							FCEUD_UpdatePPUView(-1, 0);
							PPUViewDoBlit();
							break;
						}
					}
				}
			}
			break;
		}
		case WM_HSCROLL:
			if(lParam)
			{
				//refresh trackbar
				PPUViewRefresh = SendDlgItemMessage(hwndDlg,CTL_PPUVIEW_TRACKBAR,TBM_GETPOS,0,0);
			}
			break;
	}
	return FALSE;
}

void DoPPUView()
{
		if(!GameInfo) {
				FCEUD_PrintError("You must have a game loaded before you can use the PPU Viewer.");
				return;
		}
		if(GameInfo->type==GIT_NSF) {
				FCEUD_PrintError("Sorry, you can't use the PPU Viewer with NSFs.");
				return;
		}

		if(!hPPUView)
		{
			hPPUView = CreateDialog(fceu_hInstance,"PPUVIEW",NULL,PPUViewCallB);
			EnsurePixelSize();
		}
		if(hPPUView)
		{
			//SetWindowPos(hPPUView,HWND_TOP,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);
			ShowWindow(hPPUView, SW_SHOWNORMAL);
			SetForegroundWindow(hPPUView);
			// redraw now
			PPUViewSkip = PPUViewRefresh;
			FCEUD_UpdatePPUView(-1,1);
			PPUViewDoBlit();
		}
}
