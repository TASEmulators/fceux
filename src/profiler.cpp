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

#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
#include <unistd.h>
#endif

#include "utils/mutex.h"
#include "profiler.h"

#if defined(WIN32)
#include <windows.h>
#endif

namespace FCEU
{
static thread_local profilerFuncMap threadProfileMap;

FILE *profilerManager::pLog = nullptr;

static profilerManager  pMgr;

//-------------------------------------------------------------------------
//---- Time Stamp Record
//-------------------------------------------------------------------------
#if defined(WIN32)
uint64_t timeStampRecord::qpcFreq = 0;
#include <intrin.h>
#pragma intrinsic(__rdtsc)
#else
#include <x86intrin.h>
#endif
uint64_t timeStampRecord::tscFreq = 0;

static uint64_t rdtsc()
{
	return __rdtsc();
}

void timeStampRecord::readNew(void)
{
#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
	clock_gettime( CLOCK_REALTIME, &ts );
#elif defined(WIN32)
	QueryPerformanceCounter((LARGE_INTEGER*)&ts);
#else
	ts = 0;
#endif
	tsc = rdtsc();
}

static void calibrateTSC(void)
{
	constexpr int numSamples = 1;
	timeStampRecord t1, t2, td;
	uint64_t td_sum = 0;
	double td_avg;

#if defined(WIN32)
	if (QueryPerformanceFrequency((LARGE_INTEGER*)&timeStampRecord::qpcFreq) == 0)
	{
		printf("QueryPerformanceFrequency FAILED!\n");
	}
#endif
	printf("Running TSC Calibration: %i sec...\n", numSamples);

	for (int i=0; i<numSamples; i++)
	{
		t1.readNew();
#if defined(WIN32)
		Sleep(1000);
#else
		sleep(1);
#endif
		t2.readNew();

		td += t2 - t1;

		td_sum = td.tsc;

		td_avg = static_cast<double>(td_sum);

		timeStampRecord::tscFreq = static_cast<uint64_t>( td_avg / td.toSeconds() );

		printf("%i Calibration: %f sec   TSC:%llu   TSC Freq: %f MHz\n", i, td.toSeconds(), 
			static_cast<unsigned long long>(td.tsc), static_cast<double>(timeStampRecord::tscFreq) * 1.0e-6 );
	}
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
//-----  profilerManager class
//-------------------------------------------------------------------------
profilerManager::profilerManager(void)
{
	calibrateTSC();

	printf("profilerManager Constructor\n");
	if (pLog == nullptr)
	{
		pLog = stdout;
	}
}

profilerManager::~profilerManager(void)
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

int profilerManager::addThreadProfiler( profilerFuncMap *m )
{
	autoScopedLock aLock(threadListMtx);
	threadList.push_back(m);
	return 0;
}

int profilerManager::removeThreadProfiler( profilerFuncMap *m, bool shouldDestroy )
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
//-------------------------------------------------------------------------
//
}
#endif //  __FCEU_PROFILER_ENABLE__
