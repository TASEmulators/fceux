// ConsoleViewerInterface.cpp
//
#include "Qt/ConsoleViewerInterface.h"

//----------------------------------------------------------
void ConsoleViewerBase::memset32( void *buf, uint32_t val, size_t size)
{
	uint32_t *p = static_cast<uint32_t*>(buf);
	size_t n = size / sizeof(uint32_t);

	for (size_t i=0; i<n; i++)
	{
		*p = val; p++;
	}
}
//----------------------------------------------------------
void ConsoleViewerBase::copyPixels32( void *dest, void *src, size_t size, uint32_t alphaMask)
{
	uint32_t *d = static_cast<uint32_t*>(dest);
	uint32_t *s = static_cast<uint32_t*>(src);
	size_t n = size / sizeof(uint32_t);

	for (size_t i=0; i<n; i++)
	{
		*d = *s | alphaMask; d++; s++;
	}
}
//----------------------------------------------------------
