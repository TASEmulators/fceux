// SymbolicDebug.h
//
#ifndef __SYMBOLIC_DEBUG_H__
#define __SYMBOLIC_DEBUG_H__

#include <string>
#include <list>
#include <map>

struct debugSymbol_t 
{
	int   addr;
	std::string  name;
	std::string  comment;

	debugSymbol_t(void)
	{
		addr = 0;
	};
};

struct debugSymbolPage_t
{
	int pageNum;

	debugSymbolPage_t(void);
	~debugSymbolPage_t(void);

	std::map <int, debugSymbol_t*> symMap;
};

class debugSymbolTable_t
{

	public:
		debugSymbolTable_t(void);
		~debugSymbolTable_t(void);

		int loadFileNL( int addr );
		int loadGameSymbols(void);

		void clear(void);

	private:
		std::map <int, debugSymbolPage_t*> pageMap;
	

};

extern  debugSymbolTable_t  debugSymbolTable;

//struct MemoryMappedRegister
//{
//	char* offset;
//	char* name;
//};



#endif
