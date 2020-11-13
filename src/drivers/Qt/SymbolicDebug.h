// SymbolicDebug.h
//
#ifndef __SYMBOLIC_DEBUG_H__
#define __SYMBOLIC_DEBUG_H__

#include <string>
#include <list>
#include <map>

struct debugSymbol_t 
{
	int   ofs;
	std::string  name;
	std::string  comment;

	debugSymbol_t(void)
	{
		ofs = 0;
	};

	debugSymbol_t( int ofs, const char *name, const char *comment = NULL )
	{
		this->ofs = ofs;
		this->name.assign( name );

		if ( comment )
		{
			this->comment.assign( comment );
		}
	}

	void trimTrailingSpaces(void)
	{
		while ( name.size() > 0 )
		{
			if ( isspace( name.back() ) )
			{
				name.pop_back();
			}
			else
			{
				break;
			}
		}
		while ( comment.size() > 0 )
		{
			if ( isspace( comment.back() ) )
			{
				comment.pop_back();
			}
			else
			{
				break;
			}
		}
	}
};

struct debugSymbolPage_t
{
	int pageNum;

	debugSymbolPage_t(void);
	~debugSymbolPage_t(void);

	int  save(void);
	void print(void);
	int size(void){ return symMap.size(); }

	int addSymbol( debugSymbol_t *sym );

	debugSymbol_t *getSymbolAtOffset( int ofs );

	std::map <int, debugSymbol_t*> symMap;
};

class debugSymbolTable_t
{

	public:
		debugSymbolTable_t(void);
		~debugSymbolTable_t(void);

		int loadFileNL( int addr );
		int loadGameSymbols(void);
		int numPages(void){ return pageMap.size(); }

		void save(void);
		void clear(void);
		void print(void);

		debugSymbol_t *getSymbolAtBankOffset( int bank, int ofs );

		int addSymbolAtBankOffset(  int bank, int ofs, debugSymbol_t *sym );

	private:
		std::map <int, debugSymbolPage_t*> pageMap;

		int loadRegisterMap(void);

};

extern  debugSymbolTable_t  debugSymbolTable;

//struct MemoryMappedRegister
//{
//	char* offset;
//	char* name;
//};


int generateNLFilenameForBank(int bank, char *NLfilename);
int generateNLFilenameForAddress(int address, char *NLfilename);

#define  ASM_DEBUG_SYMS       0x0001
#define  ASM_DEBUG_REGS       0x0002
#define  ASM_DEBUG_REPLACE    0x0004
#define  ASM_DEBUG_ADDR_02X   0x0008

int DisassembleWithDebug(int addr, uint8_t *opcode, int flags, char *str, debugSymbol_t *symOut = NULL, debugSymbol_t *symOut2 = NULL );

#endif
