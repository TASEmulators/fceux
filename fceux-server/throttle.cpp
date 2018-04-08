/* FCE Ultra Network Play Server
 *
 * Copyright notice for this file:
 *  Copyright (C) 2004 Xodnizel
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */



#include <sys/time.h>
#include <unistd.h>
#include "types.h"
#include "throttle.h"

#ifdef TODO
THROTTLE *MakeThrottle(uint32 fps)
{


}

/* Returns 1 if it's time to run, 0 if it's not time yet. */
int TestThrottle(THROTTLE *throt)
{

}

/* Pass it a pointer to an array of THROTTLE structs */
/* It will check  */
void DoAllThrottles(THROTTLE *throt)
{


}

void KillThrottle(THROTTLE *throt)
{

}

#endif

static uint64 tfreq;
static uint64 desiredfps;

int32 FCEUI_GetDesiredFPS(void)
{
  //if(PAL)
  // return(838977920); // ~50.007
  //else
   return(1008307711);  // ~60.1
}

void RefreshThrottleFPS(int divooder)
{
 desiredfps=(FCEUI_GetDesiredFPS() / divooder)>>8;
 tfreq=1000000;
 tfreq<<=16;    /* Adjustment for fps returned from FCEUI_GetDesiredFPS(). */
}

static uint64 GetCurTime(void)
{
 uint64 ret;
 struct timeval tv;

 gettimeofday(&tv,0);
 ret=(uint64)tv.tv_sec*1000000;
 ret+=tv.tv_usec;
 return(ret);
}

void SpeedThrottle(void)
{
 static uint64 ttime,ltime;

 waiter:
 ttime=GetCurTime();

 if( (ttime-ltime) < (tfreq/desiredfps) )
 {
  usleep(1000);
  goto waiter;
 }
 if( (ttime-ltime) >= (tfreq*4/desiredfps))
  ltime=ttime;
 else
  ltime+=tfreq/desiredfps;
}

