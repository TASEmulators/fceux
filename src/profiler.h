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

#include <stdint.h>
#include <string>
#include <vector>
#include <list>
#include <map>


#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
#include <time.h>
#endif

#include "utils/mutex.h"

namespace FCEU
{
	struct timeStampRecord
	{
#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
		struct timespec ts;
		uint64_t tsc;

		timeStampRecord(void)
		{
			ts.tv_sec = 0;
			ts.tv_nsec = 0;
			tsc = 0;
		}

		timeStampRecord& operator = (const timeStampRecord& in)
		{
			ts = in.ts;
			tsc = in.tsc;
			return *this;
		}

		timeStampRecord& operator += (const timeStampRecord& op)
		{
			ts.tv_sec  += op.ts.tv_sec;
			ts.tv_nsec += op.ts.tv_nsec;

			if (ts.tv_nsec >= 1000000000)
			{
				ts.tv_nsec -= 1000000000;
				ts.tv_sec++;	
			}
			tsc += op.tsc;
			return *this;
		}

		timeStampRecord operator + (const timeStampRecord& op)
		{
			timeStampRecord res;

			res.ts.tv_sec  = ts.tv_sec  + op.ts.tv_sec;
			res.ts.tv_nsec = ts.tv_nsec + op.ts.tv_nsec;

			if (res.ts.tv_nsec >= 1000000000)
			{
				res.ts.tv_nsec -= 1000000000;
				res.ts.tv_sec++;	
			}
			res.tsc = tsc + op.tsc;
			return res;
		}

		timeStampRecord operator - (const timeStampRecord& op)
		{
			timeStampRecord res;

			res.ts.tv_sec  = ts.tv_sec  - op.ts.tv_sec;
			res.ts.tv_nsec = ts.tv_nsec - op.ts.tv_nsec;

			if (res.ts.tv_nsec < 0)
			{
				res.ts.tv_nsec += 1000000000;
				res.ts.tv_sec--;	
			}
			res.tsc = tsc - op.tsc;

			return res;
		}

		bool operator > (const timeStampRecord& op)
		{
			bool res = false;
			if (ts.tv_sec == op.ts.tv_sec)
			{
				res = (ts.tv_nsec > op.ts.tv_nsec);
			}
			else if (ts.tv_sec > op.ts.tv_sec)
			{
				res = (ts.tv_sec > op.ts.tv_sec);
			}
			return res;
		}

		bool operator < (const timeStampRecord& op)
		{
			bool res = false;
			if (ts.tv_sec == op.ts.tv_sec)
			{
				res = (ts.tv_nsec < op.ts.tv_nsec);
			}
			else if (ts.tv_sec > op.ts.tv_sec)
			{
				res = (ts.tv_sec < op.ts.tv_sec);
			}
			return res;
		}

		void zero(void)
		{
			ts.tv_sec = 0;
			ts.tv_nsec = 0;
			tsc = 0;
		}

		double toSeconds(void)
		{
			double sec = static_cast<double>(ts.tv_sec) + ( static_cast<double>(ts.tv_nsec) * 1.0e-9 );
			return sec;
		}
#else	// WIN32
		uint64_t ts;
		uint64_t tsc;

		timeStampRecord(void)
		{
			ts = 0;
			tsc = 0;
		}

		timeStampRecord& operator = (const timeStampRecord& in)
		{
			ts = in.ts;
			tsc = in.tsc;
			return *this;
		}

		timeStampRecord& operator += (const timeStampRecord& op)
		{
			ts  += op.ts;
			tsc += op.tsc;
			return *this;
		}

		timeStampRecord operator + (const timeStampRecord& op)
		{
			timeStampRecord res;

			res.ts  = ts  + op.ts;
			res.tsc = tsc + op.tsc;
			return res;
		}

		timeStampRecord operator - (const timeStampRecord& op)
		{
			timeStampRecord res;

			res.ts  = ts  - op.ts;
			res.tsc = tsc - op.tsc;

			return res;
		}

		bool operator > (const timeStampRecord& op)
		{
			return ts > op.ts;
		}

		bool operator < (const timeStampRecord& op)
		{
			return ts < op.ts;
		}

		void zero(void)
		{
			ts = 0;
			tsc = 0;
		}

		double toSeconds(void)
		{
			double sec = static_cast<double>(ts) / static_cast<double>(qpcFreq);
			return sec;
		}
		static uint64_t qpcFreq;
#endif
		static uint64_t tscFreq;

		void readNew(void);
	};

	struct funcProfileRecord
	{
		const int   fileLineNum;
		const char *fileName;
		const char *funcName;
		const char *comment;

		timeStampRecord min;
		timeStampRecord max;
		timeStampRecord sum;
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

		profileFuncScoped(const char *fileNameStringLiteral,
				  const int   fileLineNumber,
				  const char *funcNameStringLiteral,
				  const char *commentStringLiteral);

		~profileFuncScoped(void);
	};

	class profilerFuncMap
	{
		public:
			profilerFuncMap();
			~profilerFuncMap();

			funcProfileRecord *findRecord(const char *fileNameStringLiteral,
						      const int   fileLineNumber,
						      const char *funcNameStringLiteral,
						      const char *commentStringLiteral,
						      bool create = false);

			void pushStack(funcProfileRecord *rec);
			void popStack(funcProfileRecord *rec);
		private:
			std::map<std::string, funcProfileRecord*> _map;

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
		private:
	
			mutex  threadListMtx;
			std::list <profilerFuncMap*> threadList;
	};
}

#if  defined(__PRETTY_FUNCTION__)
#define  __FCEU_PROFILE_FUNC_NAME__  __PRETTY_FUNCTION__
#else
#define  __FCEU_PROFILE_FUNC_NAME__  __func__
#endif

#define  FCEU_PROFILE_FUNC(id, comment)   FCEU::profileFuncScoped  id( __FILE__, __LINE__, __FCEU_PROFILE_FUNC_NAME__, comment )

#else  // __FCEU_PROFILER_ENABLE__ not defined

#define  FCEU_PROFILE_FUNC(id, comment)   

#endif // __FCEU_PROFILER_ENABLE__

