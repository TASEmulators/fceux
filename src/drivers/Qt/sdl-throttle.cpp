/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020 mjbudd77
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
/// \file
/// \brief Handles emulation speed throttling using the SDL timing functions.

#include "Qt/sdl.h"
#include "Qt/NetPlay.h"
#include "Qt/throttle.h"
#include "utils/timeStamp.h"

#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
#include <time.h>
#endif

#ifdef __linux__
#include <sys/timerfd.h>
#endif

static const double Slowest = 0.015625; // 1/64x speed (around 1 fps on NTSC)
static const double Fastest = 32;       // 32x speed   (around 1920 fps on NTSC)
static const double Normal  = 1.0;      // 1x speed    (around 60 fps on NTSC)

static uint32 frameLateCounter = 0;
static FCEU::timeStampRecord Lasttime, Nexttime, Latetime;
static FCEU::timeStampRecord DesiredFrameTime, HalfFrameTime, QuarterFrameTime, DoubleFrameTime;
static FCEU::timeStampRecord emuSignalTx, guiSignalRx, emuSignalLatency;
static double desired_frametime = (1.0 / 60.099823);
static double desired_frameRate = (60.099823);
static double baseframeRate     = (60.099823);
static double frameDeltaCur = 0.0;
static double frameDeltaMin = 1.0;
static double frameDeltaMax = 0.0;
static double frameIdleCur = 0.0;
static double frameIdleMin = 1.0;
static double frameIdleMax = 0.0;
static double videoLastTs  = 0.0;
static double videoPeriodCur  = 0.0;
static double videoPeriodMin  = 1.0;
static double videoPeriodMax  = 0.0;
static double emuLatencyCur  = 0.0;
static double emuLatencyMin  = 1.0;
static double emuLatencyMax  = 0.0;
static bool   keepFrameTimeStats = false;
static int InFrame = 0;
double g_fpsScale = Normal; // used by sdl.cpp
bool MaxSpeed = false;
bool useIntFrameRate = false;
static double frmRateAdjRatio = 1.000000f; // Frame Rate Adjustment Ratio
extern bool turbo;

int NetPlayThrottleControl();

double getHighPrecTimeStamp(void)
{
	double t;

	if (FCEU::timeStampModuleInitialized())
	{
		FCEU::timeStampRecord ts;

		ts.readNew();

		t = ts.toSeconds();
	}
	else
	{
		t = (double)SDL_GetTicks();

		t = t * 1e-3;
	}
	return t;
}

#ifdef __linux__
static char useTimerFD = 0;
static int  timerfd = -1;

static void setTimer( double hz )
{
	struct itimerspec ispec;

	if ( !useTimerFD )
	{
		if ( timerfd != -1 )
		{
			::close( timerfd ); timerfd = -1;
		}
		return;
	}

	if ( timerfd == -1 )
	{
		timerfd = timerfd_create( CLOCK_REALTIME, 0 );

		if ( timerfd == -1 )
		{
			perror("timerfd_create failed: ");
			return;
		}
	}
	ispec.it_interval.tv_sec  = 0;
	ispec.it_interval.tv_nsec = (long)( 1.0e9 / hz );
	ispec.it_value.tv_sec  = ispec.it_interval.tv_sec;
	ispec.it_value.tv_nsec = ispec.it_interval.tv_nsec;

	if ( timerfd_settime( timerfd, 0, &ispec, NULL ) == -1 )
	{
		perror("timerfd_settime failed: ");
	}

	//printf("Timer Set: %li ns\n", ispec.it_value.tv_nsec );

	Lasttime.readNew();
	Nexttime = Lasttime + DesiredFrameTime;
	Latetime = Nexttime + HalfFrameTime;
}
#endif

int getTimingMode(void)
{
#ifdef __linux__
	if ( useTimerFD )
	{
		return 1;
	}
#endif
	return 0;
}

int setTimingMode( int mode )
{
#ifdef __linux__
	useTimerFD = (mode == 1);
#endif
	return 0;
}

void setFrameTimingEnable( bool enable )
{
	keepFrameTimeStats = enable;
}

int  getFrameTimingStats( struct frameTimingStat_t *stats )
{
	stats->enabled   = keepFrameTimeStats;
	stats->lateCount = frameLateCounter;

	stats->frameTimeAbs.tgt = desired_frametime;
	stats->frameTimeAbs.cur = frameDeltaCur;
	stats->frameTimeAbs.min = frameDeltaMin;
	stats->frameTimeAbs.max = frameDeltaMax;

	stats->frameTimeDel.tgt = 0.0;
	stats->frameTimeDel.cur = frameDeltaCur - desired_frametime;
	stats->frameTimeDel.min = frameDeltaMin - desired_frametime;
	stats->frameTimeDel.max = frameDeltaMax - desired_frametime;

	stats->frameTimeIdle.tgt = desired_frametime * 0.50;
	stats->frameTimeIdle.cur = frameIdleCur;
	stats->frameTimeIdle.min = frameIdleMin;
	stats->frameTimeIdle.max = frameIdleMax;

	stats->frameTimeWork.tgt = desired_frametime - stats->frameTimeIdle.tgt;
	stats->frameTimeWork.cur = desired_frametime - frameIdleCur;
	stats->frameTimeWork.min = desired_frametime - frameIdleMax;
	stats->frameTimeWork.max = desired_frametime - frameIdleMin;

	if ( stats->frameTimeWork.cur < 0 )
	{
		stats->frameTimeWork.cur = 0;
	}
	if ( stats->frameTimeWork.min < 0 )
	{
		stats->frameTimeWork.min = 0;
	}
	if ( stats->frameTimeWork.max < 0 )
	{
		stats->frameTimeWork.max = 0;
	}

	stats->videoTimeDel.tgt = desired_frametime;
	stats->videoTimeDel.cur = videoPeriodCur;
	stats->videoTimeDel.min = videoPeriodMin;
	stats->videoTimeDel.max = videoPeriodMax;

	stats->emuSignalDelay.tgt = 0.0;
	stats->emuSignalDelay.cur = emuLatencyCur;
	stats->emuSignalDelay.min = emuLatencyMin;
	stats->emuSignalDelay.max = emuLatencyMax;

	return 0;
}

void videoBufferSwapMark(void)
{
	if ( keepFrameTimeStats )
	{
		double ts = getHighPrecTimeStamp();

		videoPeriodCur = ts - videoLastTs;

		if ( videoPeriodCur < videoPeriodMin )
		{
			videoPeriodMin = videoPeriodCur;
		}
		if ( videoPeriodCur > videoPeriodMax )
		{
			videoPeriodMax = videoPeriodCur;
		}
		videoLastTs = ts;
	}
}

void emuSignalSendMark(void)
{
	if ( keepFrameTimeStats )
	{
		emuSignalTx.readNew();
	}
}

void guiSignalRecvMark(void)
{
	if ( keepFrameTimeStats )
	{
		guiSignalRx.readNew();
		emuSignalLatency = guiSignalRx - emuSignalTx;

		emuLatencyCur = emuSignalLatency.toSeconds();

		if ( emuLatencyCur < emuLatencyMin )
		{
			emuLatencyMin = emuLatencyCur;
		}
		if ( emuLatencyCur > emuLatencyMax )
		{
			emuLatencyMax = emuLatencyCur;
		}
	}
}

void resetFrameTiming(void)
{
	frameLateCounter = 0;
	frameDeltaMax = 0.0;
	frameDeltaMin = 1.0;
	frameIdleMax = 0.0;
	frameIdleMin = 1.0;
	videoPeriodMin = 1.0;
	videoPeriodMax = 0.0;
	emuLatencyMin =  1.0;
	emuLatencyMax =  0.0;
}

/* LOGMUL = exp(log(2) / 3)
 *
 * This gives us a value such that if we do x*=LOGMUL three times,
 * then after that, x is twice the value it was before.
 *
 * This gives us three speed steps per order of magnitude.
 *
 */
#define LOGMUL 1.259921049894873

/**
 * Refreshes the FPS throttling variables.
 */
void
RefreshThrottleFPS(void)
{
	double hz;
	int32_t fps = FCEUI_GetDesiredFPS(); // Do >> 24 to get in Hz
	int32_t T;

	hz = ( ((double)fps) / 16777216.0 );

	desired_frametime = 1.0 / ( hz * g_fpsScale );

	if ( useIntFrameRate )
	{
		hz = (double)( (int)(hz) );

		frmRateAdjRatio = (1.0 / ( hz * g_fpsScale )) / desired_frametime;

		//printf("frameAdjRatio:%f \n", frmRateAdjRatio );
	}
	else
	{
		frmRateAdjRatio = 1.000000f;
	}
	desired_frametime = 1.0 / ( hz * g_fpsScale );
	desired_frameRate = ( hz * g_fpsScale );
	baseframeRate = hz;

	T = (int32_t)( desired_frametime * 1000.0 );

	if ( T < 0 ) T = 1;

	DesiredFrameTime.fromSeconds( desired_frametime );
	HalfFrameTime = DesiredFrameTime / 2;
	QuarterFrameTime = DesiredFrameTime / 4;
	DoubleFrameTime = DesiredFrameTime * 2;

	//printf("FrameTime: %f  %f  %f  %f \n", DesiredFrameTime.toSeconds(),
	//		HalfFrameTime.toSeconds(), QuarterFrameTime.toSeconds(), DoubleFrameTime.toSeconds() );
	//printf("FrameTime: %llu  %llu  %f  %lf \n", fps, fps >> 24, hz, desired_frametime );

	Lasttime.zero();
	Nexttime.zero();
	InFrame=0;

#ifdef __linux__
	setTimer( hz * g_fpsScale );
#endif

}

double getBaseFrameRate(void)
{
	return baseframeRate;
}

double getFrameRate(void)
{
	return desired_frameRate;
}

double getFrameRateAdjustmentRatio(void)
{
	return frmRateAdjRatio;
}

static int highPrecSleep( FCEU::timeStampRecord &ts )
{
	int ret = 0;
#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
	struct timespec req, rem;
	
	req = ts.toTimeSpec();

	ret = nanosleep( &req, &rem );
#else
	SDL_Delay( ts.toMilliSeconds() );
#endif
	return ret;
}

/**
 * Perform FPS speed throttling by delaying until the next time slot.
 */
int
SpeedThrottle(void)
{
	bool isEmuPaused = FCEUI_EmulationPaused() ? true : false;
	bool noWaitActive = (NoWaiting & 0x01) ? true : false;
	bool turboActive = (turbo || noWaitActive || NetPlaySkipWait());

	// If Emulator is paused, don't waste CPU cycles spinning on nothing.
	if ( !isEmuPaused && ((g_fpsScale >= 32) || turboActive) )
	{
		//printf("Skip Wait\n");
		return 0; /* Done waiting */
	}
	FCEU::timeStampRecord cur_time, idleStart, time_left;
    
	cur_time.readNew();
	idleStart = cur_time;

	if (Lasttime.isZero())
	{
		//printf("Lasttime Reset\n");
		Lasttime = cur_time;
		Latetime = Lasttime + DoubleFrameTime;
	}
    
	if (!InFrame)
	{
		InFrame = 1;
		Nexttime = Lasttime + DesiredFrameTime;
		Latetime = Nexttime + HalfFrameTime;
	}
    
	if (cur_time >= Nexttime)
	{
		time_left.zero();
	}
	else
	{
		time_left = Nexttime - cur_time;
	}
    
	// If Emulator is paused, ensure that sleep time cannot be zero
	// we don't want to waste a full CPU core on nothing.
	if (isEmuPaused)
	{
		if (time_left.toMilliSeconds() < 1)
		{
			time_left.fromMilliSeconds(1);
		}
	}

	if (time_left.toMilliSeconds() > 50)
	{
		time_left.fromMilliSeconds(50);
		/* In order to keep input responsive, don't wait too long at once */
		/* 50 ms wait gives us a 20 Hz responsetime which is nice. */
	}
	else
	{
		InFrame = 0;
	}

	//fprintf(stderr, "attempting to sleep %Ld ms, frame complete=%s\n",
	//	time_left, InFrame?"no":"yes");

#ifdef __linux__
	if ( timerfd != -1 )
	{
		uint64_t val;

		if ( read( timerfd, &val, sizeof(val) ) > 0 )
		{
			if ( val > 1 )
			{
				frameLateCounter += (val - 1);
				//printf("Late Frame: %u \n", frameLateCounter);
			}
		}
	}
	else if ( !time_left.isZero() )
	{
		highPrecSleep( time_left );
	}
	else
	{
		if ( cur_time >= Latetime )
		{
			frameLateCounter++;
			//printf("Late Frame: %u  - %llu ms\n", frameLateCounter, cur_time - Latetime);
		}
	}
#else
	if ( !time_left.isZero() )
	{
		highPrecSleep( time_left );
	}
	else
	{
		if ( cur_time >= Latetime )
		{
			frameLateCounter++;
			//printf("Late Frame: %u  - %llu ms\n", frameLateCounter, cur_time - Latetime);
		}
	}
#endif
    
	cur_time.readNew();

	if ( cur_time >= (Nexttime - QuarterFrameTime) )
	{
		if ( keepFrameTimeStats )
		{
			FCEU::timeStampRecord diffTime = (cur_time - Lasttime);

			frameDeltaCur = diffTime.toSeconds();

			if ( frameDeltaCur < frameDeltaMin )
			{
				frameDeltaMin = frameDeltaCur;
			}
			if ( frameDeltaCur > frameDeltaMax )
			{
				frameDeltaMax = frameDeltaCur;
			}

			diffTime = (cur_time - idleStart);

			frameIdleCur = diffTime.toSeconds();

			if ( frameIdleCur < frameIdleMin )
			{
				frameIdleMin = frameIdleCur;
			}
			if ( frameIdleCur > frameIdleMax )
			{
				frameIdleMax = frameIdleCur;
			}
			//printf("Frame Delta: %f us   min:%f   max:%f \n", frameDelta * 1e6, frameDeltaMin * 1e6, frameDeltaMax * 1e6 );
			//printf("Frame Sleep Time: %f   Target Error: %f us\n", time_left * 1e6, (cur_time - Nexttime) * 1e6 );
		}
		NetPlayThrottleControl();

		Lasttime = Nexttime;
		Nexttime = Lasttime + DesiredFrameTime;
		Latetime = Nexttime + HalfFrameTime;

		if ( cur_time >= Nexttime )
		{
			Lasttime = cur_time;
			Nexttime = Lasttime + DesiredFrameTime;
			Latetime = Nexttime + HalfFrameTime;
		}

		return 0; /* Done waiting */
	}

	return 1; /* Must still wait some more */
}

/**
 * Set the emulation speed throttling to the next entry in the speed table.
 */
void IncreaseEmulationSpeed(void)
{
	g_fpsScale *= LOGMUL;
    
	if (g_fpsScale > Fastest) g_fpsScale = Fastest;

	RefreshThrottleFPS();
     
	FCEU_DispMessage("Emulation speed %.1f%%",0, g_fpsScale*100.0);
}

/**
 * Set the emulation speed throttling to the previous entry in the speed table.
 */
void DecreaseEmulationSpeed(void)
{
	g_fpsScale /= LOGMUL;
	if(g_fpsScale < Slowest)
		g_fpsScale = Slowest;

	RefreshThrottleFPS();

	FCEU_DispMessage("Emulation speed %.1f%%",0, g_fpsScale*100.0);
}

int CustomEmulationSpeed(int spdPercent)
{
   if ( spdPercent < 1 )
   {
      return -1;
   }
	g_fpsScale = ((double)spdPercent) / 100.0f;

	if (g_fpsScale < Slowest)
	{
		g_fpsScale = Slowest;
	}
	else if (g_fpsScale > Fastest)
	{
		g_fpsScale = Fastest;
	}

	RefreshThrottleFPS();

	FCEU_DispMessage("Emulation speed %.1f%%",0, g_fpsScale*100.0);

   return 0;
}

int NetPlayThrottleControl()
{
	NetPlayClient *client = NetPlayClient::GetInstance();

	if (client)
	{
		uint32_t inputAvailCount = client->inputAvailableCount();
		const uint32_t tailTarget = client->tailTarget;

		double targetDelta = static_cast<double>( static_cast<intptr_t>(inputAvailCount) - static_cast<intptr_t>(tailTarget) );

		// Simple linear FPS scaling adjustment based on target error.
		constexpr double speedUpSlope = (0.05) / (10.0);
		double newScale = 1.0 + (targetDelta * speedUpSlope);

		if (newScale != g_fpsScale)
		{
			double hz;
			int32_t fps = FCEUI_GetDesiredFPS(); // Do >> 24 to get in Hz
			int32_t T;

			g_fpsScale = newScale;

			hz = ( ((double)fps) / 16777216.0 );

			desired_frametime = 1.0 / ( hz * g_fpsScale );

			if ( useIntFrameRate )
			{
				hz = (double)( (int)(hz) );

				frmRateAdjRatio = (1.0 / ( hz * g_fpsScale )) / desired_frametime;

				//printf("frameAdjRatio:%f \n", frmRateAdjRatio );
			}
			else
			{
				frmRateAdjRatio = 1.000000f;
			}
			desired_frametime = 1.0 / ( hz * g_fpsScale );
			desired_frameRate = ( hz * g_fpsScale );
			baseframeRate = hz;

			T = (int32_t)( desired_frametime * 1000.0 );

			if ( T < 0 ) T = 1;

			DesiredFrameTime.fromSeconds( desired_frametime );
			HalfFrameTime = DesiredFrameTime / 2;
			QuarterFrameTime = DesiredFrameTime / 4;
			DoubleFrameTime = DesiredFrameTime * 2;
		}
		//printf("NetPlayCPUThrottle: %f   %f    %f  Target:%u  InputAvail:%u\n", newScale, desired_frameRate, targetDelta, tailTarget, inputAvailCount);
	}

   return 0;
}


/**
 * Set the emulation speed throttling to a specific value.
 */
void
FCEUD_SetEmulationSpeed(int cmd)
{
	MaxSpeed = false;
    
	switch(cmd) {
	case EMUSPEED_SLOWEST:
		g_fpsScale = Slowest;
		break;
	case EMUSPEED_SLOWER:
		DecreaseEmulationSpeed();
		break;
	case EMUSPEED_NORMAL:
		g_fpsScale = Normal;
		break;
	case EMUSPEED_FASTER:
		IncreaseEmulationSpeed();
		break;
	case EMUSPEED_FASTEST:
		g_fpsScale = Fastest;
		MaxSpeed = true;
		break;
	default:
		return;
	}

	RefreshThrottleFPS();

	FCEU_DispMessage("Emulation speed %.1f%%",0, g_fpsScale*100.0);
}
