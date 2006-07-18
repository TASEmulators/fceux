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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>

#include "types.h"
#include "fceu.h"
#include "memory.h"
#include "general.h"

void *FCEU_gmalloc(uint32 size)
{
 void *ret;
 ret=malloc(size);
 if(!ret)  
 {
  FCEU_PrintError("Error allocating memory!  Doing a hard exit.");
  exit(1);
 }
 return ret;
}

void *FCEU_malloc(uint32 size)
{
 void *ret;
 ret=malloc(size);
 if(!ret)
 {
  FCEU_PrintError("Error allocating memory!");
  return(0);
 }
 return ret;
}

void FCEU_free(void *ptr)    // Might do something with this and FCEU_malloc later...
{
 free(ptr);
}

void FCEU_gfree(void *ptr)
{
 free(ptr);
}

void FASTAPASS(3) FCEU_memmove(void *d, void *s, uint32 l)
{
 uint32 x;
 int t;

 /* Type really doesn't matter. */
 t=(int)d;
 t|=(int)s;
 t|=(int)l;

 if(t&3)    // Not 4-byte aligned and/or length is not a multiple of 4.
 {
  uint8 *tmpd, *tmps;  

  tmpd = d;
  tmps = s;

  for(x=l;x;x--)  // This could be optimized further, though(more tests could be performed).
  {
   *tmpd=*tmps;
   tmpd++;
   tmps++;
  }
 }
 else
 {
  uint32 *tmpd, *tmps;

  tmpd = d;
  tmps = s;

  for(x=l>>2;x;x--)
  {
   *tmpd=*tmps;
   tmpd++;
   tmps++;
  }
 }
}
