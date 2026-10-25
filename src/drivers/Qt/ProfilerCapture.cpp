// ProfilerCapture.cpp
//
#ifdef __FCEU_PROFILER_ENABLE__

#include <string.h>

#include <QFile>

#include "../../profiler.h"
#include "Qt/ProfilerCapture.h"

namespace
{
	void putU8(QByteArray &b, uint8_t v)
	{
		b.append(static_cast<char>(v));
	}

	void putU32(QByteArray &b, uint32_t v)
	{
		char d[4];
		for (int i = 0; i < 4; i++){ d[i] = static_cast<char>((v >> (8 * i)) & 0xFF); }
		b.append(d, 4);
	}

	void putU64(QByteArray &b, uint64_t v)
	{
		char d[8];
		for (int i = 0; i < 8; i++){ d[i] = static_cast<char>((v >> (8 * i)) & 0xFF); }
		b.append(d, 8);
	}

	void putChunk(QByteArray &out, ProfilerCapture::ChunkType type, const QByteArray &payload)
	{
		putU8(out, type);
		putU32(out, static_cast<uint32_t>(payload.size()));
		out.append(payload);
	}
}
//----------------------------------------------------------------------------
ProfilerCaptureThread_t::ProfilerCaptureThread_t(QObject *parent)
	: QThread(parent)
{
}
//----------------------------------------------------------------------------
ProfilerCaptureThread_t::~ProfilerCaptureThread_t(void)
{
	stopCapture();
	wait();
}
//----------------------------------------------------------------------------
void ProfilerCaptureThread_t::stopCapture(void)
{
	requestInterruption();
}
//----------------------------------------------------------------------------
uint32_t ProfilerCaptureThread_t::internString(const char *str)
{
	if (str == nullptr)
	{
		str = "";
	}
	auto pit = stringPtrCache.find(str);
	if (pit != stringPtrCache.end())
	{
		return pit->second;
	}

	// Different pointers may hold identical text (e.g. __FILE__ across translation units)
	std::string text(str);
	uint32_t id;
	auto it = stringIds.find(text);
	if (it != stringIds.end())
	{
		id = it->second;
	}
	else
	{
		id = static_cast<uint32_t>(stringIds.size());
		stringIds[text] = id;

		QByteArray payload;
		putU32(payload, id);
		payload.append(text.data(), static_cast<int>(text.size()));
		putChunk(outBuf, ProfilerCapture::CHUNK_STRING, payload);
	}
	stringPtrCache[str] = id;
	return id;
}
//----------------------------------------------------------------------------
uint32_t ProfilerCaptureThread_t::internSite(int line, const char *file, const char *func, const char *comment)
{
	auto key = std::make_tuple(file, line, func, comment);
	auto it = siteIds.find(key);
	if (it != siteIds.end())
	{
		return it->second;
	}

	uint32_t fileId = internString(file);
	uint32_t funcId = internString(func);
	uint32_t commentId = internString(comment);
	uint32_t id = static_cast<uint32_t>(siteIds.size());
	siteIds[key] = id;

	QByteArray payload;
	putU32(payload, id);
	putU32(payload, static_cast<uint32_t>(line));
	putU32(payload, fileId);
	putU32(payload, funcId);
	putU32(payload, commentId);
	putChunk(outBuf, ProfilerCapture::CHUNK_SITE, payload);
	return id;
}
//----------------------------------------------------------------------------
void ProfilerCaptureThread_t::run(void)
{
	FCEU::profilerManager *mgr = FCEU::profilerManager::getInstance();

	if (mgr == nullptr)
	{
		emit captureError(tr("Profiler manager is not available."));
		return;
	}

	QFile file(filePath);

	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
	{
		emit captureError(tr("Failed to open capture file for writing: %1").arg(file.errorString()));
		return;
	}
	markerCount = 0;
	bytesWritten = 0;
	outBuf.clear();
	stringPtrCache.clear();
	stringIds.clear();
	siteIds.clear();
	announcedThreads.clear();

	FCEU::timeStampRecord startTs;
	startTs.readNew();

	outBuf.append(ProfilerCapture::FILE_MAGIC, sizeof(ProfilerCapture::FILE_MAGIC));
	putU32(outBuf, ProfilerCapture::FORMAT_VERSION);
	putU64(outBuf, FCEU::timeStampRecord::countFreq());
	putU64(outBuf, startTs.toCounts());

	bool writeOk = true;

	auto flushOutput = [&]() -> bool
	{
		if (outBuf.isEmpty())
		{
			return true;
		}
		bool ok = file.write(outBuf) == outBuf.size();
		bytesWritten += outBuf.size();
		outBuf.clear();
		if (!ok)
		{
			emit captureError(tr("Failed to write capture file: %1").arg(file.errorString()));
		}
		return ok;
	};

	auto onMarkers = [this](uint32_t threadId, const std::string &threadName,
			const FCEU::profileMarker *markers, size_t count)
	{
		auto ait = announcedThreads.find(threadId);
		if (ait == announcedThreads.end() || ait->second != threadName)
		{
			announcedThreads[threadId] = threadName;

			QByteArray payload;
			putU32(payload, threadId);
			payload.append(threadName.data(), static_cast<int>(threadName.size()));
			putChunk(outBuf, ProfilerCapture::CHUNK_THREAD, payload);
		}

		QByteArray payload;
		payload.reserve(static_cast<int>(8 + count * ProfilerCapture::MARKER_RECORD_SIZE));
		putU32(payload, threadId);
		putU32(payload, static_cast<uint32_t>(count));

		for (size_t i = 0; i < count; i++)
		{
			const FCEU::profileMarker &m = markers[i];
			uint32_t site = internSite(m.fileLineNum, m.fileName, m.funcName, m.comment);

			putU8(payload, (m.type == FCEU::profileMarker::MARKER_START) ? ProfilerCapture::MARKER_START : ProfilerCapture::MARKER_END);
			putU32(payload, site);
			putU64(payload, const_cast<FCEU::timeStampRecord&>(m.ts).toCounts());
		}
		putChunk(outBuf, ProfilerCapture::CHUNK_MARKERS, payload);
		markerCount += count;
	};

	mgr->setExternalCollector(true);
	mgr->setEnabled(true);

	while (writeOk && !isInterruptionRequested())
	{
		mgr->drainMarkers(onMarkers);
		writeOk = flushOutput();
		if (writeOk)
		{
			msleep(pollIntervalMs);
		}
	}

	// Stop producers, give in-flight writers time to finish, then empty every buffer
	mgr->setEnabled(false);
	msleep(pollIntervalMs * 2);
	mgr->drainMarkers(onMarkers, true);
	mgr->setExternalCollector(false);

	QByteArray footer;
	putU64(footer, markerCount);
	putChunk(outBuf, ProfilerCapture::CHUNK_FOOTER, footer);

	if (writeOk)
	{
		flushOutput();
	}
	file.close();
}
//----------------------------------------------------------------------------
#endif // __FCEU_PROFILER_ENABLE__
