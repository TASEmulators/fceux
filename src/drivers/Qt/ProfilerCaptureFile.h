// ProfilerCaptureFile.h
//
// Reader for the binary profiler capture format written by ProfilerCaptureThread_t
// (see ProfilerCapture.h). Marker pairs are matched into nested blocks per thread.

#pragma once

#ifdef __FCEU_PROFILER_ENABLE__

#include <stdint.h>
#include <functional>
#include <memory>
#include <vector>

#include <QString>
#include <QThread>

struct ProfilerSite
{
	uint32_t line = 0;
	QString  file;
	QString  func;
	QString  comment;
};

struct ProfilerBlock
{
	uint64_t start = 0;  // counts, relative to the first marker in the capture
	uint64_t end = 0;
	uint32_t site = 0;
};

struct ProfilerThreadData
{
	uint32_t id = 0;
	QString  name;
	// depths[d] holds the blocks nested d levels deep, ordered by start time.
	// Blocks within one depth never overlap, so they are also ordered by end time.
	std::vector<std::vector<ProfilerBlock>> depths;
};

struct ProfilerCaptureData
{
	uint32_t version = 0;
	uint64_t countFreq = 1;       // counts per second
	uint64_t duration = 1;        // counts, first to last marker
	uint64_t markerCount = 0;
	uint64_t footerCount = 0;
	uint64_t unmatchedStarts = 0; // blocks still open at the end of the capture
	uint64_t strayEnds = 0;       // end markers with no matching start
	bool hasFooter = false;
	bool truncated = false;

	std::vector<ProfilerSite> sites;
	std::vector<ProfilerThreadData> threads;

	double countsToSeconds(uint64_t counts) const
	{
		return static_cast<double>(counts) / static_cast<double>(countFreq ? countFreq : 1);
	}
};

// The progress callback receives a percentage and returns false to cancel loading.
bool profilerLoadCapture(const QString &path, ProfilerCaptureData &out, QString &err,
	const std::function<bool(int)> &progress = nullptr);

class ProfilerLoadThread_t : public QThread
{
	Q_OBJECT

public:
	ProfilerLoadThread_t(QObject *parent = nullptr);
	~ProfilerLoadThread_t(void);

	void setFilePath(const QString &path){ filePath = path; }
	bool succeeded(void) const { return ok; }
	QString errorString(void) const { return error; }
	std::shared_ptr<const ProfilerCaptureData> data(void) const { return result; }

signals:
	void progress(int percent);

protected:
	void run(void) override;

private:
	QString filePath;
	bool ok = false;
	QString error;
	std::shared_ptr<const ProfilerCaptureData> result;
};

#endif // __FCEU_PROFILER_ENABLE__
