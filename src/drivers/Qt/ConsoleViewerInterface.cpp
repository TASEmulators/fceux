// ConsoleViewerInterface.cpp
//
#include "Qt/ConsoleViewerInterface.h"

#include <limits>

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
QImage ConsoleViewerBase::copyLocalBufferToImage(const uint32_t *buffer, size_t bufferSize,
	int width, int height)
{
	if (!buffer || width <= 0 || height <= 0)
		return QImage();

	const size_t rowPixels = static_cast<size_t>(width);
	if (width > std::numeric_limits<int>::max() / static_cast<int>(sizeof(uint32_t)) ||
		static_cast<size_t>(height) > (bufferSize / sizeof(uint32_t)) / rowPixels)
		return QImage();

	QImage image(reinterpret_cast<const uchar *>(buffer), width, height,
		width * static_cast<int>(sizeof(uint32_t)), QImage::Format_ARGB32);
	return image.copy();
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
