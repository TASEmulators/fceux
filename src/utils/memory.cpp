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
/// \brief memory management services provided by FCEU core

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../types.h"
#include "../fceu.h"
#include "memory.h"

static void *_FCEU_malloc(uint32 size)
{
	//do not add an aligned allocation function. if a larger alignment is needed, change this constant to use it for all allocations.
	static const int alignment = 32;
	
	size = (size + alignment - 1) & ~(alignment-1);

	#ifdef _MSC_VER
	void *ret = _aligned_malloc(size,alignment);
	#else
	void *ret = aligned_alloc(alignment,size);
	#endif

	if(!ret)
	{
		FCEU_PrintError("Error allocating memory!  Doing a hard exit.");
		exit(1);
	}

	return ret;
}

static void _FCEU_free(void* ptr)
{
	#ifdef _MSC_VER
	_aligned_free(ptr);
	#else
	free(ptr);
	#endif
}

///allocates the specified number of bytes. exits process if this fails
void *FCEU_gmalloc(uint32 size)
{
 void *ret = _FCEU_malloc(size);
 
 // initialize according to RAMInitOption, default zero
 FCEU_MemoryRand((uint8*)ret,size,true);

 return ret;
}

void *FCEU_malloc(uint32 size)
{
	void *ret = _FCEU_malloc(size);
	memset(ret, 0, size);
	return ret;
}

//frees memory allocated with FCEU_gmalloc
void FCEU_gfree(void *ptr)
{
	_FCEU_free(ptr);
}

//frees memory allocated with FCEU_malloc
void FCEU_free(void *ptr)
{
	_FCEU_free(ptr);
}

void *FCEU_dmalloc(uint32 size)
{
	return FCEU_malloc(size);
}

void FCEU_dfree(void *ptr)
{
	return FCEU_free(ptr);
}
