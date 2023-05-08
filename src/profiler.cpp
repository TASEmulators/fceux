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

#include "utils/mutex.h"
#include "profiler.h"

namespace FCEU
{
static thread_local profilerFuncMap threadProfileMap;

class profilerManager
{
	public:
		profilerManager(void)
		{
			printf("profilerManager Constructor\n");
			if (pLog == nullptr)
			{
				pLog = stdout;
			}
		}

		~profilerManager(void)
		{
			printf("profilerManager Destructor\n");
			{
				autoScopedLock aLock(threadListMtx);
				threadList.clear();
			}

			if (pLog && (pLog != stdout))
			{
				fclose(pLog); pLog = nullptr;
			}
		}

		int addThreadProfiler( profilerFuncMap *m )
		{
			autoScopedLock aLock(threadListMtx);
			threadList.push_back(m);
			return 0;
		}

		int removeThreadProfiler( profilerFuncMap *m, bool shouldDestroy = false )
		{
			int result = -1;
			autoScopedLock aLock(threadListMtx);

			for (auto it = threadList.begin(); it != threadList.end(); it++)
			{
				if (*it == m )
				{
					threadList.erase(it);
					if (shouldDestroy)
					{
						delete m;
					}
					result = 0;
					break;
				}
			}
			return result;
		}

		static FILE *pLog;
	private:

		mutex  threadListMtx;
		std::list <profilerFuncMap*> threadList;

};
FILE *profilerManager::pLog = nullptr;

static profilerManager  pMgr;

//-------------------------------------------------------------------------
//---- Time Stamp Record
//-------------------------------------------------------------------------
void timeStampRecord::readNew(void)
{
#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
	clock_gettime( CLOCK_REALTIME, &ts );
#else
	ts = 0;
#endif
}

//-------------------------------------------------------------------------
//---- Function Profile Record
//-------------------------------------------------------------------------
funcProfileRecord::funcProfileRecord(const char *fileNameStringLiteral,
				     const int   fileLineNumber,
				     const char *funcNameStringLiteral,
				     const char *commentStringLiteral)

	: fileLineNum(fileLineNumber), fileName(fileNameStringLiteral),
	  funcName(funcNameStringLiteral), comment(commentStringLiteral)
{
	min.zero();
	max.zero();
	sum.zero();
	numCalls = 0;
	recursionCount = 0;
}
//-------------------------------------------------------------------------
void funcProfileRecord::reset(void)
{
	min.zero();
	max.zero();
	sum.zero();
	numCalls = 0;
}
//-------------------------------------------------------------------------
double funcProfileRecord::average(void)
{
	double avg = 0.0;

	if (numCalls)
	{
		avg = sum.toSeconds() / static_cast<double>(numCalls);
	}
	return avg;
}
//-------------------------------------------------------------------------
//---- Profile Scoped Function Class
//-------------------------------------------------------------------------
profileFuncScoped::profileFuncScoped(const char *fileNameStringLiteral,
				     const int   fileLineNumber,
				     const char *funcNameStringLiteral,
				     const char *commentStringLiteral)
{
	rec = nullptr;

	//if (threadProfileMap == nullptr)
	//{
	//	threadProfileMap = new profilerFuncMap();
	//}

	rec = threadProfileMap.findRecord( fileNameStringLiteral, fileLineNumber,
						funcNameStringLiteral, commentStringLiteral, true);

	if (rec)
	{
		threadProfileMap.pushStack(rec);
		start.readNew();
		rec->numCalls++;
		rec->recursionCount++;
	}
}
//-------------------------------------------------------------------------
profileFuncScoped::~profileFuncScoped(void)
{
	if (rec)
	{
		timeStampRecord ts, dt;
		ts.readNew();
		dt = ts - start;

		rec->sum += dt;
		if (dt < rec->min) rec->min = dt;
		if (dt > rec->max) rec->max = dt;

		rec->recursionCount--;

		//printf("%s: %u  %f  %f  %f  %f\n", rec->funcName, rec->numCalls, dt.toSeconds(), rec->average(), rec->min.toSeconds(), rec->max.toSeconds());
		threadProfileMap.popStack(rec);
	}
}
//-------------------------------------------------------------------------
//---- Profile Function Record Map
//-------------------------------------------------------------------------
profilerFuncMap::profilerFuncMap(void)
{
	printf("profilerFuncMap Constructor: %p\n", this);
	pMgr.addThreadProfiler(this);
}
//-------------------------------------------------------------------------
profilerFuncMap::~profilerFuncMap(void)
{
	printf("profilerFuncMap Destructor: %p\n", this);
	pMgr.removeThreadProfiler(this);

	for (auto it = _map.begin(); it != _map.end(); it++)
	{
		delete it->second;
	}
	_map.clear();
}
//-------------------------------------------------------------------------
void profilerFuncMap::pushStack(funcProfileRecord *rec)
{
	stack.push_back(rec);
}
//-------------------------------------------------------------------------
void profilerFuncMap::popStack(funcProfileRecord *rec)
{
	stack.pop_back();
}
//-------------------------------------------------------------------------
funcProfileRecord *profilerFuncMap::findRecord(const char *fileNameStringLiteral,
					       const int   fileLineNumber,
					       const char *funcNameStringLiteral,
					       const char *commentStringLiteral,
					       bool create)
{
	char lineString[64];
	funcProfileRecord *rec = nullptr;

	sprintf( lineString, ":%i", fileLineNumber);

	std::string fname(fileNameStringLiteral);

	fname.append( lineString );

	auto it = _map.find(fname);

	if (it != _map.end())
	{
		rec = it->second;
	}
	else if (create)
	{
		fprintf( pMgr.pLog, "Creating Function Profile Record: %s  %s\n", fname.c_str(), funcNameStringLiteral);

		rec = new funcProfileRecord( fileNameStringLiteral, fileLineNumber,
						funcNameStringLiteral, commentStringLiteral);

		_map[fname] = rec;
	}
	return rec;
}
//-------------------------------------------------------------------------
//
}
#endif //  __FCEU_PROFILER_ENABLE__
