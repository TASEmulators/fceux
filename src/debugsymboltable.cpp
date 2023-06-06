/// \file
/// \brief Implements debug symbol table (from .nl files)

#include "debugsymboltable.h"

#include "types.h"
#include "debug.h"
#include "fceu.h"
#include "cart.h"
#include "ld65dbg.h"

#ifdef __QT_DRIVER__
#include "Qt/ConsoleUtilities.h"
#else
extern char LoadedRomFName[4096];
static inline const char* getRomFile() { return LoadedRomFName; }
#endif

extern FCEUGI *GameInfo;

debugSymbolTable_t  debugSymbolTable;

static char dbgSymTblErrMsg[256] = {0};
static bool dbgSymAllowDuplicateNames = true;
//--------------------------------------------------------------
// debugSymbol_t
//--------------------------------------------------------------
int debugSymbol_t::updateName( const char *name, int arrayIndex )
{
	std::string newName;

	newName.assign( name );

	while ( newName.size() > 0 )
	{
		if ( isspace( newName.back() ) )
		{
			newName.pop_back();
		}
		else
		{
			break;
		}
	}

	if (arrayIndex >= 0)
	{
		char stmp[32];

		sprintf( stmp, "[%i]", arrayIndex );

		newName.append(stmp);
	}

	if (page)
	{
		debugSymbol_t *dupSym = debugSymbolTable.getSymbol( page->pageNum(), newName );

		if (!dbgSymAllowDuplicateNames && dupSym != nullptr && dupSym != this)
		{
			snprintf( dbgSymTblErrMsg, sizeof(dbgSymTblErrMsg), "Error: debug symbol '%s' already exists in %s page.\n", newName.c_str(), page->pageName() );
			return -1;
		}
	}
	_name = newName;

	debugSymbolTable.updateSymbol(this);

	return 0;
}
//--------------------------------------------------------------
void debugSymbol_t::trimTrailingSpaces(void)
{
	while ( _name.size() > 0 )
	{
		if ( isspace( _name.back() ) )
		{
			_name.pop_back();
		}
		else
		{
			break;
		}
	}
	while ( _comment.size() > 0 )
	{
		if ( isspace( _comment.back() ) )
		{
			_comment.pop_back();
		}
		else
		{
			break;
		}
	}
}
//--------------------------------------------------------------
// debugSymbolPage_t
//--------------------------------------------------------------
debugSymbolPage_t::debugSymbolPage_t(int page)
{
	_pageNum = page;

	_pageName[0] = 0;

	if (page == -2)
	{
		strcpy( _pageName, "REG");
	}
	else if (page == -1)
	{
		strcpy( _pageName, "RAM");
	}
	else
	{
		snprintf( _pageName, sizeof(_pageName), "%X", page);
		_pageName[sizeof(_pageName)-1] = 0;
	}
}
//--------------------------------------------------------------
debugSymbolPage_t::~debugSymbolPage_t(void)
{
	for (auto it=symMap.begin(); it!=symMap.end(); it++)
	{
		delete it->second;
	}
}
//--------------------------------------------------------------
int debugSymbolPage_t::addSymbol( debugSymbol_t*sym )
{
	// Check if symbol already is loaded by that name or offset
	if ( symMap.count( sym->offset() ) )
	{
		snprintf( dbgSymTblErrMsg, sizeof(dbgSymTblErrMsg), "Error: symbol offset 0x%04X already has an entry on %s page\n", sym->offset(), _pageName );
		return -1;
	}
	if ( !dbgSymAllowDuplicateNames && (sym->name().size() > 0) && symNameMap.count( sym->name() ) )
	{
		snprintf( dbgSymTblErrMsg, sizeof(dbgSymTblErrMsg), "Error: symbol name '%s' already exists on %s page\n", sym->name().c_str(), _pageName );
		return -1;
	}

	symMap[ sym->offset() ] = sym;

	sym->page = this;

	// Comment only lines don't need to have a name.
	if (sym->name().size() > 0)
	{
		symNameMap[ sym->name() ] = sym;
	}

	return 0;
}
//--------------------------------------------------------------
debugSymbol_t *debugSymbolPage_t::getSymbolAtOffset( int ofs )
{
	auto it = symMap.find( ofs );
	return it != symMap.end() ? it->second : nullptr;
}
//--------------------------------------------------------------
debugSymbol_t *debugSymbolPage_t::getSymbol( const std::string &name )
{
	auto it = symNameMap.find( name );
	return it != symNameMap.end() ? it->second : nullptr;
}
//--------------------------------------------------------------
int debugSymbolPage_t::deleteSymbolAtOffset( int ofs )
{
	auto it = symMap.find( ofs );

	if ( it != symMap.end() )
	{
		auto sym = it->second;

		if ( sym->name().size() > 0 )
		{
			auto itName = symNameMap.find( sym->name() );

			if ( (itName != symNameMap.end()) && (itName->second == sym) )
			{
				symNameMap.erase(itName);
			}
		}
		symMap.erase(it);
		delete sym;

		return 0;
	}
	return -1;
}
//--------------------------------------------------------------
int debugSymbolPage_t::updateSymbol(debugSymbol_t *sym)
{
	auto itName = symNameMap.begin();

	while (itName != symNameMap.end())
	{
		if (itName->second == sym)
		{
			if (sym->name().size() == 0 || sym->name().compare( itName->first ) )
			{
				//printf("Changing Name: %s %s\n", itName->first.c_str(), sym->name().c_str() );
				itName = symNameMap.erase(itName);
			}
			break;
		}
		else
		{
			itName++;
		}
	}
	if (sym->name().size() > 0)
	{
		symNameMap[ sym->name() ] = sym;
	}

	// Sanity Check
	auto it = symMap.find( sym->offset() );

	if ( it == symMap.end() )
	{	// This shouldn't happen
		return -1;
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
	std::string filename;
	char stmp[512];
	int i,j;

	if ( symMap.size() == 0 )
	{
		//printf("Skipping Empty Debug Page Save\n");
		return 0;
	}
	if ( _pageNum == -2 )
	{
		//printf("Skipping Register Debug Page Save\n");
		return 0;
	}

	romFile = getRomFile();

	if ( romFile == nullptr )
	{
		return -1;
	}
	i=0;
	while ( romFile[i] != 0 )
	{

		if ( romFile[i] == '|' )
		{
			filename.push_back('.');
		}
		else
		{
			filename.push_back(romFile[i]);
		}
		i++;
	}

	if ( _pageNum < 0 )
	{
		filename.append(".ram.nl" );
	}
	else
	{
		char suffix[32];

		sprintf( suffix, ".%X.nl", _pageNum );

		filename.append( suffix );
	}

	fp = ::fopen( filename.c_str(), "w" );

	if ( fp == nullptr )
	{
		FCEU_printf("Error: Could not open file '%s' for writing\n", filename.c_str() );
		return -1;
	}

	for (it=symMap.begin(); it!=symMap.end(); it++)
	{
		const char *c;

		sym = it->second;

		i=0; j=0; c = sym->_comment.c_str();

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

		fprintf( fp, "$%04X#%s#%s\n", sym->offset(), sym->name().c_str(), stmp );

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

	fprintf( fp, "Page: %X \n", _pageNum );

	for (it=symMap.begin(); it!=symMap.end(); it++)
	{
		sym = it->second;

		fprintf( fp, "   Sym: $%04X '%s' \n", sym->ofs, sym->name().c_str() );
	}
}
//--------------------------------------------------------------
// debugSymbolTable_t
//--------------------------------------------------------------
debugSymbolTable_t::debugSymbolTable_t(void)
{
	cs = new FCEU::mutex();

	dbgSymTblErrMsg[0] = 0;
}
//--------------------------------------------------------------
debugSymbolTable_t::~debugSymbolTable_t(void)
{
	this->clear();

	if (cs)
	{
		delete cs;
	}
}
//--------------------------------------------------------------
void debugSymbolTable_t::clear(void)
{
	FCEU::autoScopedLock alock(cs);

	std::map <int, debugSymbolPage_t*>::iterator it;

	for (it=pageMap.begin(); it!=pageMap.end(); it++)
	{
		delete it->second;
	}
	pageMap.clear();
}
//--------------------------------------------------------------
int debugSymbolTable_t::numSymbols(void)
{
	int n = 0;
	FCEU::autoScopedLock alock(cs);

	for (auto it=pageMap.begin(); it!=pageMap.end(); it++)
	{
		n += it->second->size();
	}
	return n;
}
//--------------------------------------------------------------
static int generateNLFilenameForBank(int bank, std::string &NLfilename)
{
	int i;
	const char *romFile;

	romFile = getRomFile();

	if ( romFile == nullptr )
	{
		return -1;
	}
	i=0;
	while ( romFile[i] != 0 )
	{

		if ( romFile[i] == '|' )
		{
			NLfilename.push_back('.');
		}
		else
		{
			NLfilename.push_back(romFile[i]);
		}
		i++;
	}

	if (bank < 0)
	{
		// The NL file for the RAM addresses has the name nesrom.nes.ram.nl
		NLfilename.append(".ram.nl");
	}
	else
	{
		char stmp[64];
		#ifdef DW3_NL_0F_1F_HACK
		if(bank == 0x0F)
			bank = 0x1F;
		#endif
		sprintf( stmp, ".%X.nl", bank);
		NLfilename.append( stmp );
	}
	return 0;
}
//--------------------------------------------------------------
int generateNLFilenameForAddress(int address, std::string &NLfilename)
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
int debugSymbolTable_t::loadFileNL( int bank )
{
	FILE *fp;
	int i, j, ofs, lineNum = 0, literal = 0, array = 0;
	std::string fileName;
	char stmp[512], line[512];
	debugSymbolPage_t *page = nullptr;
	debugSymbol_t *sym = nullptr;
	FCEU::autoScopedLock alock(cs);

	//printf("Looking to Load Debug Bank: $%X \n", bank );

	if ( generateNLFilenameForBank( bank, fileName ) )
	{
		return -1;
	}
	//printf("Loading NL File: %s\n", fileName.c_str() );

	fp = ::fopen( fileName.c_str(), "r" );

	if ( fp == nullptr )
	{
		return -1;
	}
	page = new debugSymbolPage_t(bank);

	pageMap[ page->pageNum() ] = page;

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
			if ( sym != nullptr )
			{
				sym->_comment.append( stmp );
			}
		}
		else if ( line[i] == '$' )
		{
			// Line is a new debug offset
			array = 0;

			j=0; i++;
			if ( !isxdigit( line[i] ) )
			{
				FCEU_printf("Error: Invalid Offset on Line %i of File %s\n", lineNum, fileName.c_str() );
			}
			while ( isxdigit( line[i] ) )
			{
				stmp[j] = line[i]; i++; j++;
			}
			stmp[j] = 0;

			ofs = strtol( stmp, nullptr, 16 );

			if ( line[i] == '/' )
			{
				j=0; i++;
				while ( isxdigit( line[i] ) )
				{
					stmp[j] = line[i]; i++; j++;
				}
				stmp[j] = 0;

				array = strtol( stmp, nullptr, 16 );
			}

			if ( line[i] != '#' )
			{
				FCEU_printf("Error: Missing field delimiter following offset $%X on Line %i of File %s\n", ofs, lineNum, fileName.c_str() );
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
				FCEU_printf("Error: Missing field delimiter following name '%s' on Line %i of File %s\n", stmp, lineNum, fileName.c_str() );
				continue;
			}
			i++;

			sym = new debugSymbol_t( ofs, stmp );

			if ( sym == nullptr )
			{
				FCEU_printf("Error: Failed to allocate memory for offset $%04X Name '%s' on Line %i of File %s\n", ofs, stmp, lineNum, fileName.c_str() );
				continue;
			}
			sym->ofs = ofs;
			sym->_name.assign( stmp );

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

			sym->_comment.assign( stmp );

			if ( array > 0 )
			{
				debugSymbol_t *arraySym = nullptr;

				for (j=0; j<array; j++)
				{
					arraySym = new debugSymbol_t();

					if ( arraySym )
					{
						arraySym->ofs = sym->ofs + j;

						sprintf( stmp, "[%i]", j );
						arraySym->_name.assign( sym->name() );
						arraySym->_name.append( stmp );
						arraySym->_comment.assign( sym->comment() );

						if ( page->addSymbol( arraySym ) )
						{
							FCEU_printf("Error: Failed to add symbol for offset $%04X Name '%s' on Line %i of File %s\n", ofs, arraySym->name().c_str(), lineNum, fileName.c_str() );
							FCEU_printf("%s\n", errorMessage() );
							delete arraySym; arraySym = nullptr; // Failed to add symbol
						}
					}
				}
				delete sym; sym = nullptr; // Delete temporary symbol
			}
			else
			{
				if ( page->addSymbol( sym ) )
				{
					FCEU_printf("Error: Failed to add symbol for offset $%04X Name '%s' on Line %i of File %s\n", ofs, sym->name().c_str(), lineNum, fileName.c_str() );
					FCEU_printf("%s\n", errorMessage() );
					delete sym; sym = nullptr; // Failed to add symbol
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
	FCEU::autoScopedLock alock(cs);
	debugSymbolPage_t *page;

	page = new debugSymbolPage_t(-2);

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

	pageMap[ page->pageNum() ] = page;

	return 0;
}
//--------------------------------------------------------------
int debugSymbolTable_t::loadGameSymbols(void)
{
	int nPages, pageSize, romSize = 0x10000;

	this->clear();

	if ( GameInfo != nullptr )
	{
		romSize = NES_HEADER_SIZE + CHRsize[0] + PRGsize[0];
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
int debugSymbolTable_t::addSymbolAtBankOffset(int bank, int ofs, const char *name, const char *comment)
{
	int result = -1;
	debugSymbol_t *sym = new debugSymbol_t(ofs, name, comment);

	result = addSymbolAtBankOffset(bank, ofs, sym);

	if (result)
	{	// Symbol add failed
		delete sym;
	}
	return result;
}
//--------------------------------------------------------------
int debugSymbolTable_t::addSymbolAtBankOffset( int bank, int ofs, debugSymbol_t *sym )
{
	int result = -1;
	debugSymbolPage_t *page;
	std::map <int, debugSymbolPage_t*>::iterator it;
	FCEU::autoScopedLock alock(cs);

	it = pageMap.find( bank );

	if ( it == pageMap.end() )
	{
		page = new debugSymbolPage_t(bank);
		pageMap[ bank ] = page;
	}
	else
	{
		page = it->second;
	}
	result = page->addSymbol( sym );

	return result;
}
//--------------------------------------------------------------
int debugSymbolTable_t::deleteSymbolAtBankOffset( int bank, int ofs )
{
	debugSymbolPage_t *page;
	std::map <int, debugSymbolPage_t*>::iterator it;
	FCEU::autoScopedLock alock(cs);

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
int debugSymbolTable_t::updateSymbol(debugSymbol_t *sym)
{
	FCEU::autoScopedLock alock(cs);

	if (sym->page == nullptr)
	{
		return -1;
	}
	return sym->page->updateSymbol(sym);
}
//--------------------------------------------------------------
debugSymbol_t *debugSymbolTable_t::getSymbolAtBankOffset( int bank, int ofs )
{
	FCEU::autoScopedLock alock(cs);

	auto it = pageMap.find( bank );

	return it != pageMap.end() ? it->second->getSymbolAtOffset( ofs ) : nullptr;
}
//--------------------------------------------------------------
debugSymbol_t *debugSymbolTable_t::getSymbol( int bank, const std::string &name )
{
	FCEU::autoScopedLock alock(cs);

	auto it = pageMap.find( bank );

	return it != pageMap.end() ? it->second->getSymbol( name ) : nullptr;
}
//--------------------------------------------------------------
debugSymbol_t *debugSymbolTable_t::getSymbolAtAnyBank( const std::string &name )
{
	FCEU::autoScopedLock alock(cs);

	for (auto &page : pageMap)
	{
		auto sym = getSymbol( page.first, name );

		if ( sym )
		{
			return sym;
		}
	}

	return nullptr;
}
//--------------------------------------------------------------
void debugSymbolTable_t::save(void)
{
	debugSymbolPage_t *page;
	std::map <int, debugSymbolPage_t*>::iterator it;
	FCEU::autoScopedLock alock(cs);

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
	FCEU::autoScopedLock alock(cs);

	for (it=pageMap.begin(); it!=pageMap.end(); it++)
	{
		page = it->second;

		page->print();
	}
}
//--------------------------------------------------------------
const char *debugSymbolTable_t::errorMessage(void)
{
	return dbgSymTblErrMsg;
}
//--------------------------------------------------------------
static void ld65_iterate_cb( void *userData, ld65::sym *s )
{
	debugSymbolTable_t *tbl = static_cast<debugSymbolTable_t*>(userData);

	if (tbl)
	{
		tbl->ld65_SymbolLoad(s);
	}
}
//--------------------------------------------------------------
void debugSymbolTable_t::ld65_SymbolLoad( ld65::sym *s )
{
	int bank = -1;
	debugSymbol_t *sym;
	debugSymbolPage_t *page;
	ld65::scope *scope = s->getScope();
	ld65::segment *seg = s->getSegment();

	if ( s->type() == ld65::sym::LABEL )
	{
		//printf("Symbol Label Load: name:\"%s\"  val:%i  0x%x\n", s->name(), s->value(), s->value() );
		if (seg)
		{
			int romAddr = seg->ofs() - NES_HEADER_SIZE;

			bank =  romAddr >= 0 ? romAddr / (1<<debuggerPageSize) : -1;

			//printf("  Seg: name:'%s'  ofs:%i  Bank:%x\n", seg->name(), romAddr, bank );
		}
		//printf("\n");

		auto pageIt = pageMap.find(bank);

		if (pageIt == pageMap.end() )
		{
			page = new debugSymbolPage_t(bank);

			pageMap[bank] = page;
		}
		else
		{
			page = pageIt->second;
		}
		std::string name;

		if (scope)
		{
			scope->getFullName(name);
		}
		name.append(s->name());

		//printf("Creating Symbol: %s\n", name.c_str() );

		sym = new debugSymbol_t( s->value(), name.c_str() );

		if ( page->addSymbol( sym ) )
		{
			//printf("Failed to load sym: id:%i name:'%s' bank:%i \n", s->id(), s->name(), bank );
			delete sym;
		}
	}
}
//--------------------------------------------------------------
int debugSymbolTable_t::ld65LoadDebugFile( const char *dbgFilePath )
{
	ld65::database db;

	if ( db.dbgFileLoad( dbgFilePath ) )
	{
		return -1;
	}
	FCEU::autoScopedLock alock(cs);

	db.iterateSymbols( this, ld65_iterate_cb );

	return 0;
}
//--------------------------------------------------------------
