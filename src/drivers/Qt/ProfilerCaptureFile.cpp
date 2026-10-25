// ProfilerCaptureFile.cpp
//

#include <string.h>
#include <algorithm>
#include <unordered_map>

#include <QFile>

#include "Qt/ProfilerCaptureFormat.h"
#include "Qt/ProfilerCaptureFile.h"

namespace
{
	uint32_t rd32(const unsigned char *p)
	{
		return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
			(static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
	}

	uint64_t rd64(const unsigned char *p)
	{
		return static_cast<uint64_t>(rd32(p)) | (static_cast<uint64_t>(rd32(p + 4)) << 32);
	}

	struct OpenBlock
	{
		uint64_t start;
		uint32_t site;
	};
}
//----------------------------------------------------------------------------
bool profilerLoadCapture(const QString &path, ProfilerCaptureData &out, QString &err,
	const std::function<bool(int)> &progress)
{
	using namespace ProfilerCapture;

	QFile file(path);

	if (!file.open(QIODevice::ReadOnly))
	{
		err = file.errorString();
		return false;
	}

	unsigned char hdr[HEADER_SIZE];

	if (file.read(reinterpret_cast<char*>(hdr), HEADER_SIZE) != static_cast<qint64>(HEADER_SIZE) ||
		memcmp(hdr, FILE_MAGIC, sizeof(FILE_MAGIC)) != 0)
	{
		err = "Not a profiler capture file.";
		return false;
	}
	out = ProfilerCaptureData();
	out.version = rd32(hdr + 8);
	out.countFreq = rd64(hdr + 12);
	if (out.countFreq == 0)
	{
		out.countFreq = 1;
	}
	if (out.version > FORMAT_VERSION)
	{
		err = QString("Unsupported capture file version %1.").arg(out.version);
		return false;
	}

	std::vector<QString> strings;
	std::unordered_map<uint32_t, size_t> threadIndex;
	std::vector<std::vector<OpenBlock>> stacks;
	uint64_t minTs = UINT64_MAX, maxTs = 0;
	const qint64 fileSize = file.size();
	int lastPercent = -1;
	QByteArray payload;

	auto threadFor = [&](uint32_t id) -> size_t
	{
		auto it = threadIndex.find(id);
		if (it != threadIndex.end())
		{
			return it->second;
		}
		size_t idx = out.threads.size();
		threadIndex[id] = idx;
		out.threads.emplace_back();
		out.threads.back().id = id;
		stacks.emplace_back();
		return idx;
	};

	while (true)
	{
		unsigned char ch[5];
		qint64 n = file.read(reinterpret_cast<char*>(ch), sizeof(ch));

		if (n == 0)
		{
			break;
		}
		if (n < static_cast<qint64>(sizeof(ch)))
		{
			out.truncated = true;
			break;
		}
		const uint8_t type = ch[0];
		const uint32_t size = rd32(ch + 1);

		if (static_cast<qint64>(size) > fileSize - file.pos())
		{
			out.truncated = true;
			break;
		}
		payload.resize(static_cast<int>(size));
		if (file.read(payload.data(), size) != static_cast<qint64>(size))
		{
			out.truncated = true;
			break;
		}
		const unsigned char *p = reinterpret_cast<const unsigned char*>(payload.constData());

		switch (type)
		{
			case CHUNK_STRING:
			{
				if (size < 4) { break; }
				uint32_t id = rd32(p);
				if (id >= strings.size()) { strings.resize(id + 1); }
				strings[id] = QString::fromUtf8(payload.constData() + 4, static_cast<int>(size) - 4);
			} break;
			case CHUNK_SITE:
			{
				if (size < 20) { break; }
				uint32_t id = rd32(p);
				if (id >= out.sites.size()) { out.sites.resize(id + 1); }
				ProfilerSite &s = out.sites[id];
				auto str = [&](uint32_t sid){ return sid < strings.size() ? strings[sid] : QString(); };
				s.line = rd32(p + 4);
				s.file = str(rd32(p + 8));
				s.func = str(rd32(p + 12));
				s.comment = str(rd32(p + 16));
			} break;
			case CHUNK_THREAD:
			{
				if (size < 4) { break; }
				size_t ti = threadFor(rd32(p));
				out.threads[ti].name = QString::fromUtf8(payload.constData() + 4, static_cast<int>(size) - 4);
			} break;
			case CHUNK_MARKERS:
			{
				if (size < 8) { break; }
				const uint32_t count = rd32(p + 4);
				if (static_cast<uint64_t>(size) < 8 + static_cast<uint64_t>(count) * MARKER_RECORD_SIZE)
				{
					out.truncated = true;
					break;
				}
				const size_t ti = threadFor(rd32(p));
				auto &stack = stacks[ti];
				auto &depths = out.threads[ti].depths;
				const unsigned char *rec = p + 8;

				for (uint32_t i = 0; i < count; i++, rec += MARKER_RECORD_SIZE)
				{
					const uint32_t site = rd32(rec + 1);
					const uint64_t ts = rd64(rec + 5);

					minTs = std::min(minTs, ts);
					maxTs = std::max(maxTs, ts);

					if (rec[0] == MARKER_START)
					{
						stack.push_back({ts, site});
					}
					else if (stack.empty() || stack.back().site != site)
					{
						out.strayEnds++;
					}
					else
					{
						const OpenBlock o = stack.back();
						stack.pop_back();
						const size_t depth = stack.size();
						if (depths.size() <= depth) { depths.resize(depth + 1); }
						depths[depth].push_back({o.start, ts, o.site});
					}
				}
				out.markerCount += count;
			} break;
			case CHUNK_FOOTER:
			{
				if (size >= 8)
				{
					out.hasFooter = true;
					out.footerCount = rd64(p);
				}
			} break;
			default:
				break;
		}

		if (progress && fileSize > 0)
		{
			int percent = static_cast<int>((file.pos() * 100) / fileSize);
			if (percent != lastPercent)
			{
				lastPercent = percent;
				if (!progress(percent))
				{
					err = "Canceled.";
					return false;
				}
			}
		}
	}

	if (minTs == UINT64_MAX)
	{
		minTs = maxTs = 0;
	}

	// Blocks still open at the end of the capture extend to the last marker
	for (size_t ti = 0; ti < stacks.size(); ti++)
	{
		auto &depths = out.threads[ti].depths;
		for (size_t d = 0; d < stacks[ti].size(); d++)
		{
			if (depths.size() <= d) { depths.resize(d + 1); }
			depths[d].push_back({stacks[ti][d].start, maxTs, stacks[ti][d].site});
			out.unmatchedStarts++;
		}
	}

	for (auto &t : out.threads)
	{
		for (auto &row : t.depths)
		{
			for (auto &b : row)
			{
				b.start -= minTs;
				b.end -= minTs;
			}
		}
	}
	out.duration = std::max<uint64_t>(1, maxTs - minTs);

	std::sort(out.threads.begin(), out.threads.end(),
		[](const ProfilerThreadData &a, const ProfilerThreadData &b){ return a.id < b.id; });

	// Blocks may reference sites that never got a definition chunk (truncated file)
	for (auto &t : out.threads)
	{
		for (auto &row : t.depths)
		{
			for (auto &b : row)
			{
				if (b.site >= out.sites.size())
				{
					out.sites.resize(b.site + 1);
				}
			}
		}
	}
	return true;
}
//----------------------------------------------------------------------------
ProfilerLoadThread_t::ProfilerLoadThread_t(QObject *parent)
	: QThread(parent)
{
}
//----------------------------------------------------------------------------
ProfilerLoadThread_t::~ProfilerLoadThread_t(void)
{
	requestInterruption();
	wait();
}
//----------------------------------------------------------------------------
void ProfilerLoadThread_t::run(void)
{
	auto data = std::make_shared<ProfilerCaptureData>();

	ok = profilerLoadCapture(filePath, *data, error, [this](int percent){
		emit progress(percent);
		return !isInterruptionRequested();
	});
	if (ok)
	{
		result = data;
	}
}
//----------------------------------------------------------------------------

