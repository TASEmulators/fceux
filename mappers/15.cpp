/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2003 Xodnizel
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

static void Sync(void)
{
        int x;

        setmirror(((mapbyte1[0]>>6)&1)^1);
        switch(mapbyte1[1]&0x3)
        {
         case 0x0:
          for(x=0;x<4;x++)
           setprg8(0x8000+x*8192,(((mapbyte1[0]&0x7F)<<1)+x)^(mapbyte1[0]>>7));
          break;
         case 0x2:
          for(x=0;x<4;x++)
           setprg8(0x8000+x*8192,((mapbyte1[0]&0x7F)<<1)+(mapbyte1[0]>>7));
          break;
         case 0x1:
         case 0x3:
          for(x=0;x<4;x++)
          {
           unsigned int b;

           b=mapbyte1[0]&0x7F;
           if(x>=2 && !(mapbyte1[1]&0x2))
            b=0x7F;
           setprg8(0x8000+x*8192,(x&1)+((b<<1)^(mapbyte1[0]>>7)));
          }
          break;
        }
}


static DECLFW(Mapper15_write)
{
 mapbyte1[0]=V;
 mapbyte1[1]=A&3;
 Sync();
}

static void StateRestore(int version)
{
 Sync();
}

void Mapper15_init(void)
{
        mapbyte1[0]=mapbyte1[1]=0;
        Sync();
        GameStateRestore=StateRestore;
        SetWriteHandler(0x8000,0xFFFF,Mapper15_write);
}

