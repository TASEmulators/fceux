#ifndef _DEBUGSYMBOLTABLE_H_
#define _DEBUGSYMBOLTABLE_H_

#include <string>
#include <map>

#include "utils/mutex.h"

class debugSymbolPage_t;
class debugSymbolTable_t;

class debugSymbol_t
{
	public:
	debugSymbol_t(void)
	{
		ofs = 0;
	};

	debugSymbol_t( int ofs, const char *name = nullptr, const char *comment = nullptr )
	{
		this->ofs = ofs;

		if (name)
		{
			this->_name.assign(name);
		}
		if ( comment )
		{
			this->_comment.assign( comment );
		}
	}

	const std::string &name(void)
	{
		return _name;
	}

	const std::string &comment(void)
	{
		return _comment;
	}

	void commentAssign( std::string str )
	{
		_comment.assign(str);
		return;
	}

	void commentAssign( const char *str )
	{
		_comment.assign(str);
		return;
	}

	int offset(void)
	{
		return ofs;
	}

	void setOffset( int o )
	{
		if (o != ofs)
		{
			ofs = o;
		}
	}

	void updateName( const char *name, int arrayIndex = -1 )
	{
		_name.assign( name );

		trimTrailingSpaces();

		if (arrayIndex >= 0)
		{
			char stmp[32];

			sprintf( stmp, "[%i]", arrayIndex );

			_name.append(stmp);
		}
	}

	void trimTrailingSpaces(void)
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

	private:

	int   ofs;
	std::string  _name;
	std::string  _comment;

	friend class debugSymbolPage_t;
	friend class debugSymbolTable_t;
};

class debugSymbolPage_t
{
	public:
	debugSymbolPage_t(void);
	~debugSymbolPage_t(void);

	int  save(void);
	void print(void);
	int size(void){ return symMap.size(); }

	int addSymbol( debugSymbol_t *sym );

	int deleteSymbolAtOffset( int ofs );

	debugSymbol_t *getSymbolAtOffset( int ofs );

	debugSymbol_t *getSymbol( const std::string &name );

	private:
	int pageNum;
	std::map <int, debugSymbol_t*> symMap;
	std::map <std::string, debugSymbol_t*> symNameMap;

	friend class debugSymbolTable_t;
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

		int addSymbolAtBankOffset(int bank, int ofs, const char* name, const char* comment = nullptr);

		int deleteSymbolAtBankOffset( int bank, int ofs );

		const char *errorMessage(void);

	private:
		std::map <int, debugSymbolPage_t*> pageMap;
		FCEU::mutex *cs;

		int loadRegisterMap(void);

};

extern  debugSymbolTable_t  debugSymbolTable;

#endif
