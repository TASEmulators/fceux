// mutex.h
#pragma once

#if defined(__QT_DRIVER__) && !defined(__SANITIZE_THREAD__)
#define QT_USE_QMUTEX
#endif

#ifdef QT_USE_QMUTEX

#include <QMutex>
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
#include <QRecursiveMutex>
#endif

#else

#include <mutex> // fallback to std::mutex

#endif

namespace FCEU
{
	class mutex
	{
		public:
			mutex(void);
			~mutex(void);
	
			void lock(void);
			void unlock(void);
			bool tryLock(void);
			bool tryLock(int timeout);
	
		private:
	#ifdef QT_USE_QMUTEX
		#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
			QRecursiveMutex *mtx;
		#else
			QMutex *mtx;
		#endif
	#else
			std::recursive_mutex mtx;
	#endif
	};

	class autoScopedLock
	{
		public:
			autoScopedLock( mutex *mtx );
			autoScopedLock( mutex &mtx );
			~autoScopedLock(void);

		private:
			mutex *m;
	};

};
