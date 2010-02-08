#include "common.h"

bool directoryExists(const char* dirname)
{
	DWORD res = GetFileAttributes(dirname);
	return res != 0xFFFFFFFF && res & FILE_ATTRIBUTE_DIRECTORY;
}

void WindowBoundsCheckResize(int &windowPosX, int &windowPosY, int windowSizeX, long windowRight)
{
		if (windowRight < 59) {
		windowPosX = 59 - windowSizeX;
		}
		if (windowPosY < -18) {
		windowPosY = -18;
		} 
}

void WindowBoundsCheckNoResize(int &windowPosX, int &windowPosY, long windowRight)
{
		if (windowRight < 59) {
		windowPosX = 0;
		}
		if (windowPosY < -18) {
		windowPosY = -18;
		} 
}
