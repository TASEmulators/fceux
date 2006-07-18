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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>

#include "types.h"
#include "netplay.h"
#include  "fceu.h"
#include "general.h"
#include "state.h"
#include "cheat.h"
#include "input.h"
#include "endian.h"

int FCEUnetplay=0;

static uint8 netjoy[4]; /* Controller cache. */
static int numlocal;
static int netdivisor;
static int netdcount;

/* NetError should only be called after a FCEUD_*Data function returned 0, in the function
   that called FCEUD_*Data, to prevent it from being called twice.
*/

static void NetError(void)
{
 FCEU_DispMessage("Network error/connection lost!");
 FCEUD_NetworkClose();
}

void FCEUI_NetplayStop(void)
{
	if(FCEUnetplay)
	{
	 FCEUnetplay = 0;
    FCEU_FlushGameCheats(0,1);  /* Don't save netplay cheats. */
    FCEU_LoadGameCheats(0);    /* Reload our original cheats. */
	}
	else puts("Check your code!");
}

int FCEUI_NetplayStart(int nlocal, int divisor)
{
  FCEU_FlushGameCheats(0, 0);  /* Save our pre-netplay cheats. */
  FCEU_LoadGameCheats(0);    /* Load them again, for pre-multiplayer
					   action.
					*/
	FCEUnetplay = 1;
        memset(netjoy,0,sizeof(netjoy));
        numlocal = nlocal;
	netdivisor = divisor;
	netdcount = 0;
	return(1);
}

int FCEUNET_SendCommand(uint8 cmd, uint32 len)
{
 uint8 buf[numlocal + 1 + 4];

 buf[0] = 0xFF;
 FCEU_en32lsb(&buf[numlocal], len);
 buf[numlocal + 4] = cmd;
 #ifdef NETWORK
 if(!FCEUD_SendData(buf,numlocal + 1 + 4)) 
 {
  NetError();
  return(0);
 }
 #endif
 return(1);
}

void FCEUI_NetplayText(uint8 *text)
{
 uint32 len;

 len = strlen(text);

 if(!FCEUNET_SendCommand(FCEUNPCMD_TEXT,len)) return;

 if(!FCEUD_SendData(text,len))
  NetError();
}

int FCEUNET_SendFile(uint8 cmd, char *fn)
{
 uint32 len;
 uLongf clen;
 char *buf, *cbuf;
 FILE *fp;
 struct stat sb;

 if(!(fp=FCEUD_UTF8fopen(fn,"rb"))) return(0);

 fstat(fileno(fp),&sb);
 len = sb.st_size;
 buf = malloc(len);
 fread(buf, 1, len, fp);
 fclose(fp);

 cbuf = malloc(4 + len + len / 1000 + 12);
 FCEU_en32lsb(cbuf, len);
 compress2(cbuf + 4, &clen, buf, len, 7);
 free(buf);

 //printf("Sending file: %s, %d, %d\n",fn,len,clen);

 len = clen + 4;

 #ifdef NETWORK
 if(!FCEUNET_SendCommand(cmd,len))
 {
  free(cbuf);
  return(0);
 }
 if(!FCEUD_SendData(cbuf, len))
 {
  NetError();
  free(cbuf);
  return(0);
 }
 #endif
 free(cbuf);

 return(1);
}

static FILE *FetchFile(uint32 remlen)
{
				uint32 clen = remlen;
                               	char *cbuf;
                                uLongf len;
                                char *buf;  
                                FILE *fp;
                                char *fn;   
                                 
        if(clen > 500000)  // Sanity check
				{
				 NetError();
				 return(0);
				}

                                //printf("Receiving file: %d...\n",clen);
                                fn = FCEU_MakeFName(FCEUMKF_NPTEMP,0,0);
                                if((fp = fopen(fn,"w+b")))
                                {
                                 cbuf = malloc(clen);
                                 if(!FCEUD_RecvData(cbuf, clen))
				 {
				  NetError();
				  unlink(fn);
				  fclose(fp);
				  free(cbuf);
				  free(fn);
				  return(0);
				 }
                             
                                 len = FCEU_de32lsb(cbuf);
         if(len > 500000)    // Another sanity check
				 {
				  NetError();
				  unlink(fn);
				  fclose(fp);
				  free(cbuf);
				  free(fn);
				  return(0);
				 }
                                 buf = malloc(len);
                                 uncompress(buf, &len, cbuf + 4, clen - 4);
                                
                                 fwrite(buf, 1, len, fp);
                                 free(buf);
                                 fseek(fp, 0, SEEK_SET);
				 unlink(fn);
				 free(fn);
				 return(fp);
				}
				free(fn);
				return(0);
}

void NetplayUpdate(uint8 *joyp)
{
 static uint8 buf[5];  /* 4 play states, + command/extra byte */
 static uint8 joypb[4];

 memcpy(joypb,joyp,4);

 /* This shouldn't happen, but just in case.  0xFF is used as a command escape elsewhere. */
 if(joypb[0] == 0xFF) 
  joypb[0] = 0xF;
 #ifdef NETWORK
 if(!netdcount)
  if(!FCEUD_SendData(joypb,numlocal))
  {
   NetError();
   return;
  }

 if(!netdcount)
 do
 {
  if(!FCEUD_RecvData(buf,5))
  {
   NetError();
   return;
  }

  switch(buf[4])
  {
   default: FCEU_DoSimpleCommand(buf[4]);break;
   case FCEUNPCMD_TEXT:
		       {
			uint8 *tbuf;
			uint32 len = FCEU_de32lsb(buf);

      if(len > 100000)  // Insanity check!
			{
			 NetError();
			 return;
			}
			tbuf = malloc(len + 1);
			tbuf[len] = 0;
                        if(!FCEUD_RecvData(tbuf, len))
                        {
                         NetError();
                         free(tbuf);
                         return;
                        }
			FCEUD_NetplayText(tbuf);
			free(tbuf);
		       }
		       break;
   case FCEUNPCMD_SAVESTATE:	
			    {
			       char *fn;
			        FILE *fp;

				/* Send the cheats first, then the save state, since
				   there might be a frame or two in between the two sendfile
				   commands on the server side.
				*/
				fn = FCEU_MakeFName(FCEUMKF_CHEAT,0,0);
                                //if(!
                                FCEUNET_SendFile(FCEUNPCMD_LOADCHEATS,fn);

                               // {
                               //  free(fn);
                               //  return;
                               // }
				free(fn);
                                if(!FCEUnetplay) return;
                               
			        fn = FCEU_MakeFName(FCEUMKF_NPTEMP,0,0);
			        fp = fopen(fn, "wb");
			        if(FCEUSS_SaveFP(fp))
			        {
			         fclose(fp);
			         if(!FCEUNET_SendFile(FCEUNPCMD_LOADSTATE, fn))
				 {
				  unlink(fn);
				  free(fn);
				  return;
				 }
			         unlink(fn);
			         free(fn);
			        }
			        else
			        {
			         fclose(fp);
			         FCEUD_PrintError("File error.  (K)ill, (M)aim, (D)estroy?  Now!");
			         unlink(fn);
			         free(fn);
		        	 return;
			        }

			    }
  	  	 	    break;
   case FCEUNPCMD_LOADCHEATS:
				{
				 FILE *fp = FetchFile(FCEU_de32lsb(buf));
				 if(!fp) return;
				 FCEU_FlushGameCheats(0,1);
				 FCEU_LoadGameCheats(fp);
				}
				break;  
 case FCEUNPCMD_LOADSTATE:
				{
				 FILE *fp = FetchFile(FCEU_de32lsb(buf));
				 if(!fp) return;
				 if(FCEUSS_LoadFP(fp,1))
				 {
				  fclose(fp);				
			          FCEU_DispMessage("Remote state loaded.");
				 } else FCEUD_PrintError("File error.  (K)ill, (M)aim, (D)estroy?");
			    }
			    break;
  }
 } while(buf[4]);
 #endif

 netdcount=(netdcount+1)%netdivisor;

 memcpy(netjoy,buf,4);
 *(uint32 *)joyp=*(uint32 *)netjoy;
}
