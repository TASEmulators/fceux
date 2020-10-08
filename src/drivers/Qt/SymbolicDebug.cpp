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
	int i, j, ofs, lineNum = 0, literal = 0;
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

			if ( page->addSymbol( sym ) )
			{  
				printf("Error: Failed to add symbol for offset $%04X Name '%s' on Line %i of File %s\n", ofs, stmp, lineNum, fileName );
				delete sym; sym = NULL; // Failed to add symbol
			}
		}
	}

	::fclose(fp);

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
