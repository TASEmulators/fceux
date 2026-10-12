// mutex.cpp
#include <cstdio>

#if defined(WIN32)
#include <windows.h>
#include <direct.h>
#include <io.h>
#else
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#endif

#include "mutex.h"

namespace FCEU
{

//-----------------------------------------------------
// Cross platform mutex 
// __QT_DRIVER__  multi-threaded application that uses Qt mutex implementation for synchronization
// __WIN_DRIVER__ is single thread application so sync methods are unimplemented.
//-----------------------------------------------------
mutex::mutex(void)
{
#ifdef QT_USE_QMUTEX
	#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
	mtx = new QRecursiveMutex();
	#else
	mtx = new QMutex( QMutex::Recursive );
	#endif
#endif

}

mutex::~mutex(void)
{
#ifdef QT_USE_QMUTEX
	if (mtx)
	{
		delete mtx;
		mtx = nullptr;
	}
#endif
}

void mutex::lock(void)
{
#ifdef QT_USE_QMUTEX
	mtx->lock();
#else
	mtx.lock();
#endif
}

void mutex::unlock(void)
{
#ifdef QT_USE_QMUTEX
	mtx->unlock();
#else
	mtx.unlock();
#endif
}

bool mutex::tryLock()
{
	bool success = false;
#ifdef QT_USE_QMUTEX
	success = mtx->tryLock();
#else
	success = mtx.try_lock();
#endif
	return success;
}

static int msleep( int ms )
{
	int ret = 0;
	#ifdef WIN32
	Sleep(ms);
	#else
	ret = usleep(ms*1000);
	#endif
	return ret;
}

bool mutex::tryLock(int timeout)
{
	bool success = false;
#ifdef QT_USE_QMUTEX
	success = mtx->tryLock(timeout);
#else
	// std::mutex does not support timeout
	do
	{
		success = mtx.try_lock();
		if (success)
		{
			break;
		}
		msleep(1);
		timeout--;
	} while (timeout > 0);
#endif
	return success;
}

//-----------------------------------------------------
// Scoped AutoLock
//-----------------------------------------------------
autoScopedLock::autoScopedLock( mutex *mtx )
{
	m = mtx;
	if (m)
	{
		m->lock();
	}
}

autoScopedLock::autoScopedLock( mutex &mtx )
{
	m = &mtx;
	if (m)
	{
		m->lock();
	}
}

autoScopedLock::~autoScopedLock(void)
{
	if (m)
	{
		m->unlock();
	}
}

};
