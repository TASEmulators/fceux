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

};

extern  debugSymbolTable_t  debugSymbolTable;

//struct MemoryMappedRegister
//{
//	char* offset;
//	char* name;
//};


int generateNLFilenameForBank(int bank, char *NLfilename);
int generateNLFilenameForAddress(int address, char *NLfilename);

#endif
