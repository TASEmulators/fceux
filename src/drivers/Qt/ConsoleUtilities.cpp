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
	const char *synopsis = NULL;
	const char *addrMode = NULL;
	const char *longDesc = NULL;
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
			synopsis = "A,Z,C,N = A+M+C";

			longDesc = "Add the value at the specified memory address to the accumulator + the carry bit. On overflow, the carry bit is set.";
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
			synopsis = "A,Z,N = A&M";

			longDesc = "Perform an AND operation between the accumulator and the value at the specified memory address.";
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
			synopsis = "A,Z,C,N = M*2    or    M,Z,C,N = M*2";

			longDesc = "Shifts all the bits of the accumulator (or the byte at the specified memory address) by 1 bit to the left. Bit 0 will be set to 0 and the carry flag (C) will take the value of bit 7 (before the shift).";
		break;
		// BCC - Branch if Carry Clear
		case 0x90:
			title = "BCC - Branch if Carry Clear";

			if ( opcode[0] == 0x90 )
			{
				addrMode = "Relative";	
			}

			longDesc = "If the carry flag (C) is clear, jump to location specified.";
		break;
		// BCS - Branch if Carry Set
		case 0xB0:
			title = "BCS - Branch if Carry Set";

			if ( opcode[0] == 0xB0 )
			{
				addrMode = "Relative";	
			}

			longDesc = "If the carry flag (C) is set, jump to the location specified.";
		break;
		// BEQ - Branch if Equal
		case 0xF0:
			title = "BEQ - Branch if Equal";

			if ( opcode[0] == 0xF0 )
			{
				addrMode = "Relative";	
			}

			longDesc = "If the zero flag (Z) is set, jump to the location specified.";
		break;
		// BIT - Bit Test
		case 0x24:
		case 0x2C:
			title = "BIT - Bit Test";

			synopsis = "A & M, N = M7, V = M6";

			longDesc = "Bits 6 and 7 of the byte at the specified memory address are copied to the negative (N) and overflow (V) flags."
				"If the accumulator's value ANDed with that byte is 0, the zero flag (Z) is set (otherwise it is cleared).";
		break;
		// BMI - Branch if Minus
		case 0x30:
			title = "BMI - Branch if Minus";

			addrMode = "Relative";	

			longDesc = "If the negative flag (N) is set, jump to the location specified.";
		break;
		// BNE - Branch if Not Equal
		case 0xD0:
			title = "BNE - Branch if Not Equal";

			addrMode = "Relative";	

			longDesc = "If the zero flag (Z) is clear, jump to the location specified.";
		break;
		// BPL - Branch if Positive
		case 0x10:
			title = "BPL - Branch if Positive";

			addrMode = "Relative";	

			longDesc = "If the negative flag (N) is clear, jump to the location specified.";
		break;
		// BRK - Force Interrupt
		case 0x00:
			title = "BRK - Force Interrupt";

			longDesc = "The BRK instruction causes the CPU to jump to its IRQ vector, as if an interrupt had occurred. The PC and status flags are pushed on the stack.";
		break;
		// BVC - Branch if Overflow Clear
		case 0x50:
			title = "BVC - Branch if Overflow Clear";

			addrMode = "Relative";	

			longDesc = "If the overflow flag (V) is clear, jump to the location specified.";
		break;
		// BVS - Branch if Overflow Set
		case 0x70:
			title = "BVS - Branch if Overflow Set";

			addrMode = "Relative";	

			longDesc = "If the overflow flag (V) is set then, jump to the location specified.";
		break;
		// CLC - Clear Carry Flag
		case 0x18:
			title = "CLC - Clear Carry Flag";

			longDesc = "Clears the carry flag (C).";
		break;
		// CLD - Clear Decimal Mode
		case 0xD8:
			title = "CLD - Clear Decimal Mode";

			longDesc = "Clears the decimal mode flag (D).";
		break;
		// CLI - Clear Interrupt Disable
		case 0x58:
			title = "CLI - Clear Interrupt Disable";

			longDesc = "Clears the interrupt disable flag (I).";
		break;
		// CLV - Clear Overflow Flag
		case 0xB8:
			title = "CLV - Clear Overflow Flag";

			longDesc = "Clears the overflow flag (V).";
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

			longDesc = "Compares the accumulator with the byte at the specified memory address..";
		break;
		// CPX - Compare X Register
		case 0xE0:
		case 0xE4:
		case 0xEC:
			title = "CPX - Compare X Register";

			if ( opcode[0] == 0xE0 )
			{
				addrMode = "Immediate";	
			}

			longDesc = "Compares the X register with the byte at the specified memory address.";
		break;
		// CPY - Compare Y Register
		case 0xC0:
		case 0xC4:
		case 0xCC:
			title = "CPY - Compare Y Register";

			if ( opcode[0] == 0xE0 )
			{
				addrMode = "Immediate";	
			}

			longDesc = "Compares the Y register with the byte at the specified memory address.";
		break;
		// DEC - Decrement Memory
		case 0xC6:
		case 0xD6:
		case 0xCE:
		case 0xDE:
			title = "DEC - Decrement Memory";

			longDesc = "Subtracts one from the byte at the specified memory address.";
		break;
		// DEX - Decrement X Register
		case 0xCA:
			title = "DEX - Decrement X Register";

			longDesc = "Subtracts one from the X register.";
		break;
		// DEY - Decrement Y Register
		case 0x88:
			title = "DEY - Decrement Y Register";

			longDesc = "Subtracts one from the Y register.";
		break;
		// EOR - Exclusive OR
		case 0x49:
		case 0x45:
		case 0x55:
		case 0x4D:
		case 0x5D:
		case 0x59:
		case 0x41:
		case 0x51:
			title = "EOR - Exclusive OR";

			if ( opcode[0] == 0x49 )
			{
				addrMode = "Immediate";	
			}

			longDesc = "Performs an exclusive OR operation between the accumulator and the byte at the specified memory address.";
		break;
		// INC - Increment Memory
		case 0xE6:
		case 0xF6:
		case 0xEE:
		case 0xFE:
			title = "INC - Decrement Memory";

			longDesc = "Adds one to the the byte at the specified memory address.";
		break;
		// INX - Increment X Register
		case 0xE8:
			title = "INX - Increment X Register";

			longDesc = "Adds one to the X register.";
		break;
		// INY - Increment Y Register
		case 0xC8:
			title = "INY - Increment Y Register";

			longDesc = "Adds one to the Y register.";
		break;
		// JMP - Jump
		case 0x4C:
		case 0x6C:
			title = "JMP - Jump";

			longDesc = "Jumps to the specified location (alters the program counter)";
		break;
		// JSR - Jump to Subroutine
		case 0x20:
			title = "JSR - Jump to Subroutine";

			longDesc = "Pushes the address (minus one) of the next instruction to the stack and then jumps to the target address.";
		break;
		// LDA - Load Accumulator
		case 0xA9:
		case 0xA5:
		case 0xB5:
		case 0xAD:
		case 0xBD:
		case 0xB9:
		case 0xA1:
		case 0xB1:
			title = "LDA - Load Accumulator";

			if ( opcode[0] == 0xA9 )
			{
				addrMode = "Immediate";	
			}

			longDesc = "Loads a byte from the specified memory address into the accumulator.";
		break;
		// LDX - Load X Register
		case 0xA2:
		case 0xA6:
		case 0xB6:
		case 0xAE:
		case 0xBE:
			title = "LDX - Load X Register";

			if ( opcode[0] == 0xA2 )
			{
				addrMode = "Immediate";	
			}

			longDesc = "Loads a byte from the specified memory address into the X register.";
		break;
		// LDY - Load Y Register
		case 0xA0:
		case 0xA4:
		case 0xB4:
		case 0xAC:
		case 0xBC:
			title = "LDY - Load Y Register";

			if ( opcode[0] == 0xA0 )
			{
				addrMode = "Immediate";	
			}

			longDesc = "Loads a byte from the specified memory address into the Y register.";
		break;
		// LSR - Logical Shift Right
		case 0x4A:
		case 0x46:
		case 0x56:
		case 0x4E:
		case 0x5E:
			title = "LSR - Logical Shift Right";

			if ( opcode[0] == 0x4A )
			{
				addrMode = "Accumulator";	
			}
			longDesc = "Shifts all the bits of the accumulator (or the byte at the specified memory address) by 1 bit to the right. Bit 7 will be set to 0 and the carry flag (C) will take the value of bit 0 (before the shift).";
		break;
		// NOP - No Operation
		case 0xEA:
			title = "NOP - No Operation";

			longDesc = "Performs no operation other than delaying execution of the next instruction by 2 cycles.";
		break;
		// ORA - Logical Inclusive OR
		case 0x09:
		case 0x05:
		case 0x15:
		case 0x0D:
		case 0x1D:
		case 0x19:
		case 0x01:
		case 0x11:
			title = "ORA - Logical Inclusive OR";

			if ( opcode[0] == 0x09 )
			{
				addrMode = "Immediate";	
			}

			longDesc = "Performs an inclusive OR operation between the accumulator and the byte at the specified memory address.";
		break;
		// PHA - Push Accumulator
		case 0x48:
			title = "PHA - Push Accumulator";

			longDesc = "Pushes the value of the accumulator to the stack.";
		break;
		// PHP - Push Processor Status
		case 0x08:
			title = "PHP - Push Processor Status";

			longDesc = "Pushes the value of the status flags to the stack.";
		break;
		// PLA - Pull Accumulator
		case 0x68:
			title = "PLA - Pull Accumulator";

			longDesc = "Pulls a byte from the stack and stores it into the accumulator.";
		break;
		// PLP - Pull Processor Status
		case 0x28:
			title = "PLP - Pull Processor Status";

			longDesc = "Pulls a byte from the stack and stores it into the processor flags.  The flags will be modified based on the value pulled.";
		break;
		// ROL - Rotate Left
		case 0x2A:
		case 0x26:
		case 0x36:
		case 0x2E:
		case 0x3E:
			title = "ROL - Rotate Left";

			if ( opcode[0] == 0x2A )
			{
				addrMode = "Accumulator";	
			}

			longDesc = "Shifts all bits 1 position to the left. The right-most bit takes the current value of the carry flag (C). The left-most bit is stored into the carry flag (C).";
		break;
		// ROR - Rotate Right
		case 0x6A:
		case 0x66:
		case 0x76:
		case 0x6E:
		case 0x7E:
			title = "ROR - Rotate Right";

			if ( opcode[0] == 0x6A )
			{
				addrMode = "Accumulator";	
			}

			longDesc = "Shifts all bits 1 position to the right. The left-most bit takes the current value of the carry flag (C). The right-most bit is stored into the carry flag (C).";
		break;
		// RTI - Return from Interrupt
		case 0x40:
			title = "RTI - Return from Interrupt";

			longDesc = "The RTI instruction is used at the end of the interrupt handler to return execution to its original location.  It pulls the status flags and program counter from the stack.";
		break;
		// RTS - Return from Subroutine
		case 0x60:
			title = "RTS - Return from Subroutine";

			longDesc = "The RTS instruction is used at the end of a subroutine to return execution to the calling function. It pulls the status flags and program counter (minus 1) from the stack.";
		break;
		// SBC - Subtract with Carry
		case 0xE9:
		case 0xE5:
		case 0xF5:
		case 0xED:
		case 0xFD:
		case 0xF9:
		case 0xE1:
		case 0xF1:
			title = "SBC - Subtract with Carry";

			if ( opcode[0] == 0xE9 )
			{
				addrMode = "Immediate";	
			}

			longDesc = "Substracts the byte at the specified memory address from the value of the accumulator (affected by the carry flag (C)).";
		break;
		// SEC - Set Carry Flag
		case 0x38:
			title = "SEC - Set Carry Flag";

			longDesc = "Sets the carry flag (C).";
		break;
		// SED - Set Decimal Flag
		case 0xF8:
			title = "SED - Set Decimal Flag";

			longDesc = "Sets the decimal mode flag (D).";
		break;
		// SEI - Set Interrupt Disable
		case 0x78:
			title = "SEI - Set Interrupt Disable";

			longDesc = "Sets the interrupt disable flag (I).";
		break;
		// STA - Store Accumulator
		case 0x85:
		case 0x95:
		case 0x8D:
		case 0x9D:
		case 0x99:
		case 0x81:
		case 0x91:
			title = "STA - Store Accumulator";

			longDesc = "Stores the contents of the accumulator into memory.";
		break;
		// STX - Store X Register
		case 0x86:
		case 0x96:
		case 0x8E:
			title = "STX - Store X Register";

			longDesc = "Stores the value of the X register into memory.";
		break;
		// STY - Store Y Register
		case 0x84:
		case 0x94:
		case 0x8C:
			title = "STY - Store Y Register";

			longDesc = "Stores the value of the Y register into memory.";
		break;
		// TAX - Transfer Accumulator to X Register
		case 0xAA:
			title = "TAX - Transfer Accumulator to X Register";

			longDesc = "Copies the accumulator into the X register.";
		break;
		// TAY - Transfer Accumulator to Y Register
		case 0xA8:
			title = "TAY - Transfer Accumulator to Y Register";

			longDesc = "Copies the accumulator into the Y register.";
		break;
		// TSX - Transfer Stack Pointer to X Register
		case 0xBA:
			title = "TSX - Transfer Stack Pointer to X Register";

			longDesc = "Copies the stack pointer into the X register.";
		break;
		// TXA - Transfer X Register to Accumulator
		case 0x8A:
			title = "TXA - Transfer X Register to Accumulator";

			longDesc = "Copies the X register into the accumulator.";
		break;
		// TXS - Transfer X Register to Stack Pointer
		case 0x9A:
			title = "TXS - Transfer X Register to Stack Pointer";

			longDesc = "Copies the X register into the stack pointer.";
		break;
		// TYA - Transfer Y Register to Accumulator
		case 0x98:
			title = "TYA - Transfer Y Register to Accumulator";

			longDesc = "Copies the Y register into the accumulator.";
		break;
		default:
			title = "Undefined";
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
			case 1: // Indirect,X
				addrMode = "Indirect,X";
			break;
			case 2: // Zero Page
				addrMode = "Zero Page";
			break;
			case 3: // Absolute
				addrMode = "Absolute";
			break;
			case 4: // Indirect,Y
				addrMode = "Indirect,Y";
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

	if ( synopsis )
	{
		text.append( "\n" );
		text.append( synopsis );
		text.append( "\n" );
	}
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

	if ( longDesc )
	{
		int i=0, len=0;
		text.append( "\nDescription:\n\t" );

		while ( longDesc[i] != 0 )
		{
			if ( longDesc[i] == '\n' )
			{
				len = 0;
			}

			// Check if a new line is needed
			if ( (len > 50) && isspace(longDesc[i]) )
			{
				text.append( 1, '\n' ); len = 0;

				while ( isspace(longDesc[i]) ) i++;

				if ( longDesc[i] == 0 ) break;

				text.append( 1, '\t' );
			}
			text.append( 1, longDesc[i] ); len++;

			i++;
		}
	}

	return QString::fromStdString( text );
}
//---------------------------------------------------------------------------
