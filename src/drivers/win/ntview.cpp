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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "common.h"
#include "ntview.h"
#include "debugger.h"
#include "..\..\fceu.h"
#include "..\..\debug.h"
#define INESPRIV
#include "..\..\cart.h"
#include "..\..\ines.h"
#include "..\..\palette.h"
#include "..\..\ppu.h"

HWND hNTView;

int NTViewPosX,NTViewPosY;
static uint8 palcache[32]; //palette cache //mbg merge 7/19/06 needed to be static
//uint8 ntcache0[0x400],ntcache1[0x400],ntcache2[0x400],ntcache3[0x400]; //cache CHR, fixes a refresh problem when right-clicking
uint8 *ntable0, *ntable1, *ntable2, *ntable3; //name table bitmap array
uint8 *ntablemirr[4];
uint8 ntcache[4][0x400]; //todo: use an array for the above variables too
int NTViewScanline=0,NTViewer=0;
int NTViewSkip,NTViewRefresh;
static int mouse_x,mouse_y; //todo: is static needed here? --mbg 7/19/06 - i think so
int redrawtables = 0;
int chrchanged = 0;
int ntmirroring, oldntmirroring = -1; //0 is horizontal, 1 is vertical, 2 is four screen.
int lockmirroring;
//int ntmirroring[4]; //these 4 ints are either 0, 1, 2, or 3 and tell where each of the 4 nametables point to
//uint8 *ntmirroringpointers[] = {&NTARAM[0x000],&NTARAM[0x400],ExtraNTARAM,ExtraNTARAM+0x400};

int horzscroll, vertscroll;

#define NTWIDTH			256
#define NTHEIGHT		240
//#define PATTERNBITWIDTH	PATTERNWIDTH*3
#define NTDESTX		10
#define NTDESTY		15
#define ZOOM		1

//#define PALETTEWIDTH	32*4*4
//#define PALETTEHEIGHT	32*2
//#define PALETTEBITWIDTH	PALETTEWIDTH*3
//#define PALETTEDESTX	10
//#define PALETTEDESTY	327

void UpdateMirroringButtons();
void ChangeMirroring();

static BITMAPINFO bmInfo; //todo is static needed here so it won't interefere with the pattern table viewer?
static HDC pDC,TmpDC0,TmpDC1;
static HBITMAP TmpBmp0,TmpBmp1;
static HGDIOBJ TmpObj0,TmpObj1;

static HDC TmpDC2,TmpDC3;
static HBITMAP TmpBmp2,TmpBmp3;
static HGDIOBJ TmpObj2,TmpObj3;

extern uint32 TempAddr, RefreshAddr;
extern uint8 XOffset;

int xpos, ypos;
int scrolllines = 1;

//if you pass this 1 then it will draw no matter what. If you pass it 0
//then it will draw if redrawtables is 1
void NTViewDoBlit(int autorefresh) {
	if (!hNTView) return;
	if (NTViewSkip < NTViewRefresh) {
		NTViewSkip++;
		return;
	}
	NTViewSkip=0;

	if((redrawtables && !autorefresh) || (autorefresh) || (scrolllines)){
		//todo: use switch and use bitblt for because its faster
		switch(ntmirroring){
			case 0: //horizontal
				BitBlt(pDC,NTDESTX,NTDESTY,NTWIDTH,NTHEIGHT,TmpDC0,0,0,SRCCOPY);
				BitBlt(pDC,NTDESTX+NTWIDTH,NTDESTY,NTWIDTH,NTHEIGHT,TmpDC0,0,0,SRCCOPY);
				BitBlt(pDC,NTDESTX,NTDESTY+NTHEIGHT,NTWIDTH,NTHEIGHT,TmpDC1,0,0,SRCCOPY);
				BitBlt(pDC,NTDESTX+NTWIDTH,NTDESTY+NTHEIGHT,NTWIDTH,NTHEIGHT,TmpDC1,0,0,SRCCOPY);
			break;
			case 1: //vertical
				BitBlt(pDC,NTDESTX,NTDESTY,NTWIDTH,NTHEIGHT,TmpDC0,0,0,SRCCOPY);
				BitBlt(pDC,NTDESTX+NTWIDTH,NTDESTY,NTWIDTH,NTHEIGHT,TmpDC1,0,0,SRCCOPY);
				BitBlt(pDC,NTDESTX,NTDESTY+NTHEIGHT,NTWIDTH,NTHEIGHT,TmpDC0,0,0,SRCCOPY);
				BitBlt(pDC,NTDESTX+NTWIDTH,NTDESTY+NTHEIGHT,NTWIDTH,NTHEIGHT,TmpDC1,0,0,SRCCOPY);
				break;
			case 2: //four screen
				BitBlt(pDC,NTDESTX,NTDESTY,NTWIDTH,NTHEIGHT,TmpDC0,0,0,SRCCOPY);
				BitBlt(pDC,NTDESTX+NTWIDTH,NTDESTY,NTWIDTH,NTHEIGHT,TmpDC1,0,0,SRCCOPY);
				BitBlt(pDC,NTDESTX,NTDESTY+NTHEIGHT,NTWIDTH,NTHEIGHT,TmpDC2,0,0,SRCCOPY);
				BitBlt(pDC,NTDESTX+NTWIDTH,NTDESTY+NTHEIGHT,NTWIDTH,NTHEIGHT,TmpDC3,0,0,SRCCOPY);
			break;
			case 3: //single screen, table 0
				BitBlt(pDC,NTDESTX,NTDESTY,NTWIDTH,NTHEIGHT,TmpDC0,0,0,SRCCOPY);
				BitBlt(pDC,NTDESTX+NTWIDTH,NTDESTY,NTWIDTH,NTHEIGHT,TmpDC0,0,0,SRCCOPY);
				BitBlt(pDC,NTDESTX,NTDESTY+NTHEIGHT,NTWIDTH,NTHEIGHT,TmpDC0,0,0,SRCCOPY);
				BitBlt(pDC,NTDESTX+NTWIDTH,NTDESTY+NTHEIGHT,NTWIDTH,NTHEIGHT,TmpDC0,0,0,SRCCOPY);
			break;
			case 4: //single screen, table 1
				BitBlt(pDC,NTDESTX,NTDESTY,NTWIDTH,NTHEIGHT,TmpDC1,0,0,SRCCOPY);
				BitBlt(pDC,NTDESTX+NTWIDTH,NTDESTY,NTWIDTH,NTHEIGHT,TmpDC1,0,0,SRCCOPY);
				BitBlt(pDC,NTDESTX,NTDESTY+NTHEIGHT,NTWIDTH,NTHEIGHT,TmpDC1,0,0,SRCCOPY);
				BitBlt(pDC,NTDESTX+NTWIDTH,NTDESTY+NTHEIGHT,NTWIDTH,NTHEIGHT,TmpDC1,0,0,SRCCOPY);
			break;
			case 5: //single screen, table 2
				BitBlt(pDC,NTDESTX,NTDESTY,NTWIDTH,NTHEIGHT,TmpDC2,0,0,SRCCOPY);
				BitBlt(pDC,NTDESTX+NTWIDTH,NTDESTY,NTWIDTH,NTHEIGHT,TmpDC2,0,0,SRCCOPY);
				BitBlt(pDC,NTDESTX,NTDESTY+NTHEIGHT,NTWIDTH,NTHEIGHT,TmpDC2,0,0,SRCCOPY);
				BitBlt(pDC,NTDESTX+NTWIDTH,NTDESTY+NTHEIGHT,NTWIDTH,NTHEIGHT,TmpDC2,0,0,SRCCOPY);
			break;
			case 6: //single screen, table 3
				BitBlt(pDC,NTDESTX,NTDESTY,NTWIDTH,NTHEIGHT,TmpDC3,0,0,SRCCOPY);
				BitBlt(pDC,NTDESTX+NTWIDTH,NTDESTY,NTWIDTH,NTHEIGHT,TmpDC3,0,0,SRCCOPY);
				BitBlt(pDC,NTDESTX,NTDESTY+NTHEIGHT,NTWIDTH,NTHEIGHT,TmpDC3,0,0,SRCCOPY);
				BitBlt(pDC,NTDESTX+NTWIDTH,NTDESTY+NTHEIGHT,NTWIDTH,NTHEIGHT,TmpDC3,0,0,SRCCOPY);
			break;
		}
		/*
		if(ntmirroring == 0){ //horizontal
			StretchBlt(pDC,NTDESTX,NTDESTY,NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC0,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
			StretchBlt(pDC,NTDESTX+(NTWIDTH*ZOOM),NTDESTY,NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC0,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
			StretchBlt(pDC,NTDESTX,NTDESTY+(NTHEIGHT*ZOOM),NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC1,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
			StretchBlt(pDC,NTDESTX+(NTWIDTH*ZOOM),NTDESTY+(NTHEIGHT*ZOOM),NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC1,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
		}
		if(ntmirroring == 1){ //vertical
			StretchBlt(pDC,NTDESTX,NTDESTY,NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC0,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
			StretchBlt(pDC,NTDESTX+(NTWIDTH*ZOOM),NTDESTY,NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC1,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
			StretchBlt(pDC,NTDESTX,NTDESTY+(NTHEIGHT*ZOOM),NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC0,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
			StretchBlt(pDC,NTDESTX+(NTWIDTH*ZOOM),NTDESTY+(NTHEIGHT*ZOOM),NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC1,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
		}
		if(ntmirroring == 2){ //four screen
			StretchBlt(pDC,NTDESTX,NTDESTY,NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC0,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
			StretchBlt(pDC,NTDESTX+(NTWIDTH*ZOOM),NTDESTY,NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC1,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
			StretchBlt(pDC,NTDESTX,NTDESTY+(NTHEIGHT*ZOOM),NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC2,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
			StretchBlt(pDC,NTDESTX+(NTWIDTH*ZOOM),NTDESTY+(NTHEIGHT*ZOOM),NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC3,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
		}
		if(ntmirroring == 3){ //single screen, table 0
			StretchBlt(pDC,NTDESTX,NTDESTY,NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC0,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
			StretchBlt(pDC,NTDESTX+(NTWIDTH*ZOOM),NTDESTY,NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC0,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
			StretchBlt(pDC,NTDESTX,NTDESTY+(NTHEIGHT*ZOOM),NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC0,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
			StretchBlt(pDC,NTDESTX+(NTWIDTH*ZOOM),NTDESTY+(NTHEIGHT*ZOOM),NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC0,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
		}
		if(ntmirroring == 4){ //single screen, table 1
			StretchBlt(pDC,NTDESTX,NTDESTY,NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC1,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
			StretchBlt(pDC,NTDESTX+(NTWIDTH*ZOOM),NTDESTY,NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC1,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
			StretchBlt(pDC,NTDESTX,NTDESTY+(NTHEIGHT*ZOOM),NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC1,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
			StretchBlt(pDC,NTDESTX+(NTWIDTH*ZOOM),NTDESTY+(NTHEIGHT*ZOOM),NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC1,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
		}
		if(ntmirroring == 5){ //single screen, table 2
			StretchBlt(pDC,NTDESTX,NTDESTY,NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC2,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
			StretchBlt(pDC,NTDESTX+(NTWIDTH*ZOOM),NTDESTY,NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC2,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
			StretchBlt(pDC,NTDESTX,NTDESTY+(NTHEIGHT*ZOOM),NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC2,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
			StretchBlt(pDC,NTDESTX+(NTWIDTH*ZOOM),NTDESTY+(NTHEIGHT*ZOOM),NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC2,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
		}
		if(ntmirroring == 6){ //single screen, table 3
			StretchBlt(pDC,NTDESTX,NTDESTY,NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC3,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
			StretchBlt(pDC,NTDESTX+(NTWIDTH*ZOOM),NTDESTY,NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC3,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
			StretchBlt(pDC,NTDESTX,NTDESTY+(NTHEIGHT*ZOOM),NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC3,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
			StretchBlt(pDC,NTDESTX+(NTWIDTH*ZOOM),NTDESTY+(NTHEIGHT*ZOOM),NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC3,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
		}
		*/
	redrawtables = 0;
	}

	if(scrolllines){
		SetROP2(pDC,R2_NOT);

		//draw vertical line
		MoveToEx(pDC,NTDESTX+xpos,NTDESTY,NULL);
		LineTo(pDC,NTDESTX+xpos,NTDESTY+(NTHEIGHT*2));
	
		//draw horizontal line
		MoveToEx(pDC,NTDESTX,NTDESTY+ypos,NULL);
		LineTo(pDC,NTDESTX+(NTWIDTH*2),NTDESTY+ypos);

		SetROP2(pDC,R2_COPYPEN);
	}
	//StretchBlt(pDC,NTDESTX,NTDESTY,NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC0,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
	//StretchBlt(pDC,NTDESTX+(NTWIDTH*ZOOM),NTDESTY,NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC1,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
	//StretchBlt(pDC,NTDESTX,NTDESTY+(NTHEIGHT*ZOOM),NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC2,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
	//StretchBlt(pDC,NTDESTX+(NTWIDTH*ZOOM),NTDESTY+(NTHEIGHT*ZOOM),NTWIDTH*ZOOM,NTHEIGHT*ZOOM,TmpDC3,0,NTHEIGHT-1,NTWIDTH,-NTHEIGHT,SRCCOPY);
}

void UpdateMirroringButtons(){
	int i;
	for(i = 0; i < 7;i++){
		if(i != ntmirroring)CheckDlgButton(hNTView, i+1001, BST_UNCHECKED);
		else CheckDlgButton(hNTView, i+1001, BST_CHECKED);
	}
	return;
}

void ChangeMirroring(){
	switch(ntmirroring){
		case 0:
			vnapage[0] = vnapage[1] = &NTARAM[0x000];
			vnapage[2] = vnapage[3] = &NTARAM[0x400];
			break;
		case 1:
			vnapage[0] = vnapage[2] = &NTARAM[0x000];
			vnapage[1] = vnapage[3] = &NTARAM[0x400];
			break;
		case 2:
			vnapage[0] = &NTARAM[0x000];
			vnapage[1] = &NTARAM[0x400];
			vnapage[2] = ExtraNTARAM;
			vnapage[3] = ExtraNTARAM+0x400;
			break;
		case 3:
			vnapage[0] = vnapage[1] = vnapage[2] = vnapage[3] = &NTARAM[0x000];
			break;
		case 4:
			vnapage[0] = vnapage[1] = vnapage[2] = vnapage[3] = &NTARAM[0x400];
			break;
		case 5:
			vnapage[0] = vnapage[1] = vnapage[2] = vnapage[3] = ExtraNTARAM;
			break;
		case 6:
			vnapage[0] = vnapage[1] = vnapage[2] = vnapage[3] = ExtraNTARAM+0x400;
			break;
	}
	return;
}

INLINE void DrawChr(uint8 *pbitmap,uint8 *chr,int pal){
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

void DrawNameTable(uint8 *bitmap, uint8 *table, uint8 *tablecache) {
	int x,y,a, chr, ptable=0, cacheaddr0, cacheaddr1;//index=0;
	uint8 *pbitmap = bitmap;
	
	if(PPU[0]&0x10){ //use the correct pattern table based on this bit
		ptable=0x1000;
	}

	pbitmap = bitmap;
	for(y = 0;y < 30;y++){
		for(x = 0;x < 32;x++){
			cacheaddr0 = (y*32)+x;
			cacheaddr1 = 0x3C0+((y>>2)<<3)+(x>>2);
			if((tablecache == 0) || (table[cacheaddr0] != tablecache[cacheaddr0]) || 
				(table[cacheaddr1] != tablecache[cacheaddr1])){
				redrawtables = 1;
				a = (table[0x3C0+((y>>2)<<3)+(x>>2)] & (3<<(((y&2)<<1)+(x&2)))) >> (((y&2)<<1)+(x&2));
				//the commented out code below is all equilivent to the single line above.
				//tmpx = x>>2;
				//tmpy = y>>2;
				//a = 0x3C0+(tmpy*8)+tmpx;
				//if((((x>>1)&1) == 0) && (((y>>1)&1) == 0)) a = table[a]&0x3;
				//if((((x>>1)&1) == 1) && (((y>>1)&1) == 0)) a = (table[a]&0xC)>>2;
				//if((((x>>1)&1) == 0) && (((y>>1)&1) == 1)) a = (table[a]&0x30)>>4;
				//if((((x>>1)&1) == 1) && (((y>>1)&1) == 1)) a = (table[a]&0xC0)>>6;

				chr = table[y*32+x]*16;
				DrawChr(pbitmap,&VPage[(ptable+chr)>>10][ptable+chr],a);
				if(tablecache != 0){
					tablecache[cacheaddr0] = table[cacheaddr0];
					//tablecache[cacheaddr1] = table[cacheaddr1];
				}
			}
			pbitmap += (8*3);
		}
		pbitmap += 7*((NTWIDTH*3));
	}

	if((tablecache != 0) && (redrawtables)){ //this copies the attribute tables to the cache if needed
		memcpy(tablecache+0x3c0,table+0x3c0,0x40);
	}
}

void FCEUD_UpdateNTView(int scanline, int drawall) {
	if(!NTViewer) return;
	if(scanline != -1 && scanline != NTViewScanline) return;

	//uint8 *pbitmap = ppuv_palette;
	if (!hNTView) return;
	
	xpos = ((RefreshAddr & 0x400) >> 2) | ((RefreshAddr & 0x1F) << 3) | XOffset;

	ypos = ((RefreshAddr & 0x3E0) >> 2) | ((RefreshAddr & 0x7000) >> 12); 
		if(RefreshAddr & 0x800) ypos += 240;

	//char str[50];
	//sprintf(str,"%d,%d,%d",horzscroll,vertscroll,ntmirroring);
	//sprintf(str,"%08X  %08X",TempAddr, RefreshAddr);
	//SetDlgItemText(hNTView, 103, str);

	if (NTViewSkip < NTViewRefresh) return;

	if(chrchanged) drawall = 1;

	//update palette only if required
	if (memcmp(palcache,PALRAM,32) != 0) {
		memcpy(palcache,PALRAM,32);
		drawall = 1; //palette has changed, so redraw all
	}
	

	ntmirroring = -1;
	if(vnapage[0] == vnapage[1])ntmirroring = 0;
	if(vnapage[0] == vnapage[2])ntmirroring = 1;
	if((vnapage[0] != vnapage[1]) && (vnapage[0] != vnapage[2]))ntmirroring = 2;

	if((vnapage[0] == vnapage[1]) && (vnapage[1] == vnapage[2]) && (vnapage[2] == vnapage[3])){ 
		if(vnapage[0] == &NTARAM[0x000])ntmirroring = 3;
		if(vnapage[0] == &NTARAM[0x400])ntmirroring = 4;
		if(vnapage[0] == ExtraNTARAM)ntmirroring = 5;
		if(vnapage[0] == ExtraNTARAM+0x400)ntmirroring = 6;
	}

	if(oldntmirroring != ntmirroring){
		UpdateMirroringButtons();
		oldntmirroring = ntmirroring;
	}

	if(drawall){
		DrawNameTable(ntable0,&NTARAM[0x000],0);
		DrawNameTable(ntable1,&NTARAM[0x400],0);
		DrawNameTable(ntable2,ExtraNTARAM,0);
		DrawNameTable(ntable3,ExtraNTARAM+0x400,0);
	} else {
		if((ntmirroring == 0) || (ntmirroring == 1) || (ntmirroring == 3))
			DrawNameTable(ntable0,&NTARAM[0x000],ntcache[0]);
		if((ntmirroring == 0) || (ntmirroring == 1) || (ntmirroring == 4))
			DrawNameTable(ntable1,&NTARAM[0x400],ntcache[1]);
		if(ntmirroring == 2){
			DrawNameTable(ntable2,ExtraNTARAM,ntcache[2]);
			DrawNameTable(ntable3,ExtraNTARAM+0x400,ntcache[3]);
		}
	}

	chrchanged = 0;

	return;	
}

void KillNTView() {
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
	DeleteObject(TmpBmp3);
	SelectObject(TmpDC3,TmpObj3);
	DeleteDC(TmpDC3);
	ReleaseDC(hNTView,pDC);
	
	DeleteObject (SelectObject (pDC, GetStockObject (BLACK_PEN))) ;

	DestroyWindow(hNTView);
	hNTView=NULL;
	NTViewer=0;
	NTViewSkip=0;
}

BOOL CALLBACK NTViewCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	RECT wrect;
	char str[50];
	int TileID, TileX, TileY, NameTable, PPUAddress;

	switch(uMsg) {
		case WM_INITDIALOG:
			//SetWindowPos(hwndDlg,0,PPUViewPosX,PPUViewPosY,0,0,SWP_NOSIZE|SWP_NOZORDER|SWP_NOOWNERZORDER);

			//prepare the bitmap attributes
			//pattern tables
			memset(&bmInfo.bmiHeader,0,sizeof(BITMAPINFOHEADER));
			bmInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
			bmInfo.bmiHeader.biWidth = NTWIDTH;
			bmInfo.bmiHeader.biHeight = -NTHEIGHT;
			bmInfo.bmiHeader.biPlanes = 1;
			bmInfo.bmiHeader.biBitCount = 24;

			//create memory dcs
			pDC = GetDC(hwndDlg); // GetDC(GetDlgItem(hwndDlg,101));
			TmpDC0 = CreateCompatibleDC(pDC); //name table 0
			TmpDC1 = CreateCompatibleDC(pDC); //name table 1
			TmpDC2 = CreateCompatibleDC(pDC); //name table 2
			TmpDC3 = CreateCompatibleDC(pDC); //name table 3
			//TmpDC2 = CreateCompatibleDC(pDC); //palettes

			//create bitmaps and select them into the memory dc's
			TmpBmp0 = CreateDIBSection(pDC,&bmInfo,DIB_RGB_COLORS,(void**)&ntable0,0,0);
			TmpObj0 = SelectObject(TmpDC0,TmpBmp0);
			TmpBmp1 = CreateDIBSection(pDC,&bmInfo,DIB_RGB_COLORS,(void**)&ntable1,0,0);
			TmpObj1 = SelectObject(TmpDC1,TmpBmp1);
			TmpBmp2 = CreateDIBSection(pDC,&bmInfo,DIB_RGB_COLORS,(void**)&ntable2,0,0);
			TmpObj2 = SelectObject(TmpDC2,TmpBmp2);
			TmpBmp3 = CreateDIBSection(pDC,&bmInfo,DIB_RGB_COLORS,(void**)&ntable3,0,0);
			TmpObj3 = SelectObject(TmpDC3,TmpBmp3);

			//Refresh Trackbar
			SendDlgItemMessage(hwndDlg,201,TBM_SETRANGE,0,(LPARAM)MAKELONG(0,25));
			SendDlgItemMessage(hwndDlg,201,TBM_SETPOS,1,NTViewRefresh);

			//Set Text Limit
			SendDlgItemMessage(hwndDlg,102,EM_SETLIMITTEXT,3,0);

			//force redraw the first time the PPU Viewer is opened
			NTViewSkip=100;

			SelectObject (pDC, CreatePen (PS_SOLID, 2, RGB (255, 255, 255))) ;

			CheckDlgButton(hwndDlg, 1008, BST_CHECKED);
			//clear cache
			//memset(palcache,0,32);
			//memset(ntcache0,0,0x400);
			//memset(ntcache1,0,0x400);
			//memset(ntcache2,0,0x400);
			//memset(ntcache3,0,0x400);

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
			GetWindowRect(hwndDlg,&wrect);
			//NTViewPosX = wrect.left; //todo: is this needed?
			//NTViewPosY = wrect.top;
			break;
		case WM_RBUTTONDBLCLK:
		case WM_RBUTTONDOWN:
			mouse_x = GET_X_LPARAM(lParam);
			mouse_y = GET_Y_LPARAM(lParam);
			/*if (((mouse_x >= PATTERNDESTX) && (mouse_x < (PATTERNDESTX+(PATTERNWIDTH*ZOOM)))) && (mouse_y >= PATTERNDESTY) && (mouse_y < (PATTERNDESTY+(PATTERNHEIGHT*ZOOM)))) {
				if (pindex0 == 7) pindex0 = 0;
				else pindex0++;
			}
			else if (((mouse_x >= PATTERNDESTX+(PATTERNWIDTH*ZOOM)+1) && (mouse_x < (PATTERNDESTX+(PATTERNWIDTH*ZOOM)*2+1))) && (mouse_y >= PATTERNDESTY) && (mouse_y < (PATTERNDESTY+(PATTERNHEIGHT*ZOOM)))) {
				if (pindex1 == 7) pindex1 = 0;
				else pindex1++;
			}
			UpdatePPUView(0);
			PPUViewDoBlit();*/
			break;
		case WM_MOUSEMOVE:
			mouse_x = GET_X_LPARAM(lParam);
			mouse_y = GET_Y_LPARAM(lParam);
			if((mouse_x > NTDESTX) && (mouse_x < NTDESTX+(NTWIDTH*2))
				&& (mouse_y > NTDESTY) && (mouse_y < NTDESTY+(NTHEIGHT*2))){
				TileX = (mouse_x-NTDESTX)/8;
				TileY = (mouse_y-NTDESTY)/8;
				sprintf(str,"X / Y: %0d / %0d",TileX,TileY);
				SetDlgItemText(hwndDlg,104,str);
				NameTable = (TileX/32)+((TileY/30)*2);
				PPUAddress = 0x2000+(NameTable*0x400)+((TileY%30)*32)+(TileX%32);
				sprintf(str,"PPU Address: %04X",PPUAddress);
				SetDlgItemText(hwndDlg,105,str);
				TileID = vnapage[(PPUAddress>>10)&0x3][PPUAddress&0x3FF];
				sprintf(str,"Tile ID: %02X",TileID);
				SetDlgItemText(hwndDlg,103,str);
			}

/*			if (((mouse_x >= PATTERNDESTX) && (mouse_x < (PATTERNDESTX+(PATTERNWIDTH*ZOOM)))) && (mouse_y >= PATTERNDESTY) && (mouse_y < (PATTERNDESTY+(PATTERNHEIGHT*ZOOM)))) {
				mouse_x = (mouse_x-PATTERNDESTX)/(8*ZOOM);
				mouse_y = (mouse_y-PATTERNDESTY)/(8*ZOOM);
				sprintf(str,"Tile: $%X%X",mouse_y,mouse_x);
				SetDlgItemText(hwndDlg,103,str);
				SetDlgItemText(hwndDlg,104,"Tile:");
				SetDlgItemText(hwndDlg,105,"Palettes");
			}
			else if (((mouse_x >= PATTERNDESTX+(PATTERNWIDTH*ZOOM)+1) && (mouse_x < (PATTERNDESTX+(PATTERNWIDTH*ZOOM)*2+1))) && (mouse_y >= PATTERNDESTY) && (mouse_y < (PATTERNDESTY+(PATTERNHEIGHT*ZOOM)))) {
				mouse_x = (mouse_x-(PATTERNDESTX+(PATTERNWIDTH*ZOOM)+1))/(8*ZOOM);
				mouse_y = (mouse_y-PATTERNDESTY)/(8*ZOOM);
				sprintf(str,"Tile: $%X%X",mouse_y,mouse_x);
				SetDlgItemText(hwndDlg,104,str);
				SetDlgItemText(hwndDlg,103,"Tile:");
				SetDlgItemText(hwndDlg,105,"Palettes");
			}
			else if (((mouse_x >= PALETTEDESTX) && (mouse_x < (PALETTEDESTX+PALETTEWIDTH))) && (mouse_y >= PALETTEDESTY) && (mouse_y < (PALETTEDESTY+PALETTEHEIGHT))) {
				mouse_x = (mouse_x-PALETTEDESTX)/32;
				mouse_y = (mouse_y-PALETTEDESTY)/32;
				sprintf(str,"Palette: $%02X",palcache[(mouse_y<<4)|mouse_x]);
				SetDlgItemText(hwndDlg,103,"Tile:");
				SetDlgItemText(hwndDlg,104,"Tile:");
				SetDlgItemText(hwndDlg,105,str);
			}
			else {
				SetDlgItemText(hwndDlg,103,"Tile:");
				SetDlgItemText(hwndDlg,104,"Tile:");
				SetDlgItemText(hwndDlg,105,"Palettes");
			}
*/
			break;
		case WM_NCACTIVATE:
			sprintf(str,"%d",NTViewScanline);
			SetDlgItemText(hwndDlg,102,str);
			break;
		case WM_COMMAND:
			switch(HIWORD(wParam)) {
				case EN_UPDATE:
					GetDlgItemText(hwndDlg,102,str,4);
					sscanf(str,"%d",&NTViewScanline);
					if (NTViewScanline > 239) NTViewScanline = 239;
					chrchanged = 1;
					break;
				case BN_CLICKED:
					switch(LOWORD(wParam)) {
						case 1001 :
						case 1002 :
						case 1003 :
						case 1004 :
						case 1005 :
						case 1006 :
						case 1007 :
							oldntmirroring = ntmirroring = LOWORD(wParam)-1001;
							ChangeMirroring();
							break;
						case 1008 : 
							scrolllines ^= 1;
							chrchanged = 1;
							break;
					}
					break;
			}
			break;
		case WM_HSCROLL:
			if (lParam) { //refresh trackbar
				NTViewRefresh = SendDlgItemMessage(hwndDlg,201,TBM_GETPOS,0,0);
			}
			break;
	}
	return FALSE;
}

void DoNTView() {
	if (!GameInfo) {
		FCEUD_PrintError("You must have a game loaded before you can use the Name Table Viewer.");
		return;
	}
	if (GameInfo->type==GIT_NSF) {
		FCEUD_PrintError("Sorry, you can't use the Name Table Viewer with NSFs.");
		return;
	}

	if (!hNTView) hNTView = CreateDialog(fceu_hInstance,"NTVIEW",NULL,NTViewCallB);
	if (hNTView) {
		SetWindowPos(hNTView,HWND_TOP,0,0,0,0,SWP_NOSIZE|SWP_NOMOVE|SWP_NOOWNERZORDER);
		FCEUD_UpdateNTView(-1,1);
		NTViewDoBlit(1);
	}
}

