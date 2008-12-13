#ifndef __FCEU_ENDIAN
#define __FCEU_ENDIAN

#include <iosfwd>
#include <stdio.h>


int write16le(uint16 b, FILE *fp);
int write32le(uint32 b, FILE *fp);
int write32le(uint32 b, std::ostream* os);
int write64le(uint64 b, std::ostream* os);
int read64le(uint64 *Bufo, std::istream *is);
int read32le(uint32 *Bufo, std::istream *is);
int read32le(uint32 *Bufo, FILE *fp);
int read16le(uint16 *Bufo, std::istream *is);

void FlipByteOrder(uint8 *src, uint32 count);

void FCEU_en32lsb(uint8 *, uint32);
void FCEU_en16lsb(uint8* buf, uint16 val);
uint64 FCEU_de64lsb(uint8 *morp);
uint32 FCEU_de32lsb(uint8 *morp);
uint16 FCEU_de16lsb(uint8 *morp);

#endif

