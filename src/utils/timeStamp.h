// timeStamp.h
#pragma once

#include <stdint.h>

#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
#include <time.h>
#endif

namespace FCEU
{
	class timeStampRecord
	{
		public:
		static constexpr uint64_t ONE_SEC_TO_MILLI = 1000;

#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
		static constexpr long int ONE_SEC_TO_NANO  = 1000000000;
		static constexpr long int MILLI_TO_NANO    = 1000000;

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

			if (ts.tv_nsec >= ONE_SEC_TO_NANO)
			{
				ts.tv_nsec -= ONE_SEC_TO_NANO;
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

			if (res.ts.tv_nsec >= ONE_SEC_TO_NANO)
			{
				res.ts.tv_nsec -= ONE_SEC_TO_NANO;
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
				res.ts.tv_nsec += ONE_SEC_TO_NANO;
				res.ts.tv_sec--;	
			}
			res.tsc = tsc - op.tsc;

			return res;
		}

		timeStampRecord operator * (const unsigned int multiplier)
		{
			timeStampRecord res;

			res.ts.tv_sec  = ts.tv_sec * multiplier;
			res.ts.tv_nsec = ts.tv_nsec * multiplier;

			if (res.ts.tv_nsec >= ONE_SEC_TO_NANO)
			{
				res.ts.tv_nsec -= ONE_SEC_TO_NANO;
				res.ts.tv_sec++;
			}
			res.tsc = tsc * multiplier;

			return res;
		}

		timeStampRecord operator / (const unsigned int divisor)
		{
			timeStampRecord res;

			res.ts.tv_sec  = ts.tv_sec / divisor;
			res.ts.tv_nsec = ts.tv_nsec / divisor;
			res.tsc = tsc / divisor;

			return res;
		}

		bool operator > (const timeStampRecord& op)
		{
			bool res;
			if (ts.tv_sec == op.ts.tv_sec)
			{
				res = (ts.tv_nsec > op.ts.tv_nsec);
			}
			else
			{
				res = (ts.tv_sec > op.ts.tv_sec);
			}
			return res;
		}
		bool operator >= (const timeStampRecord& op)
		{
			bool res;
			if (ts.tv_sec == op.ts.tv_sec)
			{
				res = (ts.tv_nsec >= op.ts.tv_nsec);
			}
			else
			{
				res = (ts.tv_sec >= op.ts.tv_sec);
			}
			return res;
		}

		bool operator < (const timeStampRecord& op)
		{
			bool res;
			if (ts.tv_sec == op.ts.tv_sec)
			{
				res = (ts.tv_nsec < op.ts.tv_nsec);
			}
			else
			{
				res = (ts.tv_sec < op.ts.tv_sec);
			}
			return res;
		}
		bool operator <= (const timeStampRecord& op)
		{
			bool res;
			if (ts.tv_sec == op.ts.tv_sec)
			{
				res = (ts.tv_nsec <= op.ts.tv_nsec);
			}
			else
			{
				res = (ts.tv_sec <= op.ts.tv_sec);
			}
			return res;
		}

		void zero(void)
		{
			ts.tv_sec = 0;
			ts.tv_nsec = 0;
			tsc = 0;
		}

		bool isZero(void)
		{
			return (ts.tv_sec == 0) && (ts.tv_nsec == 0);
		}

		void fromSeconds(unsigned int sec)
		{
			ts.tv_sec = sec;
			ts.tv_nsec = 0;
			tsc = 0;
		}

		void fromSeconds(double sec)
		{
			double ns;
			ts.tv_sec = static_cast<time_t>(sec);
			ns = (sec - static_cast<double>(ts.tv_sec)) * 1.0e9;
			ts.tv_nsec = static_cast<long>(ns);
			tsc = 0;
		}

		double toSeconds(void)
		{
			double sec = static_cast<double>(ts.tv_sec) + ( static_cast<double>(ts.tv_nsec) * 1.0e-9 );
			return sec;
		}

		void fromMilliSeconds(uint64_t ms)
		{
			ts.tv_sec = ms / ONE_SEC_TO_MILLI;
			ts.tv_nsec = (ms * MILLI_TO_NANO) - (ts.tv_sec * ONE_SEC_TO_NANO);
		}

		uint64_t toMilliSeconds(void)
		{
			uint64_t ms = (ts.tv_sec * ONE_SEC_TO_MILLI) + (ts.tv_nsec / MILLI_TO_NANO );
			return ms;
		}

		uint64_t toCounts(void)
		{
			return (ts.tv_sec * ONE_SEC_TO_NANO) + ts.tv_nsec;
		}

		static uint64_t countFreq(void)
		{
			return ONE_SEC_TO_NANO;
		}

		struct timespec toTimeSpec(void)
		{
			return ts;
		}
#else	// WIN32

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

		timeStampRecord operator * (const unsigned int multiplier)
		{
			timeStampRecord res;

			res.ts  = ts  * multiplier;
			res.tsc = tsc * multiplier;

			return res;
		}

		timeStampRecord operator / (const unsigned int divisor)
		{
			timeStampRecord res;

			res.ts  = ts  / divisor;
			res.tsc = tsc / divisor;

			return res;
		}

		bool operator > (const timeStampRecord& op)
		{
			return ts > op.ts;
		}
		bool operator >= (const timeStampRecord& op)
		{
			return ts >= op.ts;
		}

		bool operator < (const timeStampRecord& op)
		{
			return ts < op.ts;
		}
		bool operator <= (const timeStampRecord& op)
		{
			return ts <= op.ts;
		}

		void zero(void)
		{
			ts = 0;
			tsc = 0;
		}

		bool isZero(void)
		{
			return (ts == 0);
		}


		void fromSeconds(unsigned int sec)
		{
			ts = sec * qpcFreq;
			tsc = 0;
		}

		void fromSeconds(double sec)
		{
			ts = static_cast<uint64_t>(sec * static_cast<double>(qpcFreq));
			tsc = 0;
		}

		double toSeconds(void)
		{
			double sec = static_cast<double>(ts) / static_cast<double>(qpcFreq);
			return sec;
		}

		void fromMilliSeconds(uint64_t ms)
		{
			ts = (ms * qpcFreq) / ONE_SEC_TO_MILLI;
		}

		uint64_t toMilliSeconds(void)
		{
			uint64_t ms = (ts * ONE_SEC_TO_MILLI) / qpcFreq;
			return ms;
		}

		uint64_t toCounts(void)
		{
			return ts;
		}

		static uint64_t countFreq(void)
		{
			return qpcFreq;
		}

		static void qpcCalibrate(void);
#endif

		uint64_t getTSC(void){ return tsc; };

		static uint64_t tscFreq(void)
		{
			return _tscFreq;
		}
		static bool tscValid(void){ return _tscFreq != 0; };

		// Call this function to calibrate the estimated TSC frequency
		static void tscCalibrate(int numSamples = 0);

		void readNew(void);

		private:
#if defined(__linux__) || defined(__APPLE__) || defined(__unix__)
		struct timespec ts;
#else // Win32
		uint64_t ts;
		static uint64_t qpcFreq;
#endif
		uint64_t tsc;
		static uint64_t _tscFreq;
	};

	bool timeStampModuleInitialized(void);

} // namespace FCEU

