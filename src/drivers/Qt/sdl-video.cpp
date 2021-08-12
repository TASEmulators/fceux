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

/// \file
/// \brief Handles the graphical game display for the SDL implementation.

#include "Qt/sdl.h"
#include "Qt/nes_shm.h"
#include "common/vidblit.h"
#include "../../fceu.h"
#include "../../version.h"
#include "../../video.h"
#include "../../input.h"

#include "utils/memory.h"

#include "Qt/dface.h"

#include "common/configSys.h"
#include "Qt/sdl-video.h"
#include "Qt/AviRecord.h"
#include "Qt/fceuWrapper.h"

#ifdef CREATE_AVI
#include "../videolog/nesvideos-piece.h"
#endif

#include <cstdio>
#include <cstring>
#include <cstdlib>

#if __BYTE_ORDER == __LITTLE_ENDIAN
#define  LSB_FIRST 
#endif

// GLOBALS
extern Config *g_config;

// STATIC GLOBALS
static int s_curbpp = 0;
static int s_srendline, s_erendline;
static int s_tlines;
static int s_inited = 0;

static double s_exs = 1.0, s_eys = 1.0;
static int s_eefx = 0;
static int s_clipSides = 0;
static int s_fullscreen = 0;
static int noframe = 0;
static int initBlitToHighDone = 0;

#define NWIDTH	(256 - (s_clipSides ? 16 : 0))
#define NOFFSET	(s_clipSides ? 8 : 0)

static int s_paletterefresh = 1;

extern bool MaxSpeed;
extern int input_display;
extern int frame_display;
extern int rerecord_display;

/**
 * Attempts to destroy the graphical video display.  Returns 0 on
 * success, -1 on failure.
 */
int
KillVideo(void)
{
	//printf("Killing Video\n");

	if ( nes_shm != NULL )
	{
		nes_shm->clear_pixbuf();
	}

	// if the rest of the system has been initialized, shut it down
	// shut down the system that converts from 8 to 16/32 bpp
	if (initBlitToHighDone)
	{
		KillBlitToHigh();

		initBlitToHighDone = 0;
	}

	// return failure if the video system was not initialized
	if (s_inited == 0)
	{
		return -1;
	}

	// SDL Video system is not used.
	// shut down the SDL video sub-system
	//SDL_QuitSubSystem(SDL_INIT_VIDEO);

	s_inited = 0;
	return 0;
}


// this variable contains information about the special scaling filters
static int s_sponge = 0;

/**
 * These functions determine an appropriate scale factor for fullscreen/
 */
inline double GetXScale(int xres)
{
	return ((double)xres) / NWIDTH;
}
inline double GetYScale(int yres)
{
	return ((double)yres) / s_tlines;
}
void FCEUD_VideoChanged()
{
	int buf;
	g_config->getOption("SDL.PAL", &buf);
	if(buf == 1)
		PAL = 1;
	else
		PAL = 0; // NTSC and Dendy
}

void CalcVideoDimensions(void)
{
	g_config->getOption("SDL.SpecialFilter", &s_sponge);

	FCEUI_GetCurrentVidSystem(&s_srendline, &s_erendline);
	s_tlines = s_erendline - s_srendline + 1;

	//printf("Calc Video: %i -> %i \n", s_srendline, s_erendline );

	nes_shm->video.preScaler = s_sponge;

	switch ( s_sponge )
	{
		default:
		case 0: // None
			nes_shm->video.xscale = 1;
			nes_shm->video.yscale = 1;
		break;
		case 1: // hq2x
		case 2: // Scale2x
		case 3: // NTSC 2x
		case 6: // Prescale2x
			nes_shm->video.xscale = 2;
			nes_shm->video.yscale = 2;
		break;
		case 4: // hq3x
		case 5: // Scale3x
		case 7: // Prescale3x
			nes_shm->video.xscale = 3;
			nes_shm->video.yscale = 3;
		break;
		case 8: // Prescale4x
			nes_shm->video.xscale = 4;
			nes_shm->video.yscale = 4;
		break;
		case 9: // PAL
			nes_shm->video.xscale = 3;
			nes_shm->video.yscale = 1;
		break;
	}

	int iScale = nes_shm->video.xscale;
	if ( s_sponge == 3 )
	{
		nes_shm->video.ncol = iScale*301;
	}
	else
	{
		nes_shm->video.ncol = iScale*NWIDTH;
	}
	if ( s_sponge == 9 )
	{
		nes_shm->video.nrow  = 1*s_tlines;
		nes_shm->video.xyRatio = 3;
	}
	else
	{
		nes_shm->video.nrow  = iScale*s_tlines;
		nes_shm->video.xyRatio = 1;
	}
	nes_shm->video.pitch = nes_shm->video.ncol * 4;
}

int InitVideo(FCEUGI *gi)
{
	int doublebuf, xstretch, ystretch, xres, yres;
	int show_fps;
	int startNTSC, endNTSC, startPAL, endPAL;

	FCEUI_printf("Initializing video...");

	// load the relevant configuration variables
	g_config->getOption("SDL.Fullscreen", &s_fullscreen);
	g_config->getOption("SDL.DoubleBuffering", &doublebuf);
	g_config->getOption("SDL.SpecialFilter", &s_sponge);
	g_config->getOption("SDL.XStretch", &xstretch);
	g_config->getOption("SDL.YStretch", &ystretch);
	//g_config->getOption("SDL.LastXRes", &xres);
	//g_config->getOption("SDL.LastYRes", &yres);
	g_config->getOption("SDL.ClipSides", &s_clipSides);
	g_config->getOption("SDL.NoFrame", &noframe);
	g_config->getOption("SDL.ShowFPS", &show_fps);
	g_config->getOption("SDL.ShowFrameCount", &frame_display);
	g_config->getOption("SDL.ShowLagCount", &lagCounterDisplay);
	g_config->getOption("SDL.ShowRerecordCount", &rerecord_display);
	//g_config->getOption("SDL.XScale", &s_exs);
	//g_config->getOption("SDL.YScale", &s_eys);
	g_config->getOption("SDL.ScanLineStartNTSC", &startNTSC);
	g_config->getOption("SDL.ScanLineEndNTSC", &endNTSC);
	g_config->getOption("SDL.ScanLineStartPAL", &startPAL);
	g_config->getOption("SDL.ScanLineEndPAL", &endPAL);
	uint32_t  rmask, gmask, bmask;

	ClipSidesOffset = s_clipSides ? 8 : 0;

	FCEUI_SetRenderedLines(startNTSC, endNTSC, startPAL, endPAL);

	s_exs = 1.0;
	s_eys = 1.0;
	xres = gui_draw_area_width;
	yres = gui_draw_area_height;
	// check the starting, ending, and total scan lines

	FCEUI_GetCurrentVidSystem(&s_srendline, &s_erendline);
	s_tlines = s_erendline - s_srendline + 1;

	nes_shm->video.preScaler = s_sponge;

	switch ( s_sponge )
	{
		default:
		case 0: // None
			nes_shm->video.xscale = 1;
			nes_shm->video.yscale = 1;
		break;
		case 1: // hq2x
		case 2: // Scale2x
		case 3: // NTSC 2x
		case 6: // Prescale2x
			nes_shm->video.xscale = 2;
			nes_shm->video.yscale = 2;
		break;
		case 4: // hq3x
		case 5: // Scale3x
		case 7: // Prescale3x
			nes_shm->video.xscale = 3;
			nes_shm->video.yscale = 3;
		break;
		case 8: // Prescale4x
			nes_shm->video.xscale = 4;
			nes_shm->video.yscale = 4;
		break;
		case 9: // PAL
			nes_shm->video.xscale = 3;
			nes_shm->video.yscale = 1;
		break;
	}
	nes_shm->render_count = nes_shm->blit_count = 0;

	s_inited = 1;

	// check to see if we are showing FPS
	FCEUI_SetShowFPS(show_fps);

	int iScale = nes_shm->video.xscale;
	if ( s_sponge == 3 )
	{
		nes_shm->video.ncol = iScale*301;
	}
	else
	{
		nes_shm->video.ncol = iScale*NWIDTH;
	}
	if ( s_sponge == 9 )
	{
		nes_shm->video.nrow  = 1*s_tlines;
		nes_shm->video.xyRatio = 3;
	}
	else
	{
		nes_shm->video.nrow  = iScale*s_tlines;
		nes_shm->video.xyRatio = 1;
	}
	nes_shm->video.pitch = nes_shm->video.ncol * 4;

#ifdef LSB_FIRST
	rmask = 0x00FF0000;
	gmask = 0x0000FF00;
	bmask = 0x000000FF;
#else
	rmask = 0x00FF0000;
	gmask = 0x0000FF00;
	bmask = 0x000000FF;
#endif

	s_curbpp = 32; // Bits per pixel is always 32

	FCEU_printf(" Video Mode: %d x %d x %d bpp %s\n",
				xres, yres, s_curbpp,
				s_fullscreen ? "full screen" : "");

	if (s_curbpp != 8 && s_curbpp != 16 && s_curbpp != 24 && s_curbpp != 32) 
	{
		FCEU_printf("  Sorry, %dbpp modes are not supported by FCE Ultra.  Supported bit depths are 8bpp, 16bpp, and 32bpp.\n", s_curbpp);
		KillVideo();
		return -1;
	}

	if ( !initBlitToHighDone )
	{
		InitBlitToHigh(s_curbpp >> 3,
							rmask,
							gmask,
							bmask,
							s_eefx, s_sponge, 0);

		initBlitToHighDone = 1;
	}

	s_paletterefresh = 1;

	return 0;
}

/**
 * Toggles the full-screen display.
 */
void ToggleFS(void)
{
    // pause while we we are making the switch
	bool paused = FCEUI_EmulationPaused();
	if(!paused)
		FCEUI_ToggleEmulationPause();

	// flip the fullscreen flag
	g_config->setOption("SDL.Fullscreen", !s_fullscreen);

	// TODO Call method to make full Screen

	// if we paused to make the switch; unpause
	if(!paused)
		FCEUI_ToggleEmulationPause();
}

static SDL_Color s_psdl[256];

/**
 * Sets the color for a particular index in the palette.
 */
void
FCEUD_SetPalette(uint8 index,
                 uint8 r,
                 uint8 g,
                 uint8 b)
{
	s_psdl[index].r = r;
	s_psdl[index].g = g;
	s_psdl[index].b = b;

	s_paletterefresh = 1;
}

/**
 * Gets the color for a particular index in the palette.
 */
void
FCEUD_GetPalette(uint8 index,
				uint8 *r,
				uint8 *g,
				uint8 *b)
{
	*r = s_psdl[index].r;
	*g = s_psdl[index].g;
	*b = s_psdl[index].b;
}

/** 
 * Pushes the palette structure into the underlying video subsystem.
 */
static void RedoPalette()
{
	if (s_curbpp > 8) 
	{
		//printf("Refresh Palette\n");
		SetPaletteBlitToHigh((uint8*)s_psdl);
	} 
}
// XXX soules - console lock/unlock unimplemented?

///Currently unimplemented.
void LockConsole(){}

///Currently unimplemented.
void UnlockConsole(){}

static int testPattern = 0;

static void WriteTestPattern(void)
{
	int i, j, k;

	k=0;
	for (i=0; i<GL_NES_WIDTH; i++)
	{
		for (j=0; j<GL_NES_HEIGHT; j++)
		{
			nes_shm->pixbuf[k] = 0xffffffff; k++;
		}
	}
}

static void
doBlitScreen(uint8_t *XBuf, uint8_t *dest)
{
	int w, h, pitch, bw, ixScale, iyScale;

	// refresh the palette if required
	if (s_paletterefresh) 
	{
		RedoPalette();
		s_paletterefresh = 0;
	}

	// XXX soules - not entirely sure why this is being done yet
	XBuf += s_srendline * 256;

	//dest    = (uint8*)nes_shm->pixbuf;
	ixScale = nes_shm->video.xscale;
	iyScale = nes_shm->video.yscale;

	if ( s_sponge == 3 )
	{
		w = ixScale*301;
		bw = 256;
	}
	else
	{
		w = ixScale*NWIDTH;
		bw = NWIDTH;
	}
	if ( s_sponge == 9 )
	{
		h  = 1*s_tlines;
	}
	else
	{
		h  = ixScale*s_tlines;
	}
	pitch  = w*4;

	nes_shm->video.ncol    = w;
	nes_shm->video.nrow    = h;
	nes_shm->video.pitch   = pitch;
	nes_shm->video.preScaler = s_sponge;

	if ( dest == NULL ) return;

	if ( testPattern )
	{
		WriteTestPattern();
	}
	else
	{
		Blit8ToHigh(XBuf + NOFFSET, dest, bw, s_tlines, pitch, ixScale, iyScale);
	}
}
/**
 * Pushes the given buffer of bits to the screen.
 */
void
BlitScreen(uint8 *XBuf)
{
	doBlitScreen(XBuf, (uint8_t*)nes_shm->pixbuf);

	nes_shm->blit_count++;
	nes_shm->blitUpdated = 1;
}

void FCEUI_AviVideoUpdate(const unsigned char* buffer)
{	// This is not used by Qt Emulator, avi recording pulls from the post processed video buffer
	// instead of emulation core video buffer. This allows for the video scaler effects
	// and higher resolution to be seen in recording.
	doBlitScreen( (uint8_t*)buffer, (uint8_t*)nes_shm->avibuf);

	aviRecordAddFrame();

	return;
}

/**
 *  Converts an x-y coordinate in the window manager into an x-y
 *  coordinate on FCEU's screen.
 */
uint32 PtoV(double nx, double ny)
{
	int x, y;
	y = (int)( ny * (double)nes_shm->video.nrow );
	x = (int)( nx * (double)nes_shm->video.ncol );

	//printf("Scaled (%i,%i) \n", x, y);

	x = x / nes_shm->video.xscale;
	y = y / nes_shm->video.yscale;

	//if ( nes_shm->video.xyRatio == 1 )
	//{
	//	y = y / nes_shm->video.scale;
	//}
	//printf("UnScaled (%i,%i) \n", x, y);

	if (s_clipSides) 
	{
		x += 8;
	}
	y += s_srendline;

	return (x | (y << 16));
}

bool enableHUDrecording = false;
bool FCEUI_AviEnableHUDrecording()
{
	if (enableHUDrecording)
		return true;

	return false;
}
void FCEUI_SetAviEnableHUDrecording(bool enable)
{
	enableHUDrecording = enable;
}

bool disableMovieMessages = false;
bool FCEUI_AviDisableMovieMessages()
{
	if (disableMovieMessages)
		return true;

	return false;
}
void FCEUI_SetAviDisableMovieMessages(bool disable)
{
	disableMovieMessages = disable;
}
