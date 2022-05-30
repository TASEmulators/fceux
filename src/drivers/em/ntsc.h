/* FCE Ultra - NES/Famicom Emulator - NTSC Emulation
 *
 * Copyright notice for this file:
 *  Copyright (C) 2016 Valtteri "tsone" Heikkila
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
#ifndef _NTSC_H_
#define _NTSC_H_


// 64 palette colors, 8 color de-emphasis settings.
#define NUM_COLORS  (64 * 8)
#define NUM_PHASES  3
#define NUM_SUBPS   4
#define NUM_TAPS    5
// Lookup width must be POT >= NUM_PHASES*NUM_TAPS*NUM_SUBPS, ex. 3*5*4=60 -> 64
#define LOOKUP_W    64


struct NtscLookup
{
    float yiq_mins[3];
    float yiq_maxs[3];
    const unsigned char* texture;
};

const NtscLookup& ntscGetLookup();


#endif /* _NTSC_H_ */

