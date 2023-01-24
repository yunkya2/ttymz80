/*
 * MZ-80K/C & MZ-700 emulator on terminal
 *
 * Copyright (c) 2023, Yuichi Nakamura <yunk@ya2.so-net.ne.jp>
 * This software is released under 2-clause BSD license.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <ctype.h>
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
  "▔▔", "▏ ", "▏▁", "▁▕", "╶╶", " ▏", "\1▇▇", "▎ ",   /* 30 */
  "▁▁", " ▕", "▄▄", " █", "▁▁", " ▕" , "▂▂", "\1█▊",
  "　", "♠ ", "◥ ", "██", "♦ ", "← ", "♣ ", "● ",       /* 40 */
  "○ ", "？", "\1● ", "╭", "╮", " ◣", "◢ ", "：",
  "↑ ", "＜", "［", "♥ ", "］", "＠", "◤", "＞",     /* 50 */
  "░ ", "＼", "\1░░", "▀▄", "┌", "┐", "┤", "┬",
  "π ", "！", "\" ", "＃", "＄", "％", "＆", "' ",  /* 60 */
  "（", "）", "＋", "＊", "▄▀", "✕ ", "╯ ", "╰ " ,
  "▔▔", "▏ ", "▔▕", "▏▔", "▔▔", "▏ ", "／", "＼",  /* 70 */
  "▁▁", " ▕", "▀▀", "█ ", "▁▁", " ▕", "▃▃", "\1█▍",
  "　", "チ", "コ", "ソ", "シ", "イ", "ハ", "キ",   /* 80 */
  "ク", "ニ", "マ", "ノ", "リ", "モ", "ミ", "ラ",
  "セ", "タ", "ス", "ト", "カ", "ナ", "ヒ", "テ",   /* 90 */
  "サ", "ン", "ツ", "ロ", "ケ", "「", "ァ", "ャ",
  "ワ", "ヌ", "フ", "ア", "ウ", "エ", "オ", "ヤ",   /* a0 */
  "ユ", "ヨ", "ホ", "ヘ", "レ", "メ", "ル", "ネ",
  "ム", "」", "ィ", "ュ", "ヲ", "、", "ゥ", "ョ",   /* b0 */
  "゜", ". ", "ェ", "ッ", "゛", "。", "ォ", "ー",
  "⭳ ", "\1↓", "\1↑", "\1→", "\1←", "\1Ｈ", "\1Ｃ", "🛸", /* c0 */
  "🚗", "🚘", "⮙ ", "⮘ ", "⮚ ", "⮛ ", "😐", "😃",
  "日", "月", "火", "水", "木", "金", "土", "生",   /* d0 */
  "年", "時", "分", "秒", "円", "￥", "￡", "🐍",
  "-⸸", " ⸸", "-⸸", "～", "╶᚜", "ᘮ ", "᚛╶", "ᐅ|", /* e0 */
  "|ᐊ", "|↘", "|↙", ">-", "⊣⊢", "⍊ ", "≀ ", "░░",
  "　", "▀ ", " ▀", "▀▀", "▄ ", "█ ", "▄▀", "█▀",    /* f0 */
  " ▄", "▀▄", " █", "▀█", "▄▄", "█▄", "▄█", "██",
};

char *mz700disp[256] = {
  NULL, "ａ", "ｂ", "ｃ", "ｄ", "ｅ", "ｆ", "ｇ",   /* 00 */
  "ｈ", "ｉ", "ｊ", "ｋ", "ｌ", "ｍ", "ｎ", "ｏ",
  "ｐ", "ｑ", "ｒ", "ｓ", "ｔ", "ｕ", "ｖ", "ｗ",   /* 10 */
  "ｘ", "ｙ", "ｚ", NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 20 */
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 30 */
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 40 */
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 50 */
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 60 */
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* 70 */
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, "ち", "こ", "そ", "し", "い", "は", "き",   /* 80 */
  "く", "に", "ま", "の", "り", "も", "み", "ら",
  "せ", "た", "す", "と", "か", "な", "ひ", "て",   /* 90 */
  "さ", "ん", "つ", "ろ", "け", "「", "ぁ", "ゃ",
  "わ", "ぬ", "ふ", "あ", "う", "え", "お", "や",   /* a0 */
  "ゆ", "よ", "ほ", "へ", "れ", "め", "る", "ね",
  "む", NULL, "ぃ", "ゅ", "を", NULL, "ぅ", "ょ",   /* b0 */
  NULL, NULL, "ぇ", "っ", NULL, NULL, "ぉ", NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* c0 */
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* d0 */
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* e0 */
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, /* f0 */
  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
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
    { NULL, NULL, "\x1bOP", ".", "m", "b", "c", "z" },
    { NULL, NULL, NULL, "/", ",", "n", "v", "x" },
    { NULL, NULL, NULL, "\r", "\x1b[C", NULL, "\x7f", "\t" },
    { NULL, NULL, NULL, NULL, NULL, "\x1b[B", " ", "\x1b[H" },
  },
  {   /* shift keymap */
    { NULL, NULL, "+", ")", "'", "%", "#", "!" },
    { NULL, NULL, NULL, "_", "(", "&", "$", "\"" },
    { NULL, NULL, "*", ":", "@", "]", "E", "<" },
    { NULL, NULL, NULL, "^", "?", "\\", "[", ">" },
    { NULL, NULL, NULL, "L", "J", "G", "D", "A" },
    { NULL, NULL, NULL, NULL, "K", "H", "F", "S" },
    { NULL, NULL, "\x1b[1;2P", NULL, "M", "B", "C" "Z" },
    { NULL, NULL, NULL, NULL, "<", "N", "V", "X" },
    { NULL, NULL, NULL, NULL, "\x1b[D", NULL, "\x1b[2~", NULL },
    { NULL, NULL, NULL, NULL, "\x1a", "\x1b[A", NULL, "\x1b[F" },
  },
};

char *mz700keytbl[][10][8] = {
  {   /* non-shift keymap */
    { "\x1bOQ", "\x1bOR", "=",  "\x1bOP", NULL, ";",  ":",  "\r" },
    { "y",  "z",  "@",  "(",  ")",  NULL, NULL, NULL },
    { "q",  "r",  "s",  "t",  "u",  "v",  "w",  "x"  },
    { "i",  "j",  "k",  "l",  "m",  "n",  "o",  "p"  },
    { "a",  "b",  "c",  "d",  "e",  "f",  "g",  "h"  },
    { "1",  "2",  "3",  "4",  "5",  "6",  "7",  "8"  },
    { "*",  "+",  "-",  " ",  "0",  "9",  ",",  "."  },
    { "\x1b[2~", "\x7f", "\x1b[A", "\x1b[B", "\x1b[C", "\x1b[D", "?", "/" },
    { NULL, "\x1b", NULL, NULL, NULL, NULL, NULL, "\t" },
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  }, {
    { "\x1b[1;2P", NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    { "Y",  "Z",  "@",  NULL, NULL, NULL, NULL, NULL },
    { "Q",  "R",  "S",  "T",  "U",  "V",  "W",  "X"  },
    { "I",  "J",  "K",  "L",  "M",  "N",  "O",  "P"  },
    { "A",  "B",  "C",  "D",  "E",  "F",  "G",  "H"  },
    { "!",  "\"", "#",  "$",  "%",  "&",  "'",  "["  },
    { NULL, NULL, "\\", NULL, NULL, "]",  "<",  ">"  },
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    { "\x1a", NULL, NULL, NULL, NULL, NULL, NULL, NULL },
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
  },
};

/****************************************************************************/

int verbose = 0;
int nodisp = 0;
int nowait = 0;
int halfwidth = 0;
int terminate = 0;
int mz700 = 0;

/* CPU context */
z80 cpu;

int total_cycles;
int cpu_clock = 2 * 1000 * 1000;
int count1_clock = 31250;

/* Memory */
byte mz80rom[0x1000];
byte mz80ram[0x10000];
byte mz80text[0x1000];
int mz700bank0;
int mz700bank1;

/* Keyboard support */
char *(*keytbl)[10][8] = mz80keytbl;
int mz80key_i8255pa;

char mz80_i8255pc = 0x00;

int mz80cur_stat;
int mz80cur_timer;
int mz80vsync_stat;
int mz80vsync_timer;
int mz80tempo_stat;
int mz80tempo_timer;
int mz80count1_timer;

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

char *mz80scankey = NULL;
char *mz80autokey = NULL;

char *mz80autocmd = NULL;
char *mz80waitcmd = NULL;
char *mz80waitcmd_p;

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

#define AUTOCMD_DELAY       40
#define AUTOCMD_DELAY_700   30

static int mz80keyscan(void)
{
  int i, j, k;
  char *key = NULL;

  if (mz80scankey) {
    key = mz80scankey;
    mz80scankey = NULL;
  } else if (mz80autokey) {
    static int delay = AUTOCMD_DELAY;
    if (delay >= 0) {
      delay--;
      return -1;
    }
    delay = mz700 ? AUTOCMD_DELAY_700 : AUTOCMD_DELAY;
    key = mz80autokey;
    mz80autokey = NULL;
  }
  if (key == NULL)
    return -1;

  for (i = 0; i < 2; i++) {
    for (j = 0; j < 10; j++) {
      for (k = 0; k < 8; k++) {
        if (keytbl[i][j][k] &&
            (strcmp(keytbl[i][j][k], key) == 0)) {
              return k + j * 8 + i * 128;
        }
      }
    }
  }
  return -1;
}

byte z80_read(word address)
{
  byte data = 0;
  char *device = NULL;

  if (!mz700bank0 && address < 0x1000) {

    /* ROM */
    data = mz80rom[address];
    device = disasm(address, "ROM ");

  } else if (mz700bank1 || address < 0xd000) {

    /* RAM */
    data = mz80ram[address];
    device = disasm(address, "RAM ");

  } else if (address >= 0xd000 && address < 0xe000) {

    /* Text VRAM */
    int offset = mz700 ? address & 0x0fff : address & 0x03ff;
    data = mz80text[offset];
    if (verbose) {
      int x = (offset & 0x03ff) % 40;
      int y = (offset & 0x03ff) / 40;
      printf("VRAM R %04x (%2d,%2d) %02x\n", offset, x, y, data);
    }

  } else if (address >= 0xe000 && address <= 0xe003) {

#define KEYPRESS_DELAY 3

    /* 8255 */
    data = 0xff;
    if (address == 0xe001) {
      static int strobe;
      static int bit;
      static enum keystate {
        KEY_NONE,
        KEY_PRESS,
        KEY_SHIFTPRESS,
        KEY_SHIFTPRESS1,
      } state = KEY_NONE;
      static int scanned = 0;
      static int count;
      static int presstime;

      if (state == KEY_NONE &&
          (mz80key_i8255pa == 9 ||
           (scanned & (1 << mz80key_i8255pa)) != 0)) {
        int key = mz80keyscan();
        if (key >= 0) {
          bit = 1 << (7 - (key % 8));
          strobe = (key & 0x7f) / 8;
          state = key > 0x80 ? KEY_SHIFTPRESS : KEY_PRESS;
          count = KEYPRESS_DELAY;
          presstime = total_cycles;
        }
        scanned = 0;
      }
      scanned |= 1 << mz80key_i8255pa;

      switch (state) {
        case KEY_NONE:
          break;

        case KEY_PRESS:
          if (strobe == mz80key_i8255pa) {
            data = ~bit;
            if (--count <= 0 ||
                total_cycles - presstime > cpu_clock) {
              if (total_cycles - presstime > cpu_clock) {
                data = 0xff;
              }
              state = KEY_NONE;
              count = KEYPRESS_DELAY;
              presstime = total_cycles;
            }
          }
          break;

        case KEY_SHIFTPRESS:
          if (mz80key_i8255pa == 8) {
            data = ~0x01;     /* shift key */
            state = KEY_SHIFTPRESS1;
            count = KEYPRESS_DELAY;
            scanned = 0;
            presstime = total_cycles;
          }
          break;
        case KEY_SHIFTPRESS1:
          if (mz80key_i8255pa == 8) {
            data = ~0x01;
          }
          if (strobe == mz80key_i8255pa) {
            data &= ~bit;
          }
          if ((scanned & (1 << 8)) &&
              (scanned & (1 << strobe))) {
            if (--count <= 0) {
              state = KEY_NONE;
              count = KEYPRESS_DELAY;
              presstime = total_cycles;
            }
          }
          break;
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
    data = 0xff;
    if (verbose)
      device = "????";

  }

  if (device)
    printf("%s R %04x %02x\n", device, address, data);

  return data;
}

/****************************************************************************/

static const int colconv[8] = { 0, 4, 1, 5, 2, 6, 3, 7 };

void z80_write(word address, byte data)
{
  char *device = NULL;
  
  if (mz700bank1 || address < 0xd000) {

    /* RAM */
    mz80ram[address] = data;
    if (verbose)
      device = "RAM ";

  } else if (address >= 0xd000 && address < 0xe000) {

    /* Text VRAM */
    int x, y;
    int offset = mz700 ? address & 0x0fff : address & 0x03ff;
    char *p;
    int rev = 0;
    byte attr;
    byte disp = data;
    mz80text[offset] = data;

    if (mz700) {
      disp = mz80text[offset & 0x07ff];
      attr = mz80text[(offset & 0x07ff) + 0x0800];
    }

    if (mz80waitcmd && address < 0xd800) {
      char ch;
      static int prev;
      p = mz80disphalf[disp];
      if (p && *p >= ' ' && *p < 0x80) {
        ch = *p; 
        if (total_cycles - prev > 500 &&
            toupper(*mz80waitcmd_p++) == ch) {
          ch = *mz80waitcmd_p;
          if (ch == '\0') {
            mz80autocmd = NULL;
            mz80waitcmd = NULL;
          } else if (ch == '}') {
            mz80autocmd = ++mz80waitcmd_p;
            mz80waitcmd = NULL;
          }
        } else {
          mz80waitcmd_p = mz80waitcmd;
        }
        prev = total_cycles;
      }
    }

    if (halfwidth) {
      p = mz80disphalf[disp];
      p = p ? p : " ";
    } else {
      if (mz700 && (attr & 0x80)) {
        p = mz700disp[disp];
        p = p ? p : mz80disp[disp];
      } else {
        p = mz80disp[disp];
      }
      p = p ? p : "  ";
    }
    if (*p == '\1') {
      p++;
      rev = 1;
    }
    x = (offset & 0x03ff) % 40;
    y = (offset & 0x03ff) / 40;
    if (!verbose) {
      if (!nodisp && !(offset & 0x0400) && y < 25) {
        if (mz700) {
          printf("\x1b[%d;%dm",
                 30 + colconv[(attr >> 4) & 7], 
                 40 + colconv[attr & 7]);
        }
        printf("\x1b[%d;%dH%s%s%s",
               y + 1, (x * (halfwidth ? 1 : 2)) + 1,
               rev ? "\x1b[7m" : "",
               p,
               rev ? "\x1b[27m" : "");
      }
    } else {
      printf("VRAM W %04x (%2d,%2d) %02x %s\n",
             offset, x, y, data, offset >= 0x0800 ? "  " : p);
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
  address &= 0xff;
  if (verbose)
    printf("z80_in: %04x\n", address);
  return 0;
}

/****************************************************************************/

void z80_out(word address, byte data)
{
  address &= 0xff;
  if (address == 0xe0) {
    mz700bank0 = 1;
  } else if (address == 0xe1) {
    mz700bank1 = 1;
  } else if (address == 0xe2) {
    mz700bank0 = 0;
  } else if (address == 0xe3) {
    mz700bank1 = 0;
  } else if (address == 0xe4) {
    mz700bank0 = 0;
    mz700bank1 = 0;
  }

  if (verbose)
    printf("z80_out: %04x %02x\n", address, data);
}

/****************************************************************************/

#define VSYNC_LOW     (cpu_clock * 230 / 260 / 60)
#define VSYNC_HIGH    (cpu_clock *  30 / 260 / 60)
#define CURSOR_HZ     (3)
#define TEMPO_HZ      (100)
#define SYNCHZ        (1000)
#define NANOSEC       (1000 * 1000 * 1000)

static void mz80main(void)
{
  struct termios oldt, newt;
  int oldf;
  struct timespec oldts, newts, wait;
  long cycles;
  int keyscan = 1;

  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  cfmakeraw(&newt);
  newt.c_oflag |= OPOST;
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  printf("\x1b[?25l\n");    /* cursor off */

  total_cycles = 0;
  cycles = 0;
  wait.tv_sec = 0;
  wait.tv_nsec = NANOSEC / SYNCHZ;

  z80_reset(&cpu);
  clock_gettime(CLOCK_MONOTONIC, &oldts);

  do {
    if (!mz80waitcmd && !mz80autokey && mz80autocmd) {
      char ch;
      do {
        ch = *mz80autocmd++;
        if (ch == '\0') {
          mz80autocmd = NULL;
          break;
        }
      } while (ch < ' ');
      if (ch == '|') {
        ch = '\r';
      } else if (ch == '{') {
        ch = '\0';
        if (*mz80autocmd) {
          mz80waitcmd = mz80autocmd;
          mz80waitcmd_p = mz80waitcmd;
        }
      }
      if (ch) {
        static char key[2];
        key[0] = ch;
        key[1] = '\0';
        mz80autokey = key;
      }
    }

    if (keyscan) {
      static char key[10];
      static int delay = 50;
      int len = -1;
      keyscan = 0;

      delay = nowait ? (delay - 1) : 0;
      if (delay <= 0) {
        len = read(fileno(stdin), key, sizeof(key) - 1);
        delay = 50;
      }
      if (len >= 0) {
        if (strcmp(key, "\x03") == 0) {       /* ^C : exit */
          break;
        } else if (strcmp(key, "\x01") == 0) {/* ^A : switch verbose */
          verbose = 1 - verbose;
        } else if (strcmp(key, "\x17") == 0) {/* ^W : wait */
          nowait = 1 - nowait;
        } else {
          key[len] = '\0';
          mz80scankey = key;
        }
      }
    }

    if (!mz700 && cpu.pc == 0x003e) {
      printf("\a");     /* bell */
      fflush(stdout);
    }

    z80_main(&cpu);
    total_cycles += cpu.cycles;

    mz80cur_timer += cpu.cycles;
    if (mz80cur_timer >= cpu_clock / CURSOR_HZ) {
      mz80cur_timer -= cpu_clock / CURSOR_HZ;
      mz80cur_stat = 1 - mz80cur_stat;
    }

    mz80tempo_timer += cpu.cycles;
    if (mz80tempo_timer >= cpu_clock / TEMPO_HZ) {
      mz80tempo_timer -= cpu_clock / TEMPO_HZ;
      mz80tempo_stat = 1 - mz80tempo_stat;
    }

    mz80vsync_timer += cpu.cycles;
    if (mz80vsync_stat) {
      if (mz80vsync_timer >= VSYNC_HIGH) {
        mz80vsync_timer -= VSYNC_HIGH;
        mz80vsync_stat = 0;
        keyscan = 1;
        fflush(stdout);
      }
    } else {
      if (mz80vsync_timer >= VSYNC_LOW) {
        mz80vsync_timer -= VSYNC_LOW;
        mz80vsync_stat = 1;
      }
    }

    mz80count1_timer += cpu.cycles;
    if (mz80count1_timer >= (cpu_clock / count1_clock)) {
      mz80count1_timer -= cpu_clock / count1_clock;
      struct i8253ctr *p;
      p = &mz80i8253ctr[1];       /* 8253 counter 1 */
      if (!p->enable) {
        if (p->start) {
          p->start = 0;
          p->enable = 1;
          p->count = p->reload;
        }
      } else {
        if (--p->count == 0) {
          p->count = p->reload;
        }
        if (p->count == 1) {
          p++;                    /* 8253 counter 2 */
          if (!p->enable) {
            if (p->start) {
              p->start = 0;
              p->enable = 1;
              p->count = p->reload;
            }
          } else {
            if (--p->count == 0) {
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
      if (cycles > (cpu_clock / SYNCHZ)) {
        nanosleep(&wait, NULL);
        clock_gettime(CLOCK_MONOTONIC, &newts);
        long nsec2;
        nsec2 = (newts.tv_sec - oldts.tv_sec) * NANOSEC;
        nsec2 += newts.tv_nsec - oldts.tv_nsec;
        cycles -= nsec2 / (NANOSEC / cpu_clock);
        oldts = newts;
      } 
    }

  } while (!((cpu.pc == 0) && (mz700bank0 == 0)));

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  printf("\x1b[0m\x1b[?25h\n");     /* cursor on */
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
  ".incbin \"mz_newmon/NEWMON.ROM\"\n"
  ".global mz_newmon7\n"
  "mz_newmon7:\n"
  ".incbin \"mz_newmon/NEWMON7.ROM\""
);

int main(int argc, char **argv)
{
  FILE *fp;
  int i;
  int help = 0;
  int romload = 0;
  int autocmdlen = 0;

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
        romload = 1;
        i++;
      } else {
        help = 1;
      }
    } else if (strcmp(argv[i], "-a") == 0) {
      mz80autocmd = realloc(mz80autocmd,
                            autocmdlen + strlen("load|") + 1);
      strcat(mz80autocmd, "load|");
      autocmdlen = strlen(mz80autocmd);
    } else if (strcmp(argv[i], "-c") == 0) {
      if (i + 1 < argc) {
        mz80autocmd = realloc(mz80autocmd,
                              autocmdlen + strlen(argv[i + 1]) + 1);
        strcat(mz80autocmd, argv[i + 1]);
        autocmdlen = strlen(mz80autocmd);
        i++;
      } else {
        help = 1;
      }
    } else if (strcmp(argv[i], "-C") == 0) {
      if (i + 1 < argc) {
        fp = fopen(argv[i + 1], "rb");
        if (fp == NULL) {
          printf("file not found\n");
          help = 1;
          break;
        }
        fseek(fp, 0, SEEK_END);
        int len = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        mz80autocmd = realloc(mz80autocmd, autocmdlen + len + 1);
        fread(&mz80autocmd[autocmdlen], len, 1, fp);
        mz80autocmd[autocmdlen + len] = '\0';
        autocmdlen = strlen(mz80autocmd);
        fclose(fp);
        i++;
      } else {
        help = 1;
      }
    } else if (strcmp(argv[i], "-n") == 0) {
      nodisp = 1;
    } else if (strcmp(argv[i], "-w") == 0) {
      nowait = 1;
    } else if (strcmp(argv[i], "-H") == 0) {
      halfwidth = 1;
    } else if (strcmp(argv[i], "-7") == 0) {
      mz700 = 1;
      keytbl = mz700keytbl;
      cpu_clock = 3579545;
      count1_clock = 15699;
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
    printf("Usage: ttymz80 [-a][-n][-w][-H][-7][-r <ROM image>][-c <cmd>][-C <cmdfile>] [<mzt/mzf file>...]\n");
    return 1;
  }

  if (!romload) {
    if (mz700) {
      extern char mz_newmon7[];
      memcpy(mz80rom, mz_newmon7, sizeof(mz80rom));
      memset(&mz80text[0x800], 0x70, 0x800);
    } else {
      extern char mz_newmon[];
      memcpy(mz80rom, mz_newmon, sizeof(mz80rom));
    }
  }

  mz80main();
}
