/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2002 Xodnizel
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
// profiler.cpp
//
#ifdef __FCEU_PROFILER_ENABLE__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#ifdef __QT_DRIVER__
#include <QThread>
#endif

#include "utils/mutex.h"
#include "fceu.h"
#include "profiler.h"

namespace FCEU
{
static thread_local profileMarkerBuffer execList;

std::atomic<bool> profilerEnabledFlag{false};

FILE *profilerManager::pLog = nullptr;

static profilerManager  pMgr;

profileMarkerBuffer *threadBuffer(void){ return &execList; }

//-------------------------------------------------------------------------
//---- Profile Scoped Function Class
//-------------------------------------------------------------------------
static void pushSiteMarker(const profileSite *site, profileMarker::MarkerType type)
{
	profileMarker marker;

	marker.ts.readNew();
	marker.fileLineNum = site->line;
	marker.fileName = site->file;
	marker.funcName = site->func;
	marker.comment = site->comment;
	marker.type = type;

	execList.pushMarker(marker);
}
//-------------------------------------------------------------------------
void profileFuncScoped::beginSlow(const profileSite *s)
{
	pushSiteMarker(s, profileMarker::MARKER_START);
}
//-------------------------------------------------------------------------
void profileFuncScoped::endSlow(const profileSite *s)
{
	pushSiteMarker(s, profileMarker::MARKER_END);
}
//-------------------------------------------------------------------------
//---- Profile Marker Vector
//-------------------------------------------------------------------------
	profileMarkerBuffer::profileMarkerBuffer()
	{
		for (auto i=0u; i<maxBuffers; i++)
		{
			vec[i].reserve(1024);
		}
		activeBufferIndex = 0;

		pMgr.addThreadProfileBuffer(this);
	}

	profileMarkerBuffer::~profileMarkerBuffer()
	{
		pMgr.removeThreadProfileBuffer(this);
	}

	void profileMarkerBuffer::pushMarker(const profileMarker &marker)
	{
		unsigned int idx = activeBufferIndex.load(std::memory_order_relaxed);
		idx = idx % maxBuffers;
		vec[idx].push_back(marker);
	}

//-------------------------------------------------------------------------
//-----  profilerManager class
//-------------------------------------------------------------------------
profilerManager* profilerManager::instance = nullptr;

profilerManager* profilerManager::getInstance(void)
{
	return instance;
}
//-------------------------------------------------------------------------
profilerManager::profilerManager(void)
{
	//printf("profilerManager Constructor\n");
	if (pLog == nullptr)
	{
		pLog = stdout;
	}

	if (instance == nullptr)
	{
		instance = this;
	}
}

profilerManager::~profilerManager(void)
{
	//printf("profilerManager Destructor\n");
	{
		autoScopedLock aLock(threadListMtx);
		bufferList.clear();
	}

	if (pLog && (pLog != stdout))
	{
		fclose(pLog); pLog = nullptr;
	}
	if (instance == this)
	{
		instance = nullptr;
	}
}

int profilerManager::addThreadProfileBuffer( profileMarkerBuffer *b )
{
	autoScopedLock aLock(threadListMtx);
	b->threadId = nextThreadId++;
	bufferList.push_back(b);
	return 0;
}

int profilerManager::removeThreadProfileBuffer( profileMarkerBuffer *b )
{
	int result = -1;
	autoScopedLock aLock(threadListMtx);

	for (auto it = bufferList.begin(); it != bufferList.end(); it++)
	{
		if (*it == b )
		{
			if (externalCollector)
			{
				// Keep the unread markers, oldest buffer first, for the next drain
				retiredBuffer r;
				r.threadId = b->threadId;
				r.threadName = b->threadName;
				const unsigned int active = b->activeBufferIndex.load();
				for (unsigned int i = 1; i <= profileMarkerBuffer::maxBuffers; i++)
				{
					auto &v = b->vec[(active + i) % profileMarkerBuffer::maxBuffers];
					r.markers.insert(r.markers.end(), v.begin(), v.end());
				}
				if (!r.markers.empty())
				{
					retiredList.push_back(std::move(r));
				}
			}
			bufferList.erase(it);
			result = 0;
			break;
		}
	}
	return result;
}

void profilerManager::setThreadName( profileMarkerBuffer *b, const char *name )
{
	autoScopedLock aLock(threadListMtx);
	b->threadName = name ? name : "";
}

int profilerManager::drainMarkers(const drainCallback &cb, bool flushAll)
{
	autoScopedLock aLock(threadListMtx);

	for (auto &r : retiredList)
	{
		cb(r.threadId, r.threadName, r.markers.data(), r.markers.size());
	}
	retiredList.clear();

	const unsigned int passes = flushAll ? profileMarkerBuffer::maxBuffers : 1;

	for (profileMarkerBuffer *b : bufferList)
	{
		for (unsigned int p = 0; p < passes; p++)
		{
			// Same rotation as dumpProfileMarkers: swap the producer to the next buffer
			// and drain the one that was retired two swaps ago.
			unsigned int idx = b->activeBufferIndex++;
			idx = (idx + 2) % profileMarkerBuffer::maxBuffers;

			auto &v = b->vec[idx];
			if (!v.empty())
			{
				cb(b->threadId, b->threadName, v.data(), v.size());
				v.clear();
			}
		}
	}
	return 0;
}

int profilerManager::dumpProfileMarkers(FILE *pFile)
{
	int result = -1;

	if (!isEnabled() || externalCollector)
	{
		return result;
	}
	autoScopedLock aLock(threadListMtx);

	if (pFile == nullptr)
	{
		pFile = stdout;
	}

	if (pFile)
	{
		fprintf(pFile, "--------------------\n");
		fprintf(pFile, "Dumping Profile Markers\n");
		fprintf(pFile, "--------------------\n");

		for (auto it = bufferList.begin(); it != bufferList.end(); it++)
		{
			profileMarkerBuffer *b = *it;

			unsigned int idx = b->activeBufferIndex++;
			idx = (idx + 2) % profileMarkerBuffer::maxBuffers;

			for (auto &marker : b->vec[idx])
			{
				const char *markerTypeStr = (marker.type == profileMarker::MARKER_START) ? "MARKER_START" : "MARKER_END";
				fprintf(pFile, "%s  %s:%i  %s  %s  Time: %" PRIu64 "\n",
					markerTypeStr,
					marker.fileName,
					marker.fileLineNum,
					marker.funcName,
					marker.comment,
					marker.ts.toMicroSeconds());
			}
			b->vec[idx].clear();
		}
		result = 0;
	}
	return result;
}

//-------------------------------------------------------------------------
} // namespace FCEU

void FCEU_profiler_set_thread_name(const char *name)
{
	FCEU::profilerManager *mgr = FCEU::profilerManager::getInstance();

	if (mgr)
	{
		mgr->setThreadName(FCEU::threadBuffer(), name);
	}
}

void FCEU_profiler_log_thread_activity()
{
	FCEU::profilerManager *mgr = FCEU::profilerManager::getInstance();

	if (mgr)
	{
		mgr->dumpProfileMarkers(mgr->pLog);
	}
}
//-------------------------------------------------------------------------
#endif //  __FCEU_PROFILER_ENABLE__
