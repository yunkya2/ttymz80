/*
 * MZ-80K/C emulator on terminal
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <termios.h>
#include <sys/stat.h>
#include "z80.h"
#include "mz80cmt.h"

/****************************************************************************/
/* character table */
/****************************************************************************/

char *mz80disp[256] = {
  "　", "Ａ", "Ｂ", "Ｃ", "Ｄ", "Ｅ", "Ｆ", "Ｇ",   /* 00 */
  "Ｈ", "Ｉ", "Ｊ", "Ｋ", "Ｌ", "Ｍ", "Ｎ", "Ｏ",
  "Ｐ", "Ｑ", "Ｒ", "Ｓ", "Ｔ", "Ｕ", "Ｖ", "Ｗ",   /* 10 */
  "Ｘ", "Ｙ", "Ｚ", "┼", "└", "┘", "├", "┴",
  "０", "１", "２", "３", "４", "５", "６", "７",   /* 20 */
  "８", "９", "－", "＝", "；", "／", "．", "，",
  NULL, NULL, NULL, NULL, NULL, NULL, "▔", "▎",   /* 30 */
  NULL, NULL, "▄", NULL, "▁", "▕" , "▂", NULL,
  "　", "♠", "◥", "█", "♦", "←", "♣", "●",       /* 40 */
  "○", "？", NULL, "╭", "╮", "	◣", "◢	", "：",
  "↑", "＜", "［", "♥", "］", "＠", "◤", "＞",     /* 50 */
  NULL, "＼", NULL, "▚", "┌", "┐", "┤", "┬",
  "π", "！", "\" ", "＃", "＄", "％", "＆", "' ",  /* 60 */
  "（", "）", "＋", "＊", "▞", "╳", "╯", "╰",
  "▔", "▏", NULL, NULL, NULL, NULL, "╱", "╲",      /* 70 */
  NULL, NULL, NULL, "▌", NULL, NULL, "▃", NULL,
  "　", "チ", "コ", "ソ", "シ", "イ", "ハ", "キ",   /* 80 */
  "ク", "ニ", "マ", "ノ", "リ", "モ", "ミ", "ラ",
  "セ", "タ", "ス", "ト", "カ", "ナ", "ヒ", "テ",   /* 90 */
  "サ", "ン", "ツ", "ロ", "ケ", "「", "ァ", "ャ",
  "ワ", "ヌ", "フ", "ア", "ウ", "エ", "オ", "ヤ",   /* a0 */
  "ユ", "ヨ", "ホ", "ヘ", "レ", "メ", "ル", "ネ",
  "ム", "」", "ィ", "ュ", "ヲ", "、", "ゥ", "ョ",   /* b0 */
  "゜", "・", "ェ", "ッ", "゛", "。", "ォ", "ー",
  "⭳", "⬇", "⬆", "➡", "⬅", NULL, NULL, NULL, /* c0 */
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  "日", "月", "火", "水", "木", "金", "土", "生",   /* d0 */
  "年", "時", "分", "秒", "円", "￥", "￡", "🐍",
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* e0 */
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, "■",
  "　", "▘", "▝", "▀", "▖", "▌", "▞", "▛",    /* f0 */
  "▗", "▚", "▐", "▜", "▄", "▙", "▟", "█",
};

char *mz80disphalf[256] = {
  " ", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M", "N", "O",
  "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",  0,   0,   0,   0,   0,
  "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "-", "=", ";", "/", ".", ",",
  " ",  0,   0,   0,   0,   0,  "▔",  0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,  "?",  0,   0,   0,   0,   0,  ":",
   0,  "<", "[",  0,  "]", "@",  0,  ">",  0,  "\\", 0,   0,   0,   0,   0,   0,
   0,  "!", "\"","#", "$", "%", "&", "\'","(", ")", "+", "*",  0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,  "/", "\\", 0,   0,   0,   0,   0,   0,   0,   0,

  " ", "ﾁ", "ｺ", "ｿ", "ｼ", "ｲ", "ﾊ", "ｷ", "ｸ", "ﾆ", "ﾏ", "ﾉ", "ﾘ", "ﾓ", "ﾐ", "ﾗ",
  "ｾ", "ﾀ", "ｽ", "ﾄ", "ｶ", "ﾅ", "ﾋ", "ﾃ", "ｻ", "ﾝ", "ﾂ", "ﾛ", "ｹ", "｢", "ｧ", "ｬ",
  "ﾜ", "ﾇ", "ﾌ", "ｱ", "ｳ", "ｴ", "ｵ", "ﾔ", "ﾕ", "ﾖ", "ﾎ", "ﾍ", "ﾚ", "ﾒ", "ﾙ", "ﾈ",
  "ﾑ", "｣", "ｨ", "ｭ", "ｦ", "､", "ｩ", "ｮ", "ﾟ", "･", "ｪ", "ｯ", "ﾞ", "｡", "ｫ", "ｰ",
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, "░",
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
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
    { NULL, NULL, NULL, "/", ",", "n", "v", "x" },
    { NULL, NULL, NULL, "\r", "\x1b[C", NULL, "\x7f", NULL },
      { NULL, NULL, NULL, NULL, NULL, "\x1b[B", " ", NULL },
  },
  {   /* shift keymap */
    { NULL, NULL, "+", ")", "'", "%", "#", "!" },
    { NULL, NULL, NULL, "_", "(", "&", "$", "\"" },
    { NULL, NULL, "*", ":", "@", "]", "E", "<" },
    { NULL, NULL, NULL, "^", "?", "\\", "[", ">" },
    { NULL, NULL, NULL, "L", "J", "G", "D", "A" },
    { NULL, NULL, NULL, NULL, "K", "H", "F", "S" },
    { NULL, NULL, NULL, ">", "M", "B", "C" "Z" },
    { NULL, NULL, NULL, NULL, "<", "N", "V", "X" },
    { NULL, NULL, NULL, NULL, "\x1b[D", NULL, NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL, "\x1b[A", NULL, NULL },
  },
};

/****************************************************************************/

int verbose = 0;
int nodisp = 0;
int nowait = 0;
int halfwidth = 0;
int terminate = 0;

/* CPU context */
z80 cpu;

unsigned long long total_cycles;

/* Memory */
byte mz80rom[0x1000];
byte mz80ram[0xc000];
byte mz80text[0x400];

/* Keyboard support */
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

char mz80_i8255pc = 0x00;

int mz80cur_stat;
int mz80cur_timer;
int mz80vsync_stat = 0;
int mz80vsync_timer = 0;
int mz80tempo_stat = 0;
int mz80tempo_timer = 0;

struct i8253ctr {
  int start;
  int enable;
  int mode;
  int fmt;
  int count;
  int reload;
  int latch;
  int latched;
  int hilo;
  int loval;
} mz80i8253ctr[3];

int mz80count1_cycle = 0;

char *mz80autocmd = NULL;

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
            data &= ~mz80key_bit;
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
      static int x;

      data = mz80_i8255pc & 0x0f;
      data |= (1 - mz80vsync_stat) << 7;
      data |= mz80cur_stat << 6;
      data |= (mz80cmt_motorstat() << 4) | (mz80cmt_read() << 5);
    }
    if (verbose)
      device = "8255";

  } else if (address >= 0xe004 && address <= 0xe007) {

    /* 8253 */
    data = 0;
    if (address < 0xe007) {
      struct i8253ctr *p = &mz80i8253ctr[address - 0xe004];
      if (!p->latched) {
        p->latch = p->count;
      }
      if (p->hilo) {
        data = (p->latch) >> 8;
        p->latched = 0;
      } else {
        data = p->latch & 0xff;
      }
      p->hilo = 1 - p->hilo;
    }
    if (verbose)
      device = "8253";

  } else if (address == 0xe008) {

    /* TEMPO */
    data = mz80tempo_stat ? 0xff : 0xfe;
    if (verbose)
      device = "TEMPO";

  } else {

    /* Unknown device */
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
    p = halfwidth ? mz80disphalf[data] : mz80disp[data];
    p = p ? p : "";
    if (!verbose) {
      if (!nodisp) {
        printf("\x1b[%d;%dH%s", y + 1, (x * (halfwidth ? 1 : 2)) + 1, p);
        fflush(stdout);
      }
    } else {
      printf("VRAM W (%2d,%2d) %02x %s\n", x, y, data, p);
    }

  } else if (address >= 0xe000 && address <= 0xe003) {

    /* 8255 */
    if (address == 0xe000) {
      mz80key_i8255pa = data & 0x0f;
      if (!(data & 0x80)) {
        mz80cur_stat = 0;
        mz80cur_timer = 0;
      }
    } else if (address == 0xe002) {
      ;
    } else if (address == 0xe003) {
      if (!(data & 0x80)) {
        int bit = (data >> 1) & 3;
        int set = data & 1;
        int prev = mz80_i8255pc;

        if (bit == 3) {
          mz80cmt_motoron(set, total_cycles);
        } else if (bit == 1) {
          mz80cmt_write(set, total_cycles);
        }
        mz80_i8255pc &= ~(1 << bit);
        mz80_i8255pc |= set << bit;
      }
    }
    if (verbose)
      device = "8255";

  } else if (address >= 0xe004 && address <= 0xe007) {

    /* 8253 */
    if (address < 0xe007) {
      struct i8253ctr *p = &mz80i8253ctr[address - 0xe004];
      if (p->hilo) {
        int newval = (data << 8) | p->loval;
        p->reload = newval;
        if (p->mode == 0) {
          p->enable = 0;
          p->start = 1;
        } else {
          if (!p->enable) {
            p->start = 1;
          }
        }
      } else {
        p->loval = data;
      }
      p->hilo = 1 - p->hilo;
    } else {
      int ch = (data & 0xc0) >> 6;
      struct i8253ctr *p = &mz80i8253ctr[ch];
      p->fmt = (data & 0x0e) >> 1;
      if (p->fmt == 0) {
        p->latch = p->count;
        p->latched = 1;
      } else {
        int mode = (data & 0x0e) >> 1;
        p->latched = 0;
        p->mode = mode;
        p->start = 0;
        p->enable = 0;
        p->reload = 0;
      }
    }
    if (verbose)
      device = "8253";

  } else if (address == 0xe008) {

    /* TEMPO */
    if (verbose)
      device = "TEMPO";

  } else {

    /* Unknown device */
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
      verbose = 1 - verbose;
      return 0;
    } else if (ch == '\x17') {  /* ^W : wait */
      nowait = 1 - nowait;
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

#define CPU_2MHZ      (2 * 1000 * 1000)
#define VSYNC_LOW     (CPU_2MHZ * 230 / 260 / 60)
#define VSYNC_HIGH    (CPU_2MHZ *  30 / 260 / 60)
#define CURSOR_HZ     (3)
#define TEMPO_HZ      (100)
#define SYNCHZ        (1000)
#define NANOSEC       (1000 * 1000 * 1000)
#define COUNTER1_HZ   31250

static void mz80main(void)
{
  struct termios oldt, newt;
  int oldf;
  int x;
  struct timespec oldts, newts, wait;
  long cycles;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  newt.c_iflag &= ~(INLCR | IGNCR | ICRNL);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  total_cycles = 0;
  cycles = 0;
  wait.tv_sec = 0;
  wait.tv_nsec = NANOSEC / SYNCHZ;

  z80_reset(&cpu);
  clock_gettime(CLOCK_MONOTONIC, &oldts);

  x = 1000;
  do {
    if (!nowait || (--x <= 0)) {
      if (mz80keyscan() < 0) {
        break;
      }
      x = 1000;
    }

    if (cpu.pc == 0x003e) {
      printf("\a");     /* bell */
      fflush(stdout);
    }

    z80_main(&cpu);
    total_cycles += cpu.cycles;

    mz80cur_timer += cpu.cycles;
    if (mz80cur_timer >= CPU_2MHZ / CURSOR_HZ) {
      mz80cur_timer -= CPU_2MHZ / CURSOR_HZ;
      mz80cur_stat = 1 - mz80cur_stat;
    }

    mz80tempo_timer += cpu.cycles;
    if (mz80tempo_timer >= CPU_2MHZ / TEMPO_HZ) {
      mz80tempo_timer -= CPU_2MHZ / TEMPO_HZ;
      mz80tempo_stat = 1 - mz80tempo_stat;
    }

    mz80vsync_timer += cpu.cycles;
    if (mz80vsync_stat) {
      if (mz80vsync_timer >= VSYNC_HIGH) {
        mz80vsync_timer -= VSYNC_HIGH;
        mz80vsync_stat = 0;
      }
    } else {
      if (mz80vsync_timer >= VSYNC_LOW) {
        mz80vsync_timer -= VSYNC_LOW;
        mz80vsync_stat = 1;
      }
    }

    mz80count1_cycle += cpu.cycles;
    if (mz80count1_cycle >= (CPU_2MHZ / COUNTER1_HZ)) {
      mz80count1_cycle -= CPU_2MHZ / COUNTER1_HZ;

      int i;
      struct i8253ctr *p;
      for (i = 0; i < 3; i++) {
        p = &mz80i8253ctr[i];
        if (p->start && !p->enable) {
          p->start = 0;
          p->enable = 1;
          p->count = p->reload;
        }
      }
      
      p = &mz80i8253ctr[1];       /* 8253 counter 1 */
      if (p->enable) {
        if ((--p->count <= 0) ||
            (p->count <= 1 && p->mode == 2)) {
          p->count = p->reload;
          p++;                    /* 8253 counter 2 */
          if (p->enable) {
            if ((--p->count <= 0) ||
                (p->count <= 1 && p->mode == 2)) {
              cpu.intr = 1;
            }
          }
        }
      }
    }

    if (z80_intack(&cpu)) {
      if (cpu.intr) {
        z80_int(&cpu, 0);
        cpu.intr = 0;
      }
    }

    if (!nowait) {
      cycles += cpu.cycles;
      if (cycles > (CPU_2MHZ / SYNCHZ)) {
        nanosleep(&wait, NULL);
        clock_gettime(CLOCK_MONOTONIC, &newts);
        long nsec2;
        nsec2 = (newts.tv_sec - oldts.tv_sec) * NANOSEC;
        nsec2 += newts.tv_nsec - oldts.tv_nsec;
        cycles -= nsec2 / (NANOSEC / CPU_2MHZ);
        oldts = newts;
      } 
    }

  } while (cpu.pc != 0);

  printf("\n");

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);
}

char *loadfiles[10];
int loadfiles_num = 0;

char *mz80cmt_loadfilename(void)
{
  static int n;

  if (n < loadfiles_num) {
    return loadfiles[n++];
  }
  return "";
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

  extern char mz_newmon[];
  memcpy(mz80rom, mz_newmon, sizeof(mz80rom));

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
    } else if (strcmp(argv[i], "-n") == 0) {
      nodisp = 1;
    } else if (strcmp(argv[i], "-H") == 0) {
      halfwidth = 1;
    } else {
      struct stat statbuf;
      if (stat(argv[i], &statbuf) < 0) {
        help = 1;
        break;
      }
      if (loadfiles_num < (sizeof(loadfiles) / sizeof(loadfiles[0]))) {
        loadfiles[loadfiles_num++] = argv[i];
      }
    }
  }

  if (help) {
    printf("Usage: ttymz80 [-n][-H][-r <ROM image>] [<mzt/mzf file>...]\n");
    return 1;
  }

  mz80main();
}
