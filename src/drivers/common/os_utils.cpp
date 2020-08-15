// os_util.cpp
#include <stdio.h>
#include <stdlib.h>

#if defined(WIN32)
#include <windows.h>
#else
#include <errno.h>
#include <sys/stat.h>
#endif

#include "common/os_utils.h"
//************************************************************
int fceu_mkdir( const char *path )
{
	int retval;
#if defined(WIN32)
	retval = mkdir(path);
	chmod(path, 755);
#else
	retval = mkdir(path, S_IRWXU);

	if ( retval != 0 )
	{	
		if ( errno == EEXIST )
		{
			//printf("Path Exists: '%s'\n", path);
			retval = 0;
		}
	}
#endif
	return retval;
}
//************************************************************
int fceu_mkpath( const char *path )
{
	int i, retval = 0;
	char p[512];

	i=0;
	while ( path[i] != 0 )
	{
		if ( path[i] == '/' )
		{
			if ( i > 0 )
			{
				p[i] = 0;

				retval = fceu_mkdir( p );

				if ( retval )
				{
					return retval;
				}
			}
		}
		p[i] = path[i]; i++;
	}
	p[i] = 0;

	retval = fceu_mkdir( p );

	return retval;
}
//************************************************************
bool fceu_file_exists( const char *filepath )
{
#ifdef WIN32
	FILE *fp;
	fp = ::fopen( filename, "r" );

	if ( fp != NULL )
	{
		::fclose(fp);
		return true;
	}
#else
	struct stat sb;

	if ( stat( filepath, &sb ) == 0 )
	{
		return true;
	}
#endif
	return false;
}
//************************************************************
