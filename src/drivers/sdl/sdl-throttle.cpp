/// \file
/// \brief Handles emulation speed throttling using the SDL timing functions.

#include "sdl.h"
#include "throttle.h"

static uint64 tfreq;
static uint64 desiredfps;

static int32 fps_scale_table[]=
{ 3, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048 };
int32 fps_scale = 256;

#define fps_table_size (sizeof(fps_scale_table) / sizeof(fps_scale_table[0]))

/**
 * Refreshes the FPS throttling variables.
 */
void
RefreshThrottleFPS()
{
    desiredfps = FCEUI_GetDesiredFPS() >> 8;
    desiredfps = (desiredfps * fps_scale) >> 8;
    tfreq = 10000000;
    tfreq <<= 16; /* Adjustment for fps returned from FCEUI_GetDesiredFPS(). */
}

/**
 * Perform FPS speed throttling by delaying until the next time slot.
 */
void
SpeedThrottle()
{
    bool doDelay;

    // XXX soules - go back through and get rid of static function variables
    static uint64 ttime,ltime=0;
  
    // loop until we've delayed enough
    do {
        doDelay = false;

        // check the current time
        ttime = SDL_GetTicks();
        ttime *= 10000;

        if((ttime - ltime) < (tfreq / desiredfps)) {
            int64 delay = (tfreq / desiredfps) - (ttime - ltime);
            if(delay > 0) {
                SDL_Delay(delay / 10000);
            }

            doDelay = true;
        }
    } while(doDelay);

    // update the "last time" to match when we want the next tick
    if((ttime - ltime) >= ((tfreq * 4) / desiredfps)) {
        ltime = ttime;
    } else {
        ltime += tfreq / desiredfps;
    }
}

/**
 * Set the emulation speed throttling to the next entry in the speed table.
 */
void
IncreaseEmulationSpeed()
{
    int i = 0;

    // find the next entry in the FPS rate table
    while(i < (fps_table_size - 1) && fps_scale_table[i] < fps_scale) {
        i++;
    }
    fps_scale = fps_scale_table[i+1];

    // refresh the FPS throttling variables
    RefreshThrottleFPS();

    FCEU_DispMessage("emulation speed %d%%",(fps_scale*100)>>8);
}

/**
 * Set the emulation speed throttling to the previous entry in the speed table.
 */
void
DecreaseEmulationSpeed()
{
    int i = 1;

    // find the previous entry in the FPS rate table
    while(i < fps_table_size && fps_scale_table[i] < fps_scale) {
        i++;
    } 
    fps_scale = fps_scale_table[i - 1];

    // refresh the FPS throttling variables
    RefreshThrottleFPS();

    FCEU_DispMessage("emulation speed %d%%",(fps_scale*100)>>8);
}

/**
 * Set the emulation speed throttling to a specific value.
 */
void
FCEUD_SetEmulationSpeed(int cmd)
{
    switch(cmd) {
    case EMUSPEED_SLOWEST:
        fps_scale = fps_scale_table[0];
        break;
    case EMUSPEED_SLOWER:
        DecreaseEmulationSpeed();
        break;
    case EMUSPEED_NORMAL:
        fps_scale = 256;
        break;
    case EMUSPEED_FASTER:
        IncreaseEmulationSpeed();
        break;
    case EMUSPEED_FASTEST:
        fps_scale = fps_scale_table[fps_table_size - 1];
        break;
    default:
        return;
    }

    RefreshThrottleFPS();

    FCEU_DispMessage("emulation speed %d%%",(fps_scale*100)>>8);
}
