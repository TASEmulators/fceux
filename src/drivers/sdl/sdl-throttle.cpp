/// \file
/// \brief Handles emulation speed throttling using the SDL timing functions.

#include "sdl.h"
#include "throttle.h"

static uint64 s_tfreq;
static uint64 s_desiredfps;

static int32 s_fpsScaleTable[]=
{ 3, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048 };
int32 g_fpsScale = 256;

#define FPS_TABLE_SIZE (sizeof(s_fpsScaleTable) / sizeof(s_fpsScaleTable[0]))

/**
 * Refreshes the FPS throttling variables.
 */
void
RefreshThrottleFPS()
{
    s_desiredfps = FCEUI_GetDesiredFPS() >> 8;
    s_desiredfps = (s_desiredfps * g_fpsScale) >> 8;
    s_tfreq = 10000000;
    s_tfreq <<= 16; /* Adjust for fps returned from FCEUI_GetDesiredFPS(). */
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

        if((ttime - ltime) < (s_tfreq / s_desiredfps)) {
            int64 delay = (s_tfreq / s_desiredfps) - (ttime - ltime);
            if(delay > 0) {
                SDL_Delay(delay / 10000);
            }

            doDelay = true;
        }
    } while(doDelay);

    // update the "last time" to match when we want the next tick
    if((ttime - ltime) >= ((s_tfreq * 4) / s_desiredfps)) {
        ltime = ttime;
    } else {
        ltime += s_tfreq / s_desiredfps;
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
    while(i < (FPS_TABLE_SIZE - 2) && s_fpsScaleTable[i] < g_fpsScale) {
        i++;
    }
    g_fpsScale = s_fpsScaleTable[i+1];

    // refresh the FPS throttling variables
    RefreshThrottleFPS();

    FCEU_DispMessage("emulation speed %d%%",(g_fpsScale*100)>>8);
}

/**
 * Set the emulation speed throttling to the previous entry in the speed table.
 */
void
DecreaseEmulationSpeed()
{
    int i = 1;

    // find the previous entry in the FPS rate table
    while(i < FPS_TABLE_SIZE && s_fpsScaleTable[i] < g_fpsScale) {
        i++;
    } 
    g_fpsScale = s_fpsScaleTable[i - 1];

    // refresh the FPS throttling variables
    RefreshThrottleFPS();

    FCEU_DispMessage("emulation speed %d%%",(g_fpsScale*100)>>8);
}

/**
 * Set the emulation speed throttling to a specific value.
 */
void
FCEUD_SetEmulationSpeed(int cmd)
{
    switch(cmd) {
    case EMUSPEED_SLOWEST:
        g_fpsScale = s_fpsScaleTable[0];
        break;
    case EMUSPEED_SLOWER:
        DecreaseEmulationSpeed();
        break;
    case EMUSPEED_NORMAL:
        g_fpsScale = 256;
        break;
    case EMUSPEED_FASTER:
        IncreaseEmulationSpeed();
        break;
    case EMUSPEED_FASTEST:
        g_fpsScale = s_fpsScaleTable[FPS_TABLE_SIZE - 1];
        break;
    default:
        return;
    }

    RefreshThrottleFPS();

    FCEU_DispMessage("emulation speed %d%%",(g_fpsScale*100)>>8);
}
