/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2017 CaH4e3
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
 *
 * HP10xx/HP20xx - a simplified version of FK23C mapper with pretty strict and better
 * organized banking behaviour. It seems that some 176 mapper assigned game may be
 * actually this kind of board instead but in common they aren't compatible at all,
 * the games on the regular FK23C boards couldn't run on this mapper and vice versa...
 *
 */

#include "mapinc.h"
#include "mmc3.h"
#include "../ines.h"

static uint8 unromchr, lock;
static uint32 dipswitch;

static void BMCHPxxCW(uint32 A, uint8 V)
{
	if(EXPREGS[0]&4) {		// custom banking
		switch(EXPREGS[0]&3) {
		case 0:
		case 1:
			setchr8(EXPREGS[2]&0x3F);
//			FCEU_printf("\tCHR8 %02X\n",EXPREGS[2]&0x3F);
			break;
		case 2:
			setchr8((EXPREGS[2]&0x3E)|(unromchr&1));
//			FCEU_printf("\tCHR8 %02X\n",(EXPREGS[2]&0x3E)|(unromchr&1));
			break;
		case 3:
			setchr8((EXPREGS[2]&0x3C)|(unromchr&3));
//			FCEU_printf("\tCHR8 %02X\n",(EXPREGS[2]&0x3C)|(unromchr&3));
			break;
		}
	} else {				// mmc3 banking
		int base, mask;
		if(EXPREGS[0]&1) {	// 128K mode
			base=EXPREGS[2]&0x30; 
			mask=0x7F;
		} else {			// 256K mode
			base=EXPREGS[2]&0x20;
			mask=0xFF;
		}
//		FCEU_printf("\tCHR1 %04x:%02X\n",A,(V&mask)|(base<<3));
		setchr1(A,(V&mask)|(base<<3));
	}
}

//PRG wrapper
static void BMCHPxxPW(uint32 A, uint8 V)
{
	if(EXPREGS[0]&4) {		// custom banking
		if((EXPREGS[0]&0xF)==4) {	// 16K mode
//			FCEU_printf("\tPRG16 %02X\n",EXPREGS[1]&0x1F);
			setprg16(0x8000,EXPREGS[1]&0x1F);
			setprg16(0xC000,EXPREGS[1]&0x1F);
		} else {			// 32K modes
//			FCEU_printf("\tPRG32 %02X\n",(EXPREGS[1]&0x1F)>>1);
			setprg32(0x8000,(EXPREGS[1]&0x1F)>>1);
		}
	} else {				// mmc3 banking
		int base, mask;
		if(EXPREGS[0]&2) {	// 128K mode
			base=EXPREGS[1]&0x18;
			mask=0x0F;
		} else {			// 256K mode
			base=EXPREGS[1]&0x10;
			mask=0x1F;
		}
//		FCEU_printf("\tPRG8 %02X\n",(V&mask)|(base<<1));
		setprg8(A,(V&mask)|(base<<1));
		setprg8r(0x10,0x6000,A001B&3);
	}
}

//MIRROR wrapper
static void BMCHPxxMW(uint8 V) {
	if(EXPREGS[0]&4) {		// custom banking
//		FCEU_printf("CUSTOM MIRR: %d\n",(unromchr>>2)&1);
		setmirror(((unromchr>>2)&1)^1);
	} else {				// mmc3 banking
//		FCEU_printf("MMC3 MIRR: %d\n",(V&1)^1);
		A000B = V;
		setmirror((A000B & 1) ^ 1);
	}
}

//PRG handler ($8000-$FFFF)
static DECLFW(BMCHPxxHiWrite)
{
//	FCEU_printf("HI WRITE %04X:%02X\n",A,V);
	if(EXPREGS[0]&4) {		// custom banking
//		FCEU_printf("CUSTOM\n");
		unromchr=V;
		FixMMC3CHR(MMC3_cmd);
	} else {				// mmc3 banking
//		FCEU_printf("MMC3\n");
		if(A<0xC000) {
			MMC3_CMDWrite(A,V);
			FixMMC3PRG(MMC3_cmd);
			FixMMC3CHR(MMC3_cmd);
		} else {
			MMC3_IRQWrite(A,V);
		}
	}
}

//EXP handler ($5000-$5FFF)
static DECLFW(BMCHPxxWrite)
{
	if(!lock) {
//		FCEU_printf("LO WRITE %04X:%02X\n",A,V);
		EXPREGS[A&3]=V;
		lock=V&0x80;
		FixMMC3PRG(MMC3_cmd);
		FixMMC3CHR(MMC3_cmd);
	}
}

static DECLFR(BMCHPxxRead) {
	return dipswitch;
}

static void BMCHPxxReset(void)
{
	dipswitch++;
	dipswitch&=0xF;
	lock=0;
//	FCEU_printf("BMCHPxx dipswitch set to %d\n",dipswitch);
	EXPREGS[0]=EXPREGS[1]=EXPREGS[2]=EXPREGS[3]=0;
	MMC3RegReset();
	FixMMC3PRG(MMC3_cmd);
	FixMMC3CHR(MMC3_cmd);
}

static void BMCHPxxPower(void)
{
	GenMMC3Power();
	dipswitch=lock=0;
	EXPREGS[0]=EXPREGS[1]=EXPREGS[2]=EXPREGS[3]=0;
	FixMMC3PRG(MMC3_cmd);
	FixMMC3CHR(MMC3_cmd);
	SetReadHandler(0x5000,0x5fff,BMCHPxxRead);
	SetWriteHandler(0x5000,0x5fff,BMCHPxxWrite);
	SetWriteHandler(0x8000,0xffff,BMCHPxxHiWrite);
}

void BMCHPxx_Init(CartInfo *info)
{
	GenMMC3_Init(info, 256, 256, 8, 0);
	cwrap=BMCHPxxCW;
	pwrap=BMCHPxxPW;
	mwrap=BMCHPxxMW;
	info->Power=BMCHPxxPower;
	info->Reset=BMCHPxxReset;
	AddExState(EXPREGS, 8, 0, "EXPR");
	AddExState(&unromchr, 1, 0, "UCHR");
	AddExState(&dipswitch, 1, 0, "DPSW");
	AddExState(&lock, 1, 0, "LOCK");
}
