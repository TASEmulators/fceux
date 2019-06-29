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

/*
zeromus's ruminations on the ntview:
nothing in the emu internals really tracks mirroring.
all we really know at any point is what is pointed to by each vnapage[4]
So if the ntview is to let a person change the mapping, it needs to infer the mapping mode
by looking at vnapages and adjust them as the user sees fit.
This was all working before I got here.
However, the rendering system was relying on a notion of mirroring modes, which was a mistake.
It should, instead, rely strictly on the vnapages.
Since it has a caching mechanism, the cache needs to track what vnapage it thinks it is caching,
and invalidate it when the vnapage changes.
*/

#include "common.h"
#include "ntview.h"
#include "debugger.h"
#include "../../fceu.h"
#include "../../debug.h"
#define INESPRIV
#include "../../cart.h"
#include "../../ines.h"
#include "../../palette.h"
#include "../../ppu.h"

HWND hNTView;

int NTViewPosX,NTViewPosY;

static uint8 palcache[36]; //palette cache
int NTViewScanline=0,NTViewer=0;
int NTViewSkip;
int NTViewRefresh = 0;
static int mouse_x,mouse_y;
bool redrawtables = false;
int chrchanged = 0;

static class NTCache {
public:
	NTCache() 
		: curr_vnapage(0)
	{}

	uint8* curr_vnapage;
	uint8* bitmap;
	uint8 cache[0x400];
	HDC hdc;
	HBITMAP hbmp;
	HGDIOBJ tmpobj;
} cache[4];

enum NT_MirrorType {
	NT_NONE = -1,
	NT_HORIZONTAL, NT_VERTICAL, NT_FOUR_SCREEN,
	NT_SINGLE_SCREEN_TABLE_0, NT_SINGLE_SCREEN_TABLE_1,
	NT_SINGLE_SCREEN_TABLE_2, NT_SINGLE_SCREEN_TABLE_3,
	NT_NUM_MIRROR_TYPES
};
#define IDC_NTVIEW_MIRROR_TYPE_0 IDC_NTVIEW_MIRROR_HORIZONTAL
NT_MirrorType ntmirroring, oldntmirroring = NT_NONE;
//int lockmirroring;
//int ntmirroring[4]; //these 4 ints are either 0, 1, 2, or 3 and tell where each of the 4 nametables point to
//uint8 *ntmirroringpointers[] = {&NTARAM[0x000],&NTARAM[0x400],ExtraNTARAM,ExtraNTARAM+0x400};

int horzscroll, vertscroll;

#define NTWIDTH             256
#define NTHEIGHT            240
#define NTDESTX_BASE        6
#define NTDESTY_BASE        18

int ntDestX = NTDESTX_BASE;
int ntDestY = NTDESTY_BASE;

void UpdateMirroringButtons();
void ChangeMirroring();

static BITMAPINFO bmInfo;
static HDC pDC;

extern uint32 TempAddr, RefreshAddr;
extern uint8 XOffset;

int xpos, ypos;
int scrolllines = 1;
int attview = 0;
int hidepal = 0;

// checkerboard tile for attribute view
const uint8 ATTRIBUTE_VIEW_TILE[16] = {	0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF };

//if you pass this 1 then it will draw no matter what. If you pass it 0
//then it will draw if redrawtables is true
void NTViewDoBlit(int autorefresh) {
	if (!hNTView) return;
	if (NTViewSkip < NTViewRefresh) {
		NTViewSkip++;
		return;
	}
	NTViewSkip=0;

	if((redrawtables && !autorefresh) || (autorefresh) || (scrolllines)){

	BitBlt(pDC,ntDestX,ntDestY,NTWIDTH,NTHEIGHT,cache[0].hdc,0,0,SRCCOPY);
	BitBlt(pDC,ntDestX+NTWIDTH,ntDestY,NTWIDTH,NTHEIGHT,cache[1].hdc,0,0,SRCCOPY);
	BitBlt(pDC,ntDestX,ntDestY+NTHEIGHT,NTWIDTH,NTHEIGHT,cache[2].hdc,0,0,SRCCOPY);
	BitBlt(pDC,ntDestX+NTWIDTH,ntDestY+NTHEIGHT,NTWIDTH,NTHEIGHT,cache[3].hdc,0,0,SRCCOPY);

	redrawtables = false;
	}

	if(scrolllines){
		SetROP2(pDC,R2_NOT);

		//draw vertical line
		MoveToEx(pDC,ntDestX+xpos,ntDestY,NULL);
		LineTo(pDC,ntDestX+xpos,ntDestY+(NTHEIGHT*2)-1);
	
		//draw horizontal line
		MoveToEx(pDC,ntDestX,ntDestY+ypos,NULL);
		LineTo(pDC,ntDestX+(NTWIDTH*2)-1,ntDestY+ypos);

		SetROP2(pDC,R2_COPYPEN);
	}
}

void UpdateMirroringButtons(){
	int i;
	for(i = 0; i < NT_NUM_MIRROR_TYPES;i++){
		if(i != ntmirroring)CheckDlgButton(hNTView, i+IDC_NTVIEW_MIRROR_TYPE_0, BST_UNCHECKED);
		else CheckDlgButton(hNTView, i+IDC_NTVIEW_MIRROR_TYPE_0, BST_CHECKED);
	}
	return;
}

void ChangeMirroring(){
	switch(ntmirroring){
		case NT_HORIZONTAL:
			vnapage[0] = vnapage[1] = &NTARAM[0x000];
			vnapage[2] = vnapage[3] = &NTARAM[0x400];
			break;
		case NT_VERTICAL:
			vnapage[0] = vnapage[2] = &NTARAM[0x000];
			vnapage[1] = vnapage[3] = &NTARAM[0x400];
			break;
		case NT_FOUR_SCREEN:
			vnapage[0] = &NTARAM[0x000];
			vnapage[1] = &NTARAM[0x400];
			if(ExtraNTARAM)
			{
				vnapage[2] = ExtraNTARAM;
				vnapage[3] = ExtraNTARAM + 0x400;
			}
			break;
		case NT_SINGLE_SCREEN_TABLE_0:
			vnapage[0] = vnapage[1] = vnapage[2] = vnapage[3] = &NTARAM[0x000];
			break;
		case NT_SINGLE_SCREEN_TABLE_1:
			vnapage[0] = vnapage[1] = vnapage[2] = vnapage[3] = &NTARAM[0x400];
			break;
		case NT_SINGLE_SCREEN_TABLE_2:
			if(ExtraNTARAM)
				vnapage[0] = vnapage[1] = vnapage[2] = vnapage[3] = ExtraNTARAM;
			break;
		case NT_SINGLE_SCREEN_TABLE_3:
			if(ExtraNTARAM)
				vnapage[0] = vnapage[1] = vnapage[2] = vnapage[3] = ExtraNTARAM + 0x400;
			break;
	}
	return;
}

INLINE void DrawChr(uint8 *pbitmap,const uint8 *chr,int pal){
	int y, x, tmp, index=0, p=0;
	uint8 chr0, chr1;
	//uint8 *table = &VPage[0][0]; //use the background table
	//pbitmap += 3*

	for (y = 0; y < 8; y++) { //todo: use index for y?
		chr0 = chr[index];
		chr1 = chr[index+8];
		tmp=7;
		for (x = 0; x < 8; x++) { //todo: use tmp for x?
			p = (chr0>>tmp)&1;
			p |= ((chr1>>tmp)&1)<<1;
			p = palcache[p+(pal*4)];
			tmp--;

			*(uint8*)(pbitmap++) = palo[p].b;
			*(uint8*)(pbitmap++) = palo[p].g;
			*(uint8*)(pbitmap++) = palo[p].r;
		}
		index++;
		pbitmap += (NTWIDTH*3)-24;
	}
	//index+=8;
	//pbitmap -= (((PALETTEBITWIDTH>>2)<<3)-24);
}

void DrawNameTable(int scanline, int ntnum, bool invalidateCache) {

	NTCache &c = cache[ntnum];
	uint8 *bitmap = c.bitmap, *tablecache = c.cache;

	uint8 *table = vnapage[ntnum];
	if(table == NULL)
		table = vnapage[ntnum&1];

	int a, ptable=0;
	uint8 *pbitmap = bitmap;
	
	if(PPU[0]&0x10){ //use the correct pattern table based on this bit
		ptable=0x1000;
	}

	bool invalid = invalidateCache;
	//if we werent asked to invalidate the cache, maybe we need to invalidate it anyway due to vnapage changing
	if(!invalid)
		invalid = (c.curr_vnapage != vnapage[ntnum]);
	c.curr_vnapage = vnapage[ntnum];
	
	//HACK: never cache anything
	invalid = true;

	pbitmap = bitmap;
	for(int y=0;y<30;y++){
		for(int x=0;x<32;x++){
			int ntaddr = (y*32)+x;
			int attraddr = 0x3C0+((y>>2)<<3)+(x>>2);
			if(invalid
				|| (table[ntaddr] != tablecache[ntaddr]) 
				|| (table[attraddr] != tablecache[attraddr])) {
				redrawtables = true;
				int temp = (((y&2)<<1)+(x&2));
				a = (table[attraddr] & (3<<temp)) >> temp;
				
				//the commented out code below is all allegedly equivalent to the single line above:
				//tmpx = x>>2;
				//tmpy = y>>2;
				//a = 0x3C0+(tmpy*8)+tmpx;
				//if((((x>>1)&1) == 0) && (((y>>1)&1) == 0)) a = table[a]&0x3;
				//if((((x>>1)&1) == 1) && (((y>>1)&1) == 0)) a = (table[a]&0xC)>>2;
				//if((((x>>1)&1) == 0) && (((y>>1)&1) == 1)) a = (table[a]&0x30)>>4;
				//if((((x>>1)&1) == 1) && (((y>>1)&1) == 1)) a = (table[a]&0xC0)>>6;

				int chr = table[ntaddr]*16;

				extern int FCEUPPU_GetAttr(int ntnum, int xt, int yt);

				//test.. instead of pretending that the nametable is a screen at 0,0 we pretend that it is at the current xscroll and yscroll
				//int xpos = ((RefreshAddr & 0x400) >> 2) | ((RefreshAddr & 0x1F) << 3) | XOffset;
				//int ypos = ((RefreshAddr & 0x3E0) >> 2) | ((RefreshAddr & 0x7000) >> 12); 
				//if(RefreshAddr & 0x800) ypos += 240;
				//int refreshaddr = (xpos/8+x)+(ypos/8+y)*32;

				int refreshaddr = (x)+(y)*32;

				a = FCEUPPU_GetAttr(ntnum,x,y);
				if (hidepal) a = 8;

				const uint8* chrp = FCEUPPU_GetCHR(ptable+chr,refreshaddr);
				if (attview) chrp = ATTRIBUTE_VIEW_TILE;

				//a good way to do it:
				DrawChr(pbitmap,chrp,a);

				tablecache[ntaddr] = table[ntaddr];
				tablecache[attraddr] = table[attraddr];
				//one could comment out the line above...
				//since there are so many fewer attribute values than NT values, it might be best just to refresh the whole attr table below with the memcpy

				//obviously this whole scheme of nt cache doesnt work if an mmc5 game is playing tricks with the attribute table
			}
			pbitmap += (8*3);
		}
		pbitmap += 7*((NTWIDTH*3));
	}

	 //this copies the attribute tables to the cache if needed. but we arent using it now because 
	//if(redrawtables){
	//	memcpy(tablecache+0x3c0,table+0x3c0,0x40);
	//}
}

static void CalculateBitmapPositions(HWND hwndDlg)
{
	RECT rect;
	POINT pt;
	GetWindowRect(GetDlgItem(hwndDlg, IDC_NTVIEW_TABLE_BOX), &rect);
	pt.x = rect.left;
	pt.y = rect.top;
	ScreenToClient(hwndDlg, &pt);
	ntDestX = pt.x + NTDESTX_BASE;
	ntDestY = pt.y + NTDESTY_BASE;
}

static BOOL CALLBACK EnsurePixelSizeEnumWindowsProc(HWND hwnd, LPARAM lParam)
{
	const int shift = lParam;
	HWND ntbox = GetDlgItem(hNTView, IDC_NTVIEW_TABLE_BOX);

	if (hwnd != ntbox)
	{
		RECT rect;
		GetWindowRect(hwnd, &rect);
		ScreenToClient(hNTView,(LPPOINT)&rect);
		ScreenToClient(hNTView,((LPPOINT)&rect)+1);
		SetWindowPos(hwnd,0,rect.left,rect.top+shift,0,0,SWP_NOZORDER | SWP_NOSIZE);
	}
	return TRUE;
}

static void EnsurePixelSize()
{
	// DPI varies, so the pixel size of the window may be too small to fit the viewer.
	// This expands the window and moves its controls around if necessary.

	if (!hNTView) return;
	HWND ntbox = GetDlgItem(hNTView, IDC_NTVIEW_TABLE_BOX);

	const int MIN_W = (NTWIDTH  * 2) + (NTDESTX_BASE * 2);
	const int MIN_H = (NTHEIGHT * 2) + (NTDESTY_BASE + 8);

	RECT rect, client;
	GetWindowRect(ntbox,&rect);
	GetClientRect(ntbox,&client);

	int nt_w = rect.right - rect.left;
	int nt_h = rect.bottom - rect.top;

	int nt_w_add = (nt_w < MIN_W) ? (MIN_W - nt_w) : 0;
	int nt_h_add = (nt_h < MIN_H) ? (MIN_H - nt_h) : 0;

	if (nt_w_add <= 0 && nt_h_add <= 0) return;

	// expand parent window
	RECT wrect;
	GetWindowRect(hNTView,&wrect);
	int ww = (wrect.right - wrect.left) + nt_w_add;
	int wh = (wrect.bottom - wrect.top) + nt_h_add;
	SetWindowPos(hNTView,0,0,0,ww,wh,SWP_NOZORDER | SWP_NOMOVE);

	// expand NT box
	SetWindowPos(ntbox,0,0,0,nt_w + nt_w_add,nt_h + nt_h_add, SWP_NOZORDER | SWP_NOMOVE);

	// move children
	if (nt_h_add > 0)
	{
		EnumChildWindows(hNTView, EnsurePixelSizeEnumWindowsProc, nt_h_add);
	}

	CalculateBitmapPositions(hNTView);
	RedrawWindow(hNTView,0,0,RDW_ERASE | RDW_INVALIDATE);
}

void FCEUD_UpdateNTView(int scanline, bool drawall) {
	if(!NTViewer) return;
	if(scanline != -1 && scanline != NTViewScanline) return;

	//uint8 *pbitmap = ppuv_palette;
	if (!hNTView) return;
	
	ppu_getScroll(xpos,ypos);


	if (NTViewSkip < NTViewRefresh) return;

	if(chrchanged) drawall = 1;

	//update palette only if required
	if (memcmp(palcache,PALRAM,32) != 0) {
		memcpy(palcache,PALRAM,32);
		drawall = 1; //palette has changed, so redraw all
	}
	 
	ntmirroring = NT_NONE;
	if(vnapage[0] == vnapage[1])ntmirroring = NT_HORIZONTAL;
	if(vnapage[0] == vnapage[2])ntmirroring = NT_VERTICAL;
	if((vnapage[0] != vnapage[1]) && (vnapage[0] != vnapage[2]))ntmirroring = NT_FOUR_SCREEN;

	if((vnapage[0] == vnapage[1]) && (vnapage[1] == vnapage[2]) && (vnapage[2] == vnapage[3])){ 
		if(vnapage[0] == &NTARAM[0x000])ntmirroring = NT_SINGLE_SCREEN_TABLE_0;
		if(vnapage[0] == &NTARAM[0x400])ntmirroring = NT_SINGLE_SCREEN_TABLE_1;
		if(vnapage[0] == ExtraNTARAM)ntmirroring = NT_SINGLE_SCREEN_TABLE_2;
		if(vnapage[0] == ExtraNTARAM+0x400)ntmirroring = NT_SINGLE_SCREEN_TABLE_3;
	}

	if(oldntmirroring != ntmirroring){
		UpdateMirroringButtons();
		oldntmirroring = ntmirroring;
	}

	for(int i=0;i<4;i++)
		DrawNameTable(scanline,i,drawall);

	chrchanged = 0;

	return;	
}

void KillNTView() {
	//GDI cleanup
	for(int i=0;i<4;i++) {
		DeleteObject(cache[i].hbmp);
		SelectObject(cache[i].hdc,cache[i].tmpobj);
		DeleteDC(cache[i].hdc);
	}

	ReleaseDC(hNTView,pDC);
	
	DeleteObject (SelectObject (pDC, GetStockObject (BLACK_PEN))) ;

	DestroyWindow(hNTView);
	hNTView=NULL;
	NTViewer=0;
	NTViewSkip=0;
}

INT_PTR CALLBACK NTViewCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	RECT wrect;
	char str[50];
	int TileID, TileX, TileY, NameTable, PPUAddress, AttAddress, Attrib;

	switch(uMsg) {
		case WM_INITDIALOG:
			if (NTViewPosX==-32000) NTViewPosX=0; //Just in case
			if (NTViewPosY==-32000) NTViewPosY=0;
			SetWindowPos(hwndDlg,0,NTViewPosX,NTViewPosY,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER);

			CalculateBitmapPositions(hwndDlg);

			//prepare the bitmap attributes
			//pattern tables
			memset(&bmInfo.bmiHeader,0,sizeof(BITMAPINFOHEADER));
			bmInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmInfo.bmiHeader.biWidth = NTWIDTH;
			bmInfo.bmiHeader.biHeight = -NTHEIGHT;
			bmInfo.bmiHeader.biPlanes = 1;
			bmInfo.bmiHeader.biBitCount = 24;

			//create memory dcs
			pDC = GetDC(hwndDlg);
			for(int i=0;i<4;i++) {
				NTCache &c = cache[i];
				c.hdc = CreateCompatibleDC(pDC);
				c.hbmp = CreateDIBSection(pDC,&bmInfo,DIB_RGB_COLORS,(void**)&c.bitmap,0,0);
				c.tmpobj = SelectObject(c.hdc,c.hbmp);
			}

			//Refresh Trackbar
			SendDlgItemMessage(hwndDlg,IDC_NTVIEW_REFRESH_TRACKBAR,TBM_SETRANGE,0,(LPARAM)MAKELONG(0,25));
			SendDlgItemMessage(hwndDlg,IDC_NTVIEW_REFRESH_TRACKBAR,TBM_SETPOS,1,NTViewRefresh);

			//Set Text Limit
			SendDlgItemMessage(hwndDlg,IDC_NTVIEW_SCANLINE,EM_SETLIMITTEXT,3,0);

			//force redraw the first time the PPU Viewer is opened
			NTViewSkip=100;

			SelectObject (pDC, CreatePen (PS_SOLID, 2, RGB (255, 255, 255))) ;

			CheckDlgButton(hwndDlg, IDC_NTVIEW_SHOW_SCROLL_LINES, scrolllines ? BST_CHECKED : BST_UNCHECKED);
			EnableWindow(GetDlgItem(hwndDlg, IDC_NTVIEW_SCANLINE_TEXT), scrolllines);
			EnableWindow(GetDlgItem(hwndDlg, IDC_NTVIEW_SCANLINE), scrolllines);
			CheckDlgButton(hwndDlg, IDC_NTVIEW_SHOW_ATTRIBUTES,   attview ? BST_CHECKED : BST_UNCHECKED);
			CheckDlgButton(hwndDlg, IDC_NTVIEW_HIDE_PALETTES,     hidepal ? BST_CHECKED : BST_UNCHECKED);

			//clear cache
			memset(palcache,0,32);

			// forced palette (e.g. for debugging nametables when palettes are all-black)
			palcache[(8*4)+0] = 0x0F;
			palcache[(8*4)+1] = 0x00;
			palcache[(8*4)+2] = 0x10;
			palcache[(8*4)+3] = 0x20;

			NTViewer=1;
			break;
		case WM_PAINT:
			NTViewDoBlit(1);
			break;
		case WM_CLOSE:
		case WM_QUIT:
			KillNTView();
			break;
		case WM_MOVING:
			break;
		case WM_MOVE:
			if (!IsIconic(hwndDlg)) {
			GetWindowRect(hwndDlg,&wrect);
			NTViewPosX = wrect.left;
			NTViewPosY = wrect.top;

			#ifdef WIN32
			WindowBoundsCheckNoResize(NTViewPosX,NTViewPosY,wrect.right);
			#endif
			}
			break;
		case WM_RBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
			mouse_x = GET_X_LPARAM(lParam);
			mouse_y = GET_Y_LPARAM(lParam);
			break;
		case WM_MOUSEMOVE:
			mouse_x = GET_X_LPARAM(lParam);
			mouse_y = GET_Y_LPARAM(lParam);
			if((mouse_x > ntDestX) && (mouse_x < ntDestX+(NTWIDTH*2))
				&& (mouse_y > ntDestY) && (mouse_y < ntDestY+(NTHEIGHT*2))){
				TileX = (mouse_x-ntDestX)/8;
				TileY = (mouse_y-ntDestY)/8;
				sprintf(str,"X / Y: %0d / %0d",TileX,TileY);
				SetDlgItemText(hwndDlg,IDC_NTVIEW_PROPERTIES_LINE_2,str);
				NameTable = (TileX/32)+((TileY/30)*2);
				PPUAddress = 0x2000+(NameTable*0x400)+((TileY%30)*32)+(TileX%32);
				sprintf(str,"PPU Address: %04X",PPUAddress);
				SetDlgItemText(hwndDlg,IDC_NTVIEW_PROPERTIES_LINE_3,str);
				TileID = vnapage[(PPUAddress>>10)&0x3][PPUAddress&0x3FF];
				sprintf(str,"Tile ID: %02X",TileID);
				SetDlgItemText(hwndDlg,IDC_NTVIEW_PROPERTIES_LINE_1,str);
				AttAddress = 0x23C0 | (PPUAddress & 0x0C00) | ((PPUAddress >> 4) & 0x38) | ((PPUAddress >> 2) & 0x07);
				Attrib = vnapage[(AttAddress>>10)&0x3][AttAddress&0x3FF];
				Attrib = (Attrib >> ((PPUAddress&2) | ((PPUAddress&64)>>4))) & 0x3;
				sprintf(str,"Attribute: %1X (%04X)",Attrib,AttAddress);
				SetDlgItemText(hwndDlg,IDC_NTVIEW_PROPERTIES_LINE_4,str);
			}
			break;
		case WM_NCACTIVATE:
			sprintf(str,"%d",NTViewScanline);
			SetDlgItemText(hwndDlg,IDC_NTVIEW_SCANLINE,str);
			break;
		case WM_COMMAND:
			switch(HIWORD(wParam)) {
				case EN_UPDATE:
					GetDlgItemText(hwndDlg,IDC_NTVIEW_SCANLINE,str,4);
					sscanf(str,"%d",&NTViewScanline);
					if (NTViewScanline > 239) NTViewScanline = 239;
					chrchanged = 1;
					break;
				case BN_CLICKED:
					switch(LOWORD(wParam)) {
						case IDC_NTVIEW_MIRROR_HORIZONTAL :
						case IDC_NTVIEW_MIRROR_VERTICAL :
						case IDC_NTVIEW_MIRROR_FOUR_SCREEN :
						case IDC_NTVIEW_MIRROR_SS_TABLE_0 :
						case IDC_NTVIEW_MIRROR_SS_TABLE_1 :
						case IDC_NTVIEW_MIRROR_SS_TABLE_2 :
						case IDC_NTVIEW_MIRROR_SS_TABLE_3 :
							oldntmirroring = ntmirroring = NT_MirrorType(LOWORD(wParam)-IDC_NTVIEW_MIRROR_TYPE_0);
							ChangeMirroring();
							break;
						case IDC_NTVIEW_SHOW_SCROLL_LINES : 
							scrolllines ^= 1;
							EnableWindow(GetDlgItem(hwndDlg, IDC_NTVIEW_SCANLINE_TEXT), scrolllines);
							EnableWindow(GetDlgItem(hwndDlg, IDC_NTVIEW_SCANLINE), scrolllines);
							chrchanged = 1;
							break;
						case IDC_NTVIEW_SHOW_ATTRIBUTES :
							attview ^= 1;
							chrchanged = 1;
							break;
						case IDC_NTVIEW_HIDE_PALETTES :
							hidepal ^= 1;
							chrchanged = 1;
							break;
					}
					break;
			}
			break;
		case WM_HSCROLL:
			if (lParam) { //refresh trackbar
				NTViewRefresh = SendDlgItemMessage(hwndDlg,IDC_NTVIEW_REFRESH_TRACKBAR,TBM_GETPOS,0,0);
			}
			break;
	}
	return FALSE;
}

void DoNTView()
{
	if (!GameInfo) {
		FCEUD_PrintError("You must have a game loaded before you can use the Name Table Viewer.");
		return;
	}
	if (GameInfo->type==GIT_NSF) {
		FCEUD_PrintError("Sorry, you can't use the Name Table Viewer with NSFs.");
		return;
	}

	if (!hNTView)
	{
		hNTView = CreateDialog(fceu_hInstance,"NTVIEW",NULL,NTViewCallB);
		EnsurePixelSize();
		new(cache) NTCache[4]; //reinitialize NTCache
	}
	if (hNTView)
	{
		//SetWindowPos(hNTView,HWND_TOP,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);
		ShowWindow(hNTView, SW_SHOWNORMAL);
		SetForegroundWindow(hNTView);
		FCEUD_UpdateNTView(-1,true);
		NTViewDoBlit(1);
	}
}

