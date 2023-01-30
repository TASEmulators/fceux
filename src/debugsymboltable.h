#ifndef _DEBUGSYMBOLTABLE_H_
#define _DEBUGSYMBOLTABLE_H_

#include <string>
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

	int deleteSymbolAtOffset( int ofs );

	debugSymbol_t *getSymbolAtOffset( int ofs );

	debugSymbol_t *getSymbol( const std::string &name );

	std::map <int, debugSymbol_t*> symMap;
	std::map <std::string, debugSymbol_t*> symNameMap;
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

		debugSymbol_t *getSymbol( int bank, const std::string& name);

		debugSymbol_t *getSymbolAtAnyBank( const std::string& name);

		int addSymbolAtBankOffset( int bank, int ofs, debugSymbol_t *sym );

		int deleteSymbolAtBankOffset( int bank, int ofs );

	private:
		std::map <int, debugSymbolPage_t*> pageMap;

		int loadRegisterMap(void);

};

extern  debugSymbolTable_t  debugSymbolTable;

#endif
