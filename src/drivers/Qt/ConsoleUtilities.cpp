/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2020 mjbudd77
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
// ConsoleUtilities.cpp
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if WIN32
#include <Windows.h>
#include <direct.h>
#else
// unix, linux, or apple
#include <unistd.h>
#include <libgen.h>

#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif

#endif

#include "../../fceu.h"
#include "../../x6502.h"
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
//  Returns the path of fceux.exe as a string.
int fceuExecutablePath( char *outputPath, int outputSize )
{
	if ( (outputPath == NULL) || (outputSize <= 0) )
	{
		return -1;
	}
	outputPath[0] = 0;

#ifdef WIN32
	char fullPath[2048];
	char driveLetter[3];
	char directory[2048];
	char finalPath[2048];

	GetModuleFileNameA(NULL, fullPath, 2048);
	_splitpath(fullPath, driveLetter, directory, NULL, NULL);
	snprintf(finalPath, sizeof(finalPath), "%s%s", driveLetter, directory);
	strncpy( outputPath, finalPath, outputSize );
	outputPath[outputSize-1] = 0;

	return 0;
#elif __linux__ || __unix__
	char exePath[ 2048 ];
	ssize_t count = ::readlink( "/proc/self/exe", exePath, sizeof(exePath)-1 );

	if ( count > 0 )
	{
		char *dir;
		exePath[count] = 0;
		//printf("EXE Path: '%s' \n", exePath );

		dir = ::dirname( exePath );

		if ( dir )
		{
			//printf("DIR Path: '%s' \n", dir );
			strncpy( outputPath, dir, outputSize );
			outputPath[outputSize-1] = 0;
			return 0;
		}
	}
#elif	  __APPLE__
	char exePath[ 2048 ];
	uint32_t bufSize = sizeof(exePath);
	int result = _NSGetExecutablePath( exePath, &bufSize );

	if ( result == 0 )
	{
		char *dir;
		exePath[ sizeof(exePath)-1 ] = 0;
		//printf("EXE Path: '%s' \n", exePath );

		dir = ::dirname( exePath );

		if ( dir )
		{
			//printf("DIR Path: '%s' \n", dir );
			strncpy( outputPath, dir, outputSize );
			outputPath[outputSize-1] = 0;
			return 0;
		}
	}
#endif
	return -1;
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
//---  Opcode Tool Tip Description
//---------------------------------------------------------------------------
QString fceuGetOpcodeToolTip( uint8_t *opcode, int size )
{
	std::string text;
	const char *title = "";
	const char *addrMode = NULL;
	char stmp[32];

	switch (opcode[0])
	{
		// ADC - Add with Carry
		case 0x69:
		case 0x65:
		case 0x75:
		case 0x6D:
		case 0x7D:
		case 0x79:
		case 0x61:
		case 0x71:
			title = "ADC - Add with Carry";

			if ( opcode[0] == 0x69 )
			{
				addrMode = "Immediate";	
			}
		break;
		// AND - Logical AND
		case 0x29:
		case 0x25:
		case 0x35:
		case 0x2D:
		case 0x3D:
		case 0x39:
		case 0x21:
		case 0x31:
			title = "AND - Logical AND";

			if ( opcode[0] == 0x29 )
			{
				addrMode = "Immediate";	
			}
		break;
		// ASL - Arithmetic Shift Left
		case 0x0A:
		case 0x06:
		case 0x16:
		case 0x0E:
		case 0x1E:
			title = "ASL - Arithmetic Shift Left";

			if ( opcode[0] == 0x0A )
			{
				addrMode = "Accumulator";	
			}
		break;
		// BCC - Branch if Carry Clear
		case 0x90:
			title = "BCC - Branch if Carry Clear";

			if ( opcode[0] == 0x90 )
			{
				addrMode = "Relative";	
			}
		break;
		// BCS - Branch if Carry Set
		case 0xB0:
			title = "BCC - Branch if Carry Set";

			if ( opcode[0] == 0xB0 )
			{
				addrMode = "Relative";	
			}
		break;
		// BEQ - Branch if Equal
		case 0xF0:
			title = "BCC - Branch if Equal";

			if ( opcode[0] == 0xF0 )
			{
				addrMode = "Relative";	
			}
		break;
		// BIT - Bit Test
		case 0x24:
		case 0x2C:
			title = "BIT - Bit Test";
		break;
		// BMI - Branch if Minus
		case 0x30:
			title = "BMI - Branch if Minus";

			if ( opcode[0] == 0x30 )
			{
				addrMode = "Relative";	
			}
		break;
		// BNE - Branch if Not Equal
		case 0xD0:
			title = "BNE - Branch if Not Equal";

			if ( opcode[0] == 0xD0 )
			{
				addrMode = "Relative";	
			}
		break;
		// BPL - Branch if Positive
		case 0x10:
			title = "BPL - Branch if Positive";

			if ( opcode[0] == 0x10 )
			{
				addrMode = "Relative";	
			}
		break;
		// BRK - Force Interrupt
		case 0x00:
			title = "BRK - Force Interrupt";
		break;
		// BVC - Branch if Overflow Clear
		case 0x50:
			title = "BVC - Branch if Overflow Clear";

			if ( opcode[0] == 0x50 )
			{
				addrMode = "Relative";	
			}
		break;
		// BVS - Branch if Overflow Set
		case 0x70:
			title = "BVS - Branch if Overflow Set";

			if ( opcode[0] == 0x70 )
			{
				addrMode = "Relative";	
			}
		break;
		// CLC - Clear Carry Flag
		case 0x18:
			title = "CLC - Clear Carry Flag";
		break;
		// CLD - Clear Decimal Mode
		case 0xD8:
			title = "CLD - Clear Decimal Mode";
		break;
		// CLI - Clear Interrupt Disable
		case 0x58:
			title = "CLI - Clear Interrupt Disable";
		break;
		// CLV - Clear Overflow Flag
		case 0xB8:
			title = "CLV - Clear Overflow Flag";
		break;
		// CMP - Compare
		case 0xC9:
		case 0xC5:
		case 0xD5:
		case 0xCD:
		case 0xDD:
		case 0xD9:
		case 0xC1:
		case 0xD1:
			title = "CMP - Compare";

			if ( opcode[0] == 0xC9 )
			{
				addrMode = "Immediate";	
			}
		break;
		default:
			title = "";
		break;
	}

	if ( addrMode == NULL )
	{
		switch ( optype[ opcode[0] ] )
		{
			default:
			case 0: //Implied\Accumulator\Immediate\Branch\NULL
				addrMode = "Implied";
			break;
			case 1: // (Indirect,X)
				addrMode = "(Indirect,X)";
			break;
			case 2: // Zero Page
				addrMode = "Zero Page";
			break;
			case 3: // Absolute
				addrMode = "Absolute";
			break;
			case 4: // (Indirect),Y
				addrMode = "(Indirect),Y";
			break;
			case 5: // Zero Page,X
				addrMode = "Zero Page,X";
			break;
			case 6: // Absolute,Y
				addrMode = "Absolute,Y";
			break;
			case 7: // Absolute,X
				addrMode = "Absolute,X";
			break;
			case 8: // Zero Page,Y
				addrMode = "Zero Page,Y";
			break;
		}
	}
	text.assign( title );
	text.append( "\n" );
	text.append( "\nByte Code:\t\t" );

	for (int i=0; i<size; i++)
	{
		sprintf(stmp, "$%02X  ", opcode[i] );

		text.append( stmp );
	}
	text.append( "\nAddressing Mode:\t" );
	text.append( addrMode );

	text.append( "\nCycle Count:\t\t" );
	sprintf( stmp, "%i", X6502_GetOpcodeCycles( opcode[0] ) );
	text.append( stmp );
	text.append( "\n" );

	return QString::fromStdString( text );
}
//---------------------------------------------------------------------------
