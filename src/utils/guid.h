#ifndef _guid_h_
#define _guid_h_

#include <string>
#include "../types.h"
#include "valuearray.h"

struct FCEU_Guid : public ValueArray<uint8,16>
{
	FCEU_Guid() : ValueArray<uint8,16>() {}
	// The all-zero GUID represents an invalid or unset value.
	bool isValid() const
	{
		for(size_t i=0;i<size;i++)
			if(data[i] != 0)
				return true;
		return false;
	}
	void newGuid();
	std::string toString();
	static FCEU_Guid fromString(std::string str);
	static uint8 hexToByte(char** ptrptr);
	void scan(std::string& str);
};


#endif
