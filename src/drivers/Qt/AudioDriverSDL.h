// AudioDriverSDL.h
//
#pragma once

#include "Qt/AudioDriverInterface.h"

class AudioDriverSDL : public AudioDriverBase
{
	public:
		AudioDriverSDL(void);
		~AudioDriverSDL(void);

		int driver(void) const override { return AUDIO_DRIVER_SDL; }
		std::string driverInfo(void) const override;
		void silence(int pause) override;

		// Output devices SDL can open with the current SDL audio driver
		static std::vector<AudioDeviceInfo> enumerateDevices(void);

	protected:
		bool openDevice(unsigned int sampleRate, int periodSamples,
				const std::string &deviceId) override;
		void closeDevice(void) override;
		void lockBuffer(void) override;
		void unlockBuffer(void) override;

	private:
		static void sdlCallback(void *udata, uint8_t *stream, int len);

		// SDL_AudioDeviceID, 0 when no device is open
		uint32_t sdlDevice;
		bool subsystemInit;
};
