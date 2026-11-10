// AudioDriverQt.h
//
#pragma once

#include "Qt/AudioDriverInterface.h"

// Audio output using QtMultimedia: QAudioOutput on Qt 5, QAudioSink on Qt 6.
class AudioDriverQt : public AudioDriverBase
{
	public:
		AudioDriverQt(void);
		~AudioDriverQt(void);

		int driver(void) const override { return AUDIO_DRIVER_QT; }
		std::string driverInfo(void) const override;
		void silence(int pause) override;

	protected:
		bool openDevice(unsigned int sampleRate, int periodSamples) override;
		void closeDevice(void) override;
		void lockBuffer(void) override;
		void unlockBuffer(void) override;

	private:
		struct Impl;
		Impl *impl;
};
