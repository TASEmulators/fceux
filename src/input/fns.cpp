/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2019 CaH4e3
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
 *
 * Family Network System controller
 *
 */

#include <string.h>
#include "share.h"

static int readbit;
static int32 readdata;

static uint8 Read(int w, uint8 ret)
{
	if (!w)
	{
		if (readbit < 24) {
			ret |= ((readdata >> readbit) & 1) << 1;
			readbit++;
		} else
			ret |= 2;	// sense!
	}
	return ret;
}

static void Strobe(void)
{
	readbit = 0;
}

static void Update(void *data, int arg)
{
	readdata = *(uint32*)data;
}

static INPUTCFC FamiNetSys = { Read, 0, Strobe, Update, 0, 0 };

INPUTCFC *FCEU_InitFamiNetSys(void)
{
	return(&FamiNetSys);
}

