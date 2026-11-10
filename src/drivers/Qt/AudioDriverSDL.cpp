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

#include "Qt/AudioDriverSDL.h"

#if _SDL2
#include <SDL2/SDL.h>
#else
#include <SDL.h>
#endif

#include <cstdio>
#include <cstring>

//----------------------------------------------------------
AudioDriverSDL::AudioDriverSDL(void)
	: deviceOpen(false), subsystemInit(false)
{
}
//----------------------------------------------------------
AudioDriverSDL::~AudioDriverSDL(void)
{
	closeDevice();
}
//----------------------------------------------------------
std::string AudioDriverSDL::driverInfo(void) const
{
	const char *name = deviceOpen ? SDL_GetCurrentAudioDriver() : nullptr;

	if (name == nullptr)
	{
		return std::string();
	}
	return std::string("SDL ") + name;
}
//----------------------------------------------------------
/**
 * Callback from the SDL to get and play audio data.
 */
void AudioDriverSDL::sdlCallback(void *udata, uint8_t *stream, int len)
{
	// SDL holds its audio lock while calling this, same lock as lockBuffer()
	AudioDriverSDL *self = static_cast<AudioDriverSDL*>(udata);

	self->fillAudio(reinterpret_cast<int16_t*>(stream), len >> 1);
}
//----------------------------------------------------------
bool AudioDriverSDL::openDevice(unsigned int sampleRate, int periodSamples)
{
	SDL_AudioSpec spec;
	const char *driverName;

	memset(&spec, 0, sizeof(spec));

	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
	{
		puts(SDL_GetError());
		return false;
	}
	subsystemInit = true;

	spec.freq = sampleRate;
	spec.format = AUDIO_S16SYS;
	spec.channels = 1;
	spec.samples = periodSamples; // This must stay a power of two per SDL documentation
	spec.callback = sdlCallback;
	spec.userdata = this;

	if (SDL_OpenAudio(&spec, 0) < 0)
	{
		puts(SDL_GetError());
		return false;
	}
	deviceOpen = true;

	SDL_PauseAudio(0);

	driverName = SDL_GetCurrentAudioDriver();

	if ( driverName )
	{
		fprintf(stderr, "Loading SDL sound with %s driver...\n", driverName);
	}
	return true;
}
//----------------------------------------------------------
void AudioDriverSDL::closeDevice(void)
{
	if (deviceOpen)
	{
		SDL_CloseAudio();
		deviceOpen = false;
	}
	if (subsystemInit)
	{
		SDL_QuitSubSystem(SDL_INIT_AUDIO);
		subsystemInit = false;
	}
}
//----------------------------------------------------------
void AudioDriverSDL::lockBuffer(void)
{
	SDL_LockAudio();
}
//----------------------------------------------------------
void AudioDriverSDL::unlockBuffer(void)
{
	SDL_UnlockAudio();
}
//----------------------------------------------------------
void AudioDriverSDL::silence(int pause)
{
	SDL_PauseAudio(pause);
}
//----------------------------------------------------------
