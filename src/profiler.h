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
		enum { MARKER_START, MARKER_END } type;
	};

	struct profileMarkerBuffer
	{
		static constexpr unsigned int maxBuffers = 4;

		profileMarkerBuffer();
		~profileMarkerBuffer();

		void pushMarker(const profileMarker &marker);

		std::vector<profileMarker> vec[maxBuffers];
		std::atomic<unsigned int> activeBufferIndex{0};
	};

	struct profileFuncScoped
	{
		profileFuncScoped( 
			int fileLineNum,
			const char *fileName,
			const char *funcName,
			const char *comment
		);

		~profileFuncScoped(void);

		private:
			profileMarker start;
	};
	class profilerManager
	{
		public:
			profilerManager(void);
			~profilerManager(void);
	
			int addThreadProfileBuffer( profileMarkerBuffer *b );
			int removeThreadProfileBuffer( profileMarkerBuffer *b );
			int dumpProfileMarkers(FILE *pFile = nullptr);
	
			static FILE *pLog;

			static profilerManager *getInstance();
		private:
	
			mutex  threadListMtx;
			std::list <profileMarkerBuffer*> bufferList;
			static profilerManager *instance;
	};
} // namespace FCEU

#if  defined(__PRETTY_FUNCTION__)
#define  __FCEU_PROFILE_FUNC_NAME__  __PRETTY_FUNCTION__
#else
#define  __FCEU_PROFILE_FUNC_NAME__  __func__
#endif

#define FCEU_CONCAT_IMPL(a, b) a##b
#define FFCEU_UNIQUE_ID(a, b) CONCAT_IMPL(a, b)

#define  FCEU_PROFILE_FUNC(comment)   \
	FCEU::profileFuncScoped _##__LINE__( __LINE__, __FILE__, __FCEU_PROFILE_FUNC_NAME__, comment )

	void FCEU_profiler_log_thread_activity();

#else  // __FCEU_PROFILER_ENABLE__ not defined

#define  FCEU_PROFILE_FUNC(id, comment)

#endif // __FCEU_PROFILER_ENABLE__

