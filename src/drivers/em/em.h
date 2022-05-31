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

namespace emscripten
{
    class val;
};

// Audio options.
// NOTE: Both AUDIO_BUF_MAX and AUDIO_HW_BUF_MAX must be power of two!
#define AUDIO_BUF_MAX		8192
#define AUDIO_HW_BUF_MAX	2048
#define AUDIO_BUF_MASK		(AUDIO_BUF_MAX - 1)

#define INPUT_W     256 // Width of input generated PPU image (in px).
#define INPUT_H     240 // Height of input generated PPU image (in px).

// For some reason required in a driver header...
extern int pal_emulation;

// The rate of output and emulated (internal) audio (frequency, in Hz).
extern int em_audio_rate;
// Number of scanlines to show in current video mode: NTSC -> 224, PAL -> 240.
extern int em_scanlines;

extern std::string em_region;

bool Audio_Init();
bool Audio_TryInitWebAudioContext();
void Audio_Write(int32 *buffer, int count);
void Audio_SetMuted(bool muted);
bool Audio_Muted();

int  Video_Init(const char* canvasQuerySelector);
void Video_ResizeCanvas();
void Video_Render();
void Video_CanvasToNESCoords(uint32 *x, uint32 *y);
bool Video_SetConfig(const std::string& key, const emscripten::val& value);
void Video_SetSystem(const std::string& region);

#endif // _EM_H_
