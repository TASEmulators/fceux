int write16le(uint16 b, FILE *fp);
int write32le(uint32 b, FILE *fp);
int read32le(uint32 *Bufo, FILE *fp);
void FlipByteOrder(uint8 *src, uint32 count);

void FCEU_en32lsb(uint8 *, uint32);
uint32 FCEU_de32lsb(uint8 *);
