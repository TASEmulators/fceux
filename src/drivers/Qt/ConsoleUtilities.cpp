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
	static char filePath[2048];

	if ( GameInfo )
	{
		//printf("filename: '%s' \n", GameInfo->filename );
		//printf("archiveFilename: '%s' \n", GameInfo->archiveFilename );

		if ( GameInfo->archiveFilename != NULL )
		{
			char dir[1024], base[512], suffix[64];

			parseFilepath( GameInfo->archiveFilename, dir, base, suffix );

			filePath[0] = 0;

			if ( dir[0] != 0 )
			{
				strcat( filePath, dir );
			}

			parseFilepath( GameInfo->filename, dir, base, suffix );

			strcat( filePath, base   );
			strcat( filePath, suffix );

			//printf("ArchivePath: '%s' \n", filePath );

			return filePath;
		}
		else
		{
			return GameInfo->filename;
		}
	}
	return NULL;
}
//---------------------------------------------------------------------------
// Return file base name stripping out preceding path and trailing suffix.
int getFileBaseName( const char *filepath, char *base, char *suffix )
{
	int i=0,j=0,end=0;

	if ( suffix != NULL )
	{
		suffix[0] = 0;
	}
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
			if ( suffix != NULL )
			{
				strcpy( suffix, &base[j] );
			}
			end=j; base[j] = 0; break;
		}
	}
	return end;
}
//---------------------------------------------------------------------------
int parseFilepath( const char *filepath, char *dir, char *base, char *suffix )
{
	int i=0,j=0,end=0;

	if ( suffix != NULL )
	{
		suffix[0] = 0;
	}
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
// FCEU Data Entry Custom Validators
//---------------------------------------------------------------------------
fceuDecIntValidtor::fceuDecIntValidtor( int min, int max, QObject *parent)
     : QValidator(parent)
{
	this->min = min;
	this->max = max;
}
//---------------------------------------------------------------------------
void fceuDecIntValidtor::setMinMax( int min, int max)
{
	this->min = min;
	this->max = max;
}
//---------------------------------------------------------------------------
QValidator::State fceuDecIntValidtor::validate(QString &input, int &pos) const
{
   int i, v;
   //printf("Validate: %i '%s'\n", input.size(), input.toStdString().c_str() );

   if ( input.size() == 0 )
   {
      return QValidator::Acceptable;
   }
   std::string s = input.toStdString();
   i=0;

   if (s[i] == '-')
	{
		if ( min >= 0 )
		{
   		return QValidator::Invalid;
		}
		i++;
	}
	else if ( s[i] == '+' )
   {
      i++;
   }

   if ( s[i] == 0 )
   {
      return QValidator::Acceptable;
   }

   if ( isdigit(s[i]) )
   {
      while ( isdigit(s[i]) ) i++;

      if ( s[i] == 0 )
      {
			v = strtol( s.c_str(), NULL, 0 );

			if ( v < min )
			{
   			return QValidator::Invalid;
			}
			else if ( v > max )
			{
   			return QValidator::Invalid;
			}
         return QValidator::Acceptable;
      }
   }
   return QValidator::Invalid;
}
//---------------------------------------------------------------------------
// FCEU Data Entry Custom Validators
//---------------------------------------------------------------------------
fceuHexIntValidtor::fceuHexIntValidtor( int min, int max, QObject *parent)
     : QValidator(parent)
{
	this->min = min;
	this->max = max;
}
//---------------------------------------------------------------------------
void fceuHexIntValidtor::setMinMax( int min, int max)
{
	this->min = min;
	this->max = max;
}
//---------------------------------------------------------------------------
QValidator::State fceuHexIntValidtor::validate(QString &input, int &pos) const
{
   int i, v;
   //printf("Validate: %i '%s'\n", input.size(), input.toStdString().c_str() );

   if ( input.size() == 0 )
   {
      return QValidator::Acceptable;
   }
	input = input.toUpper();
   std::string s = input.toStdString();
   i=0;

   if (s[i] == '-')
	{
		if ( min >= 0 )
		{
   		return QValidator::Invalid;
		}
		i++;
	}
	else if ( s[i] == '+' )
   {
      i++;
   }

   if ( s[i] == 0 )
   {
      return QValidator::Acceptable;
   }

   if ( isxdigit(s[i]) )
   {
      while ( isxdigit(s[i]) ) i++;

      if ( s[i] == 0 )
      {
			v = strtol( s.c_str(), NULL, 16 );

			if ( v < min )
			{
   			return QValidator::Invalid;
			}
			else if ( v > max )
			{
   			return QValidator::Invalid;
			}
         return QValidator::Acceptable;
      }
   }
   return QValidator::Invalid;
}
//---------------------------------------------------------------------------
