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

#include "video.h"

static int RecalcCustom(void);

static DDCAPS caps;
static int mustrestore=0;
static DWORD CBM[3];

static int bpp;
static int vflags;
static int veflags;

int disvaccel = 0;      /* Disable video hardware acceleration. */

int fssync=0;
int winsync=0;

#ifdef _USE_SHARED_MEMORY_
PALETTEENTRY *color_palette;  // shared memory changes
HANDLE mapColorPalette;
#else
PALETTEENTRY color_palette[256];
#endif //_USE_SHARED_MEMORY_

static int PaletteChanged=0;

LPDIRECTDRAWCLIPPER lpClipper=0;
LPDIRECTDRAW  lpDD=0;
LPDIRECTDRAW7 lpDD7=0;
LPDIRECTDRAWPALETTE lpddpal = 0;

DDSURFACEDESC2 ddsd;

DDSURFACEDESC2        ddsdback;
LPDIRECTDRAWSURFACE7  lpDDSPrimary=0;
LPDIRECTDRAWSURFACE7  lpDDSDBack=0;
LPDIRECTDRAWSURFACE7  lpDDSBack=0;

static void ShowDDErr(char *s)
{
 char tempo[512];
 sprintf(tempo,"DirectDraw: %s",s);
 FCEUD_PrintError(tempo);
}

int RestoreDD(int w)
{
 if(w)
  {
   if(!lpDDSBack) return 0;
   if(IDirectDrawSurface7_Restore(lpDDSBack)!=DD_OK) return 0;
  }
 else
  {
   if(!lpDDSPrimary) return 0;
   if(IDirectDrawSurface7_Restore(lpDDSPrimary)!=DD_OK) return 0;
  }
 veflags|=1;
 return 1;
}

void FCEUD_SetPalette(unsigned char index, unsigned char r, unsigned char g, unsigned char b)
{
        color_palette[index].peRed=r;
        color_palette[index].peGreen=g;
        color_palette[index].peBlue=b;
        PaletteChanged=1;
}

void FCEUD_GetPalette(unsigned char i, unsigned char *r, unsigned char *g, unsigned char *b)
{
        *r=color_palette[i].peRed;
        *g=color_palette[i].peGreen;
        *b=color_palette[i].peBlue;
}

static int InitializeDDraw(int fs)
{
#ifdef _USE_SHARED_MEMORY_
	mapColorPalette = CreateFileMapping((HANDLE)0xFFFFFFFF,NULL,PAGE_READWRITE, 0, 256 * sizeof(PALETTEENTRY),"fceu.ColorPalette");
	if(mapColorPalette == NULL || GetLastError() == ERROR_ALREADY_EXISTS)
	{
		CloseHandle(mapColorPalette);
		mapColorPalette = NULL;
		color_palette = (PALETTEENTRY*)malloc(256 * sizeof(PALETTEENTRY)); //mbg merge 7/18/06 added cast
	}
	else
		color_palette   = (PALETTEENTRY *)MapViewOfFile(mapColorPalette, FILE_MAP_WRITE, 0, 0, 0);
#endif
	
	//(disvaccel&(1<<(fs?1:0)))?(GUID FAR *)DDCREATE_EMULATIONONLY:
        ddrval = DirectDrawCreate((disvaccel&(1<<(fs?1:0)))?(GUID FAR *)DDCREATE_EMULATIONONLY:NULL, &lpDD, NULL);
        if (ddrval != DD_OK)
        {
         ShowDDErr("Error creating DirectDraw object.");
         return 0;
        }

		//mbg merge 7/17/06 changed:
		ddrval = IDirectDraw_QueryInterface(lpDD,IID_IDirectDraw7,(LPVOID *)&lpDD7);
        //ddrval = IDirectDraw_QueryInterface(lpDD,&IID_IDirectDraw7,(LPVOID *)&lpDD7);
        IDirectDraw_Release(lpDD);

        if (ddrval != DD_OK)
        {
         ShowDDErr("Error querying interface.");
         return 0;
        }

        caps.dwSize=sizeof(caps);
        if(IDirectDraw7_GetCaps(lpDD7,&caps,0)!=DD_OK)
        {
         ShowDDErr("Error getting capabilities.");
         return 0;
        }
        return 1;
}

static int GetBPP(void)
{
        DDPIXELFORMAT ddpix;

        memset(&ddpix,0,sizeof(ddpix));
        ddpix.dwSize=sizeof(ddpix);

        ddrval=IDirectDrawSurface7_GetPixelFormat(lpDDSPrimary,&ddpix);
        if (ddrval != DD_OK)
        {
         ShowDDErr("Error getting primary surface pixel format.");
         return 0;
        }

        if(ddpix.dwFlags&DDPF_RGB)
        {
			//mbg merge 7/17/06 removed silly dummy union stuff now that we have c++
         bpp=ddpix.dwRGBBitCount;
         CBM[0]=ddpix.dwRBitMask;
         CBM[1]=ddpix.dwGBitMask;
         CBM[2]=ddpix.dwBBitMask;
        }
        else
        {
         ShowDDErr("RGB data not valid.");
         return 0;
        }
        if(bpp==15) bpp=16;

        return 1;
}

static int InitBPPStuff(int fs)
{
   if(bpp >= 16)
   {
    InitBlitToHigh(bpp >> 3, CBM[0], CBM[1], CBM[2], 0, fs?vmodes[vmod].special:winspecial);
   }
   else if(bpp==8)
   {
    ddrval=IDirectDraw7_CreatePalette( lpDD7, DDPCAPS_8BIT|DDPCAPS_ALLOW256|DDPCAPS_INITIALIZE,color_palette,&lpddpal,NULL);
    if (ddrval != DD_OK)
    {
     ShowDDErr("Error creating palette object.");
     return 0;
    }
    ddrval=IDirectDrawSurface7_SetPalette(lpDDSPrimary, lpddpal);
    if (ddrval != DD_OK)
    {
     ShowDDErr("Error setting palette object.");
     return 0;
    }
   }
   return 1;
}

int SetVideoMode(int fs)
{
        int specmul;    // Special scaler size multiplier


        if(fs)
         if(!vmod)
          if(!RecalcCustom())
           return(0);

        vflags=0;
        veflags=1;
        PaletteChanged=1;

        ResetVideo();

        if(!InitializeDDraw(fs)) return(1);     // DirectDraw not initialized


        if(!fs)
        { 
         if(winspecial == 2 || winspecial == 1)
          specmul = 2;
         else if(winspecial == 3 || winspecial == 4)
          specmul = 3;
         else
          specmul = 1;

         ShowCursorAbs(1);
         windowedfailed=1;
         HideFWindow(0);

         ddrval = IDirectDraw7_SetCooperativeLevel ( lpDD7, hAppWnd, DDSCL_NORMAL);
         if (ddrval != DD_OK)
         {
          ShowDDErr("Error setting cooperative level.");
          return 1;
         }

         /* Beginning */
         memset(&ddsd,0,sizeof(ddsd));
         ddsd.dwSize = sizeof(ddsd);
         ddsd.dwFlags = DDSD_CAPS;
         ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

         ddrval = IDirectDraw7_CreateSurface ( lpDD7, &ddsd, &lpDDSPrimary,(IUnknown FAR*)NULL);
         if (ddrval != DD_OK)
         {
          FCEU_PrintError("%08x, %d\n",ddrval,lpDD7);
          ShowDDErr("Error creating primary surface.");
          return 1;
         }

         memset(&ddsdback,0,sizeof(ddsdback));
         ddsdback.dwSize=sizeof(ddsdback);
         ddsdback.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
         ddsdback.ddsCaps.dwCaps= DDSCAPS_OFFSCREENPLAIN;

         ddsdback.dwWidth=256 * specmul;
         ddsdback.dwHeight=240 * specmul;
         
         /* 
           If the blit hardware can't stretch, assume it's cheap(and slow)
           and create the buffer in system memory.
         */
         if(!(caps.dwCaps&DDCAPS_BLTSTRETCH))
          ddsdback.ddsCaps.dwCaps|=DDSCAPS_SYSTEMMEMORY;
       
         ddrval = IDirectDraw7_CreateSurface ( lpDD7, &ddsdback, &lpDDSBack, (IUnknown FAR*)NULL);
         if (ddrval != DD_OK)
         {
          ShowDDErr("Error creating secondary surface.");
          return 0;
         }
        
         if(!GetBPP())
          return 0;

         if(bpp!=16 && bpp!=24 && bpp!=32)
         {
          ShowDDErr("Current bit depth not supported!");
          return 0;
         }

         if(!InitBPPStuff(fs))
          return 0;

         ddrval=IDirectDraw7_CreateClipper(lpDD7,0,&lpClipper,0);
         if (ddrval != DD_OK)
         {
          ShowDDErr("Error creating clipper.");
          return 0;
         }

         ddrval=IDirectDrawClipper_SetHWnd(lpClipper,0,hAppWnd);
         if (ddrval != DD_OK)
         {
          ShowDDErr("Error setting clipper window.");
          return 0;
         }
         ddrval=IDirectDrawSurface7_SetClipper(lpDDSPrimary,lpClipper);
         if (ddrval != DD_OK)
         {
          ShowDDErr("Error attaching clipper to primary surface.");
          return 0;
         }

         windowedfailed=0;
         SetMainWindowStuff();
        }
        else    /* Following is full-screen */
        {
         if(vmod == 0)
         {         
          if(vmodes[0].special == 2 || vmodes[0].special == 1)
           specmul = 2;
          else if(vmodes[0].special == 3 || vmodes[0].special == 4)
           specmul = 3;
          else
           specmul = 1;
         }
         HideFWindow(1);

         ddrval = IDirectDraw7_SetCooperativeLevel ( lpDD7, hAppWnd,DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT);
         if (ddrval != DD_OK)
         {
          ShowDDErr("Error setting cooperative level.");
          return 0;
         }

         ddrval = IDirectDraw7_SetDisplayMode(lpDD7, vmodes[vmod].x, vmodes[vmod].y,vmodes[vmod].bpp,0,0);
         if (ddrval != DD_OK)
         {
          ShowDDErr("Error setting display mode.");
          return 0;
         }
         if(vmodes[vmod].flags&VMDF_DXBLT)
         {
          memset(&ddsdback,0,sizeof(ddsdback));
          ddsdback.dwSize=sizeof(ddsdback);
          ddsdback.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
          ddsdback.ddsCaps.dwCaps= DDSCAPS_OFFSCREENPLAIN;

          ddsdback.dwWidth=256 * specmul; //vmodes[vmod].srect.right;
          ddsdback.dwHeight=240 * specmul; //vmodes[vmod].srect.bottom;

	  if(!(caps.dwCaps&DDCAPS_BLTSTRETCH))
           ddsdback.ddsCaps.dwCaps|=DDSCAPS_SYSTEMMEMORY; 

          ddrval = IDirectDraw7_CreateSurface ( lpDD7, &ddsdback, &lpDDSBack, (IUnknown FAR*)NULL);
          if(ddrval!=DD_OK)
          {
           ShowDDErr("Error creating secondary surface.");
           return 0;
          }
         }

         // create foreground surface
        
         memset(&ddsd,0,sizeof(ddsd));
         ddsd.dwSize = sizeof(ddsd);
 
         ddsd.dwFlags = DDSD_CAPS;
         ddsd.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

         if(fssync==3) // Double buffering.
         {
          ddsd.dwFlags |= DDSD_BACKBUFFERCOUNT;
          ddsd.dwBackBufferCount = 1;
          ddsd.ddsCaps.dwCaps |= DDSCAPS_COMPLEX | DDSCAPS_FLIP;
         }

         ddrval = IDirectDraw7_CreateSurface ( lpDD7, &ddsd, &lpDDSPrimary,(IUnknown FAR*)NULL);
         if (ddrval != DD_OK)
         {
          ShowDDErr("Error creating primary surface.");
          return 0;
         } 

         if(fssync==3)
         {
          DDSCAPS2 tmp;

          memset(&tmp,0,sizeof(tmp));
          tmp.dwCaps=DDSCAPS_BACKBUFFER;

          if(IDirectDrawSurface7_GetAttachedSurface(lpDDSPrimary,&tmp,&lpDDSDBack)!=DD_OK)
          {
           ShowDDErr("Error getting attached surface.");
           return 0;
          }
         }

         if(!GetBPP())
          return 0;
         if(!InitBPPStuff(fs))
          return 0;

         mustrestore=1;
         ShowCursorAbs(0);
        }

        InputScreenChanged(fs);
        fullscreen=fs;
        return 1;
}

static void BlitScreenWindow(uint8 *XBuf);
static void BlitScreenFull(uint8 *XBuf);
//static uint8 *XBSave;
void FCEUD_BlitScreen(uint8 *XBuf)
{
 xbsave = XBuf;
 if(!NoWaiting)
 {
  int ws;

  if(fullscreen) ws=fssync;
  else ws = winsync;

  if(ws==1)
   IDirectDraw7_WaitForVerticalBlank(lpDD7,DDWAITVB_BLOCKBEGIN,0);
  else if(ws == 2)   
  {
   BOOL invb = 0;

   while((DD_OK == IDirectDraw7_GetVerticalBlankStatus(lpDD7,&invb)) && !invb)
    Sleep(0);
  }
 }

 if(fullscreen)
 {
  BlitScreenFull(XBuf);
 }
 else
 {
  if(!windowedfailed)
   BlitScreenWindow(XBuf);
 }
}

static void FixPaletteHi(void)
{
   SetPaletteBlitToHigh((uint8*)color_palette); //mbg merge 7/17/06 added cast
}

static void BlitScreenWindow(unsigned char *XBuf)
{
 int pitch;
 unsigned char *ScreenLoc;
 static RECT srect;
 RECT drect;
 int specialmul;

 if(winspecial == 2 || winspecial == 1)
  specialmul = 2;
 else if(winspecial == 4 || winspecial == 3)
  specialmul = 3;
 else specialmul = 1;


 srect.top=srect.left=0;
 srect.right=VNSWID * specialmul;
 srect.bottom=totallines * specialmul;

 if(PaletteChanged==1)
 {
  FixPaletteHi();
  PaletteChanged=0;
 }

 if(!GetClientAbsRect(&drect)) return;

 ddrval=IDirectDrawSurface7_Lock(lpDDSBack,NULL,&ddsdback, 0, NULL);
 if(ddrval!=DD_OK)
 {
  if(ddrval==DDERR_SURFACELOST) RestoreDD(1);
  return;
 }

 //mbg merge 7/17/06 removing dummyunion stuff
 pitch=ddsdback.lPitch;
 ScreenLoc=(unsigned char*)ddsdback.lpSurface; //mbg merge 7/17/06 added cst
 if(veflags&1)
 {
  memset(ScreenLoc,0,pitch*240);
  veflags&=~1;
 }
 Blit8ToHigh(XBuf+srendline*256+VNSCLIP,ScreenLoc, VNSWID, totallines, pitch,specialmul,specialmul);

 IDirectDrawSurface7_Unlock(lpDDSBack, NULL);

 if(IDirectDrawSurface7_Blt(lpDDSPrimary, &drect,lpDDSBack,&srect,DDBLT_ASYNC,0)!=DD_OK)
 {
  ddrval=IDirectDrawSurface7_Blt(lpDDSPrimary, &drect,lpDDSBack,&srect,DDBLT_WAIT,0);
  if(ddrval!=DD_OK)
  {
   if(ddrval==DDERR_SURFACELOST) {RestoreDD(1);RestoreDD(0);}
   return;
  }
 }
}

static void BlitScreenFull(uint8 *XBuf)
{
  static int pitch;
  char *ScreenLoc;
  //unsigned long x; //mbg merge 7/17/06 removed
  //uint8 y; //mbg merge 7/17/06 removed
  RECT srect,drect;
  LPDIRECTDRAWSURFACE7 lpDDSVPrimary;
  int specmul;    // Special scaler size multiplier
  if(vmodes[0].special == 2 || vmodes[0].special == 1)
   specmul = 2;
  else if(vmodes[0].special == 3 || vmodes[0].special == 4)
   specmul = 3;
  else
   specmul = 1;


  if(fssync==3)
   lpDDSVPrimary=lpDDSDBack;
  else
   lpDDSVPrimary=lpDDSPrimary;

  if(PaletteChanged==1)
  {
   if(bpp>=16)
    FixPaletteHi();
   else
   {
    ddrval=IDirectDrawPalette_SetEntries(lpddpal,0,0,256,color_palette);
    if(ddrval!=DD_OK)
    {
     if(ddrval==DDERR_SURFACELOST) RestoreDD(0);
     return;
    }   
   }
   PaletteChanged=0;
  }

 if(vmodes[vmod].flags&VMDF_DXBLT)
 {
  ddrval=IDirectDrawSurface7_Lock(lpDDSBack,NULL,&ddsdback, 0, NULL);
  if(ddrval!=DD_OK)
  {
   if(ddrval==DDERR_SURFACELOST) RestoreDD(1);
   return;
  }
  ScreenLoc=(char *)ddsdback.lpSurface; //mbg merge 7/17/06 added cast
  pitch=ddsdback.lPitch; //mbg merge 7/17/06 removed dummyunion stuff

  srect.top=0;
  srect.left=0;
  srect.right=VNSWID * specmul;
  srect.bottom=totallines * specmul;

  if(vmodes[vmod].flags&VMDF_STRFS)
  {
   drect.top=0;
   drect.left=0;
   drect.right=vmodes[vmod].x;
   drect.bottom=vmodes[vmod].y;
  }
  else
  {
   drect.top=(vmodes[vmod].y-(totallines*vmodes[vmod].yscale))>>1;
   drect.bottom=drect.top+(totallines*vmodes[vmod].yscale);
   drect.left=(vmodes[vmod].x-VNSWID*vmodes[vmod].xscale)>>1;
   drect.right=drect.left+VNSWID*vmodes[vmod].xscale;
  }
 }
 else
 {
  ddrval=IDirectDrawSurface7_Lock(lpDDSVPrimary,NULL,&ddsd, 0, NULL);
  if(ddrval!=DD_OK)
  {
   if(ddrval==DDERR_SURFACELOST) RestoreDD(0);
   return;
  }

  ScreenLoc=(char*)ddsd.lpSurface; //mbg merge 7/17/06 added cast
  pitch=ddsd.lPitch; //mbg merge 7/17/06 removing dummyunion stuff
 }

 if(veflags&1)
 {
  if(vmodes[vmod].flags&VMDF_DXBLT)
  {
   veflags|=2;
   memset((char *)ScreenLoc,0,pitch*srect.bottom);
  }
  else
  {
   memset((char *)ScreenLoc,0,pitch*vmodes[vmod].y);
  }
  PaletteChanged=1;
  veflags&=~1;
 }

 //mbg 6/29/06 merge
#ifndef MSVC
 if(vmod==5)
 {
  if(eoptions&EO_CLIPSIDES)
  {
   asm volatile(
      "xorl %%edx, %%edx\n\t"
      "akoop1:\n\t"
      "movb $120,%%al     \n\t"
      "akoop2:\n\t"
      "movb 1(%%esi),%%dl\n\t"
      "shl  $16,%%edx\n\t"
      "movb (%%esi),%%dl\n\t"
      "movl %%edx,(%%edi)\n\t"
      "addl $2,%%esi\n\t"
      "addl $4,%%edi\n\t"
      "decb %%al\n\t"
      "jne akoop2\n\t"
      "addl $16,%%esi\n\t"
      "addl %%ecx,%%edi\n\t"
      "decb %%bl\n\t"
      "jne akoop1\n\t"
      :
      : "S" (XBuf+srendline*256+VNSCLIP), "D" (ScreenLoc+((240-totallines)/2)*pitch+(640-(VNSWID<<1))/2),"b" (totallines), "c" ((pitch-VNSWID)<<1)
      : "%al", "%edx", "%cc" );
  }
  else
  {
   asm volatile(
     "xorl %%edx, %%edx\n\t"
     "koop1:\n\t"
     "movb $128,%%al     \n\t"
     "koop2:\n\t"
     "movb 1(%%esi),%%dl\n\t"
     "shl  $16,%%edx\n\t"
     "movb (%%esi),%%dl\n\t"
     "movl %%edx,(%%edi)\n\t"
     "addl $2,%%esi\n\t"
     "addl $4,%%edi\n\t"
     "decb %%al\n\t"
     "jne koop2\n\t"
     "addl %%ecx,%%edi\n\t"
     "decb %%bl\n\t"
     "jne koop1\n\t"
      :
      : "S" (XBuf+srendline*256), "D" (ScreenLoc+((240-totallines)/2)*pitch+(640-512)/2),"b" (totallines), "c" (pitch-512+pitch)
      : "%al", "%edx", "%cc" );
  }
 }
 else if(vmod==4)
 {
  if(eoptions&EO_CLIPSIDES)
  {
   asm volatile(
      "ayoop1:\n\t"
      "movb $120,%%al     \n\t"
      "ayoop2:\n\t"
      "movb 1(%%esi),%%dh\n\t"
      "movb %%dh,%%dl\n\t"
      "shl  $16,%%edx\n\t"
      "movb (%%esi),%%dl\n\t"
      "movb %%dl,%%dh\n\t"               // Ugh
      "movl %%edx,(%%edi)\n\t"
      "addl $2,%%esi\n\t"
      "addl $4,%%edi\n\t"
      "decb %%al\n\t"
      "jne ayoop2\n\t"
      "addl $16,%%esi\n\t"
      "addl %%ecx,%%edi\n\t"
      "decb %%bl\n\t"
      "jne ayoop1\n\t"
      :
      : "S" (XBuf+srendline*256+VNSCLIP), "D" (ScreenLoc+((240-totallines)/2)*pitch+(640-(VNSWID<<1))/2),"b" (totallines), "c" ((pitch-VNSWID)<<1)
      : "%al", "%edx", "%cc" );
  }
  else
  {
   asm volatile(
      "yoop1:\n\t"
      "movb $128,%%al     \n\t"
      "yoop2:\n\t"
      "movb 1(%%esi),%%dh\n\t"
      "movb %%dh,%%dl\n\t"
      "shl  $16,%%edx\n\t"
      "movb (%%esi),%%dl\n\t"
      "movb %%dl,%%dh\n\t"               // Ugh
      "movl %%edx,(%%edi)\n\t"
      "addl $2,%%esi\n\t"
      "addl $4,%%edi\n\t"
      "decb %%al\n\t"
      "jne yoop2\n\t"
      "addl %%ecx,%%edi\n\t"
      "decb %%bl\n\t"
      "jne yoop1\n\t"
      :
      : "S" (XBuf+srendline*256), "D" (ScreenLoc+((240-totallines)/2)*pitch+(640-512)/2),"b" (totallines), "c" (pitch-512+pitch)
      : "%al", "%edx", "%cc" );
  }
 }
 else
 #endif 
 //mbg 6/29/06 merge
 {
  if(!(vmodes[vmod].flags&VMDF_DXBLT))
  {  
   if(vmodes[vmod].special)
    ScreenLoc += (vmodes[vmod].drect.left*(bpp>>3)) + ((vmodes[vmod].drect.top)*pitch);   
   else
    ScreenLoc+=((vmodes[vmod].x-VNSWID)>>1)*(bpp>>3)+(((vmodes[vmod].y-totallines)>>1))*pitch;
  }

  if(bpp>=16)
  {
   Blit8ToHigh(XBuf+srendline*256+VNSCLIP,(uint8*)ScreenLoc, VNSWID, totallines, pitch,specmul,specmul); //mbg merge 7/17/06 added cast
  }
  else
  {
   XBuf+=srendline*256+VNSCLIP;
   if(vmodes[vmod].special)
    Blit8To8(XBuf,(uint8*)ScreenLoc, VNSWID, totallines, pitch,vmodes[vmod].xscale,vmodes[vmod].yscale,0,vmodes[vmod].special); //mbg merge 7/17/06 added cast
   else
    Blit8To8(XBuf,(uint8*)ScreenLoc, VNSWID, totallines, pitch,1,1,0,0); //mbg merge 7/17/06 added cast
  }
 }

 if(vmodes[vmod].flags&VMDF_DXBLT)
 { 
  IDirectDrawSurface7_Unlock(lpDDSBack, NULL);

  if(veflags&2)
  {
   if(IDirectDrawSurface7_Lock(lpDDSVPrimary,NULL,&ddsd, 0, NULL)==DD_OK)
   {
    memset(ddsd.lpSurface,0,ddsd.lPitch*vmodes[vmod].y); //mbg merge 7/17/06 removing dummyunion stuff
    IDirectDrawSurface7_Unlock(lpDDSVPrimary, NULL);
    veflags&=~2;
   }
  }

 
  if(IDirectDrawSurface7_Blt(lpDDSVPrimary, &drect,lpDDSBack,&srect,DDBLT_ASYNC,0)!=DD_OK)
  {
   ddrval=IDirectDrawSurface7_Blt(lpDDSVPrimary, &drect,lpDDSBack,&srect,DDBLT_WAIT,0);
   if(ddrval!=DD_OK)
   {
    if(ddrval==DDERR_SURFACELOST)
    {
     RestoreDD(0);
     RestoreDD(1);
    }
    return;
   }

  }
 }
 else
  IDirectDrawSurface7_Unlock(lpDDSVPrimary, NULL);
 if(fssync==3)
 {
  IDirectDrawSurface7_Flip(lpDDSPrimary,0,0);

 }
}

void ResetVideo(void)
{
 ShowCursorAbs(1);
 KillBlitToHigh();
 if(lpDD7)
  if(mustrestore)
   {IDirectDraw7_RestoreDisplayMode(lpDD7);mustrestore=0;}
 if(lpddpal) {IDirectDrawPalette_Release(lpddpal); lpddpal = 0;}
 if(lpDDSBack) {IDirectDrawSurface7_Release(lpDDSBack);lpDDSBack=0;}
 if(lpDDSPrimary) {IDirectDrawSurface7_Release(lpDDSPrimary);lpDDSPrimary=0;} 
 if(lpClipper) {IDirectDrawClipper_Release(lpClipper);lpClipper=0;}
 if(lpDD7)
 { IDirectDraw_Release(lpDD7); lpDD7 = NULL; }

}

int specialmlut[5] = {1,2,2,3,3};

static int RecalcCustom(void)
{
 vmdef *cmode = &vmodes[0];

 cmode->flags&=~VMDF_DXBLT;

 if(cmode->flags&VMDF_STRFS)
 {
  cmode->flags|=VMDF_DXBLT;
 }
 else if(cmode->xscale!=1 || cmode->yscale!=1 || cmode->special) 
 {
  if(cmode->special)
  {
   int mult = specialmlut[cmode->special];

   if(cmode->xscale < mult)
    cmode->xscale = mult;
   if(cmode->yscale < mult)
    cmode->yscale = mult;

   if(cmode->xscale != mult || cmode->yscale != mult)
    cmode->flags|=VMDF_DXBLT;
  }
  else
   cmode->flags|=VMDF_DXBLT;


  if(VNSWID*cmode->xscale>cmode->x)
  {
   if(cmode->special)
   {
    FCEUD_PrintError("Scaled width is out of range.");
    return(0);
   }
   else
   {
    FCEUD_PrintError("Scaled width is out of range.  Reverting to no horizontal scaling.");
    cmode->xscale=1;
   }
  }
  if(totallines*cmode->yscale>cmode->y)
  {
   if(cmode->special)
   {
    FCEUD_PrintError("Scaled height is out of range.");
    return(0);
   }
   else
   {
    FCEUD_PrintError("Scaled height is out of range.  Reverting to no vertical scaling.");
    cmode->yscale=1;
   }
  }

  cmode->srect.left=VNSCLIP;
  cmode->srect.top=srendline;
  cmode->srect.right=256-VNSCLIP;
  cmode->srect.bottom=erendline+1;

  cmode->drect.top=(cmode->y-(totallines*cmode->yscale))>>1;
  cmode->drect.bottom=cmode->drect.top+totallines*cmode->yscale;
  
  cmode->drect.left=(cmode->x-(VNSWID*cmode->xscale))>>1;
  cmode->drect.right=cmode->drect.left+VNSWID*cmode->xscale;
 }

 if((cmode->special == 1 || cmode->special == 3) && cmode->bpp == 8)
 {
  cmode->bpp = 32;
  //FCEUD_PrintError("HQ2x/HQ3x requires 16bpp or 32bpp(best).");
  //return(0);
 }

 if(cmode->x<VNSWID)
 {
  FCEUD_PrintError("Horizontal resolution is too low.");
  return(0);
 }
 if(cmode->y<totallines && !(cmode->flags&VMDF_STRFS))
 {
  FCEUD_PrintError("Vertical resolution must not be less than the total number of drawn scanlines.");
  return(0);
 }

 return(1);
}

BOOL SetDlgItemDouble(HWND hDlg, int item, double value)
{
 char buf[9]; //mbg merge 7/19/06 changed to 9 to leave room for \0
 sprintf(buf,"%.6f",value);
 return SetDlgItemText(hDlg, item, buf); //mbg merge 7/17/06 added this return value
}

double GetDlgItemDouble(HWND hDlg, int item)
{
 char buf[8];
 double ret = 0;

 GetDlgItemText(hDlg, item, buf, 8);
 sscanf(buf,"%lf",&ret);
 return(ret);
}

BOOL CALLBACK VideoConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
        static char *vmstr[11]={
                                "Custom",
                                "320x240 Full Screen",
                                "512x384 Centered",
                                "640x480 Centered",
                                "640x480 Scanlines",
                                "640x480 \"4 per 1\"",
                                "640x480 2x,2y",
                                "1024x768 4x,3y",
                                "1280x1024 5x,4y",
                                "1600x1200 6x,5y",
                                "800x600 Stretched"
                               };
        int x;

        switch(uMsg)
        {
         case WM_INITDIALOG:      
			 SetDlgItemText(hwndDlg,65454,"For the custom video mode settings to be in effect, the \"Custom\" video mode needs to be selected.  If you select a sync method, and sound is disabled, you may want to disable speed throttling(in the \"Timing\" window).  If you use \"wait for vblank\", you should use the \"lazy\" form.\x0AAllowing more than 8 sprites per scanline can cause graphics corruption in some games, including \"Solstice\".");
                for(x=0;x<11;x++)
                 SendDlgItemMessage(hwndDlg,100,CB_ADDSTRING,0,(LPARAM)(LPSTR)vmstr[x]);
                SendDlgItemMessage(hwndDlg,100,CB_SETCURSEL,vmod,(LPARAM)(LPSTR)0);

                SendDlgItemMessage(hwndDlg,202,CB_ADDSTRING,0,(LPARAM)(LPSTR)"8");
                SendDlgItemMessage(hwndDlg,202,CB_ADDSTRING,0,(LPARAM)(LPSTR)"16");
                SendDlgItemMessage(hwndDlg,202,CB_ADDSTRING,0,(LPARAM)(LPSTR)"24");
                SendDlgItemMessage(hwndDlg,202,CB_ADDSTRING,0,(LPARAM)(LPSTR)"32");
                SendDlgItemMessage(hwndDlg,202,CB_SETCURSEL,(vmodes[0].bpp>>3)-1,(LPARAM)(LPSTR)0);

                SetDlgItemInt(hwndDlg,200,vmodes[0].x,0);
                SetDlgItemInt(hwndDlg,201,vmodes[0].y,0);

                SetDlgItemInt(hwndDlg,302,vmodes[0].xscale,0);
                SetDlgItemInt(hwndDlg,303,vmodes[0].yscale,0);
                CheckRadioButton(hwndDlg,300,301,(vmodes[0].flags&VMDF_STRFS)?301:300);

                {
                 char *str[]={"<none>","hq2x","Scale2x","hq3x","Scale3x"};
                 int x;
                 for(x=0;x<5;x++)
                 {
                  SendDlgItemMessage(hwndDlg,304,CB_ADDSTRING,0,(LPARAM)(LPSTR)str[x]);
                  SendDlgItemMessage(hwndDlg,406,CB_ADDSTRING,0,(LPARAM)(LPSTR)str[x]);
                 }
                }
                SendDlgItemMessage(hwndDlg,304,CB_SETCURSEL,vmodes[0].special,(LPARAM)(LPSTR)0);
                SendDlgItemMessage(hwndDlg,406,CB_SETCURSEL,winspecial,(LPARAM)(LPSTR)0);

                if(eoptions&EO_FSAFTERLOAD)
                 CheckDlgButton(hwndDlg,102,BST_CHECKED);

                if(eoptions&EO_CLIPSIDES)
                 CheckDlgButton(hwndDlg,106,BST_CHECKED);

                if(disvaccel&1)
                 CheckDlgButton(hwndDlg,130,BST_CHECKED);

                if(disvaccel&2)
                 CheckDlgButton(hwndDlg,131,BST_CHECKED);

                if(eoptions&EO_FORCEISCALE)
                 CheckDlgButton(hwndDlg,402,BST_CHECKED);

                if(eoptions&EO_FORCEASPECT)
                 CheckDlgButton(hwndDlg,403,BST_CHECKED);

                SetDlgItemInt(hwndDlg,500,srendlinen,0);
                SetDlgItemInt(hwndDlg,501,erendlinen,0);

                SetDlgItemInt(hwndDlg,502,srendlinep,0);
                SetDlgItemInt(hwndDlg,503,erendlinep,0);


                SetDlgItemDouble(hwndDlg, 400, winsizemulx);
                SetDlgItemDouble(hwndDlg, 401, winsizemuly);
                SetDlgItemDouble(hwndDlg, 404, saspectw);
                SetDlgItemDouble(hwndDlg, 405, saspecth);

                //SetDlgI               temInt(hwndDlg,103,winsizemul,0);

                SendDlgItemMessage(hwndDlg,104,CB_ADDSTRING,0,(LPARAM)(LPSTR)"<none>");
                SendDlgItemMessage(hwndDlg,105,CB_ADDSTRING,0,(LPARAM)(LPSTR)"<none>");

                SendDlgItemMessage(hwndDlg,104,CB_ADDSTRING,0,(LPARAM)(LPSTR)"Wait for VBlank");
                SendDlgItemMessage(hwndDlg,104,CB_ADDSTRING,0,(LPARAM)(LPSTR)"Lazy wait for VBlank");

                SendDlgItemMessage(hwndDlg,105,CB_ADDSTRING,0,(LPARAM)(LPSTR)"Wait for VBlank");
                SendDlgItemMessage(hwndDlg,105,CB_ADDSTRING,0,(LPARAM)(LPSTR)"Lazy wait for VBlank");
                SendDlgItemMessage(hwndDlg,105,CB_ADDSTRING,0,(LPARAM)(LPSTR)"Double Buffering");

                SendDlgItemMessage(hwndDlg,104,CB_SETCURSEL,winsync,(LPARAM)(LPSTR)0);
                SendDlgItemMessage(hwndDlg,105,CB_SETCURSEL,fssync,(LPARAM)(LPSTR)0);

                if(eoptions&EO_NOSPRLIM)
                 CheckDlgButton(hwndDlg,600,BST_CHECKED);

                break;
         case WM_CLOSE:
         case WM_QUIT: goto gornk;
         case WM_COMMAND:
                        if(!(wParam>>16))
                        switch(wParam&0xFFFF)
                        {
                         case 1:
                         gornk:
         
                         if(IsDlgButtonChecked(hwndDlg,106)==BST_CHECKED)
                          eoptions|=EO_CLIPSIDES;
                         else
                          eoptions&=~EO_CLIPSIDES;

                         if(IsDlgButtonChecked(hwndDlg,600)==BST_CHECKED)
                          eoptions|=EO_NOSPRLIM;
                         else
                          eoptions&=~EO_NOSPRLIM;

                         srendlinen=GetDlgItemInt(hwndDlg,500,0,0);
                         erendlinen=GetDlgItemInt(hwndDlg,501,0,0);
                         srendlinep=GetDlgItemInt(hwndDlg,502,0,0);
                         erendlinep=GetDlgItemInt(hwndDlg,503,0,0);


                         if(erendlinen>239) erendlinen=239;
                         if(srendlinen>erendlinen) srendlinen=erendlinen;

                         if(erendlinep>239) erendlinep=239;
                         if(srendlinep>erendlinen) srendlinep=erendlinep;

                         UpdateRendBounds();

                         if(IsDlgButtonChecked(hwndDlg,301)==BST_CHECKED)
                          vmodes[0].flags|=VMDF_STRFS;
                         else
                          vmodes[0].flags&=~VMDF_STRFS;

                         vmod=SendDlgItemMessage(hwndDlg,100,CB_GETCURSEL,0,(LPARAM)(LPSTR)0);
                         vmodes[0].x=GetDlgItemInt(hwndDlg,200,0,0);
                         vmodes[0].y=GetDlgItemInt(hwndDlg,201,0,0);
                         vmodes[0].bpp=(SendDlgItemMessage(hwndDlg,202,CB_GETCURSEL,0,(LPARAM)(LPSTR)0)+1)<<3;

                         vmodes[0].xscale=GetDlgItemInt(hwndDlg,302,0,0);
                         vmodes[0].yscale=GetDlgItemInt(hwndDlg,303,0,0);
                         vmodes[0].special=SendDlgItemMessage(hwndDlg,304,CB_GETCURSEL,0,(LPARAM)(LPSTR)0);

                         winspecial = SendDlgItemMessage(hwndDlg,406,CB_GETCURSEL,0,(LPARAM)(LPSTR)0);
                         disvaccel = 0;

                         if(IsDlgButtonChecked(hwndDlg,130)==BST_CHECKED)
                          disvaccel |= 1;
                         if(IsDlgButtonChecked(hwndDlg,131)==BST_CHECKED)
                          disvaccel |= 2;


                         if(IsDlgButtonChecked(hwndDlg,101)==BST_CHECKED)
                          fullscreen=1;
                         else
                          fullscreen=0;
                         if(IsDlgButtonChecked(hwndDlg,102)==BST_CHECKED)
                          eoptions|=EO_FSAFTERLOAD;
                         else
                          eoptions&=~EO_FSAFTERLOAD;

                         eoptions &= ~(EO_FORCEISCALE | EO_FORCEASPECT);
                         if(IsDlgButtonChecked(hwndDlg,402)==BST_CHECKED)
                          eoptions|=EO_FORCEISCALE;
                         if(IsDlgButtonChecked(hwndDlg,403)==BST_CHECKED)
                          eoptions|=EO_FORCEASPECT;


                         winsizemulx=GetDlgItemDouble(hwndDlg, 400);
                         winsizemuly=GetDlgItemDouble(hwndDlg, 401);
                         saspectw=GetDlgItemDouble(hwndDlg, 404);
                         saspecth=GetDlgItemDouble(hwndDlg, 405);
                         FixWXY(0);

                         winsync=SendDlgItemMessage(hwndDlg,104,CB_GETCURSEL,0,(LPARAM)(LPSTR)0);
                         fssync=SendDlgItemMessage(hwndDlg,105,CB_GETCURSEL,0,(LPARAM)(LPSTR)0);
                         EndDialog(hwndDlg,0);
                         break;
               }
              }
  return 0;
}

void SetFSVideoMode()
{
 changerecursive=1;
 if(!SetVideoMode(1))
  SetVideoMode(0);
 changerecursive=0;
}


void DoVideoConfigFix(void)
{
        FCEUI_DisableSpriteLimitation(eoptions&EO_NOSPRLIM);
        UpdateRendBounds();
}

/**
* Shows the Video configuration dialog.
**/
void ConfigVideo(void)
{
	DialogBox(fceu_hInstance, "VIDEOCONFIG", hAppWnd, VideoConCallB); 
	DoVideoConfigFix();

	if(fullscreen)
	{
		SetFSVideoMode();
	}
	else
	{
		changerecursive = 1;
		SetVideoMode(0);
		changerecursive = 0;
	}
	//SetMainWindowStuff();
}

