// AudioDriverInterface.h
//
#pragma once
#include <stdint.h>

#include <atomic>
#include <string>
#include <vector>

// An audio output device a backend can open
struct AudioDeviceInfo
{
	// Backend specific identifier, saved in the config file
	std::string id;
	// Name to show the user
	std::string name;
};

// Common base class for the audio output backends (SDL, Qt).
//
// The base class owns everything that is independent of the audio API: the
// sample FIFO that the emulation thread fills, the pause/mute/noise gate logic
// applied by the device thread when draining it, the rate matching done in
// write(), and the user level mute state. A backend only has to open and close
// an output device that periodically calls fillAudio(), and provide the lock
// that excludes that device thread from the FIFO.
class AudioDriverBase
{
	public:
		enum AudioDriver
		{
			AUDIO_DRIVER_SDL = 0,
			AUDIO_DRIVER_QT
		};

		AudioDriverBase(void);
		virtual ~AudioDriverBase(void);

		// Which AudioDriver this is
		virtual int driver(void) const = 0;
		// Human readable driver name and version info, empty if not yet known
		virtual std::string driverInfo(void) const { return std::string(); }
		// Pause (non-zero) or unpause (0) the audio device
		virtual void silence(int pause) = 0;

		// Loads configuration, opens the device and configures the sound core.
		// Returns 1 on success, 0 on failure (sound is left shut down).
		int  init(void);
		// Closes the device and releases the FIFO.
		int  kill(void);
		// Queue samples from the emulation thread
		void write(int32_t *buf, int count);
		// Total FIFO size in samples
		uint32_t getMaxSound(void) const;
		// Free space in the FIFO in samples
		uint32_t getWriteSound(void) const;

		// User mute (persisted in the config file) and window focus mute.
		// These outlive any particular driver instance.
		static bool isMuted(void);
		static void setMuted(bool value);
		static void setWindowMuted(bool value);

		static bool isDriverAvailable(int driverId);
		// Creates the driver for driverId, nullptr if it is not available.
		static AudioDriverBase *create(int driverId);
		// Output devices driverId can open, does not include the system default
		static std::vector<AudioDeviceInfo> listDevices(int driverId);
		// Config option holding the device id selected for driverId. An empty
		// id selects the system default device.
		static const char *deviceConfigKey(int driverId);

	protected:
		// Open the device for mono signed 16 bit samples at sampleRate.
		// deviceId is the saved AudioDeviceInfo::id, empty for the system
		// default. If it can not be found the backend falls back to the
		// default device. periodSamples is the preferred number of samples
		// per device callback. On success the device must be running and
		// calling fillAudio() from its own thread, with the backend lock held.
		virtual bool openDevice(unsigned int sampleRate, int periodSamples,
				const std::string &deviceId) = 0;
		virtual void closeDevice(void) = 0;
		// Mutual exclusion between the device thread and write()
		virtual void lockBuffer(void) = 0;
		virtual void unlockBuffer(void) = 0;

		// Device thread entry point, fills out with sample count samples.
		// The backend lock must be held by the caller.
		void fillAudio(int16_t *out, int count);

	private:
		bool waitForSpace(int &waitCount);

		int *buffer;
		unsigned int bufferSize;
		unsigned int bufferSize25;
		unsigned int bufferSize50;
		unsigned int bufferSize75;
		unsigned int bufferRead;
		unsigned int bufferWrite;
		std::atomic<unsigned int> bufferIn;
		int16_t lastSample;
		int skipCounter;
		bool fillInit;
		bool noiseGateActive;
		double noiseGate;
		double noiseGateRate;
};
