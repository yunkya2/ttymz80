/*
 * MZ-80K/C emulator on terminal
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include "z80.h"

/****************************************************************************/

char *mz80disp[256] = {
  "　", "Ａ", "Ｂ", "Ｃ", "Ｄ", "Ｅ", "Ｆ", "Ｇ",   /* 00 */
  "Ｈ", "Ｉ", "Ｊ", "Ｋ", "Ｌ", "Ｍ", "Ｎ", "Ｏ",
  "Ｐ", "Ｑ", "Ｒ", "Ｓ", "Ｔ", "Ｕ", "Ｖ", "Ｗ",   /* 10 */
  "Ｘ", "Ｙ", "Ｚ", "┼", "└", "┘", "├", "┴",
  "０", "１", "２", "３", "４", "５", "６", "７",   /* 20 */
  "８", "９", "－", "＝", "；", "／", "．", "，",
  NULL, NULL, NULL, NULL, NULL, NULL, "▔", NULL, /* 30 */
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 40 */
  NULL, "？", NULL, NULL, NULL, NULL, NULL, "：",
  "↑", "＜", "［", "♥", "］", "＠", "◤", "＞",     /* 50 */
  NULL, "＼", NULL, "▚", "┌", "┐", "┤", "┬",
  "π", "！", "\" ", "＃", "＄", "％", "＆", "' ",    /* 60 */
  "（", "）", "＋", "＊", "▞", NULL, "╯", "╰",
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 70 */
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 80 */
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 90 */
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* a0 */
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* b0 */
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  "⭳", "⬇", "⬆", "➡", "⬅", NULL, NULL, NULL, /* c0 */
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* d0 */
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* e0 */
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  "　", "▘", "▝", "▀", "▖", "▌", "▞", "▛",    /* f0 */
  "▗", "▚", "▐", "▜", "▄", "▙", "▟", "█",
};

char *mz80keytbl[][10][8] = {
  {   /* non-shift keymap */
    { NULL, NULL, "-", "9", "7", "5", "3", "1" },
    { NULL, NULL, NULL, "0", "8", "6", "4", "2" },
    { NULL, NULL, "=", "o", "u", "t", "e", "q" },
    { NULL, NULL, NULL, "p", "i", "y", "r", "w" },
    { NULL, NULL, NULL, "l", "j", "g", "d", "a" },
    { NULL, NULL, NULL, ";", "k", "h", "f", "s" },
    { NULL, NULL, NULL, ".", "m", "b", "c", "z" },
    { NULL, NULL, NULL, NULL, ",", "n", "v", "x" },
    { NULL, NULL, NULL, "\n", "\x1b[C", NULL, "\x7f", NULL },
    { NULL, NULL, NULL, NULL, NULL, "\x1b[B", " ", NULL },
  },
  {   /* shift keymap */
    { NULL, NULL, "_", "(", "&", "%", "#", "!" },
    { NULL, NULL, NULL, ")", "*", "^", "$", "@" },
    { NULL, NULL, "+", "O", "U", "T", "E", "Q" },
    { NULL, NULL, NULL, "P", "I", "Y", "R", "W" },
    { NULL, NULL, NULL, "L", "J", "G", "D", "A" },
    { NULL, NULL, NULL, ":", "K", "H", "F", "S" },
    { NULL, NULL, NULL, "<", "M", "B", "C" "Z" },
    { NULL, NULL, NULL, NULL, ">", "N", "V", "X" },
    { NULL, NULL, NULL, NULL, "\x1b[D", NULL, NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL, "\x1b[A", NULL, NULL },
  },
};

/****************************************************************************/

int verbose = 0;

/* CPU context */
z80 cpu;

/* Memory */
byte mz80rom[0x1000];
byte mz80ram[0xc000];
byte mz80text[0x400];

/* Keyboard support*/
int mz80key_i8255pa;
int mz80key_select;
int mz80key_bit;
enum keystate {
  KEY_NONE,
  KEY_PRESS,
  KEY_SHIFTPRESS,
  KEY_SHIFTPRESS1,
  KEY_SHIFTPRESS2,
};
enum keystate mz80key_state = KEY_NONE;
enum keystate mz80key_nextstate = KEY_NONE;

/****************************************************************************/

static char *disasm(word address, char *device)
{
  static int reent = 0;
  static int count = 0;
  char line[100];
  char *res = NULL;

  if (!verbose || reent)
    return NULL;

  if (cpu.m1) {
    reent = 1;
    count = z80_dasm(line, address);
    reent = 0;
    printf("  %04x %d %s\n", address, count, line);
  } else {
    if (count <= 0)
      res = device;
  }
  count--;

  return res;
}

byte z80_read(word address)
{
  byte data = 0;
  char *device = NULL;


  if (address < 0x1000) {

    /* ROM */
    data = mz80rom[address];
    device = disasm(address, "ROM ");

  } else if (address < 0xd000) {

    /* RAM */
    data = mz80ram[address - 0x1000];
    device = disasm(address, "RAM ");

  } else if (address >= 0xd000 && address < 0xe000) {

    /* Text VRAM */
    int x, y;
    int offset = address & 0x3ff;
    x = offset % 40;
    y = offset / 40;
    data = mz80text[offset];
    if (verbose)
      printf("VRAM R (%2d,%2d) %02x\n", x, y, data);

  } else if (address >= 0xe000 && address <= 0xe003) {

    /* 8255 */
    data = 0xff;
    if (address == 0xe001) {
      switch (mz80key_state) {
        case KEY_NONE:
          break;

        case KEY_PRESS:
          if (mz80key_select == mz80key_i8255pa)
            data = ~mz80key_bit;
          mz80key_nextstate = KEY_NONE;
          break;

        case KEY_SHIFTPRESS:
          if (mz80key_i8255pa == 8)
            data = ~0x20;
          mz80key_nextstate = KEY_SHIFTPRESS1;
          break;
        case KEY_SHIFTPRESS1:
          if (mz80key_i8255pa == 8)
            data = ~0x20;
          if (mz80key_select == mz80key_i8255pa)
            data = ~mz80key_bit;
          mz80key_nextstate = KEY_SHIFTPRESS2;
          break;
        case KEY_SHIFTPRESS2:
          mz80key_nextstate = KEY_NONE;
          break;
      }
      if (mz80key_i8255pa == 0) {
        mz80key_state = mz80key_nextstate;
      }
    } else if (address == 0xe002) {
      static int vblank;      /* dummy VBLANK */
      data = vblank ? 0x80 : 0x00;
      vblank = 1 - vblank;
    }
    if (verbose)
      device = "8255";

  } else if (address >= 0xe004 && address <= 0xe007) {

    /* 8253 */
    data = 0;
    static int hilo;
    static int count2;        /* dummy Count2 */

    switch (address) {
      case 0xe006:
        data = (hilo % 2) ? (count2 & 0xff) : (count2 >> 8);
        hilo = 1 - hilo;
        if (hilo == 0)
          count2--;
        break;
    }
    if (verbose)
      device = "8253";

  } else if (address == 0xe008) {

    /* GPIO */
    static int stat;          /* dummy music timer */
    data = stat ? 0xff : 0xfe;
    stat = 1 - stat;
    if (verbose)
      device = "GPIO";

  } else {

    /* Unknown device*/
    if (verbose)
      device = "????";

  }

  if (device)
    printf("%s R %04x %02x\n", device, address, data);

  return data;
}

/****************************************************************************/

void z80_write(word address, byte data)
{
  char *device = NULL;
  
  if (address >= 0x1000 && address < 0xd000) {

    /* RAM */
    mz80ram[address - 0x1000] = data;
    if (verbose)
      device = "RAM ";

  } else if (address >= 0xd000 && address < 0xe000) {

    /* Text VRAM */
    int x, y;
    int offset = address & 0x3ff;
    char *p;
    x = offset % 40;
    y = offset / 40;
    mz80text[offset] = data;
    p = mz80disp[data];
    p = p ? p : "";
    if (!verbose) {
      printf("\x1b[%d;%dH%s", y + 1, (x * 2) + 1, p);
      fflush(stdout);
    } else {
      printf("VRAM W (%2d,%2d) %02x %s\n", x, y, data, p);
    }

  } else if (address >= 0xe000 && address <= 0xe003) {

    /* 8255 */
    if (address == 0xe000) {
      mz80key_i8255pa = data & 0x0f;
    } else {
      if (verbose)
        printf("8255 write: %04x %02x\n", address, data);
    }
    if (verbose)
      device = "8255";

  } else if (address >= 0xe004 && address <= 0xe007) {

    /* 8253 */
    if (verbose)
      device = "8253";

  } else if (address == 0xe008) {

    /* GPIO */
    if (verbose)
      device = "GPIO";

  } else {

    /* Unknown device*/
    if (verbose)
      device = "????";

  }

  if (device)
    printf("%s W %04x %02x\n", device, address, data);
}

/****************************************************************************/

byte z80_in(word address)
{
  printf("z80_in: %04x\n", address);
  return 0;
}

/****************************************************************************/

void z80_out(word address, byte data)
{
  printf("z80_out: %04x %02x\n", address, data);
}

/****************************************************************************/

static int mz80keyscan(void)
{
  int i, j, k;
  int ch;
  static char key[10];
  static char *p = NULL;

  if (mz80key_state != KEY_NONE)
    return 0;

  ch = getchar();
  if (ch < 0) {
    if (p == NULL)
      return 0;
  } else {
    if (ch == '\x11') {         /* ^Q : exit */
      return -1;
    } else if (ch == '\x01') {  /* ^A : switch verbose */
      verbose= 1 - verbose;
      return 0;
    } else if (ch == '\x1b') {  /* ESC sequence start */
      p = key;
      *p++ = ch;
      *p = '\0';
      return 0;
    } else {
      if (p == NULL) {
        key[0] = ch;
        key[1] = '\0';
      } else {
        *p++ = ch;
        *p = '\0';
        if (!(ch >= 'A' && ch <= 'Z'))
          return 0;
      }
    }
  }
  p = NULL;

  for (i = 0; i < 2; i++) {
    for (j = 0; j < 10; j++) {
      for (k = 0; k < 8; k++) {
        if (mz80keytbl[i][j][k] &&
            (strcmp(mz80keytbl[i][j][k], key) == 0)) {
          mz80key_select = j;
          mz80key_bit = 1 << (7 - k);
          mz80key_nextstate = i ? KEY_SHIFTPRESS : KEY_PRESS;
          return 0;
        }
      }
    }
  }
  return 0;
}

static void mz80main(void)
{
  struct termios oldt, newt;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  z80_reset(&cpu);

  while (mz80keyscan() >= 0) {
    if (cpu.pc == 0x003e) {
      printf("\a");     /* bell */
      fflush(stdout);
    }

    z80_main(&cpu);
  }

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);
}

__asm__ (
  ".global mz_newmon\n"
  "mz_newmon:\n"
  ".incbin \"mz_newmon/NEWMON.ROM\""
);

int main(int argc, char **argv)
{
  FILE *fp;
  int i;
  int help = 0;

  extern char mz_newmon;
  memcpy(mz80rom, &mz_newmon, sizeof(mz80rom));

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-r") == 0) {
      if (i + 1 < argc) {
        fp = fopen(argv[i + 1], "rb");
        if (fp == NULL) {
          printf("ROM file not found\n");
          help = 1;
          break;
        }
        fread(mz80rom, sizeof(mz80rom), 1, fp);
        fclose(fp);
        i++;
      } else {
        help = 1;
      }
    } else {
      char header[0x80];
      int addr;
      int jump;
      char *p;

      fp = fopen(argv[i], "rb");
      if (fp == NULL) {
        help = 1;
        break;
      }
      fread(header, sizeof(header), 1, fp);
      addr = header[0x14] + (header[0x15] << 8);
      jump = header[0x16] + (header[0x17] << 8);
      printf("Loading: \"");
      for (p = &header[1]; *p >= ' '; p++)
        putchar(*p);
      printf("\" $%04x- entry $%04x\n", addr, jump);
      fread(&mz80ram[addr - 0x1000], 32768, 1, fp);
      fclose(fp);
    }
  }

  if (help) {
    printf("Usage: ttymz80 [-r <ROM image>] [<mzt/mzf file>]\n");
    return 1;
  }

  mz80main();
}
