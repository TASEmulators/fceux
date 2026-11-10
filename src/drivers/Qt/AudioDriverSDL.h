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

	protected:
		bool openDevice(unsigned int sampleRate, int periodSamples) override;
		void closeDevice(void) override;
		void lockBuffer(void) override;
		void unlockBuffer(void) override;

	private:
		static void sdlCallback(void *udata, uint8_t *stream, int len);

		bool deviceOpen;
		bool subsystemInit;
};
