
if (!strlen(astr)) {
	//Implied instructions
		 if (!strcmp(ins,"BRK")) opcode[0] = 0x00;
	else if (!strcmp(ins,"PHP")) opcode[0] = 0x08;
	else if (!strcmp(ins,"ASL")) opcode[0] = 0x0A;
	else if (!strcmp(ins,"CLC")) opcode[0] = 0x18;
	else if (!strcmp(ins,"PLP")) opcode[0] = 0x28;
	else if (!strcmp(ins,"ROL")) opcode[0] = 0x2A;
	else if (!strcmp(ins,"SEC")) opcode[0] = 0x38;
	else if (!strcmp(ins,"RTI")) opcode[0] = 0x40;
	else if (!strcmp(ins,"PHA")) opcode[0] = 0x48;
	else if (!strcmp(ins,"LSR")) opcode[0] = 0x4A;
	else if (!strcmp(ins,"CLI")) opcode[0] = 0x58;
	else if (!strcmp(ins,"RTS")) opcode[0] = 0x60;
	else if (!strcmp(ins,"PLA")) opcode[0] = 0x68;
	else if (!strcmp(ins,"ROR")) opcode[0] = 0x6A;
	else if (!strcmp(ins,"SEI")) opcode[0] = 0x78;
	else if (!strcmp(ins,"DEY")) opcode[0] = 0x88;
	else if (!strcmp(ins,"TXA")) opcode[0] = 0x8A;
	else if (!strcmp(ins,"TYA")) opcode[0] = 0x98;
	else if (!strcmp(ins,"TXS")) opcode[0] = 0x9A;
	else if (!strcmp(ins,"TAY")) opcode[0] = 0xA8;
	else if (!strcmp(ins,"TAX")) opcode[0] = 0xAA;
	else if (!strcmp(ins,"CLV")) opcode[0] = 0xB8;
	else if (!strcmp(ins,"TSX")) opcode[0] = 0xBA;
	else if (!strcmp(ins,"INY")) opcode[0] = 0xC8;
	else if (!strcmp(ins,"DEX")) opcode[0] = 0xCA;
	else if (!strcmp(ins,"CLD")) opcode[0] = 0xD8;
	else if (!strcmp(ins,"INX")) opcode[0] = 0xE8;
	else if (!strcmp(ins,"NOP")) opcode[0] = 0xEA;
	else if (!strcmp(ins,"SED")) opcode[0] = 0xF8;
	else return 1;
}
else {
	//Instructions with Operands
	     if (!strcmp(ins,"ORA")) opcode[0] = 0x01;
	else if (!strcmp(ins,"ASL")) opcode[0] = 0x06;
	else if (!strcmp(ins,"BPL")) opcode[0] = 0x10;
	else if (!strcmp(ins,"JSR")) opcode[0] = 0x20;
	else if (!strcmp(ins,"AND")) opcode[0] = 0x21;
	else if (!strcmp(ins,"BIT")) opcode[0] = 0x24;
	else if (!strcmp(ins,"ROL")) opcode[0] = 0x26;
	else if (!strcmp(ins,"BMI")) opcode[0] = 0x30;
	else if (!strcmp(ins,"EOR")) opcode[0] = 0x41;
	else if (!strcmp(ins,"LSR")) opcode[0] = 0x46;
	else if (!strcmp(ins,"JMP")) opcode[0] = 0x4C;
	else if (!strcmp(ins,"BVC")) opcode[0] = 0x50;
	else if (!strcmp(ins,"ADC")) opcode[0] = 0x61;
	else if (!strcmp(ins,"ROR")) opcode[0] = 0x66;
	else if (!strcmp(ins,"BVS")) opcode[0] = 0x70;
	else if (!strcmp(ins,"STA")) opcode[0] = 0x81;
	else if (!strcmp(ins,"STY")) opcode[0] = 0x84;
	else if (!strcmp(ins,"STX")) opcode[0] = 0x86;
	else if (!strcmp(ins,"BCC")) opcode[0] = 0x90;
	else if (!strcmp(ins,"LDY")) opcode[0] = 0xA0;
	else if (!strcmp(ins,"LDA")) opcode[0] = 0xA1;
	else if (!strcmp(ins,"LDX")) opcode[0] = 0xA2;
	else if (!strcmp(ins,"BCS")) opcode[0] = 0xB0;
	else if (!strcmp(ins,"CPY")) opcode[0] = 0xC0;
	else if (!strcmp(ins,"CMP")) opcode[0] = 0xC1;
	else if (!strcmp(ins,"DEC")) opcode[0] = 0xC6;
	else if (!strcmp(ins,"BNE")) opcode[0] = 0xD0;
	else if (!strcmp(ins,"CPX")) opcode[0] = 0xE0;
	else if (!strcmp(ins,"SBC")) opcode[0] = 0xE1;
	else if (!strcmp(ins,"INC")) opcode[0] = 0xE6;
	else if (!strcmp(ins,"BEQ")) opcode[0] = 0xF0;
	else return 1;

	{
		//Parse Operands
		// It's not the sexiest thing ever, but it works well enough!

		//TODO:
		// Add branches.
		// Fix certain instructions. (Setting bits is not 100% perfect.)
		// Fix instruction/operand matching. (Instructions like "jmp ($94),Y" are no good!)
		// Optimizations?
		int tmpint;
		char tmpchr,tmpstr[20];

		if (sscanf(astr,"#$%2X%c",&tmpint,&tmpchr) == 1) { //#Immediate
			switch (opcode[0]) {
				case 0x20: case 0x4C: //Jumps
				case 0x10: case 0x30: case 0x50: case 0x70: //Branches
				case 0x90: case 0xB0: case 0xD0: case 0xF0:
				case 0x06: case 0x24: case 0x26: case 0x46: //Other instructions incapable of #Immediate
				case 0x66: case 0x81: case 0x84: case 0x86:
				case 0xC6: case 0xE6:
					return 1;
				default:
					//cheap hack for certain instructions
					switch (opcode[0]) {
						case 0xA0: case 0xA2: case 0xC0: case 0xE0:
							break;
						default:
							opcode[0] |= 0x08;
							break;
					}
					opcode[1] = tmpint;
					break;
			}
		}
		else if (sscanf(astr,"$%4X%c",&tmpint,&tmpchr) == 1) { //Absolute, Zero Page, Branch, or Jump
			switch (opcode[0]) {
				case 0x20: case 0x4C: //Jumps
					opcode[1] = (tmpint & 0xFF);
					opcode[2] = (tmpint >> 8);
					break;
				case 0x10: case 0x30: case 0x50: case 0x70: //Branches
				case 0x90: case 0xB0: case 0xD0: case 0xF0:
					tmpint -= (addr+2);
					if ((tmpint < -128) || (tmpint > 127)) return 1;
					opcode[1] = (tmpint & 0xFF);
					break;
					//return 1; //FIX ME
				default:
					if (tmpint > 0xFF) { //Absolute
						opcode[0] |= 0x0C;
						opcode[1] = (tmpint & 0xFF);
						opcode[2] = (tmpint >> 8);
					}
					else { //Zero Page
						opcode[0] |= 0x04;
						opcode[1] = (tmpint & 0xFF);
					}
					break;
			}
		}
		else if (sscanf(astr,"$%4X%s",&tmpint,tmpstr) == 2) { //Absolute,X, Zero Page,X, Absolute,Y or Zero Page,Y
			if (!strcmp(tmpstr,",X")) { //Absolute,X or Zero Page,X
				switch (opcode[0]) {
					case 0x20: case 0x4C: //Jumps
					case 0x10: case 0x30: case 0x50: case 0x70: //Branches
					case 0x90: case 0xB0: case 0xD0: case 0xF0:
					case 0x24: case 0x86: case 0xA2: case 0xC0: //Other instructions incapable of Absolute,X or Zero Page,X
					case 0xE0:
						return 1;
					default:
						if (tmpint > 0xFF) { //Absolute
							if (opcode[0] == 0x84) return 1; //No STY Absolute,X!
							opcode[0] |= 0x1C;
							opcode[1] = (tmpint & 0xFF);
							opcode[2] = (tmpint >> 8);
						}
						else { //Zero Page
							opcode[0] |= 0x14;
							opcode[1] = (tmpint & 0xFF);
						}
						break;
				}
			}
			else if (!strcmp(tmpstr,",Y")) { //Absolute,Y or Zero Page,Y
				switch (opcode[0]) {
					case 0x20: case 0x4C: //Jumps
					case 0x10: case 0x30: case 0x50: case 0x70: //Branches
					case 0x90: case 0xB0: case 0xD0: case 0xF0:
					case 0x06: case 0x24: case 0x26: case 0x46: //Other instructions incapable of Absolute,Y or Zero Page,Y
					case 0x66: case 0x84: case 0x86: case 0xA0:
					case 0xC0: case 0xC6: case 0xE0: case 0xE6:
						return 1;
					case 0xA2: //cheap hack for LDX
						opcode[0] |= 0x04;
					default:
						if (tmpint > 0xFF) { //Absolute
							if (opcode[0] == 0x86) return 1; //No STX Absolute,Y!
							opcode[0] |= 0x18;
							opcode[1] = (tmpint & 0xFF);
							opcode[2] = (tmpint >> 8);
						}
						else { //Zero Page
							if ((opcode[0] != 0x86) || (opcode[0] != 0xA2)) return 1; //only STX and LDX Absolute,Y!
							opcode[0] |= 0x10;
							opcode[1] = (tmpint & 0xFF);
						}
						break;
				}
			}
			else return 1;
		}
		else if (sscanf(astr,"($%4X%s",&tmpint,tmpstr) == 2) { //Jump (Indirect), (Indirect,X) or (Indirect),Y
			switch (opcode[0]) {
				case 0x20: //Jumps
				case 0x10: case 0x30: case 0x50: case 0x70: //Branches
				case 0x90: case 0xB0: case 0xD0: case 0xF0:
				case 0x06: case 0x24: case 0x26: case 0x46: //Other instructions incapable of Jump (Indirect), (Indirect,X) or (Indirect),Y
				case 0x66: case 0x84: case 0x86: case 0xA0:
				case 0xA2: case 0xC0: case 0xC6: case 0xE0:
				case 0xE6:
					return 1;
				default:
					if ((!strcmp(tmpstr,")")) && (opcode[0] == 0x4C)) { //Jump (Indirect)
						opcode[0] = 0x6C;
						opcode[1] = (tmpint & 0xFF);
						opcode[2] = (tmpint >> 8);
					}
					else if ((!strcmp(tmpstr,",X)")) && (tmpint <= 0xFF) && (opcode[0] != 0x4C)) { //(Indirect,X)
						opcode[1] = (tmpint & 0xFF);
					}
					else if ((!strcmp(tmpstr,"),Y")) && (tmpint <= 0xFF) && (opcode[0] != 0x4C)) { //(Indirect),Y
						opcode[0] |= 0x10;
						opcode[1] = (tmpint & 0xFF);
					}
					else return 1;
					break;
			}
		}
		else return 1;
	}
}
