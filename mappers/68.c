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

static void Fixerit(void)
{
 switch(mapbyte2[0]&3)
 {
  case 0:vnapage[0]=vnapage[2]=CHRptr[0]+(((mapbyte1[0]|128)&CHRmask1[0])<<10);
         vnapage[1]=vnapage[3]=CHRptr[0]+(((mapbyte1[1]|128)&CHRmask1[0])<<10);
         break;
  case 1:vnapage[0]=vnapage[1]=CHRptr[0]+(((mapbyte1[0]|128)&CHRmask1[0])<<10);
         vnapage[2]=vnapage[3]=CHRptr[0]+(((mapbyte1[1]|128)&CHRmask1[0])<<10);
         break;
  case 2:vnapage[0]=vnapage[1]=vnapage[2]=vnapage[3]=CHRptr[0]+(((mapbyte1[0]|128)&CHRmask1[0])<<10);
         break;
  case 3:vnapage[0]=vnapage[1]=vnapage[2]=vnapage[3]=CHRptr[0]+(((mapbyte1[1]|128)&CHRmask1[0])<<10);
         break;
 }
}

DECLFW(Mapper68_write)
{
// FCEU_printf("%04x,%04x\n",A,V);
 A&=0xF000;

 if(A>=0x8000 && A<=0xB000)
 {
  VROM_BANK2((A-0x8000)>>1,V);
 }
 else switch(A)
 {
  case 0xc000:mapbyte1[0]=V;
              if(VROM_size && mapbyte2[0]&0x10)
               Fixerit();
              break;

  case 0xd000:mapbyte1[1]=V;
              if(VROM_size && mapbyte2[0]&0x10)
               Fixerit();
              break;

  case 0xe000: mapbyte2[0]=V;
               if(!(V&0x10))
               {
                switch(V&3)
                {
                 case 0:MIRROR_SET2(1);break;
                 case 1:MIRROR_SET2(0);break;
                 case 2:onemir(0);break;
                 case 3:onemir(1);break;
                }
               }
               else if(VROM_size)
               {
                Fixerit();
                PPUNTARAM=0;
               }
               break;
  case 0xf000: ROM_BANK16(0x8000,V);break;
 }
}

static void Mapper68_StateRestore(int version)
{
              if(!(mapbyte2[0]&0x10))
              {
               switch(mapbyte2[0]&3)
               {
                case 0:MIRROR_SET(0);break;
                case 1:MIRROR_SET(1);break;
                case 2:onemir(0);break;
                case 3:onemir(1);break;
               }
              }
              else if(VROM_size)
              {
                Fixerit();
                PPUNTARAM=0;
              }
}

void Mapper68_init(void)
{
 SetWriteHandler(0x8000,0xffff,Mapper68_write);
 MapStateRestore=Mapper68_StateRestore;
}
