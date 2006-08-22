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

//http://www.geisswerks.com/ryan/FAQS/timing.html

static uint64 tmethod,tfreq;
static uint64 desiredfps;

static int32 fps_scale_table[]=
{ 3, 3, 4, 8, 16, 32, 64, 128, 192, 256, 384, 512, 768, 1024, 2048, 4096, 8192, 16384, 16384};
int32 fps_scale = 256;

static void RefreshThrottleFPS(void)
{
	printf("WTF\n");
	fflush(stdout);
 desiredfps=FCEUI_GetDesiredFPS()>>8;
 desiredfps=(desiredfps*fps_scale)>>8;
 
}

static uint64 GetCurTime(void)
{
 if(tmethod)
 {
  uint64 tmp;

  /* Practically, LARGE_INTEGER and uint64 differ only by signness and name. */
  QueryPerformanceCounter((LARGE_INTEGER*)&tmp);

  return(tmp);
 }
 else
  return((uint64)GetTickCount());

}

static uint64 ttime,ltime;

static void InitSpeedThrottle(void)
{
	timeBeginPeriod(1);
	SetThreadAffinityMask(GetCurrentThread(),1);
	

	tmethod=0;
	if(QueryPerformanceFrequency((LARGE_INTEGER*)&tfreq)) {
		tmethod=1;
	}
	else tfreq=1000;

	tfreq<<=16;    /* Adjustment for fps returned from FCEUI_GetDesiredFPS(). */
	ltime = 0; //mbg
}

///Resets the throttle timing. use this when the player releases the turbo
void ResetSpeedThrottle() {
	ltime = 0;
}


static int SpeedThrottle(void)
{
	//the desired running time for this frame
	uint64 desiredRunningTime = tfreq/desiredfps;

	ttime = GetCurTime();

	//if this is our first time, save ltime and bail out
	if(ltime == 0) {
		ltime = ttime;
		return 0;
	}

	//otherwise calculate our delta
	uint64 delta = ttime-ltime;
	
	/*printf("%20I64d %20I64d\n",delta,desiredRunningTime);
	fflush(stdout);*/
	if( delta < desiredRunningTime ) {
		//if the elapsed time is less than the desired running time

		//sleepy gets the time that needs to be slept.
		//it is then converted to ms<<16
		uint64 sleepy = desiredRunningTime-delta; 
		//sleepy *= 1000;
		//if we have more than 1 ms to sleep, sleep 1 ms and return
		if(sleepy>=65536) {
			Sleep(1);
			return 1;
		} else {
			//otherwise, we can't throttle for less than 1ms. assume we're close enough and return
			ltime += desiredRunningTime;
			return 0;
		}

		//sleepy*=1000;
		//if(tfreq>=65536)
		//	sleepy/=tfreq>>16;
		//else
		//	sleepy=0;
		//if(sleepy>100)
		//{
		//	// block for a max of 100ms to
		//	// keep the gui responsive
		//	Sleep(100);
		//	return 1;
		//}
		//Sleep(sleepy);
		//goto waiter;
	} else {
		//we're behind...
		if(delta > 2*desiredRunningTime) {
			//if we're behind by 2 frames, then reset the throttling
			ResetSpeedThrottle();
		} else {
			//we're only behind by part of a frame
			//just don't throttle!
			ltime += desiredRunningTime;
		}
		return 0;
	}
	//}
	//if( (ttime-ltime) >= (tfreq*4/desiredfps))
	//	ltime=ttime;
	//else
	//{
	//	ltime+=tfreq/desiredfps;

	//	if( (ttime-ltime) >= (tfreq/desiredfps) ) // Oops, we're behind!
	//		return(1);
	//}
	//return(0);
}

// Quick code for internal FPS display.
uint64 FCEUD_GetTime(void)
{
 return(GetCurTime());
}
uint64 FCEUD_GetTimeFreq(void)
{
 return(tfreq>>16);
}

static void IncreaseEmulationSpeed(void)
{
 int i;
 for(i=1; fps_scale_table[i]<fps_scale; i++)
  ;
 fps_scale = fps_scale_table[i+1];
}

static void DecreaseEmulationSpeed(void)
{
 int i;
 for(i=1; fps_scale_table[i]<fps_scale; i++)
  ;
 fps_scale = fps_scale_table[i-1];
}

#define fps_table_size		(sizeof(fps_scale_table)/sizeof(fps_scale_table[0]))

void FCEUD_SetEmulationSpeed(int cmd)
{
	switch(cmd)
	{
	case EMUSPEED_SLOWEST:	fps_scale=fps_scale_table[0];  break;
	case EMUSPEED_SLOWER:	DecreaseEmulationSpeed(); break;
	case EMUSPEED_NORMAL:	fps_scale=256; break;
	case EMUSPEED_FASTER:	IncreaseEmulationSpeed(); break;
	case EMUSPEED_FASTEST:	fps_scale=fps_scale_table[fps_table_size-1]; break;
	default:
		return;
	}

	RefreshThrottleFPS();

	FCEU_DispMessage("emulation speed %d%%",(fps_scale*100)>>8);
}
