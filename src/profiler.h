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
// profiler.h

#pragma once

/*
 *  This module is intended for debug use only. This allows for high precision timing of function
 *  execution. This functionality is not included in the build unless __FCEU_PROFILER_ENABLE__
 *  is defined. To check timing on a particular function, add FCEU_PROFILE_FUNC macro to the top
 *  of the function body in the following manner.
 *  FCEU_PROFILE_FUNC(prof, "String Literal comment, whatever I want it to say")
 *  When __FCEU_PROFILER_ENABLE__ is not defined, the FCEU_PROFILE_FUNC macro evaluates to nothing
 *  so it won't break the regular build by having it used in code.
 */
#ifdef __FCEU_PROFILER_ENABLE__

#include <stdio.h>
#include <stdint.h>
#include <string>
#include <vector>
#include <list>
#include <map>


#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
#include <time.h>
#endif

#include "utils/mutex.h"
#include "utils/timeStamp.h"

namespace FCEU
{
	struct funcProfileRecord
	{
		const int   fileLineNum;
		const char *fileName;
		const char *funcName;
		const char *comment;

		timeStampRecord min;
		timeStampRecord max;
		timeStampRecord sum;
		timeStampRecord last;
		unsigned int numCalls;
		unsigned int recursionCount;

		funcProfileRecord(const char *fileNameStringLiteral,
				  const int   fileLineNumber,
				  const char *funcNameStringLiteral,
				  const char *commentStringLiteral);

		void reset(void);

		double average(void);
	};

	struct profileFuncScoped
	{
		funcProfileRecord *rec;
		timeStampRecord start;

		profileFuncScoped( funcProfileRecord *recordIn );

		~profileFuncScoped(void);
	};

	struct profileExecVector
	{
		profileExecVector(void);
		~profileExecVector(void);

		void update(void);

		std::vector <funcProfileRecord> _vec;

		FILE *logFp;
	};

	class profilerFuncMap
	{
		public:
			profilerFuncMap();
			~profilerFuncMap();

			int addRecord(const char *fileNameStringLiteral,
				      const int   fileLineNumber,
				      const char *funcNameStringLiteral,
				      const char *commentStringLiteral,
				      funcProfileRecord *rec );

			funcProfileRecord *findRecord(const char *fileNameStringLiteral,
						      const int   fileLineNumber,
						      const char *funcNameStringLiteral,
						      const char *commentStringLiteral,
						      bool create = false);

			funcProfileRecord *iterateBegin(void);
			funcProfileRecord *iterateNext(void);

			void pushStack(funcProfileRecord *rec);
			void popStack(funcProfileRecord *rec);
		private:
			mutex  _mapMtx;
			std::map<std::string, funcProfileRecord*> _map;
			std::map<std::string, funcProfileRecord*>::iterator _map_it;

			std::vector <funcProfileRecord*> stack;
	};

	class profilerManager
	{
		public:
			profilerManager(void);
			~profilerManager(void);
	
			int addThreadProfiler( profilerFuncMap *m );
			int removeThreadProfiler( profilerFuncMap *m, bool shouldDestroy = false );
	
			static FILE *pLog;

			static profilerManager *getInstance();
		private:
	
			mutex  threadListMtx;
			std::list <profilerFuncMap*> threadList;
			static profilerManager *instance;
	};
}

#if  defined(__PRETTY_FUNCTION__)
#define  __FCEU_PROFILE_FUNC_NAME__  __PRETTY_FUNCTION__
#else
#define  __FCEU_PROFILE_FUNC_NAME__  __func__
#endif

#define  FCEU_PROFILE_FUNC(id, comment)   \
	static thread_local FCEU::funcProfileRecord  id( __FILE__, __LINE__, __FCEU_PROFILE_FUNC_NAME__, comment ); \
	FCEU::profileFuncScoped id ## _unique_scope( &id )


int FCEU_profiler_log_thread_activity(void);

#else  // __FCEU_PROFILER_ENABLE__ not defined

#define  FCEU_PROFILE_FUNC(id, comment)   

#endif // __FCEU_PROFILER_ENABLE__

