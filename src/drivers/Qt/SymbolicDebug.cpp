// SymbolicDebug.cpp
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../types.h"
#include "../../fceu.h"
#include "../../debug.h"

#include "Qt/SymbolicDebug.h"
#include "Qt/ConsoleUtilities.h"

debugSymbolTable_t  debugSymbolTable;

//--------------------------------------------------------------
// debugSymbolPage_t
//--------------------------------------------------------------
debugSymbolPage_t::debugSymbolPage_t(void)
{
	pageNum = 0;

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

	if ( pageNum > 0 )
	{
		char suffix[32];

		sprintf( suffix, ".%X.nl", pageNum );

		strcat( stmp, suffix );
	}
	else
	{
		strcat( stmp, ".ram.nl" );
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
static int generateNLFilenameForAddress(int address, char *NLfilename)
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

	if (address < 0x8000)
	{
		// The NL file for the RAM addresses has the name nesrom.nes.ram.nl
		strcat(NLfilename, ".ram.nl");
	} 
	else
	{
		int bank = getBank(address);
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
int debugSymbolTable_t::loadFileNL( int addr )
{
	FILE *fp;
	int i, j, ofs, lineNum = 0, literal = 0;
	char fileName[512], line[512];
	char stmp[512];
	debugSymbolPage_t *page = NULL;
	debugSymbol_t *sym = NULL;

	printf("Looking to Load Debug Addr: $%04X \n", addr );

	if ( generateNLFilenameForAddress( addr, fileName ) )
	{
		return -1;
	}
	printf("Loading NL File: %s\n", fileName );

	fp = ::fopen( fileName, "r" );

	if ( fp == NULL )
	{
		return -1;
	}
	page = new debugSymbolPage_t;

	if ( addr >= 0x8000 )
	{
		page->pageNum = getBank( addr );
	}
	pageMap[ page->pageNum ] = page;

	while ( fgets( line, sizeof(line), fp ) != 0 )
	{
		i=0; lineNum++;
		printf("%4i:%s", lineNum, line );

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
	int nPages;

	this->save();
	this->clear();

	loadFileNL( 0x0000 );

	nPages = 1<<(15-debuggerPageSize);

	for(int i=0;i<nPages;i++)
	{
		int pageIndexAddress = 0x8000 + (1<<debuggerPageSize)*i;

		// Find out which bank is loaded at the page index
		//cb = getBank(pageIndexAddress);

		loadFileNL( pageIndexAddress );
	}

	print();

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
