// AudioDriverInterface.cpp
//
#include "Qt/AudioDriverInterface.h"
#include "Qt/AudioDriverSDL.h"
#ifdef FCEUX_HAS_QT_AUDIO
#include "Qt/AudioDriverQt.h"
#endif

#include "../../types.h"
#include "../../fceu.h"
#include "../../driver.h"
#include "common/configSys.h"
#include "utils/memory.h"
#include "Qt/main.h"
#include "Qt/input.h"
#include "Qt/nes_shm.h"
#include "Qt/throttle.h"

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <thread>

extern Config *g_config;
extern bool turbo;
extern int EmulationPaused;
extern double g_fpsScale;

static std::atomic<bool> s_mute(false);
static std::atomic<bool> s_windowMute(false);
static std::atomic<bool> s_chimeRequest(false);

static const unsigned int supportedSampleRates[] = { 11025, 22050, 44100, 48000, 96000, 0 };

// Short rising arpeggio (C6 E6 G6 C7), synthesized so no sound asset has to be shipped.
static std::vector<int16_t> synthesizeChime(unsigned int sampleRate)
{
	const double notes[] = { 1046.5, 1318.5, 1568.0, 2093.0 };
	const double noteStart = 0.09;
	const double noteLength = 0.35;
	const double pi = 3.14159265358979323846;
	const size_t total = static_cast<size_t>(sampleRate * (noteStart * 3 + noteLength));
	std::vector<double> mix(total, 0.0);
	std::vector<int16_t> samples(total);

	for (int n = 0; n < 4; n++)
	{
		const size_t first = static_cast<size_t>(sampleRate * noteStart * n);
		const size_t count = static_cast<size_t>(sampleRate * noteLength);

		for (size_t i = 0; i < count && (first + i) < total; i++)
		{
			const double t = static_cast<double>(i) / sampleRate;
			const double attack = (t < 0.003) ? (t / 0.003) : 1.0;
			const double env = attack * std::exp(-t * 9.0);
			const double tone = std::sin(2.0 * pi * notes[n] * t) + 0.3 * std::sin(4.0 * pi * notes[n] * t);

			mix[first + i] += env * tone;
		}
	}

	for (size_t i = 0; i < total; i++)
	{
		double v = mix[i] * 0.2;

		if (v > 1.0) { v = 1.0; }
		if (v < -1.0) { v = -1.0; }

		samples[i] = static_cast<int16_t>(v * 32767.0);
	}
	return samples;
}

//----------------------------------------------------------
AudioDriverBase::AudioDriverBase(void)
	: buffer(nullptr), bufferSize(0), bufferSize25(0), bufferSize50(0), bufferSize75(0),
	  bufferRead(0), bufferWrite(0), bufferIn(0), lastSample(0), skipCounter(0),
	  fillInit(true), noiseGateActive(true), noiseGate(0.0), noiseGateRate(0.010),
	  chimePos(0)
{
}
//----------------------------------------------------------
AudioDriverBase::~AudioDriverBase(void)
{
	// The device can not be stopped from here since the derived part is
	// already gone, callers must kill() the driver before deleting it.
	if (buffer)
	{
		free(buffer);
		buffer = nullptr;
	}
}
//----------------------------------------------------------
bool AudioDriverBase::isMuted(void)
{
	return s_mute;
}
//----------------------------------------------------------
void AudioDriverBase::setMuted(bool value)
{
	s_mute = value;
}
//----------------------------------------------------------
void AudioDriverBase::setWindowMuted(bool value)
{
	s_windowMute = value;
}
//----------------------------------------------------------
void AudioDriverBase::playChime(void)
{
	s_chimeRequest = true;
}
//----------------------------------------------------------
bool AudioDriverBase::isDriverAvailable(int driverId)
{
	switch (driverId)
	{
		case AUDIO_DRIVER_SDL:
			return true;
#ifdef FCEUX_HAS_QT_AUDIO
		case AUDIO_DRIVER_QT:
			return true;
#endif
		default:
			break;
	}
	return false;
}
//----------------------------------------------------------
AudioDriverBase *AudioDriverBase::create(int driverId)
{
	switch (driverId)
	{
		case AUDIO_DRIVER_SDL:
			return new AudioDriverSDL();
#ifdef FCEUX_HAS_QT_AUDIO
		case AUDIO_DRIVER_QT:
			return new AudioDriverQt();
#endif
		default:
			break;
	}
	return nullptr;
}
//----------------------------------------------------------
std::vector<AudioDeviceInfo> AudioDriverBase::listDevices(int driverId)
{
	switch (driverId)
	{
		case AUDIO_DRIVER_SDL:
			return AudioDriverSDL::enumerateDevices();
#ifdef FCEUX_HAS_QT_AUDIO
		case AUDIO_DRIVER_QT:
			return AudioDriverQt::enumerateDevices();
#endif
		default:
			break;
	}
	return std::vector<AudioDeviceInfo>();
}
//----------------------------------------------------------
// SDL and Qt name the same device differently, so each keeps its own selection
const char *AudioDriverBase::deviceConfigKey(int driverId)
{
	switch (driverId)
	{
		case AUDIO_DRIVER_QT:
			return "SDL.AudioDevice.Qt";
		case AUDIO_DRIVER_SDL:
		default:
			break;
	}
	return "SDL.AudioDevice.SDL";
}
//----------------------------------------------------------
uint32_t AudioDriverBase::getMaxSound(void) const
{
	return bufferSize;
}
//----------------------------------------------------------
uint32_t AudioDriverBase::getWriteSound(void) const
{
	return bufferSize - bufferIn;
}
//----------------------------------------------------------
int AudioDriverBase::init(void)
{
	int i, soundrate, soundbufsize, soundvolume, soundtrianglevolume, soundsquare1volume, soundsquare2volume, soundnoisevolume, soundpcmvolume, soundq;
	bool mute = false;
	int frmRateSampleAdj = 0;
	int samplesPerFrame;
	int periodSamples;
	std::string deviceId;
	bool sampleRateIsSupported = false;

	// load configuration variables
	g_config->getOption("SDL.Sound.Mute", &mute);
	g_config->getOption("SDL.Sound.Rate", &soundrate);
	g_config->getOption("SDL.Sound.BufSize", &soundbufsize);
	g_config->getOption("SDL.Sound.Volume", &soundvolume);
	g_config->getOption("SDL.Sound.Quality", &soundq);
	g_config->getOption("SDL.Sound.TriangleVolume", &soundtrianglevolume);
	g_config->getOption("SDL.Sound.Square1Volume", &soundsquare1volume);
	g_config->getOption("SDL.Sound.Square2Volume", &soundsquare2volume);
	g_config->getOption("SDL.Sound.NoiseVolume", &soundnoisevolume);
	g_config->getOption("SDL.Sound.PCMVolume", &soundpcmvolume);
	g_config->getOption(deviceConfigKey(driver()), &deviceId);

	s_mute = mute;

	i = 0;
	while (supportedSampleRates[i] != 0)
	{
		if ( static_cast<unsigned int>(soundrate) == supportedSampleRates[i])
		{
			sampleRateIsSupported = true;
			break;
		}
		i++;
	}

	if (!sampleRateIsSupported)
	{
		printf("Error: Audio Sample Rate %i is either invalid or not supported, reverting to default of 44100\n", soundrate);
		soundrate = 44100;
		g_config->setOption("SDL.Sound.Rate", soundrate);
	}

	if ( (soundbufsize < 15) || (soundbufsize > 200) )
	{
		printf("Error: Audio Buffer Size of %i ms is invalid, reverting to default of 128\n", soundbufsize);
		soundbufsize = 128;
		g_config->setOption("SDL.Sound.BufSize", soundbufsize);
	}

	periodSamples = 512; // Backends may require a power of two
	samplesPerFrame = (int)( ( (double)soundrate / getBaseFrameRate() ) );

	if ( samplesPerFrame >= 1024 )
	{
		periodSamples = 1024;
	}

	bufferSize = soundbufsize * soundrate / 1000;

	// For safety, set a bare minimum:
	if (bufferSize < static_cast<unsigned int>(periodSamples * 4))
	{
		bufferSize = periodSamples * 4;
	}
	bufferSize25 =    bufferSize/4;
	bufferSize50 =    bufferSize/2;
	bufferSize75 = (3*bufferSize)/4;

	noiseGate = 0.0;
	noiseGateRate = 1.0 / (double)periodSamples;
	noiseGateActive = true;
	fillInit = true;
	lastSample = 0;
	skipCounter = 0;

	buffer = (int *)FCEU_dmalloc(sizeof(int) * bufferSize);

	if (!buffer)
	{
		return 0;
	}
	bufferRead = bufferWrite = 0;
	bufferIn = 0;

	// Must be ready before the device thread starts calling fillAudio()
	chimeSamples = synthesizeChime(static_cast<unsigned int>(soundrate));
	chimePos = chimeSamples.size();
	s_chimeRequest = false;

	if (!openDevice(static_cast<unsigned int>(soundrate), periodSamples, deviceId))
	{
		kill();
		return 0;
	}

	frmRateSampleAdj = (int)( ( ((double)soundrate) * getFrameRateAdjustmentRatio()) - ((double)soundrate) );

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
//----------------------------------------------------------
int AudioDriverBase::kill(void)
{
	FCEUI_Sound(0);
	closeDevice();
	if (buffer)
	{
		free((void *)buffer);
		buffer = nullptr;
	}
	bufferIn = 0;
	return 0;
}
//----------------------------------------------------------
void AudioDriverBase::fillAudio(int16_t *out, int len)
{
	fillStream(out, len);
	mixChime(out, len);
}
//----------------------------------------------------------
// Adds the notification chime on top of the emulator output. This happens
// after the FIFO is drained so the chime is not part of the emulated sound
// stream (e.g. what is sent to nes_shm) and it is not delayed by the FIFO.
void AudioDriverBase::mixChime(int16_t *out, int len)
{
	if ( s_chimeRequest.exchange(false) )
	{
		chimePos = 0;
	}

	if ( chimePos >= chimeSamples.size() )
	{
		return;
	}

	if ( s_windowMute || s_mute )
	{
		chimePos = chimeSamples.size();
		return;
	}

	for (int i = 0; i < len && chimePos < chimeSamples.size(); i++, chimePos++)
	{
		int v = out[i] + chimeSamples[chimePos];

		if (v > 32767) { v = 32767; }
		if (v < -32768) { v = -32768; }

		out[i] = static_cast<int16_t>(v);
	}
}
//----------------------------------------------------------
void AudioDriverBase::fillStream(int16_t *out, int len)
{
	int16_t *tmps = out;
	bool mute;

	if ( bufferIn > bufferSize25 )
	{
		fillInit = false;
	}
	// If emulation is paused:
	// 1. preserve sound buffer as is
	// 2. fade from last known sample to zero
	// 3. activate noise gate to avoid popping when coming out of pause.
	if ( EmulationPaused || fillInit )
	{
		while ( len )
		{
			if ( lastSample > 0 )
			{
				lastSample--;
			}
			else if ( lastSample < 0 )
			{
				lastSample++;
			}
			*tmps = lastSample;
			tmps++;
			len--;
		}
		noiseGate = 0.0;
		noiseGateActive = true;
		return;
	}
	mute = EmulationPaused || s_windowMute || s_mute;

	if ( mute || noiseGateActive )
	{
		// This noise gate helps avoid abrupt snaps in audio
		// when pausing emulation.
		while (len)
		{
			if (mute)
			{
				noiseGate -= noiseGateRate;

				if ( noiseGate < 0.0 )
				{
					noiseGate = 0.0;
				}
				noiseGateActive = true;
			}
			else
			{
				if ( bufferIn )
				{
					noiseGate += noiseGateRate;

					if ( noiseGate > 1.0 )
					{
						noiseGate = 1.0;
						noiseGateActive = false;
					}
				}
			}
			if (bufferIn)
			{
				lastSample = buffer[bufferRead] * noiseGate;
				bufferRead = (bufferRead + 1) % bufferSize;
				bufferIn--;

				*tmps = lastSample * noiseGate;
			}
			else
			{
				// Retain last known sample value, helps avoid clicking
				// noise when sound system is starved of audio data.
				*tmps = lastSample * noiseGate;
			}

			tmps++;
			len--;
		}
	}
	else
	{
		while (len)
		{
			if (bufferIn)
			{
				lastSample = buffer[bufferRead];
				bufferRead = (bufferRead + 1) % bufferSize;
				bufferIn--;
			}
			else
			{
				// Retain last known sample value, helps avoid clicking
				// noise when sound system is starved of audio data.
				nes_shm->sndBuf.starveCounter++;
			}

			nes_shm->push_sound_sample( lastSample );

			*tmps = lastSample;
			tmps++;
			len--;
		}
	}
}
//----------------------------------------------------------
// Called with the buffer lock released. Returns false if the device
// stopped draining the buffer.
bool AudioDriverBase::waitForSpace(int &waitCount)
{
	while (bufferIn == bufferSize)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
		waitCount++;

		if ( waitCount > 1000 )
		{
			printf("Error: Sound sink is not draining... Breaking out of audio loop to prevent lockup.\n");
			return false;
		}
	}
	return true;
}
//----------------------------------------------------------
void AudioDriverBase::write(int32_t *buf, int count)
{
	int uflowMode = 0;
	int ovrFlowSkip = 1;
	int udrFlowDup  = 1;

	if ( !buffer )
	{
		return;
	}

	if ( (NoWaiting & 0x01) || turbo )
	{	// During Turbo mode, don't bother with sound as
		// overflowing the audio buffer can cause delays.
		return;
	}

	if ( g_fpsScale >= 0.99995 )
	{
		uflowMode = 0;
		ovrFlowSkip = (int)(g_fpsScale * 1000);

		if ( bufferIn >= bufferSize50 )
		{
			ovrFlowSkip += 1;
		}
	}
	else
	{
		udrFlowDup = (int)(1.0 / g_fpsScale);

		if ( udrFlowDup < 1 )
		{
			udrFlowDup = 1;
		}
		if ( bufferIn < bufferSize50 )
		{
			udrFlowDup++;
		}
		else if ( bufferIn > bufferSize75 )
		{
			udrFlowDup--;
		}
		uflowMode = (udrFlowDup > 1);
	}

	if (EmulationPaused == 0)
	{
		int waitCount = 0;

		if ( uflowMode )
		{	// Underflow mode
			lockBuffer();
			while (count)
			{
				if ( bufferIn == bufferSize )
				{
					unlockBuffer();
					if ( !waitForSpace(waitCount) )
					{
						return;
					}
					lockBuffer();
				}

				for (int i=0; i<udrFlowDup; i++)
				{
					buffer[bufferWrite] = *buf;
					bufferWrite = (bufferWrite + 1) % bufferSize;

					bufferIn++;
				}

				count--;
				buf++;
			}
			unlockBuffer();
		}
		else if ( ovrFlowSkip <= 1000 )
		{	// Perfect one to one realtime
			skipCounter = 0;

			lockBuffer();
			while (count)
			{
				if (bufferIn == bufferSize)
				{
					unlockBuffer();
					if ( !waitForSpace(waitCount) )
					{
						return;
					}
					lockBuffer();
				}

				buffer[bufferWrite] = *buf;
				count--;
				bufferWrite = (bufferWrite + 1) % bufferSize;

				bufferIn++;
				buf++;
			}
			unlockBuffer();
		}
		else
		{	// Overflow mode
			lockBuffer();
			while (count)
			{
				if (bufferIn == bufferSize)
				{
					unlockBuffer();
					if ( !waitForSpace(waitCount) )
					{
						return;
					}
					lockBuffer();
				}

				if ( skipCounter >= ovrFlowSkip )
				{
					buffer[bufferWrite] = *buf;
					bufferWrite = (bufferWrite + 1) % bufferSize;

					bufferIn++;

					skipCounter -= ovrFlowSkip;
				}
				skipCounter = (skipCounter+1000);

				count--;
				buf++;
			}
			unlockBuffer();
		}
	}
}
//----------------------------------------------------------
