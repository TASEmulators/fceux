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
#if defined(WIN32)
uint64_t timeStampRecord::qpcFreq = 0;
#include <intrin.h>
#pragma intrinsic(__rdtsc)
#else
#include <x86intrin.h>
#endif

static uint64_t rdtsc()
{
	return __rdtsc();
}

namespace FCEU
{

uint64_t timeStampRecord::tscFreq = 0;

void timeStampRecord::readNew(void)
{
#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
	clock_gettime( CLOCK_REALTIME, &ts );
#else
	QueryPerformanceCounter((LARGE_INTEGER*)&ts);
#endif
	tsc = rdtsc();
}

static timeStampRecord cal_t1, cal_t2, cal_td;

class timeStampModule
{
	public:
	timeStampModule(void)
	{
		printf("timeStampModuleInit\n");
	#if defined(WIN32)
		if (QueryPerformanceFrequency((LARGE_INTEGER*)&StampRecord::qpcFreq) == 0)
		{
			printf("QueryPerformanceFrequency FAILED!\n");
		}
	#endif
	}
};

static timeStampModule module;

bool timeStampModuleInitialized(void)
{
	bool initialized = false;
#if defined(WIN32)
	initialized = timeStampRecord::qpcFreq != 0;
#else
	initialized = true;
#endif
	return initialized;
}

void timeStampModuleCalibrate(int numSamples)
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

		timeStampRecord::tscFreq = static_cast<uint64_t>( td_avg / td.toSeconds() );

		printf("%i Calibration: %f sec   TSC:%llu   TSC Freq: %f MHz\n", i, td.toSeconds(), 
			static_cast<unsigned long long>(td.tsc), static_cast<double>(timeStampRecord::tscFreq) * 1.0e-6 );
	}
}

} // namespace FCEU
