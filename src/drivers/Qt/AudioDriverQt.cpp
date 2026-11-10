// AudioDriverQt.cpp
//
#include "Qt/AudioDriverQt.h"

#include <QtGlobal>
#include <QCoreApplication>
#include <QIODevice>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QSysInfo>
#include <QThread>
#include <QAudio>
#include <QAudioFormat>

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
#include <QAudioDevice>
#include <QAudioSink>
#include <QMediaDevices>
typedef QAudioSink   QtAudioSinkType;
typedef QAudioDevice QtAudioDeviceType;
#else
#include <QAudioDeviceInfo>
#include <QAudioOutput>
typedef QAudioOutput     QtAudioSinkType;
typedef QAudioDeviceInfo QtAudioDeviceType;
#endif

#include <cstdio>
#include <cstring>
#include <functional>
#include <mutex>
#include <vector>

namespace
{

// Pull mode source for the Qt sink. readData() is invoked from the audio
// thread, whenever the sink needs more samples.
class AudioPullDevice : public QIODevice
{
	public:
		typedef std::function<void(int16_t*, int)> FillFunc;

		AudioPullDevice(FillFunc f, int numChannels)
			: fill(f), channels(numChannels)
		{
		}

		bool isSequential(void) const override { return true; }

		// Always report data as available so a sink never concludes the
		// source has run dry, silence is generated when the FIFO is empty.
		qint64 bytesAvailable(void) const override
		{
			return 1 << 20;
		}

	protected:
		qint64 readData(char *data, qint64 maxlen) override
		{
			const int bytesPerFrame = static_cast<int>(sizeof(int16_t)) * channels;
			const int frames = static_cast<int>(maxlen / bytesPerFrame);

			if (frames <= 0)
			{
				return 0;
			}
			if (static_cast<int>(scratch.size()) < frames)
			{
				scratch.resize(frames);
			}
			fill(scratch.data(), frames);

			if (channels == 1)
			{
				memcpy(data, scratch.data(), frames * sizeof(int16_t));
			}
			else
			{
				int16_t frame[8];
				const int n = (channels < 8) ? channels : 8;

				for (int i=0; i<frames; i++)
				{
					for (int c=0; c<n; c++)
					{
						frame[c] = scratch[i];
					}
					memcpy(data + i * bytesPerFrame, frame, bytesPerFrame);
				}
			}
			return static_cast<qint64>(frames) * bytesPerFrame;
		}

		qint64 writeData(const char *, qint64) override
		{
			return -1;
		}

	private:
		FillFunc fill;
		int channels;
		std::vector<int16_t> scratch;
};

} // namespace

struct AudioDriverQt::Impl
{
	Impl(void) : thread(nullptr), context(nullptr), sink(nullptr), source(nullptr) {}

	std::mutex mtx;
	QThread *thread;
	// Lives in the audio thread, used as the target of calls into it
	QObject *context;
	// Both of these are only touched from the audio thread
	QtAudioSinkType *sink;
	AudioPullDevice *source;
	std::string description;
};

//----------------------------------------------------------
AudioDriverQt::AudioDriverQt(void)
	: impl(new Impl())
{
}
//----------------------------------------------------------
AudioDriverQt::~AudioDriverQt(void)
{
	closeDevice();
	delete impl;
	impl = nullptr;
}
//----------------------------------------------------------
std::string AudioDriverQt::driverInfo(void) const
{
	if (impl->thread == nullptr)
	{
		return std::string();
	}
	std::string info = std::string("Qt Multimedia ") + qVersion();

	if (!impl->description.empty())
	{
		info += " (" + impl->description + ")";
	}
	return info;
}
//----------------------------------------------------------
bool AudioDriverQt::openDevice(unsigned int sampleRate, int periodSamples)
{
	if (QCoreApplication::instance() == nullptr)
	{
		fprintf(stderr, "Error: Qt audio driver requires a running Qt application.\n");
		return false;
	}

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
	QtAudioDeviceType device = QMediaDevices::defaultAudioOutput();
#else
	QtAudioDeviceType device = QtAudioDeviceType::defaultOutputDevice();
#endif

	if (device.isNull())
	{
		fprintf(stderr, "Error: Qt audio driver could not find a default audio output device.\n");
		return false;
	}

	QAudioFormat format;
	format.setSampleRate(static_cast<int>(sampleRate));
	format.setChannelCount(2);
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
	format.setSampleFormat(QAudioFormat::Int16);
#else
	format.setCodec("audio/pcm");
	format.setSampleSize(16);
	format.setSampleType(QAudioFormat::SignedInt);
	format.setByteOrder(QSysInfo::ByteOrder == QSysInfo::BigEndian ?
			QAudioFormat::BigEndian : QAudioFormat::LittleEndian);
#endif

	// The emulator produces mono samples. Prefer a stereo stream with the
	// samples duplicated, fall back to a mono stream if the device refuses.
	if (!device.isFormatSupported(format))
	{
		QAudioFormat mono(format);
		mono.setChannelCount(1);

		if (device.isFormatSupported(mono))
		{
			format = mono;
		}
		else
		{
			fprintf(stderr, "Warning: Qt audio device does not report support for %u Hz 16-bit audio, trying anyway.\n", sampleRate);
		}
	}
	const int channels = format.channelCount();
	// Keep the device side buffer small, the FIFO in the base class provides
	// the configured latency.
	const int sinkBufferBytes = periodSamples * 2 * channels * static_cast<int>(sizeof(int16_t));

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
	impl->description = device.description().toStdString();
#else
	impl->description = device.deviceName().toStdString();
#endif

	impl->thread = new QThread();
	impl->thread->setObjectName("FceuxAudio");
	impl->thread->start();

	impl->context = new QObject();
	impl->context->moveToThread(impl->thread);

	bool ok = false;
	Impl *p = impl;

	QMetaObject::invokeMethod(impl->context, [this, p, device, format, channels, sinkBufferBytes, &ok]()
	{
		p->source = new AudioPullDevice([this, p](int16_t *out, int count)
		{
			std::lock_guard<std::mutex> lock(p->mtx);
			fillAudio(out, count);
		}, channels);
		// Unbuffered: otherwise QIODevice reads ahead 16KB at a time, which
		// is more than the whole FIFO and drains it on every pull.
		p->source->open(QIODevice::ReadOnly | QIODevice::Unbuffered);

		p->sink = new QtAudioSinkType(device, format);
#if QT_VERSION < QT_VERSION_CHECK(6,0,0)
		// Match the stream role SDL uses, Qt defaults to "music".
		p->sink->setCategory("game");
#endif
		// Qt 6: QAudioSink has no role setting, the backend always labels the
		// stream "music". Environment overrides can not change that for the
		// PulseAudio backend (PULSE_PROP_* only applies to the connection).
		p->sink->setBufferSize(sinkBufferBytes);
		p->sink->start(p->source);

		if ((p->sink->error() != QAudio::NoError) || (p->sink->state() == QAudio::StoppedState))
		{
			fprintf(stderr, "Error: Qt audio output failed to start (error %i).\n", static_cast<int>(p->sink->error()));
			p->sink->stop();
			delete p->sink;
			p->sink = nullptr;
			delete p->source;
			p->source = nullptr;
			return;
		}
		ok = true;
	}, Qt::BlockingQueuedConnection);

	if (!ok)
	{
		return false;
	}

	fprintf(stderr, "Loading Qt sound with %s device (%s)...\n", impl->description.c_str(),
			(channels == 1) ? "mono" : "stereo");
	return true;
}
//----------------------------------------------------------
void AudioDriverQt::closeDevice(void)
{
	if (impl->thread == nullptr)
	{
		return;
	}
	Impl *p = impl;

	QMetaObject::invokeMethod(impl->context, [p]()
	{
		if (p->sink)
		{
			p->sink->stop();
			delete p->sink;
			p->sink = nullptr;
		}
		delete p->source;
		p->source = nullptr;
	}, Qt::BlockingQueuedConnection);

	// The context object is destroyed by the audio thread as it exits
	QObject::connect(impl->thread, &QThread::finished, impl->context, &QObject::deleteLater);
	impl->thread->quit();
	impl->thread->wait();

	delete impl->thread;
	impl->thread = nullptr;
	impl->context = nullptr;
}
//----------------------------------------------------------
void AudioDriverQt::lockBuffer(void)
{
	impl->mtx.lock();
}
//----------------------------------------------------------
void AudioDriverQt::unlockBuffer(void)
{
	impl->mtx.unlock();
}
//----------------------------------------------------------
void AudioDriverQt::silence(int pause)
{
	if (impl->thread == nullptr)
	{
		return;
	}
	Impl *p = impl;

	QMetaObject::invokeMethod(impl->context, [p, pause]()
	{
		if (p->sink == nullptr)
		{
			return;
		}
		if (pause)
		{
			p->sink->suspend();
		}
		else
		{
			p->sink->resume();
		}
	}, Qt::QueuedConnection);
}
//----------------------------------------------------------
