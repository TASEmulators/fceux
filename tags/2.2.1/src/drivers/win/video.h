#ifndef WIN_VIDEO_H
#define WIN_VIDEO_H

#include "common.h"

// I like hacks.
#define uint8 __UNO492032
#include <winsock.h>
#include "ddraw.h"
#undef LPCWAVEFORMATEX
#include "dsound.h"
#include "dinput.h"
#include <commctrl.h>
#include <shlobj.h>     // For directories configuration dialog.
#undef uint8

#include "main.h"
#include "window.h"

#define VF_DDSTRETCHED     1

#define VEF_LOSTSURFACE 1
#define VEF____INTERNAL 2

#define VMDF_DXBLT 1
#define VMDF_STRFS 2

typedef struct {
        int x;
        int y;
        int bpp;
        int flags;
        int xscale;
        int yscale;
        RECT srect;
        RECT drect;        
        int special;
} vmdef;

// left, top, right, bottom
extern vmdef vmodes[11];
extern int winspecial;
extern int NTSCwinspecial;

extern int disvaccel;
extern int fssync;
extern int winsync;

void SetFSVideoMode();
void FCEUD_BlitScreen(uint8 *XBuf);
void ConfigVideo();
void recalculateBestFitRect(int width, int height);
int SetVideoMode(int fs);
void DoVideoConfigFix();
void FCEUD_BlitScreen(uint8 *XBuf);
void ResetVideo();
void SetFSVideoMode();
void PushCurrentVideoSettings();
void ResetCustomMode();
#endif
