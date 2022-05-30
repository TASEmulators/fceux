#!/usr/bin/env python3

# FCE Ultra - NES/Famicom Emulator
#
# Copyright notice for this file:
#  Copyright (C) 2002 Xodnizel, 2022 tsone
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

import argparse

# String values are C++ code, numbers are references to dictionary entry.
spec = {
0x00: # BRK
"""
            _PC++;
            PUSH(_PC>>8);
            PUSH(_PC);
            PUSH(_P|U_FLAG|B_FLAG);
	    _P|=I_FLAG;
	    _PI|=I_FLAG;
            _PC=RdMem(0xFFFE);
            _PC|=RdMem(0xFFFF)<<8;
""",
0x40: # RTI
"""
            _P=POP();
	    /* _PI=_P; This is probably incorrect, so it's commented out. */
	    _PI = _P;
            _PC=POP();
            _PC|=POP()<<8;
""",
0x60: # RTS
"""
            _PC=POP();
            _PC|=POP()<<8;
            _PC++;
""",
0x48: # PHA
"""
           PUSH(_A);
""",
0x08: # PHP
"""
           PUSH(_P|U_FLAG|B_FLAG);
""",
0x68: # PLA
"""
           _A=POP();
           X_ZN(_A);
""",
0x28: # PLP
"""
           _P=POP();
""",
0x4C: # JMP ABSOLUTE
"""
	   uint16 ptmp=_PC;
	   unsigned int npc;

	   npc=RdMem(ptmp);
	   ptmp++;
	   npc|=RdMem(ptmp)<<8;
	   _PC=npc;
""",
0x6C: # JMP INDIRECT
"""
	    uint32 tmp;
	    GetAB(tmp);
	    _PC=RdMem(tmp);
	    _PC|=RdMem( ((tmp+1)&0x00FF) | (tmp&0xFF00))<<8;
""",
0x20: # JSR
"""
	    uint8 npc;
	    npc=RdMem(_PC);
	    _PC++;
            PUSH(_PC>>8);
            PUSH(_PC);
            _PC=RdMem(_PC)<<8;
	    _PC|=npc;
""",
0xAA: # TAX
"""
           _X=_A;
           X_ZN(_A);
""",
0x8A: # TXA
"""
           _A=_X;
           X_ZN(_A);
""",
0xA8: # TAY
"""
           _Y=_A;
           X_ZN(_A);
""",
0x98: # TYA
"""
           _A=_Y;
           X_ZN(_A);
""",
0xBA: # TSX
"""
           _X=_S;
           X_ZN(_X);
""",
0x9A: # TXS
"""
           _S=_X;
""",
0xCA: # DEX
"""
           _X--;
           X_ZN(_X);
""",
0x88: # DEY
"""
           _Y--;
           X_ZN(_Y);
""",
0xE8: # INX
"""
           _X++;
           X_ZN(_X);
""",
0xC8: # INY
"""
           _Y++;
           X_ZN(_Y);
""",
0x18: # CLC
"""
           _P&=~C_FLAG;
""",
0xD8: # CLD
"""
           _P&=~D_FLAG;
""",
0x58: # CLI
"""
           _P&=~I_FLAG;
""",
0xB8: # CLV
"""
           _P&=~V_FLAG;
""",
0x38: # SEC
"""
           _P|=C_FLAG;
""",
0xF8: # SED
"""
           _P|=D_FLAG;
""",
0x78: # SEI
"""
           _P|=I_FLAG;
""",
0xEA: # NOP
"""
""",
0x0A:
"""
 RMW_A(ASL);
""",
0x06:
"""
 RMW_ZP(ASL);
""",
0x16:
"""
 RMW_ZPX(ASL);
""",
0x0E:
"""
 RMW_AB(ASL);
""",
0x1E:
"""
 RMW_ABX(ASL);
""",
0xC6:
"""
 RMW_ZP(DEC);
""",
0xD6:
"""
 RMW_ZPX(DEC);
""",
0xCE:
"""
 RMW_AB(DEC);
""",
0xDE:
"""
 RMW_ABX(DEC);
""",
0xE6:
"""
 RMW_ZP(INC);
""",
0xF6:
"""
 RMW_ZPX(INC);
""",
0xEE:
"""
 RMW_AB(INC);
""",
0xFE:
"""
 RMW_ABX(INC);
""",
0x4A:
"""
 RMW_A(LSR);
""",
0x46:
"""
 RMW_ZP(LSR);
""",
0x56:
"""
 RMW_ZPX(LSR);
""",
0x4E:
"""
 RMW_AB(LSR);
""",
0x5E:
"""
 RMW_ABX(LSR);
""",
0x2A:
"""
 RMW_A(ROL);
""",
0x26:
"""
 RMW_ZP(ROL);
""",
0x36:
"""
 RMW_ZPX(ROL);
""",
0x2E:
"""
 RMW_AB(ROL);
""",
0x3E:
"""
 RMW_ABX(ROL);
""",
0x6A:
"""
 RMW_A(ROR);
""",
0x66:
"""
 RMW_ZP(ROR);
""",
0x76:
"""
 RMW_ZPX(ROR);
""",
0x6E:
"""
 RMW_AB(ROR);
""",
0x7E:
"""
 RMW_ABX(ROR);
""",
0x69:
"""
 LD_IM(ADC);
""",
0x65:
"""
 LD_ZP(ADC);
""",
0x75:
"""
 LD_ZPX(ADC);
""",
0x6D:
"""
 LD_AB(ADC);
""",
0x7D:
"""
 LD_ABX(ADC);
""",
0x79:
"""
 LD_ABY(ADC);
""",
0x61:
"""
 LD_IX(ADC);
""",
0x71:
"""
 LD_IY(ADC);
""",
0x29:
"""
 LD_IM(AND);
""",
0x25:
"""
 LD_ZP(AND);
""",
0x35:
"""
 LD_ZPX(AND);
""",
0x2D:
"""
 LD_AB(AND);
""",
0x3D:
"""
 LD_ABX(AND);
""",
0x39:
"""
 LD_ABY(AND);
""",
0x21:
"""
 LD_IX(AND);
""",
0x31:
"""
 LD_IY(AND);
""",
0x24:
"""
 LD_ZP(BIT);
""",
0x2C:
"""
 LD_AB(BIT);
""",
0xC9:
"""
 LD_IM(CMP);
""",
0xC5:
"""
 LD_ZP(CMP);
""",
0xD5:
"""
 LD_ZPX(CMP);
""",
0xCD:
"""
 LD_AB(CMP);
""",
0xDD:
"""
 LD_ABX(CMP);
""",
0xD9:
"""
 LD_ABY(CMP);
""",
0xC1:
"""
 LD_IX(CMP);
""",
0xD1:
"""
 LD_IY(CMP);
""",
0xE0:
"""
 LD_IM(CPX);
""",
0xE4:
"""
 LD_ZP(CPX);
""",
0xEC:
"""
 LD_AB(CPX);
""",
0xC0:
"""
 LD_IM(CPY);
""",
0xC4:
"""
 LD_ZP(CPY);
""",
0xCC:
"""
 LD_AB(CPY);
""",
0x49:
"""
 LD_IM(EOR);
""",
0x45:
"""
 LD_ZP(EOR);
""",
0x55:
"""
 LD_ZPX(EOR);
""",
0x4D:
"""
 LD_AB(EOR);
""",
0x5D:
"""
 LD_ABX(EOR);
""",
0x59:
"""
 LD_ABY(EOR);
""",
0x41:
"""
 LD_IX(EOR);
""",
0x51:
"""
 LD_IY(EOR);
""",
0xA9:
"""
 LD_IM(LDA);
""",
0xA5:
"""
 LD_ZP(LDA);
""",
0xB5:
"""
 LD_ZPX(LDA);
""",
0xAD:
"""
 LD_AB(LDA);
""",
0xBD:
"""
 LD_ABX(LDA);
""",
0xB9:
"""
 LD_ABY(LDA);
""",
0xA1:
"""
 LD_IX(LDA);
""",
0xB1:
"""
 LD_IY(LDA);
""",
0xA2:
"""
 LD_IM(LDX);
""",
0xA6:
"""
 LD_ZP(LDX);
""",
0xB6:
"""
 LD_ZPY(LDX);
""",
0xAE:
"""
 LD_AB(LDX);
""",
0xBE:
"""
 LD_ABY(LDX);
""",
0xA0:
"""
 LD_IM(LDY);
""",
0xA4:
"""
 LD_ZP(LDY);
""",
0xB4:
"""
 LD_ZPX(LDY);
""",
0xAC:
"""
 LD_AB(LDY);
""",
0xBC:
"""
 LD_ABX(LDY);
""",
0x09:
"""
 LD_IM(ORA);
""",
0x05:
"""
 LD_ZP(ORA);
""",
0x15:
"""
 LD_ZPX(ORA);
""",
0x0D:
"""
 LD_AB(ORA);
""",
0x1D:
"""
 LD_ABX(ORA);
""",
0x19:
"""
 LD_ABY(ORA);
""",
0x01:
"""
 LD_IX(ORA);
""",
0x11:
"""
 LD_IY(ORA);
""",
0xEB:
"""
  /* (undocumented) */
""",
0xE9:
"""
 LD_IM(SBC);
""",
0xE5:
"""
 LD_ZP(SBC);
""",
0xF5:
"""
 LD_ZPX(SBC);
""",
0xED:
"""
 LD_AB(SBC);
""",
0xFD:
"""
 LD_ABX(SBC);
""",
0xF9:
"""
 LD_ABY(SBC);
""",
0xE1:
"""
 LD_IX(SBC);
""",
0xF1:
"""
 LD_IY(SBC);
""",
0x85:
"""
 ST_ZP(_A);
""",
0x95:
"""
 ST_ZPX(_A);
""",
0x8D:
"""
 ST_AB(_A);
""",
0x9D:
"""
 ST_ABX(_A);
""",
0x99:
"""
 ST_ABY(_A);
""",
0x81:
"""
 ST_IX(_A);
""",
0x91:
"""
 ST_IY(_A);
""",
0x86:
"""
 ST_ZP(_X);
""",
0x96:
"""
 ST_ZPY(_X);
""",
0x8E:
"""
 ST_AB(_X);
""",
0x84:
"""
 ST_ZP(_Y);
""",
0x94:
"""
 ST_ZPX(_Y);
""",
0x8C:
"""
 ST_AB(_Y);
""",
0x90: # BCC
"""
 JR(!(_P&C_FLAG));
""",
0xB0: # BCS
"""
 JR(_P&C_FLAG);
""",
0xF0: # BEQ
"""
 JR(_P&Z_FLAG);
""",
0xD0: # BNE
"""
 JR(!(_P&Z_FLAG));
""",
0x30: # BMI
"""
 JR(_P&N_FLAG);
""",
0x10: # BPL
"""
 JR(!(_P&N_FLAG));
""",
0x50: # BVC
"""
 JR(!(_P&V_FLAG));
""",
0x70: # BVS
"""
 JR(_P&V_FLAG);
""",

# Here comes the undocumented instructions block.  Note that this implementation
# may be "wrong".  If so, please tell me.

0x2B: # AAC
"""
 LD_IM(AND;_P&=~C_FLAG;_P|=_A>>7);
""",
0x0B: 0x2B,
0x87: # AAX
"""
 ST_ZP(_A&_X);
""",
0x97:
"""
 ST_ZPY(_A&_X);
""",
0x8F:
"""
 ST_AB(_A&_X);
""",
0x83:
"""
 ST_IX(_A&_X);
""",
0x6B: # ARR
"""
	     uint8 arrtmp;
	     LD_IM(AND;_P&=~V_FLAG;_P|=(_A^(_A>>1))&0x40;arrtmp=_A>>7;_A>>=1;_A|=(_P&C_FLAG)<<7;_P&=~C_FLAG;_P|=arrtmp;X_ZN(_A));
""",
0x4B: # ASR
"""
    LD_IM(AND;LSRA);
""",
# ATX(OAL) Is this(OR with $EE) correct? Blargg did some test
# and found the constant to be OR with is $FF for NES
0xAB: # ATX
"""
 LD_IM(_A|=0xFF;AND;_X=_A);
""",
0xCB: # AXS
"""
 LD_IM(AXS);
""",
0xC7: # DCP
"""
 RMW_ZP(DEC;CMP);
""",
0xD7:
"""
 RMW_ZPX(DEC;CMP);
""",
0xCF:
"""
 RMW_AB(DEC;CMP);
""",
0xDF:
"""
 RMW_ABX(DEC;CMP);
""",
0xDB:
"""
 RMW_ABY(DEC;CMP);
""",
0xC3:
"""
 RMW_IX(DEC;CMP);
""",
0xD3:
"""
 RMW_IY(DEC;CMP);
""",
0xE7: # ISB
"""
 RMW_ZP(INC;SBC);
""",
0xF7:
"""
 RMW_ZPX(INC;SBC);
""",
0xEF:
"""
 RMW_AB(INC;SBC);
""",
0xFF:
"""
 RMW_ABX(INC;SBC);
""",
0xFB:
"""
 RMW_ABY(INC;SBC);
""",
0xE3:
"""
 RMW_IX(INC;SBC);
""",
0xF3:
"""
 RMW_IY(INC;SBC);
""",
0x04: # DOP
"""
 _PC++;
""",
0x14: 0x04,
0x34: 0x04,
0x44: 0x04,
0x54: 0x04,
0x64: 0x04,
0x74: 0x04,
0x80: 0x04,
0x82: 0x04,
0x89: 0x04,
0xC2: 0x04,
0xD4: 0x04,
0xE2: 0x04,
0xF4: 0x04,
0x02: # KIL
"""
       ADDCYC(0xFF);
       _jammed=1;
       _PC--;
""",
0x12: 0x02,
0x22: 0x02,
0x32: 0x02,
0x42: 0x02,
0x52: 0x02,
0x62: 0x02,
0x72: 0x02,
0x92: 0x02,
0xB2: 0x02,
0xD2: 0x02,
0xF2: 0x02,
0xBB: # LAR
"""
 RMW_ABY(_S&=x;_A=_X=_S;X_ZN(_X));
""",
0xA7: # LAX
"""
 LD_ZP(LDA;LDX);
""",
0xB7:
"""
 LD_ZPY(LDA;LDX);
""",
0xAF:
"""
 LD_AB(LDA;LDX);
""",
0xBF:
"""
 LD_ABY(LDA;LDX);
""",
0xA3:
"""
 LD_IX(LDA;LDX);
""",
0xB3:
"""
 LD_IY(LDA;LDX);
""",
# More NOP
0x1A: 0xEA,
0x3A: 0xEA,
0x5A: 0xEA,
0x7A: 0xEA,
0xDA: 0xEA,
0xFA: 0xEA,
0x27: # RLA
"""
 RMW_ZP(ROL;AND);
""",
0x37:
"""
 RMW_ZPX(ROL;AND);
""",
0x2F:
"""
 RMW_AB(ROL;AND);
""",
0x3F:
"""
 RMW_ABX(ROL;AND);
""",
0x3B:
"""
 RMW_ABY(ROL;AND);
""",
0x23:
"""
 RMW_IX(ROL;AND);
""",
0x33:
"""
 RMW_IY(ROL;AND);
""", # RRA
0x67:
"""
 RMW_ZP(ROR;ADC);
""",
0x77:
"""
 RMW_ZPX(ROR;ADC);
""",
0x6F:
"""
 RMW_AB(ROR;ADC);
""",
0x7F:
"""
 RMW_ABX(ROR;ADC);
""",
0x7B:
"""
 RMW_ABY(ROR;ADC);
""",
0x63:
"""
 RMW_IX(ROR;ADC);
""",
0x73:
"""
 RMW_IY(ROR;ADC);
""",
0x07: # SLO
"""
 RMW_ZP(ASL;ORA);
""",
0x17:
"""
 RMW_ZPX(ASL;ORA);
""",
0x0F:
"""
 RMW_AB(ASL;ORA);
""",
0x1F:
"""
 RMW_ABX(ASL;ORA);
""",
0x1B:
"""
 RMW_ABY(ASL;ORA);
""",
0x03:
"""
 RMW_IX(ASL;ORA);
""",
0x13:
"""
 RMW_IY(ASL;ORA);
""",
0x47: # SRE
"""
 RMW_ZP(LSR;EOR);
""",
0x57:
"""
 RMW_ZPX(LSR;EOR);
""",
0x4F:
"""
 RMW_AB(LSR;EOR);
""",
0x5F:
"""
 RMW_ABX(LSR;EOR);
""",
0x5B:
"""
 RMW_ABY(LSR;EOR);
""",
0x43:
"""
 RMW_IX(LSR;EOR);
""",
0x53:
"""
 RMW_IY(LSR;EOR);
""",
# AXA - SHA
0x93: # AXA
"""
 ST_IY(_A&_X&(((A-_Y)>>8)+1));
""",
0x9F:
"""
 ST_ABY(_A&_X&(((A-_Y)>>8)+1));
""",
# Can't reuse existing ST_ABI macro here, due to addressing weirdness.
0x9C: # SYA
"""
unsigned int A;
GetABIWR(A,_X);
A = ((_Y&((A>>8)+1)) << 8) | (A & 0xff);
WrMem(A,A>>8);
""",
# Can't reuse existing ST_ABI macro here, due to addressing weirdness.
0x9E: # SXA
"""
unsigned int A;
GetABIWR(A,_Y);
A = ((_X&((A>>8)+1)) << 8) | (A & 0xff);
WrMem(A,A>>8);
""",
0x9B: # XAS
"""
_S=_A&_X;
ST_ABY(_S& (((A-_Y)>>8)+1) );
""",
0x0C: # TOP
"""
LD_AB(;);
""",
0x1C:
"""
LD_ABX(;);
""",
0x3C: 0x1C,
0x5C: 0x1C,
0x7C: 0x1C,
0xDC: 0x1C,
0xFC: 0x1C,
# XAA - BIG QUESTION MARK HERE
0x8B: # XAA
"""
_A|=0xEE; _A&=_X; LD_IM(AND);
""",
}

def generate_func_table():
  ops_with_source = sorted(k for k,v in spec.items() if isinstance(v, str))

  for op in ops_with_source:
    print("static void op%02X(void) {%s}" % (op, spec[op]))

  print("static void (*opstab[256])(void) = {")
  for op in range(256):
    end = "" if (op + 1) % 16 else "\n"
    if op in spec:
      if op in ops_with_source:
        print("op%02X," % (op), end=end)
      else:
        print("op%02X," % (spec[op]), end=end)
    else:
      print("opEA,", end=end) # NOP
  print("};")

def generate_switch():
  ops_with_source = sorted(k for k,v in spec.items() if isinstance(v, str))

  for op in ops_with_source:
    duplicate_ops = [k for k,v in spec.items() if v == op]
    for dupe in duplicate_ops:
      print("case 0x%02X:" % (dupe))
    print("case 0x%02X:\n{%sbreak;\n}" % (op, spec[op]))

def main():
  parser = argparse.ArgumentParser(description="generate fceux c++ source code for ricoh 2a03 opcode emulation")
  parser.add_argument("-f", "--func", action='store_true', help="generate function table instead of switch-case")
  args = parser.parse_args()
  if args.func:
    generate_func_table()
  else:
    generate_switch()

if __name__ == "__main__":
  main()
