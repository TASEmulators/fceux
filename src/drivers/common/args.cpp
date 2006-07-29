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

/****************************************************************/
/*			FCE Ultra				*/
/*								*/
/*	This file contains code for parsing command-line    	*/
/*	options.						*/
/*								*/
/****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../types.h"
#include "args.h"

void ParseEA(int x, int argc, char *argv[], ARGPSTRUCT *argsps)
{
  int y=0;

  do
  {
   if(!argsps[y].name)
   {
    ParseEA(x,argc,argv,(ARGPSTRUCT*)argsps[y].var);
    y++;
    continue;
   }
   if(!strcmp(argv[x],argsps[y].name))	// A match.
   {
    if(argsps[y].subs)
    {
     if((x+1)>=argc)
      break;
     if(argsps[y].substype&0x2000)
     {
      ((void (*)(char *))argsps[y].subs)(argv[x+1]);
     }
     else if(argsps[y].substype&0x8000)
     {
      *(int *)argsps[y].subs&=~(argsps[y].substype&(~0x8000));
      *(int *)argsps[y].subs|=atoi(argv[x+1])?(argsps[y].substype&(~0x8000)):0;
     }
     else
      switch(argsps[y].substype&(~0x4000))
      {
       case 0:		// Integer
 	      *(int *)argsps[y].subs=atoi(argv[x+1]);
	      break;
       case 2:		// Double float
	      *(double *)argsps[y].subs=atof(argv[x+1]);
	      break;
       case 1:		// String
	      if(argsps[y].substype&0x4000)
	      {
               if(*(char **)argsps[y].subs)
		free(*(char **)argsps[y].subs);
	       if(!( *(char **)argsps[y].subs=(char*)malloc(strlen(argv[x+1])+1) ))
		break;
	      }	
	      strcpy(*(char **)argsps[y].subs,argv[x+1]);
	      break;
      }
    }
    if(argsps[y].var)
     *argsps[y].var=1;
   }
   y++;
  } while(argsps[y].var || argsps[y].subs);
}

void ParseArguments(int argc, char *argv[], ARGPSTRUCT *argsps)
{
 int x;

 for(x=0;x<argc;x++)
  ParseEA(x,argc,argv,argsps);
}

