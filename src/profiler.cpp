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

FILE *profilerManager::pLog = nullptr;

static profilerManager  pMgr;

//-------------------------------------------------------------------------
//---- Profile Scoped Function Class
//-------------------------------------------------------------------------
profileFuncScoped::profileFuncScoped(
	int fileLineNum,
	const char *fileName,
	const char *funcName,
	const char *comment )
{
	start.ts.readNew();
	start.fileLineNum = fileLineNum;
	start.fileName = fileName;
	start.funcName = funcName;
	start.comment = comment;
	start.type = profileMarker::MARKER_START;

	execList.pushMarker(start);
}
//-------------------------------------------------------------------------
profileFuncScoped::~profileFuncScoped(void)
{
	timeStampRecord ts, dt;
	ts.readNew();
	dt = ts - start.ts;

	profileMarker marker = start;
	marker.ts = ts;
	marker.type = profileMarker::MARKER_END;

	execList.pushMarker(marker);
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
			bufferList.erase(it);
			result = 0;
			break;
		}
	}
	return result;
}

int profilerManager::dumpProfileMarkers(FILE *pFile)
{
	int result = -1;

	if (!enabled)
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
