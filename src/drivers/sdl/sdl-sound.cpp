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

/// \file
/// \brief Handles sound emulation using the SDL.

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "sdl.h"

static volatile int *Buffer = 0;
static unsigned int BufferSize;
static unsigned int BufferRead;
static unsigned int BufferWrite;
static volatile unsigned int BufferIn;

/**
 * Callback from the SDL to get and play audio data.
 */
static void
fillaudio(void *udata,
          uint8 *stream,
          int len)
{
    int16 *tmps = (int16*)stream;
    len >>= 1;

    while(len) {
        int16 sample = 0;
        if(BufferIn) {
            sample = Buffer[BufferRead];
            BufferRead = (BufferRead + 1) % BufferSize;
            BufferIn--;
        } else {
            sample = 0;
        }

        *tmps = sample;
        tmps++;
        len--;
    }
}

/**
 * Initialize the audio subsystem.
 */
int
InitSound(FCEUGI *gi)
{
    SDL_AudioSpec spec;
    if(!_sound) return(0);

    memset(&spec, 0, sizeof(spec));
    if(SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
        puts(SDL_GetError());
        KillSound();
        return(0);
    }

    spec.freq = soundrate;
    spec.format = AUDIO_S16SYS;
    spec.channels = 1;
    spec.samples = 256;
    spec.callback = fillaudio;
    spec.userdata = 0;

    BufferSize = soundbufsize * soundrate / 1000;

    // XXX soules - what is this??
    /* SDL uses at least double-buffering, so multiply by 2. */
    BufferSize -= spec.samples * 2;

    if(BufferSize < spec.samples) BufferSize = spec.samples;

    Buffer = (int *)malloc(sizeof(int) * BufferSize);
    BufferRead = BufferWrite = BufferIn = 0;

    //printf("SDL Size: %d, Internal size: %d\n",spec.samples,BufferSize);

    if(SDL_OpenAudio(&spec, 0) < 0) {
        puts(SDL_GetError());
        KillSound();
        return(0);
    }
    SDL_PauseAudio(0);
    FCEUI_Sound(soundrate);
    return(1);
}


/**
 * Returns the size of the audio buffer.
 */
uint32
GetMaxSound(void)
{
    return(BufferSize);
}

/**
 * Returns the amount of free space in the audio buffer.
 */
uint32
GetWriteSound(void)
{
    return(BufferSize - BufferIn);
}

/**
 * Send a sound clip to the audio subsystem.
 */
void
WriteSound(int32 *buf,
           int Count)
{
    while(Count) {
        while(BufferIn == BufferSize) {
            SDL_Delay(1);
        }

        Buffer[BufferWrite] = *buf;
        Count--;
        BufferWrite = (BufferWrite + 1) % BufferSize;
        BufferIn++;
        buf++;
    }
}

/**
 * Pause (1) or unpause (0) the audio output.
 */
void
SilenceSound(int n)
{ 
    SDL_PauseAudio(n);   
}

/**
 * Shut down the audio subsystem.
 */
int
KillSound(void)
{
    FCEUI_Sound(0);
    SDL_CloseAudio();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    if(Buffer) {
        free((void *)Buffer);
        Buffer = 0;
    }
    return(0);
}


static int mute=0;
static int soundvolume=100;

/**
 * Adjust the volume either down (-1), up (1), or to the default (0).
 * Unmutes if mute was active before.
 */
void
FCEUD_SoundVolumeAdjust(int n)
{
    switch(n) {
    case -1:
        soundvolume -= 10;
        if(soundvolume < 0) {
            soundvolume = 0;
        }
        break;
    case 0:
        soundvolume = 100;
        break;
    case 1:
        soundvolume += 10;
        if(soundvolume > 150) {
            soundvolume = 150;
        }
        break;
    }

    mute = 0;
    FCEUI_SetSoundVolume(soundvolume);
    FCEU_DispMessage("Sound volume %d.", soundvolume);
}

/**
 * Toggles the sound on or off.
 */
void
FCEUD_SoundToggle(void)
{
    if(mute) {
        mute = 0;
        FCEUI_SetSoundVolume(soundvolume);
        FCEU_DispMessage("Sound mute off.");
    } else {
        mute = 1;
        FCEUI_SetSoundVolume(0);
        FCEU_DispMessage("Sound mute on.");
    }
}
