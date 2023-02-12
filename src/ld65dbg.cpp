// ld65dbg.cpp
#include <stdio.h>

#include "ld65dbg.h"


namespace ld65
{
	//---------------------------------------------------------------------------------------------------
	segment::segment( int id, const char *name, int startAddr, int size, int ofs, unsigned char type )
		: _name(name ? name : ""), _id(id), _startAddr(startAddr), _size(size), _ofs(ofs), _type(type)
	{
	}
	//---------------------------------------------------------------------------------------------------
	scope::scope(int id, const char *name, int size, int parentID)
		: _name(name ? name : ""), _id(id), _parentID(parentID), _size(size), _parent(nullptr)
	{
	}
	//---------------------------------------------------------------------------------------------------
	sym::sym(int id, const char *name, int size)
		: _name(name ? name : ""), _id(id), _size(size)
	{
	}
	//---------------------------------------------------------------------------------------------------
	database::database(void)
	{
	}
	//---------------------------------------------------------------------------------------------------
}
