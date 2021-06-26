int Assemble(unsigned char *output, int addr, char *str);
char *Disassemble(int addr, uint8 *opcode);
char *DisassembleLine(int addr);
char *DisassembleData(int addr, uint8 *opcode);
