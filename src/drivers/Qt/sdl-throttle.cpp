/// \file
/// \brief Handles emulation speed throttling using the SDL timing functions.

#include "Qt/sdl.h"
#include "Qt/throttle.h"

#if defined(__linux) || defined(__APPLE__)
#include <time.h>
#endif

#ifdef __linux__
#include <sys/timerfd.h>
#endif

static const double Slowest = 0.015625; // 1/64x speed (around 1 fps on NTSC)
static const double Fastest = 32;       // 32x speed   (around 1920 fps on NTSC)
static const double Normal  = 1.0;      // 1x speed    (around 60 fps on NTSC)

static uint32 frameLateCounter = 0;
static double Lasttime=0, Nexttime=0, Latetime=0;
static double desired_frametime = (1.0 / 60.099823);
static double frameDeltaCur = 0.0;
static double frameDeltaMin = 1.0;
static double frameDeltaMax = 0.0;
static double frameIdleCur = 0.0;
static double frameIdleMin = 1.0;
static double frameIdleMax = 0.0;
static bool   keepFrameTimeStats = false;
static int InFrame = 0;
double g_fpsScale = Normal; // used by sdl.cpp
bool MaxSpeed = false;

double getHighPrecTimeStamp(void)
{
#if defined(__linux) || defined(__APPLE__)
	struct timespec ts;
	double t;

	clock_gettime( CLOCK_REALTIME, &ts );

	t = (double)ts.tv_sec + (double)(ts.tv_nsec * 1.0e-9);
#else
	double t;

	t = (double)SDL_GetTicks();

	t = t * 1e-3;
#endif

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

	Lasttime = getHighPrecTimeStamp();
	Nexttime = Lasttime + desired_frametime;
	Latetime = Nexttime + (desired_frametime*0.50);
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

	return 0;
}

void resetFrameTiming(void)
{
	frameLateCounter = 0;
	frameDeltaMax = 0.0;
	frameDeltaMin = 1.0;
	frameIdleMax = 0.0;
	frameIdleMin = 1.0;
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

	T = (int32_t)( desired_frametime * 1000.0 );

	if ( T < 0 ) T = 1;

   //printf("FrameTime: %llu  %llu  %f  %lf \n", fps, fps >> 24, hz, desired_frametime );

	Lasttime=0;   
	Nexttime=0;
	InFrame=0;

#ifdef __linux__
	setTimer( hz * g_fpsScale );
#endif

}

int highPrecSleep( double timeSeconds )
{
	int ret = 0;
#if defined(__linux) || defined(__APPLE__)
	struct timespec req, rem;
	
	req.tv_sec  = (long)timeSeconds;
	req.tv_nsec = (long)((timeSeconds - (double)req.tv_sec) * 1e9);

	ret = nanosleep( &req, &rem );
#else
	SDL_Delay( (long)(time_left * 1e3) );
#endif
	return ret;
}

/**
 * Perform FPS speed throttling by delaying until the next time slot.
 */
int
SpeedThrottle(void)
{
	if (g_fpsScale >= 32)
	{
		return 0; /* Done waiting */
	}
	double time_left;
	double cur_time, idleStart;
	double frame_time = desired_frametime;
	double halfFrame = 0.500 * frame_time;
	double quarterFrame = 0.250 * frame_time;
    
	idleStart = cur_time = getHighPrecTimeStamp();

	if (Lasttime < 1.0)
	{
		Lasttime = cur_time;
		Latetime = Lasttime + 2.0*frame_time;
	}
    
	if (!InFrame)
	{
		InFrame = 1;
		Nexttime = Lasttime + frame_time;
		Latetime = Nexttime + halfFrame;
	}
    
	if (cur_time >= Nexttime)
	{
		time_left = 0;
	}
	else
	{
		time_left = Nexttime - cur_time;
	}
    
	if (time_left > 50)
	{
		time_left = 50;
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
	else if ( time_left > 0 )
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
	if ( time_left > 0 )
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
    
	cur_time = getHighPrecTimeStamp();

	if ( cur_time >= (Nexttime - quarterFrame) )
	{
		if ( keepFrameTimeStats )
		{

			frameDeltaCur = (cur_time - Lasttime);

			if ( frameDeltaCur < frameDeltaMin )
			{
				frameDeltaMin = frameDeltaCur;
			}
			if ( frameDeltaCur > frameDeltaMax )
			{
				frameDeltaMax = frameDeltaCur;
			}

			frameIdleCur = (cur_time - idleStart);

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
		Lasttime = Nexttime;
		Nexttime = Lasttime + frame_time;
		Latetime = Nexttime + halfFrame;

		if ( cur_time >= Nexttime )
		{
			Lasttime = cur_time;
			Nexttime = Lasttime + frame_time;
			Latetime = Nexttime + halfFrame;
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
