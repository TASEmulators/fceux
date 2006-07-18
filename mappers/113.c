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

#include "mapinc.h"

/*      I'm getting the feeling this is another "jam two different bank
        switching hardwares into one mapper".
*/

/* HES 4-in-1 */
DECLFW(Mapper113_write)
{
        ROM_BANK32((V>>3)&7);
        VROM_BANK8(((V>>3)&8)|(V&7));
        //printf("$%04x:$%02x\n",A,V);
}


/*      Deathbots */
DECLFW(Mapper113_writeh)
{
        //printf("$%04x:$%02x\n",A,V);
 //       ROM_BANK32(V&0x7);
        //VROM_BANK8((V>>4)&0x7);
        switch(A) {
                case        0x8008:
                case        0x8009:
                        ROM_BANK32(V>>3);
                        VROM_BANK8(((V>>3)&0x08)+(V&0x07) );
                        break;
                case        0x8E66:
                case        0x8E67:
                        VROM_BANK8( (V&0x07)?0:1 );
                        break;
                case        0xE00A:
                        MIRROR_SET2( 2 );
                        break;
        }
}


void Mapper113_init(void)
{
 ROM_BANK32(0);
 SetWriteHandler(0x4020,0x7fff,Mapper113_write);
 SetWriteHandler(0x8000,0xffff,Mapper113_writeh);
}
