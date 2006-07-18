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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include "scalebit.h"
#include "hq2x.h"
#include "hq3x.h"

#include "../../types.h"

static uint32 CBM[3];
static uint32 *palettetranslate=0;

static uint16 *specbuf=NULL;            // 8bpp -> 16bpp, pre hq2x/hq3x
static uint32 *specbuf32bpp = NULL;      // Buffer to hold output
                                        // of hq2x/hq3x when converting
                                        // to 16bpp and 24bpp
static int backBpp, backshiftr[3], backshiftl[3];
//static uint32 backmask[3];

static uint8 *specbuf8bpp = NULL;	// For 2xscale, 3xscale.


static int silt;

static int Bpp;	// BYTES per pixel
static int highefx;

#define BLUR_RED	20
#define BLUR_GREEN	20
#define BLUR_BLUE	10

#define FVB_SCANLINES	1

/* The blur effect is only available for bpp>=16.  It could be easily modified
   to look like what happens on the real NES and TV, but lack of decent
   synchronization to the vertical retrace period makes it look rather
   blah.
*/
#define FVB_BLUR	2

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


int InitBlitToHigh(int b, uint32 rmask, uint32 gmask, uint32 bmask, int efx, int specfilt)
{
 if(specfilt == 2 || specfilt == 4) // scale2x and scale3x
 {
  int multi;

  if(specfilt == 2) multi = 2 * 2;
  else if(specfilt == 4) multi = 3 * 3;

  specbuf8bpp = malloc(256*240*multi);

 }
 else if(specfilt == 1 || specfilt == 3) // hq2x and hq3x
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
   if(specfilt == 1) specbuf32bpp = malloc(256*240*4*sizeof(uint32));
   else if(specfilt == 3) specbuf32bpp = malloc(256*240*9*sizeof(uint32));
  }

  efx=0;
  b=2;
  rmask=0x1F<<11;
  gmask=0x3F<<5;
  bmask=0x1F;

  if(specfilt == 3)
   hq3x_InitLUTs();
  else
   hq2x_InitLUTs();

  specbuf=malloc(256*240*sizeof(uint16));
 }

 silt = specfilt;

 Bpp=b;

 highefx=efx;

 if(Bpp<=1 || Bpp>4)
  return(0);

 if(efx&FVB_BLUR)
 {
  if(Bpp==2)
   palettetranslate=(uint32 *)malloc(65536*4);
  else if(Bpp>=3)
   palettetranslate=(uint32 *)malloc(65536*4);
 }
 else
 {
  if(Bpp==2)
   palettetranslate=(uint32*)malloc(65536*4);
  else if(Bpp>=3)
   palettetranslate=(uint32*)malloc(256*4);
 }

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
  if(silt == 3)
   hq3x_Kill();
  else
   hq2x_Kill();
  specbuf=NULL;
 }
}


void SetPaletteBlitToHigh(uint8 *src)
{ 
             int cshiftr[3];
             int cshiftl[3];
             int x,y;

              CalculateShift(CBM, cshiftr, cshiftl);

 switch(Bpp)
 {
    case 2:
	     if(highefx&FVB_BLUR)
	     {
              for(x=0;x<256;x++)   
              {
               uint32 r,g,b;
	       for(y=0;y<256;y++)
               {
                r=src[x<<2]*(100-BLUR_RED);
                g=src[(x<<2)+1]*(100-BLUR_GREEN);
                b=src[(x<<2)+2]*(100-BLUR_BLUE);
  
                r+=src[y<<2]*BLUR_RED;
                g+=src[(y<<2)+1]*BLUR_GREEN;
                b+=src[(y<<2)+2]*BLUR_BLUE;
                r/=100;
		g/=100; 
		b/=100;

		if(r>255) r=255; if(g>255) g=255; if(b>255) b=255;
                palettetranslate[x|(y<<8)]=((r>>cshiftr[0])<<cshiftl[0])|
					   ((g>>cshiftr[1])<<cshiftl[1])|
					   ((b>>cshiftr[2])<<cshiftl[2]);
	       }
              }
	     }
	     else
             for(x=0;x<65536;x++)
             {
              uint16 lower,upper;

              lower=(src[((x&255)<<2)]>>cshiftr[0])<<cshiftl[0];
              lower|=(src[((x&255)<<2)+1]>>cshiftr[1])<<cshiftl[1];
              lower|=(src[((x&255)<<2)+2]>>cshiftr[2])<<cshiftl[2];
              upper=(src[((x>>8)<<2)]>>cshiftr[0])<<cshiftl[0];
              upper|=(src[((x>>8)<<2)+1]>>cshiftr[1])<<cshiftl[1];
              upper|=(src[((x>>8)<<2)+2]>>cshiftr[2])<<cshiftl[2];

              palettetranslate[x]=lower|(upper<<16);
             }
	    break;
    case 3:
    case 4:
            for(x=0;x<256;x++)
            {
	     uint32 r,g,b;

	     if(!(highefx&FVB_BLUR))
	     {
	      r=src[x<<2];
              g=src[(x<<2)+1];
              b=src[(x<<2)+2];
              palettetranslate[x]=(r<<cshiftl[0])|(g<<cshiftl[1])|(b<<cshiftl[2]);
	     }
	     else	
	     for(y=0;y<256;y++)
	     {
                r=src[x<<2]*(100-BLUR_RED);
                g=src[(x<<2)+1]*(100-BLUR_GREEN);
                b=src[(x<<2)+2]*(100-BLUR_BLUE);

                r+=src[y<<2]*BLUR_RED;
                g+=src[(y<<2)+1]*BLUR_GREEN;
                b+=src[(y<<2)+2]*BLUR_BLUE;
               
                r/=100;
                g/=100;
                b/=100;                  
                if(r>255) r=255; if(g>255) g=255; if(b>255) b=255;

                palettetranslate[x|(y<<8)]=(r<<cshiftl[0])|(g<<cshiftl[1])|(b<<cshiftl[2]);
	     }
            }
            break;
 }
}

static void Blit32to24(uint32 *src, uint8 *dest, int xr, int yr, int dpitch)
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


static void Blit32to16(uint32 *src, uint16 *dest, int xr, int yr, int dpitch,
        int shiftr[3], int shiftl[3])
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
 
 if(special==2)
 {
  if(xscale!=2 || yscale!=2) return;
  
  scale(2,dest,pitch,src,256,1,xr,yr);
  return;
 }

 if(special==4)
 {
  if(xscale!=3 || yscale!=3) return;
  scale(3,dest,pitch,src,256,1,xr,yr);
  return;
 }     

 pinc=pitch-(xr*xscale);
 if(xscale!=1 || yscale!=1)
 {
  if(efx&FVB_SCANLINES)
  {
   for(y=yr;y;y--,src+=256-xr)
   {
    int doo=yscale-(yscale>>1);
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
    //src-=xr*(yscale-(yscale>>1));
    dest+=pitch*(yscale>>1);

    src+=xr;
   }

  }
  else
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
 
 }
 else
 {
  for(y=yr;y;y--,dest+=pinc,src+=256-xr)
   for(x=xr;x;x-=4,dest+=4,src+=4)
    *(uint32 *)dest=*(uint32 *)src;
 }
}

/* Todo:  Make sure 24bpp code works right with big-endian cpus */

void Blit8ToHigh(uint8 *src, uint8 *dest, int xr, int yr, int pitch, 
		 int xscale, int yscale)
{
 int x,y;
 int pinc;
 uint8 *destbackup = NULL;	/* For hq2x */
 int pitchbackup = 0;

 //static int google=0;
 //google^=1;

 if(specbuf8bpp)        // 2xscale/3xscale
 {
  int mult; 
  int base;

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
 else if(specbuf)
 {
  destbackup=dest;
  dest=(uint8 *)specbuf;
  pitchbackup=pitch;

  pitch=xr*sizeof(uint16);
  xscale=1;
  yscale=1;
 }

 if(highefx&FVB_BLUR)	// DONE
 {
  if(xscale!=1 || yscale!=1 || (highefx&FVB_SCANLINES)) // DONE
  {
   switch(Bpp)
   {
    case 4:
     pinc=pitch-((xr*xscale)<<2);
     for(y=yr;y;y--,src+=256-xr)
     {
      int doo=yscale;   
     
      if(highefx&FVB_SCANLINES)
       doo-=yscale>>1;
      do
      {
       uint8 last=0x00;

       //if(doo == 1 && google) dest+=4;
       for(x=xr;x;x--,src++)
       {
        int too=xscale;
        do
        {
         *(uint32 *)dest=palettetranslate[*src|(last<<8)];
         dest+=4;
        } while(--too);
	last=*src;
       }
       //if(doo == 1 && google) dest-=4;
       src-=xr;
       dest+=pinc;
      } while(--doo);
      src+=xr;
      if(highefx&FVB_SCANLINES)
       dest+=pitch*(yscale>>1);
     }
     break;
  case 3:
     pinc=pitch-((xr*xscale)*3);
     for(y=yr;y;y--,src+=256-xr)
     {
      int doo=yscale;
 
      if(highefx&FVB_SCANLINES)
       doo-=yscale>>1;
      do
      {
       uint8 last=0x00;
       for(x=xr;x;x--,src++)
       {
        int too=xscale;
        do
        {  
         *(uint32 *)dest=palettetranslate[*src|(last<<8)];
         dest+=3;
        } while(--too);
	last=*src;
       }
       src-=xr;
       dest+=pinc;
      } while(--doo); 
      src+=xr;
      if(highefx&FVB_SCANLINES)
       dest+=pitch*(yscale>>1);
     }     
     break;

  case 2:
    pinc=pitch-((xr*xscale)<<1);

    for(y=yr;y;y--,src+=256-xr)
    {
     int doo=yscale;

     if(highefx& FVB_SCANLINES)
      doo-=yscale>>1;     
     do
     {
      uint8 last=0x00;
      for(x=xr;x;x--,src++)
      {
       int too=xscale;
       do
       {
        *(uint16 *)dest=palettetranslate[*src|(last<<8)];
        dest+=2;
       } while(--too);
       last=*src;
      }
     src-=xr;
     dest+=pinc;
     } while(--doo);
     src+=xr;
     if(highefx&FVB_SCANLINES) 
      dest+=pitch*(yscale>>1);
    }
    break;   
   }   
  }
  else // No scaling, no scanlines, just blurring. - DONE
  switch(Bpp)
  {
   case 4:   
    pinc=pitch-(xr<<2);
    for(y=yr;y;y--,src+=256-xr)
    {
     uint8 last=0x00;
     for(x=xr;x;x--)
     {
      *(uint32 *)dest=palettetranslate[*src|(last<<8)];
      last=*src;
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
     uint8 last=0x00;
     for(x=xr;x;x--)
     {
      uint32 tmp=palettetranslate[*src|(last<<8)];
      last=*src;
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
    uint8 last=0x00;
    for(x=xr;x;x--)
    {
     *(uint16 *)dest=palettetranslate[*src|(last<<8)];
     last=*src;
     dest+=2;
     src++;
    }
    dest+=pinc;
   }
   break;
  }
 }
 else	// No blur effects.
 {
  if(xscale!=1 || yscale!=1 || (highefx&FVB_SCANLINES))
  {
   switch(Bpp)
   {
    case 4:
    pinc=pitch-((xr*xscale)<<2);
    for(y=yr;y;y--,src+=256-xr)
    {
     int doo=yscale;
             
     if(highefx& FVB_SCANLINES)
      doo-=yscale>>1;
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
     if(highefx&FVB_SCANLINES)
      dest+=pitch*(yscale>>1);
    }  
    break;

   case 3:
    pinc=pitch-((xr*xscale)*3);
    for(y=yr;y;y--,src+=256-xr)
    {  
     int doo=yscale;
      
     if(highefx& FVB_SCANLINES)
      doo-=yscale>>1;
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
     if(highefx&FVB_SCANLINES)
      dest+=pitch*(yscale>>1);
    }
    break;

  case 2:
    pinc=pitch-((xr*xscale)<<1);
       
    for(y=yr;y;y--,src+=256-xr)
    {   
     int doo=yscale;
        
     if(highefx& FVB_SCANLINES)
      doo-=yscale>>1;
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
     if(highefx&FVB_SCANLINES)
      dest+=pitch*(yscale>>1);
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
      *(uint32 *)dest=palettetranslate[(uint32)*src];
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
      uint32 tmp=palettetranslate[(uint32)*src];
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
 }
 
 if(specbuf)
 {
  if(specbuf32bpp)
  {
   int mult = (silt == 3)?3:2;

   if(silt == 3)
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
   if(silt == 3)
    hq3x_32((uint8 *)specbuf,destbackup,xr,yr,pitchbackup);
   else
    hq2x_32((uint8 *)specbuf,destbackup,xr,yr,pitchbackup);
  }
 }
}
