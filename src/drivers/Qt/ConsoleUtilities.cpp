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
// Return file base name stripping out preceding path and trailing suffix.
int getFileBaseName( const char *filepath, char *base )
{
	int i=0,j=0,end=0;
	if ( filepath == NULL )
	{
		base[0] = 0;
		return 0;
	}
	i=0; j=0;
	while ( filepath[i] != 0 )
	{
		if ( (filepath[i] == '/') || (filepath[i] == '\\') )
		{
			j = i+1;
		}
		i++;
	}
	i = j;

	j=0;
	while ( filepath[i] != 0 )
	{
		base[j] = filepath[i]; i++; j++;
	}
	base[j] = 0; end=j;

	while ( j > 1 )
	{
		j--;
		if ( base[j] == '.' )
		{
			end=j; base[j] = 0; break;
		}
	}
	return end;
}
//---------------------------------------------------------------------------
int parseFilepath( const char *filepath, char *dir, char *base, char *suffix )
{
	int i=0,j=0,end=0;
	if ( filepath == NULL )
	{
		if ( dir   ) dir[0] = 0;
		if ( base  ) base[0] = 0;
		if ( suffix) suffix[0] = 0;
		return 0;
	}
	i=0; j=0;
	while ( filepath[i] != 0 )
	{
		if ( (filepath[i] == '/') || (filepath[i] == '\\') )
		{
			j = i+1;
		}
		if ( dir )
		{
			dir[i] = filepath[i];
		}
		i++;
	}
	if ( dir )
	{
		dir[j] = 0;
	}
	i = j;

	if ( base == NULL )
	{
		return end;
	}

	j=0;
	while ( filepath[i] != 0 )
	{
		base[j] = filepath[i]; i++; j++;
	}
	base[j] = 0; end=j;

	if ( suffix )
	{
		suffix[0] = 0;
	}

	while ( j > 1 )
	{
		j--;
		if ( base[j] == '.' )
		{
			if ( suffix )
			{
				strcpy( suffix, &base[j] );
			}
			end=j; base[j] = 0; 
			break;
		}
	}
	return end;
}
//---------------------------------------------------------------------------
