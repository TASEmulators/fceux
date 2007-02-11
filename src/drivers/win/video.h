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
static vmdef vmodes[11]={
                         {320,240,8,0,1,1,0}, //0
                         {320,240,8,0,1,1,0}, //1
                         {512,384,8,0,1,1,0}, //2
                         {640,480,8,0,1,1,0}, //3
                         {640,480,8,0,1,1,0}, //4
                         {640,480,8,0,1,1,0}, //5
                         {640,480,8,VMDF_DXBLT,2,2,0}, //6
                         {1024,768,8,VMDF_DXBLT,4,3,0}, //7
                         {1280,1024,8,VMDF_DXBLT,5,4,0}, //8
                         {1600,1200,8,VMDF_DXBLT,6,5,0}, //9
                         {800,600,8,VMDF_DXBLT|VMDF_STRFS,0,0}    //10
                       };

static int winspecial = 0;

extern int disvaccel;
extern int fssync;
extern int winsync;

void SetFSVideoMode();
void FCEUD_BlitScreen(uint8 *XBuf);
void ConfigVideo();
int SetVideoMode(int fs);
void DoVideoConfigFix();
void FCEUD_BlitScreen(uint8 *XBuf);
void ResetVideo();

#endif