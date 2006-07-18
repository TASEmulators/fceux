#include "mapinc.h"



DECLFW(Mapper180_write)
{
ROM_BANK16(0xC000,V);
}

void Mapper180_init(void)
{
  SetWriteHandler(0x8000,0xffff,Mapper180_write);
}

