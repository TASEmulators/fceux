// SymbolicDebug.cpp
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../types.h"
#include "../../fceu.h"
#include "../../debug.h"
#include "../../driver.h"
#include "../../cart.h"
#include "../../ines.h"
#include "../../asm.h"
#include "../../x6502.h"

#include "Qt/fceuWrapper.h"
#include "Qt/SymbolicDebug.h"
#include "Qt/ConsoleUtilities.h"

debugSymbolTable_t  debugSymbolTable;

//--------------------------------------------------------------
// debugSymbolPage_t
//--------------------------------------------------------------
debugSymbolPage_t::debugSymbolPage_t(void)
{
	pageNum = -1;

}
//--------------------------------------------------------------
debugSymbolPage_t::~debugSymbolPage_t(void)
{
	std::map <int, debugSymbol_t*>::iterator it;

	for (it=symMap.begin(); it!=symMap.end(); it++)
	{
		delete it->second;
	}
	symMap.clear();
}
//--------------------------------------------------------------
int debugSymbolPage_t::addSymbol( debugSymbol_t*sym )
{
	std::map <int, debugSymbol_t*>::iterator it;

	it = symMap.find( sym->ofs );

	if ( it != symMap.end() )
	{
		return -1;
	}
	symMap[ sym->ofs ] = sym;

	return 0;
}
//--------------------------------------------------------------
debugSymbol_t *debugSymbolPage_t::getSymbolAtOffset( int ofs )
{
	debugSymbol_t*sym = NULL;
	std::map <int, debugSymbol_t*>::iterator it;

	it = symMap.find( ofs );

	if ( it != symMap.end() )
	{
		sym = it->second;
	}
	return sym;
}
//--------------------------------------------------------------
int debugSymbolPage_t::deleteSymbolAtOffset( int ofs )
{
	debugSymbol_t*sym = NULL;
	std::map <int, debugSymbol_t*>::iterator it;

	it = symMap.find( ofs );

	if ( it != symMap.end() )
	{
		sym = it->second;
		symMap.erase(it);
	}
	else
	{
		return -1;
	}
	if ( sym != NULL )
	{
		delete sym;
	}
	return 0;
}
//--------------------------------------------------------------
int debugSymbolPage_t::save(void)
{
	FILE *fp;
	debugSymbol_t *sym;
	std::map <int, debugSymbol_t*>::iterator it;
	const char *romFile;
	char stmp[512];
	int i,j;

	romFile = getRomFile();

	if ( romFile == NULL )
	{
		return -1;
	}
	i=0;
	while ( romFile[i] != 0 )
	{

		if ( romFile[i] == '|' )
		{
			stmp[i] = '.';
		}
		else
		{
			stmp[i] = romFile[i];
		}
		i++;
	}
	stmp[i] = 0;

	if ( pageNum < 0 )
	{
		strcat( stmp, ".ram.nl" );
	}
	else
	{
		char suffix[32];

		sprintf( suffix, ".%X.nl", pageNum );

		strcat( stmp, suffix );
	}

	fp = fopen( stmp, "w" );

	if ( fp == NULL )
	{
		printf("Error: Could not open file '%s' for writing\n", stmp );
		return -1;
	}

	for (it=symMap.begin(); it!=symMap.end(); it++)
	{
		const char *c;

		sym = it->second;

		i=0; j=0; c = sym->comment.c_str();
		
		while ( c[i] != 0 )
		{
			if ( c[i] == '\n' )
			{
				i++; break;
			}
			else
			{
				stmp[j] = c[i]; j++; i++;
			}
		}
		stmp[j] = 0;

		fprintf( fp, "$%04X#%s#%s\n", sym->ofs, sym->name.c_str(), stmp );

		j=0;
		while ( c[i] != 0 )
		{
			if ( c[i] == '\n' )
			{
				i++; stmp[j] = 0;

				if ( j > 0 )
				{
					fprintf( fp, "\\%s\n", stmp );
				}
				j=0;
			}
			else
			{
				stmp[j] = c[i]; j++; i++;
			}
		}
	}

	fclose(fp);

	return 0;
}
//--------------------------------------------------------------
void debugSymbolPage_t::print(void)
{
	FILE *fp;
	debugSymbol_t *sym;
	std::map <int, debugSymbol_t*>::iterator it;

	fp = stdout;

	fprintf( fp, "Page: %X \n", pageNum );

	for (it=symMap.begin(); it!=symMap.end(); it++)
	{
		sym = it->second;

		fprintf( fp, "   Sym: $%04X '%s' \n", sym->ofs, sym->name.c_str() );
	}
}
//--------------------------------------------------------------
// debugSymbolTable_t
//--------------------------------------------------------------
debugSymbolTable_t::debugSymbolTable_t(void)
{

}
//--------------------------------------------------------------
debugSymbolTable_t::~debugSymbolTable_t(void)
{
	this->clear();
}
//--------------------------------------------------------------
void debugSymbolTable_t::clear(void)
{
	std::map <int, debugSymbolPage_t*>::iterator it;

	for (it=pageMap.begin(); it!=pageMap.end(); it++)
	{
		delete it->second;
	}
	pageMap.clear();
}
//--------------------------------------------------------------
int generateNLFilenameForAddress(int address, char *NLfilename)
{
	int bank;

	if (address < 0x8000)
	{
		bank = -1;
	} 
	else
	{
		bank = getBank(address);
		#ifdef DW3_NL_0F_1F_HACK
		if(bank == 0x0F)
			bank = 0x1F;
		#endif
	}
	return generateNLFilenameForBank( bank, NLfilename );
}
//--------------------------------------------------------------
int generateNLFilenameForBank(int bank, char *NLfilename)
{
	int i;
	const char *romFile;

	romFile = getRomFile();

	if ( romFile == NULL )
	{
		return -1;
	}
	i=0;
	while ( romFile[i] != 0 )
	{

		if ( romFile[i] == '|' )
		{
			NLfilename[i] = '.';
		}
		else
		{
			NLfilename[i] = romFile[i];
		}
		i++;
	}
	NLfilename[i] = 0;

	if (bank < 0)
	{
		// The NL file for the RAM addresses has the name nesrom.nes.ram.nl
		strcat(NLfilename, ".ram.nl");
	} 
	else
	{
		char stmp[64];
		#ifdef DW3_NL_0F_1F_HACK
		if(bank == 0x0F)
			bank = 0x1F;
		#endif
		sprintf( stmp, ".%X.nl", bank);
		strcat(NLfilename, stmp );
	}
	return 0;
}
//--------------------------------------------------------------
int debugSymbolTable_t::loadFileNL( int bank )
{
	FILE *fp;
	int i, j, ofs, lineNum = 0, literal = 0, array = 0;
	char fileName[512], line[512];
	char stmp[512];
	debugSymbolPage_t *page = NULL;
	debugSymbol_t *sym = NULL;

	//printf("Looking to Load Debug Bank: $%X \n", bank );

	if ( generateNLFilenameForBank( bank, fileName ) )
	{
		return -1;
	}
	//printf("Loading NL File: %s\n", fileName );

	fp = ::fopen( fileName, "r" );

	if ( fp == NULL )
	{
		return -1;
	}
	page = new debugSymbolPage_t;

	page->pageNum = bank;

	pageMap[ page->pageNum ] = page;

	while ( fgets( line, sizeof(line), fp ) != 0 )
	{
		i=0; lineNum++;
		//printf("%4i:%s", lineNum, line );

		if ( line[i] == '\\' )
		{
			// Line is a comment continuation line.
			i++;

			j=0;
			stmp[j] = '\n'; j++;

			while ( line[i] != 0 )
			{
				stmp[j] = line[i]; j++; i++;
			}
			stmp[j] = 0;

			j--;
			while ( j >= 0 )
			{
				if ( isspace( stmp[j] ) )
				{
					stmp[j] = 0; 
				}
				else
				{
					break;
				}
				j--;
			}
			if ( sym != NULL )
			{
				sym->comment.append( stmp );
			}
		}
		else if ( line[i] == '$' )
		{
			// Line is a new debug offset
			array = 0;

			j=0; i++;
			if ( !isxdigit( line[i] ) )
			{
				printf("Error: Invalid Offset on Line %i of File %s\n", lineNum, fileName );
			}
			while ( isxdigit( line[i] ) )
			{
				stmp[j] = line[i]; i++; j++;
			}
			stmp[j] = 0;

			ofs = strtol( stmp, NULL, 16 );

			if ( line[i] == '/' )
			{
				j=0; i++;
				while ( isxdigit( line[i] ) )
				{
					stmp[j] = line[i]; i++; j++;
				}
				stmp[j] = 0;

				array = strtol( stmp, NULL, 16 );
			}

			if ( line[i] != '#' )
			{
				printf("Error: Missing field delimiter following offset $%X on Line %i of File %s\n", ofs, lineNum, fileName );
				continue;
			}
			i++;

			while ( isspace(line[i]) ) i++;

			j = 0;
			while ( line[i] != 0 )
			{
				if ( line[i] == '\\' )
				{
					if ( literal )
					{
						switch ( line[i] )
						{
							case 'r':
								stmp[j] = '\r';
							break;
							case 'n':
								stmp[j] = '\n';
							break;
							case 't':
								stmp[j] = '\t';
							break;
							default:
								stmp[j] = line[i];
							break;
						}
						j++; i++;
						literal = 0;
					}
					else
					{
						i++;
						literal = !literal;
					}
				}
				else if ( line[i] == '#' )
				{
					break;
				}
				else
				{
					stmp[j] = line[i]; j++; i++;
				}
			}
			stmp[j] = 0;

			j--;
			while ( j >= 0 )
			{
				if ( isspace( stmp[j] ) )
				{
					stmp[j] = 0; 
				}
				else
				{
					break;
				}
				j--;
			}

			if ( line[i] != '#' )
			{
				printf("Error: Missing field delimiter following name '%s' on Line %i of File %s\n", stmp, lineNum, fileName );
				continue;
			}
			i++;

			sym = new debugSymbol_t();

			if ( sym == NULL )
			{
				printf("Error: Failed to allocate memory for offset $%04X Name '%s' on Line %i of File %s\n", ofs, stmp, lineNum, fileName );
				continue;
			}
			sym->ofs = ofs;
			sym->name.assign( stmp );
			
			while ( isspace( line[i] ) ) i++;

			j=0;
			while ( line[i] != 0 )
			{
				stmp[j] = line[i]; j++; i++;
			}
			stmp[j] = 0;

			j--;
			while ( j >= 0 )
			{
				if ( isspace( stmp[j] ) )
				{
					stmp[j] = 0; 
				}
				else
				{
					break;
				}
				j--;
			}

			sym->comment.assign( stmp );

			if ( array > 0 )
			{
				debugSymbol_t *arraySym = NULL;

				for (j=0; j<array; j++)
				{
					arraySym = new debugSymbol_t();

					if ( arraySym )
					{
						arraySym->ofs = sym->ofs + j;

						sprintf( stmp, "[%i]", j );
						arraySym->name.assign( sym->name );
						arraySym->name.append( stmp );
						arraySym->comment.assign( sym->comment );

						if ( page->addSymbol( arraySym ) )
						{
							printf("Error: Failed to add symbol for offset $%04X Name '%s' on Line %i of File %s\n", ofs, arraySym->name.c_str(), lineNum, fileName );
							delete arraySym; arraySym = NULL; // Failed to add symbol
						}
					}
				}
				delete sym; sym = NULL; // Delete temporary symbol
			}
			else
			{
				if ( page->addSymbol( sym ) )
				{
					printf("Error: Failed to add symbol for offset $%04X Name '%s' on Line %i of File %s\n", ofs, sym->name.c_str(), lineNum, fileName );
					delete sym; sym = NULL; // Failed to add symbol
				}
			}
		}
	}

	::fclose(fp);

	return 0;
}
//--------------------------------------------------------------
int debugSymbolTable_t::loadRegisterMap(void)
{
	debugSymbolPage_t *page;

	page = new debugSymbolPage_t();

	page->pageNum = -2;

	page->addSymbol( new debugSymbol_t( 0x2000, "PPU_CTRL" ) );
	page->addSymbol( new debugSymbol_t( 0x2001, "PPU_MASK" ) );
	page->addSymbol( new debugSymbol_t( 0x2002, "PPU_STATUS" ) );
	page->addSymbol( new debugSymbol_t( 0x2003, "PPU_OAM_ADDR" ) );
	page->addSymbol( new debugSymbol_t( 0x2004, "PPU_OAM_DATA" ) );
	page->addSymbol( new debugSymbol_t( 0x2005, "PPU_SCROLL" ) );
	page->addSymbol( new debugSymbol_t( 0x2006, "PPU_ADDRESS" ) );
	page->addSymbol( new debugSymbol_t( 0x2007, "PPU_DATA" ) );
	page->addSymbol( new debugSymbol_t( 0x4000, "SQ1_VOL" ) );
	page->addSymbol( new debugSymbol_t( 0x4001, "SQ1_SWEEP" ) );
	page->addSymbol( new debugSymbol_t( 0x4002, "SQ1_LO" ) );
	page->addSymbol( new debugSymbol_t( 0x4003, "SQ1_HI" ) );
	page->addSymbol( new debugSymbol_t( 0x4004, "SQ2_VOL" ) );
	page->addSymbol( new debugSymbol_t( 0x4005, "SQ2_SWEEP" ) );
	page->addSymbol( new debugSymbol_t( 0x4006, "SQ2_LO" ) );
	page->addSymbol( new debugSymbol_t( 0x4007, "SQ2_HI" ) );
	page->addSymbol( new debugSymbol_t( 0x4008, "TRI_LINEAR" ) );
//	page->addSymbol( new debugSymbol_t( 0x4009, "UNUSED" ) );
	page->addSymbol( new debugSymbol_t( 0x400A, "TRI_LO" ) );
	page->addSymbol( new debugSymbol_t( 0x400B, "TRI_HI" ) );
	page->addSymbol( new debugSymbol_t( 0x400C, "NOISE_VOL" ) );
//	page->addSymbol( new debugSymbol_t( 0x400D, "UNUSED" ) );
	page->addSymbol( new debugSymbol_t( 0x400E, "NOISE_LO" ) );
	page->addSymbol( new debugSymbol_t( 0x400F, "NOISE_HI" ) );
	page->addSymbol( new debugSymbol_t( 0x4010, "DMC_FREQ" ) );
	page->addSymbol( new debugSymbol_t( 0x4011, "DMC_RAW" ) );
	page->addSymbol( new debugSymbol_t( 0x4012, "DMC_START" ) );
	page->addSymbol( new debugSymbol_t( 0x4013, "DMC_LEN" ) );
	page->addSymbol( new debugSymbol_t( 0x4014, "OAM_DMA" ) );
	page->addSymbol( new debugSymbol_t( 0x4015, "APU_STATUS" ) );
	page->addSymbol( new debugSymbol_t( 0x4016, "JOY1" ) );
	page->addSymbol( new debugSymbol_t( 0x4017, "JOY2_FRAME" ) );

	pageMap[ page->pageNum ] = page;

	return 0;
}
//--------------------------------------------------------------
int debugSymbolTable_t::loadGameSymbols(void)
{
	int nPages, pageSize, romSize = 0x10000;

	this->save();
	this->clear();

	if ( GameInfo != NULL )
	{
		romSize = 16 + CHRsize[0] + PRGsize[0];
	}

	loadFileNL( -1 );

	loadRegisterMap();

	pageSize = (1<<debuggerPageSize);

	//nPages = 1<<(15-debuggerPageSize);
	nPages = romSize / pageSize;

	//printf("RomSize: %i    NumPages: %i \n", romSize, nPages );

	for(int i=0;i<nPages;i++)
	{
		//printf("Loading Page Offset: $%06X\n", pageSize*i );

		loadFileNL( i );
	}

	//print();

	return 0;
}
//--------------------------------------------------------------
int debugSymbolTable_t::addSymbolAtBankOffset( int bank, int ofs, debugSymbol_t *sym )
{
	debugSymbolPage_t *page;
	std::map <int, debugSymbolPage_t*>::iterator it;

	it = pageMap.find( bank );

	if ( it == pageMap.end() )
	{
		page = new debugSymbolPage_t();
		page->pageNum = bank;
		pageMap[ bank ] = page;
	}
	else
	{
		page = it->second;
	}
	page->addSymbol( sym );

	return 0;
}
//--------------------------------------------------------------
int debugSymbolTable_t::deleteSymbolAtBankOffset( int bank, int ofs )
{
	debugSymbolPage_t *page;
	std::map <int, debugSymbolPage_t*>::iterator it;

	it = pageMap.find( bank );

	if ( it == pageMap.end() )
	{
		return -1;
	}
	else
	{
		page = it->second;
	}

	return page->deleteSymbolAtOffset( ofs );
}
//--------------------------------------------------------------
debugSymbol_t *debugSymbolTable_t::getSymbolAtBankOffset( int bank, int ofs )
{
	debugSymbol_t*sym = NULL;
	std::map <int, debugSymbolPage_t*>::iterator it;

	it = pageMap.find( bank );

	if ( it != pageMap.end() )
	{
		sym = (it->second)->getSymbolAtOffset( ofs );
	}
	return sym;
}
//--------------------------------------------------------------
void debugSymbolTable_t::save(void)
{
	debugSymbolPage_t *page;
	std::map <int, debugSymbolPage_t*>::iterator it;

	for (it=pageMap.begin(); it!=pageMap.end(); it++)
	{
		page = it->second;

		page->save();
	}
}
//--------------------------------------------------------------
void debugSymbolTable_t::print(void)
{
	debugSymbolPage_t *page;
	std::map <int, debugSymbolPage_t*>::iterator it;

	for (it=pageMap.begin(); it!=pageMap.end(); it++)
	{
		page = it->second;

		page->print();
	}
}
//--------------------------------------------------------------
debugSymbol_t *replaceSymbols( int flags, int addr, char *str )
{
	debugSymbol_t *sym;
  
	if ( addr >= 0x8000 )
	{
		int bank = getBank(addr);

  		sym = debugSymbolTable.getSymbolAtBankOffset( bank, addr );
	}
	else
	{
  		sym = debugSymbolTable.getSymbolAtBankOffset( -1, addr );

		if ( (sym == NULL) && (flags & ASM_DEBUG_REGS) )
		{
  			sym = debugSymbolTable.getSymbolAtBankOffset( -2, addr );
		}
	}

	if ( sym )
	{
		if ( flags & ASM_DEBUG_REPLACE )
		{
			strcpy( str, sym->name.c_str() );
		}
		else
		{
			if ( flags & ASM_DEBUG_ADDR_02X )
			{
				sprintf( str, "%02X ", addr );
			}
			else
			{
				sprintf( str, "%04X ", addr );
			}
			strcat( str, sym->name.c_str() );
		}
	}
	else
	{
		if ( flags & ASM_DEBUG_ADDR_02X )
		{
			sprintf( str, "%02X", addr );
		}
		else
		{
			sprintf( str, "%04X", addr );
		}
	}

	return sym;
}
//--------------------------------------------------------------
int DisassembleWithDebug(int addr, uint8_t *opcode, int flags, char *str, debugSymbol_t *symOut, debugSymbol_t *symOut2 )
{
	debugSymbol_t *sym  = NULL;
	debugSymbol_t *sym2 = NULL;
	static char chr[8]={0};
	uint16_t tmp,tmp2;
	char stmp[128], stmp2[128];

	//these may be replaced later with passed-in values to make a lighter-weight disassembly mode that may not query the referenced values
	#define RX (X.X)
	#define RY (X.Y)

	switch (opcode[0]) 
	{
		#define relative(a) { \
			if (((a)=opcode[1])&0x80) (a) = addr-(((a)-1)^0xFF); \
			else (a)+=addr; \
		}
		#define absolute(a) { \
			(a) = opcode[1] | opcode[2]<<8; \
		}
		#define zpIndex(a,i) { \
			(a) = (opcode[1]+(i))&0xFF; \
		}
		#define indirectX(a) { \
			(a) = (opcode[1]+RX)&0xFF; \
			(a) = GetMem((a)) | (GetMem(((a)+1)&0xff))<<8; \
		}
		#define indirectY(a) { \
			(a) = GetMem(opcode[1]) | (GetMem((opcode[1]+1)&0xff))<<8; \
			(a) += RY; \
		}


		#ifdef BRK_3BYTE_HACK
			case 0x00:
			sprintf(str,"BRK %02X %02X", opcode[1], opcode[2]);
			break;
		#else
			case 0x00: strcpy(str,"BRK"); break;
		#endif

		//odd, 1-byte opcodes
		case 0x08: strcpy(str,"PHP"); break;
		case 0x0A: strcpy(str,"ASL"); break;
		case 0x18: strcpy(str,"CLC"); break;
		case 0x28: strcpy(str,"PLP"); break;
		case 0x2A: strcpy(str,"ROL"); break;
		case 0x38: strcpy(str,"SEC"); break;
		case 0x40: strcpy(str,"RTI"); break;
		case 0x48: strcpy(str,"PHA"); break;
		case 0x4A: strcpy(str,"LSR"); break;
		case 0x58: strcpy(str,"CLI"); break;
		case 0x60: strcpy(str,"RTS"); break;
		case 0x68: strcpy(str,"PLA"); break;
		case 0x6A: strcpy(str,"ROR"); break;
		case 0x78: strcpy(str,"SEI"); break;
		case 0x88: strcpy(str,"DEY"); break;
		case 0x8A: strcpy(str,"TXA"); break;
		case 0x98: strcpy(str,"TYA"); break;
		case 0x9A: strcpy(str,"TXS"); break;
		case 0xA8: strcpy(str,"TAY"); break;
		case 0xAA: strcpy(str,"TAX"); break;
		case 0xB8: strcpy(str,"CLV"); break;
		case 0xBA: strcpy(str,"TSX"); break;
		case 0xC8: strcpy(str,"INY"); break;
		case 0xCA: strcpy(str,"DEX"); break;
		case 0xD8: strcpy(str,"CLD"); break;
		case 0xE8: strcpy(str,"INX"); break;
		case 0xEA: strcpy(str,"NOP"); break;
		case 0xF8: strcpy(str,"SED"); break;

		//(Indirect,X)
		case 0x01: strcpy(chr,"ORA"); goto _indirectx;
		case 0x21: strcpy(chr,"AND"); goto _indirectx;
		case 0x41: strcpy(chr,"EOR"); goto _indirectx;
		case 0x61: strcpy(chr,"ADC"); goto _indirectx;
		case 0x81: strcpy(chr,"STA"); goto _indirectx;
		case 0xA1: strcpy(chr,"LDA"); goto _indirectx;
		case 0xC1: strcpy(chr,"CMP"); goto _indirectx;
		case 0xE1: strcpy(chr,"SBC"); goto _indirectx;
		_indirectx:
			indirectX(tmp);

			if ( flags )
			{
				sym = replaceSymbols( flags, tmp, stmp );
				sprintf(str,"%s ($%02X,X) @ $%s = #$%02X", chr,opcode[1],stmp,GetMem(tmp));
			}
			else
			{
				sprintf(str,"%s ($%02X,X) @ $%04X = #$%02X", chr,opcode[1],tmp,GetMem(tmp));
			}
			break;

		//Zero Page
		case 0x05: strcpy(chr,"ORA"); goto _zeropage;
		case 0x06: strcpy(chr,"ASL"); goto _zeropage;
		case 0x24: strcpy(chr,"BIT"); goto _zeropage;
		case 0x25: strcpy(chr,"AND"); goto _zeropage;
		case 0x26: strcpy(chr,"ROL"); goto _zeropage;
		case 0x45: strcpy(chr,"EOR"); goto _zeropage;
		case 0x46: strcpy(chr,"LSR"); goto _zeropage;
		case 0x65: strcpy(chr,"ADC"); goto _zeropage;
		case 0x66: strcpy(chr,"ROR"); goto _zeropage;
		case 0x84: strcpy(chr,"STY"); goto _zeropage;
		case 0x85: strcpy(chr,"STA"); goto _zeropage;
		case 0x86: strcpy(chr,"STX"); goto _zeropage;
		case 0xA4: strcpy(chr,"LDY"); goto _zeropage;
		case 0xA5: strcpy(chr,"LDA"); goto _zeropage;
		case 0xA6: strcpy(chr,"LDX"); goto _zeropage;
		case 0xC4: strcpy(chr,"CPY"); goto _zeropage;
		case 0xC5: strcpy(chr,"CMP"); goto _zeropage;
		case 0xC6: strcpy(chr,"DEC"); goto _zeropage;
		case 0xE4: strcpy(chr,"CPX"); goto _zeropage;
		case 0xE5: strcpy(chr,"SBC"); goto _zeropage;
		case 0xE6: strcpy(chr,"INC"); goto _zeropage;
		_zeropage:
		// ################################## Start of SP CODE ###########################
		// Change width to %04X // don't!
			if ( flags )
			{
				sym = replaceSymbols( flags | ASM_DEBUG_ADDR_02X, opcode[1], stmp );
				sprintf(str,"%s $%s = #$%02X", chr,stmp,GetMem(opcode[1]));
			}
			else
			{
				sprintf(str,"%s $%02X = #$%02X", chr,opcode[1],GetMem(opcode[1]));
			}
		// ################################## End of SP CODE ###########################
			break;

		//#Immediate
		case 0x09: strcpy(chr,"ORA"); goto _immediate;
		case 0x29: strcpy(chr,"AND"); goto _immediate;
		case 0x49: strcpy(chr,"EOR"); goto _immediate;
		case 0x69: strcpy(chr,"ADC"); goto _immediate;
		//case 0x89: strcpy(chr,"STA"); goto _immediate;  //baka, no STA #imm!!
		case 0xA0: strcpy(chr,"LDY"); goto _immediate;
		case 0xA2: strcpy(chr,"LDX"); goto _immediate;
		case 0xA9: strcpy(chr,"LDA"); goto _immediate;
		case 0xC0: strcpy(chr,"CPY"); goto _immediate;
		case 0xC9: strcpy(chr,"CMP"); goto _immediate;
		case 0xE0: strcpy(chr,"CPX"); goto _immediate;
		case 0xE9: strcpy(chr,"SBC"); goto _immediate;
		_immediate:
			sprintf(str,"%s #$%02X", chr,opcode[1]);
			break;

		//Absolute
		case 0x0D: strcpy(chr,"ORA"); goto _absolute;
		case 0x0E: strcpy(chr,"ASL"); goto _absolute;
		case 0x2C: strcpy(chr,"BIT"); goto _absolute;
		case 0x2D: strcpy(chr,"AND"); goto _absolute;
		case 0x2E: strcpy(chr,"ROL"); goto _absolute;
		case 0x4D: strcpy(chr,"EOR"); goto _absolute;
		case 0x4E: strcpy(chr,"LSR"); goto _absolute;
		case 0x6D: strcpy(chr,"ADC"); goto _absolute;
		case 0x6E: strcpy(chr,"ROR"); goto _absolute;
		case 0x8C: strcpy(chr,"STY"); goto _absolute;
		case 0x8D: strcpy(chr,"STA"); goto _absolute;
		case 0x8E: strcpy(chr,"STX"); goto _absolute;
		case 0xAC: strcpy(chr,"LDY"); goto _absolute;
		case 0xAD: strcpy(chr,"LDA"); goto _absolute;
		case 0xAE: strcpy(chr,"LDX"); goto _absolute;
		case 0xCC: strcpy(chr,"CPY"); goto _absolute;
		case 0xCD: strcpy(chr,"CMP"); goto _absolute;
		case 0xCE: strcpy(chr,"DEC"); goto _absolute;
		case 0xEC: strcpy(chr,"CPX"); goto _absolute;
		case 0xED: strcpy(chr,"SBC"); goto _absolute;
		case 0xEE: strcpy(chr,"INC"); goto _absolute;
		_absolute:
			absolute(tmp);

			if ( flags )
			{
				sym = replaceSymbols( flags, tmp, stmp );
				sprintf(str,"%s $%s = #$%02X", chr,stmp,GetMem(tmp));
			}
			else
			{
				sprintf(str,"%s $%04X = #$%02X", chr,tmp,GetMem(tmp));
			}
			break;

		//branches
		case 0x10: strcpy(chr,"BPL"); goto _branch;
		case 0x30: strcpy(chr,"BMI"); goto _branch;
		case 0x50: strcpy(chr,"BVC"); goto _branch;
		case 0x70: strcpy(chr,"BVS"); goto _branch;
		case 0x90: strcpy(chr,"BCC"); goto _branch;
		case 0xB0: strcpy(chr,"BCS"); goto _branch;
		case 0xD0: strcpy(chr,"BNE"); goto _branch;
		case 0xF0: strcpy(chr,"BEQ"); goto _branch;
		_branch:
			relative(tmp);

			if ( flags )
			{
				sym = replaceSymbols( flags, tmp, stmp );
				sprintf(str,"%s $%s", chr,stmp);
			}
			else
			{
				sprintf(str,"%s $%04X", chr,tmp);
			}
			break;

		//(Indirect),Y
		case 0x11: strcpy(chr,"ORA"); goto _indirecty;
		case 0x31: strcpy(chr,"AND"); goto _indirecty;
		case 0x51: strcpy(chr,"EOR"); goto _indirecty;
		case 0x71: strcpy(chr,"ADC"); goto _indirecty;
		case 0x91: strcpy(chr,"STA"); goto _indirecty;
		case 0xB1: strcpy(chr,"LDA"); goto _indirecty;
		case 0xD1: strcpy(chr,"CMP"); goto _indirecty;
		case 0xF1: strcpy(chr,"SBC"); goto _indirecty;
		_indirecty:
			indirectY(tmp);

			if ( flags )
			{
				sym = replaceSymbols( flags, tmp, stmp );
				sprintf(str,"%s ($%02X),Y @ $%s = #$%02X", chr,opcode[1],stmp,GetMem(tmp));
			}
			else
			{
				sprintf(str,"%s ($%02X),Y @ $%04X = #$%02X", chr,opcode[1],tmp,GetMem(tmp));
			}
			break;

		//Zero Page,X
		case 0x15: strcpy(chr,"ORA"); goto _zeropagex;
		case 0x16: strcpy(chr,"ASL"); goto _zeropagex;
		case 0x35: strcpy(chr,"AND"); goto _zeropagex;
		case 0x36: strcpy(chr,"ROL"); goto _zeropagex;
		case 0x55: strcpy(chr,"EOR"); goto _zeropagex;
		case 0x56: strcpy(chr,"LSR"); goto _zeropagex;
		case 0x75: strcpy(chr,"ADC"); goto _zeropagex;
		case 0x76: strcpy(chr,"ROR"); goto _zeropagex;
		case 0x94: strcpy(chr,"STY"); goto _zeropagex;
		case 0x95: strcpy(chr,"STA"); goto _zeropagex;
		case 0xB4: strcpy(chr,"LDY"); goto _zeropagex;
		case 0xB5: strcpy(chr,"LDA"); goto _zeropagex;
		case 0xD5: strcpy(chr,"CMP"); goto _zeropagex;
		case 0xD6: strcpy(chr,"DEC"); goto _zeropagex;
		case 0xF5: strcpy(chr,"SBC"); goto _zeropagex;
		case 0xF6: strcpy(chr,"INC"); goto _zeropagex;
		_zeropagex:
			zpIndex(tmp,RX);
		// ################################## Start of SP CODE ###########################
		// Change width to %04X // don't!
			if ( flags )
			{
				sym = replaceSymbols( flags, tmp, stmp );
				sprintf(str,"%s $%02X,X @ $%s = #$%02X", chr,opcode[1],stmp,GetMem(tmp));
			}
			else
			{
				sprintf(str,"%s $%02X,X @ $%04X = #$%02X", chr,opcode[1],tmp,GetMem(tmp));
			}
		// ################################## End of SP CODE ###########################
			break;

		//Absolute,Y
		case 0x19: strcpy(chr,"ORA"); goto _absolutey;
		case 0x39: strcpy(chr,"AND"); goto _absolutey;
		case 0x59: strcpy(chr,"EOR"); goto _absolutey;
		case 0x79: strcpy(chr,"ADC"); goto _absolutey;
		case 0x99: strcpy(chr,"STA"); goto _absolutey;
		case 0xB9: strcpy(chr,"LDA"); goto _absolutey;
		case 0xBE: strcpy(chr,"LDX"); goto _absolutey;
		case 0xD9: strcpy(chr,"CMP"); goto _absolutey;
		case 0xF9: strcpy(chr,"SBC"); goto _absolutey;
		_absolutey:
			absolute(tmp);
			tmp2=(tmp+RY);
			if ( flags )
			{
				sym  = replaceSymbols( flags, tmp , stmp  );
				sym2 = replaceSymbols( flags, tmp2, stmp2 );
				sprintf(str,"%s $%s,Y @ $%s = #$%02X", chr,stmp,stmp2,GetMem(tmp2));
			}
			else
			{
				sprintf(str,"%s $%04X,Y @ $%04X = #$%02X", chr,tmp,tmp2,GetMem(tmp2));
			}
			break;

		//Absolute,X
		case 0x1D: strcpy(chr,"ORA"); goto _absolutex;
		case 0x1E: strcpy(chr,"ASL"); goto _absolutex;
		case 0x3D: strcpy(chr,"AND"); goto _absolutex;
		case 0x3E: strcpy(chr,"ROL"); goto _absolutex;
		case 0x5D: strcpy(chr,"EOR"); goto _absolutex;
		case 0x5E: strcpy(chr,"LSR"); goto _absolutex;
		case 0x7D: strcpy(chr,"ADC"); goto _absolutex;
		case 0x7E: strcpy(chr,"ROR"); goto _absolutex;
		case 0x9D: strcpy(chr,"STA"); goto _absolutex;
		case 0xBC: strcpy(chr,"LDY"); goto _absolutex;
		case 0xBD: strcpy(chr,"LDA"); goto _absolutex;
		case 0xDD: strcpy(chr,"CMP"); goto _absolutex;
		case 0xDE: strcpy(chr,"DEC"); goto _absolutex;
		case 0xFD: strcpy(chr,"SBC"); goto _absolutex;
		case 0xFE: strcpy(chr,"INC"); goto _absolutex;
		_absolutex:
			absolute(tmp);
			tmp2=(tmp+RX);
			if ( flags )
			{
				sym  = replaceSymbols( flags, tmp , stmp  );
				sym2 = replaceSymbols( flags, tmp2, stmp2 );
				sprintf(str,"%s $%s,X @ $%s = #$%02X", chr,stmp,stmp2,GetMem(tmp2));
			}
			else
			{
				sprintf(str,"%s $%04X,X @ $%04X = #$%02X", chr,tmp,tmp2,GetMem(tmp2));
			}
			break;

		//jumps
		case 0x20: strcpy(chr,"JSR"); goto _jump;
		case 0x4C: strcpy(chr,"JMP"); goto _jump;
		case 0x6C: absolute(tmp); sprintf(str,"JMP ($%04X) = $%04X", tmp,GetMem(tmp)|GetMem(tmp+1)<<8); break;
		_jump:
			absolute(tmp);

			if ( flags )
			{
				sym = replaceSymbols( flags, tmp, stmp );
				sprintf(str,"%s $%s", chr,stmp);
			}
			else
			{
				sprintf(str,"%s $%04X", chr,tmp);
			}
			break;

		//Zero Page,Y
		case 0x96: strcpy(chr,"STX"); goto _zeropagey;
		case 0xB6: strcpy(chr,"LDX"); goto _zeropagey;
		_zeropagey:
			zpIndex(tmp,RY);
		// ################################## Start of SP CODE ###########################
		// Change width to %04X // don't!
			if ( flags )
			{
				sym = replaceSymbols( flags, tmp, stmp );
				sprintf(str,"%s $%02X,Y @ $%s = #$%02X", chr,opcode[1],stmp,GetMem(tmp));
			}
			else
			{
				sprintf(str,"%s $%02X,Y @ $%04X = #$%02X", chr,opcode[1],tmp,GetMem(tmp));
			}
		// ################################## End of SP CODE ###########################
			break;

		//UNDEFINED
		default: strcpy(str,"ERROR"); break;

	}

	if ( symOut )
	{
		if ( sym )
		{
			*symOut = *sym;
		}
		else if ( sym2 )
		{
			*symOut = *sym2; sym2 = NULL;
		}
	}
	if ( symOut2 )
	{
		if ( sym2 )
		{
			*symOut2 = *sym2;
		}
	}
	
	return 0;
}
//--------------------------------------------------------------
// Symbol Add/Edit Window Object
//--------------------------------------------------------------
SymbolEditWindow::SymbolEditWindow(QWidget *parent)
	: QDialog(parent)
{
	QHBoxLayout *hbox;
	QVBoxLayout *mainLayout;
	QGridLayout *grid;
	QLabel      *lbl;
	fceuHexIntValidtor *validator;

	font.setFamily("Courier New");
	font.setStyle( QFont::StyleNormal );
	font.setStyleHint( QFont::Monospace );

	QFontMetrics fm(font);

#if QT_VERSION > QT_VERSION_CHECK(5, 11, 0)
    charWidth = fm.horizontalAdvance(QLatin1Char('2'));
#else
    charWidth = fm.width(QLatin1Char('2'));
#endif

	bank = -1;
	addr = -1;
	sym  = NULL;

	setWindowTitle( tr("Symbolic Debug Naming") );

	hbox       = new QHBoxLayout();
	mainLayout = new QVBoxLayout();

	lbl = new QLabel( tr("File") );
	filepath = new QLineEdit();
	filepath->setFont( font );
	filepath->setReadOnly( true );
	filepath->setMinimumWidth( charWidth * (filepath->text().size() + 4) );

	hbox->addWidget( lbl );
	hbox->addWidget( filepath );

	mainLayout->addLayout( hbox );

	hbox = new QHBoxLayout();
	lbl  = new QLabel( tr("Address") );
	addrEntry = new QLineEdit();
	addrEntry->setFont( font );
	addrEntry->setReadOnly( true );
	addrEntry->setAlignment(Qt::AlignCenter);
	addrEntry->setMaximumWidth( charWidth * 6 );

	hbox->addWidget( lbl, 1, Qt::AlignLeft );
	hbox->addWidget( addrEntry, 10, Qt::AlignLeft );

	isArrayBox = new QCheckBox( tr("Array") );
	hbox->addWidget( isArrayBox, 10, Qt::AlignRight );

	validator = new fceuHexIntValidtor( 0x00, 0xFF, this );
	arraySizeLbl[0] = new QLabel( tr("Size: 0x") );
	arraySize = new QLineEdit();
	arraySize->setFont( font );
	arraySize->setText( tr("01") );
	arraySize->setAlignment(Qt::AlignCenter);
	arraySize->setMaximumWidth( charWidth * 4 );
	arraySize->setValidator( validator );

	hbox->addWidget( arraySizeLbl[0], 0, Qt::AlignRight );
	hbox->addWidget( arraySize );

	arraySizeLbl[1] = new QLabel( tr("Bytes") );
	hbox->addWidget( arraySizeLbl[1], 0, Qt::AlignLeft );

	validator = new fceuHexIntValidtor( 0x00, 0xFF, this );
	arrayInitLbl = new QLabel( tr("Init: 0x") );
	arrayInit = new QLineEdit();
	arrayInit->setFont( font );
	arrayInit->setText( tr("00") );
	arrayInit->setAlignment(Qt::AlignCenter);
	arrayInit->setMaximumWidth( charWidth * 4 );
	arrayInit->setValidator( validator );

	hbox->addWidget( arrayInitLbl, 0, Qt::AlignRight );
	hbox->addWidget( arrayInit );

	mainLayout->addLayout( hbox );

	//hbox = new QHBoxLayout();
	grid = new QGridLayout();
	mainLayout->addLayout( grid );
	lbl  = new QLabel( tr("Name") );
	nameEntry = new QLineEdit();

	arrayNameOverWrite = new QCheckBox( tr("Overwrite Names in Array Body") );
	arrayNameOverWrite->setChecked( true );

	grid->addWidget( lbl, 0, 0 );
	grid->addWidget( nameEntry, 0, 1 );

	grid->addWidget( arrayNameOverWrite, 1, 1 );

	lbl  = new QLabel( tr("Comment") );
	commentEntry = new QPlainTextEdit();

	grid->addWidget( lbl, 2, 0 );
	grid->addWidget( commentEntry, 2, 1 );

	commentHeadOnly       = new QCheckBox( tr("Comment Head Address Only") );
	arrayCommentOverWrite = new QCheckBox( tr("Overwrite Comments in Array Body") );

	commentHeadOnly->setChecked(true);

	grid->addWidget( commentHeadOnly, 3, 1 );
	grid->addWidget( arrayCommentOverWrite, 4, 1 );

	hbox         = new QHBoxLayout();
	deleteBox    = new QCheckBox( tr("Delete") );
	okButton     = new QPushButton( tr("OK") );
	cancelButton = new QPushButton( tr("Cancel") );

	//okButton->setIcon( style()->standardIcon( QStyle::SP_DialogApplyButton ) );
	okButton->setIcon( style()->standardIcon( QStyle::SP_DialogOkButton ) );
	cancelButton->setIcon( style()->standardIcon( QStyle::SP_DialogCancelButton ) );

	mainLayout->addLayout( hbox );
	hbox->addWidget( deleteBox    );
	hbox->addWidget( cancelButton );
	hbox->addWidget(     okButton );

	connect(     okButton, SIGNAL(clicked(void)), this, SLOT(accept(void)) );
   connect( cancelButton, SIGNAL(clicked(void)), this, SLOT(reject(void)) );

	deleteBox->setEnabled( false );
	okButton->setDefault(true);

	if ( sym != NULL )
	{
		nameEntry->setText( tr(sym->name.c_str()) );
		commentEntry->setPlainText( tr(sym->comment.c_str()) );
	}

	setLayout( mainLayout );

	updateArraySensitivity();

	connect( isArrayBox     , SIGNAL(stateChanged(int)), this, SLOT(isArrayStateChanged(int)) );
	connect( commentHeadOnly, SIGNAL(stateChanged(int)), this, SLOT(arrayCommentHeadOnlyChanged(int)) );
}
//--------------------------------------------------------------
SymbolEditWindow::~SymbolEditWindow(void)
{

}
//--------------------------------------------------------------
void SymbolEditWindow::closeEvent(QCloseEvent *event)
{
   printf("Debugger Close Window Event\n");
   done(0);
	deleteLater();
   event->accept();
}
//--------------------------------------------------------------
void SymbolEditWindow::closeWindow(void)
{
   //printf("Close Window\n");
   done(0);
	deleteLater();
}
//--------------------------------------------------------------
void SymbolEditWindow::updateArraySensitivity(void)
{
	if ( isArrayBox->isChecked() )
	{
		arraySize->setEnabled( true );
		arrayInit->setEnabled( true );
		arraySizeLbl[0]->setEnabled( true );
		arraySizeLbl[1]->setEnabled( true );
		arrayInitLbl->setEnabled( true );
		commentHeadOnly->setEnabled( true );
		arrayNameOverWrite->setEnabled( true );

		arrayCommentOverWrite->setEnabled( !commentHeadOnly->isChecked() );
	}
	else
	{
		arraySize->setEnabled( false );
		arrayInit->setEnabled( false );
		arraySizeLbl[0]->setEnabled( false );
		arraySizeLbl[1]->setEnabled( false );
		arrayInitLbl->setEnabled( false );
		commentHeadOnly->setEnabled( false );
		arrayNameOverWrite->setEnabled( false );
		arrayCommentOverWrite->setEnabled( false );
	}

}
//--------------------------------------------------------------
void SymbolEditWindow::isArrayStateChanged( int state )
{
	updateArraySensitivity();
}
//--------------------------------------------------------------
void SymbolEditWindow::arrayCommentHeadOnlyChanged( int state )
{
	updateArraySensitivity();
}
//--------------------------------------------------------------
void SymbolEditWindow::setAddr( int addrIn )
{
	char stmp[512];

	addr = addrIn;

	sprintf( stmp, "%04X", addr );

	addrEntry->setText( tr(stmp) );

	if ( bank < 0 )
	{
		if ( addr < 0x8000 )
		{
		   bank = -1;
		}
		else
		{
		   bank = getBank( addr );
		}
	}

	generateNLFilenameForAddress( addr, stmp );

	filepath->setText( tr(stmp) );
	filepath->setMinimumWidth( charWidth * (filepath->text().size() + 4) );
}
//--------------------------------------------------------------
void SymbolEditWindow::setBank( int bankIn )
{
	bank = bankIn;
}
//--------------------------------------------------------------
void SymbolEditWindow::setSym( debugSymbol_t *symIn )
{
	sym = symIn;

	if ( sym != NULL )
	{
		nameEntry->setText( tr(sym->name.c_str()) );
		commentEntry->setPlainText( tr(sym->comment.c_str()) );
		deleteBox->setEnabled( true );

		determineArrayStart();
	}
	else
	{
		nameEntry->clear();
		commentEntry->clear();
		deleteBox->setEnabled( false );

		arrayInit->setText( tr("00") );
	}
}
//--------------------------------------------------------------
int SymbolEditWindow::exec(void)
{
	int ret, b, a, size, init, i;
	bool isNew = 0;

	ret = QDialog::exec();

	if ( ret == QDialog::Accepted )
	{
		fceuWrapperLock();
		if ( isArrayBox->isChecked() )
		{
			size = atoi( arraySize->text().toStdString().c_str() );
			init = atoi( arrayInit->text().toStdString().c_str() );

			for (i=0; i<size; i++)
			{
				isNew = false;

				a = addr + i;

				if ( a < 0x8000 )
				{
				   b = -1;
				}
				else
				{
				   b = getBank( a );
				}
				
				sym = debugSymbolTable.getSymbolAtBankOffset( b, a );

				if ( deleteBox->isChecked() )
				{
					if ( sym != NULL )
					{
						debugSymbolTable.deleteSymbolAtBankOffset( b, a );
					}
				}
				else
				{
					if ( sym == NULL )
					{
						sym = new debugSymbol_t();

						sym->ofs = a;

						debugSymbolTable.addSymbolAtBankOffset( b, a, sym );

						isNew = true;
					}
					sym->ofs = a;

					if ( (i == 0) || isNew || arrayNameOverWrite->isChecked() )
					{
						setSymNameWithArray( init + i );
					}
					if ( (i == 0) || !commentHeadOnly->isChecked() )
					{
						if ( isNew || arrayCommentOverWrite->isChecked() || (i == 0) )
						{
							sym->comment = commentEntry->toPlainText().toStdString();
						}
					}
					sym->trimTrailingSpaces();
				}
			}
		}
		else
		{
			if ( deleteBox->isChecked() )
			{
				if ( sym != NULL )
				{
					debugSymbolTable.deleteSymbolAtBankOffset( bank, addr );
				}
			}
			else if ( sym == NULL )
			{
				sym = new debugSymbol_t();
				sym->ofs     = addr;
				sym->name    = nameEntry->text().toStdString();
				sym->comment = commentEntry->toPlainText().toStdString();

				debugSymbolTable.addSymbolAtBankOffset( bank, addr, sym );
			}
			else
			{
				sym->name    = nameEntry->text().toStdString();
				sym->comment = commentEntry->toPlainText().toStdString();
			}
			sym->trimTrailingSpaces();
		}
		fceuWrapperUnLock();
	}
	return ret;
}
//--------------------------------------------------------------
void SymbolEditWindow::determineArrayStart(void)
{
	int i,j;
	char stmp[512];
	char digits[128];

	if ( sym == NULL )
	{
		return;
	}
	strcpy( stmp, nameEntry->text().toStdString().c_str() );

	// Find Current Array Braces
	i=0;
	while ( stmp[i] != 0 )
	{
		if ( stmp[i] == '[' )
		{
			break;
		}
		i++;
	}

	if ( stmp[i] != '[' )
	{
		return;
	}
	i++;

	while ( isspace( stmp[i] ) ) i++;

	j=0;
	while ( isdigit( stmp[i] ) )
	{
		digits[j] = stmp[i]; i++; j++;
	}
	digits[j] = 0;

	while ( isspace( stmp[i] ) ) i++;

	if ( stmp[i] != ']' )
	{
		return;
	}

	if ( j > 0 )
	{
		int val;

		val = atoi( digits );

		if ( (val >= 0) && (val < 256) )
		{
			sprintf( digits, "%02X", val );

			arrayInit->setText( tr(digits) );
		}
	}

	return;
}
//--------------------------------------------------------------
void SymbolEditWindow::setSymNameWithArray(int idx)
{
	int i;
	char stmp[512];

	if ( sym == NULL )
	{
		return;
	}
	strcpy( stmp, nameEntry->text().toStdString().c_str() );

	// Remove Current Array Braces
	i=0;
	while ( stmp[i] != 0 )
	{
		if ( stmp[i] == '[' )
		{
			stmp[i] = 0; break;
		}
		i++;
	}
	i--;

	// Remove Trailing Spaces
	while ( i >= 0 )
	{
		if ( isspace( stmp[i] ) )
		{
			stmp[i] = 0;
		}
		else
		{
			break;
		}
		i--;
	}

	// Reform with base string and new index.
	sym->name.assign( stmp );

	sym->trimTrailingSpaces();

	sprintf( stmp, "[%i]", idx );

	sym->name.append( stmp );

}
//--------------------------------------------------------------
