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
	: sdlDevice(0), subsystemInit(false)
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
	const char *name = (sdlDevice != 0) ? SDL_GetCurrentAudioDriver() : nullptr;

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
std::vector<AudioDeviceInfo> AudioDriverSDL::enumerateDevices(void)
{
	std::vector<AudioDeviceInfo> devices;

	// Reference counted, safe to do while a device is open
	if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0)
	{
		puts(SDL_GetError());
		return devices;
	}
	const int count = SDL_GetNumAudioDevices(0);

	for (int i=0; i<count; i++)
	{
		const char *name = SDL_GetAudioDeviceName(i, 0);

		if (name != nullptr)
		{
			AudioDeviceInfo info;
			info.id = name;
			info.name = name;
			devices.push_back(info);
		}
	}
	SDL_QuitSubSystem(SDL_INIT_AUDIO);

	return devices;
}
//----------------------------------------------------------
bool AudioDriverSDL::openDevice(unsigned int sampleRate, int periodSamples,
		const std::string &deviceId)
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

	if (!deviceId.empty())
	{
		sdlDevice = SDL_OpenAudioDevice(deviceId.c_str(), 0, &spec, nullptr, 0);

		if (sdlDevice == 0)
		{
			fprintf(stderr, "Warning: SDL could not open audio device '%s' (%s), using the default device.\n",
					deviceId.c_str(), SDL_GetError());
		}
	}
	if (sdlDevice == 0)
	{
		sdlDevice = SDL_OpenAudioDevice(nullptr, 0, &spec, nullptr, 0);
	}
	if (sdlDevice == 0)
	{
		puts(SDL_GetError());
		return false;
	}

	SDL_PauseAudioDevice(sdlDevice, 0);

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
	if (sdlDevice != 0)
	{
		SDL_CloseAudioDevice(sdlDevice);
		sdlDevice = 0;
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
	SDL_LockAudioDevice(sdlDevice);
}
//----------------------------------------------------------
void AudioDriverSDL::unlockBuffer(void)
{
	SDL_UnlockAudioDevice(sdlDevice);
}
//----------------------------------------------------------
void AudioDriverSDL::silence(int pause)
{
	SDL_PauseAudioDevice(sdlDevice, pause);
}
//----------------------------------------------------------
