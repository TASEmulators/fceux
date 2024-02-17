// timeStamp.cpp
#include <stdio.h>

#include "timeStamp.h"

#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
#include <unistd.h>
#endif

#if defined(WIN32)
#include <windows.h>
#endif

//-------------------------------------------------------------------------
//---- Time Stamp Record
//-------------------------------------------------------------------------
#ifdef __FCEU_X86_TSC_ENABLE
#if defined(WIN32)
#include <intrin.h>
#pragma intrinsic(__rdtsc)
#else
#if defined(__x86_64_) || defined(__i386__)
#include <x86intrin.h>
#endif
#endif

static uint64_t rdtsc()
{
#if defined(__arm64__) || defined(__arm__)
    // SPDX-License-Identifier: GPL-2.0
    uint64_t val = 0;

    /*
     * According to ARM DDI 0487F.c, from Armv8.0 to Armv8.5 inclusive, the
     * system counter is at least 56 bits wide; from Armv8.6, the counter
     * must be 64 bits wide.  So the system counter could be less than 64
     * bits wide and it is attributed with the flag 'cap_user_time_short'
     * is true.
     */
    asm volatile("mrs %0, cntvct_el0" : "=r" (val));

    return val;
#else
	return __rdtsc();
#endif
}
#endif

namespace FCEU
{

uint64_t timeStampRecord::_tscFreq = 0;
#if defined(WIN32)
uint64_t timeStampRecord::qpcFreq = 0;
#endif

void timeStampRecord::readNew(void)
{
	#ifdef __FCEU_X86_TSC_ENABLE
		tsc = rdtsc();
	#endif

	#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
		clock_gettime( CLOCK_REALTIME, &ts );
	#else
		QueryPerformanceCounter((LARGE_INTEGER*)&ts);
	#endif
}
#if defined(WIN32)
void timeStampRecord::qpcCalibrate(void)
{
		if (QueryPerformanceFrequency((LARGE_INTEGER*)&timeStampRecord::qpcFreq) == 0)
		{
			printf("QueryPerformanceFrequency FAILED!\n");
		}
}
#endif

class timeStampModule
{
	public:
	timeStampModule(void)
	{
		//printf("timeStampModuleInit\n");
	#if defined(WIN32)
		timeStampRecord::qpcCalibrate();
	#endif
	}
};

static timeStampModule module;

bool timeStampModuleInitialized(void)
{
#if defined(WIN32)
	bool initialized = timeStampRecord::countFreq() != 0;
#else
	bool initialized = true;
#endif
	return initialized;
}

void timeStampRecord::tscCalibrate(int numSamples)
{
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

		timeStampRecord::_tscFreq = static_cast<uint64_t>( td_avg / td.toSeconds() );

		printf("%i Calibration: %f sec   TSC:%llu   TSC Freq: %f MHz\n", i, td.toSeconds(), 
			static_cast<unsigned long long>(td.tsc), static_cast<double>(timeStampRecord::_tscFreq) * 1.0e-6 );
	}
}

} // namespace FCEU
