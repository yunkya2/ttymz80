/* Z80 emulator header file                              */
/*  by AKIKAWA, Hisashi 2015-2023                        */
/* This software is released under 2-clause BSD license. */

#ifndef Z80_H_INCLUDED
#define Z80_H_INCLUDED

#include <stdint.h>

typedef uint8_t byte;
typedef uint16_t word;

typedef struct {
  byte a, f, b, c, d, e, h, l;
  byte alt_a, alt_f, alt_b, alt_c, alt_d, alt_e, alt_h, alt_l;
  byte ixh, ixl, iyh, iyl;
  byte i;
  byte r;
  word sp, pc, wz;
  int iff1, iff2;
  int im;
  int ei;
  int m1;
  int halt;
  int cycles;
  int mcycle;
  int busreq;
  int nmi;
  int intr;
  int intmode2;
  byte op1;
  byte op2;
  byte op3;
} z80;

void z80_reset(z80 *pcpu);
void z80_main(z80 *pcpu);
void z80_nmi(z80 *pcpu);
int z80_intack(z80 *pcpu);
void z80_int(z80 *pcpu, byte vector);
byte z80_read(word address);
void z80_write(word address, byte data);
byte z80_in(word address);
void z80_out(word address, byte data);
int z80_dasm(char *disasm, word address);
int z80_asm(char *input, byte *data, word address);

#endif
