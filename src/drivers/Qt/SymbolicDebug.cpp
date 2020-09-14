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
	char fileName[512], line[256];

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
	while ( fgets( line, sizeof(line), fp ) != 0 )
	{
		printf("%s", line );
	}

	::fclose(fp);

	return 0;
}
//--------------------------------------------------------------
int debugSymbolTable_t::loadGameSymbols(void)
{
	int nPages;

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

	return 0;
}
//--------------------------------------------------------------
