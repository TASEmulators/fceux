
#define relative(a) { \
	if (((a)=opcode[1])&0x80) (a) = addr-(((a)-1)^0xFF); \
	else (a)+=addr; \
}
#define absolute(a) { \
	(a) = opcode[1] | opcode[2]<<8; \
}
#define zpIndex(a,i) { \
	(a) = opcode[1]+(i); \
}
#define indirectX(a) { \
	(a) = (opcode[1]+X.X)&0xFF; \
	(a) = GetMem((a)) | (GetMem((a)+1))<<8; \
}
#define indirectY(a) { \
	(a) = GetMem(opcode[1]) | (GetMem(opcode[1]+1))<<8; \
	(a) += X.Y; \
}


//odd, 1-byte opcodes
case 0x00: strcpy(str,"BRK"); break;
case 0x08: strcpy(str,"PHP"); break;
case 0x0A: strcpy(str,"ASL"); break;
case 0x18: strcpy(str,"CLC"); break;
case 0x28: strcpy(str,"PLP"); break;
case 0x2A: strcpy(str,"ROL"); break;
case 0x38: strcpy(str,"SEC"); break;
case 0x40: strcpy(str,"RTI"); break;
case 0x48: strcpy(str,"PHA"); break;
case 0x4A: strcpy(str,"LSR"); break;
case 0x58: strcpy(str,"CLI"); break;
case 0x60: strcpy(str,"RTS"); break;
case 0x68: strcpy(str,"PLA"); break;
case 0x6A: strcpy(str,"ROR"); break;
case 0x78: strcpy(str,"SEI"); break;
case 0x88: strcpy(str,"DEY"); break;
case 0x8A: strcpy(str,"TXA"); break;
case 0x98: strcpy(str,"TYA"); break;
case 0x9A: strcpy(str,"TXS"); break;
case 0xA8: strcpy(str,"TAY"); break;
case 0xAA: strcpy(str,"TAX"); break;
case 0xB8: strcpy(str,"CLV"); break;
case 0xBA: strcpy(str,"TSX"); break;
case 0xC8: strcpy(str,"INY"); break;
case 0xCA: strcpy(str,"DEX"); break;
case 0xD8: strcpy(str,"CLD"); break;
case 0xE8: strcpy(str,"INX"); break;
case 0xEA: strcpy(str,"NOP"); break;
case 0xF8: strcpy(str,"SED"); break;

//(Indirect,X)
case 0x01: strcpy(chr,"ORA"); goto _indirectx;
case 0x21: strcpy(chr,"AND"); goto _indirectx;
case 0x41: strcpy(chr,"EOR"); goto _indirectx;
case 0x61: strcpy(chr,"ADC"); goto _indirectx;
case 0x81: strcpy(chr,"STA"); goto _indirectx;
case 0xA1: strcpy(chr,"LDA"); goto _indirectx;
case 0xC1: strcpy(chr,"CMP"); goto _indirectx;
case 0xE1: strcpy(chr,"SBC"); goto _indirectx;
_indirectx:
	indirectX(tmp);
	sprintf(str,"%s ($%02X,X) @ $%04X = #$%02X", chr,opcode[1],tmp,GetMem(tmp));
	break;

//Zero Page
case 0x05: strcpy(chr,"ORA"); goto _zeropage;
case 0x06: strcpy(chr,"ASL"); goto _zeropage;
case 0x24: strcpy(chr,"BIT"); goto _zeropage;
case 0x25: strcpy(chr,"AND"); goto _zeropage;
case 0x26: strcpy(chr,"ROL"); goto _zeropage;
case 0x45: strcpy(chr,"EOR"); goto _zeropage;
case 0x46: strcpy(chr,"LSR"); goto _zeropage;
case 0x65: strcpy(chr,"ADC"); goto _zeropage;
case 0x66: strcpy(chr,"ROR"); goto _zeropage;
case 0x84: strcpy(chr,"STY"); goto _zeropage;
case 0x85: strcpy(chr,"STA"); goto _zeropage;
case 0x86: strcpy(chr,"STX"); goto _zeropage;
case 0xA4: strcpy(chr,"LDY"); goto _zeropage;
case 0xA5: strcpy(chr,"LDA"); goto _zeropage;
case 0xA6: strcpy(chr,"LDX"); goto _zeropage;
case 0xC4: strcpy(chr,"CPY"); goto _zeropage;
case 0xC5: strcpy(chr,"CMP"); goto _zeropage;
case 0xC6: strcpy(chr,"DEC"); goto _zeropage;
case 0xE4: strcpy(chr,"CPX"); goto _zeropage;
case 0xE5: strcpy(chr,"SBC"); goto _zeropage;
case 0xE6: strcpy(chr,"INC"); goto _zeropage;
_zeropage:
// ################################## Start of SP CODE ###########################
// Change width to %04X
	sprintf(str,"%s $%04X = #$%02X", chr,opcode[1],GetMem(opcode[1]));
// ################################## End of SP CODE ###########################
	break;

//#Immediate
case 0x09: strcpy(chr,"ORA"); goto _immediate;
case 0x29: strcpy(chr,"AND"); goto _immediate;
case 0x49: strcpy(chr,"EOR"); goto _immediate;
case 0x69: strcpy(chr,"ADC"); goto _immediate;
//case 0x89: strcpy(chr,"STA"); goto _immediate;  //baka, no STA #imm!!
case 0xA0: strcpy(chr,"LDY"); goto _immediate;
case 0xA2: strcpy(chr,"LDX"); goto _immediate;
case 0xA9: strcpy(chr,"LDA"); goto _immediate;
case 0xC0: strcpy(chr,"CPY"); goto _immediate;
case 0xC9: strcpy(chr,"CMP"); goto _immediate;
case 0xE0: strcpy(chr,"CPX"); goto _immediate;
case 0xE9: strcpy(chr,"SBC"); goto _immediate;
_immediate:
	sprintf(str,"%s #$%02X", chr,opcode[1]);
	break;

//Absolute
case 0x0D: strcpy(chr,"ORA"); goto _absolute;
case 0x0E: strcpy(chr,"ASL"); goto _absolute;
case 0x2C: strcpy(chr,"BIT"); goto _absolute;
case 0x2D: strcpy(chr,"AND"); goto _absolute;
case 0x2E: strcpy(chr,"ROL"); goto _absolute;
case 0x4D: strcpy(chr,"EOR"); goto _absolute;
case 0x4E: strcpy(chr,"LSR"); goto _absolute;
case 0x6D: strcpy(chr,"ADC"); goto _absolute;
case 0x6E: strcpy(chr,"ROR"); goto _absolute;
case 0x8C: strcpy(chr,"STY"); goto _absolute;
case 0x8D: strcpy(chr,"STA"); goto _absolute;
case 0x8E: strcpy(chr,"STX"); goto _absolute;
case 0xAC: strcpy(chr,"LDY"); goto _absolute;
case 0xAD: strcpy(chr,"LDA"); goto _absolute;
case 0xAE: strcpy(chr,"LDX"); goto _absolute;
case 0xCC: strcpy(chr,"CPY"); goto _absolute;
case 0xCD: strcpy(chr,"CMP"); goto _absolute;
case 0xCE: strcpy(chr,"DEC"); goto _absolute;
case 0xEC: strcpy(chr,"CPX"); goto _absolute;
case 0xED: strcpy(chr,"SBC"); goto _absolute;
case 0xEE: strcpy(chr,"INC"); goto _absolute;
_absolute:
	absolute(tmp);
	sprintf(str,"%s $%04X = #$%02X", chr,tmp,GetMem(tmp));
	break;

//branches
case 0x10: strcpy(chr,"BPL"); goto _branch;
case 0x30: strcpy(chr,"BMI"); goto _branch;
case 0x50: strcpy(chr,"BVC"); goto _branch;
case 0x70: strcpy(chr,"BVS"); goto _branch;
case 0x90: strcpy(chr,"BCC"); goto _branch;
case 0xB0: strcpy(chr,"BCS"); goto _branch;
case 0xD0: strcpy(chr,"BNE"); goto _branch;
case 0xF0: strcpy(chr,"BEQ"); goto _branch;
_branch:
	relative(tmp);
	sprintf(str,"%s $%04X", chr,tmp);
	break;

//(Indirect),Y
case 0x11: strcpy(chr,"ORA"); goto _indirecty;
case 0x31: strcpy(chr,"AND"); goto _indirecty;
case 0x51: strcpy(chr,"EOR"); goto _indirecty;
case 0x71: strcpy(chr,"ADC"); goto _indirecty;
case 0x91: strcpy(chr,"STA"); goto _indirecty;
case 0xB1: strcpy(chr,"LDA"); goto _indirecty;
case 0xD1: strcpy(chr,"CMP"); goto _indirecty;
case 0xF1: strcpy(chr,"SBC"); goto _indirecty;
_indirecty:
	indirectY(tmp);
	sprintf(str,"%s ($%02X),Y @ $%04X = #$%02X", chr,opcode[1],tmp,GetMem(tmp));
	break;

//Zero Page,X
case 0x15: strcpy(chr,"ORA"); goto _zeropagex;
case 0x16: strcpy(chr,"ASL"); goto _zeropagex;
case 0x35: strcpy(chr,"AND"); goto _zeropagex;
case 0x36: strcpy(chr,"ROL"); goto _zeropagex;
case 0x55: strcpy(chr,"EOR"); goto _zeropagex;
case 0x56: strcpy(chr,"LSR"); goto _zeropagex;
case 0x75: strcpy(chr,"ADC"); goto _zeropagex;
case 0x76: strcpy(chr,"ROR"); goto _zeropagex;
case 0x94: strcpy(chr,"STY"); goto _zeropagex;
case 0x95: strcpy(chr,"STA"); goto _zeropagex;
case 0xB4: strcpy(chr,"LDY"); goto _zeropagex;
case 0xB5: strcpy(chr,"LDA"); goto _zeropagex;
case 0xD5: strcpy(chr,"CMP"); goto _zeropagex;
case 0xD6: strcpy(chr,"DEC"); goto _zeropagex;
case 0xF5: strcpy(chr,"SBC"); goto _zeropagex;
case 0xF6: strcpy(chr,"INC"); goto _zeropagex;
_zeropagex:
	zpIndex(tmp,X.X);
// ################################## Start of SP CODE ###########################
// Change width to %04X
	sprintf(str,"%s $%02X,X @ $%04X = #$%02X", chr,opcode[1],tmp,GetMem(tmp));
// ################################## End of SP CODE ###########################
	break;

//Absolute,Y
case 0x19: strcpy(chr,"ORA"); goto _absolutey;
case 0x39: strcpy(chr,"AND"); goto _absolutey;
case 0x59: strcpy(chr,"EOR"); goto _absolutey;
case 0x79: strcpy(chr,"ADC"); goto _absolutey;
case 0x99: strcpy(chr,"STA"); goto _absolutey;
case 0xB9: strcpy(chr,"LDA"); goto _absolutey;
case 0xBE: strcpy(chr,"LDX"); goto _absolutey;
case 0xD9: strcpy(chr,"CMP"); goto _absolutey;
case 0xF9: strcpy(chr,"SBC"); goto _absolutey;
_absolutey:
	absolute(tmp);
	tmp2=(tmp+X.Y);
	sprintf(str,"%s $%04X,Y @ $%04X = #$%02X", chr,tmp,tmp2,GetMem(tmp2));
	break;

//Absolute,X
case 0x1D: strcpy(chr,"ORA"); goto _absolutex;
case 0x1E: strcpy(chr,"ASL"); goto _absolutex;
case 0x3D: strcpy(chr,"AND"); goto _absolutex;
case 0x3E: strcpy(chr,"ROL"); goto _absolutex;
case 0x5D: strcpy(chr,"EOR"); goto _absolutex;
case 0x5E: strcpy(chr,"LSR"); goto _absolutex;
case 0x7D: strcpy(chr,"ADC"); goto _absolutex;
case 0x7E: strcpy(chr,"ROR"); goto _absolutex;
case 0x9D: strcpy(chr,"STA"); goto _absolutex;
case 0xBC: strcpy(chr,"LDY"); goto _absolutex;
case 0xBD: strcpy(chr,"LDA"); goto _absolutex;
case 0xDD: strcpy(chr,"CMP"); goto _absolutex;
case 0xDE: strcpy(chr,"DEC"); goto _absolutex;
case 0xFD: strcpy(chr,"SBC"); goto _absolutex;
case 0xFE: strcpy(chr,"INC"); goto _absolutex;
_absolutex:
	absolute(tmp);
	tmp2=(tmp+X.X);
	sprintf(str,"%s $%04X,X @ $%04X = #$%02X", chr,tmp,tmp2,GetMem(tmp2));
	break;

//jumps
case 0x20: strcpy(chr,"JSR"); goto _jump;
case 0x4C: strcpy(chr,"JMP"); goto _jump;
case 0x6C: absolute(tmp); sprintf(str,"JMP ($%04X) = $%04X", tmp,GetMem(tmp)|GetMem(tmp+1)<<8); break;
_jump:
	absolute(tmp);
	sprintf(str,"%s $%04X", chr,tmp);
	break;

//Zero Page,Y
case 0x96: strcpy(chr,"STX"); goto _zeropagey;
case 0xB6: strcpy(chr,"LDX"); goto _zeropagey;
_zeropagey:
	zpIndex(tmp,X.Y);
// ################################## Start of SP CODE ###########################
// Change width to %04X
	sprintf(str,"%s $%04X,Y @ $%04X = #$%02X", chr,opcode[1],tmp,GetMem(tmp));
// ################################## End of SP CODE ###########################
	break;

//UNDEFINED
default: strcpy(str,"ERROR"); break;
