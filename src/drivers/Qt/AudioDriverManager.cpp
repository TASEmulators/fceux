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
/// \brief Sound output entry points, forwards to the selected audio driver.

#include "Qt/AudioDriverInterface.h"

#include "../../types.h"
#include "../../fceu.h"
#include "../../driver.h"
#include "common/configSys.h"
#include "Qt/dface.h"

#include <cstdio>

extern Config *g_config;

static AudioDriverBase *s_driver = nullptr;

/**
 * Initialize the audio subsystem with the driver selected in the config.
 */
int
InitSound()
{
	int sound = 0;
	int driverId = AudioDriverBase::AUDIO_DRIVER_SDL;

	g_config->getOption("SDL.Sound", &sound);
	if (!sound)
	{
		return 0;
	}
	g_config->getOption("SDL.AudioDriver", &driverId);

	KillSound();

	s_driver = AudioDriverBase::create(driverId);

	if (s_driver == nullptr)
	{
		fprintf(stderr, "Error: Audio driver %i is not available in this build.\n", driverId);
		return 0;
	}

	if (!s_driver->init())
	{
		delete s_driver;
		s_driver = nullptr;
		return 0;
	}
	return 1;
}

/**
 * Returns the size of the audio buffer.
 */
uint32
GetMaxSound(void)
{
	return s_driver ? s_driver->getMaxSound() : 0;
}

/**
 * Returns the amount of free space in the audio buffer.
 */
uint32
GetWriteSound(void)
{
	return s_driver ? s_driver->getWriteSound() : 0;
}

/**
 * Send a sound clip to the audio subsystem.
 */
void
WriteSound(int32 *buf,
           int Count)
{
	if (s_driver)
	{
		s_driver->write(buf, Count);
	}
}

/**
 * Pause (1) or unpause (0) the audio output.
 */
void
SilenceSound(int n)
{
	if (s_driver)
	{
		s_driver->silence(n);
	}
}

/**
 * Shut down the audio subsystem.
 */
int
KillSound(void)
{
	FCEUI_Sound(0);

	if (s_driver)
	{
		s_driver->kill();
		delete s_driver;
		s_driver = nullptr;
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

	AudioDriverBase::setMuted(false);
	g_config->setOption("SDL.Sound.Mute", false);

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
	FCEUD_MuteSoundOutput( !AudioDriverBase::isMuted() );
}

bool FCEUD_SoundIsMuted(void)
{
	return AudioDriverBase::isMuted();
}

void FCEUD_MuteSoundOutput( bool value )
{
	if (value != AudioDriverBase::isMuted())
	{
		g_config->setOption("SDL.Sound.Mute", value);

		if (value)
		{
			FCEU_DispMessage("Sound mute on.",0);
		}
		else
		{
			FCEU_DispMessage("Sound mute off.",0);
		}
	}
	AudioDriverBase::setMuted(value);
}

// This function is used by the GUI to mute sound when main window is not in focus.
void FCEUD_MuteSoundWindow( bool value )
{
	AudioDriverBase::setWindowMuted(value);
}
