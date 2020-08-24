// ConsoleUtilities.cpp
#include <stdio.h>
#include <string.h>

#include "../../fceu.h"
#include "Qt/ConsoleUtilities.h"

//---------------------------------------------------------------------------
int  getDirFromFile( const char *path, char *dir )
{
	int i, lastSlash = -1, lastPeriod = -1;

	i=0; 
	while ( path[i] != 0 )
	{
		if ( path[i] == '/' )
		{
			lastSlash = i;
		}
		else if ( path[i] == '.' )
		{
			lastPeriod = i;
		}
		dir[i] = path[i]; i++;
	}
	dir[i] = 0;

	if ( lastPeriod >= 0 )
	{
		if ( lastPeriod > lastSlash )
		{
			dir[lastSlash] = 0;
		}
	}

	return 0;
}
//---------------------------------------------------------------------------
const char *getRomFile( void )
{
	if ( GameInfo )
	{
		return GameInfo->filename;
	}
	return NULL;
}
//---------------------------------------------------------------------------
// TODO Finish writing this func
int getFileBaseName( const char *filepath, char *base )
{
	int i,j=0, start=0;
	if ( filepath == NULL )
	{
		return 0;
	}
	i=0; j=0;
	while ( filepath[i] != 0 )
	{
		if ( filepath[i] == '/' )
		{
			j = i;
		}
		i++;
	}
	start = j;

	return 0;
}
//---------------------------------------------------------------------------
