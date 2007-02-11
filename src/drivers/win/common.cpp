#include "common.h"

bool directoryExists(const char* dirname)
{
	DWORD res = GetFileAttributes(dirname);
	return res != 0xFFFFFFFF && res & FILE_ATTRIBUTE_DIRECTORY;
}

