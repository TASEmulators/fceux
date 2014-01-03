#ifndef WIN_VIDEO_H
#define WIN_VIDEO_H

#include <Windows.h>
#include "types.h"

typedef enum
{
	DIRECTDRAW_MODE_SOFTWARE = 0, // all features are emulated in software
	DIRECTDRAW_MODE_SURFACE_IN_RAM, // place offscreen surface in RAM rather than in VRAM; may or may not use hardware acceleration
	DIRECTDRAW_MODE_FULL, // use available hardware features, emulate the rest
	// ...
	DIRECTDRAW_MODES_TOTAL
} DIRECTDRAW_MODE;

#define VIDEOMODEFLAG_DXBLT 1
#define VIDEOMODEFLAG_STRFS 2

typedef enum
{
	FILTER_NONE = 0,
	FILTER_HQ2X,
	FILTER_SCALE2X,
	FILTER_NTSC2X,
	FILTER_HQ3X,
	FILTER_SCALE3X
} VFILTER;

typedef struct {
        int width;
        int height;
        int bpp;
        int flags;
        int xscale;
        int yscale;
        RECT srcRect;
        RECT dstRect;        
        VFILTER filter;
} VideoMode;

typedef enum {
	SYNCMODE_NONE = 0,
	SYNCMODE_WAIT,
	SYNCMODE_LAZYWAIT,
	SYNCMODE_DOUBLEBUF
} VSYNCMODE;

void InitVideoDriver(void);
void ShutdownVideoDriver();

// Recalculate blit rectangle within window of given size
void OnWindowSizeChange(int width, int height);

// Get current blit rectangle
RECT GetActiveRect(void);

// Get driver palette
PALETTEENTRY* GetPalette(void);

// Returns true when in fullscreen mode, false when in windowed
bool GetIsFullscreen(void);

// Set fullscreen mode flag
// FCEUD_VideoChanged() must be called in order to make value take effect
void SetIsFullscreen(bool f);

VSYNCMODE GetWindowedSyncModeIdx(void);
void SetWindowedSyncModeIdx(VSYNCMODE idx);


// Bring up the Video configuration dialog
void ShowConfigVideoDialog();

// (Re)apply render lines and sprite limitation to FCE
void DoVideoConfigFix();


// Implements FCEUD requirements
bool FCEUD_ShouldDrawInputAids(void);
void FCEUD_BlitScreen(uint8 *XBuf);
void FCEUD_VideoChanged(void); // this one should be declared here
void FCEUD_SetPalette(unsigned char index, unsigned char r, unsigned char g, unsigned char b);
void FCEUD_GetPalette(unsigned char i, unsigned char *r, unsigned char *g, unsigned char *b);


// see win/config.h
bool& _FIXME_getFullscreenVar(void);
int& _FIXME_getVModeIdxVar(void);
VSYNCMODE& _FIXME_getFullscreenSyncModeIdxVar(void);
VSYNCMODE& _FIXME_getWindowedSyncModeIdxVar(void);
VFILTER& _FIXME_getFilterModeWindowedIdxVar(void);
int& _FIXME_getFilterOptionVar(void);
DIRECTDRAW_MODE& _FIXME_getDDrawModeWindowedVar(void);
DIRECTDRAW_MODE& _FIXME_getDDrawModeFullscreenVar(void);
VideoMode& _FIXME_getCustomVideoModeVar(void);
#endif
