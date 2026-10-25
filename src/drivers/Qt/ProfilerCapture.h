// ProfilerCapture.h
//
// Collector thread that drains profiler markers from all threads and streams
// them to a binary capture file.
//
// Capture file layout (all integers little-endian):
//
//   Header (28 bytes)
//     char[8]  magic         "FCEUPROF"
//     u32      version       ProfilerCapture::FORMAT_VERSION
//     u64      countFreq     timestamp counts per second
//     u64      startCounts   timestamp (counts) when the capture started
//
//   Followed by a sequence of chunks until EOF:
//     u8       chunkType
//     u32      payloadSize
//     u8[]     payload
//
//   CHUNK_STRING   : u32 stringId, utf8 text (rest of payload)
//   CHUNK_SITE     : u32 siteId, u32 line, u32 fileStrId, u32 funcStrId, u32 commentStrId
//   CHUNK_THREAD   : u32 threadId, utf8 name (rest of payload). May repeat if a thread is renamed.
//   CHUNK_MARKERS  : u32 threadId, u32 count, then count records of:
//                        u8 type (MARKER_START/MARKER_END), u32 siteId, u64 timestamp (counts)
//   CHUNK_FOOTER   : u64 totalMarkerCount
//
// String and site definitions always precede the first marker block that uses them.
// A truncated capture (e.g. after a crash) is valid up to the last complete chunk.

#pragma once

#ifdef __FCEU_PROFILER_ENABLE__

#include <stdint.h>
#include <atomic>
#include <map>
#include <string>
#include <tuple>
#include <unordered_map>

#include <QThread>
#include <QString>
#include <QByteArray>

namespace ProfilerCapture
{
	static constexpr char     FILE_MAGIC[8] = { 'F','C','E','U','P','R','O','F' };
	static constexpr uint32_t FORMAT_VERSION = 1;
	static constexpr size_t   HEADER_SIZE = 28;
	static constexpr size_t   MARKER_RECORD_SIZE = 13;

	enum ChunkType : uint8_t
	{
		CHUNK_STRING  = 1,
		CHUNK_SITE    = 2,
		CHUNK_THREAD  = 3,
		CHUNK_MARKERS = 4,
		CHUNK_FOOTER  = 5,
	};

	enum MarkerType : uint8_t
	{
		MARKER_START = 0,
		MARKER_END   = 1,
	};
}

class ProfilerCaptureThread_t : public QThread
{
	Q_OBJECT

public:
	ProfilerCaptureThread_t(QObject *parent = nullptr);
	~ProfilerCaptureThread_t(void);

	void setFilePath(const QString &path){ filePath = path; }
	void setPollIntervalMs(unsigned int ms){ pollIntervalMs = ms; }

	uint64_t getMarkerCount(void) const { return markerCount; }
	uint64_t getBytesWritten(void) const { return bytesWritten; }

	// Ask the thread to flush remaining markers, finalize the file and exit.
	void stopCapture(void);

signals:
	void captureError(const QString &msg);

protected:
	void run(void) override;

private:
	uint32_t internString(const char *str);
	uint32_t internSite(int line, const char *file, const char *func, const char *comment);

	QString filePath;
	unsigned int pollIntervalMs = 10;

	std::atomic<uint64_t> markerCount{0};
	std::atomic<uint64_t> bytesWritten{0};

	// Only accessed from the collector thread
	QByteArray outBuf;
	std::unordered_map<const char*, uint32_t> stringPtrCache;
	std::unordered_map<std::string, uint32_t> stringIds;
	std::map<std::tuple<const char*, int, const char*, const char*>, uint32_t> siteIds;
	std::unordered_map<uint32_t, std::string> announcedThreads;
};

#endif // __FCEU_PROFILER_ENABLE__
