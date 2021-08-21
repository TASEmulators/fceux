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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/// \file
/// \brief Handles sound emulation using the SDL.

#include "sdl.h"

#include "common/configSys.h"
#include "utils/memory.h"
#include "Qt/nes_shm.h"
#include "Qt/throttle.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>

extern Config *g_config;

static volatile int *s_Buffer = 0;
static unsigned int s_BufferSize;
static unsigned int s_BufferSize25;
static unsigned int s_BufferSize50;
static unsigned int s_BufferSize75;
static unsigned int s_BufferRead;
static unsigned int s_BufferWrite;
static volatile unsigned int s_BufferIn;
static unsigned int s_SampleRate = 44100;
static double noiseGate = 0.0;
static double noiseGateRate = 0.010;
static bool   noiseGateActive = true;

static int s_mute = 0;

extern int EmulationPaused;
extern double frmRateAdjRatio;
extern double g_fpsScale;

/**
 * Callback from the SDL to get and play audio data.
 */
static void
fillaudio(void *udata,
		uint8 *stream,
		int len)
{
	char bufStarveDetected = 0;
	static int16_t sample = 0;
	//unsigned int starve_lp = nes_shm->sndBuf.starveCounter;
	int16 *tmps = (int16*)stream;
	len >>= 1;

	if ( EmulationPaused || noiseGateActive )
	{
		// This noise gate helps avoid abrupt snaps in audio
		// when pausing emulation.
		while (len) 
		{
			if (EmulationPaused)
			{
				noiseGate -= noiseGateRate;

				if ( noiseGate < 0.0 )
				{
					noiseGate = 0.0;
				}
				noiseGateActive = 1;
			}
			else
			{
				if ( s_BufferIn )
				{	
					noiseGate += noiseGateRate;

					if ( noiseGate > 1.0 )
					{
						noiseGate = 1.0;
						noiseGateActive = 0;
					}
				}
			}
			if (s_BufferIn) 
			{
				sample = s_Buffer[s_BufferRead] * noiseGate;
				s_BufferRead = (s_BufferRead + 1) % s_BufferSize;
				s_BufferIn--;

				*tmps = sample;
			}
			else
			{
         			// Retain last known sample value, helps avoid clicking
         			// noise when sound system is starved of audio data.
				*tmps = sample * noiseGate;
			}

			tmps++;
			len--;
		}
	}
	else
	{
		while (len) 
		{
			if (s_BufferIn) 
			{
				sample = s_Buffer[s_BufferRead];
				s_BufferRead = (s_BufferRead + 1) % s_BufferSize;
				s_BufferIn--;
			} else {
        	 		// Retain last known sample value, helps avoid clicking
        	 		// noise when sound system is starved of audio data.
				//sample = 0; 
				bufStarveDetected = 1;
				nes_shm->sndBuf.starveCounter++;
			}

			nes_shm->push_sound_sample( sample );

			*tmps = sample;
			tmps++;
			len--;
		}
	}
	if ( bufStarveDetected )
	{
		//s_StarveCounter = nes_shm->sndBuf.starveCounter - starve_lp;
		//printf("Starve:%u\n", s_StarveCounter );
	}
}

/**
 * Initialize the audio subsystem.
 */
int
InitSound()
{
	int sound, soundrate, soundbufsize, soundvolume, soundtrianglevolume, soundsquare1volume, soundsquare2volume, soundnoisevolume, soundpcmvolume, soundq;
	SDL_AudioSpec spec;
	const char *driverName;
	int frmRateSampleAdj = 0;

	g_config->getOption("SDL.Sound", &sound);
	if (!sound) 
	{
		return 0;
	}

	memset(&spec, 0, sizeof(spec));
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) 
	{
		puts(SDL_GetError());
		KillSound();
		return 0;
	}

	// load configuration variables
	g_config->getOption("SDL.Sound.Rate", &soundrate);
	g_config->getOption("SDL.Sound.BufSize", &soundbufsize);
	g_config->getOption("SDL.Sound.Volume", &soundvolume);
	g_config->getOption("SDL.Sound.Quality", &soundq);
	g_config->getOption("SDL.Sound.TriangleVolume", &soundtrianglevolume);
	g_config->getOption("SDL.Sound.Square1Volume", &soundsquare1volume);
	g_config->getOption("SDL.Sound.Square2Volume", &soundsquare2volume);
	g_config->getOption("SDL.Sound.NoiseVolume", &soundnoisevolume);
	g_config->getOption("SDL.Sound.PCMVolume", &soundpcmvolume);

	spec.freq = s_SampleRate = soundrate;
	spec.format = AUDIO_S16SYS;
	spec.channels = 1;
	//spec.samples = 512;
	spec.samples = (int)( ( (double)s_SampleRate / getBaseFrameRate() ) );
	spec.callback = fillaudio;
	spec.userdata = 0;

	s_BufferSize = soundbufsize * soundrate / 1000;

	// For safety, set a bare minimum:
	if (s_BufferSize < spec.samples * 2)
	{
		s_BufferSize = spec.samples * 2;
	}
	s_BufferSize25 =    s_BufferSize/4;
	s_BufferSize50 =    s_BufferSize/2;
	s_BufferSize75 = (3*s_BufferSize)/4;

	//printf("Audio Buffer: %i  %i \n", spec.samples, s_BufferSize );

	noiseGate = 0.0;
	noiseGateRate = 1.0 / (double)spec.samples;
	noiseGateActive = true;

	s_Buffer = (int *)FCEU_dmalloc(sizeof(int) * s_BufferSize);

	if (!s_Buffer)
	{
		return 0;
	}
	s_BufferRead = s_BufferWrite = s_BufferIn = 0;

	if (SDL_OpenAudio(&spec, 0) < 0)
	{
		puts(SDL_GetError());
		KillSound();
		return 0;
	}
	SDL_PauseAudio(0);

	driverName = SDL_GetCurrentAudioDriver();

	if ( driverName )
	{
		fprintf(stderr, "Loading SDL sound with %s driver...\n", driverName);
	}
 
	frmRateSampleAdj = (int)( ( ((double)soundrate) * getFrameRateAdjustmentRatio()) - ((double)soundrate) );
	//frmRateSampleAdj = 0;
	//printf("Sample Rate Adjustment: %+i\n", frmRateSampleAdj );

	FCEUI_SetSoundVolume(soundvolume);
	FCEUI_SetSoundQuality(soundq);
	FCEUI_Sound(soundrate + frmRateSampleAdj);
	FCEUI_SetTriangleVolume(soundtrianglevolume);
	FCEUI_SetSquare1Volume(soundsquare1volume);
	FCEUI_SetSquare2Volume(soundsquare2volume);
	FCEUI_SetNoiseVolume(soundnoisevolume);
	FCEUI_SetPCMVolume(soundpcmvolume);
	return 1;
}


/**
 * Returns the size of the audio buffer.
 */
uint32
GetMaxSound(void)
{
	return(s_BufferSize);
}

/**
 * Returns the amount of free space in the audio buffer.
 */
uint32
GetWriteSound(void)
{
	return(s_BufferSize - s_BufferIn);
}

/**
 * Send a sound clip to the audio subsystem.
 */
void
WriteSound(int32 *buf,
           int Count)
{

	int uflowMode = 0;
	int ovrFlowSkip = 1;
	int udrFlowDup  = 1;
	static int skipCounter = 0;

	if ( NoWaiting & 0x01 )
	{	// During Turbo mode, don't bother with sound as
		// overflowing the audio buffer can cause delays.
		return;
	}

	if ( g_fpsScale >= 0.99995 )
	{
		uflowMode = 0;
		ovrFlowSkip = (int)(g_fpsScale * 1000);

		if ( s_BufferIn >= s_BufferSize50 )
		{
			ovrFlowSkip += 100;
		}
	}
	else
	{
		udrFlowDup = (int)(1.0 / g_fpsScale);

		if ( udrFlowDup < 1 )
		{
			udrFlowDup = 1;
		}
		if ( s_BufferIn < s_BufferSize50 )
		{
			udrFlowDup++;
		}
		else if ( s_BufferIn > s_BufferSize75 )
		{
			udrFlowDup--;
		}
		uflowMode = (udrFlowDup > 1);
	}
	extern int EmulationPaused;
	if (EmulationPaused == 0)
	{
		int waitCount = 0;

		if ( uflowMode )
		{	// Underflow mode
			SDL_LockAudio();
			while (Count)
			{
				if ( s_BufferIn == s_BufferSize )
				{
					SDL_UnlockAudio();
					while (s_BufferIn == s_BufferSize) 
					{
						SDL_Delay(1); waitCount++;

						if ( waitCount > 1000 )
						{
							printf("Error: Sound sink is not draining... Breaking out of audio loop to prevent lockup.\n");
							return;
						}
					}
					SDL_LockAudio();
				}

				for (int i=0; i<udrFlowDup; i++)
				{
					s_Buffer[s_BufferWrite] = *buf;
					s_BufferWrite = (s_BufferWrite + 1) % s_BufferSize;
            
					s_BufferIn++;
				}
            
				Count--;
				buf++;
			}
			SDL_UnlockAudio();
		}
		else
		{
			if ( ovrFlowSkip <= 1000 )
			{	// Perfect one to one realtime
				skipCounter = 0;

				SDL_LockAudio();
				while (Count)
				{
					if (s_BufferIn == s_BufferSize) 
					{
						SDL_UnlockAudio();
						while (s_BufferIn == s_BufferSize) 
						{
							SDL_Delay(1); waitCount++;

							if ( waitCount > 1000 )
							{
								printf("Error: Sound sink is not draining... Breaking out of audio loop to prevent lockup.\n");
								return;
							}
						}
						SDL_LockAudio();
					}

					s_Buffer[s_BufferWrite] = *buf;
					Count--;
					s_BufferWrite = (s_BufferWrite + 1) % s_BufferSize;
            
					s_BufferIn++;
					buf++;
				}
				SDL_UnlockAudio();
			}
			else
			{	// Overflow mode
				SDL_LockAudio();
				while (Count)
				{
					if (s_BufferIn == s_BufferSize) 
					{
						SDL_UnlockAudio();
						while (s_BufferIn == s_BufferSize) 
						{
							SDL_Delay(1); waitCount++;

							if ( waitCount > 1000 )
							{
								printf("Error: Sound sink is not draining... Breaking out of audio loop to prevent lockup.\n");
								return;
							}
						}
						SDL_LockAudio();
					}

					if ( skipCounter >= ovrFlowSkip )
					{
						s_Buffer[s_BufferWrite] = *buf;
						s_BufferWrite = (s_BufferWrite + 1) % s_BufferSize;
            
						s_BufferIn++;

						skipCounter -= ovrFlowSkip;
					}
					skipCounter = (skipCounter+1000);
            
					Count--;
					buf++;
				}
				SDL_UnlockAudio();
			}

		}
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
	if(s_Buffer) {
		free((void *)s_Buffer);
		s_Buffer = 0;
	}
	return 0;
}


/**
 * Adjust the volume either down (-1), up (1), or to the default (0).
 * Unmutes if mute was active before.
 */
void
FCEUD_SoundVolumeAdjust(int n)
{
	int soundvolume;
	g_config->getOption("SDL.Sound.Volume", &soundvolume);

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

	s_mute = 0;
	FCEUI_SetSoundVolume(soundvolume);
	g_config->setOption("SDL.Sound.Volume", soundvolume);

	FCEU_DispMessage("Sound volume %d.",0, soundvolume);
}

/**
 * Toggles the sound on or off.
 */
void
FCEUD_SoundToggle(void)
{
	if(s_mute) {
		int soundvolume;
		g_config->getOption("SDL.SoundVolume", &soundvolume);

		s_mute = 0;
		FCEUI_SetSoundVolume(soundvolume);
		FCEU_DispMessage("Sound mute off.",0);
	} else {
		s_mute = 1;
		FCEUI_SetSoundVolume(0);
		FCEU_DispMessage("Sound mute on.",0);
	}
}
