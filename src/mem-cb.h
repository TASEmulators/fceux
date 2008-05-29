#ifndef MEM_CB_H
#define MEM_CB_H

#include <map>

void FCEU_AddressReadCallback(uint32 A, readfunc cb);
void FCEU_AddressWriteCallback(uint32 A, writefunc cb);
uint32 FCEU_AddressCanonicalize(uint32 A);

extern struct MEMCALLBACKS
{
	std::map<uint32,  readfunc> * read_cb;
	std::map<uint32, writefunc> *write_cb;
	~MEMCALLBACKS();
} memCallbacks;

#endif
