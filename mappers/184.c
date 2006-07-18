#include "mapinc.h"



DECLFW(Mapper184_write)
{
VROM_BANK4(0x0000,V);
VROM_BANK4(0x1000,(V>>4));
}

void Mapper184_init(void)
{
  SetWriteHandler(0x6000,0xffff,Mapper184_write);
}

