/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
 *  Copyright (C) 2015 Valtteri "tsone" Heikkila
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
#ifndef _EM_H_
#define _EM_H_
#include "../../driver.h"
#include "../common/args.h"
#include "../common/config.h"
#include "../common/configSys.h"

namespace emscripten
{
    class val;
};

#define FCEM_DEBUG 1 // Set to 0 to disable debug stuff.

// Audio options.
// NOTE: Both AUDIO_BUF_MAX and AUDIO_HW_BUF_MAX must be power of two!
#define AUDIO_BUF_MAX		8192
#define AUDIO_HW_BUF_MAX	2048
#define AUDIO_BUF_MASK		(AUDIO_BUF_MAX - 1)

// SDL and Windows drivers use following values for FPS, however PAL FPS is
// documented to be 50.0070 in http://wiki.nesdev.com/w/index.php/Clock_rate
#define NTSC_FPS 60.0988
#define PAL_FPS  50.0069

#define INPUT_W     256 // Width of input PPU image by fceux (in px).
#define INPUT_H     240 // Height of input PPU image by fceux (in px).

// The rate of output and emulated (internal) audio (frequency, in Hz).
extern int em_audio_rate;
// Number of audio samples per frame.
extern double em_audio_frame_samples;
// Number of scanlines to show in current video mode: NTSC -> 224, PAL -> 240.
extern int em_scanlines;

extern std::string em_region;

extern int pal_emulation;

bool Audio_Init();
bool Audio_IsInitialized();
void Audio_Write(int32 *buffer, int count);
int  Audio_GetBufferCount();
void Audio_SetMuted(bool muted);
bool Audio_Muted();

int  Video_Init(const char* canvasQuerySelector);
void Video_Render();
void Video_UpdateController(int idx, double v);
void Video_CanvasToNESCoords(uint32 *x, uint32 *y);
bool Video_SetConfig(const std::string& key, const emscripten::val& value);
void Video_SetSystem(const std::string& region);

bool FCEUD_ShouldDrawInputAids();
bool FCEUI_AviDisableMovieMessages();
bool FCEUI_AviEnableHUDrecording();
void FCEUI_SetAviEnableHUDrecording(bool enable);
bool FCEUI_AviDisableMovieMessages();
void FCEUI_SetAviDisableMovieMessages(bool disable);

#endif // _EM_H_
