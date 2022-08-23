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

//returns a 32-aligned buffer, initialized to 0
void *FCEU_malloc(uint32 size); 

//returns a 32-aligned buffer, with jumbled initial contents
//used by boards for WRAM etc, initialized to 0 (default) or other via RAMInitOption
void *FCEU_gmalloc(uint32 size); 

//free memory allocated with FCEU_gmalloc
void FCEU_gfree(void *ptr);

//free memory allocated with 
void FCEU_free(void *ptr);

//don't use these. change them if you find them.
void *FCEU_dmalloc(uint32 size);

//don't use these. change them if you find them.
void FCEU_dfree(void *ptr);
