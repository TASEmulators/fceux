/* FCE Ultra - NES/Famicom Emulator
 *
 * Copyright notice for this file:
 *  Copyright (C) 2001 Aaron Oneal
 *  Copyright (C) 2002 Xodnizel
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */  

#ifndef __FCEU_TYPES
#define __FCEU_TYPES

#include <string>
#include <stdlib.h>

#define FCEU_VERSION_NUMERIC 19901
#define FCEU_NAME "FCE Ultra"
#define FCEU_VERSION_STRING "1.9.9"
#define FCEU_NAME_AND_VERSION FCEU_NAME " " FCEU_VERSION_STRING

///causes the code fragment argument to be compiled in if the build includes debugging
#ifdef FCEUDEF_DEBUGGER
#define DEBUG(X) X;
#else
#define DEBUG(X)
#endif

#ifdef MSVC
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
typedef signed char int8;
typedef signed short int16;
typedef signed int int32;
#include <sys/types.h>
#include <sys/stat.h>
#include <io.h>
#include <direct.h>
#include <malloc.h>
#define dup _dup
#define stat _stat
#define fstat _fstat
#define mkdir _mkdir
#define alloca _alloca
#define snprintf _snprintf
#define vsnprintf _vsnprintf
#define W_OK 2
#define R_OK 2
#define X_OK 1
#define F_OK 0
#define PATH_MAX 260
#else

//mingw32 doesnt prototype this for some reason
#ifdef __MINGW32__
void *alloca(size_t);
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <inttypes.h>
typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
#endif

#ifdef __GNUC__
 typedef unsigned long long uint64;
 typedef long long int64;
 #define INLINE inline
 #define GINLINE inline
#elif MSVC
 typedef __int64 int64;
 typedef unsigned __int64 uint64;
 #define __restrict__ 
 #define INLINE __inline
 #define GINLINE			/* Can't declare a function INLINE
					   and global in MSVC.  Bummer.
					*/
 #define PSS_STYLE 2			/* Does MSVC compile for anything
					   other than Windows/DOS targets?
					*/

 #if _MSC_VER >= 1300 
  #pragma warning(disable:4244) //warning C4244: '=' : conversion from 'uint32' to 'uint8', possible loss of data
  #pragma warning(disable:4996) //'strdup' was declared deprecated
#endif

 #if _MSC_VER < 1400
  #define vsnprintf _vsnprintf
 #endif
#endif

#if PSS_STYLE==2

#define PSS "\\"
#define PS '\\'

#elif PSS_STYLE==1

#define PSS "/"
#define PS '/'

#elif PSS_STYLE==3

#define PSS "\\"
#define PS '\\'

#elif PSS_STYLE==4

#define PSS ":"
#define PS ':'

#endif


typedef void (*writefunc)(uint32 A, uint8 V);
typedef uint8 (*readfunc)(uint32 A);


template<typename T, int N>
struct ValueArray
{
	T data[N];
	T &operator[](int index) { return data[index]; }
	static const int size = N;
	bool operator!=(ValueArray<T,N> &other) { return !operator==(other); }
	bool operator==(ValueArray<T,N> &other)
	{
		for(int i=0;i<size;i++)
			if(data[i] != other[i])
				return false;
		return true;
	}
};

#include "utils/endian.h"


struct FCEU_Guid : public ValueArray<uint8,16>
{
	void newGuid()
	{
		for(int i=0;i<size;i++)
			data[i] = rand();
	}

	std::string toString()
	{
		char buf[37];
		sprintf(buf,"%08X-%04X-%04X-%04X-%02X%02X%02X%02X%02X%02X",
			FCEU_de32lsb(data),FCEU_de16lsb(data+4),FCEU_de16lsb(data+6),FCEU_de16lsb(data+8),data[10],data[11],data[12],data[13],data[14],data[15]);
		return std::string(buf);
	}

	static FCEU_Guid fromString(std::string str)
	{
		FCEU_Guid ret;
		ret.scan(str);
		return ret;
	}

	static uint8 hexToByte(char** ptrptr)
	{
		char a = toupper(**ptrptr);
		(*ptrptr)++;
		char b = toupper(**ptrptr);
		(*ptrptr)++;
		if(a>='A') a=a-'A'+10;
		else a-='0';
		if(b>='A') b=b-'A'+10;
		else b-='0';
		return ((unsigned char)a<<4)|(unsigned char)b; 
	}

	void scan(std::string str)
	{
		char* endptr = (char*)str.c_str();
		FCEU_en32lsb(data,strtoul(endptr,&endptr,16));
		FCEU_en16lsb(data+4,strtoul(endptr+1,&endptr,16));
		FCEU_en16lsb(data+6,strtoul(endptr+1,&endptr,16));
		FCEU_en16lsb(data+8,strtoul(endptr+1,&endptr,16));
		endptr++;
		for(int i=0;i<6;i++)
			data[10+i] = hexToByte(&endptr);
	}
};

#endif
