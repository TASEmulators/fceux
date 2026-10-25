// ProfilerCaptureFormat.h
//
// Constants of the binary profiler capture file format. Shared by the capture
// writer (ProfilerCapture.h) and the reader (ProfilerCaptureFile.h); the layout is
// documented in ProfilerCapture.h.

#pragma once

#include <stdint.h>
#include <stddef.h>

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
