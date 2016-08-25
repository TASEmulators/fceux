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

// implementation of Subor Mouse
// used in Educational Computer 2000

#include <string.h>
#include <stdlib.h>
#include "share.h"

typedef struct {
	uint8 latch;
	int32 dx,dy;
	uint32 mb;
} MOUSE;

static MOUSE Mouse;

// since this game only picks up 1 mickey per frame,
// allow a single delta to spread out over a few frames
// to make it easier to move.
const int INERTIA = 32;

static void StrobeMOUSE(int w)
{
	Mouse.latch = Mouse.mb & 0x03;

	int32 dx = Mouse.dx;
	int32 dy = Mouse.dy;

	if      (dx > 0) { Mouse.latch |= (0x2 << 2); --Mouse.dx; }
	else if (dx < 0) { Mouse.latch |= (0x3 << 2); ++Mouse.dx; }
	if      (dy > 0) { Mouse.latch |= (0x2 << 4); --Mouse.dy; }
	else if (dy < 0) { Mouse.latch |= (0x3 << 4); ++Mouse.dy; }

	//FCEU_printf("Subor Mouse: %02X\n",Mouse.latch);
}

static uint8 ReadMOUSE(int w)
{
	uint8 result = Mouse.latch & 0x01;
	Mouse.latch = (Mouse.latch >> 1) | 0x80;
	return result;
}

static void UpdateMOUSE(int w, void *data, int arg)
{
	uint32 *ptr=(uint32*)data;
	Mouse.dx += ptr[0]; ptr[0] = 0;
	Mouse.dy += ptr[1]; ptr[1] = 0;
	Mouse.mb = ptr[2];

	if      (Mouse.dx >  INERTIA) Mouse.dx =  INERTIA;
	else if (Mouse.dx < -INERTIA) Mouse.dx = -INERTIA;

	if      (Mouse.dy >  INERTIA) Mouse.dy =  INERTIA;
	else if (Mouse.dy < -INERTIA) Mouse.dy = -INERTIA;
}

static INPUTC MOUSEC={ReadMOUSE,0,StrobeMOUSE,UpdateMOUSE,0,0};

INPUTC *FCEU_InitMouse(int w)
{
	memset(&Mouse,0,sizeof(Mouse));
	return(&MOUSEC);
}
