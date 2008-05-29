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

#include "main.h"
#include "args.h"
#include "common.h"
#include "../common/args.h"

char* MovieToLoad = 0;
char* StateToLoad = 0;

// TODO: Parsing arguments needs to be improved a lot. A LOT.

/**
* Parses commandline arguments
**/
char *ParseArgies(int argc, char *argv[])
{         
        //int x;  //mbg merge 7/17/06 removed
        static ARGPSTRUCT FCEUArgs[]={
         {"-pal",0,&pal_emulation,0},
         {"-noicon",0,&status_icon,0},
         {"-gg",0,&genie,0},
         {"-no8lim",0,&eoptions,0x8000|EO_NOSPRLIM},
         //{"-nofs",0,&eoptions,0},   
         {"-clipsides",0,&eoptions,0x8000|EO_CLIPSIDES},  
         {"-nothrottle",0,&eoptions,0x8000|EO_NOTHROTTLE},
         {"-playmovie",0,&MovieToLoad,0x4001},
         {"-loadstate",0,&StateToLoad,0x4001},
         {"-readonly",0,&replayReadOnlySetting,0},
         {"-stopframe",0,&replayStopFrameSetting,0},
         {"-framedisplay",0,&frame_display,0},
         {"-inputdisplay",0,&input_display,0},
         {"-allowUDLR",0,&allowUDLR,0},
         {"-stopmovie",0,&pauseAfterPlayback,0},
         {"-bginput",0,&EnableBackgroundInput,0},
         {0, 0, 0, 0},
	};

       if(argc <= 1)
	   {
		   return(0);
	   }

       ParseArguments(argc-2, &argv[1], FCEUArgs);

       return(argv[argc-1]);
}
