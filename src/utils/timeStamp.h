// timeStamp.h
#pragma once

#include <stdint.h>

#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
#include <time.h>
#endif

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
				res = true;
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
			else if (ts.tv_sec < op.ts.tv_sec)
			{
				res = true;
			}
			return res;
		}

		void zero(void)
		{
			ts.tv_sec = 0;
			ts.tv_nsec = 0;
			tsc = 0;
		}

		void fromSeconds(unsigned int sec)
		{
			ts.tv_sec = sec;
			ts.tv_nsec = 0;
			tsc = 0;
		}

		double toSeconds(void)
		{
			double sec = static_cast<double>(ts.tv_sec) + ( static_cast<double>(ts.tv_nsec) * 1.0e-9 );
			return sec;
		}

		uint64_t toCounts(void)
		{
			return (ts.tv_sec * 1000000000) + ts.tv_nsec;
		}

		static uint64_t countFreq(void)
		{
			return 1000000000;
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

		void fromSeconds(unsigned int sec)
		{
			ts = sec * qpcFreq;
			tsc = 0;
		}

		double toSeconds(void)
		{
			double sec = static_cast<double>(ts) / static_cast<double>(qpcFreq);
			return sec;
		}

		uint64_t toCounts(void)
		{
			return ts;
		}

		static uint64_t countFreq(void)
		{
			return qpcFreq;
		}
		static uint64_t qpcFreq;
#endif
		static uint64_t tscFreq;

		static bool tscValid(void){ return tscFreq != 0; };

		void readNew(void);
	};

	bool timeStampModuleInitialized(void);

	// Call this function to calibrate the estimated TSC frequency
	void timeStampModuleCalibrate(int numSamples = 1);

} // namespace FCEU

