/*
 * MZ-80K/C & MZ-700 emulator on terminal
 *   CMT emulation
 *
 * Copyright (c) 2023, Yuichi Nakamura <yunk@ya2.so-net.ne.jp>
 * This software is released under 2-clause BSD license.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "z80.h"
#include "mz80cmt.h"

/****************************************************************************/
/* static variables */
/****************************************************************************/

/* motor */
static int motor = 1;
static int motorchg_delay = 0;
static int motorchg_prev = 0;

/* load & save */
enum saveload {
  SL_IDLE,
  SL_SAVE,
  SL_LOAD,
};

enum idstate {
  INFOBLOCK,
  DATABLOCK,
};

enum blockstate {
  BS_LEAD,
  BS_HEAD1,
  BS_HEAD0,
  BS_START,
  BS_DATA,
  BS_CKSUM,
  BS_END,
};

enum action {
  AC_BIT0,        /* read/write '0' bit */
  AC_BIT1,        /* read/write '1' bit */
  AC_MEMBYTE,     /* read/write 1byte (memory) */
  AC_FILEBYTE,    /* read/write 1byte (file) */
};

static enum saveload saveload = SL_IDLE;
static enum idstate idstate = INFOBLOCK;
static enum blockstate blockstate = BS_LEAD;
static int repeatcnt = 0;
static enum action rwaction;

static int rwbitcount;
static unsigned int cksum;
static byte rwbyte;
static byte *rwptr;

static byte infoblock[128];
static byte sumdata[2];

static FILE *fp;

extern int mz700;
extern int nowait;
static int nowait_save = -1;

/****************************************************************************/
/* motor control */
/****************************************************************************/

static void motoron(void)
{
  motor = 1;
  if (nowait_save < 0)
    nowait_save = nowait;
   nowait = 1;
}

static void motoroff(void)
{
  motor = 0;
  if (nowait_save >= 0)
    nowait = nowait_save;
  nowait_save = -1;
}

int mz80cmt_motorstat(void)
{
  if (motorchg_delay) {
    if (--motorchg_delay <= 0) {
      motoron();
    }
  }
  return motor;
}

void mz80cmt_motoron(int stat, int cycle)
{
  static int prev;

  if (motorchg_delay > 0 || (prev == stat))
    return;     /* ignore control */

  prev = stat;
  if (!stat)
    return;     /* falling edge */

  /* rising edge .. motor status change */
  if (motor) {
    /* motor stop */
    motoroff();
    motorchg_prev = cycle;
  } else {
    /* motor start */
    unsigned int term = cycle - motorchg_prev;
    motorchg_prev = cycle;
    if (idstate == INFOBLOCK) {
      motorchg_delay = 20;    /* delayed start */
      idstate = INFOBLOCK;
      saveload = SL_IDLE;
    } else {
      motoron();
      motorchg_delay = 0;
    }
  }
}

/****************************************************************************/
/* read bit (load) */
/****************************************************************************/

int mz80cmt_read(void)
{
  int bit;
  static int bitstat;

  if (!motor) {
    return 1;
  }

  if (saveload == SL_IDLE) {
    saveload = SL_LOAD;
    idstate = INFOBLOCK;
    blockstate = BS_LEAD;
    repeatcnt = 0;
  } else if (saveload != SL_LOAD) {
    return 1;
  }

  bitstat = (bitstat + 1) % 3;
  if (bitstat < 2) {
    return bitstat;       /* 0 -> 1 -> ? -> 0 -> 1 -> ? -> ... */
  }

  if (repeatcnt == 0) {
//    printf("(%d%d)", idstate, blockstate); fflush(stdout);

    switch (blockstate) {
      case BS_LEAD:         /* lead bit 0 */
        rwaction = AC_BIT0;
        repeatcnt = 10000;
        break;

      case BS_HEAD1:        /* head bit 1 x 40 or 20 */
        rwaction = AC_BIT1;
        repeatcnt = (idstate == INFOBLOCK) ? 40 : 20;
        break;

      case BS_HEAD0:        /* head bit 0 x 40 or 20 */
        rwaction = AC_BIT0;
        repeatcnt = (idstate == INFOBLOCK) ? 40 : 20;
        break;

      case BS_START:        /* start bit 1 */
        rwaction = AC_BIT1;
        repeatcnt = 1;
        break;

      case BS_DATA:         /* data body */
        if (idstate == INFOBLOCK) {
          rwaction = AC_MEMBYTE;
          rwptr = infoblock;
          repeatcnt = 128;

          int len = 0;
          if (fp == NULL) {
            char *name = mz80cmt_loadfilename();
            if (name) {
              fp = fopen(name, "rb");
            }
          }
          if (fp) {
            len = fread(infoblock, 128, 1, fp);
          }
          if (len != 1) {
            memset(infoblock, 0, 128);
            blockstate = BS_LEAD;
            return 0;
          }
        } else {
          rwaction = AC_FILEBYTE;
          repeatcnt = infoblock[0x12] + (infoblock[0x13] << 8);
        }
        rwbitcount = 0;
        cksum = 0;
        break;

      case BS_CKSUM:        /* checksum */
        rwaction = AC_MEMBYTE;
        rwptr = sumdata;
        repeatcnt = 2;
        sumdata[0] = cksum >> 8;
        sumdata[1] = cksum & 0xff;
        rwbitcount = 0;
        break;

      case BS_END:
        if (idstate == INFOBLOCK) {
          rwaction = AC_BIT1;
          repeatcnt = 1;
          idstate = DATABLOCK;
          blockstate = BS_LEAD - 1;
        } else {
          if (fp != NULL) {
            struct stat statbuf;
            fstat(fileno(fp), &statbuf);
            if (ftell(fp) + 128 >= statbuf.st_size) {
              fclose(fp);
              fp = NULL;
            }
          }
          idstate = INFOBLOCK;
          blockstate = BS_LEAD;
          saveload = SL_IDLE;
          return 1;
        }
        break;
    }
  }

  if (rwaction == AC_BIT0 || rwaction == AC_BIT1) {
    bit = (rwaction == AC_BIT1);
    if (--repeatcnt <= 0) {
      blockstate++;
    }
  } else {
    if (rwbitcount == 0) {
      bit = 1;
      rwbitcount = 8;
      if (rwaction == AC_MEMBYTE) {
        rwbyte = *rwptr++;
      } else {
        if (fp) {
          rwbyte = fgetc(fp);
//          printf("[%02x]", rwbyte); fflush(stdout);
        }
      }
    } else {
      rwbitcount--;
      bit = (rwbyte & (1 << rwbitcount)) != 0;
      cksum += bit;
      if (rwbitcount == 0) {
        if (--repeatcnt <= 0) {
          blockstate++;
        }
      }
    }
  }

//  printf("%d", bit); fflush(stdout);
  return bit;
}

/****************************************************************************/
/* write bit (save) */
/****************************************************************************/

void mz80cmt_write(int bit, int cycle)
{
  int term;
  static int prev_cycle;

  if (!motor) {
    return;
  }

  if (saveload == SL_IDLE) {
    idstate = INFOBLOCK;
    blockstate = BS_LEAD;
    repeatcnt = 0;
    if (fp) {
      fclose(fp);
      fp = NULL;
    }
  }
  saveload = SL_SAVE;

  term = cycle - prev_cycle;
  prev_cycle = cycle;
  if (!bit) {
    return;
  }
  bit = (term > (mz700 ? 1200 : 700));

//  printf("%d", bit); fflush(stdout);

  if (repeatcnt == 0) {
//    printf("[%d%d]", idstate, blockstate); fflush(stdout);

    switch (blockstate) {
      /* informaiton block */
      case BS_LEAD:         /* lead bit 0 */
        blockstate++;
      case BS_HEAD1:        /* head bit 1 x 40 or 20 */
        rwaction = AC_BIT1;
        repeatcnt = (idstate == INFOBLOCK) ? 40 : 20;
        break;

      case BS_HEAD0:        /* head bit 0 x 40 or 20 */
        rwaction = AC_BIT0;
        repeatcnt = (idstate == INFOBLOCK) ? 40 : 20;
        break;

      case BS_START:        /* start bit 1 */
        rwaction = AC_BIT1;
        repeatcnt = 1;
        break;

      case BS_DATA:         /* data body */
        if (idstate == INFOBLOCK) {
          rwaction = AC_MEMBYTE;
          rwptr = infoblock;
          repeatcnt = 128;
        } else {
          static char filename[30];
          int i;

          rwaction = AC_FILEBYTE;
          repeatcnt = infoblock[0x12] + (infoblock[0x13] << 8);
          for (i = 0; i < 17; i++) {
            filename[i] = infoblock[i + 1];
              if (filename[i] < ' ') {
              filename[i] = '\0';
              break;
            }
          }
          strcat(filename, ".mzt");
          fp = fopen(filename, "wb");
          if (fp) {
            fwrite(infoblock, 128, 1, fp);
          }
        }
        rwbitcount = 0;
        cksum = 0;
        break;

      case BS_CKSUM:        /* checksum */
        rwaction = AC_MEMBYTE;
        rwptr = sumdata;
        repeatcnt = 2;
        rwbitcount = 0;
        break;
    }
  }

  if (rwaction == AC_BIT0 || rwaction == AC_BIT1) {
    if (bit == (rwaction == AC_BIT1)) {
      if (--repeatcnt <= 0) {
        blockstate++;
      }
    } else {
      blockstate = BS_LEAD;
      repeatcnt = 0;
    }
  } else {
    if (rwbitcount == 0) {
      if (bit != 1) {
        blockstate = BS_LEAD;
        repeatcnt = 0;
      } else {
        rwbitcount = 8;
        rwbyte = 0;
      }
    } else {
      rwbitcount--;
      rwbyte |= bit << rwbitcount;
      cksum += bit;
      if (rwbitcount == 0) {
        if (rwaction == AC_MEMBYTE) {
          *rwptr++ = rwbyte;
        } else {
          if (fp) {
            fputc(rwbyte, fp);
          }
        }
//        printf("[%02x]", rwbyte); fflush(stdout);
        if (--repeatcnt <= 0) {
          blockstate++;
        }
      }
    }
  }

  if (blockstate == BS_END) {
    if (idstate == INFOBLOCK) {
      idstate = DATABLOCK;
      blockstate = BS_LEAD;
    } else {
      if (fp != NULL) {
        fclose(fp);
        fp = NULL;
      }
      idstate = INFOBLOCK;
      blockstate = BS_LEAD;
      saveload = SL_IDLE;
      motoroff();
    }
  }
}
