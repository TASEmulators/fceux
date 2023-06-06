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

#ifdef __QT_DRIVER__
#include <QThread>
#endif

#include "utils/mutex.h"
#include "fceu.h"
#include "profiler.h"

namespace FCEU
{
static thread_local profileExecVector execList;
static thread_local profilerFuncMap threadProfileMap;

FILE *profilerManager::pLog = nullptr;

static profilerManager  pMgr;

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
	min.fromSeconds(9);
	max.zero();
	sum.zero();
	numCalls = 0;
	recursionCount = 0;

	threadProfileMap.addRecord( fileNameStringLiteral, fileLineNumber,
					funcNameStringLiteral, commentStringLiteral, this);
}
//-------------------------------------------------------------------------
void funcProfileRecord::reset(void)
{
	min.fromSeconds(9);
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
profileFuncScoped::profileFuncScoped( funcProfileRecord *recordIn )
{
	rec = recordIn;

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

		rec->last = dt;
		rec->sum += dt;
		if (dt < rec->min) rec->min = dt;
		if (dt > rec->max) rec->max = dt;

		rec->recursionCount--;

		execList._vec.push_back(*rec);

		threadProfileMap.popStack(rec);
	}
}
//-------------------------------------------------------------------------
//---- Profile Execution Vector
//-------------------------------------------------------------------------
profileExecVector::profileExecVector(void)
{
	_vec.reserve( 10000 );

	char threadName[128];
	char fileName[256];

	strcpy( threadName, "MainThread");

#ifdef __QT_DRIVER__
	QThread *thread = QThread::currentThread();

	if (thread)
	{
		//printf("Thread: %s\n", thread->objectName().toStdString().c_str());
		strcpy( threadName, thread->objectName().toStdString().c_str());
	}
#endif
	sprintf( fileName, "fceux-profile-%s.log", threadName);

	logFp = ::fopen(fileName, "w");

	if (logFp == nullptr)
	{
		printf("Error: Failed to create profiler logfile: %s\n", fileName);
	}
}
//-------------------------------------------------------------------------
profileExecVector::~profileExecVector(void)
{
	if (logFp)
	{
		::fclose(logFp);
	}
}
//-------------------------------------------------------------------------
void profileExecVector::update(void)
{
	size_t n = _vec.size();

	for (size_t i=0; i<n; i++)
	{
		funcProfileRecord &rec = _vec[i];

		fprintf( logFp, "%s: %u  %f  %f  %f  %f\n", rec.funcName, rec.numCalls, rec.last.toSeconds(), rec.average(), rec.min.toSeconds(), rec.max.toSeconds());
	}
	_vec.clear();
}
//-------------------------------------------------------------------------
//---- Profile Function Record Map
//-------------------------------------------------------------------------
profilerFuncMap::profilerFuncMap(void)
{
	//printf("profilerFuncMap Constructor: %p\n", this);
	pMgr.addThreadProfiler(this);

	_map_it = _map.begin();
}
//-------------------------------------------------------------------------
profilerFuncMap::~profilerFuncMap(void)
{
	//printf("profilerFuncMap Destructor: %p\n", this);
	pMgr.removeThreadProfiler(this);

	//{
	//	autoScopedLock aLock(_mapMtx);

	//	for (auto it = _map.begin(); it != _map.end(); it++)
	//	{
	//		delete it->second;
	//	}
	//	_map.clear();
	//}
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
int profilerFuncMap::addRecord(const char *fileNameStringLiteral,
			      const int   fileLineNumber,
			      const char *funcNameStringLiteral,
			      const char *commentStringLiteral,
			      funcProfileRecord *rec )
{
	autoScopedLock aLock(_mapMtx);
	char lineString[64];

	sprintf( lineString, ":%i", fileLineNumber);

	std::string fname(fileNameStringLiteral);

	fname.append( lineString );

	_map[fname] = rec;

	return 0;
}
//-------------------------------------------------------------------------
funcProfileRecord *profilerFuncMap::findRecord(const char *fileNameStringLiteral,
					       const int   fileLineNumber,
					       const char *funcNameStringLiteral,
					       const char *commentStringLiteral,
					       bool create)
{
	autoScopedLock aLock(_mapMtx);
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
funcProfileRecord *profilerFuncMap::iterateBegin(void)
{
	autoScopedLock aLock(_mapMtx);
	funcProfileRecord *rec = nullptr;

	_map_it = _map.begin();

	if (_map_it != _map.end())
	{
		rec = _map_it->second;
	}
	return rec;
}
//-------------------------------------------------------------------------
funcProfileRecord *profilerFuncMap::iterateNext(void)
{
	autoScopedLock aLock(_mapMtx);
	funcProfileRecord *rec = nullptr;

	if (_map_it != _map.end())
	{
		_map_it++;
	}
	if (_map_it != _map.end())
	{
		rec = _map_it->second;
	}
	return rec;
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
		threadList.clear();
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
} // namespace FCEU

//-------------------------------------------------------------------------
int FCEU_profiler_log_thread_activity(void)
{
	FCEU::execList.update();
	return 0;
}
#endif //  __FCEU_PROFILER_ENABLE__
