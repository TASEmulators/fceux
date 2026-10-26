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
#include <atomic>
#include <functional>


#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
#include <time.h>
#endif

#include "utils/mutex.h"
#include "utils/timeStamp.h"

namespace FCEU
{
	struct profileMarker
	{ 
		int   fileLineNum = 0;
		const char *fileName = nullptr;
		const char *funcName = nullptr;
		const char *comment = nullptr;
		timeStampRecord   ts;
		enum MarkerType { MARKER_START, MARKER_END };
		MarkerType type = MARKER_START;
	};

	struct profileMarkerBuffer
	{
		static constexpr unsigned int maxBuffers = 4;

		profileMarkerBuffer();
		~profileMarkerBuffer();

		void pushMarker(const profileMarker &marker);

		std::vector<profileMarker> vec[maxBuffers];
		std::atomic<unsigned int> activeBufferIndex{0};
		uint32_t    threadId = 0;
		std::string threadName;
	};

	// Compile-time description of one profiled scope
	struct profileSite
	{
		int         line;
		const char *file;
		const char *func;
		const char *comment;
	};

	// Master switch, kept as a plain global so the disabled check is a single inlined load
	extern std::atomic<bool> profilerEnabledFlag;

#if defined(__GNUC__) || defined(__clang__)
#define FCEU_PROFILER_UNLIKELY(x)  __builtin_expect(!!(x), 0)
#else
#define FCEU_PROFILER_UNLIKELY(x)  (x)
#endif

	struct profileFuncScoped
	{
		// When profiling is disabled this is one relaxed load and a branch, with no calls
		explicit profileFuncScoped(const profileSite *s)
		{
			if (FCEU_PROFILER_UNLIKELY(profilerEnabledFlag.load(std::memory_order_relaxed)))
			{
				site = s;
				beginSlow(s);
			}
		}

		~profileFuncScoped(void)
		{
			if (FCEU_PROFILER_UNLIKELY(site != nullptr))
			{
				endSlow(site);
			}
		}

		profileFuncScoped(const profileFuncScoped&) = delete;
		profileFuncScoped &operator=(const profileFuncScoped&) = delete;

		private:
			static void beginSlow(const profileSite *s);
			static void endSlow(const profileSite *s);

			// Non-null only if the start marker was recorded, so that end markers always pair up
			const profileSite *site = nullptr;
	};
	class profilerManager
	{
		public:
			profilerManager(void);
			~profilerManager(void);
	
			// Called once per thread with the markers drained from that thread's buffer (in time order).
			typedef std::function<void(uint32_t threadId, const std::string &threadName,
				const profileMarker *markers, size_t count)> drainCallback;

			void setEnabled(bool enable){ profilerEnabledFlag.store(enable, std::memory_order_relaxed); };
			bool isEnabled(void){ return profilerEnabledFlag.load(std::memory_order_relaxed); };

			// When an external collector is registered, the periodic text dump is suppressed
			// so that the two consumers do not compete for the same marker buffers.
			void setExternalCollector(bool active){ externalCollector = active; };

			void setThreadName( profileMarkerBuffer *b, const char *name );

			// Drains one buffer per thread, or all of them (flushAll) to empty the pipeline.
			// The callback is invoked with the thread list locked.
			int drainMarkers(const drainCallback &cb, bool flushAll = false);
			int addThreadProfileBuffer( profileMarkerBuffer *b );
			int removeThreadProfileBuffer( profileMarkerBuffer *b );
			int dumpProfileMarkers(FILE *pFile = nullptr);
	
			static FILE *pLog;

			static profilerManager *getInstance();
		private:
	
			// Markers left over from threads that exited before the collector drained them
			struct retiredBuffer
			{
				uint32_t threadId;
				std::string threadName;
				std::vector<profileMarker> markers;
			};
			std::list <retiredBuffer> retiredList;

			mutex  threadListMtx;
			std::list <profileMarkerBuffer*> bufferList;
			std::atomic<bool> externalCollector{false};
			std::atomic<uint32_t> nextThreadId{1};
			static profilerManager *instance;
	};
} // namespace FCEU

#if  defined(__PRETTY_FUNCTION__)
#define  __FCEU_PROFILE_FUNC_NAME__  __PRETTY_FUNCTION__
#else
#define  __FCEU_PROFILE_FUNC_NAME__  __func__
#endif

#define FCEU_PROFILE_CONCAT_IMPL(a, b) a##b
#define FCEU_PROFILE_CONCAT(a, b) FCEU_PROFILE_CONCAT_IMPL(a, b)

#define  FCEU_PROFILE_FUNC(comment)   \
	static constexpr FCEU::profileSite FCEU_PROFILE_CONCAT(_fceuProfSite, __LINE__) = \
		{ __LINE__, __FILE__, __FCEU_PROFILE_FUNC_NAME__, comment }; \
	FCEU::profileFuncScoped FCEU_PROFILE_CONCAT(_fceuProfScope, __LINE__)( &FCEU_PROFILE_CONCAT(_fceuProfSite, __LINE__) )

	void FCEU_profiler_log_thread_activity();
	void FCEU_profiler_set_thread_name(const char *name);

#define  FCEU_PROFILER_SET_THREAD_NAME(name)  FCEU_profiler_set_thread_name(name)

#else  // __FCEU_PROFILER_ENABLE__ not defined

#define  FCEU_PROFILE_FUNC(comment)
#define  FCEU_PROFILER_SET_THREAD_NAME(name)

#endif // __FCEU_PROFILER_ENABLE__

