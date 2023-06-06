// mutex.cpp
#include <cstdio>

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
#ifdef __QT_DRIVER__
	#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
	mtx = new QRecursiveMutex();
	#else
	mtx = new QMutex( QMutex::Recursive );
	#endif
#endif

}

mutex::~mutex(void)
{
#ifdef __QT_DRIVER__
	if (mtx)
	{
		delete mtx;
		mtx = nullptr;
	}
#endif
}

void mutex::lock(void)
{
#ifdef __QT_DRIVER__
	mtx->lock();
#endif
}

void mutex::unlock(void)
{
#ifdef __QT_DRIVER__
	mtx->unlock();
#endif
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
