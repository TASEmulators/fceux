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

/// \file
/// \brief Handles the graphical game display for the SDL implementation.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sdl.h"
#include "sdl-opengl.h"
#include "../common/vidblit.h"

#include "sdl-icon.h"
#include "dface.h"

SDL_Surface *screen;
SDL_Surface *BlitBuf;		// Used as a buffer when using hardware-accelerated blits.
SDL_Surface *IconSurface=NULL;

static int curbpp;
static int srendline,erendline;
static int tlines;
static int inited;

#ifdef OPENGL
extern int sdlhaveogl;
static int usingogl;
#endif
static double exs,eys;
static int eefx;

#define NWIDTH	(256 - ((eoptions & EO_CLIPSIDES) ? 16 : 0))
#define NOFFSET	(eoptions & EO_CLIPSIDES ? 8 : 0)


static int paletterefresh;

/**
 * Attempts to destroy the graphical video display.  Returns 0 on
 * success, -1 on failure.
 */
int
KillVideo()
{
    // if the IconSurface has been initialized, destroy it
    if(IconSurface) {
        SDL_FreeSurface(IconSurface);
        IconSurface=0;
    }

    // if the rest of the system has been initialized, shut it down
    if(inited) {
#ifdef OPENGL
        // check for OpenGL and shut it down
        if(usingogl)
            KillOpenGL();
        else
#endif
            // shut down the system that converts from 8 to 16/32 bpp
            if(curbpp > 8)
                KillBlitToHigh();

        // shut down the SDL video sub-system
        SDL_QuitSubSystem(SDL_INIT_VIDEO);

        inited = 0;
        return 0;
    }

    // return failure, since the system was not initialized
    // XXX soules - this seems odd to me... why is it doing this?
    return -1;
}

static int sponge;

/**
 * Attempts to initialize the graphical video display.  Returns 0 on
 * success, -1 on failure.
 */
int
InitVideo(FCEUGI *gi)
{
    // XXX soules - const?  is this necessary?
    const SDL_VideoInfo *vinf;
    int error, flags = 0;

    FCEUI_printf("Initializing video...");

    // check the starting, ending, and total scan lines
    FCEUI_GetCurrentVidSystem(&srendline, &erendline);
    tlines = erendline - srendline + 1;

    // XXX soules - what is the sponge variable?
    if(_fullscreen) {
        sponge = Settings.specialfs;
    } else {
        sponge = Settings.special;
    }

    // check for OpenGL and set the global flags
#ifdef OPENGL
    usingogl = 0;
    if(_opengl && sdlhaveogl && !sponge) {
        flags = SDL_OPENGL;
        usingogl = 1;
    }
#endif

    // initialize the SDL video subsystem if it is not already active
    if(!SDL_WasInit(SDL_INIT_VIDEO)) {
        error = SDL_InitSubSystem(SDL_INIT_VIDEO);
        if(error) {
            FCEUD_PrintError(SDL_GetError());
            return -1;
        }
    }
    inited = 1;

    // shows the cursor within the display window
    SDL_ShowCursor(0);

    // determine if we can allocate the display on the video card
    vinf=SDL_GetVideoInfo();
    if(vinf->hw_available) {
        flags |= SDL_HWSURFACE;
    }

    // check if we are rendering fullscreen
    if(_fullscreen) {
        flags |= SDL_FULLSCREEN;
    }

    // gives the SDL exclusive palette control... ensures the requested colors
    flags |= SDL_HWPALETTE;


    // enable double buffering if requested and we have hardware support
#ifdef OPENGL
    if(usingogl) {
        FCEU_printf("Initializing with OpenGL (Disable with '-opengl 0').\n");
        if(_doublebuf) {
            SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
        }
    } else
#endif
        if(_doublebuf && (flags & SDL_HWSURFACE)) {
            flags |= SDL_DOUBLEBUF;
        }

    if(_fullscreen) {
        int desbpp = _bpp;

        exs  = _xscalefs;
        eys  = _yscalefs;
        eefx = _efxfs;

#ifdef OPENGL
        if(!usingogl) {exs=(int)exs;eys=(int)eys;}
        else desbpp=0;

        if(sponge) {
            exs = eys = 2;
            if(sponge == 3 || sponge == 4) {
                exs = eys = 3;
            }
            eefx=0;
            if(sponge == 1 || sponge == 3) {
                desbpp = 32;
            }
        }


        if( (usingogl && !_stretchx) || !usingogl)
#endif
            if(_xres < (NWIDTH * exs) || exs <= 0.01) {
                FCEUD_PrintError("xscale out of bounds.");
                KillVideo();
                return -1;
            }

#ifdef OPENGL
        if( (usingogl && !_stretchy) || !usingogl)
#endif
            if(_yres<tlines*eys || eys <= 0.01) {
                FCEUD_PrintError("yscale out of bounds.");
                KillVideo();
                return -1;
            }


        screen = SDL_SetVideoMode(_xres, _yres, desbpp, flags);
        if(!screen) {
            FCEUD_PrintError(SDL_GetError());
            return -1;
        }
    } else {
        int desbpp=0;

        exs=_xscale;
        eys=_yscale;
        eefx=_efx;

        if(sponge) {
            exs = eys = 2;
            if(sponge >= 3) {
                exs = eys = 3;
            }
            eefx = 0;
            // SDL's 32bpp->16bpp code is slighty faster than mine, at least :/
            if(sponge == 1 || sponge == 3) {
                desbpp=32;
            }
        }

#ifdef OPENGL
        if(!usingogl) {exs=(int)exs;eys=(int)eys;}
        if(exs <= 0.01) 
            {
                FCEUD_PrintError("xscale out of bounds.");
                KillVideo();
                return -1;
            }
        if(eys <= 0.01)
            {
                FCEUD_PrintError("yscale out of bounds.");
                KillVideo();
                return -1;
            }
#endif

        screen = SDL_SetVideoMode((int)(NWIDTH*exs), (int)(tlines*eys),
                                  desbpp, flags);
        if(!screen) {
            FCEUD_PrintError(SDL_GetError());
            return -1;
        }
    }
    curbpp=screen->format->BitsPerPixel;
    if(!screen) {
        FCEUD_PrintError(SDL_GetError());
        KillVideo();
        return -1;
    }

    // XXX soules - this would be creating a surface on the video
    //              card, but was commented out for some reason...
    //BlitBuf=SDL_CreateRGBSurface(SDL_HWSURFACE,256,240,screen->format->BitsPerPixel,screen->format->Rmask,screen->format->Gmask,screen->format->Bmask,0);

    FCEU_printf(" Video Mode: %d x %d x %d bpp %s\n",
                screen->w, screen->h, screen->format->BitsPerPixel,
                _fullscreen ? "full screen" : "");

    if(curbpp != 8 && curbpp != 16 && curbpp != 24 && curbpp != 32) {
        FCEU_printf("  Sorry, %dbpp modes are not supported by FCE Ultra.  Supported bit depths are 8bpp, 16bpp, and 32bpp.\n", curbpp);
        KillVideo();
        return -1;
    }

    // if the game being run has a name, set it as the window name
    if(gi->name) {
        SDL_WM_SetCaption((const char *)gi->name, (const char *)gi->name);
    } else {
        SDL_WM_SetCaption("FCE Ultra","FCE Ultra");
    }

    // create the surface for displaying graphical messages
#ifdef LSB_FIRST
    IconSurface = SDL_CreateRGBSurfaceFrom((void *)fceu_playicon.pixel_data,
                                           32, 32, 24, 32*3,
                                           0xFF, 0xFF00, 0xFF0000, 0x00);
#else
    IconSurface = SDL_CreateRGBSurfaceFrom((void *)fceu_playicon.pixel_data,
                                           32, 32, 24, 32*3,
                                           0xFF0000, 0xFF00, 0xFF, 0x00);
#endif
    SDL_WM_SetIcon(IconSurface,0);
    paletterefresh = 1;

    // XXX soules - can't SDL do this for us?
    // if using more than 8bpp, initialize the conversion routines
    if(curbpp > 8)
#ifdef OPENGL
        if(!usingogl)
#endif
            InitBlitToHigh(curbpp>>3,screen->format->Rmask,screen->format->Gmask,screen->format->Bmask,eefx,sponge);
#ifdef OPENGL
    if(usingogl)
        if(!InitOpenGL((eoptions&EO_CLIPSIDES)?8:0,256-((eoptions&EO_CLIPSIDES)?8:0),srendline,erendline+1,exs,eys,eefx,_openglip,_stretchx,_stretchy,screen)) {
            FCEUD_PrintError("Error initializing OpenGL.");
            KillVideo();
            return -1;
        }
#endif
    return 0;
}

/**
 * Toggles the full-screen display.
 */
void
ToggleFS()
{
    int error;
    extern FCEUGI *CurGame;

    // shut down the current video system
    KillVideo();

    // flip the fullscreen flag
    _fullscreen = !_fullscreen;

    // try to initialize the video
    error = InitVideo(CurGame);
    if(error) {
        // if we fail, just continue with what worked before
        _fullscreen = !_fullscreen;
        InitVideo(CurGame);
    }
}

static SDL_Color psdl[256];

/**
 * Sets the color for a particular index in the palette.
 */
void
FCEUD_SetPalette(uint8 index,
                 uint8 r,
                 uint8 g,
                 uint8 b)
{
    psdl[index].r = r;
    psdl[index].g = g;
    psdl[index].b = b;

    paletterefresh = 1;
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
    *r = psdl[index].r;
    *g = psdl[index].g;
    *b = psdl[index].b;
}

/** 
 * Pushes the palette structure into the underlying video subsystem.
 */
static void
RedoPalette()
{
#ifdef OPENGL
    if(usingogl)
        SetOpenGLPalette((uint8*)psdl);
    else
#endif
        {
            if(curbpp > 8) {
                SetPaletteBlitToHigh((uint8*)psdl); 
            } else {
                SDL_SetPalette(screen, SDL_PHYSPAL, psdl, 0, 256);
            }
        }
}

// XXX soules - console lock/unlock unimplemented?

///Currently unimplemented.
void LockConsole(){}

///Currently unimplemented.
void UnlockConsole(){}

/**
 * Pushes the given buffer of bits to the screen.
 */
void
BlitScreen(uint8 *XBuf)
{
    SDL_Surface *TmpScreen;
    uint8 *dest;
    int xo = 0, yo = 0;

    if(!screen) {
        return;
    }

    // refresh the palette if required
    if(paletterefresh) {
        RedoPalette();
        paletterefresh=0;
    }

#ifdef OPENGL
    // OpenGL is handled separately
    if(usingogl) {
        BlitOpenGL(XBuf);
        return;
    }
#endif

    // XXX soules - not entirely sure why this is being done yet
    XBuf += srendline * 256;

    if(BlitBuf) {
        TmpScreen = BlitBuf;
    } else {
        TmpScreen = screen;
    }

    // lock the display, if necessary
    if(SDL_MUSTLOCK(TmpScreen)) {
        if(SDL_LockSurface(TmpScreen) < 0) {
            return;
        }
    }

    dest = (uint8*)TmpScreen->pixels;

    if(_fullscreen) {
        xo = (int)(((TmpScreen->w - NWIDTH * exs)) / 2);
        dest += xo * (curbpp >> 3);
        if(TmpScreen->h > (tlines * eys)) {
            yo = (int)((TmpScreen->h - tlines * eys) / 2);
            dest += yo * TmpScreen->pitch;
        }
    }

    // XXX soules - again, I'm surprised SDL can't handle this
    // perform the blit, converting bpp if necessary
    if(curbpp > 8) {
        if(BlitBuf) {
            Blit8ToHigh(XBuf + NOFFSET, dest, NWIDTH, tlines,
                        TmpScreen->pitch, 1, 1);
        } else {
            Blit8ToHigh(XBuf + NOFFSET,dest, NWIDTH, tlines,
                        TmpScreen->pitch, (int)exs, (int)eys);
        }
    } else {
        if(BlitBuf) {
            Blit8To8(XBuf + NOFFSET, dest, NWIDTH, tlines,
                     TmpScreen->pitch, 1, 1, 0, sponge);
        } else {
            Blit8To8(XBuf + NOFFSET, dest, NWIDTH, tlines,
                     TmpScreen->pitch, (int)exs, (int)eys, eefx, sponge);
        }
    }

    // unlock the display, if necessary
    if(SDL_MUSTLOCK(TmpScreen)) {
        SDL_UnlockSurface(TmpScreen);
    }

    // if we have a hardware video buffer, do a fast video->video copy
    if(BlitBuf) {
        SDL_Rect srect;
        SDL_Rect drect;

        srect.x = 0;
        srect.y = 0;
        srect.w = NWIDTH;
        srect.h = tlines;

        drect.x = 0;
        drect.y = 0;
        drect.w = (Uint16)(exs * NWIDTH);
        drect.h = (Uint16)(eys * tlines);

        SDL_BlitSurface(BlitBuf, &srect, screen, &drect);
    }

    // ensure that the display is updated
    SDL_UpdateRect(screen, xo, yo, (Uint32)(NWIDTH*exs), (Uint32)(tlines*eys));

    // have to flip the displayed buffer in the case of double buffering
    if(screen->flags & SDL_DOUBLEBUF) {
        SDL_Flip(screen);
    }
}

/**
 *  Converts an x-y coordinate in the window manager into an x-y
 *  coordinate on FCEU's screen.
 */
uint32
PtoV(uint16 x,
     uint16 y)
{
    y = (uint16)((double)y / eys);
    x = (uint16)((double)x / exs);
    if(eoptions & EO_CLIPSIDES) {
        x += 8;
    }
    y += srendline;
    return (x | (y << 16));
}
