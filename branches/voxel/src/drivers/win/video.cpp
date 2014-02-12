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

#include "video.h"
#include "ddraw.h"
#include "main.h"
#include "window.h"
#include "../../drawing.h"
#include "gui.h"
#include "../../fceu.h"
#include "../../video.h"
#include "input.h"
#include "mapinput.h"
#include <math.h>
#include "utils/bitflags.h"


static bool dispModeWasChanged = false; // tells if driver should reset display mode on shutdown; set on fullscreen
static DWORD colorsBitMask[3]; // masks to extract components from color data with different bpp

static int bpp; // bits per pixel
static bool wipeSurface = true; // tells mode/resolution was changed and we should wipe surface

// Surface modes selected for windowed and fullscreen
static DIRECTDRAW_MODE directDrawModeWindowed = DIRECTDRAW_MODE_SOFTWARE;
static DIRECTDRAW_MODE directDrawModeFullscreen = DIRECTDRAW_MODE_FULL;

// vertical sync modes for windowed and fullscreen
static VSYNCMODE idxFullscreenSyncMode = SYNCMODE_NONE;
static VSYNCMODE idxWindowedSyncMode = SYNCMODE_NONE;
	// apparently sync settings are ignored during netplay

static VFILTER idxFilterModeWindowed = FILTER_NONE; // filter selected for windowed mode
	// fullscreen mode filter index is taken from active VideoMode struct
static int ntscFilterOption = 0; // special NTSC filter effect
	// never changed in code; might be loaded from config I guess
static int vmodeIdx; // current video mode index
	// never changed in code, can be loaded from config

static VideoMode videoModes[11] =
{
	{0,0,0,VIDEOMODEFLAG_DXBLT|VIDEOMODEFLAG_STRFS,1,1,0},	// Custom - set to current resolution at the first launch
	{320,240,8,0,1,1,0}, //1
	{512,384,8,0,1,1,0}, //2
	{640,480,32,0,1,1,0}, //3
	{640,480,32,0,1,1,0}, //4
	{640,480,32,0,1,1,0}, //5
	{640,480,32,VIDEOMODEFLAG_DXBLT,2,2,0}, //6
	{1024,768,32,VIDEOMODEFLAG_DXBLT,4,3,0}, //7
	{1280,1024,32,VIDEOMODEFLAG_DXBLT,5,4,0}, //8
	{1600,1200,32,VIDEOMODEFLAG_DXBLT,6,5,0}, //9
	{800,600,32,VIDEOMODEFLAG_DXBLT|VIDEOMODEFLAG_STRFS,0,0}    //10
};

extern uint8 PALRAM[0x20]; // NES palette ram

static PALETTEENTRY color_palette[256]; // driver palette

static bool updateDDPalette = true; // flag to update DirectDraw palette

static LPDIRECTDRAW7 ddraw7Handle = 0; // our working DirectDraw
static LPDIRECTDRAWCLIPPER ddClipperHandle = 0; // DD clipper to keep image within window boundaries
static LPDIRECTDRAWPALETTE ddPaletteHandle = 0; // DD palette

// surface descriptions
static DDSURFACEDESC2 surfaceDescScreen;
static DDSURFACEDESC2 surfaceDescOffscreen;

// surfaces
static LPDIRECTDRAWSURFACE7 lpScreenSurface=0; // screen surface
static LPDIRECTDRAWSURFACE7 lpBackBuffer=0; // back buffer of screen surface
	// in doublebuffer mode image ends up here, then surface is flipped
static LPDIRECTDRAWSURFACE7 lpOffscreenSurface=0; // offscreen surface

static RECT activeRect = {0}; // active area of the screen where image is displayed and mouse input is processed

static bool fullscreenDesired = false;	// 'Desired' fullscreen status
static bool fullscreenActual = false; // Internal 'actual' fullscreen status

static int filterScaleMultiplier[6] = {1,2,2,2,3,3}; // scale multipliers for defined filters


static void RestoreLostScreenSurface()
{
	if(lpScreenSurface) {
		IDirectDrawSurface7_Restore(lpScreenSurface);
		wipeSurface = true;
	}
}

static void RestoreLostOffscreenSurface()
{
	if(lpOffscreenSurface) {
		IDirectDrawSurface7_Restore(lpOffscreenSurface);
		wipeSurface = true;
	}
}

static bool CreateDDraw()
{
	ShutdownVideoDriver();

	HRESULT status;
	{
		GUID FAR *guid;
		if ((GetIsFullscreen() && directDrawModeFullscreen == DIRECTDRAW_MODE_SOFTWARE) || (!GetIsFullscreen() && directDrawModeWindowed == DIRECTDRAW_MODE_SOFTWARE))
			guid = (GUID FAR *)DDCREATE_EMULATIONONLY;
	else
			guid = NULL;

		LPDIRECTDRAW ddrawHandle;
		status = DirectDrawCreate(guid, &ddrawHandle, NULL);
		if (status != DD_OK)
	{
		FCEU_printf("Error creating DirectDraw object.\n");
			return false;
	}

		status = IDirectDraw_QueryInterface(ddrawHandle,IID_IDirectDraw7,(LPVOID *)&ddraw7Handle);
		IDirectDraw_Release(ddrawHandle);
		if (status != DD_OK)
	{
		FCEU_printf("Error querying interface.\n");
			return false;
		}
	}

	DDCAPS caps;
	memset(&caps,0,sizeof(caps));
	caps.dwSize=sizeof(caps);
	if(IDirectDraw7_GetCaps(ddraw7Handle,&caps,0)!=DD_OK)
	{
		FCEU_printf("Error getting capabilities.\n");
		return false;
	}
	
	return true;
}

static bool InitBPPStuff(bool fullscreen)
{
	DDPIXELFORMAT ddpix;
	memset(&ddpix,0,sizeof(ddpix));
	ddpix.dwSize=sizeof(ddpix);

	HRESULT status = IDirectDrawSurface7_GetPixelFormat(lpScreenSurface,&ddpix);
	if (status == DD_OK) {
		if (FL_TEST(ddpix.dwFlags, DDPF_RGB)) {
		bpp=ddpix.dwRGBBitCount;
			colorsBitMask[0]=ddpix.dwRBitMask;
			colorsBitMask[1]=ddpix.dwGBitMask;
			colorsBitMask[2]=ddpix.dwBBitMask;
			
			if (bpp==15) bpp=16;

			if (!fullscreen || (bpp==16 || bpp==24 || bpp==32)) { // in fullscreen check for supported bitcount
				if (bpp >= 16)
				{
					int filterIdx = (fullscreen)? videoModes[vmodeIdx].filter:idxFilterModeWindowed;
					int ntscFiltOpt = 0;
					if (filterIdx == FILTER_NTSC2X) {
						ntscFiltOpt = ntscFilterOption;
					}

					InitBlitToHigh(bpp/8,
						colorsBitMask[0],
						colorsBitMask[1],
						colorsBitMask[2],
						0,
						filterIdx,
						ntscFiltOpt);

					return true;
				}
				else if (bpp==8)
				{
					HRESULT status = IDirectDraw7_CreatePalette( ddraw7Handle,
						DDPCAPS_8BIT|DDPCAPS_ALLOW256|DDPCAPS_INITIALIZE,
						color_palette,
						&ddPaletteHandle,
						NULL);
					if (status == DD_OK) {
						status = IDirectDrawSurface7_SetPalette(lpScreenSurface, ddPaletteHandle);
						if (status == DD_OK) {
							return true;
	}
						else {
							FCEU_printf("Error setting palette object.\n");
	}
	}
					else {
			FCEU_printf("Error creating palette object.\n");
		}
		}
			} // if(supported bpp)
		}
		else {
			FCEU_printf("RGB data not valid.\n");
		}
	}
	else {
		FCEU_printf("Error getting primary surface pixel format.\n");
	}

	return false;
}

void OnWindowSizeChange(int wndWidth, int wndHeight)
{
	if (!ddraw7Handle)
		return;	// DirectDraw isn't initialized yet

	double srcHeight = FSettings.TotalScanlines();
	double srcWidth = VNSWID;
	if (FL_TEST(eoptions, EO_TVASPECT))
		srcWidth = ceil(srcHeight * (srcWidth / 256) * (tvAspectX / tvAspectY));

	double current_aspectratio = (double)wndWidth / (double)wndHeight;
	double needed_aspectratio = srcWidth / srcHeight;

	double new_width;
	double new_height;
	if (current_aspectratio >= needed_aspectratio) {
		// the window is wider or match emulated screen
		new_height = wndHeight;
		if (FL_TEST(eoptions, EO_SQUAREPIXELS)) {
			new_height = srcHeight * int((double)wndHeight / srcHeight); // integer scale of src
			if (new_height == 0)
				new_height = wndHeight; // window is smaller than src (or srcHeight is zero, which I hope never the case)
		}

		new_width = (new_height * needed_aspectratio);
		if (FL_TEST(eoptions, EO_SQUAREPIXELS) && !FL_TEST(eoptions, EO_TVASPECT)) {
			int new_width_integer = int((double)new_width / srcWidth) * srcWidth;
			if (new_width_integer > 0)
				new_width = new_width_integer;
		}
	}
	else {
		// the window is taller than emulated screen
		new_width = wndWidth;
		if (FL_TEST(eoptions, EO_SQUAREPIXELS))
		{
			new_width = int((double)wndWidth / srcWidth) * srcWidth;
			if (new_width == 0)
				new_width = wndWidth;
		}

		new_height = (new_width / needed_aspectratio);
		if (FL_TEST(eoptions, EO_SQUAREPIXELS) && !FL_TEST(eoptions, EO_TVASPECT))
		{
			int new_height_integer = int((double)new_height / srcHeight) * srcHeight;
			if (new_height_integer > 0)
				new_height = new_height_integer;
		}
	}

	activeRect.left = (int)((wndWidth - new_width) / 2);
	activeRect.right = activeRect.left + new_width;
	activeRect.top = (int)((wndHeight - new_height) / 2);
	activeRect.bottom = activeRect.top + new_height;
}

static void ResetVideoModeParams()
{
	// use current display settings
	if (!ddraw7Handle)
		return;

	DDSURFACEDESC2 surfaceDescription;
	memset(&surfaceDescription,0,sizeof(surfaceDescription));
	surfaceDescription.dwSize = sizeof(DDSURFACEDESC2);
	HRESULT status = IDirectDraw7_GetDisplayMode(ddraw7Handle, &surfaceDescription);
	if (SUCCEEDED(status)) {
		videoModes[vmodeIdx].width = surfaceDescription.dwWidth;
		videoModes[vmodeIdx].height = surfaceDescription.dwHeight;
		videoModes[vmodeIdx].bpp = surfaceDescription.ddpfPixelFormat.dwRGBBitCount;
		videoModes[vmodeIdx].xscale = videoModes[vmodeIdx].yscale = 1;
		videoModes[vmodeIdx].flags = VIDEOMODEFLAG_DXBLT|VIDEOMODEFLAG_STRFS;
	}
}

static bool RecalcVideoModeParams()
{
	VideoMode& vmode = videoModes[0];
	if ((vmode.width <= 0) || (vmode.height <= 0))
		ResetVideoModeParams();

	if(FL_TEST(vmode.flags, VIDEOMODEFLAG_STRFS)) {
		FL_SET(vmode.flags, VIDEOMODEFLAG_DXBLT);
	}
	else if(vmode.xscale!=1 || vmode.yscale!=1 || vmode.filter!=FILTER_NONE)  {
		// if scaled or have filter (which may have scale)
		FL_SET(vmode.flags, VIDEOMODEFLAG_DXBLT);

		if(vmode.filter != FILTER_NONE) {
			// adjust vmode scale to be no less than filter scale
			int mult = filterScaleMultiplier[vmode.filter];

			if(vmode.xscale < mult)
				vmode.xscale = mult;
			if(vmode.yscale < mult)
				vmode.yscale = mult;

			if(vmode.xscale == mult && vmode.yscale == mult)
				FL_CLEAR(vmode.flags, VIDEOMODEFLAG_DXBLT); // exact match, no need for "region blit"?
		}

		if(VNSWID*vmode.xscale > vmode.width) {
			// videomode scale makes image too wide for it
			if(vmode.filter == FILTER_NONE) {
				FCEUD_PrintError("Scaled width is out of range.  Reverting to no horizontal scaling.");
				vmode.xscale=1;
			}
			else {
				FCEUD_PrintError("Scaled width is out of range.");
				return false;
			}
		}

		if(FSettings.TotalScanlines()*vmode.yscale > vmode.height) {
			// videomode scale makes image too tall for it
			if(vmode.filter == FILTER_NONE) {
				FCEUD_PrintError("Scaled height is out of range.  Reverting to no vertical scaling.");
				vmode.yscale=1;
			}
			else {
				FCEUD_PrintError("Scaled height is out of range.");
				return false;
			}
		}
		
		vmode.srcRect.left = VNSCLIP;
		vmode.srcRect.top = FSettings.FirstSLine;
		vmode.srcRect.right = 256-VNSCLIP;
		vmode.srcRect.bottom = FSettings.LastSLine+1;

		vmode.dstRect.top = (vmode.height-(FSettings.TotalScanlines()*vmode.yscale)) / 2;
		vmode.dstRect.bottom = vmode.dstRect.top+FSettings.TotalScanlines()*vmode.yscale;
		vmode.dstRect.left = (vmode.width-(VNSWID*vmode.xscale)) / 2;
		vmode.dstRect.right = vmode.dstRect.left+VNSWID*vmode.xscale;
		}

	// -Video Modes Tag-
	if((vmode.filter == FILTER_HQ2X || vmode.filter == FILTER_HQ3X) && vmode.bpp == 8)
		{
		// HQ2x/HQ3x requires 16bpp or 32bpp(best)
		vmode.bpp = 32;
		}

	if(vmode.width<VNSWID)
		{
		FCEUD_PrintError("Horizontal resolution is too low.");
		return false;
		}
	if(vmode.height<FSettings.TotalScanlines() && !FL_TEST(vmode.flags, VIDEOMODEFLAG_STRFS))
		{
		FCEUD_PrintError("Vertical resolution must not be less than the total number of drawn scanlines.");
		return false;
		}

	return true;
}

// Apply current mode indicated by fullscreen var
static bool SetVideoMode()
{
	int specmul = 1;    // Special scaler size multiplier

	if(!fullscreenDesired || vmodeIdx!=0 || RecalcVideoModeParams()) {
		wipeSurface = true;
		updateDDPalette = true;
		if(CreateDDraw()) {

			if(fullscreenDesired) {
				if(vmodeIdx == 0) {
					specmul = filterScaleMultiplier[videoModes[0].filter];
		}

				HideFWindow(1);

				HRESULT status = IDirectDraw7_SetCooperativeLevel ( ddraw7Handle, hAppWnd,DDSCL_EXCLUSIVE | DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT);
				if (status == DD_OK) {
					status = IDirectDraw7_SetDisplayMode(ddraw7Handle, videoModes[vmodeIdx].width, videoModes[vmodeIdx].height, videoModes[vmodeIdx].bpp,0,0);
					if (status == DD_OK) {
						if(FL_TEST(videoModes[vmodeIdx].flags, VIDEOMODEFLAG_DXBLT)) {
							memset(&surfaceDescOffscreen,0,sizeof(surfaceDescOffscreen));
							surfaceDescOffscreen.dwSize=sizeof(surfaceDescOffscreen);
							surfaceDescOffscreen.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
							surfaceDescOffscreen.ddsCaps.dwCaps= DDSCAPS_OFFSCREENPLAIN;

							surfaceDescOffscreen.dwWidth=256 * specmul; //videoModes[vmodeIdx].srcRect.right;
							surfaceDescOffscreen.dwHeight=FSettings.TotalScanlines() * specmul; //videoModes[vmodeIdx].srcRect.bottom;

			if (directDrawModeFullscreen == DIRECTDRAW_MODE_SURFACE_IN_RAM)
				// create the buffer in system memory
								FL_SET(surfaceDescOffscreen.ddsCaps.dwCaps, DDSCAPS_SYSTEMMEMORY); 

							status = IDirectDraw7_CreateSurface(ddraw7Handle, &surfaceDescOffscreen, &lpOffscreenSurface, (IUnknown FAR*)NULL);
		}

						if(status == DD_OK) {
							OnWindowSizeChange(videoModes[vmodeIdx].width, videoModes[vmodeIdx].height);

		// create foreground surface

							memset(&surfaceDescScreen,0,sizeof(surfaceDescScreen));
							surfaceDescScreen.dwSize = sizeof(surfaceDescScreen);

							surfaceDescScreen.dwFlags = DDSD_CAPS;
							surfaceDescScreen.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

							if(idxFullscreenSyncMode == SYNCMODE_DOUBLEBUF)
		{
								FL_SET(surfaceDescScreen.dwFlags, DDSD_BACKBUFFERCOUNT);
								surfaceDescScreen.dwBackBufferCount = 1;
								FL_SET(surfaceDescScreen.ddsCaps.dwCaps, DDSCAPS_COMPLEX | DDSCAPS_FLIP);
		} 

							status = IDirectDraw7_CreateSurface ( ddraw7Handle, &surfaceDescScreen, &lpScreenSurface,(IUnknown FAR*)NULL);
							if (status == DD_OK) {
								if(idxFullscreenSyncMode == SYNCMODE_DOUBLEBUF)
		{
			DDSCAPS2 tmp;

			memset(&tmp,0,sizeof(tmp));
			tmp.dwCaps=DDSCAPS_BACKBUFFER;

									status = IDirectDrawSurface7_GetAttachedSurface(lpScreenSurface,&tmp,&lpBackBuffer);
		}
								if(status == DD_OK) {
									if(InitBPPStuff(fullscreenDesired!=0)) {
										dispModeWasChanged = true;

										if (FL_TEST(eoptions, EO_HIDEMOUSE))
			ShowCursorAbs(0);

										fullscreenActual = true; // now in actual fullscreen mode
										return true;
									}
								}
								else {
									FCEU_printf("Error getting attached surface.\n");
								}
							}
							else {
								FCEU_printf("Error creating primary surface.\n");
							} 
						}
						else {
							FCEU_printf("Error creating secondary surface.\n");
						}
					}
					else {
						FCEU_printf("Error setting display mode.\n");
					}
				}
				else {
					FCEU_printf("Error setting cooperative level.\n");
				}
			}
			else {
				// windowed
				specmul = filterScaleMultiplier[idxFilterModeWindowed];

				ShowCursorAbs(1);
				windowedfailed=1;
				HideFWindow(0);

				HRESULT status = IDirectDraw7_SetCooperativeLevel ( ddraw7Handle, hAppWnd, DDSCL_NORMAL);
				if(status == DD_OK) {
					//Beginning
					memset(&surfaceDescScreen,0,sizeof(surfaceDescScreen));
					surfaceDescScreen.dwSize = sizeof(surfaceDescScreen);
					surfaceDescScreen.dwFlags = DDSD_CAPS;
					surfaceDescScreen.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE;

					status = IDirectDraw7_CreateSurface ( ddraw7Handle, &surfaceDescScreen, &lpScreenSurface,(IUnknown FAR*)NULL);
					if (status == DD_OK) {
						memset(&surfaceDescOffscreen,0,sizeof(surfaceDescOffscreen));
						surfaceDescOffscreen.dwSize=sizeof(surfaceDescOffscreen);
						surfaceDescOffscreen.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
						surfaceDescOffscreen.ddsCaps.dwCaps= DDSCAPS_OFFSCREENPLAIN;

						surfaceDescOffscreen.dwWidth=256 * specmul;
						surfaceDescOffscreen.dwHeight=FSettings.TotalScanlines() * specmul;

						if (directDrawModeWindowed == DIRECTDRAW_MODE_SURFACE_IN_RAM)
							// create the buffer in system memory
							FL_SET(surfaceDescOffscreen.ddsCaps.dwCaps, DDSCAPS_SYSTEMMEMORY);

						status = IDirectDraw7_CreateSurface(ddraw7Handle, &surfaceDescOffscreen, &lpOffscreenSurface, (IUnknown FAR*)NULL);
						if (status == DD_OK) {
							if(InitBPPStuff(fullscreenDesired!=0)) {
								status=IDirectDraw7_CreateClipper(ddraw7Handle,0,&ddClipperHandle,0);
								if (status == DD_OK) {
									status=IDirectDrawClipper_SetHWnd(ddClipperHandle,0,hAppWnd);
									if (status == DD_OK) {
										status=IDirectDrawSurface7_SetClipper(lpScreenSurface,ddClipperHandle);
										if (status == DD_OK) {
											windowedfailed=0;
											SetMainWindowStuff();

											fullscreenActual = false; // now in actual windowed mode
											return true;
										}
										else {
											FCEU_printf("Error attaching clipper to primary surface.\n");
										}
									}
									else {
										FCEU_printf("Error setting clipper window.\n");
									}
								}
								else {
									FCEU_printf("Error creating clipper.\n");
								}
							}
						}
						else {
							FCEU_printf("Error creating secondary surface.\n");
						}
					}
					else {
						FCEU_printf("Error creating primary surface.\n");
					}
				}
				else {
					FCEU_printf("Error setting cooperative level.\n");
				}
			}
		}
	}

	fullscreenDesired = fullscreenActual; // restore fullscreen var
	return false;
}

static void VerticalSync()
{
	if(!NoWaiting) // apparently NoWaiting is set during netplay, which means there will be no vsync
	{
		int syncMode = (fullscreenActual)? idxFullscreenSyncMode:idxWindowedSyncMode;
		switch(syncMode) {
		case SYNCMODE_WAIT:
			IDirectDraw7_WaitForVerticalBlank(ddraw7Handle,DDWAITVB_BLOCKBEGIN,0);
			break;
		case SYNCMODE_LAZYWAIT:
			BOOL invb = 0;
			while((DD_OK == IDirectDraw7_GetVerticalBlankStatus(ddraw7Handle,&invb)) && !invb)
				Sleep(0);
			break;
	}
	}
}

/* Renders XBuf into offscreen surface
// Blits offscreen into screen with either stretch or integer scale
// FIXME doublebuffering settings are ignored, is this correct? */
static void BlitScreenWindow(unsigned char *XBuf)
{
	if (!lpOffscreenSurface) return;

	RECT rectWindow; // window client area in screen space
	if(!GetClientAbsRect(&rectWindow)) return;

	int scale = filterScaleMultiplier[idxFilterModeWindowed];

	if(updateDDPalette) {
		SetPaletteBlitToHigh((uint8*)color_palette);
		updateDDPalette = false;
	}

	// Render XBuf into offscreen surface
	{
		HRESULT status = IDirectDrawSurface7_Lock(lpOffscreenSurface,
			NULL,
			&surfaceDescOffscreen,
			0,
			NULL);

		if (status != DD_OK) {
			if (status == DDERR_SURFACELOST)
				RestoreLostOffscreenSurface();
			return;
	}

		int pitch = surfaceDescOffscreen.lPitch;
		unsigned char *dstOffscreenBuf = (unsigned char*)surfaceDescOffscreen.lpSurface;

		if(wipeSurface) {
			memset(dstOffscreenBuf, 0, pitch * surfaceDescOffscreen.dwHeight);
			wipeSurface = false;
	}
		Blit8ToHigh(XBuf + FSettings.FirstSLine * 256 + VNSCLIP,
			dstOffscreenBuf,
			VNSWID,
			FSettings.TotalScanlines(),
			pitch,
			scale,
			scale);

		IDirectDrawSurface7_Unlock(lpOffscreenSurface, NULL);
	}

	VerticalSync();

	// Blit offscreen to screen surface
	{
		RECT rectSrc; // size of bitmap in offscreen surface
		rectSrc.left = 0;
		rectSrc.top = 0;
		rectSrc.right = VNSWID * scale;
		rectSrc.bottom = FSettings.TotalScanlines() * scale;

		RECT rectDst;
		bool fillBorder = false;
		if (FL_TEST(eoptions, EO_BESTFIT) && (activeRect.top || activeRect.left))
				{
			// blit into activeRect
			rectDst.top = rectWindow.top + activeRect.top;
			rectDst.bottom = rectDst.top + activeRect.bottom - activeRect.top;
			rectDst.left = rectWindow.left + activeRect.left;
			rectDst.right = rectDst.left + activeRect.right - activeRect.left;

			fillBorder = true;
		}
		else {
			// blit into window rect
			rectDst = rectWindow;
		}

		if(IDirectDrawSurface7_Blt(lpScreenSurface, &rectDst, lpOffscreenSurface, &rectSrc, DDBLT_ASYNC, 0) != DD_OK) {
			HRESULT status = IDirectDrawSurface7_Blt(lpScreenSurface, &rectDst, lpOffscreenSurface, &rectSrc, DDBLT_WAIT, 0);
			if(status != DD_OK) {
				if(status == DDERR_SURFACELOST) {
					RestoreLostOffscreenSurface();
					RestoreLostScreenSurface();
				}
				return;
			}
		}

		if (fillBorder) {
			DDBLTFX blitfx = { sizeof(DDBLTFX) };
			if (FL_TEST(eoptions, EO_BGCOLOR)) {
			// fill the surface using BG color from PPU
			unsigned char r, g, b;
			FCEUD_GetPalette(0x80 | PALRAM[0], &r, &g, &b);
			blitfx.dwFillColor = (r << 16) + (g << 8) + b;
			}
			else {
			blitfx.dwFillColor = 0;
		}

			if (activeRect.top) {
			// upper border
				rectDst = rectWindow;
				rectDst.bottom = rectWindow.top + activeRect.top;
				IDirectDrawSurface7_Blt(lpScreenSurface, &rectDst, NULL, NULL, DDBLT_COLORFILL | DDBLT_ASYNC, &blitfx);
			// lower border
				rectDst.top += activeRect.bottom;
				rectDst.bottom = rectWindow.bottom;
				IDirectDrawSurface7_Blt(lpScreenSurface, &rectDst, NULL, NULL, DDBLT_COLORFILL | DDBLT_ASYNC, &blitfx);
		}

			if (activeRect.left) {
			// left border
				rectDst = rectWindow;
				rectDst.right = rectWindow.left + activeRect.left;
				IDirectDrawSurface7_Blt(lpScreenSurface, &rectDst, NULL, NULL, DDBLT_COLORFILL | DDBLT_ASYNC, &blitfx);
			// right border
				rectDst.left += activeRect.right;
				rectDst.right = rectWindow.right;
				IDirectDrawSurface7_Blt(lpScreenSurface, &rectDst, NULL, NULL, DDBLT_COLORFILL | DDBLT_ASYNC, &blitfx);
		}
			}
		}
}

// Renders XBuf into one of three buffers depending on current settings
// If rendered into offscreen will blit into one of screen surface buffers with either stretch or integer scale
// If doublebuffering enabled, when transferring image to screen surface its back buffer is selected, then
// surface is flipped
static void BlitScreenFull(uint8 *XBuf)
{
	// in doublebuffer mode image should end up in screen surface back buffer
	LPDIRECTDRAWSURFACE7 targetScreenSurface = (idxFullscreenSyncMode == SYNCMODE_DOUBLEBUF)? lpBackBuffer:lpScreenSurface;
	if (!targetScreenSurface) return;

	if(updateDDPalette) {
		if(bpp>=16) SetPaletteBlitToHigh((uint8*)color_palette);
		else {
			HRESULT status = IDirectDrawPalette_SetEntries(ddPaletteHandle,0,0,256,color_palette);
			if(status != DD_OK) {
				if(status == DDERR_SURFACELOST) RestoreLostScreenSurface();
				return;
			}   
		}
		updateDDPalette = false;
	}

	char* targetBuf;
	int pitch;
	RECT srcRect;
	RECT displayRect;
	int scale = filterScaleMultiplier[videoModes[vmodeIdx].filter];
	if(FL_TEST(videoModes[vmodeIdx].flags, VIDEOMODEFLAG_DXBLT)) {
		// will render to offscreen surface
		DDSURFACEDESC2 offscreenDesc;
		memset(&offscreenDesc,0,sizeof(offscreenDesc));
		offscreenDesc.dwSize = sizeof(DDSURFACEDESC2);
 		HRESULT status = IDirectDrawSurface7_Lock(lpOffscreenSurface,NULL,&offscreenDesc, 0, NULL);
		if(status != DD_OK) {
			if(status==DDERR_SURFACELOST) RestoreLostOffscreenSurface();
			return;
		}

		targetBuf = (char *)offscreenDesc.lpSurface;
		pitch = offscreenDesc.lPitch;

		srcRect.top = 0;
		srcRect.left = 0;
		srcRect.right = VNSWID * scale;
		srcRect.bottom = FSettings.TotalScanlines() * scale;

		displayRect.top = 0;
		displayRect.left = 0;
		displayRect.right = videoModes[vmodeIdx].width;
		displayRect.bottom = videoModes[vmodeIdx].height;
	}
	else {
		// will render to selected screen buffer
		VerticalSync();

		DDSURFACEDESC2 screenDesc;
		memset(&screenDesc,0,sizeof(screenDesc));
		screenDesc.dwSize = sizeof(DDSURFACEDESC2);
		HRESULT status = IDirectDrawSurface7_Lock(targetScreenSurface,NULL,&screenDesc, 0, NULL);
		if(status != DD_OK) {
			if(status == DDERR_SURFACELOST) RestoreLostScreenSurface();
			return;
	}

		targetBuf = (char*)screenDesc.lpSurface;
		pitch = screenDesc.lPitch;
	}

	if(wipeSurface) {
		int lineCount = (FL_TEST(videoModes[vmodeIdx].flags, VIDEOMODEFLAG_DXBLT))? srcRect.bottom:videoModes[vmodeIdx].height;
		memset(targetBuf, 0, pitch * lineCount);
		updateDDPalette = true;
		wipeSurface = false;
		}

	if(!FL_TEST(videoModes[vmodeIdx].flags, VIDEOMODEFLAG_DXBLT)) {  
			// -Video Modes Tag-
		if(videoModes[vmodeIdx].filter != FILTER_NONE)
			targetBuf += (videoModes[vmodeIdx].dstRect.left * (bpp/8)) + (videoModes[vmodeIdx].dstRect.top * pitch);   
			else
			targetBuf += ((videoModes[vmodeIdx].width-VNSWID)/2)*(bpp/8)+(((videoModes[vmodeIdx].height-FSettings.TotalScanlines())/2))*pitch;
	}

	// Render XBuf into selected target buf
	if(bpp >= 16) {
		Blit8ToHigh(XBuf+FSettings.FirstSLine*256+VNSCLIP,
			(uint8*)targetBuf,
			VNSWID,
			FSettings.TotalScanlines(),
			pitch,
			scale,
			scale);
	}
	else {
		// 8 bpp
		Blit8To8(XBuf+FSettings.FirstSLine*256+VNSCLIP,
			(uint8*)targetBuf,
			VNSWID,
			FSettings.TotalScanlines(),
			pitch,
			(videoModes[vmodeIdx].filter != FILTER_NONE)? videoModes[vmodeIdx].xscale:1,
			(videoModes[vmodeIdx].filter != FILTER_NONE)? videoModes[vmodeIdx].yscale:1,
			0,
			videoModes[vmodeIdx].filter);
	}

	if(FL_TEST(videoModes[vmodeIdx].flags, VIDEOMODEFLAG_DXBLT)) { 
		IDirectDrawSurface7_Unlock(lpOffscreenSurface, NULL);

		VerticalSync();

		bool fillBorder = false;
		RECT* pDstRect;
		if (FL_TEST(eoptions, EO_BESTFIT) && (activeRect.top || activeRect.left) && vmodeIdx==0) {
			pDstRect = &activeRect; // blit offscreen to activeRect
			fillBorder = true;
		}
		else {
			pDstRect = NULL; // blit offscreen to entire surface
		}

		if (IDirectDrawSurface7_Blt(targetScreenSurface, pDstRect, lpOffscreenSurface, &srcRect, DDBLT_ASYNC, 0) != DD_OK) {
			HRESULT status = IDirectDrawSurface7_Blt(targetScreenSurface, pDstRect, lpOffscreenSurface, &srcRect, DDBLT_WAIT, 0);
			if(status != DD_OK)
			{
				if(status == DDERR_SURFACELOST)
				{
					RestoreLostOffscreenSurface();
					RestoreLostScreenSurface();
					}
					return;
				}
			}

		if(fillBorder) {
			DDBLTFX blitfx = { sizeof(DDBLTFX) };
			if (FL_TEST(eoptions, EO_BGCOLOR)) {
				// fill the surface using BG color from PPU
				unsigned char r, g, b;
				FCEUD_GetPalette(0x80 | PALRAM[0], &r, &g, &b);
				blitfx.dwFillColor = (r << 16) + (g << 8) + b;
			}
			else {
				blitfx.dwFillColor = 0;
			}

			RECT borderRect;
			if (pDstRect->top) {
				// upper border
				borderRect = displayRect;
				borderRect.bottom = displayRect.top + pDstRect->top;
				IDirectDrawSurface7_Blt(lpScreenSurface, &borderRect, NULL, NULL, DDBLT_COLORFILL | DDBLT_ASYNC, &blitfx);
				// lower border
				borderRect.top += pDstRect->bottom;
				borderRect.bottom = displayRect.bottom;
				IDirectDrawSurface7_Blt(lpScreenSurface, &borderRect, NULL, NULL, DDBLT_COLORFILL | DDBLT_ASYNC, &blitfx);
			}
			if (activeRect.left)
			{
				// left border
				borderRect = displayRect;
				borderRect.right = displayRect.left + pDstRect->left;
				IDirectDrawSurface7_Blt(lpScreenSurface, &borderRect, NULL, NULL, DDBLT_COLORFILL | DDBLT_ASYNC, &blitfx);
				// right border
				borderRect.left += activeRect.right;
				borderRect.right = displayRect.right;
				IDirectDrawSurface7_Blt(lpScreenSurface, &borderRect, NULL, NULL, DDBLT_COLORFILL | DDBLT_ASYNC, &blitfx);
				}
			}
		}
	else {
		IDirectDrawSurface7_Unlock(targetScreenSurface, NULL);
	}

	if(idxFullscreenSyncMode == SYNCMODE_DOUBLEBUF) {
		IDirectDrawSurface7_Flip(lpScreenSurface,0,0);
	}
}

void InitVideoDriver() {
	SetVideoMode();
}

void ShutdownVideoDriver(void)
{
	ShowCursorAbs(1);
	KillBlitToHigh();
	if(ddraw7Handle) {
		if(dispModeWasChanged)
		{
			IDirectDraw7_RestoreDisplayMode(ddraw7Handle);
			dispModeWasChanged = false;
		}
		}

	if(ddPaletteHandle) {
		ddPaletteHandle->Release();
		ddPaletteHandle = NULL;
			}
	if(lpOffscreenSurface) {
		lpOffscreenSurface->Release();
		lpOffscreenSurface = NULL;
			}
	if(lpScreenSurface) {
		lpScreenSurface->Release();
		lpScreenSurface = NULL;
			}
	if(ddClipperHandle) {
		ddClipperHandle->Release();
		ddClipperHandle = NULL;
		}
	if(ddraw7Handle) {
		ddraw7Handle->Release();
		ddraw7Handle = NULL;
	}
}

//======================
// Settings dialog
//======================
// TODO: here's couple of things that should reside in now-empy gui.cpp
BOOL SetDlgItemDouble(HWND hDlg, int item, double value)
{
	char buf[16];
	sprintf(buf,"%.6f",value);
	return SetDlgItemText(hDlg, item, buf);
}

double GetDlgItemDouble(HWND hDlg, int item)
{
	char buf[16];
	double ret = 0;

	GetDlgItemText(hDlg, item, buf, 15);
	sscanf(buf,"%lf",&ret);
	return(ret);
}

static BOOL CALLBACK VideoConCallB(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
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
	//int x;

	switch(uMsg)
	{
	case WM_INITDIALOG:
	{
		SendDlgItemMessage(hwndDlg,IDC_VIDEOCONFIG_BPP,CB_ADDSTRING,0,(LPARAM)(LPSTR)"8");
		SendDlgItemMessage(hwndDlg,IDC_VIDEOCONFIG_BPP,CB_ADDSTRING,0,(LPARAM)(LPSTR)"16");
		SendDlgItemMessage(hwndDlg,IDC_VIDEOCONFIG_BPP,CB_ADDSTRING,0,(LPARAM)(LPSTR)"24");
		SendDlgItemMessage(hwndDlg,IDC_VIDEOCONFIG_BPP,CB_ADDSTRING,0,(LPARAM)(LPSTR)"32");
		SendDlgItemMessage(hwndDlg,IDC_VIDEOCONFIG_BPP,CB_SETCURSEL,(videoModes[vmodeIdx].bpp/8)-1,(LPARAM)(LPSTR)0);

		SetDlgItemInt(hwndDlg,IDC_VIDEOCONFIG_XRES,videoModes[vmodeIdx].width,0);
		SetDlgItemInt(hwndDlg,IDC_VIDEOCONFIG_YRES,videoModes[vmodeIdx].height,0);

		//SetDlgItemInt(hwndDlg,IDC_VIDEOCONFIG_XSCALE,videoModes[vmodeIdx].xscale,0);
		//SetDlgItemInt(hwndDlg,IDC_VIDEOCONFIG_YSCALE,videoModes[vmodeIdx].yscale,0);
		//CheckRadioButton(hwndDlg,IDC_RADIO_SCALE,IDC_RADIO_STRETCH,FL_TEST(videoModes[vmodeIdx].flags, VIDEOMODEFLAG_STRFS)?IDC_RADIO_STRETCH:IDC_RADIO_SCALE);

		// -Video Modes Tag-
		char *str[]={"<none>","hq2x","Scale2x","NTSC 2x","hq3x","Scale3x"};
		int x;
		for(x=0;x<6;x++)
		{
			SendDlgItemMessage(hwndDlg,IDC_VIDEOCONFIG_SCALER_FS,CB_ADDSTRING,0,(LPARAM)(LPSTR)str[x]);
			SendDlgItemMessage(hwndDlg,IDC_VIDEOCONFIG_SCALER_WIN,CB_ADDSTRING,0,(LPARAM)(LPSTR)str[x]);
		}

		SendDlgItemMessage(hwndDlg, IDC_VIDEOCONFIG_SCALER_FS, CB_SETCURSEL, videoModes[vmodeIdx].filter, (LPARAM)(LPSTR)0);
		SendDlgItemMessage(hwndDlg, IDC_VIDEOCONFIG_SCALER_WIN, CB_SETCURSEL, idxFilterModeWindowed, (LPARAM)(LPSTR)0);

		// Direct Draw modes
		SendDlgItemMessage(hwndDlg, IDC_VIDEOCONFIG_DIRECTDRAW_WIN, CB_ADDSTRING, 0, (LPARAM)(LPSTR)"No hardware acceleration");
		SendDlgItemMessage(hwndDlg, IDC_VIDEOCONFIG_DIRECTDRAW_WIN, CB_ADDSTRING, 0, (LPARAM)(LPSTR)"Create Surface in RAM");
		SendDlgItemMessage(hwndDlg, IDC_VIDEOCONFIG_DIRECTDRAW_WIN, CB_ADDSTRING, 0, (LPARAM)(LPSTR)"Hardware acceleration");

		SendDlgItemMessage(hwndDlg, IDC_VIDEOCONFIG_DIRECTDRAW_WIN, CB_SETCURSEL, directDrawModeWindowed, (LPARAM)(LPSTR)0);

		SendDlgItemMessage(hwndDlg, IDC_VIDEOCONFIG_DIRECTDRAW_FS, CB_ADDSTRING, 0, (LPARAM)(LPSTR)"No hardware acceleration");
		SendDlgItemMessage(hwndDlg, IDC_VIDEOCONFIG_DIRECTDRAW_FS, CB_ADDSTRING, 0, (LPARAM)(LPSTR)"Create Surface in RAM");
		SendDlgItemMessage(hwndDlg, IDC_VIDEOCONFIG_DIRECTDRAW_FS, CB_ADDSTRING, 0, (LPARAM)(LPSTR)"Hardware acceleration");

		SendDlgItemMessage(hwndDlg, IDC_VIDEOCONFIG_DIRECTDRAW_FS, CB_SETCURSEL, directDrawModeFullscreen, (LPARAM)(LPSTR)0);

		if(FL_TEST(eoptions, EO_FSAFTERLOAD))
			CheckDlgButton(hwndDlg,IDC_VIDEOCONFIG_AUTO_FS,BST_CHECKED);

		if(FL_TEST(eoptions, EO_HIDEMOUSE))
			CheckDlgButton(hwndDlg,IDC_VIDEOCONFIG_HIDEMOUSE,BST_CHECKED);

		if(FL_TEST(eoptions, EO_CLIPSIDES))
			CheckDlgButton(hwndDlg,IDC_VIDEOCONFIG_CLIPSIDES,BST_CHECKED);

		if(FL_TEST(eoptions, EO_BESTFIT))
			CheckDlgButton(hwndDlg, IDC_VIDEOCONFIG_BESTFIT, BST_CHECKED);

		if(FL_TEST(eoptions, EO_BGCOLOR))
			CheckDlgButton(hwndDlg,IDC_VIDEOCONFIG_CONSOLE_BGCOLOR,BST_CHECKED);

		if(FL_TEST(eoptions, EO_SQUAREPIXELS))
			CheckDlgButton(hwndDlg, IDC_VIDEOCONFIG_SQUARE_PIXELS, BST_CHECKED);

		if(FL_TEST(eoptions, EO_TVASPECT))
			CheckDlgButton(hwndDlg, IDC_VIDEOCONFIG_TVASPECT, BST_CHECKED);

		if(FL_TEST(eoptions, EO_FORCEISCALE))
			CheckDlgButton(hwndDlg,IDC_FORCE_INT_VIDEO_SCALARS,BST_CHECKED);

		if(FL_TEST(eoptions, EO_FORCEASPECT))
			CheckDlgButton(hwndDlg,IDC_FORCE_ASPECT_CORRECTION,BST_CHECKED);

		SetDlgItemInt(hwndDlg,IDC_SCANLINE_FIRST_NTSC,srendlinen,0);
		SetDlgItemInt(hwndDlg,IDC_SCANLINE_LAST_NTSC,erendlinen,0);

		SetDlgItemInt(hwndDlg,IDC_SCANLINE_FIRST_PAL,srendlinep,0);
		SetDlgItemInt(hwndDlg,IDC_SCANLINE_LAST_PAL,erendlinep,0);


		SetDlgItemDouble(hwndDlg, IDC_WINSIZE_MUL_X, winsizemulx);
		SetDlgItemDouble(hwndDlg, IDC_WINSIZE_MUL_Y, winsizemuly);
		SetDlgItemDouble(hwndDlg, IDC_TVASPECT_X, tvAspectX);
		SetDlgItemDouble(hwndDlg, IDC_TVASPECT_Y, tvAspectY);

		SendDlgItemMessage(hwndDlg,IDC_VIDEOCONFIG_SYNC_METHOD_WIN,CB_ADDSTRING,0,(LPARAM)(LPSTR)"<none>");
		SendDlgItemMessage(hwndDlg,IDC_VIDEOCONFIG_SYNC_METHOD_FS,CB_ADDSTRING,0,(LPARAM)(LPSTR)"<none>");

		SendDlgItemMessage(hwndDlg,IDC_VIDEOCONFIG_SYNC_METHOD_WIN,CB_ADDSTRING,0,(LPARAM)(LPSTR)"Wait for VBlank");
		SendDlgItemMessage(hwndDlg,IDC_VIDEOCONFIG_SYNC_METHOD_WIN,CB_ADDSTRING,0,(LPARAM)(LPSTR)"Lazy wait for VBlank");

		SendDlgItemMessage(hwndDlg,IDC_VIDEOCONFIG_SYNC_METHOD_FS,CB_ADDSTRING,0,(LPARAM)(LPSTR)"Wait for VBlank");
		SendDlgItemMessage(hwndDlg,IDC_VIDEOCONFIG_SYNC_METHOD_FS,CB_ADDSTRING,0,(LPARAM)(LPSTR)"Lazy wait for VBlank");
		SendDlgItemMessage(hwndDlg,IDC_VIDEOCONFIG_SYNC_METHOD_FS,CB_ADDSTRING,0,(LPARAM)(LPSTR)"Double Buffering");

		SendDlgItemMessage(hwndDlg,IDC_VIDEOCONFIG_SYNC_METHOD_WIN,CB_SETCURSEL,idxWindowedSyncMode,(LPARAM)(LPSTR)0);
		SendDlgItemMessage(hwndDlg,IDC_VIDEOCONFIG_SYNC_METHOD_FS,CB_SETCURSEL,idxFullscreenSyncMode,(LPARAM)(LPSTR)0);

		if(FL_TEST(eoptions, EO_NOSPRLIM))
			CheckDlgButton(hwndDlg,IDC_VIDEOCONFIG_NO8LIM,BST_CHECKED);

		char buf[1024] = "Full Screen";
		int c = FCEUD_CommandMapping[EMUCMD_MISC_TOGGLEFULLSCREEN];
		if (c)
		{
			strcat(buf, " (");
			strcat(buf, GetKeyComboName(c));
			if (GetIsFullscreenOnDoubleclick())
				strcat(buf, " or double-click)");
			else
				strcat(buf, ")");
		} else if (GetIsFullscreenOnDoubleclick())
		{
			strcat(buf, " (double-click anywhere)");
		}
		SetDlgItemText(hwndDlg, IDC_VIDEOCONFIG_FS, buf);
		break;
	}
	case WM_CLOSE:
	case WM_QUIT: goto gornk;
	case WM_COMMAND:
		if(!(wParam>>16))
			switch(wParam&0xFFFF)
		{
			case ID_CANCEL:
gornk:

				if(IsDlgButtonChecked(hwndDlg,IDC_VIDEOCONFIG_CLIPSIDES)==BST_CHECKED)
				{
					FL_SET(eoptions, EO_CLIPSIDES);
					ClipSidesOffset = 8;
				}
				else
				{
					FL_CLEAR(eoptions, EO_CLIPSIDES);
					ClipSidesOffset = 0;
				}

				FL_FROMBOOL(eoptions, EO_BESTFIT, IsDlgButtonChecked(hwndDlg, IDC_VIDEOCONFIG_BESTFIT) == BST_CHECKED);
				FL_FROMBOOL(eoptions, EO_BGCOLOR, IsDlgButtonChecked(hwndDlg, IDC_VIDEOCONFIG_CONSOLE_BGCOLOR) == BST_CHECKED);
				FL_FROMBOOL(eoptions, EO_SQUAREPIXELS, IsDlgButtonChecked(hwndDlg, IDC_VIDEOCONFIG_SQUARE_PIXELS) == BST_CHECKED);
				FL_FROMBOOL(eoptions, EO_TVASPECT, IsDlgButtonChecked(hwndDlg, IDC_VIDEOCONFIG_TVASPECT) == BST_CHECKED);
				FL_FROMBOOL(eoptions, EO_NOSPRLIM, IsDlgButtonChecked(hwndDlg,IDC_VIDEOCONFIG_NO8LIM) == BST_CHECKED);

				srendlinen=GetDlgItemInt(hwndDlg,IDC_SCANLINE_FIRST_NTSC,0,0);
				erendlinen=GetDlgItemInt(hwndDlg,IDC_SCANLINE_LAST_NTSC,0,0);
				srendlinep=GetDlgItemInt(hwndDlg,IDC_SCANLINE_FIRST_PAL,0,0);
				erendlinep=GetDlgItemInt(hwndDlg,IDC_SCANLINE_LAST_PAL,0,0);


				if(erendlinen>239) erendlinen=239;
				if(srendlinen>erendlinen) srendlinen=erendlinen;

				if(erendlinep>239) erendlinep=239;
				if(srendlinep>erendlinen) srendlinep=erendlinep;

				UpdateRendBounds();

				videoModes[0].width=GetDlgItemInt(hwndDlg,IDC_VIDEOCONFIG_XRES,0,0);
				videoModes[0].height=GetDlgItemInt(hwndDlg,IDC_VIDEOCONFIG_YRES,0,0);
				videoModes[0].bpp=(SendDlgItemMessage(hwndDlg,IDC_VIDEOCONFIG_BPP,CB_GETCURSEL,0,(LPARAM)(LPSTR)0)+1)<<3;

				videoModes[0].filter = static_cast<VFILTER>(SendDlgItemMessage(hwndDlg,IDC_VIDEOCONFIG_SCALER_FS,CB_GETCURSEL,0,(LPARAM)(LPSTR)0));

				idxFilterModeWindowed = static_cast<VFILTER>(SendDlgItemMessage(hwndDlg, IDC_VIDEOCONFIG_SCALER_WIN, CB_GETCURSEL, 0, (LPARAM)(LPSTR)0));

				directDrawModeWindowed = static_cast<DIRECTDRAW_MODE>(SendDlgItemMessage(hwndDlg, IDC_VIDEOCONFIG_DIRECTDRAW_WIN, CB_GETCURSEL, 0, (LPARAM)(LPSTR)0));
				if(directDrawModeWindowed >= DIRECTDRAW_MODES_TOTAL) {
					FCEU_printf("DirectDraw windowed mode index out of bounds.\n");
					directDrawModeWindowed = DIRECTDRAW_MODE_SOFTWARE;
				}

				directDrawModeFullscreen = static_cast<DIRECTDRAW_MODE>(SendDlgItemMessage(hwndDlg, IDC_VIDEOCONFIG_DIRECTDRAW_FS, CB_GETCURSEL, 0, (LPARAM)(LPSTR)0));
				if(directDrawModeFullscreen >= DIRECTDRAW_MODES_TOTAL) {
					FCEU_printf("DirectDraw fullscreen mode index out of bounds.\n");
					directDrawModeFullscreen = DIRECTDRAW_MODE_SOFTWARE;
				}

				SetIsFullscreen( IsDlgButtonChecked(hwndDlg,IDC_VIDEOCONFIG_FS)==BST_CHECKED );

				FL_FROMBOOL(eoptions, EO_FSAFTERLOAD, IsDlgButtonChecked(hwndDlg,IDC_VIDEOCONFIG_AUTO_FS)==BST_CHECKED);
				FL_FROMBOOL(eoptions, EO_HIDEMOUSE, IsDlgButtonChecked(hwndDlg,IDC_VIDEOCONFIG_HIDEMOUSE)==BST_CHECKED);

				FL_FROMBOOL(eoptions, EO_FORCEISCALE, IsDlgButtonChecked(hwndDlg,IDC_FORCE_INT_VIDEO_SCALARS)==BST_CHECKED);
				FL_FROMBOOL(eoptions, EO_FORCEASPECT, IsDlgButtonChecked(hwndDlg,IDC_FORCE_ASPECT_CORRECTION)==BST_CHECKED);

				winsizemulx = GetDlgItemDouble(hwndDlg, IDC_WINSIZE_MUL_X);
				winsizemuly = GetDlgItemDouble(hwndDlg, IDC_WINSIZE_MUL_Y);
				FixWXY(0);
				tvAspectX = GetDlgItemDouble(hwndDlg, IDC_TVASPECT_X);
				tvAspectY = GetDlgItemDouble(hwndDlg, IDC_TVASPECT_Y);
				if (tvAspectX < 0.1)
					tvAspectX = 0.1;
				if (tvAspectY < 0.1)
					tvAspectY = 0.1;

				idxWindowedSyncMode = static_cast<VSYNCMODE>(SendDlgItemMessage(hwndDlg,IDC_VIDEOCONFIG_SYNC_METHOD_WIN,CB_GETCURSEL,0,(LPARAM)(LPSTR)0));
				idxFullscreenSyncMode = static_cast<VSYNCMODE>(SendDlgItemMessage(hwndDlg,IDC_VIDEOCONFIG_SYNC_METHOD_FS,CB_GETCURSEL,0,(LPARAM)(LPSTR)0));
				EndDialog(hwndDlg,0);
				break;
		}
	}
	return 0;
}

void DoVideoConfigFix()
{
	FCEUI_DisableSpriteLimitation(FL_TEST(eoptions, EO_NOSPRLIM));
	UpdateRendBounds();
}

//Shows the Video configuration dialog.
void ShowConfigVideoDialog(void)
{
	if ((videoModes[0].width <= 0) || (videoModes[0].height <= 0))
		ResetVideoModeParams();
	DialogBox(fceu_hInstance, "VIDEOCONFIG", hAppWnd, VideoConCallB); 
	DoVideoConfigFix();
	FCEUD_VideoChanged();
}


//======================
// Get/Set
//======================

RECT GetActiveRect() {
	return activeRect;
}

PALETTEENTRY* GetPalette() {
	return color_palette;
}

bool GetIsFullscreen() {
	return (fullscreenActual != 0);
}

void SetIsFullscreen(bool f) {
	fullscreenDesired = f;
}

VSYNCMODE GetWindowedSyncModeIdx() {
	return idxWindowedSyncMode;
}

void SetWindowedSyncModeIdx(VSYNCMODE idx) {
	idxWindowedSyncMode = idx;
}


//======================
// FCEUD
//======================
//draw input aids if we are fullscreen
bool FCEUD_ShouldDrawInputAids()
{
	return GetIsFullscreen();
}

//static uint8 *XBSave;
void FCEUD_BlitScreen(uint8 *XBuf)
{
	if(fullscreenActual)
	{
		BlitScreenFull(XBuf);
	}
	else
	{
		if(!windowedfailed)
			BlitScreenWindow(XBuf);
	}
}

void FCEUD_VideoChanged()
{
		changerecursive = 1;
	if(!SetVideoMode()) {
		SetIsFullscreen(false);
		SetVideoMode();
	}
	changerecursive = 0;
}

void FCEUD_SetPalette(unsigned char index, unsigned char r, unsigned char g, unsigned char b)
{
	if (force_grayscale)
	{
		// convert the palette entry to grayscale
		int gray = ((float)r * 0.299 + (float)g * 0.587 + (float)b * 0.114);
		color_palette[index].peRed = gray;
		color_palette[index].peGreen = gray;
		color_palette[index].peBlue = gray;
	} else
	{
		color_palette[index].peRed = r;
		color_palette[index].peGreen = g;
		color_palette[index].peBlue = b;
	}
	updateDDPalette = true;
}

void FCEUD_GetPalette(unsigned char i, unsigned char *r, unsigned char *g, unsigned char *b)
{
	*r=color_palette[i].peRed;
	*g=color_palette[i].peGreen;
	*b=color_palette[i].peBlue;
}


bool& _FIXME_getFullscreenVar() {
	return fullscreenActual; // provide config with actual fullscreen state, not desired
}

int& _FIXME_getVModeIdxVar() {
	return vmodeIdx;
}

VSYNCMODE& _FIXME_getFullscreenSyncModeIdxVar() {
	return idxFullscreenSyncMode;
}

VSYNCMODE& _FIXME_getWindowedSyncModeIdxVar() {
	return idxWindowedSyncMode;
}

VFILTER& _FIXME_getFilterModeWindowedIdxVar() {
	return idxFilterModeWindowed;
}

int& _FIXME_getFilterOptionVar() {
	return ntscFilterOption;
}

DIRECTDRAW_MODE& _FIXME_getDDrawModeWindowedVar() {
	return directDrawModeWindowed;
}

DIRECTDRAW_MODE& _FIXME_getDDrawModeFullscreenVar() {
	return directDrawModeFullscreen;
}

VideoMode& _FIXME_getCustomVideoModeVar() {
	return videoModes[0];
}
