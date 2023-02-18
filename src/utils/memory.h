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

/*        Various macros for faster memory stuff
		(at least that's the idea) 
*/

#define FCEU_dwmemset(d,c,n) {int _x; for(_x=n-4;_x>=0;_x-=4) *(uint32 *)&(d)[_x]=c;}

//returns a buffer initialized to 0
void *FCEU_malloc(size_t size);

//returns a buffer, with jumbled initial contents
//used by boards for WRAM etc, initialized to 0 (default) or other via RAMInitOption
void *FCEU_gmalloc(size_t size);

//free memory allocated with FCEU_gmalloc
void FCEU_gfree(void *ptr);

//returns an aligned buffer, initialized to 0
//the alignment will default to the largest thing you could ever sensibly want for massively aligned cache friendly buffers
void *FCEU_amalloc(size_t size, size_t alignment = 256);

//frees memory allocated with FCEU_amalloc
void FCEU_afree(void* ptr);

//free memory allocated with FCEU_malloc
void FCEU_free(void *ptr);

//reallocate memory allocated with FCEU_malloc
void* FCEU_realloc(void* ptr, size_t size);

//don't use these. change them if you find them.
void *FCEU_dmalloc(size_t size);

//don't use these. change them if you find them.
void FCEU_dfree(void *ptr);

//aborts the process for fatal errors
void FCEU_abort(const char* message = nullptr);
