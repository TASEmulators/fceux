/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
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

#include <stdlib.h>
#include <math.h>
#include "scalebit.h"
#include "hq2x.h"
#include "hq3x.h"
#include "fceu.h"

#include "../../types.h"
#include "../../palette.h"
#include "../../utils/memory.h"
#include "nes_ntsc.h"

extern u8 *XBuf;
extern u8 *XBackBuf;
extern u8 *XDBuf;
extern u8 *XDBackBuf;
extern pal *palo;

nes_ntsc_t* nes_ntsc;
uint8 burst_phase = 0;

static uint32 CBM[3];
static uint32 *palettetranslate=0;

static uint16 *specbuf=NULL;		// 8bpp -> 16bpp, pre hq2x/hq3x
static uint32 *specbuf32bpp = NULL;	// Buffer to hold output of hq2x/hq3x when converting to 16bpp and 24bpp
static int backBpp, backshiftr[3], backshiftl[3];
//static uint32 backmask[3];

static uint8  *specbuf8bpp = NULL;	// For 2xscale, 3xscale.
static uint8  *ntscblit    = NULL;	// For nes_ntsc
static uint32 *prescalebuf = NULL;	// Prescale pointresizes to 2x-4x to allow less blur with hardware acceleration.
static uint32 *palrgb      = NULL;	// PAL filter buffer for lookup values of RGB with applied moir phases
static uint32 *palrgb2     = NULL;	// PAL filter buffer for lookup values of blended moir phases
static float  *moire       = NULL;
int    palsaturation       = 100;
int    palnotch            = 100;
int    palsharpness        = 50;
bool   palhdtv             = 0;
bool   palmonochrome       = 0;
bool   palupdate           = 1;
bool   paldeemphswap       = 0;

static int silt;

static int Bpp;	// BYTES per pixel
static int highefx;

static int Round(float value)
{
   return (int) floor(value + 0.5);
}

static void CalculateShift(uint32 *CBM, int *cshiftr, int *cshiftl)
{
	int a,x,z,y;
	cshiftl[0]=cshiftl[1]=cshiftl[2]=-1;
	for(a=0;a<3;a++)
	{
		for(x=0,y=-1,z=0;x<32;x++)
		{
			if(CBM[a]&(1<<x))
			{
				if(cshiftl[a]==-1) cshiftl[a]=x;
				z++;
			}
		}
		cshiftr[a]=(8-z);
	}
}


int InitBlitToHigh(int b, uint32 rmask, uint32 gmask, uint32 bmask, int efx, int specfilt, int specfilteropt)
{
	paldeemphswap = 0;

	// -Video Modes Tag-
	if(specfilt == 3) // NTSC 2x
	{
		int multi = (2 * 2);		
		//nes_ntsc variables
		nes_ntsc_setup_t ntsc_setup = nes_ntsc_composite;
		
		switch (specfilteropt)
		{
		//case 0: // Composite
			//ntsc_setup = nes_ntsc_composite;
			//break;
		case 1: //S-Video
			ntsc_setup = nes_ntsc_svideo;
			break;
		case 2: //RGB
			ntsc_setup = nes_ntsc_rgb;
			break;
		case 3: //Monochrome
			ntsc_setup = nes_ntsc_monochrome;
			break;			
		}
		
		nes_ntsc = (nes_ntsc_t*) FCEU_dmalloc( sizeof (nes_ntsc_t) );
		
		if ( nes_ntsc )
		{
			nes_ntsc_init( nes_ntsc, &ntsc_setup, b, 2 );			
			ntscblit = (uint8*)FCEU_dmalloc(256*257*b*multi); //Need to add multiplier for larger sizes
		}
		
	} // -Video Modes Tag-
	else if(specfilt == 2 || specfilt == 5) // scale2x and scale3x
	{
		int multi = ((specfilt == 2) ? 2 * 2 : 3 * 3);		
		specbuf8bpp = (uint8*)FCEU_dmalloc(256*240*multi); //mbg merge 7/17/06 added cast		
	} // -Video Modes Tag-
	else if(specfilt == 1 || specfilt == 4) // hq2x and hq3x
	{ 
		if(b == 1) 
			return(0);
		
		if(b == 2 || b == 3)          // 8->16->(hq2x)->32-> 24 or 16.  YARGH.
		{
			uint32 tmpCBM[3];
			backBpp = b;
			tmpCBM[0]=rmask;
			tmpCBM[1]=gmask;
			tmpCBM[2]=bmask;
			
			CalculateShift(tmpCBM, backshiftr, backshiftl);
			
			if(b == 2)
			{
				// ark
				backshiftr[0] += 16;
				backshiftr[1] += 8;
				backshiftr[2] += 0;
				
				// Begin iffy code(requires 16bpp and 32bpp to have same RGB order)
				//backmask[0] = (rmask>>backshiftl[0]) << (backshiftr[0]);
				//backmask[1] = (gmask>>backshiftl[1]) << (backshiftr[1]);
				//backmask[2] = (bmask>>backshiftl[2]) << (backshiftr[2]);
				
				//int x;
				//for(x=0;x<3;x++) 
				// backshiftr[x] -= backshiftl[x];
				// End iffy code
			}
			// -Video Modes Tag-
			if(specfilt == 1)
				specbuf32bpp = (uint32*)FCEU_dmalloc(256*240*4*sizeof(uint32)); //mbg merge 7/17/06 added cast
			else if(specfilt == 4)
				specbuf32bpp = (uint32*)FCEU_dmalloc(256*240*9*sizeof(uint32)); //mbg merge 7/17/06 added cast
		}
		
		efx=0;
		b=2;
		rmask=0x1F<<11;
		gmask=0x3F<<5;
		bmask=0x1F;
		
		// -Video Modes Tag-
		if(specfilt == 4)
			hq3x_InitLUTs();
		else
			hq2x_InitLUTs();
		
		specbuf=(uint16*)FCEU_dmalloc(256*240*sizeof(uint16)); //mbg merge 7/17/06 added cast
	}
	else if (specfilt >= 6 && specfilt <= 8)
	{
		int multi = specfilt - 4; // magic assuming prescales are specfilt >= 6
		prescalebuf = (uint32 *)FCEU_dmalloc(256*240*multi*sizeof(uint32));
	}
	else if (specfilt == 9)
	{
		palrgb     = (uint32 *)FCEU_dmalloc((256+512)*16*sizeof(uint32));
		palrgb2    = (uint32 *)FCEU_dmalloc((256+512)*16*sizeof(uint32));
		moire      = (float  *)FCEU_dmalloc(          16*sizeof(float));
		paldeemphswap = 1;
	}

	silt = specfilt;	
	Bpp=b;	
	highefx=efx;
	
	if(Bpp<=1 || Bpp>4)
		return(0);
	
	//allocate adequate room for 32bpp palette
	palettetranslate=(uint32*)FCEU_dmalloc(256*4 + 512*4);
	
	if(!palettetranslate)
		return(0);
	
	
	CBM[0]=rmask;
	CBM[1]=gmask;
	CBM[2]=bmask;
	return(1);
}

void KillBlitToHigh(void)
{
	if(palettetranslate)
	{
		free(palettetranslate);
		palettetranslate=NULL;
	}
	
	if(specbuf8bpp)
	{
		free(specbuf8bpp);
		specbuf8bpp = NULL;
	}
	if(specbuf32bpp)
	{
		free(specbuf32bpp);
		specbuf32bpp = NULL;
	}
	if(specbuf)
	{
	// -Video Modes Tag-
		if(silt == 4)
			hq3x_Kill();
		else
			hq2x_Kill();
		specbuf=NULL;
	}
	if (nes_ntsc) {
		free(nes_ntsc);
		nes_ntsc = NULL;
	}
	if (ntscblit) {
		free(ntscblit);
		ntscblit = NULL;
	}
	if (prescalebuf) {
		free(prescalebuf);
		prescalebuf = NULL;
	}
	if (palrgb) {
		free(palrgb);
		palrgb = NULL;
		free(palrgb2);
		palrgb2 = NULL;
		free(moire);
		moire = NULL;
	}
}


void SetPaletteBlitToHigh(uint8 *src)
{ 
	int cshiftr[3];
	int cshiftl[3];
	
	CalculateShift(CBM, cshiftr, cshiftl);

	switch(Bpp)
	{
	case 2:
		for(int x=0;x<256;x++)
		{
			uint32 r = src[x<<2];
			uint32 g = src[(x<<2)+1];
			uint32 b = src[(x<<2)+2];
			u16 color = (r>>cshiftr[0])<<cshiftl[0];
			color |= (g>>cshiftr[1])<<cshiftl[1];
			color |= (b>>cshiftr[2])<<cshiftl[2];
			palettetranslate[x]=color;
		}

		//full size deemph palette
		if(palo)
		{
			for(int x=0;x<512;x++)
			{
				uint32 r=palo[x].r;
				uint32 g=palo[x].g;
				uint32 b=palo[x].b;
				u16 color = (r>>cshiftr[0])<<cshiftl[0];
				color |= (g>>cshiftr[1])<<cshiftl[1];
				color |= (b>>cshiftr[2])<<cshiftl[2];
				palettetranslate[256+x]=color;
			}
		}

		break;

	case 3:
	case 4:
		for(int x=0;x<256;x++)
		{
			uint32 r=src[x<<2];
			uint32 g=src[(x<<2)+1];
			uint32 b=src[(x<<2)+2];
			palettetranslate[x]=(r<<cshiftl[0])|(g<<cshiftl[1])|(b<<cshiftl[2]);
		}

		//full size deemph palette
		if(palo)
		{
			for(int x=0;x<512;x++)
			{
				uint32 r=palo[x].r;
				uint32 g=palo[x].g;
				uint32 b=palo[x].b;
				palettetranslate[256+x]=(r<<cshiftl[0])|(g<<cshiftl[1])|(b<<cshiftl[2]);
			}
		}

		break;
	}
}

void Blit32to24(uint32 *src, uint8 *dest, int xr, int yr, int dpitch)
{
	int x,y;
	
	for(y=yr;y;y--)
	{
		for(x=xr;x;x--)
		{
			uint32 tmp = *src;
			*dest = tmp;
			dest++;
			*dest = tmp>>8;
			dest++;
			*dest = tmp>>16;
			dest++;
			src++;
		}
		dest += dpitch / 3 - xr;
	}
}

void Blit32to16(uint32 *src, uint16 *dest, int xr, int yr, int dpitch, int shiftr[3], int shiftl[3])
{
	int x,y;
	//printf("%d\n",shiftl[1]);
	for(y=yr;y;y--)
	{
		for(x=xr;x;x--)
		{
			uint32 tmp = *src;
			uint16 dtmp;
			
			// Begin iffy code
			//dtmp = (tmp & backmask[2]) >> shiftr[2];
			//dtmp |= (tmp & backmask[1]) >> shiftr[1];
			//dtmp |= (tmp & backmask[0]) >> shiftr[0];
			// End iffy code
			
			// Begin non-iffy code
			dtmp =  ((tmp&0x0000FF) >> shiftr[2]) << shiftl[2];
			dtmp |= ((tmp&0x00FF00) >> shiftr[1]) << shiftl[1];
			dtmp |= ((tmp&0xFF0000) >> shiftr[0]) << shiftl[0];
			// End non-iffy code
			
			//dtmp = ((tmp&0x0000FF) >> 3);
			//dtmp |= ((tmp&0x00FC00) >>5);
			//dtmp |= ((tmp&0xF80000) >>8);
			
			*dest = dtmp;
			src++;
			dest++;
		}
		dest += dpitch / 2 - xr;
	}
}


void Blit8To8(uint8 *src, uint8 *dest, int xr, int yr, int pitch, int xscale, int yscale, int efx, int special)
{
	int x,y;
	int pinc;
	
	// -Video Modes Tag-
	if(special==3) //NTSC 2x
		return; //Incompatible with 8-bit output. This is here for SDL.
	
	// -Video Modes Tag-
	if(special==2)
	{
		if(xscale!=2 || yscale!=2) return;
		
		scale(2,dest,pitch,src,256,1,xr,yr);
		return;
	}
	
	// -Video Modes Tag-
	if(special==5)
	{
		if(xscale!=3 || yscale!=3) return;
		scale(3,dest,pitch,src,256,1,xr,yr);
		return;
	}     
	
	pinc=pitch-(xr*xscale);
	if(xscale!=1 || yscale!=1)
	{
		for(y=yr;y;y--,src+=256-xr)
		{
			int doo=yscale;
			do
			{
				for(x=xr;x;x--,src++)
				{
					int too=xscale;
					do
					{
						*(uint8 *)dest=*(uint8 *)src;
						dest++;
					} while(--too);
				}
				src-=xr;
				dest+=pinc;
			} while(--doo);
			src+=xr;
		}
	}
	else
	{
		for(y=yr;y;y--,dest+=pinc,src+=256-xr)
			for(x=xr;x;x-=4,dest+=4,src+=4)
				*(uint32 *)dest=*(uint32 *)src;
	}
}

/* Todo:  Make sure 24bpp code works right with big-endian cpus */

//takes a pointer to XBuf and applies fully modern deemph palettizing
u32 ModernDeemphColorMap(u8* src)
{
	u8 pixel = *src;
	
	//look up the legacy translation
	u32 color = palettetranslate[pixel];
	
	//find out which deemph bitplane value we're on
	int ofs = src-XBuf;
	uint8 deemph = XDBuf[ofs];

	//if it was a deemph'd value, grab it from the deemph palette
	if(deemph != 0)
		color = palettetranslate[256+(pixel&0x3F)+deemph*64];

	return color;
}

int PAL_LUT(uint32 *buffer, int index, int x, int y)
{
	int color = 0;

	switch (y&3)
	{
	case 0:
		switch (x&3)
		{
			case 0: color = buffer[index*16   ]; break;
			case 1: color = buffer[index*16+ 1]; break;
			case 2: color = buffer[index*16+ 2]; break;
			case 3: color = buffer[index*16+ 3]; break;
		}
		break;
	case 1:
		switch (x&3)
		{
			case 0: color = buffer[index*16+ 4]; break;
			case 1: color = buffer[index*16+ 5]; break;
			case 2: color = buffer[index*16+ 6]; break;
			case 3: color = buffer[index*16+ 7]; break;
		}
		break;
	case 2:
		switch (x&3)
		{
			case 0: color = buffer[index*16+ 8]; break;
			case 1: color = buffer[index*16+ 9]; break;
			case 2: color = buffer[index*16+10]; break;
			case 3: color = buffer[index*16+11]; break;
		}
		break;
	case 3:
		switch (x&3)
		{
			case 0: color = buffer[index*16+12]; break;
			case 1: color = buffer[index*16+13]; break;
			case 2: color = buffer[index*16+14]; break;
			case 3: color = buffer[index*16+15]; break;
		}
		break;
	}

	return color;
}

void Blit8ToHigh(uint8 *src, uint8 *dest, int xr, int yr, int pitch, int xscale, int yscale)
{
	int x,y;
	int pinc;
	uint8 *destbackup = NULL;	/* For hq2x */
	int pitchbackup = 0;
	
	//static int google=0;
	//google^=1;
	
	if(specbuf8bpp)                  // 2xscale/3xscale
	{
		int mult; 
		int base;
		
		// -Video Modes Tag-
		if(silt == 2) mult = 2;
		else mult = 3;
		
		Blit8To8(src, specbuf8bpp, xr, yr, 256*mult, xscale, yscale, 0, silt);
		
		xr *= mult;
		yr *= mult;
		xscale=yscale=1;
		src = specbuf8bpp;
		base = 256*mult;
		
		switch(Bpp)
		{
		case 4:
			pinc=pitch-(xr<<2);
			for(y=yr;y;y--,src+=base-xr)
			{
				for(x=xr;x;x--)
				{
				 *(uint32 *)dest=palettetranslate[(uint32)*src];
				 dest+=4;
				 src++;
				}
				dest+=pinc;
			}
			break;
		case 3:
			pinc=pitch-(xr+xr+xr);
			for(y=yr;y;y--,src+=base-xr)
			{
				for(x=xr;x;x--)
				{
					uint32 tmp=palettetranslate[(uint32)*src];
					*(uint8 *)dest=tmp;
					*((uint8 *)dest+1)=tmp>>8;
					*((uint8 *)dest+2)=tmp>>16;
					dest+=3;
					src++;
					src++;
				}
				dest+=pinc;
			}
			break; 
		case 2:
			pinc=pitch-(xr<<1);
			
			for(y=yr;y;y--,src+=base-xr)
			{
				for(x=xr>>1;x;x--)
				{
					*(uint32 *)dest=palettetranslate[*(uint16 *)src];
					dest+=4;
					src+=2;
				}
				dest+=pinc;
			}
			break;
		}
		return;
	}
	else if(prescalebuf)             // bare prescale
	{
		destbackup = dest;
		dest = (uint8 *)prescalebuf;
		pitchbackup = pitch;		
		pitch = xr*sizeof(uint32);
		pinc = pitch-(xr<<2);

		for(y=yr; y; y--, src+=256-xr)
		{
			for(x=xr; x; x--)
			{
				*(uint32 *)dest = palettetranslate[(uint32)*src];
				dest += 4;
				src++;
			}
			dest += pinc;
		}

		if (Bpp == 4) // are other modes really needed?
		{
			int mult = silt - 4; // magic assuming prescales are silt >= 6
			uint32 *s = prescalebuf;
			uint32 *d = (uint32 *)destbackup; // use 32-bit pointers ftw

			for (y=0; y<yr*yscale; y++)
			{
				for (x=0; x<xr; x++)
				{
					for (int subpixel=1; subpixel<=mult; subpixel++)
					{
						*d++ = *s++;
						if (subpixel < mult)
							s--; // repeat subpixel
					}
				}
				if (x == 256 && (y+1)%mult != 0)
					s -= 256; // repeat scanline
			}
		}
		return;
	}
	else if (palrgb)                 // pal moire
	{
		// skip usual palette translation, fill lookup array of RGB+moire values per palette update, and send directly to DX dest.
		// hardcoded resolution is 768x240, makes moire mask cleaner, even though PAL consoles generate it at native res.
		// source of this whole idea: http://forum.emu-russia.net/viewtopic.php?p=9410#p9410
		if (palupdate)
		{
			uint8 *source = (uint8 *)palettetranslate;
			int16 R,G,B;
			float Y,U,V;
			float sat = (float) palsaturation/100;
			bool hdtv = palhdtv;
			bool monochrome = palmonochrome;
			int notch = palnotch;
			int unnotch = 100 - palnotch;
			int mixR[16], mixG[16], mixB[16];

			for (int i=0; i<256+512; i++)
			{
				R = source[i*4  ];
				G = source[i*4+1];
				B = source[i*4+2];
			
				if (hdtv) // HDTV BT.709
				{
					Y =  0.2126 *R + 0.7152 *G + 0.0722 *B; // Y'
					U = -0.09991*R - 0.33609*G + 0.436  *B; // B-Y
					V =  0.615  *R - 0.55861*G - 0.05639*B; // R-Y
				}
				else // SDTV BT.601
				{
					Y =  0.299  *R + 0.587  *G + 0.114  *B;
					U = -0.14713*R - 0.28886*G + 0.436  *B;
					V =  0.615  *R - 0.51499*G - 0.10001*B;
				}

				if (Y == 0) Y = 1;
				if (monochrome) sat = 0;

				// WARNING: phase order is magical!
				moire[0]  = (U == 0 && V == 0) ? 1 : (Y + V)/Y;
				moire[1]  = (U == 0 && V == 0) ? 1 : (Y + U)/Y;
				moire[2]  = (U == 0 && V == 0) ? 1 : (Y - V)/Y;
				moire[3]  = (U == 0 && V == 0) ? 1 : (Y - U)/Y;
				moire[4]  = (U == 0 && V == 0) ? 1 : (Y - V)/Y;
				moire[5]  = (U == 0 && V == 0) ? 1 : (Y + U)/Y;
				moire[6]  = (U == 0 && V == 0) ? 1 : (Y + V)/Y;
				moire[7]  = (U == 0 && V == 0) ? 1 : (Y - U)/Y;
				moire[8]  = (U == 0 && V == 0) ? 1 : (Y - V)/Y;
				moire[9]  = (U == 0 && V == 0) ? 1 : (Y - U)/Y;
				moire[10] = (U == 0 && V == 0) ? 1 : (Y + V)/Y;
				moire[11] = (U == 0 && V == 0) ? 1 : (Y + U)/Y;
				moire[12] = (U == 0 && V == 0) ? 1 : (Y + V)/Y;
				moire[13] = (U == 0 && V == 0) ? 1 : (Y - U)/Y;
				moire[14] = (U == 0 && V == 0) ? 1 : (Y - V)/Y;
				moire[15] = (U == 0 && V == 0) ? 1 : (Y + U)/Y;
				
				for (int j=0; j<16; j++)
				{
					if (hdtv) // HDTV BT.709
					{
						R = Round((Y                 + 1.28033*V*sat)*moire[j]);
						G = Round((Y - 0.21482*U*sat - 0.38059*V*sat)*moire[j]);
						B = Round((Y + 2.12798*U*sat                )*moire[j]);
					}
					else // SDTV BT.601
					{
						R = Round((Y                 + 1.13983*V*sat)*moire[j]);
						G = Round((Y - 0.39465*U*sat - 0.58060*V*sat)*moire[j]);
						B = Round((Y + 2.03211*U*sat                )*moire[j]);
					}

					if (R > 0xff) R = 0xff; else if (R < 0) R = 0;
					if (G > 0xff) G = 0xff; else if (G < 0) G = 0;
					if (B > 0xff) B = 0xff; else if (B < 0) B = 0;

					mixR[j] = R;
					mixG[j] = G;
					mixB[j] = B;

					palrgb[i*16+j] = (B<<16)|(G<<8)|R;
				}

				for (int j=0; j<16; j++)
				{
					R = (mixR[j]*unnotch + (mixR[0]+mixR[1]+mixR[2]+mixR[3])/4*notch)/100;
					G = (mixG[j]*unnotch + (mixG[0]+mixG[1]+mixG[2]+mixG[3])/4*notch)/100;
					B = (mixB[j]*unnotch + (mixB[0]+mixB[1]+mixB[2]+mixB[3])/4*notch)/100;

					palrgb2[i*16+j] = (B<<16)|(G<<8)|R;
				}
			}
			palupdate = 0;
		}

		if (Bpp == 4)
		{
			uint32 *d = (uint32 *)dest;
			uint8  xsub      = 0;
			uint8  xabs      = 0;
			uint32 index     = 0;
			uint32 lastindex = 0;
			uint32 newindex  = 0;
			int sharp = palsharpness;
			int unsharp = 100 - palsharpness;
			int rmask   = 0xff0000;
			int gmask   = 0x00ff00;
			int bmask   = 0x0000ff;
			int r, g, b, ofs;
			uint8 deemph;
			uint32 color, moirecolor, notchcolor, finalcolor, lastcolor = 0;

			for (y=0; y<yr; y++)
			{
				for (x=0; x<xr; x++)
				{
					ofs = src-XBuf;                  //find out which deemph bitplane value we're on
					deemph = XDBuf[ofs];
					int temp = *src;
					index = (*src&63) | (deemph*64); //get combined index from basic value and preemph bitplane
					index += 256;

					src++;
					
					ofs = src-XBuf;
					deemph = XDBuf[ofs];
					newindex = (*src&63) | (deemph*64);
					newindex += 256;

					if(GameInfo->type==GIT_NSF)
					{
						*d++ = palettetranslate[temp];
						*d++ = palettetranslate[temp];
						*d++ = palettetranslate[temp];
					}
					else
					for (int xsub = 0; xsub < 3; xsub++)
					{
						xabs = x*3 + xsub;
						moirecolor = PAL_LUT(palrgb,  index, xabs, y);
						notchcolor = PAL_LUT(palrgb2, index, xabs, y);

						// | | |*|*| | |
						if (index !=  newindex && xsub == 2 ||
							index != lastindex && xsub == 0)
						{
							color = moirecolor;
						}
						// | |*| | |*| |
						else if (index !=  newindex && xsub == 1 ||
							     index != lastindex && xsub == 1)
						{
							r = ((moirecolor&rmask)*70 + (notchcolor&rmask)*30)/100;
							g = ((moirecolor&gmask)*70 + (notchcolor&gmask)*30)/100;
							b = ((moirecolor&bmask)*70 + (notchcolor&bmask)*30)/100;
							color = r&rmask | g&gmask | b&bmask;
						}
						// |*| | | | |*|
						else if (index !=  newindex && xsub == 0 ||
							     index != lastindex && xsub == 2)
						{
							r = ((moirecolor&rmask)*30 + (notchcolor&rmask)*70)/100;
							g = ((moirecolor&gmask)*30 + (notchcolor&gmask)*70)/100;
							b = ((moirecolor&bmask)*30 + (notchcolor&bmask)*70)/100;
							color = r&rmask | g&gmask | b&bmask;
						}
						else
						{
							color = notchcolor;
						}

						if (color != lastcolor && sharp < 100)
						{
							r = ((color&rmask)*sharp + (lastcolor&rmask)*unsharp)/100;
							g = ((color&gmask)*sharp + (lastcolor&gmask)*unsharp)/100;
							b = ((color&bmask)*sharp + (lastcolor&bmask)*unsharp)/100;
							finalcolor = r&rmask | g&gmask | b&bmask;

							r = ((lastcolor&rmask)*sharp + (color&rmask)*unsharp)/100;
							g = ((lastcolor&gmask)*sharp + (color&gmask)*unsharp)/100;
							b = ((lastcolor&bmask)*sharp + (color&bmask)*unsharp)/100;
							lastcolor = r&rmask | g&gmask | b&bmask;

							*d-- = lastcolor;
							d++;
						}
						else
							finalcolor = color;

						lastcolor = *d++ = finalcolor;
					}
					lastindex = index;
				}
			}

		}
		return;
	}
	else if(specbuf)                 // hq2x/hq3x
	{
		destbackup=dest;
		dest=(uint8 *)specbuf;
		pitchbackup=pitch;
		
		pitch=xr*sizeof(uint16);
		xscale=1;
		yscale=1;
	}
	
	{
		if(xscale!=1 || yscale!=1)
		{
			switch(Bpp)
			{
			case 4:
				if ( nes_ntsc && GameInfo->type!=GIT_NSF) {
					burst_phase ^= 1;
					nes_ntsc_blit( nes_ntsc, (unsigned char*)src, xr, burst_phase, xr, yr, ntscblit, xr * Bpp * xscale );
					
					//Multiply 4 by the multiplier on output, because it's 4 bpp
					//Top 2 lines = line 3, due to distracting flicker
					//memcpy(dest,ntscblit+(Bpp * xscale)+(Bpp * xr * xscale),(Bpp * xr * xscale));
					//memcpy(dest+(Bpp * xr * xscale),ntscblit+(Bpp * xscale)+(Bpp * xr * xscale * 2),(Bpp * xr * xscale));
					memcpy(dest+(Bpp * xr * xscale),ntscblit+(Bpp * xscale),(xr*yr*Bpp*xscale*yscale));
				} else {
					pinc=pitch-((xr*xscale)<<2);
					for(y=yr;y;y--,src+=256-xr)
					{
						int doo=yscale;
						        
						do
						{
							for(x=xr;x;x--,src++)
							{
								int too=xscale;
								do
								{
									*(uint32 *)dest=palettetranslate[*src];
									dest+=4;
								} while(--too);
							}
							src-=xr;
							dest+=pinc;
						} while(--doo);
						src+=xr;
					}
				}
				break;
			
			case 3:
				pinc=pitch-((xr*xscale)*3);
				for(y=yr;y;y--,src+=256-xr)
				{  
					int doo=yscale;
					 
					do
					{
						for(x=xr;x;x--,src++)
						{    
							int too=xscale;
							do
							{
								uint32 tmp=palettetranslate[(uint32)*src];
								*(uint8 *)dest=tmp;
								*((uint8 *)dest+1)=tmp>>8;
								*((uint8 *)dest+2)=tmp>>16;
								dest+=3;
								
								//*(uint32 *)dest=palettetranslate[*src];
								//dest+=4;
							} while(--too);
						}
						src-=xr;
						dest+=pinc;
					} while(--doo);
					src+=xr;
				}
				break;
						
			case 2:
				pinc=pitch-((xr*xscale)<<1);
				   
				for(y=yr;y;y--,src+=256-xr)
				{   
					int doo=yscale;
					   
					do
					{
						for(x=xr;x;x--,src++)
						{
							int too=xscale;
							do
							{
								*(uint16 *)dest=palettetranslate[*src];
								dest+=2;
							} while(--too);
						}
					src-=xr;
					dest+=pinc;
					} while(--doo);
					src+=xr;
				}  
				break;
			}
		}
		else
			switch(Bpp)
			{
			case 4:
				pinc=pitch-(xr<<2);
				for(y=yr;y;y--,src+=256-xr)
				{
					for(x=xr;x;x--)
					{
						//THE MAIN BLITTING CODEPATH (there may be others that are important)
						*(uint32 *)dest = ModernDeemphColorMap(src);
						dest+=4;
						src++;
					}
					dest+=pinc;
				}
				break;
			case 3:
				pinc=pitch-(xr+xr+xr);
				for(y=yr;y;y--,src+=256-xr)
				{
					for(x=xr;x;x--)
					{     
						uint32 tmp = ModernDeemphColorMap(src);
						*(uint8 *)dest=tmp;
						*((uint8 *)dest+1)=tmp>>8;
						*((uint8 *)dest+2)=tmp>>16;
						dest+=3;
						src++;
					}
					dest+=pinc;
				}
				break;
			case 2:
				pinc=pitch-(xr<<1);
				for(y=yr;y;y--,src+=256-xr)
				{
					for(x=xr;x;x--)
					{
						*(uint16 *)dest = ModernDeemphColorMap(src);
						dest+=2;
						src++;
					}
					dest+=pinc;
				}
				break;
			}
	}
	
	if(specbuf)
	{
		if(specbuf32bpp)
		{
			// -Video Modes Tag-
			int mult = (silt == 4)?3:2;
			
			if(silt == 4)
				hq3x_32((uint8 *)specbuf,(uint8*)specbuf32bpp,xr,yr,xr*3*sizeof(uint32));
			else
				hq2x_32((uint8 *)specbuf,(uint8*)specbuf32bpp,xr,yr,xr*2*sizeof(uint32));
			
			if(backBpp == 2)
				Blit32to16(specbuf32bpp, (uint16*)destbackup, xr*mult, yr*mult, pitchbackup, backshiftr,backshiftl);
			else // == 3
				Blit32to24(specbuf32bpp, (uint8*)destbackup, xr*mult, yr*mult, pitchbackup);
		}
		else
		{
			// -Video Modes Tag-
			if(silt == 4)
				hq3x_32((uint8 *)specbuf,destbackup,xr,yr,pitchbackup);
			else
				hq2x_32((uint8 *)specbuf,destbackup,xr,yr,pitchbackup);
		}
	}
}
