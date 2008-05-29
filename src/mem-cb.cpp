#include "types.h"
#include "mem-cb.h"

using namespace std;

struct MEMCALLBACKS memCallbacks;
MEMCALLBACKS::~MEMCALLBACKS()
{
	delete  read_cb;
	delete write_cb;
}

void FCEU_AddressReadCallback(uint32 A, readfunc cb)
{
	if (!memCallbacks. read_cb) memCallbacks. read_cb = new map<uint32,  readfunc>;
	(*memCallbacks. read_cb)[FCEU_AddressCanonicalize(A)] = cb;
}

void FCEU_AddressWriteCallback(uint32 A, writefunc cb)
{
	if (!memCallbacks.write_cb) memCallbacks.write_cb = new map<uint32, writefunc>;
	(*memCallbacks.write_cb)[FCEU_AddressCanonicalize(A)] = cb;
}

uint32 FCEU_AddressCanonicalize(uint32 A)
{
	if (A >= 0x0800 && A < 0x2000) return (A & 0x07FF);
	return A;
}
