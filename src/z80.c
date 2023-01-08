/* Z80 emulator                       */
/* AKIKAWA, Hisashi 2018.5.17 version */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "z80.h"

#define BC (pcpu->c + (pcpu->b << 8))
#define DE (pcpu->e + (pcpu->d << 8))
#define HL (pcpu->l + (pcpu->h << 8))
#define IX (pcpu->ixl + (pcpu->ixh << 8))
#define IY (pcpu->iyl + (pcpu->iyh << 8))

#define FLAGS(var8)		\
  ((var8 & SF)			\
   | (var8 ? 0 : ZF)		\
   | (var8 & YF)		\
   | 0				\
   | (var8 & XF)		\
   | parity_table[var8]		\
   | 0				\
   | 0				\
   )

enum {
  SF = 0x80, ZF = 0x40, YF = 0x20, HF = 0x10,
  XF = 0x08, PF = 0x04, VF = 0x04, NF = 0x02, CF = 0x01
};

const byte parity_table[] = {
  PF, 0 , 0 , PF, 0 , PF, PF, 0 , 0 , PF, PF, 0 , PF, 0 , 0 , PF,
  0 , PF, PF, 0 , PF, 0 , 0 , PF, PF, 0 , 0 , PF, 0 , PF, PF, 0,
  0 , PF, PF, 0 , PF, 0 , 0 , PF, PF, 0 , 0 , PF, 0 , PF, PF, 0,
  PF, 0 , 0 , PF, 0 , PF, PF, 0 , 0 , PF, PF, 0 , PF, 0 , 0 , PF,
  0 , PF, PF, 0 , PF, 0 , 0 , PF, PF, 0 , 0 , PF, 0 , PF, PF, 0,
  PF, 0 , 0 , PF, 0 , PF, PF, 0 , 0 , PF, PF, 0 , PF, 0 , 0 , PF,
  PF, 0 , 0 , PF, 0 , PF, PF, 0 , 0 , PF, PF, 0 , PF, 0 , 0 , PF,
  0 , PF, PF, 0 , PF, 0 , 0 , PF, PF, 0 , 0 , PF, 0 , PF, PF, 0,
  0 , PF, PF, 0 , PF, 0 , 0 , PF, PF, 0 , 0 , PF, 0 , PF, PF, 0,
  PF, 0 , 0 , PF, 0 , PF, PF, 0 , 0 , PF, PF, 0 , PF, 0 , 0 , PF,
  PF, 0 , 0 , PF, 0 , PF, PF, 0 , 0 , PF, PF, 0 , PF, 0 , 0 , PF,
  0 , PF, PF, 0 , PF, 0 , 0 , PF, PF, 0 , 0 , PF, 0 , PF, PF, 0,
  PF, 0 , 0 , PF, 0 , PF, PF, 0 , 0 , PF, PF, 0 , PF, 0 , 0 , PF,
  0 , PF, PF, 0 , PF, 0 , 0 , PF, PF, 0 , 0 , PF, 0 , PF, PF, 0,
  0 , PF, PF, 0 , PF, 0 , 0 , PF, PF, 0 , 0 , PF, 0 , PF, PF, 0,
  PF, 0 , 0 , PF, 0 , PF, PF, 0 , 0 , PF, PF, 0 , PF, 0 , 0 , PF,
};

const byte newflags[] = {
  0x44, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
  0x08, 0x0c, 0x0c, 0x08, 0x0c, 0x08, 0x08, 0x0c,
  0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
  0x0c, 0x08, 0x08, 0x0c, 0x08, 0x0c, 0x0c, 0x08,
  0x20, 0x24, 0x24, 0x20, 0x24, 0x20, 0x20, 0x24,
  0x2c, 0x28, 0x28, 0x2c, 0x28, 0x2c, 0x2c, 0x28,
  0x24, 0x20, 0x20, 0x24, 0x20, 0x24, 0x24, 0x20,
  0x28, 0x2c, 0x2c, 0x28, 0x2c, 0x28, 0x28, 0x2c,
  0x00, 0x04, 0x04, 0x00, 0x04, 0x00, 0x00, 0x04,
  0x0c, 0x08, 0x08, 0x0c, 0x08, 0x0c, 0x0c, 0x08,
  0x04, 0x00, 0x00, 0x04, 0x00, 0x04, 0x04, 0x00,
  0x08, 0x0c, 0x0c, 0x08, 0x0c, 0x08, 0x08, 0x0c,
  0x24, 0x20, 0x20, 0x24, 0x20, 0x24, 0x24, 0x20,
  0x28, 0x2c, 0x2c, 0x28, 0x2c, 0x28, 0x28, 0x2c,
  0x20, 0x24, 0x24, 0x20, 0x24, 0x20, 0x20, 0x24,
  0x2c, 0x28, 0x28, 0x2c, 0x28, 0x2c, 0x2c, 0x28,
  0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
  0x8c, 0x88, 0x88, 0x8c, 0x88, 0x8c, 0x8c, 0x88,
  0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
  0x88, 0x8c, 0x8c, 0x88, 0x8c, 0x88, 0x88, 0x8c,
  0xa4, 0xa0, 0xa0, 0xa4, 0xa0, 0xa4, 0xa4, 0xa0,
  0xa8, 0xac, 0xac, 0xa8, 0xac, 0xa8, 0xa8, 0xac,
  0xa0, 0xa4, 0xa4, 0xa0, 0xa4, 0xa0, 0xa0, 0xa4,
  0xac, 0xa8, 0xa8, 0xac, 0xa8, 0xac, 0xac, 0xa8,
  0x84, 0x80, 0x80, 0x84, 0x80, 0x84, 0x84, 0x80,
  0x88, 0x8c, 0x8c, 0x88, 0x8c, 0x88, 0x88, 0x8c,
  0x80, 0x84, 0x84, 0x80, 0x84, 0x80, 0x80, 0x84,
  0x8c, 0x88, 0x88, 0x8c, 0x88, 0x8c, 0x8c, 0x88,
  0xa0, 0xa4, 0xa4, 0xa0, 0xa4, 0xa0, 0xa0, 0xa4,
  0xac, 0xa8, 0xa8, 0xac, 0xa8, 0xac, 0xac, 0xa8,
  0xa4, 0xa0, 0xa0, 0xa4, 0xa0, 0xa4, 0xa4, 0xa0,
  0xa8, 0xac, 0xac, 0xa8, 0xac, 0xa8, 0xa8, 0xac,
};

const int tstates[0x100][6] = {
  {4},		{4, 3, 3},	{4, 3},		{6},		/* 00-03 */
  {4},		{4},		{4, 3},		{4},		/* 04-07 */
  {4},		{4, 4, 3},	{4, 3},		{6},		/* 08-0b */
  {4},		{4},		{4, 3},		{4},		/* 0c-0f */
  {5, 3, 5},	{4, 3, 3},	{4, 3},		{6},		/* 10-13 */
  {4},		{4},		{4, 3},		{4},		/* 14-17 */
  {4, 3, 5},	{4, 4, 3},	{4, 3},		{6},		/* 18-1b */
  {4},		{4},		{4, 3},		{4},		/* 1c-1f */
  {4, 3, 5},	{4, 3, 3},	{4,3,3,3,3},	{6},		/* 20-23 */
  {4},		{4},		{4, 3},		{4},		/* 24-27 */
  {4, 3, 5},	{4, 4, 3},	{4,3,3,3,3},	{6},		/* 28-2b */
  {4},		{4},		{4, 3},		{4},		/* 2c-2f */
  {4, 3, 5},	{4, 3, 3},	{4, 3, 3, 3},	{6},		/* 30-33 */
  {4, 4, 3},	{4, 4, 3},	{4, 3, 3},	{4},		/* 34-37 */
  {4, 3, 5},	{4, 4, 3},	{4, 3, 3, 3},	{6},		/* 38-3b */
  {4},		{4},		{4, 3},		{4},		/* 3c-3f */
  {4},		{4},		{4},		{4},		/* 40-43 */
  {4},		{4},		{4, 3},		{4},		/* 44-47 */
  {4},		{4},		{4},		{4},		/* 48-4b */
  {4},		{4},		{4, 3},		{4},		/* 4c-4f */
  {4},		{4},		{4},		{4},		/* 50-53 */
  {4},		{4},		{4, 3},		{4},		/* 54-57 */
  {4},		{4},		{4},		{4},		/* 58-5b */
  {4},		{4},		{4, 3},		{4},		/* 5c-5f */
  {4},		{4},		{4},		{4},		/* 60-63 */
  {4},		{4},		{4, 3},		{4},		/* 64-67 */
  {4},		{4},		{4},		{4},		/* 68-6b */
  {4},		{4},		{4, 3},		{4},		/* 6c-6f */
  {4, 3},	{4, 3},		{4, 3},		{4, 3},		/* 70-73 */
  {4, 3},	{4, 3},		{4},		{4, 3},		/* 74-77 */
  {4},		{4},		{4},		{4},		/* 78-7b */
  {4},		{4},		{4, 3},		{4},		/* 7c-7f */
  {4},		{4},		{4},		{4},		/* 80-83 */
  {4},		{4},		{4, 3},		{4},		/* 84-87 */
  {4},		{4},		{4},		{4},		/* 88-8b */
  {4},		{4},		{4, 3},		{4},		/* 8c-8f */
  {4},		{4},		{4},		{4},		/* 90-93 */
  {4},		{4},		{4, 3},		{4},		/* 94-97 */
  {4},		{4},		{4},		{4},		/* 98-9b */
  {4},		{4},		{4, 3},		{4},		/* 9c-9f */
  {4},		{4},		{4},		{4},		/* a0-a3 */
  {4},		{4},		{4, 3},		{4},		/* a4-a7 */
  {4},		{4},		{4},		{4},		/* a8-ab */
  {4},		{4},		{4, 3},		{4},		/* ac-af */
  {4},		{4},		{4},		{4},		/* b0-b3 */
  {4},		{4},		{4, 3},		{4},		/* b4-b7 */
  {4},		{4},		{4},		{4},		/* b8-bb */
  {4},		{4},		{4, 3},		{4},		/* bc-bf */
  {5, 3, 3},	{4, 3, 3},	{4, 3, 3},	{4, 3, 3},	/* c0-c3 */
  {4,3,4,3,3},	{5, 3, 3},	{4, 3},		{5, 3, 3},	/* c4-c7 */
  {5, 3, 3},	{4, 3, 3},	{4, 3, 3},	{4},		/* c8-cb */
  {4,3,4,3,3},	{4,3,4,3,3},	{4, 3},		{5, 3, 3},	/* cc-cf */
  {5, 3, 3},	{4, 3, 3},	{4, 3, 3},	{4, 3, 4},	/* d0-d3 */
  {4,3,4,3,3},	{5, 3, 3},	{4, 3},		{5, 3, 3},	/* d4-d7 */
  {5, 3, 3},	{4},		{4, 3, 3},	{4, 3, 4},	/* d8-db */
  {4,3,4,3,3},	{4},		{4, 3},		{5, 3, 3},	/* dc-df */
  {5, 3, 3},	{4, 3, 3},	{4, 3, 3},	{4,3,4,3,5},	/* e0-e3 */
  {4,3,4,3,3},	{5, 3, 3},	{4, 3},		{5, 3, 3},	/* e4-e7 */
  {5, 3, 3},	{4},		{4, 3, 3},	{4},		/* e8-eb */
  {4,3,4,3,3},	{4},		{4, 3},		{5, 3, 3},	/* ec-ef */
  {5, 3, 3},	{4, 3, 3},	{4, 3, 3},	{4},		/* f0-f3 */
  {4,3,4,3,3},	{5, 3, 3},	{4, 3},		{5, 3, 3},	/* f4-f7 */
  {5, 3, 3},	{6},		{4, 3, 3},	{4},		/* f8-fb */
  {4,3,4,3,3},	{4},		{4, 3},		{5, 3, 3}	/* fc-ff */
};

const int tstates_cb[0x100][4] = {
  {4},		{4},		{4},		{4},		/* 00-03 */
  {4},		{4},		{4, 4, 3},	{4},		/* 04-07 */
  {4},		{4},		{4},		{4},		/* 08-0b */
  {4},		{4},		{4, 4, 3},	{4},		/* 0c-0f */
  {4},		{4},		{4},		{4},		/* 10-13 */
  {4},		{4},		{4, 4, 3},	{4},		/* 14-17 */
  {4},		{4},		{4},		{4},		/* 18-1b */
  {4},		{4},		{4, 4, 3},	{4},		/* 1c-1f */
  {4},		{4},		{4},		{4},		/* 20-23 */
  {4},		{4},		{4, 4, 3},	{4},		/* 24-27 */
  {4},		{4},		{4},		{4},		/* 28-2b */
  {4},		{4},		{4, 4, 3},	{4},		/* 2c-2f */
  {4},		{4},		{4},		{4},		/* 30-33 */
  {4},		{4},		{4, 4, 3},	{4},		/* 34-37 */
  {4},		{4},		{4},		{4},		/* 38-3b */
  {4},		{4},		{4, 4, 3},	{4},		/* 3c-3f */
  {4},		{4},		{4},		{4},		/* 40-43 */
  {4},		{4},		{4, 4},		{4},		/* 44-47 */
  {4},		{4},		{4},		{4},		/* 48-4b */
  {4},		{4},		{4, 4},		{4},		/* 4c-4f */
  {4},		{4},		{4},		{4},		/* 50-53 */
  {4},		{4},		{4, 4},		{4},		/* 54-57 */
  {4},		{4},		{4},		{4},		/* 58-5b */
  {4},		{4},		{4, 4},		{4},		/* 5c-5f */
  {4},		{4},		{4},		{4},		/* 60-63 */
  {4},		{4},		{4, 4},		{4},		/* 64-67 */
  {4},		{4},		{4},		{4},		/* 68-6b */
  {4},		{4},		{4, 4},		{4},		/* 6c-6f */
  {4},		{4},		{4},		{4},		/* 70-73 */
  {4},		{4},		{4, 4},		{4},		/* 74-77 */
  {4},		{4},		{4},		{4},		/* 78-7b */
  {4},		{4},		{4, 4},		{4},		/* 7c-7f */
  {4},		{4},		{4},		{4},		/* 80-83 */
  {4},		{4},		{4, 4, 3},	{4},		/* 84-87 */
  {4},		{4},		{4},		{4},		/* 88-8b */
  {4},		{4},		{4, 4, 3},	{4},		/* 8c-8f */
  {4},		{4},		{4},		{4},		/* 90-93 */
  {4},		{4},		{4, 4, 3},	{4},		/* 94-97 */
  {4},		{4},		{4},		{4},		/* 98-9b */
  {4},		{4},		{4, 4, 3},	{4},		/* 9c-9f */
  {4},		{4},		{4},		{4},		/* a0-a3 */
  {4},		{4},		{4, 4, 3},	{4},		/* a4-a7 */
  {4},		{4},		{4},		{4},		/* a8-ab */
  {4},		{4},		{4, 4, 3},	{4},		/* ac-af */
  {4},		{4},		{4},		{4},		/* b0-b3 */
  {4},		{4},		{4, 4, 3},	{4},		/* b4-b7 */
  {4},		{4},		{4},		{4},		/* b8-bb */
  {4},		{4},		{4, 4, 3},	{4},		/* bc-bf */
  {4},		{4},		{4},		{4},		/* c0-c3 */
  {4},		{4},		{4, 4, 3},	{4},		/* c4-c7 */
  {4},		{4},		{4},		{4},		/* c8-cb */
  {4},		{4},		{4, 4, 3},	{4},		/* cc-cf */
  {4},		{4},		{4},		{4},		/* d0-d3 */
  {4},		{4},		{4, 4, 3},	{4},		/* d4-d7 */
  {4},		{4},		{4},		{4},		/* d8-db */
  {4},		{4},		{4, 4, 3},	{4},		/* dc-df */
  {4},		{4},		{4},		{4},		/* e0-e3 */
  {4},		{4},		{4, 4, 3},	{4},		/* e4-e7 */
  {4},		{4},		{4},		{4},		/* e8-eb */
  {4},		{4},		{4, 4, 3},	{4},		/* ec-ef */
  {4},		{4},		{4},		{4},		/* f0-f3 */
  {4},		{4},		{4, 4, 3},	{4},		/* f4-f7 */
  {4},		{4},		{4},		{4},		/* f8-fb */
  {4},		{4},		{4, 4, 3},	{4},		/* fc-ff */
};

const int tstates_ed[0x100][6] = {
  {4},	{4},	{4},	{4},	{4},	{4},	{4},	{4},	/* 00-07 */
  {4},	{4},	{4},	{4},	{4},	{4},	{4},	{4},	/* 08-0f */
  {4},	{4},	{4},	{4},	{4},	{4},	{4},	{4},	/* 10-17 */
  {4},	{4},	{4},	{4},	{4},	{4},	{4},	{4},	/* 18-1f */
  {4},	{4},	{4},	{4},	{4},	{4},	{4},	{4},	/* 20-27 */
  {4},	{4},	{4},	{4},	{4},	{4},	{4},	{4},	/* 28-2f */
  {4},	{4},	{4},	{4},	{4},	{4},	{4},	{4},	/* 30-37 */
  {4},	{4},	{4},	{4},	{4},	{4},	{4},	{4},	/* 38-3f */

  {4, 4},	{4, 4},		{4, 4, 3},	{4,3,3,3,3},	/* 40-43 */
  {4},		{4, 3, 3},	{4},		{5},		/* 44-47 */
  {4, 4},	{4, 4},		{4, 4, 3},	{4,3,3,3,3},	/* 48-4b */
  {4},		{4, 3, 3},	{4},		{5},		/* 4c-4f */
  {4, 4},	{4, 4},		{4, 4, 3},	{4,3,3,3,3},	/* 50-53 */
  {4},		{4, 3, 3},	{4},		{5},		/* 54-57 */
  {4, 4},	{4, 4},		{4, 4, 3},	{4,3,3,3,3},	/* 58-5b */
  {4},		{4, 3, 3},	{4},		{5},		/* 5c-5f */
  {4, 4},	{4, 4},		{4, 4, 3},	{4,3,3,3,3},	/* 60-63 */
  {4},		{4, 3, 3},	{4},		{4, 3, 4, 3},	/* 64-67 */
  {4, 4},	{4, 4},		{4, 4, 3},	{4,3,3,3,3},	/* 68-6b */
  {4},		{4, 3, 3},	{4},		{4, 3, 4, 3},	/* 6c-6f */
  {4, 4},	{4, 4},		{4, 4, 3},	{4,3,3,3,3},	/* 70-73 */
  {4},		{4, 3, 3},	{4},		{4},		/* 74-77 */
  {4, 4},	{4, 4},		{4, 4, 3},	{4,3,3,3,3},	/* 78-7b */
  {4},		{4, 3, 3},	{4},		{4},		/* 7c-7f */

  {4},	{4},	{4},	{4},	{4},	{4},	{4},	{4},	/* 80-87 */
  {4},	{4},	{4},	{4},	{4},	{4},	{4},	{4},	/* 88-8f */
  {4},	{4},	{4},	{4},	{4},	{4},	{4},	{4},	/* 90-97 */
  {4},	{4},	{4},	{4},	{4},	{4},	{4},	{4},	/* 98-9f */

  {4, 3, 5},	{4, 3, 5},	{5, 4, 3},	{5, 3, 4},	/* a0-a3 */
  {4},		{4},		{4},		{4},		/* a4-a7 */
  {4, 3, 5},	{4, 3, 5},	{5, 4, 3},	{5, 3, 4},	/* a8-ab */
  {4},		{4},		{4},		{4},		/* ac-af */
  {4, 3, 5, 5},	{4, 3, 5, 5},	{5, 4, 3, 5},	{5, 3, 4, 5},	/* b0-b3 */
  {4},		{4},		{4},		{4},		/* b4-b7 */
  {4, 3, 5, 5},	{4, 3, 5, 5},	{5, 4, 3, 5},	{5, 3, 4, 5},	/* b8-bb */
  {4},		{4},		{4},		{4},		/* bc-bf */

  {4},	{4},	{4},	{4},	{4},	{4},	{4},	{4},	/* c0-c7 */
  {4},	{4},	{4},	{4},	{4},	{4},	{4},	{4},	/* c8-cf */
  {4},	{4},	{4},	{4},	{4},	{4},	{4},	{4},	/* d0-d7 */
  {4},	{4},	{4},	{4},	{4},	{4},	{4},	{4},	/* d8-df */
  {4},	{4},	{4},	{4},	{4},	{4},	{4},	{4},	/* e0-e7 */
  {4},	{4},	{4},	{4},	{4},	{4},	{4},	{4},	/* e8-ef */
  {4},	{4},	{4},	{4},	{4},	{4},	{4},	{4},	/* f0-f7 */
  {4},	{4},	{4},	{4},	{4},	{4},	{4},	{4},	/* f8-ff */
};

const int tstates_dd[0x100][6] = {
  {0},		{0},		{0},		{0},		/* 00-03 */
  {0},		{0},		{0},		{0},		/* 04-07 */
  {0},		{4, 4, 3},	{0},		{0},		/* 08-0b */
  {0},	       	{0},		{0},		{0},		/* 0c-0f */
  {0},		{0},		{0},		{0},		/* 10-13 */
  {0},		{0},		{0},		{0},		/* 14-17 */
  {0},		{4, 4, 3},	{0},		{0},		/* 18-1b */
  {0},		{0},		{0},		{0},		/* 1c-1f */
  {0},		{4, 3, 3},	{4,3,3,3,3},	{6},		/* 20-23 */
  {4},		{4},		{4, 3},		{0},		/* 24-27 */
  {0},		{4, 4, 3},	{4,3,3,3,3},	{6},		/* 28-2b */
  {4},		{4},		{4, 3},		{0},		/* 2c-2f */
  {0},		{0},		{0},		{0},		/* 30-33 */
  {4,3,5,4,3},	{4,3,5,4,3},	{4, 3, 5, 3},	{0},		/* 34-37 */
  {0},		{4, 4, 3},	{0},		{0},		/* 38-3b */
  {0},		{0},		{0},		{0},		/* 3c-3f */
  {0},		{0},		{0},		{0},		/* 40-43 */
  {4},		{4},		{4, 3, 5, 3},	{0},		/* 44-47 */
  {0},		{0},		{0},		{0},		/* 48-4b */
  {4},		{4},		{4, 3, 5, 3},	{0},		/* 4c-4f */
  {0},		{0},		{0},		{0},		/* 50-53 */
  {4},		{4},		{4, 3, 5, 3},	{0},		/* 54-57 */
  {0},		{0},		{0},		{0},		/* 58-5b */
  {4},		{4},		{4, 3, 5, 3},	{0},		/* 5c-5f */
  {4},		{4},		{4},		{4},		/* 60-63 */
  {4},		{4},		{4, 3, 5, 3},	{4},		/* 64-67 */
  {4},		{4},		{4},		{4},		/* 68-6b */
  {4},		{4},		{4, 3, 5, 3},	{4},		/* 6c-6f */
  {4, 3, 5, 3},	{4, 3, 5, 3},	{4, 3, 5, 3},	{4, 3, 5, 3},	/* 70-73 */
  {4, 3, 5, 3},	{4, 3, 5, 3},	{0},		{4, 3, 5, 3},	/* 74-77 */
  {0},		{0},		{0},		{0},		/* 78-7b */
  {4},		{4},		{4, 3, 5, 3},	{0},		/* 7c-7f */
  {0},		{0},		{0},		{0},		/* 80-83 */
  {4},		{4},		{4, 3, 5, 3},	{0},		/* 84-87 */
  {0},		{0},		{0},		{0},		/* 88-8b */
  {4},		{4},		{4, 3, 5, 3},	{0},		/* 8c-8f */
  {0},		{0},		{0},		{0},		/* 90-93 */
  {4},		{4},		{4, 3, 5, 3},	{0},		/* 94-97 */
  {0},		{0},		{0},		{0},		/* 98-9b */
  {4},		{4},		{4, 3, 5, 3},	{0},		/* 9c-9f */
  {0},		{0},		{0},		{0},		/* a0-a3 */
  {4},		{4},		{4, 3, 5, 3},	{0},		/* a4-a7 */
  {0},		{0},		{0},		{0},		/* a8-ab */
  {4},		{4},		{4, 3, 5, 3},	{0},		/* ac-af */
  {0},		{0},		{0},		{0},		/* b0-b3 */
  {4},		{4},		{4, 3, 5, 3},	{0},		/* b4-b7 */
  {0},		{0},		{0},		{0},		/* b8-bb */
  {4},		{4},		{4, 3, 5, 3},	{0},		/* bc-bf */
  {0},		{0},		{0},		{0},		/* c0-c3 */
  {0},		{0},		{0},		{0},		/* c4-c7 */
  {0},		{0},		{0},		{4,3,5,4,3},	/* c8-cb */
  {0},		{0},		{0},		{0},		/* cc-cf */
  {0},		{0},		{0},		{0},		/* d0-d3 */
  {0},		{0},		{0},		{0},		/* d4-d7 */
  {0},		{0},		{0},		{0},		/* d8-db */
  {0},		{0},		{0},		{0},		/* dc-df */
  {0},		{4, 3, 3},	{0},		{4,3,4,3,5},	/* e0-e3 */
  {0},		{5, 3, 3},	{0},		{0},		/* e4-e7 */
  {0},		{4},		{0},		{0},		/* e8-eb */
  {0},		{0},		{0},		{0},		/* ec-ef */
  {0},		{0},		{0},		{0},		/* f0-f3 */
  {0},		{0},		{0},		{0},		/* f4-f7 */
  {0},		{6},		{0},		{0},		/* f8-fb */
  {0},		{0},		{0},		{0}		/* fc-ff */
};

const int tstates_intmode2[6] =
  {5, 3, 3, 3, 3};

const char *mnemonics[0x100] = {
	"NOP",	"LD BC,#h",	"LD (BC),A",	"INC BC",
	"INC B",	"DEC B",	"LD B,*h",	"RLCA",
	"EX AF,AF'",	"ADD HL,BC",	"LD A,(BC)",	"DEC BC",
	"INC C",	"DEC C",	"LD C,*h",	"RRCA",
	"DJNZ @h",	"LD DE,#h",	"LD (DE),A",	"INC DE",
	"INC D",	"DEC D",	"LD D,*h",	"RLA",
	"JR @h",	"ADD HL,DE",	"LD A,(DE)",	"DEC DE",
	"INC E",	"DEC E",	"LD E,*h",	"RRA",
	"JR NZ,@h",	"LD HL,#h",	"LD (#h),HL",	"INC HL",
	"INC H",	"DEC H",	"LD H,*h",	"DAA",
	"JR Z,@h",	"ADD HL,HL",	"LD HL,(#h)",	"DEC HL",
	"INC L",	"DEC L",	"LD L,*h",	"CPL",
	"JR NC,@h",	"LD SP,#h",	"LD (#h),A",	"INC SP",
	"INC (HL)",	"DEC (HL)",	"LD (HL),*h",	"SCF",
	"JR C,@h",	"ADD HL,SP",	"LD A,(#h)",	"DEC SP",
	"INC A",	"DEC A",	"LD A,*h",	"CCF",
	"LD B,B",	"LD B,C",	"LD B,D",	"LD B,E",
	"LD B,H",	"LD B,L",	"LD B,(HL)",	"LD B,A",
	"LD C,B",	"LD C,C",	"LD C,D",	"LD C,E",
	"LD C,H",	"LD C,L",	"LD C,(HL)",	"LD C,A",
	"LD D,B",	"LD D,C",	"LD D,D",	"LD D,E",
	"LD D,H",	"LD D,L",	"LD D,(HL)",	"LD D,A",
	"LD E,B",	"LD E,C",	"LD E,D",	"LD E,E",
	"LD E,H",	"LD E,L",	"LD E,(HL)",	"LD E,A",
	"LD H,B",	"LD H,C",	"LD H,D",	"LD H,E",
	"LD H,H",	"LD H,L",	"LD H,(HL)",	"LD H,A",
	"LD L,B",	"LD L,C",	"LD L,D",	"LD L,E",
	"LD L,H",	"LD L,L",	"LD L,(HL)",	"LD L,A",
	"LD (HL),B",	"LD (HL),C",	"LD (HL),D",	"LD (HL),E",
	"LD (HL),H",	"LD (HL),L",	"HALT",		"LD (HL),A",
	"LD A,B",	"LD A,C",	"LD A,D",	"LD A,E",
	"LD A,H",	"LD A,L",	"LD A,(HL)",	"LD A,A",
	"ADD A,B",	"ADD A,C",	"ADD A,D",	"ADD A,E",
	"ADD A,H",	"ADD A,L",	"ADD A,(HL)",	"ADD A,A",
	"ADC A,B",	"ADC A,C",	"ADC A,D",	"ADC A,E",
	"ADC A,H",	"ADC A,L",	"ADC A,(HL)",	"ADC A,A",
	"SUB B",	"SUB C",	"SUB D",	"SUB E",
	"SUB H",	"SUB L",	"SUB (HL)",	"SUB A",
	"SBC A,B",	"SBC A,C",	"SBC A,D",	"SBC A,E",
	"SBC A,H",	"SBC A,L",	"SBC A,(HL)",	"SBC A,A",
	"AND B",	"AND C",	"AND D",	"AND E",
	"AND H",	"AND L",	"AND (HL)",	"AND A",
	"XOR B",	"XOR C",	"XOR D",	"XOR E",
	"XOR H",	"XOR L",	"XOR (HL)",	"XOR A",
	"OR B",		"OR C",		"OR D",		"OR E",
	"OR H",		"OR L",		"OR (HL)",	"OR A",
	"CP B",		"CP C",		"CP D",		"CP E",
	"CP H",		"CP L",		"CP (HL)",	"CP A",
	"RET NZ",	"POP BC",	"JP NZ,#h",	"JP #h",
	"CALL NZ,#h",	"PUSH BC",	"ADD A,*h",	"RST 00h",
	"RET Z",	"RET",		"JP Z,#h",	"CB prefix",
	"CALL Z,#h",	"CALL #h",	"ADC A,*h",	"RST 08h",
	"RET NC",	"POP DE",	"JP NC,#h",	"OUT (*h),A",
	"CALL NC,#h",	"PUSH DE",	"SUB *h",	"RST 10h",
	"RET C",	"EXX",		"JP C,#h",	"IN A,(*h)",
	"CALL C,#h",	"DD prefix",	"SBC A,*h",	"RST 18h",
	"RET PO",	"POP HL",	"JP PO,#h",	"EX (SP),HL",
	"CALL PO,#h",	"PUSH HL",	"AND *h",	"RST 20h",
	"RET PE",	"JP (HL)",	"JP PE,#h",	"EX DE,HL",
	"CALL PE,#h",	"ED prefix",	"XOR *h",	"RST 28h",
	"RET P",	"POP AF",	"JP P,#h",	"DI",
	"CALL P,#h",	"PUSH AF",	"OR *h",	"RST 30h",
	"RET M",	"LD SP,HL",	"JP M,#h",	"EI",
	"CALL M,#h",	"FD prefix",	"CP *h",	"RST 38h"
};

const char *mnemonics_cb[0x100] = {
	"RLC B",	"RLC C",	"RLC D",	"RLC E",
	"RLC H",	"RLC L",	"RLC (HL)",	"RLC A",
	"RRC B",	"RRC C",	"RRC D",	"RRC E",
	"RRC H",	"RRC L",	"RRC (HL)",	"RRC A",
	"RL B",		"RL C",		"RL D",		"RL E",
	"RL H",		"RL L",		"RL (HL)",	"RL A",
	"RR B",		"RR C",		"RR D",		"RR E",
	"RR H",		"RR L",		"RR (HL)",	"RR A",
	"SLA B",	"SLA C",	"SLA D",	"SLA E",
	"SLA H",	"SLA L",	"SLA (HL)",	"SLA A",
	"SRA B",	"SRA C",	"SRA D",	"SRA E",
	"SRA H",	"SRA L",	"SRA (HL)",	"SRA A",
	"SLL B",	"SLL C",	"SLL D",	"SLL E",
	"SLL H",	"SLL L",	"SLL (HL)",	"SLL A",
	"SRL B",	"SRL C",	"SRL D",	"SRL E",
	"SRL H",	"SRL L",	"SRL (HL)",	"SRL A",
	"BIT 0,B",	"BIT 0,C",	"BIT 0,D",	"BIT 0,E",
	"BIT 0,H",	"BIT 0,L",	"BIT 0,(HL)",	"BIT 0,A",
	"BIT 1,B",	"BIT 1,C",	"BIT 1,D",	"BIT 1,E",
	"BIT 1,H",	"BIT 1,L",	"BIT 1,(HL)",	"BIT 1,A",
	"BIT 2,B",	"BIT 2,C",	"BIT 2,D",	"BIT 2,E",
	"BIT 2,H",	"BIT 2,L",	"BIT 2,(HL)",	"BIT 2,A",
	"BIT 3,B",	"BIT 3,C",	"BIT 3,D",	"BIT 3,E",
	"BIT 3,H",	"BIT 3,L",	"BIT 3,(HL)",	"BIT 3,A",
	"BIT 4,B",	"BIT 4,C",	"BIT 4,D",	"BIT 4,E",
	"BIT 4,H",	"BIT 4,L",	"BIT 4,(HL)",	"BIT 4,A",
	"BIT 5,B",	"BIT 5,C",	"BIT 5,D",	"BIT 5,E",
	"BIT 5,H",	"BIT 5,L",	"BIT 5,(HL)",	"BIT 5,A",
	"BIT 6,B",	"BIT 6,C",	"BIT 6,D",	"BIT 6,E",
	"BIT 6,H",	"BIT 6,L",	"BIT 6,(HL)",	"BIT 6,A",
	"BIT 7,B",	"BIT 7,C",	"BIT 7,D",	"BIT 7,E",
	"BIT 7,H",	"BIT 7,L",	"BIT 7,(HL)",	"BIT 7,A",
	"RES 0,B",	"RES 0,C",	"RES 0,D",	"RES 0,E",
	"RES 0,H",	"RES 0,L",	"RES 0,(HL)",	"RES 0,A",
	"RES 1,B",	"RES 1,C",	"RES 1,D",	"RES 1,E",
	"RES 1,H",	"RES 1,L",	"RES 1,(HL)",	"RES 1,A",
	"RES 2,B",	"RES 2,C",	"RES 2,D",	"RES 2,E",
	"RES 2,H",	"RES 2,L",	"RES 2,(HL)",	"RES 2,A",
	"RES 3,B",	"RES 3,C",	"RES 3,D",	"RES 3,E",
	"RES 3,H",	"RES 3,L",	"RES 3,(HL)",	"RES 3,A",
	"RES 4,B",	"RES 4,C",	"RES 4,D",	"RES 4,E",
	"RES 4,H",	"RES 4,L",	"RES 4,(HL)",	"RES 4,A",
	"RES 5,B",	"RES 5,C",	"RES 5,D",	"RES 5,E",
	"RES 5,H",	"RES 5,L",	"RES 5,(HL)",	"RES 5,A",
	"RES 6,B",	"RES 6,C",	"RES 6,D",	"RES 6,E",
	"RES 6,H",	"RES 6,L",	"RES 6,(HL)",	"RES 6,A",
	"RES 7,B",	"RES 7,C",	"RES 7,D",	"RES 7,E",
	"RES 7,H",	"RES 7,L",	"RES 7,(HL)",	"RES 7,A",
	"SET 0,B",	"SET 0,C",	"SET 0,D",	"SET 0,E",
	"SET 0,H",	"SET 0,L",	"SET 0,(HL)",	"SET 0,A",
	"SET 1,B",	"SET 1,C",	"SET 1,D",	"SET 1,E",
	"SET 1,H",	"SET 1,L",	"SET 1,(HL)",	"SET 1,A",
	"SET 2,B",	"SET 2,C",	"SET 2,D",	"SET 2,E",
	"SET 2,H",	"SET 2,L",	"SET 2,(HL)",	"SET 2,A",
	"SET 3,B",	"SET 3,C",	"SET 3,D",	"SET 3,E",
	"SET 3,H",	"SET 3,L",	"SET 3,(HL)",	"SET 3,A",
	"SET 4,B",	"SET 4,C",	"SET 4,D",	"SET 4,E",
	"SET 4,H",	"SET 4,L",	"SET 4,(HL)",	"SET 4,A",
	"SET 5,B",	"SET 5,C",	"SET 5,D",	"SET 5,E",
	"SET 5,H",	"SET 5,L",	"SET 5,(HL)",	"SET 5,A",
	"SET 6,B",	"SET 6,C",	"SET 6,D",	"SET 6,E",
	"SET 6,H",	"SET 6,L",	"SET 6,(HL)",	"SET 6,A",
	"SET 7,B",	"SET 7,C",	"SET 7,D",	"SET 7,E",
	"SET 7,H",	"SET 7,L",	"SET 7,(HL)",	"SET 7,A"
};

const char *mnemonics_ed[0x100] = {
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"IN B,(C)",	"OUT (C),B",	"SBC HL,BC",	"LD (#h),BC",
	"NEG",		"RETN",		"IM 0",		"LD I,A",
	"IN C,(C)",	"OUT (C),C",	"ADC HL,BC",	"LD BC,(#h)",
	"NEG",		"RETI",		"IM 0",		"LD R,A",
	"IN D,(C)",	"OUT (C),D",	"SBC HL,DE",	"LD (#h),DE",
	"NEG",		"RETN",		"IM 1",		"LD A,I",
	"IN E,(C)",	"OUT (C),E",	"ADC HL,DE",	"LD DE,(#h)",
	"NEG",		"RETN",		"IM 2",		"LD A,R",
	"IN H,(C)",	"OUT (C),H",	"SBC HL,HL",	"LD (#h),HL",
	"NEG",		"RETN",		"IM 0",		"RRD",
	"IN L,(C)",	"OUT (C),L",	"ADC HL,HL",	"LD HL,(#h)",
	"NEG",		"RETN",		"IM 0",		"RLD",
	"IN F,(C)",	"OUT (C),0",	"SBC HL,SP",	"LD (#h),SP",
	"NEG",		"RETN",		"IM 1",		"NOP",
	"IN A,(C)",	"OUT (C),A",	"ADC HL,SP",	"LD SP,(#h)",
	"NEG",		"RETN",		"IM 2",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"LDI",		"CPI",		"INI",		"OUTI",
	"NOP",		"NOP",		"NOP",		"NOP",
	"LDD",		"CPD",		"IND",		"OUTD",
	"NOP",		"NOP",		"NOP",		"NOP",
	"LDIR",		"CPIR",		"INIR",		"OTIR",
	"NOP",		"NOP",		"NOP",		"NOP",
	"LDDR",		"CPDR",		"INDR",		"OTDR",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP",
	"NOP",		"NOP",		"NOP",		"NOP"
};

const char *mnemonics_dd[0x100] = {
	"NOP",		"LD BC,#h",	"LD (BC),A",	"INC BC",
	"INC B",	"DEC B",	"LD B,*h",	"RLCA",
	"EX AF,AF'",	"ADD I%,BC",	"LD A,(BC)",	"DEC BC",
	"INC C",	"DEC C",	"LD C,*h",	"RRCA",
	"DJNZ @h",	"LD DE,#h",	"LD (DE),A",	"INC DE",
	"INC D",	"DEC D",	"LD D,*h",	"RLA",
	"JR @h",	"ADD I%,DE",	"LD A,(DE)",	"DEC DE",
	"INC E",	"DEC E",	"LD E,*h",	"RRA",
	"JR NZ,@h",	"LD I%,#h",	"LD (#h),I%",	"INC I%",
	"INC I%h",	"DEC I%h",	"LD I%h,*h",	"DAA",
	"JR Z,@h",	"ADD I%,I%",	"LD I%,(#h)",	"DEC I%",
	"INC I%l",	"DEC I%l",	"LD I%l,*h",	"CPL",
	"JR NC,@h",	"LD SP,#h",	"LD (#h),A",	"INC SP",
	"INC (I%^h)",	"DEC (I%^h)",	"LD (I%^h),*h",	"SCF",
	"JR C,@h",	"ADD I%,SP",	"LD A,(#h)",	"DEC SP",
	"INC A",	"DEC A",	"LD A,*h",	"CCF",
	"LD B,B",	"LD B,C",	"LD B,D",	"LD B,E",
	"LD B,I%h",	"LD B,I%l",	"LD B,(I%^h)",	"LD B,A",
	"LD C,B",	"LD C,C",	"LD C,D",	"LD C,E",
	"LD C,I%h",	"LD C,I%l",	"LD C,(I%^h)",	"LD C,A",
	"LD D,B",	"LD D,C",	"LD D,D",	"LD D,E",
	"LD D,I%h",	"LD D,I%l",	"LD D,(I%^h)",	"LD D,A",
	"LD E,B",	"LD E,C",	"LD E,D",	"LD E,E",
	"LD E,I%h",	"LD E,I%l",	"LD E,(I%^h)",	"LD E,A",
	"LD I%h,B",	"LD I%h,C",	"LD I%h,D",	"LD I%h,E",
	"LD I%h,I%h",	"LD I%h,I%l",	"LD H,(I%^h)",	"LD I%h,A",
	"LD I%l,B",	"LD I%l,C",	"LD I%l,D",	"LD I%l,E",
	"LD I%l,I%h",	"LD I%l,I%l",	"LD L,(I%^h)",	"LD I%l,A",
	"LD (I%^h),B",	"LD (I%^h),C",	"LD (I%^h),D",	"LD (I%^h),E",
	"LD (I%^h),H",	"LD (I%^h),L",	"HALT",		"LD (I%^h),A",
	"LD A,B",	"LD A,C",	"LD A,D",	"LD A,E",
	"LD A,I%h",	"LD A,I%l",	"LD A,(I%^h)",	"LD A,A",
	"ADD A,B",	"ADD A,C",	"ADD A,D",	"ADD A,E",
	"ADD A,I%h",	"ADD A,I%l",	"ADD A,(I%^h)",	"ADD A,A",
	"ADC A,B",	"ADC A,C",	"ADC A,D",	"ADC A,E",
	"ADC A,I%h",	"ADC A,I%l",	"ADC A,(I%^h)",	"ADC A,A",
	"SUB B",	"SUB C",	"SUB D",	"SUB E",
	"SUB I%h",	"SUB I%l",	"SUB (I%^h)",	"SUB A",
	"SBC A,B",	"SBC A,C",	"SBC A,D",	"SBC A,E",
	"SBC A,I%h",	"SBC A,I%l",	"SBC A,(I%^h)",	"SBC A,A",
	"AND B",	"AND C",	"AND D",	"AND E",
	"AND I%h",	"AND I%l",	"AND (I%^h)",	"AND A",
	"XOR B",	"XOR C",	"XOR D",	"XOR E",
	"XOR I%h",	"XOR I%l",	"XOR (I%^h)",	"XOR A",
	"OR B",		"OR C",		"OR D",		"OR E",
	"OR I%h",	"OR I%l",	"OR (I%^h)",	"OR A",
	"CP B",		"CP C",		"CP D",		"CP E",
	"CP I%h",	"CP I%l",	"CP (I%^h)",	"CP A",
	"RET NZ",	"POP BC",	"JP NZ,#h",	"JP #h",
	"CALL NZ,#h",	"PUSH BC",	"ADD A,#h",	"RST 00h",
	"RET Z",	"RET",		"JP Z,#h",	"CB prefix",
	"CALL Z,#h",	"CALL #h",	"ADC A,*h",	"RST 08h",
	"RET NC",	"POP DE",	"JP NC,#h",	"OUT (*h),A",
	"CALL NC,#h",	"PUSH DE",	"SUB *h",	"RST 10h",
	"RET C",	"EXX",		"JP C,#h",	"IN A,(*h)",
	"CALL C,#h",	"DD prefix",	"SBC A,*h",	"RST 18h",
	"RET PO",	"POP I%",	"JP PO,#h",	"EX (SP),I%",
	"CALL PO,#h",	"PUSH I%",	"AND *h",	"RST 20h",
	"RET PE",	"JP (HL)",	"JP PE,#h",	"EX DE,HL",
	"CALL PE,#h",	"ED prefix",	"XOR *h",	"RST 28h",
	"RET P",	"POP AF",	"JP P,#h",	"DI",
	"CALL P,#h",	"PUSH AF",	"OR *h",	"RST 30h",
	"RET M",	"LD SP,I%",	"JP M,#h",	"EI",
	"CALL M,#h",	"FD prefix",	"CP *h",	"RST 38h"
};

const char *mnemonics_ddcb[0x100] = {
	"RLC (I%^h),B",	"RLC (I%^h),C",	"RLC (I%^h),D",	"RLC (I%^h),E",
	"RLC (I%^h),H",	"RLC (I%^h),L",	"RLC (I%^h)",	"RLC (I%^h),A",
	"RRC (I%^h),B",	"RRC (I%^h),C",	"RRC (I%^h),D",	"RRC (I%^h),E",
	"RRC (I%^h),H",	"RRC (I%^h),L",	"RRC (I%^h)",	"RRC (I%^h),A",
	"RL (I%^h),B",	"RL (I%^h),C",	"RL (I%^h),D",	"RL (I%^h),E",
	"RL (I%^h),H",	"RL (I%^h),L",	"RL (I%^h)",	"RL (I%^h),A",
	"RR (I%^h),B",	"RR (I%^h),C",	"RR (I%^h),D",	"RR (I%^h),E",
	"RR (I%^h),H",	"RR (I%^h),L",	"RR (I%^h)",	"RR (I%^h),A",
	"SLA (I%^h),B",	"SLA (I%^h),C",	"SLA (I%^h),D",	"SLA (I%^h),E",
	"SLA (I%^h),H",	"SLA (I%^h),L",	"SLA (I%^h)",	"SLA (I%^h),A",
	"SRA (I%^h),B",	"SRA (I%^h),C",	"SRA (I%^h),D",	"SRA (I%^h),E",
	"SRA (I%^h),H",	"SRA (I%^h),L",	"SRA (I%^h)",	"SRA (I%^h),A",
	"SLL (I%^h),B",	"SLL (I%^h),C",	"SLL (I%^h),D",	"SLL (I%^h),E",
	"SLL (I%^h),H",	"SLL (I%^h),L",	"SLL (I%^h)",	"SLL (I%^h),A",
	"SRL (I%^h),B",	"SRL (I%^h),C",	"SRL (I%^h),D",	"SRL (I%^h),E",
	"SRL (I%^h),H",	"SRL (I%^h),L",	"SRL (I%^h)",	"SRL (I%^h),A",
	"BIT 0,(I%^h)",	"BIT 0,(I%^h)",	"BIT 0,(I%^h)",	"BIT 0,(I%^h)",
	"BIT 0,(I%^h)",	"BIT 0,(I%^h)",	"BIT 0,(I%^h)",	"BIT 0,(I%^h)",
	"BIT 1,(I%^h)",	"BIT 1,(I%^h)",	"BIT 1,(I%^h)",	"BIT 1,(I%^h)",
	"BIT 1,(I%^h)",	"BIT 1,(I%^h)",	"BIT 1,(I%^h)",	"BIT 1,(I%^h)",
	"BIT 2,(I%^h)",	"BIT 2,(I%^h)",	"BIT 2,(I%^h)",	"BIT 2,(I%^h)",
	"BIT 2,(I%^h)",	"BIT 2,(I%^h)",	"BIT 2,(I%^h)",	"BIT 2,(I%^h)",
	"BIT 3,(I%^h)",	"BIT 3,(I%^h)",	"BIT 3,(I%^h)",	"BIT 3,(I%^h)",
	"BIT 3,(I%^h)",	"BIT 3,(I%^h)",	"BIT 3,(I%^h)",	"BIT 3,(I%^h)",
	"BIT 4,(I%^h)",	"BIT 4,(I%^h)",	"BIT 4,(I%^h)",	"BIT 4,(I%^h)",
	"BIT 4,(I%^h)",	"BIT 4,(I%^h)",	"BIT 4,(I%^h)",	"BIT 4,(I%^h)",
	"BIT 5,(I%^h)",	"BIT 5,(I%^h)",	"BIT 5,(I%^h)",	"BIT 5,(I%^h)",
	"BIT 5,(I%^h)",	"BIT 5,(I%^h)",	"BIT 5,(I%^h)",	"BIT 5,(I%^h)",
	"BIT 6,(I%^h)",	"BIT 6,(I%^h)",	"BIT 6,(I%^h)",	"BIT 6,(I%^h)",
	"BIT 6,(I%^h)",	"BIT 6,(I%^h)",	"BIT 6,(I%^h)",	"BIT 6,(I%^h)",
	"BIT 7,(I%^h)",	"BIT 7,(I%^h)",	"BIT 7,(I%^h)",	"BIT 7,(I%^h)",
	"BIT 7,(I%^h)",	"BIT 7,(I%^h)",	"BIT 7,(I%^h)",	"BIT 7,(I%^h)",
	"RES 0,(I%^h),B","RES 0,(I%^h),C","RES 0,(I%^h),D","RES 0,(I%^h),E",
	"RES 0,(I%^h),H","RES 0,(I%^h),L","RES 0,(I%^h)","RES 0,(I%^h),A",
	"RES 1,(I%^h),B","RES 1,(I%^h),C","RES 1,(I%^h),D","RES 1,(I%^h),E",
	"RES 1,(I%^h),H","RES 1,(I%^h),L","RES 1,(I%^h)","RES 1,(I%^h),A",
	"RES 2,(I%^h),B","RES 2,(I%^h),C","RES 2,(I%^h),D","RES 2,(I%^h),E",
	"RES 2,(I%^h),H","RES 2,(I%^h),L","RES 2,(I%^h)","RES 2,(I%^h),A",
	"RES 3,(I%^h),B","RES 3,(I%^h),C","RES 3,(I%^h),D","RES 3,(I%^h),E",
	"RES 3,(I%^h),H","RES 3,(I%^h),L","RES 3,(I%^h)","RES 3,(I%^h),A",
	"RES 4,(I%^h),B","RES 4,(I%^h),C","RES 4,(I%^h),D","RES 4,(I%^h),E",
	"RES 4,(I%^h),H","RES 4,(I%^h),L","RES 4,(I%^h)","RES 4,(I%^h),A",
	"RES 5,(I%^h),B","RES 5,(I%^h),C","RES 5,(I%^h),D","RES 5,(I%^h),E",
	"RES 5,(I%^h),H","RES 5,(I%^h),L","RES 5,(I%^h)","RES 5,(I%^h),A",
	"RES 6,(I%^h),B","RES 6,(I%^h),C","RES 6,(I%^h),D","RES 6,(I%^h),E",
	"RES 6,(I%^h),H","RES 6,(I%^h),L","RES 6,(I%^h)","RES 6,(I%^h),A",
	"RES 7,(I%^h),B","RES 7,(I%^h),C","RES 7,(I%^h),D","RES 7,(I%^h),E",
	"RES 7,(I%^h),H","RES 7,(I%^h),L","RES 7,(I%^h)","RES 7,(I%^h),A",
	"SET 0,(I%^h),B","SET 0,(I%^h),C","SET 0,(I%^h),D","SET 0,(I%^h),E",
	"SET 0,(I%^h),H","SET 0,(I%^h),L","SET 0,(I%^h)","SET 0,(I%^h),A",
	"SET 1,(I%^h),B","SET 1,(I%^h),C","SET 1,(I%^h),D","SET 1,(I%^h),E",
	"SET 1,(I%^h),H","SET 1,(I%^h),L","SET 1,(I%^h)","SET 1,(I%^h),A",
	"SET 2,(I%^h),B","SET 2,(I%^h),C","SET 2,(I%^h),D","SET 2,(I%^h),E",
	"SET 2,(I%^h),H","SET 2,(I%^h),L","SET 2,(I%^h)","SET 2,(I%^h),A",
	"SET 3,(I%^h),B","SET 3,(I%^h),C","SET 3,(I%^h),D","SET 3,(I%^h),E",
	"SET 3,(I%^h),H","SET 3,(I%^h),L","SET 3,(I%^h)","SET 3,(I%^h),A",
	"SET 4,(I%^h),B","SET 4,(I%^h),C","SET 4,(I%^h),D","SET 4,(I%^h),E",
	"SET 4,(I%^h),H","SET 4,(I%^h),L","SET 4,(I%^h)","SET 4,(I%^h),A",
	"SET 5,(I%^h),B","SET 5,(I%^h),C","SET 5,(I%^h),D","SET 5,(I%^h),E",
	"SET 5,(I%^h),H","SET 5,(I%^h),L","SET 5,(I%^h)","SET 5,(I%^h),A",
	"SET 6,(I%^h),B","SET 6,(I%^h),C","SET 6,(I%^h),D","SET 6,(I%^h),E",
	"SET 6,(I%^h),H","SET 6,(I%^h),L","SET 6,(I%^h)","SET 6,(I%^h),A",
	"SET 7,(I%^h),B","SET 7,(I%^h),C","SET 7,(I%^h),D","SET 7,(I%^h),E",
	"SET 7,(I%^h),H","SET 7,(I%^h),L","SET 7,(I%^h)","SET 7,(I%^h),A",
};

static byte fetch(z80 *pcpu);
static void exec_code(z80 *pcpu);
static int search_code(const char **mnem, char ixiy, char *input, byte *data, word address);
static int upperhex(int value, int mask);

static inline void inc(z80 *pcpu, byte *var8);
static inline void dec(z80 *pcpu, byte *var8);
static inline byte mreg(z80 *pcpu, void (*operation)(), word address, int offset);
static inline void ldrn(z80 *pcpu, byte *var8);
static inline void ldixr(z80 *pcpu, byte val8);
static inline void ldiyr(z80 *pcpu, byte val8);
static inline void ldrix(z80 *pcpu, byte *var8);
static inline void ldriy(z80 *pcpu, byte *var8);
static inline void addw(z80 *pcpu, byte *high, byte *low, word val16);
static inline void incw(byte *high, byte *low);
static inline void decw(byte *high, byte *low);
static inline void jr(z80 *pcpu, byte val8);
static inline void jrcnd(z80 *pcpu, byte flag, byte condition);
static inline void ldrhl(z80 *pcpu, byte *var8);
static inline void ldhlr(z80 *pcpu, byte val8);
static inline void add(z80 *pcpu, byte val8);
static inline void adc(z80 *pcpu, byte val8);
static inline void sub(z80 *pcpu, byte val8);
static inline void sbc(z80 *pcpu, byte val8);
static inline void and(z80 *pcpu, byte val8);
static inline void xor(z80 *pcpu, byte val8);
static inline void or(z80 *pcpu, byte val8);
static inline void cp(z80 *pcpu, byte val8);
static inline void retcnd(z80 *pcpu, byte flag, byte condition, int offset);
static inline void push(z80 *pcpu, byte high, byte low);
static inline void pushixiy(z80 *pcpu, byte high, byte low);
static inline void pop(z80 *pcpu, byte *high, byte *low);
static inline void popixiy(z80 *pcpu, byte *high, byte *low);
static inline void jpcnd(z80 *pcpu, byte flag, byte condition);
static inline void callcnd(z80 *pcpu, byte flag, byte condition);
static inline void rst(z80 *pcpu, word address);
static inline void sbchl(z80 *pcpu, word val16);
static inline void adchl(z80 *pcpu, word val16);
static inline void ldi(z80 *pcpu);
static inline void ldd(z80 *pcpu);
static inline void block_copy(z80 *pcpu);
static inline void block_search(z80 *pcpu);
static inline void cpi(z80 *pcpu);
static inline void cpd(z80 *pcpu);
static inline void repbc(z80 *pcpu);
static inline void repcp(z80 *pcpu);
static inline void repb(z80 *pcpu);
static inline void ini(z80 *pcpu);
static inline void ind(z80 *pcpu);
static inline byte block_input(z80 *pcpu);
static inline void outi(z80 *pcpu);
static inline void outd(z80 *pcpu);
static inline byte block_output(z80 *pcpu);
static inline void rlc(z80 *pcpu, byte *var8);
static inline void rrc(z80 *pcpu, byte *var8);
static inline void rl(z80 *pcpu, byte *var8);
static inline void rr(z80 *pcpu, byte *var8);
static inline void sla(z80 *pcpu, byte *var8);
static inline void sra(z80 *pcpu, byte *var8);
static inline void sll(z80 *pcpu, byte *var8);
static inline void srl(z80 *pcpu, byte *var8);
static inline void bit(z80 *pcpu, int n, byte val8);
static inline void bitm(z80 *pcpu, int n);
static inline byte resm(z80 *pcpu, int n, word address, int offset);
static inline byte setm(z80 *pcpu, int n, word address, int offset);
static inline void bitixy(z80 *pcpu, int n, word address);
static inline void intmode2(z80 *pcpu, word address);

/*
 * initialize CPU
 */
void z80_reset(z80 *pcpu)
{
  memset(pcpu, 0, sizeof(*pcpu));
  pcpu->a = pcpu->f = 0xff;
  pcpu->b = pcpu->c = 0xff;
  pcpu->d = pcpu->e = 0xff;
  pcpu->h = pcpu->l = 0xff;

  pcpu->alt_a = pcpu->alt_f = 0xff;
  pcpu->alt_b = pcpu->alt_c = 0xff;
  pcpu->alt_d = pcpu->alt_e = 0xff;
  pcpu->alt_h = pcpu->alt_l = 0xff;

  pcpu->ixh = pcpu->ixl = 0xff;
  pcpu->iyh = pcpu->iyl = 0xff;

  pcpu->i = 0;
  pcpu->r = 0;
  pcpu->sp = 0;
  pcpu->pc = 0;

  pcpu->iff1 = 0;
  pcpu->iff2 = 0;
  pcpu->im = 0;
  pcpu->ei = 0;
  pcpu->m1 = 0;
  pcpu->halt = 0;

  pcpu->intmode2 = 0;
  pcpu->mcycle = 0;
}


/*
 * Z80 main routine
 */
void z80_main(z80 *pcpu)
{
  pcpu->cycles = 0;

  if (pcpu->nmi) {
    pcpu->cycles = tstates[0xff][pcpu->mcycle];	/* equals to RST */
    pcpu->mcycle++;
    rst(pcpu, 0x66);
    if (pcpu->mcycle == 3) {
      pcpu->mcycle = 0;
      pcpu->nmi = 0;
    }
    return;
  } else if (pcpu->intmode2) {
    pcpu->cycles += tstates_intmode2[pcpu->mcycle];
    pcpu->mcycle++;
    intmode2(pcpu, 0);
    if (pcpu->mcycle == 5) {
      pcpu->mcycle = 0;
      pcpu->intmode2 = 0;
    }
    return;
  }

  if (pcpu->mcycle == 0) {
    if (pcpu->ei && pcpu->op1 != 0xfb) {	/* after EI */
      pcpu->ei = 0;
    }
    pcpu->op1 = fetch(pcpu);
  }

  exec_code(pcpu);

  switch (pcpu->op1) {
  case 0xcb:
    if (pcpu->mcycle > 1 && tstates_cb[pcpu->op2][pcpu->mcycle - 1] == 0) {
      pcpu->mcycle = 0;
    }
    break;
  case 0xed:
    if (pcpu->mcycle > 1 && tstates_ed[pcpu->op2][pcpu->mcycle - 1] == 0) {
      pcpu->mcycle = 0;
    }
    break;
  case 0xdd:
  case 0xfd:
    if (pcpu->mcycle > 1 && tstates_dd[pcpu->op2][pcpu->mcycle - 1] == 0) {
      pcpu->mcycle = 0;
    }
    break;
  default:
    if (tstates[pcpu->op1][pcpu->mcycle] == 0) {
      pcpu->mcycle = 0;
    }
    break;
  }
}

/*
 * nonmaskable interrupt
 */
void z80_nmi(z80 *pcpu)
{
  if (pcpu->mcycle) {
    return;
  }

  if (pcpu->halt) {
    pcpu->halt = 0;
    pcpu->pc++;
  }
  pcpu->iff1 = 0;
  pcpu->nmi = 1;
  pcpu->cycles = tstates[0xff][0];			/* equals to RST */
  fetch(pcpu);
  pcpu->mcycle = 1;
}

/*
 * check if interrupt accetable
 */
int z80_intack(z80 *pcpu)
{
  if (pcpu->nmi || pcpu->iff1 == 0 || pcpu->ei || pcpu->mcycle) {
    return 0;
  } else {
    return 1;
  }
}

/*
 * maskable interrupt
 */
void z80_int(z80 *pcpu, byte vector)
{
  word address;

  if (pcpu->halt) {
    pcpu->halt = 0;
    pcpu->pc++;
  }
  pcpu->iff1 = 0;
  pcpu->iff2 = 0;
  pcpu->r = (pcpu->r & 0x80) + ((pcpu->r + 1) & 0x7f);
  pcpu->mcycle = 1;
  pcpu->cycles = 2;		/* wait */

  switch (pcpu->im) {
  case 0:
    pcpu->op1 = vector;		/* rst ** */
    pcpu->cycles += tstates[pcpu->op1][0];
    exec_code(pcpu);
    break;

  case 1:
    pcpu->op1 = 0xff;		/* rst 38h */
    pcpu->cycles += tstates[pcpu->op1][0];
    exec_code(pcpu);
    break;
      
  case 2:
    pcpu->intmode2 = 1;
    address = vector + (pcpu->i << 8);
    pcpu->cycles += tstates_intmode2[0];
    intmode2(pcpu, address);
    break;
  }
}

/*
 * disassemble
 */
int z80_dasm(char *disasm, word address)
{
  int i;
  int bytes;
  byte op, prefix;
  char *pos;

  disasm[0] = '\0';
  op = z80_read(address);

  switch (op) {
  case 0xcb:
    strcpy(disasm, mnemonics_cb[z80_read(++address)]);
    bytes = 2;
    break;

  case 0xed:
    strcpy(disasm, mnemonics_ed[z80_read(++address)]);
    bytes = 2;
    break;

  case 0xdd:
  case 0xfd:
    bytes = 1;
    do {
      prefix = op;
      op = z80_read(++address);
      bytes++;
    } while (op == 0xdd || op == 0xfd);

    switch (op) {
    case 0xcb:
      strcpy(disasm, mnemonics_ddcb[z80_read(address + 2)]);
      bytes++;
      break;

    case 0xed:
      strcpy(disasm, mnemonics_ed[z80_read(++address)]);
      bytes++;
      break;

    default:
      strcpy(disasm, mnemonics_dd[op]);
      break;
    }

    for (i = 0; i < 2; i++) {
      if (pos = strchr(disasm, '%')) {
	*pos = (prefix == 0xdd ? 'X' : 'Y');
      }
    }
    break;

  default:
    strcpy(disasm, mnemonics[op]);
    bytes = 1;
    break;
  }

  if (pos = strchr(disasm, '*')) {
    memmove(pos + 3, pos + 2, strlen(pos) - 1);
    sprintf(pos, "%02X", z80_read(++address));
    *(pos + 2) = 'h';
    bytes++;
  }

  if (pos = strchr(disasm, '@')) {
    memmove(pos + 5, pos + 2, strlen(pos) - 1);
    sprintf(pos, "%04X",
	    ((int8_t)z80_read((address + 1) & 0xffff) + address + 2) & 0xffff);
    *(pos + 4) = 'h';
    bytes++;
  }

  if (pos = strchr(disasm, '^')) {
    memmove(pos + 4, pos + 2, strlen(pos) - 1);
    i = z80_read(++address);
    if (i < 0x80) {
      sprintf(pos, "+%02X", i);
    } else {
      sprintf(pos, "-%02X", 0x100 - i);
    }
    *(pos + 3) = 'h';
    bytes++;
  }

  if (pos = strchr(disasm, '#')) {
    memmove(pos + 5, pos + 2, strlen(pos) - 1);
    sprintf(pos, "%02X%02X",
	    z80_read((address + 2) & 0xffff),
	    z80_read((address + 1) & 0xffff));
    *(pos + 4) = 'h';
    bytes += 2;
  }
  return bytes;
}

/*
 * assemble
 */
int z80_asm(char *input, byte *data, word address)
{
  size_t i, j;
  int bytes;
  char *pos;

  i = strspn(input, " ");
  for (j = strlen(input) - i; j > 0; j--) {
    if (input[i + j - 1] != ' ') {
      break;
    }
  }
  memmove(input, &input[i], j);
  input[j] = '\0';
	     
  while (pos = strstr(input, ", ")) {
    memmove(pos + 1, pos + 2, strlen(pos + 1));
  }
  while (pos = strstr(input, " ,")) {
    memmove(pos, pos + 1, strlen(pos));
  }

  for (i = 0; i < strlen(input); i++) {
    input[i] = toupper((int)input[i]);
  }

  
  if (bytes = search_code(mnemonics, 0, input, data, address)) {
    return bytes;
  }

  if (bytes = search_code(mnemonics_cb, 0, input, &data[1], address)) {
    data[0] = 0xcb;
    return bytes + 1;
  }

  if (bytes = search_code(mnemonics_ed, 0, input, &data[1], address)) {
    data[0] = 0xed;
    return bytes + 1;
  }

  if (bytes = search_code(mnemonics_dd, 'X', input, &data[1], address)) {
    data[0] = 0xdd;
    return bytes + 1;
  }

  if (bytes = search_code(mnemonics_ddcb, 'X', input, &data[2], address)) {
    data[0] = 0xdd;
    data[1] = 0xcb;
    i = data[2];
    data[2] = data[3];
    data[3] = i;
    return bytes + 2;
  }

  if (bytes = search_code(mnemonics_dd, 'Y', input, &data[1], address)) {
    data[0] = 0xfd;
    return bytes + 1;
  }

  if (bytes = search_code(mnemonics_ddcb, 'Y', input, &data[2], address)) {
    data[0] = 0xfd;
    data[1] = 0xcb;
    i = data[2];
    data[2] = data[3];
    data[3] = i;
    return bytes + 2;
  }

  return 0;
}

/*
 * fetch op code
 */
byte fetch(z80 *pcpu)
{
  byte op;

  pcpu->m1 = 1;
  op = z80_read(pcpu->pc++);
  pcpu->r = (pcpu->r & 0x80) + ((pcpu->r + 1) & 0x7f);
  pcpu->m1 = 0;
  return op;
}

/*
 * execute
 */
void exec_code(z80 *pcpu)
{
  static byte tmp8 = 0;
  static word tmp16;

  pcpu->cycles += tstates[pcpu->op1][pcpu->mcycle];
  pcpu->mcycle++;

  switch (pcpu->op1) {
  case 0x00:						/* nop */
    break;

  case 0x01:						/* ld bc,nn */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	pcpu->c = z80_read(pcpu->pc++);		break;
    case 3:	pcpu->b = z80_read(pcpu->pc++);		break;
    }
    break;

  case 0x11:						/* ld de,nn */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	pcpu->e = z80_read(pcpu->pc++);		break;
    case 3:	pcpu->d = z80_read(pcpu->pc++);		break;
    }
    break;

  case 0x21:						/* ld hl,nn */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	pcpu->l = z80_read(pcpu->pc++);		break;
    case 3:	pcpu->h = z80_read(pcpu->pc++);		break;
    }
    break;

  case 0x31:						/* ld sp,nn */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	pcpu->sp = z80_read(pcpu->pc++);	break;
    case 3:	pcpu->sp += z80_read(pcpu->pc++) << 8;	break;
    }
    break;

  case 0x02:						/* ld (bc),a */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	z80_write(BC, pcpu->a);			break;
    }
    break;

  case 0x12:						/* ld (de),a */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	z80_write(DE, pcpu->a);			break;
    }
    break;

  case 0x22:						/* ld (nn),hl */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	tmp16 = z80_read(pcpu->pc++);		break;
    case 3:	tmp16 += z80_read(pcpu->pc++) << 8;	break;
    case 4:	z80_write(tmp16,     pcpu->l);		break;
    case 5:	z80_write(tmp16 + 1, pcpu->h);		break;
    }
    break;

  case 0x32:						/* ld (nn),a */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	tmp16 = z80_read(pcpu->pc++);		break;
    case 3:	tmp16 += z80_read(pcpu->pc++) << 8;	break;
    case 4:	z80_write(tmp16, pcpu->a);		break;
    }
    break;

  case 0x03:	incw(&pcpu->b, &pcpu->c);	break;	/* inc bc */
  case 0x13:	incw(&pcpu->d, &pcpu->e);	break;	/* inc de */
  case 0x23:	incw(&pcpu->h, &pcpu->l);	break;	/* inc hl */
  case 0x33:	pcpu->sp++;			break;	/* inc sp */

  case 0x04:	inc(pcpu, &pcpu->b);		break;	/* inc b */
  case 0x0c:	inc(pcpu, &pcpu->c);		break;	/* inc c */
  case 0x14:	inc(pcpu, &pcpu->d);		break;	/* inc d */
  case 0x1c:	inc(pcpu, &pcpu->e);		break;	/* inc e */
  case 0x24:	inc(pcpu, &pcpu->h);		break;	/* inc h */
  case 0x2c:	inc(pcpu, &pcpu->l);		break;	/* inc l */
  case 0x34:	mreg(pcpu, inc, HL, 0);		break;	/* inc (hl) */
  case 0x3c:	inc(pcpu, &pcpu->a);		break;	/* inc a */

  case 0x05:	dec(pcpu, &pcpu->b);		break;	/* dec b */
  case 0x0d:	dec(pcpu, &pcpu->c);		break;	/* dec c */
  case 0x15:	dec(pcpu, &pcpu->d);		break;	/* dec d */
  case 0x1d:	dec(pcpu, &pcpu->e);		break;	/* dec e */
  case 0x25:	dec(pcpu, &pcpu->h);		break;	/* dec h */
  case 0x2d:	dec(pcpu, &pcpu->l);		break;	/* dec l */
  case 0x35:	mreg(pcpu, dec, HL, 0);		break;	/* dec (hl) */
  case 0x3d:	dec(pcpu, &pcpu->a);		break;	/* dec a */

  case 0x06:	ldrn(pcpu, &pcpu->b);		break;	/* ld b,n */
  case 0x0e:	ldrn(pcpu, &pcpu->c);		break;	/* ld c,n */
  case 0x16:	ldrn(pcpu, &pcpu->d);		break;	/* ld d,n */
  case 0x1e:	ldrn(pcpu, &pcpu->e);		break;	/* ld e,n */
  case 0x26:	ldrn(pcpu, &pcpu->h);		break;	/* ld h,n */
  case 0x2e:	ldrn(pcpu, &pcpu->l);		break;	/* ld l,n */
  case 0x36:					/*ld (hl),n */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	tmp8 = z80_read(pcpu->pc++);		break;
    case 3:	z80_write(HL, tmp8);			break;
    }
    break;
  case 0x3e:	ldrn(pcpu, &pcpu->a);		break;	/* ld a,n */

  case 0x07:						/* rlca */
    pcpu->a = (pcpu->a << 1) + (pcpu->a >> 7);
    pcpu->f
      = (pcpu->f & SF)
      | (pcpu->f & ZF)
      | (pcpu->a & YF)
      | 0
      | (pcpu->a & XF)
      | (pcpu->f & PF)
      | 0
      | ((pcpu->a & 0x01) ? CF : 0);
    break;

  case 0x0f:						/* rrca */
    pcpu->a = (pcpu->a >> 1) + (pcpu->a << 7);
    pcpu->f
      = (pcpu->f & SF)
      | (pcpu->f & ZF)
      | (pcpu->a & YF)
      | 0
      | (pcpu->a & XF)
      | (pcpu->f & PF)
      | 0
      | ((pcpu->a & 0x80) ? CF : 0);
    break;

  case 0x17:						/* rla */
    tmp8 = pcpu->a;
    pcpu->a = (pcpu->a << 1) + ((pcpu->f & CF) ? 0x01 : 0);
    pcpu->f
      = (pcpu->f & SF)
      | (pcpu->f & ZF)
      | (pcpu->a & YF)
      | 0
      | (pcpu->a & XF)
      | (pcpu->f & PF)
      | 0
      | ((tmp8 & 0x80) ? CF : 0);
    break;

  case 0x1f:						/* rra */
    tmp8 = pcpu->a;
    pcpu->a = (pcpu->a >> 1) + ((pcpu->f & CF) ? 0x80 : 0);
    pcpu->f
      = (pcpu->f & SF)
      | (pcpu->f & ZF)
      | (pcpu->a & YF)
      | 0
      | (pcpu->a & XF)
      | (pcpu->f & PF)
      | 0
      | ((tmp8 & 0x01) ? CF : 0);
    break;

  case 0x08:						/* ex af,af' */
    tmp8 = pcpu->a;
    pcpu->a = pcpu->alt_a;
    pcpu->alt_a = tmp8;
    tmp8 = pcpu->f;
    pcpu->f = pcpu->alt_f;
    pcpu->alt_f = tmp8;
    break;

  case 0x09:	addw(pcpu, &pcpu->h, &pcpu->l, BC);	break;	/* add hl,bc */
  case 0x19:	addw(pcpu, &pcpu->h, &pcpu->l, DE);	break;	/* add hl,de */
  case 0x29:	addw(pcpu, &pcpu->h, &pcpu->l, HL);	break;	/* add hl,hl */
  case 0x39:	addw(pcpu, &pcpu->h, &pcpu->l, pcpu->sp);	break;	/* add hl,sp */

  case 0x0a:						/* ld a,(bc) */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	pcpu->a = z80_read(BC);			break;
    }
    break;
  case 0x1a:						/* ld a,(de) */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	pcpu->a = z80_read(DE);			break;
    }
    break;
  case 0x2a:						/* ld hl,(nn) */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	tmp16 = z80_read(pcpu->pc++);		break;
    case 3:	tmp16 += z80_read(pcpu->pc++) << 8;	break;
    case 4:	pcpu->l = z80_read(tmp16);		break;
    case 5:	pcpu->h = z80_read(tmp16 + 1);		break;
    }
    break;
  case 0x3a:						/* ld a,(nn) */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	tmp16 = z80_read(pcpu->pc++);		break;
    case 3:	tmp16 += z80_read(pcpu->pc++) << 8;	break;
    case 4:	pcpu->a = z80_read(tmp16);		break;
    }
    break;

  case 0x0b:	decw(&pcpu->b, &pcpu->c);	break;	/* dec bc */
  case 0x1b:	decw(&pcpu->d, &pcpu->e);	break;	/* dec de */
  case 0x2b:	decw(&pcpu->h, &pcpu->l);	break;	/* dec hl */
  case 0x3b:	pcpu->sp--;			break;	/* dec sp */

  case 0x10:						/* djnz e */
    switch (pcpu->mcycle) {
    case 1:				break;
    case 2:
      tmp8 = z80_read(pcpu->pc++);
      if (--pcpu->b == 0) {
	pcpu->mcycle = 0;
      }
      break;
    case 3:      jr(pcpu, tmp8);	break;
    }
    break;
  case 0x18:	jrcnd(pcpu, 0, 0);		break;	/* jr e */
  case 0x20:    jrcnd(pcpu, ZF, 0);		break;	/* jr nz,e */
  case 0x28:	jrcnd(pcpu, ZF, ZF);		break;	/* jr z,e */
  case 0x30:	jrcnd(pcpu, CF, 0);		break;	/* jr nc,e */
  case 0x38:	jrcnd(pcpu, CF, CF);		break;	/* jr c,e */

  case 0x27:						/* daa */
    {
      byte high, low, diff;

      high = pcpu->a >> 4;
      low = pcpu->a & 0x0f;
      if ((pcpu->f & HF) || low >= 0xa) {
	diff = 0x06;
      } else {
	diff = 0;
      }
      if ((pcpu->f & CF) || high >= 0xa || (high == 0x9 && low >= 0xa)) {
	diff += 0x60;
      }

      if (pcpu->f & NF) {
	pcpu->a -= diff;
      } else {
	pcpu->a += diff;
      }

      pcpu->f
	= newflags[pcpu->a]
	| ((((pcpu->f & NF) == 0 && low >= 0xa)
	    || (pcpu->f & NF) && (pcpu->f & HF) && low <= 0x5)
	   ? HF : 0)
	| (pcpu->f & NF)
	| ((diff >= 0x60) ? CF : 0);
      break;
    }

  case 0x2f:						/* cpl */
    pcpu->a = ~pcpu->a;
    pcpu->f
      = (pcpu->f & SF)
      | (pcpu->f & ZF)
      | (pcpu->a & YF)
      | HF
      | (pcpu->a & XF)
      | (pcpu->f & VF)
      | NF
      | (pcpu->f & CF);
    break;

  case 0x37:						/* scf */
    pcpu->f
      = (pcpu->f & SF)
      | (pcpu->f & ZF)
      | (pcpu->a & YF)
      | 0
      | (pcpu->a & XF)
      | (pcpu->f & VF)
      | 0
      | CF;
    break;

  case 0x3f:						/* ccf */
    pcpu->f
      = (pcpu->f & SF)
      | (pcpu->f & ZF)
      | (pcpu->a & YF)
      | ((pcpu->f & CF) ? HF : 0)
      | (pcpu->a & XF)
      | (pcpu->f & VF)
      | 0
      | ((pcpu->f & CF) ? 0 : CF);
    break;

  case 0x40:	pcpu->b = pcpu->b;	    	break;	/* ld b,b */
  case 0x41:	pcpu->b = pcpu->c;		break;	/* ld b,c */
  case 0x42:	pcpu->b = pcpu->d;		break;	/* ld b,d */
  case 0x43:	pcpu->b = pcpu->e;		break;	/* ld b,e */
  case 0x44:	pcpu->b = pcpu->h;		break;	/* ld b,h */
  case 0x45:	pcpu->b = pcpu->l;		break;	/* ld b,l */
  case 0x46:	ldrhl(pcpu, &pcpu->b);		break;	/* ld b,(hl) */
  case 0x47:	pcpu->b = pcpu->a;		break;	/* ld b,a */

  case 0x48:	pcpu->c = pcpu->b;		break;	/* ld c,b */
  case 0x49:	pcpu->c = pcpu->c;		break;	/* ld c,c */
  case 0x4a:	pcpu->c = pcpu->d;		break;	/* ld c,d */
  case 0x4b:	pcpu->c = pcpu->e;		break;	/* ld c,e */
  case 0x4c:	pcpu->c = pcpu->h;		break;	/* ld c,h */
  case 0x4d:	pcpu->c = pcpu->l;		break;	/* ld c,l */
  case 0x4e:	ldrhl(pcpu, &pcpu->c);		break;	/* ld c,(hl) */
  case 0x4f:	pcpu->c = pcpu->a;		break;	/* ld c,a */

  case 0x50:	pcpu->d = pcpu->b;		break;	/* ld d,b */
  case 0x51:	pcpu->d = pcpu->c;		break;	/* ld d,c */
  case 0x52:	pcpu->d = pcpu->d;		break;	/* ld d,d */
  case 0x53:	pcpu->d = pcpu->e;		break;	/* ld d,e */
  case 0x54:	pcpu->d = pcpu->h;		break;	/* ld d,h */
  case 0x55:	pcpu->d = pcpu->l;		break;	/* ld d,l */
  case 0x56:	ldrhl(pcpu, &pcpu->d);		break;	/* ld d,(hl) */
  case 0x57:	pcpu->d = pcpu->a;		break;	/* ld d,a */

  case 0x58:	pcpu->e = pcpu->b;		break;	/* ld e,b */
  case 0x59:	pcpu->e = pcpu->c;		break;	/* ld e,c */
  case 0x5a:	pcpu->e = pcpu->d;		break;	/* ld e,d */
  case 0x5b:	pcpu->e = pcpu->e;		break;	/* ld e,e */
  case 0x5c:	pcpu->e = pcpu->h;		break;	/* ld e,h */
  case 0x5d:	pcpu->e = pcpu->l;		break;	/* ld e,l */
  case 0x5e:	ldrhl(pcpu, &pcpu->e);		break;	/* ld e,(hl) */
  case 0x5f:	pcpu->e = pcpu->a;		break;	/* ld e,a */

  case 0x60:	pcpu->h = pcpu->b;		break;	/* ld h,b */
  case 0x61:	pcpu->h = pcpu->c;		break;	/* ld h,c */
  case 0x62:	pcpu->h = pcpu->d;		break;	/* ld h,d */
  case 0x63:	pcpu->h = pcpu->e;		break;	/* ld h,e */
  case 0x64:	pcpu->h = pcpu->h;		break;	/* ld h,h */
  case 0x65:	pcpu->h = pcpu->l;		break;	/* ld h,l */
  case 0x66:	ldrhl(pcpu, &pcpu->h);		break;	/* ld h,(hl) */
  case 0x67:	pcpu->h = pcpu->a;		break;	/* ld h,a */

  case 0x68:	pcpu->l = pcpu->b;		break;	/* ld l,b */
  case 0x69:	pcpu->l = pcpu->c;		break;	/* ld l,c */
  case 0x6a:	pcpu->l = pcpu->d;		break;	/* ld l,d */
  case 0x6b:	pcpu->l = pcpu->e;		break;	/* ld l,e */
  case 0x6c:	pcpu->l = pcpu->h;		break;	/* ld l,h */
  case 0x6d:	pcpu->l = pcpu->l;		break;	/* ld l,l */
  case 0x6e:	ldrhl(pcpu, &pcpu->l);		break;	/* ld l,(hl) */
  case 0x6f:	pcpu->l = pcpu->a;		break;	/* ld l,a */

  case 0x70:	ldhlr(pcpu, pcpu->b);		break;	/* ld (hl),b */
  case 0x71:	ldhlr(pcpu, pcpu->c);		break;	/* ld (hl),c */
  case 0x72:	ldhlr(pcpu, pcpu->d);		break;	/* ld (hl),d */
  case 0x73:	ldhlr(pcpu, pcpu->e);		break;	/* ld (hl),e */
  case 0x74:	ldhlr(pcpu, pcpu->h);		break;	/* ld (hl),h */
  case 0x75:	ldhlr(pcpu, pcpu->l);		break;	/* ld (hl),l */
  case 0x76:	pcpu->halt = 1;	pcpu->pc--;	break;	/* halt */
  case 0x77:	ldhlr(pcpu, pcpu->a);		break;	/* ld (hl),a */

  case 0x78:	pcpu->a = pcpu->b;		break;	/* ld a,b */
  case 0x79:	pcpu->a = pcpu->c;		break;	/* ld a,c */
  case 0x7a:	pcpu->a = pcpu->d;		break;	/* ld a,d */
  case 0x7b:	pcpu->a = pcpu->e;		break;	/* ld a,e */
  case 0x7c:	pcpu->a = pcpu->h;		break;	/* ld a,h */
  case 0x7d:	pcpu->a = pcpu->l;		break;	/* ld a,l */
  case 0x7e:	ldrhl(pcpu, &pcpu->a);		break;	/* ld a,(hl) */
  case 0x7f:	pcpu->a = pcpu->a;		break;	/* ld a,a */

  case 0x80:	add(pcpu, pcpu->b);		break;	/* add a,b */
  case 0x81:	add(pcpu, pcpu->c);		break;	/* add a,c */
  case 0x82:	add(pcpu, pcpu->d);		break;	/* add a,d */
  case 0x83:	add(pcpu, pcpu->e);		break;	/* add a,e */
  case 0x84:	add(pcpu, pcpu->h);		break;	/* add a,h */
  case 0x85:	add(pcpu, pcpu->l);		break;	/* add a,l */
  case 0x86:						/* add a,(hl) */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:      add(pcpu, z80_read(HL));		break;
    }
    break;
  case 0x87:	add(pcpu, pcpu->a);		break;	/* add a,a */

  case 0x88:	adc(pcpu, pcpu->b);		break;	/* adc a,b */
  case 0x89:	adc(pcpu, pcpu->c);		break;	/* adc a,c */
  case 0x8a:	adc(pcpu, pcpu->d);		break;	/* adc a,d */
  case 0x8b:	adc(pcpu, pcpu->e);		break;	/* adc a,e */
  case 0x8c:	adc(pcpu, pcpu->h);		break;	/* adc a,h */
  case 0x8d:	adc(pcpu, pcpu->l);		break;	/* adc a,l */
  case 0x8e:						/* adc a,(hl) */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	adc(pcpu, z80_read(HL));		break;
    }
    break;
  case 0x8f:	adc(pcpu, pcpu->a);		break;	/* adc a,a */

  case 0x90:	sub(pcpu, pcpu->b);		break;	/* sub b */
  case 0x91:	sub(pcpu, pcpu->c);		break;	/* sub c */
  case 0x92:	sub(pcpu, pcpu->d);		break;	/* sub d */
  case 0x93:	sub(pcpu, pcpu->e);		break;	/* sub e */
  case 0x94:	sub(pcpu, pcpu->h);		break;	/* sub h */
  case 0x95:	sub(pcpu, pcpu->l);		break;	/* sub l */
  case 0x96:						/* sub (hl) */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	sub(pcpu, z80_read(HL));		break;
    }
    break;
  case 0x97:	sub(pcpu, pcpu->a);		break;	/* sub a */

  case 0x98:	sbc(pcpu, pcpu->b);		break;	/* sbc a,b */
  case 0x99:	sbc(pcpu, pcpu->c);		break;	/* sbc a,c */
  case 0x9a:	sbc(pcpu, pcpu->d);		break;	/* sbc a,d */
  case 0x9b:	sbc(pcpu, pcpu->e);		break;	/* sbc a,e */
  case 0x9c:	sbc(pcpu, pcpu->h);		break;	/* sbc a,h */
  case 0x9d:	sbc(pcpu, pcpu->l);		break;	/* sbc a,l */
  case 0x9e:						/* sbc a,(hl) */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	sbc(pcpu, z80_read(HL));		break;
    }
    break;
  case 0x9f:	sbc(pcpu, pcpu->a);		break;	/* sbc a,a */

  case 0xa0:	and(pcpu, pcpu->b);		break;	/* and b */
  case 0xa1:	and(pcpu, pcpu->c);		break;	/* and c */
  case 0xa2:	and(pcpu, pcpu->d);		break;	/* and d */
  case 0xa3:	and(pcpu, pcpu->e);		break;	/* and e */
  case 0xa4:	and(pcpu, pcpu->h);		break;	/* and h */
  case 0xa5:	and(pcpu, pcpu->l);		break;	/* and l */
  case 0xa6:						/* and (hl) */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	and(pcpu, z80_read(HL));		break;
    }
    break;
  case 0xa7:	and(pcpu, pcpu->a);		break;	/* and a */

  case 0xa8:	xor(pcpu, pcpu->b);		break;	/* xor b */
  case 0xa9:	xor(pcpu, pcpu->c);		break;	/* xor c */
  case 0xaa:	xor(pcpu, pcpu->d);		break;	/* xor d */
  case 0xab:	xor(pcpu, pcpu->e);		break;	/* xor e */
  case 0xac:	xor(pcpu, pcpu->h);		break;	/* xor h */
  case 0xad:	xor(pcpu, pcpu->l);		break;	/* xor l */
  case 0xae:						/* xor (hl) */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	xor(pcpu, z80_read(HL));		break;
    }
    break;
  case 0xaf:	xor(pcpu, pcpu->a);		break;	/* xor a */

  case 0xb0:	or(pcpu, pcpu->b);		break;	/* or b */
  case 0xb1:	or(pcpu, pcpu->c);		break;	/* or c */
  case 0xb2:	or(pcpu, pcpu->d);		break;	/* or d */
  case 0xb3:	or(pcpu, pcpu->e);		break;	/* or e */
  case 0xb4:	or(pcpu, pcpu->h);		break;	/* or h */
  case 0xb5:	or(pcpu, pcpu->l);		break;	/* or l */
  case 0xb6:						/* or (hl) */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	or(pcpu, z80_read(HL));			break;
    }
    break;
  case 0xb7:	or(pcpu, pcpu->a);		break;	/* or a */

  case 0xb8:	cp(pcpu, pcpu->b);		break;	/* cp b */
  case 0xb9:	cp(pcpu, pcpu->c);		break;	/* cp c */
  case 0xba:	cp(pcpu, pcpu->d);		break;	/* cp d */
  case 0xbb:	cp(pcpu, pcpu->e);		break;	/* cp e */
  case 0xbc:	cp(pcpu, pcpu->h);		break;	/* cp h */
  case 0xbd:	cp(pcpu, pcpu->l);		break;	/* cp l */
  case 0xbe:						/* cp (hl) */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	cp(pcpu, z80_read(HL));			break;
    }
    break;
  case 0xbf:	cp(pcpu, pcpu->a);		break;	/* cp a */

  case 0xc0:	retcnd(pcpu, ZF, 0, 0);		break;	/* ret nz */
  case 0xc8:	retcnd(pcpu, ZF, ZF, 0);	break;	/* ret z */
  case 0xd0:	retcnd(pcpu, CF, 0, 0);		break;	/* ret nc */
  case 0xd8:	retcnd(pcpu, CF, CF, 0);	break;	/* ret c */
  case 0xe0:	retcnd(pcpu, PF, 0, 0);		break;	/* ret po */
  case 0xe8:	retcnd(pcpu, PF, PF, 0);	break;	/* ret pe */
  case 0xf0:	retcnd(pcpu, SF, 0, 0);		break;	/* ret p */
  case 0xf8:	retcnd(pcpu, SF, SF, 0);	break;	/* ret m */

  case 0xc1:	pop(pcpu, &pcpu->b, &pcpu->c);	break;	/* pop bc */
  case 0xd1:	pop(pcpu, &pcpu->d, &pcpu->e);	break;	/* pop de */
  case 0xe1:	pop(pcpu, &pcpu->h, &pcpu->l);	break;	/* pop hl */
  case 0xf1:	pop(pcpu, &pcpu->a, &pcpu->f);	break;	/* pop af */

  case 0xc2:	jpcnd(pcpu, ZF, 0);		break;	/* jp nz,nn */
  case 0xca:	jpcnd(pcpu, ZF, ZF);		break;	/* jp z,nn */
  case 0xd2:	jpcnd(pcpu, CF, 0);		break;	/* jp nc,nn */
  case 0xda:	jpcnd(pcpu, CF, CF);		break;	/* jp c,nn */
  case 0xe2:	jpcnd(pcpu, PF, 0);		break;	/* jp po,nn */
  case 0xea:	jpcnd(pcpu, PF, PF);		break;	/* jp pe,nn */
  case 0xf2:	jpcnd(pcpu, SF, 0);		break;	/* jp p,nn */
  case 0xfa:	jpcnd(pcpu, SF, SF);		break;	/* jp m,nn */

  case 0xc4:	callcnd(pcpu, ZF, 0);		break;	/* call nz,nn */
  case 0xcc:	callcnd(pcpu, ZF, ZF);		break;	/* call z,nn*/
  case 0xd4:	callcnd(pcpu, CF, 0);		break;	/* call nc,nn */
  case 0xdc:	callcnd(pcpu, CF, CF);		break;	/* call c,nn */
  case 0xe4:	callcnd(pcpu, PF, 0);		break;	/* call po,nn */
  case 0xec:	callcnd(pcpu, PF, PF);		break;	/* call pe,nn */
  case 0xf4:	callcnd(pcpu, SF, 0);		break;	/* call p,nn*/
  case 0xfc:	callcnd(pcpu, SF, SF);		break;	/* call m,nn */

  case 0xc5:	push(pcpu, pcpu->b, pcpu->c);	break;	/* push bc */
  case 0xd5:	push(pcpu, pcpu->d, pcpu->e);	break;	/* push de */
  case 0xe5:	push(pcpu, pcpu->h, pcpu->l);	break;	/* push hl */
  case 0xf5:	push(pcpu, pcpu->a, pcpu->f);	break;	/* push af */

  case 0xc6:						/* add a,n */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	add(pcpu, z80_read(pcpu->pc++));	break;
    }
    break;
  case 0xce:						/* adc a,n */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	adc(pcpu, z80_read(pcpu->pc++));	break;
    }
    break;
  case 0xd6:						/* sub n */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	sub(pcpu, z80_read(pcpu->pc++));	break;
    }
    break;
  case 0xde:						/* sbc a,n */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	sbc(pcpu, z80_read(pcpu->pc++));	break;
    }
    break;
  case 0xe6:						/* and n */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	and(pcpu, z80_read(pcpu->pc++));	break;
    }
    break;
  case 0xee:						/* xor n */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	xor(pcpu, z80_read(pcpu->pc++));	break;
    }
    break;
  case 0xf6:						/* or n */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	or(pcpu, z80_read(pcpu->pc++));		break;
    }
    break;
  case 0xfe:						/* cp n */
    switch (pcpu->mcycle) {
    case 1:						break;
    case 2:	cp(pcpu, z80_read(pcpu->pc++));		break;
    }
    break;

  case 0xc7:	rst(pcpu, 0x0000);		break;	/* rst 00h */
  case 0xcf:	rst(pcpu, 0x0008);		break;	/* rst 08h */
  case 0xd7:	rst(pcpu, 0x0010);		break;	/* rst 10h */
  case 0xdf:	rst(pcpu, 0x0018);		break;	/* rst 18h */
  case 0xe7:	rst(pcpu, 0x0020);		break;	/* rst 20h */
  case 0xef:	rst(pcpu, 0x0028);		break;	/* rst 28h */
  case 0xf7:	rst(pcpu, 0x0030);		break;	/* rst 30h */
  case 0xff:	rst(pcpu, 0x0038);		break;	/* rst 38h */

  case 0xc3:	jpcnd(pcpu, 0, 0);		break;	/* jp nn */

  case 0xc9:	retcnd(pcpu, 0, 0, 0);		break;	/* ret */

  case 0xcd:	callcnd(pcpu, 0, 0);		break;	/* call nn */

  case 0xd3:						/* out (n),a */
    switch (pcpu->mcycle) {
    case 1:							break;
    case 2:	tmp8 = z80_read(pcpu->pc++);			break;
    case 3:	z80_out(tmp8 + (pcpu->a << 8), pcpu->a);	break;
    }
    break;

  case 0xd9:						/* exx */
    tmp8 = pcpu->b;
    pcpu->b = pcpu->alt_b;
    pcpu->alt_b = tmp8;
    tmp8 = pcpu->c;
    pcpu->c = pcpu->alt_c;
    pcpu->alt_c = tmp8;
    tmp8 = pcpu->d;
    pcpu->d = pcpu->alt_d;
    pcpu->alt_d = tmp8;
    tmp8 = pcpu->e;
    pcpu->e = pcpu->alt_e;
    pcpu->alt_e = tmp8;
    tmp8 = pcpu->h;
    pcpu->h = pcpu->alt_h;
    pcpu->alt_h = tmp8;
    tmp8 = pcpu->l;
    pcpu->l = pcpu->alt_l;
    pcpu->alt_l = tmp8;
    break;

  case 0xdb:						/* in a,(n) */
    switch (pcpu->mcycle) {
    case 1:							break;
    case 2:	tmp8 =  z80_read(pcpu->pc++);			break;
    case 3:	pcpu->a = z80_in(tmp8 + (pcpu->a << 8));	break;
    }
    break;

  case 0xe3:						/* ex (sp),hl */
    switch (pcpu->mcycle) {
    case 1:							break;
    case 2:	tmp16 = z80_read(pcpu->sp++);			break;
    case 3:	tmp16 += z80_read(pcpu->sp) << 8;		break;
    case 4:	z80_write(pcpu->sp--, pcpu->h);			break;
    case 5:
      z80_write(pcpu->sp, pcpu->l);
      pcpu->l = tmp16 & 0xff;
      pcpu->h = tmp16 >> 8;
      break;
    }
    break;

  case 0xe9:	pcpu->pc = HL;			break;	/* jp (hl) */

  case 0xeb:						/* ex de,hl */
    tmp8 = pcpu->d;
    pcpu->d = pcpu->h;
    pcpu->h = tmp8;
    tmp8 = pcpu->e;
    pcpu->e = pcpu->l;
    pcpu->l = tmp8;
    break;

  case 0xf3:						/* di */
    pcpu->iff1 = 0;
    pcpu->iff2 = 0;
    break;

  case 0xf9:	pcpu->sp = HL;	break;			/* ld sp,hl */

  case 0xfb:						/* ei */
    if (pcpu->iff1 == 0 && pcpu->ei == 0) {
      pcpu->iff1 = 1;
      pcpu->iff2 = 1;
      pcpu->ei = 1;
    }
    break;

  case 0xcb:						/* CB prefix */
    if (pcpu->mcycle == 1) {
      break;
    } else if (pcpu->mcycle == 2) {
      pcpu->op2 = fetch(pcpu);
    }
    pcpu->cycles += tstates_cb[pcpu->op2][pcpu->mcycle - 2];

    switch (pcpu->op2) {
    case 0x00:	rlc(pcpu, &pcpu->b);			break;	/* rlc b */
    case 0x01:	rlc(pcpu, &pcpu->c);			break;	/* rlc c */
    case 0x02:	rlc(pcpu, &pcpu->d);			break;	/* rlc d */
    case 0x03:	rlc(pcpu, &pcpu->e);			break;	/* rlc e */
    case 0x04:	rlc(pcpu, &pcpu->h);			break;	/* rlc h */
    case 0x05:	rlc(pcpu, &pcpu->l);			break;	/* rlc l */
    case 0x06:	mreg(pcpu, rlc, HL, 1);		break;	/* rlc (hl) */
    case 0x07:	rlc(pcpu, &pcpu->a);			break;	/* rlc a */

    case 0x08:	rrc(pcpu, &pcpu->b);			break;	/* rrc b */
    case 0x09:	rrc(pcpu, &pcpu->c);			break;	/* rrc c */
    case 0x0a:	rrc(pcpu, &pcpu->d);			break;	/* rrc d */
    case 0x0b:	rrc(pcpu, &pcpu->e);			break;	/* rrc e */
    case 0x0c:	rrc(pcpu, &pcpu->h);			break;	/* rrc h */
    case 0x0d:	rrc(pcpu, &pcpu->l);			break;	/* rrc l */
    case 0x0e:	mreg(pcpu, rrc, HL, 1);		break;	/* rrc (hl) */
    case 0x0f:	rrc(pcpu, &pcpu->a);			break;	/* rrc a */

    case 0x10:	rl(pcpu, &pcpu->b);			break;	/* rl b */
    case 0x11:	rl(pcpu, &pcpu->c);			break;	/* rl c */
    case 0x12:	rl(pcpu, &pcpu->d);			break;	/* rl d */
    case 0x13:	rl(pcpu, &pcpu->e);			break;	/* rl e */
    case 0x14:	rl(pcpu, &pcpu->h);			break;	/* rl h */
    case 0x15:	rl(pcpu, &pcpu->l);			break;	/* rl l */
    case 0x16:	mreg(pcpu, rl, HL, 1);			break;	/* rl (hl) */
    case 0x17:	rl(pcpu, &pcpu->a);			break;	/* rl a */

    case 0x18:	rr(pcpu, &pcpu->b);			break;	/* rr b */
    case 0x19:	rr(pcpu, &pcpu->c);			break;	/* rr c */
    case 0x1a:	rr(pcpu, &pcpu->d);			break;	/* rr d */
    case 0x1b:	rr(pcpu, &pcpu->e);			break;	/* rr e */
    case 0x1c:	rr(pcpu, &pcpu->h);			break;	/* rr h */
    case 0x1d:	rr(pcpu, &pcpu->l);			break;	/* rr l */
    case 0x1e:	mreg(pcpu, rr, HL, 1);			break;	/* rr (hl) */
    case 0x1f:	rr(pcpu, &pcpu->a);			break;	/* rr a */

    case 0x20:	sla(pcpu, &pcpu->b);			break;	/* sla b */
    case 0x21:	sla(pcpu, &pcpu->c);			break;	/* sla c */
    case 0x22:	sla(pcpu, &pcpu->d);			break;	/* sla d */
    case 0x23:	sla(pcpu, &pcpu->e);			break;	/* sla e */
    case 0x24:	sla(pcpu, &pcpu->h);			break;	/* sla h */
    case 0x25:	sla(pcpu, &pcpu->l);			break;	/* sla l */
    case 0x26:	mreg(pcpu, sla, HL, 1);			break;	/* sla (hl) */
    case 0x27:	sla(pcpu, &pcpu->a);			break;	/* sla a */

    case 0x28:	sra(pcpu, &pcpu->b);			break;	/* sra b */
    case 0x29:	sra(pcpu, &pcpu->c);			break;	/* sra c */
    case 0x2a:	sra(pcpu, &pcpu->d);			break;	/* sra d */
    case 0x2b:	sra(pcpu, &pcpu->e);			break;	/* sra e */
    case 0x2c:	sra(pcpu, &pcpu->h);			break;	/* sra h */
    case 0x2d:	sra(pcpu, &pcpu->l);			break;	/* sra l */
    case 0x2e:	mreg(pcpu, sra, HL, 1);			break;	/* sra (hl) */
    case 0x2f:	sra(pcpu, &pcpu->a);			break;	/* sra a */

    case 0x30:	sll(pcpu, &pcpu->b);			break;	/* sll b */
    case 0x31:	sll(pcpu, &pcpu->c);			break;	/* sll c */
    case 0x32:	sll(pcpu, &pcpu->d);			break;	/* sll d */
    case 0x33:	sll(pcpu, &pcpu->e);			break;	/* sll e */
    case 0x34:	sll(pcpu, &pcpu->h);			break;	/* sll h */
    case 0x35:	sll(pcpu, &pcpu->l);			break;	/* sll l */
    case 0x36:	mreg(pcpu, sll, HL, 1);			break;	/* sll (hl) */
    case 0x37:	sll(pcpu, &pcpu->a);			break;	/* sll a */

    case 0x38:	srl(pcpu, &pcpu->b);			break;	/* srl b */
    case 0x39:	srl(pcpu, &pcpu->c);			break;	/* srl c */
    case 0x3a:	srl(pcpu, &pcpu->d);			break;	/* srl d */
    case 0x3b:	srl(pcpu, &pcpu->e);			break;	/* srl e */
    case 0x3c:	srl(pcpu, &pcpu->h);			break;	/* srl h */
    case 0x3d:	srl(pcpu, &pcpu->l);			break;	/* srl l */
    case 0x3e:	mreg(pcpu, srl, HL, 1);			break;	/* srl (hl) */
    case 0x3f:	srl(pcpu, &pcpu->a);			break;	/* srl a */

    case 0x40:	bit(pcpu, 0, pcpu->b);		break;	/* bit 0,b */
    case 0x41:	bit(pcpu, 0, pcpu->c);		break;	/* bit 0,c */
    case 0x42:	bit(pcpu, 0, pcpu->d);		break;	/* bit 0,d */
    case 0x43:	bit(pcpu, 0, pcpu->e);		break;	/* bit 0,e */
    case 0x44:	bit(pcpu, 0, pcpu->h);		break;	/* bit 0,h */
    case 0x45:	bit(pcpu, 0, pcpu->l);		break;	/* bit 0,l */
    case 0x46:	bitm(pcpu, 0);			break;	/* bit 0,(hl) */
    case 0x47:	bit(pcpu, 0, pcpu->a);		break;	/* bit 0,a */
    case 0x48:	bit(pcpu, 1, pcpu->b);		break;	/* bit 1,b */
    case 0x49:	bit(pcpu, 1, pcpu->c);		break;	/* bit 1,c */
    case 0x4a:	bit(pcpu, 1, pcpu->d);		break;	/* bit 1,d */
    case 0x4b:	bit(pcpu, 1, pcpu->e);		break;	/* bit 1,e */
    case 0x4c:	bit(pcpu, 1, pcpu->h);		break;	/* bit 1,h */
    case 0x4d:	bit(pcpu, 1, pcpu->l);		break;	/* bit 1,l */
    case 0x4e:	bitm(pcpu, 1);			break;	/* bit 1,(hl) */
    case 0x4f:	bit(pcpu, 1, pcpu->a);		break;	/* bit 1,a */
    case 0x50:	bit(pcpu, 2, pcpu->b);		break;	/* bit 2,b */
    case 0x51:	bit(pcpu, 2, pcpu->c);		break;	/* bit 2,c */
    case 0x52:	bit(pcpu, 2, pcpu->d);		break;	/* bit 2,d */
    case 0x53:	bit(pcpu, 2, pcpu->e);		break;	/* bit 2,e */
    case 0x54:	bit(pcpu, 2, pcpu->h);		break;	/* bit 2,h */
    case 0x55:	bit(pcpu, 2, pcpu->l);		break;	/* bit 2,l */
    case 0x56:	bitm(pcpu, 2);			break;	/* bit 2,(hl) */
    case 0x57:	bit(pcpu, 2, pcpu->a);		break;	/* bit 2,a */
    case 0x58:	bit(pcpu, 3, pcpu->b);		break;	/* bit 3,b */
    case 0x59:	bit(pcpu, 3, pcpu->c);		break;	/* bit 3,c */
    case 0x5a:	bit(pcpu, 3, pcpu->d);		break;	/* bit 3,d */
    case 0x5b:	bit(pcpu, 3, pcpu->e);		break;	/* bit 3,e */
    case 0x5c:	bit(pcpu, 3, pcpu->h);		break;	/* bit 3,h */
    case 0x5d:	bit(pcpu, 3, pcpu->l);		break;	/* bit 3,l */
    case 0x5e:	bitm(pcpu, 3);			break;	/* bit 3,(hl) */
    case 0x5f:	bit(pcpu, 3, pcpu->a);		break;	/* bit 3,a */
    case 0x60:	bit(pcpu, 4, pcpu->b);		break;	/* bit 4,b */
    case 0x61:	bit(pcpu, 4, pcpu->c);		break;	/* bit 4,c */
    case 0x62:	bit(pcpu, 4, pcpu->d);		break;	/* bit 4,d */
    case 0x63:	bit(pcpu, 4, pcpu->e);		break;	/* bit 4,e */
    case 0x64:	bit(pcpu, 4, pcpu->h);		break;	/* bit 4,h */
    case 0x65:	bit(pcpu, 4, pcpu->l);		break;	/* bit 4,l */
    case 0x66:	bitm(pcpu, 4);			break;	/* bit 4,(hl) */
    case 0x67:	bit(pcpu, 4, pcpu->a);		break;	/* bit 4,a */
    case 0x68:	bit(pcpu, 5, pcpu->b);		break;	/* bit 5,b */
    case 0x69:	bit(pcpu, 5, pcpu->c);		break;	/* bit 5,c */
    case 0x6a:	bit(pcpu, 5, pcpu->d);		break;	/* bit 5,d */
    case 0x6b:	bit(pcpu, 5, pcpu->e);		break;	/* bit 5,e */
    case 0x6c:	bit(pcpu, 5, pcpu->h);		break;	/* bit 5,h */
    case 0x6d:	bit(pcpu, 5, pcpu->l);		break;	/* bit 5,l */
    case 0x6e:	bitm(pcpu, 5);			break;	/* bit 5,(hl) */
    case 0x6f:	bit(pcpu, 5, pcpu->a);		break;	/* bit 5,a */
    case 0x70:	bit(pcpu, 6, pcpu->b);		break;	/* bit 6,b */
    case 0x71:	bit(pcpu, 6, pcpu->c);		break;	/* bit 6,c */
    case 0x72:	bit(pcpu, 6, pcpu->d);		break;	/* bit 6,d */
    case 0x73:	bit(pcpu, 6, pcpu->e);		break;	/* bit 6,e */
    case 0x74:	bit(pcpu, 6, pcpu->h);		break;	/* bit 6,h */
    case 0x75:	bit(pcpu, 6, pcpu->l);		break;	/* bit 6,l */
    case 0x76:	bitm(pcpu, 6);			break;	/* bit 6,(hl) */
    case 0x77:	bit(pcpu, 6, pcpu->a);		break;	/* bit 6,a */
    case 0x78:	bit(pcpu, 7, pcpu->b);		break;	/* bit 7,b */
    case 0x79:	bit(pcpu, 7, pcpu->c);		break;	/* bit 7,c */
    case 0x7a:	bit(pcpu, 7, pcpu->d);		break;	/* bit 7,d */
    case 0x7b:	bit(pcpu, 7, pcpu->e);		break;	/* bit 7,e */
    case 0x7c:	bit(pcpu, 7, pcpu->h);		break;	/* bit 7,h */
    case 0x7d:	bit(pcpu, 7, pcpu->l);		break;	/* bit 7,l */
    case 0x7e:	bitm(pcpu, 7);			break;	/* bit 7,(hl) */
    case 0x7f:	bit(pcpu, 7, pcpu->a);		break;	/* bit 7,a */

    case 0x80:	pcpu->b &= ~(1 << 0);		break;	/* res 0,b */
    case 0x81:	pcpu->c &= ~(1 << 0);		break;	/* res 0,c */
    case 0x82:	pcpu->d &= ~(1 << 0);		break;	/* res 0,d */
    case 0x83:	pcpu->e &= ~(1 << 0);		break;	/* res 0,e */
    case 0x84:	pcpu->h &= ~(1 << 0);		break;	/* res 0,h */
    case 0x85:	pcpu->l &= ~(1 << 0);		break;	/* res 0,l */
    case 0x86:	resm(pcpu, 0, HL, 0);		break;	/* res 0,(hl) */
    case 0x87:	pcpu->a &= ~(1 << 0);		break;	/* res 0,a */
    case 0x88:	pcpu->b &= ~(1 << 1);		break;	/* res 1,b */
    case 0x89:	pcpu->c &= ~(1 << 1);		break;	/* res 1,c */
    case 0x8a:	pcpu->d &= ~(1 << 1);		break;	/* res 1,d */
    case 0x8b:	pcpu->e &= ~(1 << 1);		break;	/* res 1,e */
    case 0x8c:	pcpu->h &= ~(1 << 1);		break;	/* res 1,h */
    case 0x8d:	pcpu->l &= ~(1 << 1);		break;	/* res 1,l */
    case 0x8e:	resm(pcpu, 1, HL, 0);		break;	/* res 1,(hl) */
    case 0x8f:	pcpu->a &= ~(1 << 1);		break;	/* res 1,a */
    case 0x90:	pcpu->b &= ~(1 << 2);		break;	/* res 2,b */
    case 0x91:	pcpu->c &= ~(1 << 2);		break;	/* res 2,c */
    case 0x92:	pcpu->d &= ~(1 << 2);		break;	/* res 2,d */
    case 0x93:	pcpu->e &= ~(1 << 2);		break;	/* res 2,e */
    case 0x94:	pcpu->h &= ~(1 << 2);		break;	/* res 2,h */
    case 0x95:	pcpu->l &= ~(1 << 2);		break;	/* res 2,l */
    case 0x96:	resm(pcpu, 2, HL, 0);		break;	/* res 2,(hl) */
    case 0x97:	pcpu->a &= ~(1 << 2);		break;	/* res 2,a */
    case 0x98:	pcpu->b &= ~(1 << 3);		break;	/* res 3,b */
    case 0x99:	pcpu->c &= ~(1 << 3);		break;	/* res 3,c */
    case 0x9a:	pcpu->d &= ~(1 << 3);		break;	/* res 3,d */
    case 0x9b:	pcpu->e &= ~(1 << 3);		break;	/* res 3,e */
    case 0x9c:	pcpu->h &= ~(1 << 3);		break;	/* res 3,h */
    case 0x9d:	pcpu->l &= ~(1 << 3);		break;	/* res 3,l */
    case 0x9e:	resm(pcpu, 3, HL, 0);		break;	/* res 3,(hl) */
    case 0x9f:	pcpu->a &= ~(1 << 3);		break;	/* res 3,a */
    case 0xa0:	pcpu->b &= ~(1 << 4);		break;	/* res 4,b */
    case 0xa1:	pcpu->c &= ~(1 << 4);		break;	/* res 4,c */
    case 0xa2:	pcpu->d &= ~(1 << 4);		break;	/* res 4,d */
    case 0xa3:	pcpu->e &= ~(1 << 4);		break;	/* res 4,e */
    case 0xa4:	pcpu->h &= ~(1 << 4);		break;	/* res 4,h */
    case 0xa5:	pcpu->l &= ~(1 << 4);		break;	/* res 4,l */
    case 0xa6:	resm(pcpu, 4, HL, 0);		break;	/* res 4,(hl) */
    case 0xa7:	pcpu->a &= ~(1 << 4);		break;	/* res 4,a */
    case 0xa8:	pcpu->b &= ~(1 << 5);		break;	/* res 5,b */
    case 0xa9:	pcpu->c &= ~(1 << 5);		break;	/* res 5,c */
    case 0xaa:	pcpu->d &= ~(1 << 5);		break;	/* res 5,d */
    case 0xab:	pcpu->e &= ~(1 << 5);		break;	/* res 5,e */
    case 0xac:	pcpu->h &= ~(1 << 5);		break;	/* res 5,h */
    case 0xad:	pcpu->l &= ~(1 << 5);		break;	/* res 5,l */
    case 0xae:	resm(pcpu, 5, HL, 0);		break;	/* res 5,(hl) */
    case 0xaf:	pcpu->a &= ~(1 << 5);		break;	/* res 5,a */
    case 0xb0:	pcpu->b &= ~(1 << 6);		break;	/* res 6,b */
    case 0xb1:	pcpu->c &= ~(1 << 6);		break;	/* res 6,c */
    case 0xb2:	pcpu->d &= ~(1 << 6);		break;	/* res 6,d */
    case 0xb3:	pcpu->e &= ~(1 << 6);		break;	/* res 6,e */
    case 0xb4:	pcpu->h &= ~(1 << 6);		break;	/* res 6,h */
    case 0xb5:	pcpu->l &= ~(1 << 6);		break;	/* res 6,l */
    case 0xb6:	resm(pcpu, 6, HL, 0);		break;	/* res 6,(hl) */
    case 0xb7:	pcpu->a &= ~(1 << 6);		break;	/* res 6,a */
    case 0xb8:	pcpu->b &= ~(1 << 7);		break;	/* res 7,b */
    case 0xb9:	pcpu->c &= ~(1 << 7);		break;	/* res 7,c */
    case 0xba:	pcpu->d &= ~(1 << 7);		break;	/* res 7,d */
    case 0xbb:	pcpu->e &= ~(1 << 7);		break;	/* res 7,e */
    case 0xbc:	pcpu->h &= ~(1 << 7);		break;	/* res 7,h */
    case 0xbd:	pcpu->l &= ~(1 << 7);		break;	/* res 7,l */
    case 0xbe:	resm(pcpu, 7, HL, 0);		break;	/* res 7,(hl) */
    case 0xbf:	pcpu->a &= ~(1 << 7);		break;	/* res 7,a */

    case 0xc0:	pcpu->b |= (1 << 0);		break;	/* set 0,b */
    case 0xc1:	pcpu->c |= (1 << 0);		break;	/* set 0,c */
    case 0xc2:	pcpu->d |= (1 << 0);		break;	/* set 0,d */
    case 0xc3:	pcpu->e |= (1 << 0);		break;	/* set 0,e */
    case 0xc4:	pcpu->h |= (1 << 0);		break;	/* set 0,h */
    case 0xc5:	pcpu->l |= (1 << 0);		break;	/* set 0,l */
    case 0xc6:	setm(pcpu, 0, HL, 0);		break;	/* set 0,(hl) */
    case 0xc7:	pcpu->a |= (1 << 0);		break;	/* set 0,a */
    case 0xc8:	pcpu->b |= (1 << 1);		break;	/* set 1,b */
    case 0xc9:	pcpu->c |= (1 << 1);		break;	/* set 1,c */
    case 0xca:	pcpu->d |= (1 << 1);		break;	/* set 1,d */
    case 0xcb:	pcpu->e |= (1 << 1);		break;	/* set 1,e */
    case 0xcc:	pcpu->h |= (1 << 1);		break;	/* set 1,h */
    case 0xcd:	pcpu->l |= (1 << 1);		break;	/* set 1,l */
    case 0xce:	setm(pcpu, 1, HL, 0);		break;	/* set 1,(hl) */
    case 0xcf:	pcpu->a |= (1 << 1);		break;	/* set 1,a */
    case 0xd0:	pcpu->b |= (1 << 2);		break;	/* set 2,b */
    case 0xd1:	pcpu->c |= (1 << 2);		break;	/* set 2,c */
    case 0xd2:	pcpu->d |= (1 << 2);		break;	/* set 2,d */
    case 0xd3:	pcpu->e |= (1 << 2);		break;	/* set 2,e */
    case 0xd4:	pcpu->h |= (1 << 2);		break;	/* set 2,h */
    case 0xd5:	pcpu->l |= (1 << 2);		break;	/* set 2,l */
    case 0xd6:	setm(pcpu, 2, HL, 0);		break;	/* set 2,(hl) */
    case 0xd7:	pcpu->a |= (1 << 2);		break;	/* set 2,a */
    case 0xd8:	pcpu->b |= (1 << 3);		break;	/* set 3,b */
    case 0xd9:	pcpu->c |= (1 << 3);		break;	/* set 3,c */
    case 0xda:	pcpu->d |= (1 << 3);		break;	/* set 3,d */
    case 0xdb:	pcpu->e |= (1 << 3);		break;	/* set 3,e */
    case 0xdc:	pcpu->h |= (1 << 3);		break;	/* set 3,h */
    case 0xdd:	pcpu->l |= (1 << 3);		break;	/* set 3,l */
    case 0xde:	setm(pcpu, 3, HL, 0);		break;	/* set 3,(hl) */
    case 0xdf:	pcpu->a |= (1 << 3);		break;	/* set 3,a */
    case 0xe0:	pcpu->b |= (1 << 4);		break;	/* set 4,b */
    case 0xe1:	pcpu->c |= (1 << 4);		break;	/* set 4,c */
    case 0xe2:	pcpu->d |= (1 << 4);		break;	/* set 4,d */
    case 0xe3:	pcpu->e |= (1 << 4);		break;	/* set 4,e */
    case 0xe4:	pcpu->h |= (1 << 4);		break;	/* set 4,h */
    case 0xe5:	pcpu->l |= (1 << 4);		break;	/* set 4,l */
    case 0xe6:	setm(pcpu, 4, HL, 0);		break;	/* set 4,(hl) */
    case 0xe7:	pcpu->a |= (1 << 4);		break;	/* set 4,a */
    case 0xe8:	pcpu->b |= (1 << 5);		break;	/* set 5,b */
    case 0xe9:	pcpu->c |= (1 << 5);		break;	/* set 5,c */
    case 0xea:	pcpu->d |= (1 << 5);		break;	/* set 5,d */
    case 0xeb:	pcpu->e |= (1 << 5);		break;	/* set 5,e */
    case 0xec:	pcpu->h |= (1 << 5);		break;	/* set 5,h */
    case 0xed:	pcpu->l |= (1 << 5);		break;	/* set 5,l */
    case 0xee:	setm(pcpu, 5, HL, 0);		break;	/* set 5,(hl) */
    case 0xef:	pcpu->a |= (1 << 5);		break;	/* set 5,a */
    case 0xf0:	pcpu->b |= (1 << 6);		break;	/* set 6,b */
    case 0xf1:	pcpu->c |= (1 << 6);		break;	/* set 6,c */
    case 0xf2:	pcpu->d |= (1 << 6);		break;	/* set 6,d */
    case 0xf3:	pcpu->e |= (1 << 6);		break;	/* set 6,e */
    case 0xf4:	pcpu->h |= (1 << 6);		break;	/* set 6,h */
    case 0xf5:	pcpu->l |= (1 << 6);		break;	/* set 6,l */
    case 0xf6:	setm(pcpu, 6, HL, 0);		break;	/* set 6,(hl) */
    case 0xf7:	pcpu->a |= (1 << 6);		break;	/* set 6,a */
    case 0xf8:	pcpu->b |= (1 << 7);		break;	/* set 7,b */
    case 0xf9:	pcpu->c |= (1 << 7);		break;	/* set 7,c */
    case 0xfa:	pcpu->d |= (1 << 7);		break;	/* set 7,d */
    case 0xfb:	pcpu->e |= (1 << 7);		break;	/* set 7,e */
    case 0xfc:	pcpu->h |= (1 << 7);		break;	/* set 7,h */
    case 0xfd:	pcpu->l |= (1 << 7);		break;	/* set 7,l */
    case 0xfe:	setm(pcpu, 7, HL, 0);		break;	/* set 7,(hl) */
    case 0xff:	pcpu->a |= (1 << 7);		break;	/* set 7,a */
    }
    break;						/* CB prefix end */

  case 0xed:						/* ED prefix */
    if (pcpu->mcycle == 1) {
      break;
    } else if (pcpu->mcycle == 2) {
      pcpu->op2 = fetch(pcpu);
    }
    pcpu->cycles += tstates_ed[pcpu->op2][pcpu->mcycle - 2];

    switch (pcpu->op2) {
    case 0x40:						/* in b,(c) */
      pcpu->b = z80_in(BC);
      pcpu->f = newflags[pcpu->b] | (pcpu->f & CF);
      break;

    case 0x48:						/* in c,(c) */
      pcpu->c = z80_in(BC);
      pcpu->f = newflags[pcpu->c] | (pcpu->f & CF);
      break;

    case 0x50:						/* in d,(c) */
      pcpu->d = z80_in(BC);
      pcpu->f = newflags[pcpu->d] | (pcpu->f & CF);
      break;

    case 0x58:						/* in e,(c) */
      pcpu->e = z80_in(BC);
      pcpu->f = newflags[pcpu->e] | (pcpu->f & CF);
      break;

    case 0x60:						/* in h,(c) */
      pcpu->h = z80_in(BC);
      pcpu->f = newflags[pcpu->h] | (pcpu->f & CF);
      break;

    case 0x68:						/* in l,(c) */
      pcpu->l = z80_in(BC);
      pcpu->f = newflags[pcpu->l] | (pcpu->f & CF);
      break;

    case 0x70:						/* in f,(c) */
      pcpu->f = newflags[z80_in(BC)] | (pcpu->f & CF);
      break;

    case 0x78:						/* in a,(c) */
      pcpu->a = z80_in(BC);
      pcpu->f = newflags[pcpu->a] | (pcpu->f & CF);
      break;

    case 0x41:	z80_out(BC, pcpu->b);		break;	/* out (c),b */
    case 0x49:	z80_out(BC, pcpu->c);		break;	/* out (c),c */
    case 0x51:	z80_out(BC, pcpu->d);		break;	/* out (c),d */
    case 0x59:	z80_out(BC, pcpu->e);		break;	/* out (c),e */
    case 0x61:	z80_out(BC, pcpu->h);		break;	/* out (c),h */
    case 0x69:	z80_out(BC, pcpu->l);		break;	/* out (c),l */
    case 0x71:	z80_out(BC, 0);			break;	/* out (c),0 */
    case 0x79:	z80_out(BC, pcpu->a);		break;	/* out (c),a */

    case 0x42:	sbchl(pcpu, BC);		break;	/* sbc hl,bc */
    case 0x52:	sbchl(pcpu, DE);		break;	/* sbc hl,de */
    case 0x62:	sbchl(pcpu, HL);		break;	/* sbc hl,hl */
    case 0x72:	sbchl(pcpu, pcpu->sp);		break;	/* sbc hl,sp */

    case 0x4a:	adchl(pcpu, BC);		break;	/* adc hl,bc */
    case 0x5a:	adchl(pcpu, DE);		break;	/* adc hl,de */
    case 0x6a:	adchl(pcpu, HL);		break;	/* adc hl,hl */
    case 0x7a:	adchl(pcpu, pcpu->sp);		break;	/* adc hl,sp */

    case 0x43:						/* ld (nn),bc */
      switch (pcpu->mcycle) {
      case 2:
	break;
      case 3:
	tmp16 = z80_read(pcpu->pc++);
	break;
      case 4:
	tmp16 += z80_read(pcpu->pc++) << 8;
	break;
      case 5:
	z80_write(tmp16    , pcpu->c);
	break;
      case 6:
	z80_write(tmp16 + 1, pcpu->b);
	break;
      }
      break;

    case 0x53:						/* ld (nn),de */
      switch (pcpu->mcycle) {
      case 2:
	break;
      case 3:
	tmp16 = z80_read(pcpu->pc++);
	break;
      case 4:
	tmp16 += z80_read(pcpu->pc++) << 8;
	break;
      case 5:
	z80_write(tmp16    , pcpu->e);
	break;
      case 6:
	z80_write(tmp16 + 1, pcpu->d);
	break;
      }
      break;

    case 0x63:						/* ld (nn),hl */
      switch (pcpu->mcycle) {
      case 2:
	break;
      case 3:
	tmp16 = z80_read(pcpu->pc++);
	break;
      case 4:
	tmp16 += z80_read(pcpu->pc++) << 8;
	break;
      case 5:
	z80_write(tmp16    , pcpu->l);
	break;
      case 6:
	z80_write(tmp16 + 1, pcpu->h);
	break;
      }
      break;

    case 0x73:						/* ld (nn),sp */
      switch (pcpu->mcycle) {
      case 2:
	break;
      case 3:
	tmp16 = z80_read(pcpu->pc++);
	break;
      case 4:
	tmp16 += z80_read(pcpu->pc++) << 8;
	break;
      case 5:
	z80_write(tmp16    , pcpu->sp & 0xff);
	break;
      case 6:
	z80_write(tmp16 + 1, pcpu->sp >> 8);
	break;
      }
      break;

    case 0x44:	case 0x4c:				/* neg */
    case 0x54:	case 0x5c:
    case 0x64:	case 0x6c:
    case 0x74:	case 0x7c:
      tmp8 = 0x100 - pcpu->a;
      pcpu->f
	= (tmp8 & SF)
	| (tmp8 ? 0 : ZF)
	| (tmp8 & YF)
	| ((pcpu->a ^ tmp8) & HF)
	| (tmp8 & XF)
	| ((pcpu->a == 0x80) ? VF : 0)
	| NF
	| ((pcpu->a > 0) ? CF : 0);
      pcpu->a = tmp8;
      break;

    case 0x45:						/* retn */
    case 0x55:	case 0x5d:
    case 0x65:	case 0x6d:
    case 0x75:	case 0x7d:
    case 0x4d:						/* reti */
      pcpu->iff1 = pcpu->iff2;
      retcnd(pcpu, 0, 0, -1);
      break;

    case 0x46:	case 0x4e:				/* im 0 */
    case 0x66:	case 0x6e:
      pcpu->im = 0;
      break;
      
    case 0x56:	case 0x76:				/* im 1 */
      pcpu->im = 1;
      break;

    case 0x5e:	case 0x7e:				/* im 2 */
      pcpu->im = 2;
      break;

    case 0x47:						/* ld i,a */
      pcpu->i = pcpu->a;
      break;

    case 0x4f:						/* ld r,a */
      pcpu->r = pcpu->a;
      break;

    case 0x57:						/* ld a,i */
      pcpu->a = pcpu->i;
      pcpu->f
	= (pcpu->a & SF)
	| (pcpu->a ? 0 : ZF)
	| (pcpu->a & YF)
	| 0
	| (pcpu->a & XF)
	| (pcpu->iff2 ? PF : 0)
	| 0
	| (pcpu->f & CF);
      break;

    case 0x5f:						/* ld a,r */
      pcpu->a = pcpu->r;
      pcpu->f
	= (pcpu->a & SF)
	| (pcpu->a ? 0 : ZF)
	| (pcpu->a & YF)
	| 0
	| (pcpu->a & XF)
	| (pcpu->iff2 ? PF : 0)
	| 0
	| (pcpu->f & CF);
      break;

    case 0x4b:						/* ld bc,(nn) */
      switch (pcpu->mcycle) {
      case 2:
	break;
      case 3:
	tmp16 = z80_read(pcpu->pc++);
	break;
      case 4:
	tmp16 += z80_read(pcpu->pc++) << 8;
	break;
      case 5:
	pcpu->c = z80_read(tmp16);
	break;
      case 6:
	pcpu->b = z80_read(tmp16 + 1);
	break;
      }
      break;
	
    case 0x5b:						/* ld de,(nn) */
      switch (pcpu->mcycle) {
      case 2:
	break;
      case 3:
	tmp16 = z80_read(pcpu->pc++);
	break;
      case 4:
	tmp16 += z80_read(pcpu->pc++) << 8;
	break;
      case 5:
	pcpu->e = z80_read(tmp16);
	break;
      case 6:
	pcpu->d = z80_read(tmp16 + 1);
	break;
      }
      break;

    case 0x6b:						/* ld hl,(nn) */
      switch (pcpu->mcycle) {
      case 2:
	break;
      case 3:
	tmp16 = z80_read(pcpu->pc++);
	break;
      case 4:
	tmp16 += z80_read(pcpu->pc++) << 8;
	break;
      case 5:
	pcpu->l = z80_read(tmp16);
	break;
      case 6:
	pcpu->h = z80_read(tmp16 + 1);
	break;
      }
      break;

    case 0x7b:						/* ld sp,(nn) */
      switch (pcpu->mcycle) {
      case 2:
	break;
      case 3:
	tmp16 = z80_read(pcpu->pc++);
	break;
      case 4:
	tmp16 += z80_read(pcpu->pc++) << 8;
	break;
      case 5:
	pcpu->sp = z80_read(tmp16);
	break;
      case 6:
	pcpu->sp += z80_read(tmp16 + 1) << 8;
	break;
      }
      break;

    case 0x67:							/* rrd */
      switch (pcpu->mcycle) {
      case 2:
	break;
      case 3:
	tmp8 = z80_read(HL);
	break;
      case 4:
	break;
      case 5:
	z80_write(HL, (tmp8 >> 4) + (pcpu->a << 4));
	pcpu->a = (pcpu->a & 0xf0) + (tmp8 & 0x0f);
	pcpu->f = newflags[pcpu->a] | (pcpu->f & CF);
	break;
      }
      break;

    case 0x6f:							/* rld */
      switch (pcpu->mcycle) {
      case 2:
	break;
      case 3:
	tmp8 = z80_read(HL);
	break;
      case 4:
	break;
      case 5:
	z80_write(HL, (tmp8 << 4) + (pcpu->a & 0x0f));
	pcpu->a = (pcpu->a & 0xf0) + (tmp8 >> 4);
	pcpu->f = newflags[pcpu->a] | (pcpu->f & CF);
	break;
      }
      break;

    case 0x77:	case 0x7f:					/* nop */
      break;

    case 0xa0:	ldi(pcpu);				break;	/* ldi */
    case 0xa1:	cpi(pcpu);				break;	/* cpi */
    case 0xa2:	ini(pcpu);				break;	/* ini */
    case 0xa3:	outi(pcpu);				break;	/* outi */

    case 0xa8:	ldd(pcpu);				break;	/* ldd */
    case 0xa9:	cpd(pcpu);				break;	/* cpd */
    case 0xaa:	ind(pcpu);				break;	/* ind */
    case 0xab:	outd(pcpu);				break;	/* outd */

    case 0xb0:	ldi(pcpu);	repbc(pcpu);		break;	/* ldir */
    case 0xb1:	cpi(pcpu);	repcp(pcpu);		break;	/* cpir */
    case 0xb2:	ini(pcpu);	repb(pcpu);		break;	/* inir */
    case 0xb3:	outi(pcpu);	repb(pcpu);		break;	/* otir */

    case 0xb8:	ldd(pcpu);	repbc(pcpu);		break;	/* lddr */
    case 0xb9:	cpd(pcpu);	repcp(pcpu);		break;	/* cpdr */
    case 0xba:	ind(pcpu);	repb(pcpu);		break;	/* indr */
    case 0xbb:	outd(pcpu);	repb(pcpu);		break;	/* otdr */

    default:
      break;
    }
    break;						/* ED prefix end */

  case 0xdd:						/* DD prefix */
    if (pcpu->mcycle == 1) {
      break;
    } else if (pcpu->mcycle == 2) {
      pcpu->op2 = fetch(pcpu);
    }
    pcpu->cycles += tstates_dd[pcpu->op2][pcpu->mcycle - 2];

    switch (pcpu->op2) {
    case 0x09:addw(pcpu, &pcpu->ixh, &pcpu->ixl, BC);	break;	/* add ix,bc */
    case 0x19:addw(pcpu, &pcpu->ixh, &pcpu->ixl, DE);	break;	/* add ix,bc */
    case 0x29:addw(pcpu, &pcpu->ixh, &pcpu->ixl, IX);	break;	/* add ix,ix */
    case 0x39:addw(pcpu, &pcpu->ixh, &pcpu->ixl, pcpu->sp);	break;	/* add ix,sp */
      
    case 0x21:						/* ld ix,nn */
      switch (pcpu->mcycle) {
      case 2:
	break;
      case 3:
	pcpu->ixl = z80_read(pcpu->pc++);
	break;
      case 4:
	pcpu->ixh = z80_read(pcpu->pc++);
	break;
      }
      break;

    case 0x22:						/* ld (nn),ix */
      switch (pcpu->mcycle) {
      case 2:
	break;
      case 3:
	tmp16 = z80_read(pcpu->pc++);
	break;
      case 4:
	tmp16 += z80_read(pcpu->pc++) << 8;
	break;
      case 5:
	z80_write(tmp16,     pcpu->ixl);
	break;
      case 6:
	z80_write(tmp16 + 1, pcpu->ixh);
	break;
      }
      break;

    case 0x23:	incw(&pcpu->ixh, &pcpu->ixl);	break;	/* inc ix */
    case 0x2b:	decw(&pcpu->ixh, &pcpu->ixl);	break;	/* dec ix */
    case 0x24:	inc(pcpu, &pcpu->ixh);		break;	/* inc ixh */
    case 0x2c:	inc(pcpu, &pcpu->ixl);		break;	/* inc ixl */
    case 0x25:	dec(pcpu, &pcpu->ixh);		break;	/* dec ixh */
    case 0x2d:	dec(pcpu, &pcpu->ixl);		break;	/* dec ixl */

    case 0x26:						/* ld ixh,n */
      switch (pcpu->mcycle) {
      case 2:
	break;
      case 3:
	pcpu->ixh = z80_read(pcpu->pc++);
	break;
      }
      break;
    case 0x2e:						/* ld ixl,n */
      switch (pcpu->mcycle) {
      case 2:
	break;
      case 3:
	pcpu->ixl = z80_read(pcpu->pc++);
	break;
      }
      break;
    case 0x2a:						/* ld ix,(nn) */
      switch (pcpu->mcycle) {
      case 2:
	break;
      case 3:
	tmp16 = z80_read(pcpu->pc++);
	break;
      case 4:
	tmp16 += z80_read(pcpu->pc++) << 8;
	break;
      case 5:
	pcpu->ixl = z80_read(tmp16);
	break;
      case 6:
	pcpu->ixh = z80_read(tmp16 + 1);
	break;
      }
      break;

    case 0x34:						/* inc (ix+d) */
      switch (pcpu->mcycle) {
      case 2:							break;
      case 3:	tmp8 = z80_read(pcpu->pc++);			break;
      case 4:	tmp16 = (word)((int32_t)IX + (int8_t)tmp8);	break;    
      case 5:	tmp8 = z80_read(tmp16);				break;
      case 6:	inc(pcpu, &tmp8);	z80_write(tmp16, tmp8);	break;
      }
      break;

    case 0x35:						/* dec (ix+d) */
      switch (pcpu->mcycle) {
      case 2:							break;
      case 3:	tmp8 = z80_read(pcpu->pc++);			break;
      case 4:	tmp16 = (word)((int32_t)IX + (int8_t)tmp8);	break;
      case 5:	tmp8 = z80_read(tmp16);				break;
      case 6:	dec(pcpu, &tmp8);	z80_write(tmp16, tmp8);	break;
      }
      break;

    case 0x36:						/* ld (ix+d),n */
      switch (pcpu->mcycle) {
      case 2:
	break;
      case 3:
	tmp8 = z80_read(pcpu->pc++);
	break;
      case 4:
	tmp16 = (word)((int32_t)IX + (int8_t)tmp8);
	tmp8 = z80_read(pcpu->pc++);
	break;
      case 5:
	z80_write(tmp16, tmp8);
	break;
      }
      break;

    case 0x44:	pcpu->b = pcpu->ixh;	 	break;	/* ld b,ixh */
    case 0x45:	pcpu->b = pcpu->ixl; 		break;	/* ld b,ixl */
    case 0x46:	ldrix(pcpu, &pcpu->b);		break;	/* ld b,(ix+d) */
    case 0x4c:	pcpu->c = pcpu->ixh; 		break;	/* ld c,ixh */
    case 0x4d:	pcpu->c = pcpu->ixl; 		break;	/* ld c,ixl */
    case 0x4e:	ldrix(pcpu, &pcpu->c);		break;	/* ld c,(ix+d) */
    case 0x54:	pcpu->d = pcpu->ixh; 		break;	/* ld d,ixh */
    case 0x55:	pcpu->d = pcpu->ixl; 		break;	/* ld d,ixl */
    case 0x56:	ldrix(pcpu, &pcpu->d);		break;	/* ld d,(ix+d) */
    case 0x5c:	pcpu->e = pcpu->ixh; 		break;	/* ld e,ixh */
    case 0x5d:	pcpu->e = pcpu->ixl; 		break;	/* ld e,ixl */
    case 0x5e:	ldrix(pcpu, &pcpu->e);		break;	/* ld e,(ix+d) */
    case 0x60:	pcpu->ixh = pcpu->b; 		break;	/* ld ixh,b */
    case 0x61:	pcpu->ixh = pcpu->c; 		break;	/* ld ixh,c */
    case 0x62:	pcpu->ixh = pcpu->d; 		break;	/* ld ixh,d */
    case 0x63:	pcpu->ixh = pcpu->e; 		break;	/* ld ixh,e */
    case 0x64:	pcpu->ixh = pcpu->ixh;		break;	/* ld ixh,ixh */
    case 0x65:	pcpu->ixh = pcpu->ixl;		break;	/* ld ixh,ixl */
    case 0x66:	ldrix(pcpu, &pcpu->h);		break;	/* ld h,(ix+d) */
    case 0x67:	pcpu->ixh = pcpu->a; 		break;	/* ld ixh,a */

    case 0x68:	pcpu->ixl = pcpu->b; 		break;	/* ld ixl,b */
    case 0x69:	pcpu->ixl = pcpu->c; 		break;	/* ld ixl,c */
    case 0x6a:	pcpu->ixl = pcpu->d; 		break;	/* ld ixl,d */
    case 0x6b:	pcpu->ixl = pcpu->e;		break;	/* ld ixl,e */
    case 0x6c:	pcpu->ixl = pcpu->ixh;		break;	/* ld ixl,ixh */
    case 0x6d:	pcpu->ixl = pcpu->ixl;		break;	/* ld ixl,ixl */
    case 0x6e:	ldrix(pcpu, &pcpu->l);		break;	/* ld l,(ix+d) */
    case 0x6f:	pcpu->ixl = pcpu->a;		break;	/* ld ixl,a */

    case 0x70:	ldixr(pcpu, pcpu->b);		break;	/* ld (ix+d),b */
    case 0x71:	ldixr(pcpu, pcpu->c);		break;	/* ld (ix+d),c */
    case 0x72:	ldixr(pcpu, pcpu->d);		break;	/* ld (ix+d),d */
    case 0x73:	ldixr(pcpu, pcpu->e);		break;	/* ld (ix+d),e */
    case 0x74:	ldixr(pcpu, pcpu->h);		break;	/* ld (ix+d),h */
    case 0x75:	ldixr(pcpu, pcpu->l);		break;	/* ld (ix+d),l */
    case 0x77:	ldixr(pcpu, pcpu->a);		break;	/* ld (ix+d),a */

    case 0x7c:	pcpu->a = pcpu->ixh;		break;	/* ld a,ixh */
    case 0x7d:	pcpu->a = pcpu->ixl;		break;	/* ld a,ixl */
    case 0x7e:	ldrix(pcpu, &pcpu->a);		break;	/* ld a,(ix+d) */

    case 0x84:	add(pcpu, pcpu->ixh);		break;	/* add a,ixh */
    case 0x85:	add(pcpu, pcpu->ixl);		break;	/* add a,ixl */
    case 0x86:						/* add a,(ix+d) */
      switch (pcpu->mcycle) {
      case 2:							break;
      case 3:	tmp8 = z80_read(pcpu->pc++);			break;
      case 4:	tmp16 = (word)((int32_t)IX + (int8_t)tmp8);	break;
      case 5:	add(pcpu, z80_read(tmp16));			break;
      }
      break;

    case 0x8c:	adc(pcpu, pcpu->ixh);		break;	/* adc a,ixh */
    case 0x8d:	adc(pcpu, pcpu->ixl);		break;	/* adc a,ixl */
    case 0x8e:						/* adc a,(ix+d) */
      switch (pcpu->mcycle) {
      case 2:							break;
      case 3:	tmp8 = z80_read(pcpu->pc++);			break;
      case 4:	tmp16 = (word)((int32_t)IX + (int8_t)tmp8);	break;
      case 5:	adc(pcpu, z80_read(tmp16));			break;
      }
      break;

    case 0x94:	sub(pcpu, pcpu->ixh);		break;	/* sub ixh */
    case 0x95:	sub(pcpu, pcpu->ixl);		break;	/* sub ixl */
    case 0x96:						/* sub (ix+d) */
      switch (pcpu->mcycle) {
      case 2:							break;
      case 3:	tmp8 = z80_read(pcpu->pc++);			break;
      case 4:	tmp16 = (word)((int32_t)IX + (int8_t)tmp8);	break;
      case 5:	sub(pcpu, z80_read(tmp16));			break;
      }
      break;

    case 0x9c:	sbc(pcpu, pcpu->ixh);		break;	/* sbc a,ixh */
    case 0x9d:	sbc(pcpu, pcpu->ixl);		break;	/* sbc a,ixl */
    case 0x9e:						/* sbc (ix+d) */
      switch (pcpu->mcycle) {
      case 2:							break;
      case 3:	tmp8 = z80_read(pcpu->pc++);			break;
      case 4:	tmp16 = (word)((int32_t)IX + (int8_t)tmp8);	break;
      case 5:	sbc(pcpu, z80_read(tmp16));			break;
      }
      break;

    case 0xa4:	and(pcpu, pcpu->ixh);		break;	/* and ixh */
    case 0xa5:	and(pcpu, pcpu->ixl);		break;	/* and ixl */
    case 0xa6:						/* and (ix+d) */
      switch (pcpu->mcycle) {
      case 2:							break;
      case 3:	tmp8 = z80_read(pcpu->pc++);			break;
      case 4:	tmp16 = (word)((int32_t)IX + (int8_t)tmp8);	break;
      case 5:	and(pcpu, z80_read(tmp16));			break;
      }
      break;

    case 0xac:	xor(pcpu, pcpu->ixh);		break;	/* xor ixh */
    case 0xad:	xor(pcpu, pcpu->ixl);		break;	/* xor ixl */
    case 0xae:						/* xor (ix+d) */
      switch (pcpu->mcycle) {
      case 2:							break;
      case 3:	tmp8 = z80_read(pcpu->pc++);			break;
      case 4:	tmp16 = (word)((int32_t)IX + (int8_t)tmp8);	break;
      case 5:	xor(pcpu, z80_read(tmp16));			break;
      }
      break;

    case 0xb4:	or(pcpu, pcpu->ixh);		break;	/* or ixh */
    case 0xb5:	or(pcpu, pcpu->ixl);		break;	/* or ixl */
    case 0xb6:						/* or (ix+d) */
      switch (pcpu->mcycle) {
      case 2:							break;
      case 3:	tmp8 = z80_read(pcpu->pc++);			break;
      case 4:	tmp16 = (word)((int32_t)IX + (int8_t)tmp8);	break;
      case 5:	or(pcpu, z80_read(tmp16));			break;
      }
      break;

    case 0xbc:	cp(pcpu, pcpu->ixh);		break;	/* cp ixh */
    case 0xbd:	cp(pcpu, pcpu->ixl);		break;	/* cp ixl */
    case 0xbe:						/* cp (ix+d) */
      switch (pcpu->mcycle) {
      case 2:							break;
      case 3:	tmp8 = z80_read(pcpu->pc++);			break;
      case 4:	tmp16 = (word)((int32_t)IX + (int8_t)tmp8);	break;
      case 5:	cp(pcpu, z80_read(tmp16));			break;
      }
      break;

    case 0xe1:	pop(pcpu, &pcpu->ixh, &pcpu->ixl);	break;	/* pop ix */

    case 0xe3:						/* ex (sp),ix */
      switch (pcpu->mcycle) {
      case 2:							break;
      case 3:	tmp16 = z80_read(pcpu->sp++);			break;
      case 4:	tmp16 += z80_read(pcpu->sp) << 8;		break;
      case 5:	z80_write(pcpu->sp--, pcpu->ixh);		break;
      case 6:
	z80_write(pcpu->sp, pcpu->ixl);
	pcpu->ixl = tmp16 & 0xff;
	pcpu->ixh = tmp16 >> 8;
	break;
      }
      break;

    case 0xe5:	push(pcpu, pcpu->ixh, pcpu->ixl);	break;	/* push ix */

    case 0xe9:	pcpu->pc = IX;				break;	/* jp (ix) */

    case 0xed:							/* ED prefix */
      pcpu->op1 = 0xed;
      break;

    case 0xf9:	pcpu->sp = IX;				break;	/* ld sp,ix */

    case 0xcb:						/* DDCB prefix */
      switch (pcpu->mcycle) {
      case 2:
	break;
      case 3:
	tmp8 = z80_read(pcpu->pc++);
	break;
      case 4:
	tmp16 = (word)((int32_t)IX + (int8_t)tmp8);
	pcpu->op3 = z80_read(pcpu->pc++);
	break;
      case 5:
      case 6:
	switch (pcpu->op3) {
	case 0x00: pcpu->b = mreg(pcpu, rlc, tmp16, 3); break; /*rlc (ix+d),b*/
	case 0x01: pcpu->c = mreg(pcpu, rlc, tmp16, 3); break; /*rlc (ix+d),c*/
	case 0x02: pcpu->d = mreg(pcpu, rlc, tmp16, 3); break; /*rlc (ix+d),d*/
	case 0x03: pcpu->e = mreg(pcpu, rlc, tmp16, 3); break; /*rlc (ix+d),e*/
	case 0x04: pcpu->h = mreg(pcpu, rlc, tmp16, 3); break; /*rlc (ix+d),h*/
	case 0x05: pcpu->l = mreg(pcpu, rlc, tmp16, 3); break; /*rlc (ix+d),l*/
	case 0x06:           mreg(pcpu, rlc, tmp16, 3); break; /*rlc (ix+d)  */
	case 0x07: pcpu->a = mreg(pcpu, rlc, tmp16, 3); break; /*rlc (ix+d),a*/

	case 0x08: pcpu->b = mreg(pcpu, rrc, tmp16, 3); break; /*rrc (ix+d),b*/
	case 0x09: pcpu->c = mreg(pcpu, rrc, tmp16, 3); break; /*rrc (ix+d),c*/
	case 0x0a: pcpu->d = mreg(pcpu, rrc, tmp16, 3); break; /*rrc (ix+d),d*/
	case 0x0b: pcpu->e = mreg(pcpu, rrc, tmp16, 3); break; /*rrc (ix+d),e*/
	case 0x0c: pcpu->h = mreg(pcpu, rrc, tmp16, 3); break; /*rrc (ix+d),h*/
	case 0x0d: pcpu->l = mreg(pcpu, rrc, tmp16, 3); break; /*rrc (ix+d),l*/
	case 0x0e:	     mreg(pcpu, rrc, tmp16, 3); break; /*rrc (ix+d)  */
	case 0x0f: pcpu->a = mreg(pcpu, rrc, tmp16, 3); break; /*rrc (ix+d),a*/

	case 0x10: pcpu->b = mreg(pcpu, rl, tmp16, 3); break; /*rl (ix+d),b*/
	case 0x11: pcpu->c = mreg(pcpu, rl, tmp16, 3); break; /*rl (ix+d),c*/
	case 0x12: pcpu->d = mreg(pcpu, rl, tmp16, 3); break; /*rl (ix+d),d*/
	case 0x13: pcpu->e = mreg(pcpu, rl, tmp16, 3); break; /*rl (ix+d),e*/
	case 0x14: pcpu->h = mreg(pcpu, rl, tmp16, 3); break; /*rl (ix+d),h*/
	case 0x15: pcpu->l = mreg(pcpu, rl, tmp16, 3); break; /*rl (ix+d),l*/
	case 0x16:           mreg(pcpu, rl, tmp16, 3); break; /*rl (ix+d)  */
	case 0x17: pcpu->a = mreg(pcpu, rl, tmp16, 3); break; /*rl (ix+d),a*/

	case 0x18: pcpu->b = mreg(pcpu, rr, tmp16, 3); break; /*rr (ix+d),b*/
	case 0x19: pcpu->c = mreg(pcpu, rr, tmp16, 3); break; /*rr (ix+d),c*/
	case 0x1a: pcpu->d = mreg(pcpu, rr, tmp16, 3); break; /*rr (ix+d),d*/
	case 0x1b: pcpu->e = mreg(pcpu, rr, tmp16, 3); break; /*rr (ix+d),e*/
	case 0x1c: pcpu->h = mreg(pcpu, rr, tmp16, 3); break; /*rr (ix+d),h*/
	case 0x1d: pcpu->l = mreg(pcpu, rr, tmp16, 3); break; /*rr (ix+d),l*/
	case 0x1e:           mreg(pcpu, rr, tmp16, 3); break; /*rr (ix+d)  */
	case 0x1f: pcpu->a = mreg(pcpu, rr, tmp16, 3); break; /*rr (ix+d),a*/

	case 0x20: pcpu->b = mreg(pcpu, sla, tmp16, 3); break; /*sla (ix+d),b*/
	case 0x21: pcpu->c = mreg(pcpu, sla, tmp16, 3); break; /*sla (ix+d),c*/
	case 0x22: pcpu->d = mreg(pcpu, sla, tmp16, 3); break; /*sla (ix+d),d*/
	case 0x23: pcpu->e = mreg(pcpu, sla, tmp16, 3); break; /*sla (ix+d),e*/
	case 0x24: pcpu->h = mreg(pcpu, sla, tmp16, 3); break; /*sla (ix+d),h*/
	case 0x25: pcpu->l = mreg(pcpu, sla, tmp16, 3); break; /*sla (ix+d),l*/
	case 0x26:           mreg(pcpu, sla, tmp16, 3); break; /*sla (ix+d)  */
	case 0x27: pcpu->a = mreg(pcpu, sla, tmp16, 3); break; /*sla (ix+d),a*/

	case 0x28: pcpu->b = mreg(pcpu, sra, tmp16, 3); break; /*sra (ix+d),b*/
	case 0x29: pcpu->c = mreg(pcpu, sra, tmp16, 3); break; /*sra (ix+d),c*/
	case 0x2a: pcpu->d = mreg(pcpu, sra, tmp16, 3); break; /*sra (ix+d),d*/
	case 0x2b: pcpu->e = mreg(pcpu, sra, tmp16, 3); break; /*sra (ix+d),e*/
	case 0x2c: pcpu->h = mreg(pcpu, sra, tmp16, 3); break; /*sra (ix+d),h*/
	case 0x2d: pcpu->l = mreg(pcpu, sra, tmp16, 3); break; /*sra (ix+d),l*/
	case 0x2e:           mreg(pcpu, sra, tmp16, 3); break; /*sra (ix+d)  */
	case 0x2f: pcpu->a = mreg(pcpu, sra, tmp16, 3); break; /*sra (ix+d),a*/

	case 0x30: pcpu->b = mreg(pcpu, sll, tmp16, 3); break; /*sll (ix+d),b*/
	case 0x31: pcpu->c = mreg(pcpu, sll, tmp16, 3); break; /*sll (ix+d),c*/
	case 0x32: pcpu->d = mreg(pcpu, sll, tmp16, 3); break; /*sll (ix+d),d*/
	case 0x33: pcpu->e = mreg(pcpu, sll, tmp16, 3); break; /*sll (ix+d),e*/
	case 0x34: pcpu->h = mreg(pcpu, sll, tmp16, 3); break; /*sll (ix+d),h*/
	case 0x35: pcpu->l = mreg(pcpu, sll, tmp16, 3); break; /*sll (ix+d),l*/
	case 0x36:           mreg(pcpu, sll, tmp16, 3); break; /*sll (ix+d)  */
	case 0x37: pcpu->a = mreg(pcpu, sll, tmp16, 3); break; /*sll (ix+d),a*/

	case 0x38: pcpu->b = mreg(pcpu, srl, tmp16, 3); break; /*srl (ix+d),b*/
	case 0x39: pcpu->c = mreg(pcpu, srl, tmp16, 3); break; /*srl (ix+d),c*/
	case 0x3a: pcpu->d = mreg(pcpu, srl, tmp16, 3); break; /*srl (ix+d),d*/
	case 0x3b: pcpu->e = mreg(pcpu, srl, tmp16, 3); break; /*srl (ix+d),e*/
	case 0x3c: pcpu->h = mreg(pcpu, srl, tmp16, 3); break; /*srl (ix+d),h*/
	case 0x3d: pcpu->l = mreg(pcpu, srl, tmp16, 3); break; /*srl (ix+d),l*/
	case 0x3e:           mreg(pcpu, srl, tmp16, 3); break; /*srl (ix+d)  */
	case 0x3f: pcpu->a = mreg(pcpu, srl, tmp16, 3); break; /*srl (ix+d),a*/

	case 0x40:	case 0x41:	case 0x42:	case 0x43:
	case 0x44:	case 0x45:	case 0x46:	case 0x47:
	case 0x48:	case 0x49:	case 0x4a:	case 0x4b:
	case 0x4c:	case 0x4d:	case 0x4e:	case 0x4f:
	case 0x50:	case 0x51:	case 0x52:	case 0x53:
	case 0x54:	case 0x55:	case 0x56:	case 0x57:
	case 0x58:	case 0x59:	case 0x5a:	case 0x5b:
	case 0x5c:	case 0x5d:	case 0x5e:	case 0x5f:
	case 0x60:	case 0x61:	case 0x62:	case 0x63:
	case 0x64:	case 0x65:	case 0x66:	case 0x67:
	case 0x68:	case 0x69:	case 0x6a:	case 0x6b:
	case 0x6c:	case 0x6d:	case 0x6e:	case 0x6f:
	case 0x70:	case 0x71:	case 0x72:	case 0x73:
	case 0x74:	case 0x75:	case 0x76:	case 0x77:
	case 0x78:	case 0x79:	case 0x7a:	case 0x7b:
	case 0x7c:	case 0x7d:	case 0x7e:	case 0x7f:
	  /* bit n,(ix+d) */
	  bitixy(pcpu, (pcpu->op3 >> 3) & 0x07, tmp16);
	  pcpu->mcycle = 0;
	  break;

	case 0x80: pcpu->b = resm(pcpu, 0, tmp16, 2); break; /*res 0,(ix+d),b*/
	case 0x81: pcpu->c = resm(pcpu, 0, tmp16, 2); break; /*res 0,(ix+d),c*/
	case 0x82: pcpu->d = resm(pcpu, 0, tmp16, 2); break; /*res 0,(ix+d),d*/
	case 0x83: pcpu->e = resm(pcpu, 0, tmp16, 2); break; /*res 0,(ix+d),e*/
	case 0x84: pcpu->h = resm(pcpu, 0, tmp16, 2); break; /*res 0,(ix+d),h*/
	case 0x85: pcpu->l = resm(pcpu, 0, tmp16, 2); break; /*res 0,(ix+d),l*/
	case 0x86:           resm(pcpu, 0, tmp16, 2); break; /*res 0,(ix+d)  */
	case 0x87: pcpu->a = resm(pcpu, 0, tmp16, 2); break; /*res 0,(ix+d),a*/
	case 0x88: pcpu->b = resm(pcpu, 1, tmp16, 2); break; /*res 1,(ix+d),b*/
	case 0x89: pcpu->c = resm(pcpu, 1, tmp16, 2); break; /*res 1,(ix+d),c*/
	case 0x8a: pcpu->d = resm(pcpu, 1, tmp16, 2); break; /*res 1,(ix+d),d*/
	case 0x8b: pcpu->e = resm(pcpu, 1, tmp16, 2); break; /*res 1,(ix+d),e*/
	case 0x8c: pcpu->h = resm(pcpu, 1, tmp16, 2); break; /*res 1,(ix+d),h*/
	case 0x8d: pcpu->l = resm(pcpu, 1, tmp16, 2); break; /*res 1,(ix+d),l*/
	case 0x8e:           resm(pcpu, 1, tmp16, 2); break; /*res 1,(ix+d)  */
	case 0x8f: pcpu->a = resm(pcpu, 1, tmp16, 2); break; /*res 1,(ix+d),a*/
	case 0x90: pcpu->b = resm(pcpu, 2, tmp16, 2); break; /*res 2,(ix+d),b*/
	case 0x91: pcpu->c = resm(pcpu, 2, tmp16, 2); break; /*res 2,(ix+d),c*/
	case 0x92: pcpu->d = resm(pcpu, 2, tmp16, 2); break; /*res 2,(ix+d),d*/
	case 0x93: pcpu->e = resm(pcpu, 2, tmp16, 2); break; /*res 2,(ix+d),e*/
	case 0x94: pcpu->h = resm(pcpu, 2, tmp16, 2); break; /*res 2,(ix+d),h*/
	case 0x95: pcpu->l = resm(pcpu, 2, tmp16, 2); break; /*res 2,(ix+d),l*/
	case 0x96:           resm(pcpu, 2, tmp16, 2); break; /*res 2,(ix+d)  */
	case 0x97: pcpu->a = resm(pcpu, 2, tmp16, 2); break; /*res 2,(ix+d),a*/
	case 0x98: pcpu->b = resm(pcpu, 3, tmp16, 2); break; /*res 3,(ix+d),b*/
	case 0x99: pcpu->c = resm(pcpu, 3, tmp16, 2); break; /*res 3,(ix+d),c*/
	case 0x9a: pcpu->d = resm(pcpu, 3, tmp16, 2); break; /*res 3,(ix+d),d*/
	case 0x9b: pcpu->e = resm(pcpu, 3, tmp16, 2); break; /*res 3,(ix+d),e*/
	case 0x9c: pcpu->h = resm(pcpu, 3, tmp16, 2); break; /*res 3,(ix+d),h*/
	case 0x9d: pcpu->l = resm(pcpu, 3, tmp16, 2); break; /*res 3,(ix+d),l*/
	case 0x9e:           resm(pcpu, 3, tmp16, 2); break; /*res 3,(ix+d)  */
	case 0x9f: pcpu->a = resm(pcpu, 3, tmp16, 2); break; /*res 3,(ix+d),a*/
	case 0xa0: pcpu->b = resm(pcpu, 4, tmp16, 2); break; /*res 4,(ix+d),b*/
	case 0xa1: pcpu->c = resm(pcpu, 4, tmp16, 2); break; /*res 4,(ix+d),c*/
	case 0xa2: pcpu->d = resm(pcpu, 4, tmp16, 2); break; /*res 4,(ix+d),d*/
	case 0xa3: pcpu->e = resm(pcpu, 4, tmp16, 2); break; /*res 4,(ix+d),e*/
	case 0xa4: pcpu->h = resm(pcpu, 4, tmp16, 2); break; /*res 4,(ix+d),h*/
	case 0xa5: pcpu->l = resm(pcpu, 4, tmp16, 2); break; /*res 4,(ix+d),l*/
	case 0xa6:           resm(pcpu, 4, tmp16, 2); break; /*res 4,(ix+d)  */
	case 0xa7: pcpu->a = resm(pcpu, 4, tmp16, 2); break; /*res 4,(ix+d),a*/
	case 0xa8: pcpu->b = resm(pcpu, 5, tmp16, 2); break; /*res 5,(ix+d),b*/
	case 0xa9: pcpu->c = resm(pcpu, 5, tmp16, 2); break; /*res 5,(ix+d),c*/
	case 0xaa: pcpu->d = resm(pcpu, 5, tmp16, 2); break; /*res 5,(ix+d),d*/
	case 0xab: pcpu->e = resm(pcpu, 5, tmp16, 2); break; /*res 5,(ix+d),e*/
	case 0xac: pcpu->h = resm(pcpu, 5, tmp16, 2); break; /*res 5,(ix+d),h*/
	case 0xad: pcpu->l = resm(pcpu, 5, tmp16, 2); break; /*res 5,(ix+d),l*/
	case 0xae:           resm(pcpu, 5, tmp16, 2); break; /*res 5,(ix+d)  */
	case 0xaf: pcpu->a = resm(pcpu, 5, tmp16, 2); break; /*res 5,(ix+d),a*/
	case 0xb0: pcpu->b = resm(pcpu, 6, tmp16, 2); break; /*res 6,(ix+d),b*/
	case 0xb1: pcpu->c = resm(pcpu, 6, tmp16, 2); break; /*res 6,(ix+d),c*/
	case 0xb2: pcpu->d = resm(pcpu, 6, tmp16, 2); break; /*res 6,(ix+d),d*/
	case 0xb3: pcpu->e = resm(pcpu, 6, tmp16, 2); break; /*res 6,(ix+d),e*/
	case 0xb4: pcpu->h = resm(pcpu, 6, tmp16, 2); break; /*res 6,(ix+d),h*/
	case 0xb5: pcpu->l = resm(pcpu, 6, tmp16, 2); break; /*res 6,(ix+d),l*/
	case 0xb6:           resm(pcpu, 6, tmp16, 2); break; /*res 6,(ix+d)  */
	case 0xb7: pcpu->a = resm(pcpu, 6, tmp16, 2); break; /*res 6,(ix+d),a*/
	case 0xb8: pcpu->b = resm(pcpu, 7, tmp16, 2); break; /*res 7,(ix+d),b*/
	case 0xb9: pcpu->c = resm(pcpu, 7, tmp16, 2); break; /*res 7,(ix+d),c*/
	case 0xba: pcpu->d = resm(pcpu, 7, tmp16, 2); break; /*res 7,(ix+d),d*/
	case 0xbb: pcpu->e = resm(pcpu, 7, tmp16, 2); break; /*res 7,(ix+d),e*/
	case 0xbc: pcpu->h = resm(pcpu, 7, tmp16, 2); break; /*res 7,(ix+d),h*/
	case 0xbd: pcpu->l = resm(pcpu, 7, tmp16, 2); break; /*res 7,(ix+d),l*/
	case 0xbe:           resm(pcpu, 7, tmp16, 2); break; /*res 7,(ix+d)  */
	case 0xbf: pcpu->a = resm(pcpu, 7, tmp16, 2); break; /*res 7,(ix+d),a*/

	case 0xc0: pcpu->b = setm(pcpu, 0, tmp16, 2); break; /*set 0,(ix+d),b*/
	case 0xc1: pcpu->c = setm(pcpu, 0, tmp16, 2); break; /*set 0,(ix+d),c*/
	case 0xc2: pcpu->d = setm(pcpu, 0, tmp16, 2); break; /*set 0,(ix+d),d*/
	case 0xc3: pcpu->e = setm(pcpu, 0, tmp16, 2); break; /*set 0,(ix+d),e*/
	case 0xc4: pcpu->h = setm(pcpu, 0, tmp16, 2); break; /*set 0,(ix+d),h*/
	case 0xc5: pcpu->l = setm(pcpu, 0, tmp16, 2); break; /*set 0,(ix+d),l*/
	case 0xc6:           setm(pcpu, 0, tmp16, 2); break; /*set 0,(ix+d)  */
	case 0xc7: pcpu->a = setm(pcpu, 0, tmp16, 2); break; /*set 0,(ix+d),a*/
	case 0xc8: pcpu->b = setm(pcpu, 1, tmp16, 2); break; /*set 1,(ix+d),b*/
	case 0xc9: pcpu->c = setm(pcpu, 1, tmp16, 2); break; /*set 1,(ix+d),c*/
	case 0xca: pcpu->d = setm(pcpu, 1, tmp16, 2); break; /*set 1,(ix+d),d*/
	case 0xcb: pcpu->e = setm(pcpu, 1, tmp16, 2); break; /*set 1,(ix+d),e*/
	case 0xcc: pcpu->h = setm(pcpu, 1, tmp16, 2); break; /*set 1,(ix+d),h*/
	case 0xcd: pcpu->l = setm(pcpu, 1, tmp16, 2); break; /*set 1,(ix+d),l*/
	case 0xce:           setm(pcpu, 1, tmp16, 2); break; /*set 1,(ix+d)  */
	case 0xcf: pcpu->a = setm(pcpu, 1, tmp16, 2); break; /*set 1,(ix+d),a*/
	case 0xd0: pcpu->b = setm(pcpu, 2, tmp16, 2); break; /*set 2,(ix+d),b*/
	case 0xd1: pcpu->c = setm(pcpu, 2, tmp16, 2); break; /*set 2,(ix+d),c*/
	case 0xd2: pcpu->d = setm(pcpu, 2, tmp16, 2); break; /*set 2,(ix+d),d*/
	case 0xd3: pcpu->e = setm(pcpu, 2, tmp16, 2); break; /*set 2,(ix+d),e*/
	case 0xd4: pcpu->h = setm(pcpu, 2, tmp16, 2); break; /*set 2,(ix+d),h*/
	case 0xd5: pcpu->l = setm(pcpu, 2, tmp16, 2); break; /*set 2,(ix+d),l*/
	case 0xd6:           setm(pcpu, 2, tmp16, 2); break; /*set 2,(ix+d)  */
	case 0xd7: pcpu->a = setm(pcpu, 2, tmp16, 2); break; /*set 2,(ix+d),a*/
	case 0xd8: pcpu->b = setm(pcpu, 3, tmp16, 2); break; /*set 3,(ix+d),b*/
	case 0xd9: pcpu->c = setm(pcpu, 3, tmp16, 2); break; /*set 3,(ix+d),c*/
	case 0xda: pcpu->d = setm(pcpu, 3, tmp16, 2); break; /*set 3,(ix+d),d*/
	case 0xdb: pcpu->e = setm(pcpu, 3, tmp16, 2); break; /*set 3,(ix+d),e*/
	case 0xdc: pcpu->h = setm(pcpu, 3, tmp16, 2); break; /*set 3,(ix+d),h*/
	case 0xdd: pcpu->l = setm(pcpu, 3, tmp16, 2); break; /*set 3,(ix+d),l*/
	case 0xde:           setm(pcpu, 3, tmp16, 2); break; /*set 3,(ix+d)  */
	case 0xdf: pcpu->a = setm(pcpu, 3, tmp16, 2); break; /*set 3,(ix+d),a*/
	case 0xe0: pcpu->b = setm(pcpu, 4, tmp16, 2); break; /*set 4,(ix+d),b*/
	case 0xe1: pcpu->c = setm(pcpu, 4, tmp16, 2); break; /*set 4,(ix+d),c*/
	case 0xe2: pcpu->d = setm(pcpu, 4, tmp16, 2); break; /*set 4,(ix+d),d*/
	case 0xe3: pcpu->e = setm(pcpu, 4, tmp16, 2); break; /*set 4,(ix+d),e*/
	case 0xe4: pcpu->h = setm(pcpu, 4, tmp16, 2); break; /*set 4,(ix+d),h*/
	case 0xe5: pcpu->l = setm(pcpu, 4, tmp16, 2); break; /*set 4,(ix+d),l*/
	case 0xe6:           setm(pcpu, 4, tmp16, 2); break; /*set 4,(ix+d)  */
	case 0xe7: pcpu->a = setm(pcpu, 4, tmp16, 2); break; /*set 4,(ix+d),a*/
	case 0xe8: pcpu->b = setm(pcpu, 5, tmp16, 2); break; /*set 5,(ix+d),b*/
	case 0xe9: pcpu->c = setm(pcpu, 5, tmp16, 2); break; /*set 5,(ix+d),c*/
	case 0xea: pcpu->d = setm(pcpu, 5, tmp16, 2); break; /*set 5,(ix+d),d*/
	case 0xeb: pcpu->e = setm(pcpu, 5, tmp16, 2); break; /*set 5,(ix+d),e*/
	case 0xec: pcpu->h = setm(pcpu, 5, tmp16, 2); break; /*set 5,(ix+d),h*/
	case 0xed: pcpu->l = setm(pcpu, 5, tmp16, 2); break; /*set 5,(ix+d),l*/
	case 0xee:           setm(pcpu, 5, tmp16, 2); break; /*set 5,(ix+d)  */
	case 0xef: pcpu->a = setm(pcpu, 5, tmp16, 2); break; /*set 5,(ix+d),a*/
	case 0xf0: pcpu->b = setm(pcpu, 6, tmp16, 2); break; /*set 6,(ix+d),b*/
	case 0xf1: pcpu->c = setm(pcpu, 6, tmp16, 2); break; /*set 6,(ix+d),c*/
	case 0xf2: pcpu->d = setm(pcpu, 6, tmp16, 2); break; /*set 6,(ix+d),d*/
	case 0xf3: pcpu->e = setm(pcpu, 6, tmp16, 2); break; /*set 6,(ix+d),e*/
	case 0xf4: pcpu->h = setm(pcpu, 6, tmp16, 2); break; /*set 6,(ix+d),h*/
	case 0xf5: pcpu->l = setm(pcpu, 6, tmp16, 2); break; /*set 6,(ix+d),l*/
	case 0xf6:           setm(pcpu, 6, tmp16, 2); break; /*set 6,(ix+d)  */
	case 0xf7: pcpu->a = setm(pcpu, 6, tmp16, 2); break; /*set 6,(ix+d),a*/
	case 0xf8: pcpu->b = setm(pcpu, 7, tmp16, 2); break; /*set 7,(ix+d),b*/
	case 0xf9: pcpu->c = setm(pcpu, 7, tmp16, 2); break; /*set 7,(ix+d),c*/
	case 0xfa: pcpu->d = setm(pcpu, 7, tmp16, 2); break; /*set 7,(ix+d),d*/
	case 0xfb: pcpu->e = setm(pcpu, 7, tmp16, 2); break; /*set 7,(ix+d),e*/
	case 0xfc: pcpu->h = setm(pcpu, 7, tmp16, 2); break; /*set 7,(ix+d),h*/
	case 0xfd: pcpu->l = setm(pcpu, 7, tmp16, 2); break; /*set 7,(ix+d),l*/
	case 0xfe:           setm(pcpu, 7, tmp16, 2); break; /*set 7,(ix+d)  */
	case 0xff: pcpu->a = setm(pcpu, 7, tmp16, 2); break; /*set 7,(ix+d),a*/
	}
      }
      break;						/* DDCB prefix end */

    default:
      pcpu->op1 = pcpu->op2;
      pcpu->mcycle = 0;
      exec_code(pcpu);
      break;
    }
    break;						/* DD prefix end */

  case 0xfd:						/* FD prefix */
    if (pcpu->mcycle == 1) {
      break;
    } else if (pcpu->mcycle == 2) {
      pcpu->op2 = fetch(pcpu);
    }
    pcpu->cycles += tstates_dd[pcpu->op2][pcpu->mcycle - 2];

    switch (pcpu->op2) {
    case 0x09:addw(pcpu, &pcpu->iyh, &pcpu->iyl, BC);	break;	/* add iy,bc */
    case 0x19:addw(pcpu, &pcpu->iyh, &pcpu->iyl, DE);	break;	/* add iy,bc */
    case 0x29:addw(pcpu, &pcpu->iyh, &pcpu->iyl, IY);	break;	/* add iy,iy */
    case 0x39:addw(pcpu, &pcpu->iyh, &pcpu->iyl, pcpu->sp);	break;	/* add iy,sp */
      
    case 0x21:						/* ld iy,nn */
      switch (pcpu->mcycle) {
      case 2:						break;
      case 3:	pcpu->iyl = z80_read(pcpu->pc++);	break;
      case 4:	pcpu->iyh = z80_read(pcpu->pc++);	break;
      }
      break;

    case 0x22:						/* ld (nn),iy */
      switch (pcpu->mcycle) {
      case 2:						break;
      case 3:	tmp16 = z80_read(pcpu->pc++);		break;
      case 4:	tmp16 += z80_read(pcpu->pc++) << 8;	break;
      case 5:	z80_write(tmp16,     pcpu->iyl);	break;
      case 6:	z80_write(tmp16 + 1, pcpu->iyh);	break;
      }
      break;

    case 0x23:	incw(&pcpu->iyh, &pcpu->iyl);	break;	/* inc iy */
    case 0x2b:	decw(&pcpu->iyh, &pcpu->iyl);	break;	/* dec iy */
    case 0x24:	inc(pcpu, &pcpu->iyh);		break;	/* inc iyh */
    case 0x2c:	inc(pcpu, &pcpu->iyl);		break;	/* inc iyl */
    case 0x25:	dec(pcpu, &pcpu->iyh);		break;	/* dec iyh */
    case 0x2d:	dec(pcpu, &pcpu->iyl);		break;	/* dec iyl */

    case 0x26:						/* ld iyh,n */
      switch (pcpu->mcycle) {
      case 2:						break;
      case 3:	pcpu->iyh = z80_read(pcpu->pc++);	break;
      }
      break;
    case 0x2e:						/* ld iyl,n */
      switch (pcpu->mcycle) {
      case 2:						break;
      case 3:	pcpu->iyl = z80_read(pcpu->pc++);	break;
      }
      break;

    case 0x2a:						/* ld iy,(nn) */
      switch (pcpu->mcycle) {
      case 2:						break;
      case 3:	tmp16 = z80_read(pcpu->pc++);		break;
      case 4:	tmp16 += z80_read(pcpu->pc++) << 8;	break;
      case 5:	pcpu->iyl = z80_read(tmp16);		break;
      case 6:	pcpu->iyh = z80_read(tmp16 + 1);	break;
      }
      break;

    case 0x34:						/* inc (iy+d) */
      switch (pcpu->mcycle) {
      case 2:							break;
      case 3:	tmp8 = z80_read(pcpu->pc++);			break;
      case 4:	tmp16 = (word)((int32_t)IY + (int8_t)tmp8);	break;
      case 5:	tmp8 = z80_read(tmp16);				break;
      case 6:	inc(pcpu, &tmp8);	z80_write(tmp16, tmp8);	break;
      }
      break;

    case 0x35:						/* dec (iy+d) */
      switch (pcpu->mcycle) {
      case 2:							break;
      case 3:	tmp8 = z80_read(pcpu->pc++);			break;
      case 4:	tmp16 = (word)((int32_t)IY + (int8_t)tmp8);	break;
      case 5:	tmp8 = z80_read(tmp16);				break;
      case 6:	dec(pcpu, &tmp8);	z80_write(tmp16, tmp8);	break;
      }
      break;

    case 0x36:						/* ld (iy+d),n */
      switch (pcpu->mcycle) {
      case 2:							break;
      case 3:	tmp8 = z80_read(pcpu->pc++);			break;
      case 4:
	tmp16 = (word)((int32_t)IY + (int8_t)tmp8);
     	tmp8 = z80_read(pcpu->pc++);
	break;
      case 5:	z80_write(tmp16, tmp8);				break;
      }
      break;

    case 0x44:	pcpu->b = pcpu->iyh;		break;	/* ld b,iyh */
    case 0x45:	pcpu->b = pcpu->iyl;		break;	/* ld b,iyl */
    case 0x46:	ldriy(pcpu, &pcpu->b);		break;	/* ld b,(iy+d) */
    case 0x4c:	pcpu->c = pcpu->iyh;		break;	/* ld c,iyh */
    case 0x4d:	pcpu->c = pcpu->iyl;		break;	/* ld c,iyl */
    case 0x4e:	ldriy(pcpu, &pcpu->c);		break;	/* ld c,(iy+d) */
    case 0x54:	pcpu->d = pcpu->iyh;		break;	/* ld d,iyh */
    case 0x55:	pcpu->d = pcpu->iyl;		break;	/* ld d,iyl */
    case 0x56:	ldriy(pcpu, &pcpu->d);		break;	/* ld d,(iy+d) */
    case 0x5c:	pcpu->e = pcpu->iyh;		break;	/* ld e,iyh */
    case 0x5d:	pcpu->e = pcpu->iyl;		break;	/* ld e,iyl */
    case 0x5e:	ldriy(pcpu, &pcpu->e);		break;	/* ld e,(iy+d) */
    case 0x60:	pcpu->iyh = pcpu->b;		break;	/* ld iyh,b */
    case 0x61:	pcpu->iyh = pcpu->c;		break;	/* ld iyh,c */
    case 0x62:	pcpu->iyh = pcpu->d;		break;	/* ld iyh,d */
    case 0x63:	pcpu->iyh = pcpu->e;		break;	/* ld iyh,e */
    case 0x64:	pcpu->iyh = pcpu->iyh;		break;	/* ld iyh,iyh */
    case 0x65:	pcpu->iyh = pcpu->iyl;		break;	/* ld iyh,iyl */
    case 0x66:	ldriy(pcpu, &pcpu->h);		break;	/* ld h,(iy+d) */
    case 0x67:	pcpu->iyh = pcpu->a;		break;	/* ld iyh,a */

    case 0x68:	pcpu->iyl = pcpu->b;		break;	/* ld iyl,b */
    case 0x69:	pcpu->iyl = pcpu->c;		break;	/* ld iyl,c */
    case 0x6a:	pcpu->iyl = pcpu->d;		break;	/* ld iyl,d */
    case 0x6b:	pcpu->iyl = pcpu->e;		break;	/* ld iyl,e */
    case 0x6c:	pcpu->iyl = pcpu->iyh;		break;	/* ld iyl,iyh */
    case 0x6d:	pcpu->iyl = pcpu->iyl;		break;	/* ld iyl,iyl */
    case 0x6e:	ldriy(pcpu, &pcpu->l);		break;	/* ld l,(iy+d) */
    case 0x6f:	pcpu->iyl = pcpu->a;		break;	/* ld iyl,a */

    case 0x70:	ldiyr(pcpu, pcpu->b);		break;	/* ld (iy+d),b */
    case 0x71:	ldiyr(pcpu, pcpu->c);		break;	/* ld (iy+d),c */
    case 0x72:	ldiyr(pcpu, pcpu->d);		break;	/* ld (iy+d),d */
    case 0x73:	ldiyr(pcpu, pcpu->e);		break;	/* ld (iy+d),e */
    case 0x74:	ldiyr(pcpu, pcpu->h);		break;	/* ld (iy+d),h */
    case 0x75:	ldiyr(pcpu, pcpu->l);		break;	/* ld (iy+d),l */
    case 0x77:	ldiyr(pcpu, pcpu->a);		break;	/* ld (iy+d),a */

    case 0x7c:	pcpu->a = pcpu->iyh;		break;	/* ld a,iyh */
    case 0x7d:	pcpu->a = pcpu->iyl;		break;	/* ld a,iyl */
    case 0x7e:	ldriy(pcpu, &pcpu->a);		break;	/* ld a,(iy+d) */

    case 0x84:	add(pcpu, pcpu->iyh);		break;	/* add a,iyh */
    case 0x85:	add(pcpu, pcpu->iyl);		break;	/* add a,iyl */
    case 0x86:						/* add a,(iy+d) */
      switch (pcpu->mcycle) {
      case 2:							break;
      case 3:	tmp8 = z80_read(pcpu->pc++);			break;
      case 4:	tmp16 = (word)((int32_t)IY + (int8_t)tmp8);	break;
      case 5:	add(pcpu, z80_read(tmp16));			break;
      }
      break;

    case 0x8c:	adc(pcpu, pcpu->iyh);		break;	/* adc a,iyh */
    case 0x8d:	adc(pcpu, pcpu->iyl);		break;	/* adc a,iyl */
    case 0x8e:						/* adc a,(iy+d) */
      switch (pcpu->mcycle) {
      case 2:							break;
      case 3:	tmp8 = z80_read(pcpu->pc++);			break;
      case 4:	tmp16 = (word)((int32_t)IY + (int8_t)tmp8);	break;
      case 5:	adc(pcpu, z80_read(tmp16));			break;
      }
      break;

    case 0x94:	sub(pcpu, pcpu->iyh);		break;	/* sub iyh */
    case 0x95:	sub(pcpu, pcpu->iyl);		break;	/* sub iyl */
    case 0x96:						/* sub (iy+d) */
      switch (pcpu->mcycle) {
      case 2:							break;
      case 3:	tmp8 = z80_read(pcpu->pc++);			break;
      case 4:	tmp16 = (word)((int32_t)IY + (int8_t)tmp8);	break;
      case 5:	sub(pcpu, z80_read(tmp16));			break;
      }
      break;

    case 0x9c:	sbc(pcpu, pcpu->iyh);		break;	/* sbc a,iyh */
    case 0x9d:	sbc(pcpu, pcpu->iyl);		break;	/* sbc a,iyl */
    case 0x9e:						/* sbc (iy+d) */
      switch (pcpu->mcycle) {
      case 2:							break;
      case 3:	tmp8 = z80_read(pcpu->pc++);			break;
      case 4:	tmp16 = (word)((int32_t)IY + (int8_t)tmp8);	break;
      case 5:	sbc(pcpu, z80_read(tmp16));			break;
      }
      break;

    case 0xa4:	and(pcpu, pcpu->iyh);		break;	/* and iyh */
    case 0xa5:	and(pcpu, pcpu->iyl);		break;	/* and iyl */
    case 0xa6:						/* and (iy+d) */
      switch (pcpu->mcycle) {
      case 2:							break;
      case 3:	tmp8 = z80_read(pcpu->pc++);			break;
      case 4:	tmp16 = (word)((int32_t)IY + (int8_t)tmp8);	break;
      case 5:	and(pcpu, z80_read(tmp16));			break;
      }
      break;

    case 0xac:	xor(pcpu, pcpu->iyh);		break;	/* xor iyh */
    case 0xad:	xor(pcpu, pcpu->iyl);		break;	/* xor iyl */
    case 0xae:						/* xor (iy+d) */
      switch (pcpu->mcycle) {
      case 2:							break;
      case 3:	tmp8 = z80_read(pcpu->pc++);			break;
      case 4:	tmp16 = (word)((int32_t)IY + (int8_t)tmp8);	break;
      case 5:	xor(pcpu, z80_read(tmp16));			break;
      }
      break;

    case 0xb4:	or(pcpu, pcpu->iyh);		break;	/* or iyh */
    case 0xb5:	or(pcpu, pcpu->iyl);		break;	/* or iyl */
    case 0xb6:						/* or (iy+d) */
      switch (pcpu->mcycle) {
      case 2:							break;
      case 3:	tmp8 = z80_read(pcpu->pc++);			break;
      case 4:	tmp16 = (word)((int32_t)IY + (int8_t)tmp8);	break;
      case 5:	or(pcpu, z80_read(tmp16));			break;
      }
      break;

    case 0xbc:	cp(pcpu, pcpu->iyh);		break;	/* cp iyh */
    case 0xbd:	cp(pcpu, pcpu->iyl);		break;	/* cp iyl */
    case 0xbe:						/* cp (iy+d) */
      switch (pcpu->mcycle) {
      case 2:							break;
      case 3:	tmp8 = z80_read(pcpu->pc++);			break;
      case 4:	tmp16 = (word)((int32_t)IY + (int8_t)tmp8);	break;
      case 5:	cp(pcpu, z80_read(tmp16));			break;
      }
      break;

    case 0xe1:	pop(pcpu, &pcpu->iyh, &pcpu->iyl);	break;	/* pop iy */

    case 0xe3:						/* ex (sp),iy */
      switch (pcpu->mcycle) {
      case 2:							break;
      case 3:	tmp16 = z80_read(pcpu->sp++);			break;
      case 4:	tmp16 += z80_read(pcpu->sp) << 8;		break;
      case 5:	z80_write(pcpu->sp--, pcpu->iyh);		break;
      case 6:
	z80_write(pcpu->sp, pcpu->iyl);
	pcpu->iyl = tmp16 & 0xff;
	pcpu->iyh = tmp16 >> 8;
	break;
      }
      break;

    case 0xe5:	push(pcpu, pcpu->iyh, pcpu->iyl);	break;	/* push iy */

    case 0xe9:	pcpu->pc = IY;				break;	/* jp (iy) */

    case 0xed:							/* ED prefix */
      pcpu->op1 = 0xed;
      break;

    case 0xf9:	pcpu->sp = IY;				break;	/* ld sp,iy */

    case 0xcb:						/* FDCB prefix */
      switch (pcpu->mcycle) {
      case 2:
	break;
      case 3:
	tmp8 = z80_read(pcpu->pc++);
	break;
      case 4:
	tmp16 = (word)((int32_t)IY + (int8_t)tmp8);
	pcpu->op3 = z80_read(pcpu->pc++);
	break;
      case 5:
      case 6:
	switch (pcpu->op3) {
	case 0x00: pcpu->b = mreg(pcpu, rlc, tmp16, 3); break; /*rlc (iy+d),b*/
	case 0x01: pcpu->c = mreg(pcpu, rlc, tmp16, 3); break; /*rlc (iy+d),c*/
	case 0x02: pcpu->d = mreg(pcpu, rlc, tmp16, 3); break; /*rlc (iy+d),d*/
	case 0x03: pcpu->e = mreg(pcpu, rlc, tmp16, 3); break; /*rlc (iy+d),e*/
	case 0x04: pcpu->h = mreg(pcpu, rlc, tmp16, 3); break; /*rlc (iy+d),h*/
	case 0x05: pcpu->l = mreg(pcpu, rlc, tmp16, 3); break; /*rlc (iy+d),l*/
	case 0x06:           mreg(pcpu, rlc, tmp16, 3); break; /*rlc (iy+d)  */
	case 0x07: pcpu->a = mreg(pcpu, rlc, tmp16, 3); break; /*rlc (iy+d),a*/

	case 0x08: pcpu->b = mreg(pcpu, rrc, tmp16, 3); break; /*rrc (iy+d),b*/
	case 0x09: pcpu->c = mreg(pcpu, rrc, tmp16, 3); break; /*rrc (iy+d),c*/
	case 0x0a: pcpu->d = mreg(pcpu, rrc, tmp16, 3); break; /*rrc (iy+d),d*/
	case 0x0b: pcpu->e = mreg(pcpu, rrc, tmp16, 3); break; /*rrc (iy+d),e*/
	case 0x0c: pcpu->h = mreg(pcpu, rrc, tmp16, 3); break; /*rrc (iy+d),h*/
	case 0x0d: pcpu->l = mreg(pcpu, rrc, tmp16, 3); break; /*rrc (iy+d),l*/
	case 0x0e:           mreg(pcpu, rrc, tmp16, 3); break; /*rrc (iy+d)  */
	case 0x0f: pcpu->a = mreg(pcpu, rrc, tmp16, 3); break; /*rrc (iy+d),a*/

	case 0x10: pcpu->b = mreg(pcpu, rl, tmp16, 3); break; /*rl (iy+d),b*/
	case 0x11: pcpu->c = mreg(pcpu, rl, tmp16, 3); break; /*rl (iy+d),c*/
	case 0x12: pcpu->d = mreg(pcpu, rl, tmp16, 3); break; /*rl (iy+d),d*/
	case 0x13: pcpu->e = mreg(pcpu, rl, tmp16, 3); break; /*rl (iy+d),e*/
	case 0x14: pcpu->h = mreg(pcpu, rl, tmp16, 3); break; /*rl (iy+d),h*/
	case 0x15: pcpu->l = mreg(pcpu, rl, tmp16, 3); break; /*rl (iy+d),l*/
	case 0x16:           mreg(pcpu, rl, tmp16, 3); break; /*rl (iy+d)  */
	case 0x17: pcpu->a = mreg(pcpu, rl, tmp16, 3); break; /*rl (iy+d),a*/

	case 0x18: pcpu->b = mreg(pcpu, rr, tmp16, 3); break; /*rr (iy+d),b*/
	case 0x19: pcpu->c = mreg(pcpu, rr, tmp16, 3); break; /*rr (iy+d),c*/
	case 0x1a: pcpu->d = mreg(pcpu, rr, tmp16, 3); break; /*rr (iy+d),d*/
	case 0x1b: pcpu->e = mreg(pcpu, rr, tmp16, 3); break; /*rr (iy+d),e*/
	case 0x1c: pcpu->h = mreg(pcpu, rr, tmp16, 3); break; /*rr (iy+d),h*/
	case 0x1d: pcpu->l = mreg(pcpu, rr, tmp16, 3); break; /*rr (iy+d),l*/
	case 0x1e:           mreg(pcpu, rr, tmp16, 3); break; /*rr (iy+d)  */
	case 0x1f: pcpu->a = mreg(pcpu, rr, tmp16, 3); break; /*rr (iy+d),a*/

	case 0x20: pcpu->b = mreg(pcpu, sla, tmp16, 3); break; /*sla (iy+d),b*/
	case 0x21: pcpu->c = mreg(pcpu, sla, tmp16, 3); break; /*sla (iy+d),c*/
	case 0x22: pcpu->d = mreg(pcpu, sla, tmp16, 3); break; /*sla (iy+d),d*/
	case 0x23: pcpu->e = mreg(pcpu, sla, tmp16, 3); break; /*sla (iy+d),e*/
	case 0x24: pcpu->h = mreg(pcpu, sla, tmp16, 3); break; /*sla (iy+d),h*/
	case 0x25: pcpu->l = mreg(pcpu, sla, tmp16, 3); break; /*sla (iy+d),l*/
	case 0x26:           mreg(pcpu, sla, tmp16, 3); break; /*sla (iy+d)  */
	case 0x27: pcpu->a = mreg(pcpu, sla, tmp16, 3); break; /*sla (iy+d),a*/

	case 0x28: pcpu->b = mreg(pcpu, sra, tmp16, 3); break; /*sra (iy+d),b*/
	case 0x29: pcpu->c = mreg(pcpu, sra, tmp16, 3); break; /*sra (iy+d),c*/
	case 0x2a: pcpu->d = mreg(pcpu, sra, tmp16, 3); break; /*sra (iy+d),d*/
	case 0x2b: pcpu->e = mreg(pcpu, sra, tmp16, 3); break; /*sra (iy+d),e*/
	case 0x2c: pcpu->h = mreg(pcpu, sra, tmp16, 3); break; /*sra (iy+d),h*/
	case 0x2d: pcpu->l = mreg(pcpu, sra, tmp16, 3); break; /*sra (iy+d),l*/
	case 0x2e:           mreg(pcpu, sra, tmp16, 3); break; /*sra (iy+d)  */
	case 0x2f: pcpu->a = mreg(pcpu, sra, tmp16, 3); break; /*sra (iy+d),a*/

	case 0x30: pcpu->b = mreg(pcpu, sll, tmp16, 3); break; /*sll (iy+d),b*/
	case 0x31: pcpu->c = mreg(pcpu, sll, tmp16, 3); break; /*sll (iy+d),c*/
	case 0x32: pcpu->d = mreg(pcpu, sll, tmp16, 3); break; /*sll (iy+d),d*/
	case 0x33: pcpu->e = mreg(pcpu, sll, tmp16, 3); break; /*sll (iy+d),e*/
	case 0x34: pcpu->h = mreg(pcpu, sll, tmp16, 3); break; /*sll (iy+d),h*/
	case 0x35: pcpu->l = mreg(pcpu, sll, tmp16, 3); break; /*sll (iy+d),l*/
	case 0x36:           mreg(pcpu, sll, tmp16, 3); break; /*sll (iy+d)  */
	case 0x37: pcpu->a = mreg(pcpu, sll, tmp16, 3); break; /*sll (iy+d),a*/

	case 0x38: pcpu->b = mreg(pcpu, srl, tmp16, 3); break; /*srl (iy+d),b*/
	case 0x39: pcpu->c = mreg(pcpu, srl, tmp16, 3); break; /*srl (iy+d),c*/
	case 0x3a: pcpu->d = mreg(pcpu, srl, tmp16, 3); break; /*srl (iy+d),d*/
	case 0x3b: pcpu->e = mreg(pcpu, srl, tmp16, 3); break; /*srl (iy+d),e*/
	case 0x3c: pcpu->h = mreg(pcpu, srl, tmp16, 3); break; /*srl (iy+d),h*/
	case 0x3d: pcpu->l = mreg(pcpu, srl, tmp16, 3); break; /*srl (iy+d),l*/
	case 0x3e:           mreg(pcpu, srl, tmp16, 3); break; /*srl (iy+d)  */
	case 0x3f: pcpu->a = mreg(pcpu, srl, tmp16, 3); break; /*srl (iy+d),a*/

	case 0x40:	case 0x41:	case 0x42:	case 0x43:
	case 0x44:	case 0x45:	case 0x46:	case 0x47:
	case 0x48:	case 0x49:	case 0x4a:	case 0x4b:
	case 0x4c:	case 0x4d:	case 0x4e:	case 0x4f:
	case 0x50:	case 0x51:	case 0x52:	case 0x53:
	case 0x54:	case 0x55:	case 0x56:	case 0x57:
	case 0x58:	case 0x59:	case 0x5a:	case 0x5b:
	case 0x5c:	case 0x5d:	case 0x5e:	case 0x5f:
	case 0x60:	case 0x61:	case 0x62:	case 0x63:
	case 0x64:	case 0x65:	case 0x66:	case 0x67:
	case 0x68:	case 0x69:	case 0x6a:	case 0x6b:
	case 0x6c:	case 0x6d:	case 0x6e:	case 0x6f:
	case 0x70:	case 0x71:	case 0x72:	case 0x73:
	case 0x74:	case 0x75:	case 0x76:	case 0x77:
	case 0x78:	case 0x79:	case 0x7a:	case 0x7b:
	case 0x7c:	case 0x7d:	case 0x7e:	case 0x7f:
	  /* bit n,(iy+d) */
	  bitixy(pcpu, (pcpu->op3 >> 3) & 0x07, tmp16);
	  pcpu->mcycle = 0;
	  break;

	case 0x80: pcpu->b = resm(pcpu, 0, tmp16, 2); break; /*res 0,(iy+d),b*/
	case 0x81: pcpu->c = resm(pcpu, 0, tmp16, 2); break; /*res 0,(iy+d),c*/
	case 0x82: pcpu->d = resm(pcpu, 0, tmp16, 2); break; /*res 0,(iy+d),d*/
	case 0x83: pcpu->e = resm(pcpu, 0, tmp16, 2); break; /*res 0,(iy+d),e*/
	case 0x84: pcpu->h = resm(pcpu, 0, tmp16, 2); break; /*res 0,(iy+d),h*/
	case 0x85: pcpu->l = resm(pcpu, 0, tmp16, 2); break; /*res 0,(iy+d),l*/
	case 0x86:           resm(pcpu, 0, tmp16, 2); break; /*res 0,(iy+d)  */
	case 0x87: pcpu->a = resm(pcpu, 0, tmp16, 2); break; /*res 0,(iy+d),a*/
	case 0x88: pcpu->b = resm(pcpu, 1, tmp16, 2); break; /*res 1,(iy+d),b*/
	case 0x89: pcpu->c = resm(pcpu, 1, tmp16, 2); break; /*res 1,(iy+d),c*/
	case 0x8a: pcpu->d = resm(pcpu, 1, tmp16, 2); break; /*res 1,(iy+d),d*/
	case 0x8b: pcpu->e = resm(pcpu, 1, tmp16, 2); break; /*res 1,(iy+d),e*/
	case 0x8c: pcpu->h = resm(pcpu, 1, tmp16, 2); break; /*res 1,(iy+d),h*/
	case 0x8d: pcpu->l = resm(pcpu, 1, tmp16, 2); break; /*res 1,(iy+d),l*/
	case 0x8e:           resm(pcpu, 1, tmp16, 2); break; /*res 1,(iy+d)  */
	case 0x8f: pcpu->a = resm(pcpu, 1, tmp16, 2); break; /*res 1,(iy+d),a*/
	case 0x90: pcpu->b = resm(pcpu, 2, tmp16, 2); break; /*res 2,(iy+d),b*/
	case 0x91: pcpu->c = resm(pcpu, 2, tmp16, 2); break; /*res 2,(iy+d),c*/
	case 0x92: pcpu->d = resm(pcpu, 2, tmp16, 2); break; /*res 2,(iy+d),d*/
	case 0x93: pcpu->e = resm(pcpu, 2, tmp16, 2); break; /*res 2,(iy+d),e*/
	case 0x94: pcpu->h = resm(pcpu, 2, tmp16, 2); break; /*res 2,(iy+d),h*/
	case 0x95: pcpu->l = resm(pcpu, 2, tmp16, 2); break; /*res 2,(iy+d),l*/
	case 0x96:           resm(pcpu, 2, tmp16, 2); break; /*res 2,(iy+d)  */
	case 0x97: pcpu->a = resm(pcpu, 2, tmp16, 2); break; /*res 2,(iy+d),a*/
	case 0x98: pcpu->b = resm(pcpu, 3, tmp16, 2); break; /*res 3,(iy+d),b*/
	case 0x99: pcpu->c = resm(pcpu, 3, tmp16, 2); break; /*res 3,(iy+d),c*/
	case 0x9a: pcpu->d = resm(pcpu, 3, tmp16, 2); break; /*res 3,(iy+d),d*/
	case 0x9b: pcpu->e = resm(pcpu, 3, tmp16, 2); break; /*res 3,(iy+d),e*/
	case 0x9c: pcpu->h = resm(pcpu, 3, tmp16, 2); break; /*res 3,(iy+d),h*/
	case 0x9d: pcpu->l = resm(pcpu, 3, tmp16, 2); break; /*res 3,(iy+d),l*/
	case 0x9e:           resm(pcpu, 3, tmp16, 2); break; /*res 3,(iy+d)  */
	case 0x9f: pcpu->a = resm(pcpu, 3, tmp16, 2); break; /*res 3,(iy+d),a*/
	case 0xa0: pcpu->b = resm(pcpu, 4, tmp16, 2); break; /*res 4,(iy+d),b*/
	case 0xa1: pcpu->c = resm(pcpu, 4, tmp16, 2); break; /*res 4,(iy+d),c*/
	case 0xa2: pcpu->d = resm(pcpu, 4, tmp16, 2); break; /*res 4,(iy+d),d*/
	case 0xa3: pcpu->e = resm(pcpu, 4, tmp16, 2); break; /*res 4,(iy+d),e*/
	case 0xa4: pcpu->h = resm(pcpu, 4, tmp16, 2); break; /*res 4,(iy+d),h*/
	case 0xa5: pcpu->l = resm(pcpu, 4, tmp16, 2); break; /*res 4,(iy+d),l*/
	case 0xa6:           resm(pcpu, 4, tmp16, 2); break; /*res 4,(iy+d)  */
	case 0xa7: pcpu->a = resm(pcpu, 4, tmp16, 2); break; /*res 4,(iy+d),a*/
	case 0xa8: pcpu->b = resm(pcpu, 5, tmp16, 2); break; /*res 5,(iy+d),b*/
	case 0xa9: pcpu->c = resm(pcpu, 5, tmp16, 2); break; /*res 5,(iy+d),c*/
	case 0xaa: pcpu->d = resm(pcpu, 5, tmp16, 2); break; /*res 5,(iy+d),d*/
	case 0xab: pcpu->e = resm(pcpu, 5, tmp16, 2); break; /*res 5,(iy+d),e*/
	case 0xac: pcpu->h = resm(pcpu, 5, tmp16, 2); break; /*res 5,(iy+d),h*/
	case 0xad: pcpu->l = resm(pcpu, 5, tmp16, 2); break; /*res 5,(iy+d),l*/
	case 0xae:           resm(pcpu, 5, tmp16, 2); break; /*res 5,(iy+d)  */
	case 0xaf: pcpu->a = resm(pcpu, 5, tmp16, 2); break; /*res 5,(iy+d),a*/
	case 0xb0: pcpu->b = resm(pcpu, 6, tmp16, 2); break; /*res 6,(iy+d),b*/
	case 0xb1: pcpu->c = resm(pcpu, 6, tmp16, 2); break; /*res 6,(iy+d),c*/
	case 0xb2: pcpu->d = resm(pcpu, 6, tmp16, 2); break; /*res 6,(iy+d),d*/
	case 0xb3: pcpu->e = resm(pcpu, 6, tmp16, 2); break; /*res 6,(iy+d),e*/
	case 0xb4: pcpu->h = resm(pcpu, 6, tmp16, 2); break; /*res 6,(iy+d),h*/
	case 0xb5: pcpu->l = resm(pcpu, 6, tmp16, 2); break; /*res 6,(iy+d),l*/
	case 0xb6:           resm(pcpu, 6, tmp16, 2); break; /*res 6,(iy+d)  */
	case 0xb7: pcpu->a = resm(pcpu, 6, tmp16, 2); break; /*res 6,(iy+d),a*/
	case 0xb8: pcpu->b = resm(pcpu, 7, tmp16, 2); break; /*res 7,(iy+d),b*/
	case 0xb9: pcpu->c = resm(pcpu, 7, tmp16, 2); break; /*res 7,(iy+d),c*/
	case 0xba: pcpu->d = resm(pcpu, 7, tmp16, 2); break; /*res 7,(iy+d),d*/
	case 0xbb: pcpu->e = resm(pcpu, 7, tmp16, 2); break; /*res 7,(iy+d),e*/
	case 0xbc: pcpu->h = resm(pcpu, 7, tmp16, 2); break; /*res 7,(iy+d),h*/
	case 0xbd: pcpu->l = resm(pcpu, 7, tmp16, 2); break; /*res 7,(iy+d),l*/
	case 0xbe:           resm(pcpu, 7, tmp16, 2); break; /*res 7,(iy+d)  */
	case 0xbf: pcpu->a = resm(pcpu, 7, tmp16, 2); break; /*res 7,(iy+d),a*/

	case 0xc0: pcpu->b = setm(pcpu, 0, tmp16, 2); break; /*set 0,(iy+d),b*/
	case 0xc1: pcpu->c = setm(pcpu, 0, tmp16, 2); break; /*set 0,(iy+d),c*/
	case 0xc2: pcpu->d = setm(pcpu, 0, tmp16, 2); break; /*set 0,(iy+d),d*/
	case 0xc3: pcpu->e = setm(pcpu, 0, tmp16, 2); break; /*set 0,(iy+d),e*/
	case 0xc4: pcpu->h = setm(pcpu, 0, tmp16, 2); break; /*set 0,(iy+d),h*/
	case 0xc5: pcpu->l = setm(pcpu, 0, tmp16, 2); break; /*set 0,(iy+d),l*/
	case 0xc6:           setm(pcpu, 0, tmp16, 2); break; /*set 0,(iy+d)  */
	case 0xc7: pcpu->a = setm(pcpu, 0, tmp16, 2); break; /*set 0,(iy+d),a*/
	case 0xc8: pcpu->b = setm(pcpu, 1, tmp16, 2); break; /*set 1,(iy+d),b*/
	case 0xc9: pcpu->c = setm(pcpu, 1, tmp16, 2); break; /*set 1,(iy+d),c*/
	case 0xca: pcpu->d = setm(pcpu, 1, tmp16, 2); break; /*set 1,(iy+d),d*/
	case 0xcb: pcpu->e = setm(pcpu, 1, tmp16, 2); break; /*set 1,(iy+d),e*/
	case 0xcc: pcpu->h = setm(pcpu, 1, tmp16, 2); break; /*set 1,(iy+d),h*/
	case 0xcd: pcpu->l = setm(pcpu, 1, tmp16, 2); break; /*set 1,(iy+d),l*/
	case 0xce:           setm(pcpu, 1, tmp16, 2); break; /*set 1,(iy+d)  */
	case 0xcf: pcpu->a = setm(pcpu, 1, tmp16, 2); break; /*set 1,(iy+d),a*/
	case 0xd0: pcpu->b = setm(pcpu, 2, tmp16, 2); break; /*set 2,(iy+d),b*/
	case 0xd1: pcpu->c = setm(pcpu, 2, tmp16, 2); break; /*set 2,(iy+d),c*/
	case 0xd2: pcpu->d = setm(pcpu, 2, tmp16, 2); break; /*set 2,(iy+d),d*/
	case 0xd3: pcpu->e = setm(pcpu, 2, tmp16, 2); break; /*set 2,(iy+d),e*/
	case 0xd4: pcpu->h = setm(pcpu, 2, tmp16, 2); break; /*set 2,(iy+d),h*/
	case 0xd5: pcpu->l = setm(pcpu, 2, tmp16, 2); break; /*set 2,(iy+d),l*/
	case 0xd6:           setm(pcpu, 2, tmp16, 2); break; /*set 2,(iy+d)  */
	case 0xd7: pcpu->a = setm(pcpu, 2, tmp16, 2); break; /*set 2,(iy+d),a*/
	case 0xd8: pcpu->b = setm(pcpu, 3, tmp16, 2); break; /*set 3,(iy+d),b*/
	case 0xd9: pcpu->c = setm(pcpu, 3, tmp16, 2); break; /*set 3,(iy+d),c*/
	case 0xda: pcpu->d = setm(pcpu, 3, tmp16, 2); break; /*set 3,(iy+d),d*/
	case 0xdb: pcpu->e = setm(pcpu, 3, tmp16, 2); break; /*set 3,(iy+d),e*/
	case 0xdc: pcpu->h = setm(pcpu, 3, tmp16, 2); break; /*set 3,(iy+d),h*/
	case 0xdd: pcpu->l = setm(pcpu, 3, tmp16, 2); break; /*set 3,(iy+d),l*/
	case 0xde:           setm(pcpu, 3, tmp16, 2); break; /*set 3,(iy+d)  */
	case 0xdf: pcpu->a = setm(pcpu, 3, tmp16, 2); break; /*set 3,(iy+d),a*/
	case 0xe0: pcpu->b = setm(pcpu, 4, tmp16, 2); break; /*set 4,(iy+d),b*/
	case 0xe1: pcpu->c = setm(pcpu, 4, tmp16, 2); break; /*set 4,(iy+d),c*/
	case 0xe2: pcpu->d = setm(pcpu, 4, tmp16, 2); break; /*set 4,(iy+d),d*/
	case 0xe3: pcpu->e = setm(pcpu, 4, tmp16, 2); break; /*set 4,(iy+d),e*/
	case 0xe4: pcpu->h = setm(pcpu, 4, tmp16, 2); break; /*set 4,(iy+d),h*/
	case 0xe5: pcpu->l = setm(pcpu, 4, tmp16, 2); break; /*set 4,(iy+d),l*/
	case 0xe6:           setm(pcpu, 4, tmp16, 2); break; /*set 4,(iy+d)  */
	case 0xe7: pcpu->a = setm(pcpu, 4, tmp16, 2); break; /*set 4,(iy+d),a*/
	case 0xe8: pcpu->b = setm(pcpu, 5, tmp16, 2); break; /*set 5,(iy+d),b*/
	case 0xe9: pcpu->c = setm(pcpu, 5, tmp16, 2); break; /*set 5,(iy+d),c*/
	case 0xea: pcpu->d = setm(pcpu, 5, tmp16, 2); break; /*set 5,(iy+d),d*/
	case 0xeb: pcpu->e = setm(pcpu, 5, tmp16, 2); break; /*set 5,(iy+d),e*/
	case 0xec: pcpu->h = setm(pcpu, 5, tmp16, 2); break; /*set 5,(iy+d),h*/
	case 0xed: pcpu->l = setm(pcpu, 5, tmp16, 2); break; /*set 5,(iy+d),l*/
	case 0xee:           setm(pcpu, 5, tmp16, 2); break; /*set 5,(iy+d)  */
	case 0xef: pcpu->a = setm(pcpu, 5, tmp16, 2); break; /*set 5,(iy+d),a*/
	case 0xf0: pcpu->b = setm(pcpu, 6, tmp16, 2); break; /*set 6,(iy+d),b*/
	case 0xf1: pcpu->c = setm(pcpu, 6, tmp16, 2); break; /*set 6,(iy+d),c*/
	case 0xf2: pcpu->d = setm(pcpu, 6, tmp16, 2); break; /*set 6,(iy+d),d*/
	case 0xf3: pcpu->e = setm(pcpu, 6, tmp16, 2); break; /*set 6,(iy+d),e*/
	case 0xf4: pcpu->h = setm(pcpu, 6, tmp16, 2); break; /*set 6,(iy+d),h*/
	case 0xf5: pcpu->l = setm(pcpu, 6, tmp16, 2); break; /*set 6,(iy+d),l*/
	case 0xf6:           setm(pcpu, 6, tmp16, 2); break; /*set 6,(iy+d)  */
	case 0xf7: pcpu->a = setm(pcpu, 6, tmp16, 2); break; /*set 6,(iy+d),a*/
	case 0xf8: pcpu->b = setm(pcpu, 7, tmp16, 2); break; /*set 7,(iy+d),b*/
	case 0xf9: pcpu->c = setm(pcpu, 7, tmp16, 2); break; /*set 7,(iy+d),c*/
	case 0xfa: pcpu->d = setm(pcpu, 7, tmp16, 2); break; /*set 7,(iy+d),d*/
	case 0xfb: pcpu->e = setm(pcpu, 7, tmp16, 2); break; /*set 7,(iy+d),e*/
	case 0xfc: pcpu->h = setm(pcpu, 7, tmp16, 2); break; /*set 7,(iy+d),h*/
	case 0xfd: pcpu->l = setm(pcpu, 7, tmp16, 2); break; /*set 7,(iy+d),l*/
	case 0xfe:           setm(pcpu, 7, tmp16, 2); break; /*set 7,(iy+d)  */
	case 0xff: pcpu->a = setm(pcpu, 7, tmp16, 2); break; /*set 7,(iy+d),a*/
	}
      }
      break;						/* FDCB prefix end */

    default:
      pcpu->op1 = pcpu->op2;
      pcpu->mcycle = 0;
      exec_code(pcpu);
      break;
    }
    break;						/* FD prefix end */
  }

}

/*
 * search Z80 machine code
 */
int search_code(const char **mnem, char ixiy, char *input, byte *data, word address)
{
  int i, j;
  int value1, value2, n;
  int narg;
  char format[20];
  char *pos;

  if (strlen(input) == 0) {
    return 0;
  }

  for (i = 0; i < 0x100; i++) {
    strcpy(format, mnem[i]);
    if (strncmp(format, "RST", 3) == 0) {
      format[strlen(format) - 1] = toupper((int)format[strlen(format) - 1]);
    }
    for (j = 0; j < 2; j++) {
      if (pos = strchr(format, '%')) {
	*pos = ixiy;
	*(pos + 1) = toupper((int)*(pos + 1));	/* h,l */
      }
    }
    narg = 0;
    if ((pos = strchr(format, '#'))
	|| (pos = strchr(format, '^'))
	|| (pos = strchr(format, '*'))
	|| (pos = strchr(format, '@'))) {
      memmove(pos + 3, pos + 2, strlen(pos) - 1);
      memcpy(pos, "%xH", 3);
      narg = 1;

      if (pos = strchr(format, '*')) {
	memmove(pos + 3, pos + 2, strlen(pos) - 1);
	memcpy(pos, "%xH", 3);
	narg++;
      }
    }

    strcat(format, "%n");
    n = 0;
    switch (narg) {
    case 0:
      sscanf(input, format, &n);
      break;
    case 1:
      sscanf(input, format, &value1, &n);
      break;
    case 2:
      sscanf(input, format, &value1, &value2, &n);
      break;
    }
    if (n != strlen(input)) {
      continue;
    }

    switch (narg) {
    case 0:
      data[0] = i;
      return 1;
    case 1:
      data[0] = i;
      if (strchr(mnem[i], '#')) {
	value1 = upperhex(value1, 0xffff);
	data[1] = value1 & 0xff;
	data[2] = value1 >> 8;
	return 3;
      } else if (strchr(mnem[i], '*') || strchr(mnem[i], '^')) {
	data[1] = upperhex(value1, 0xff);
	return 2;
      } else if (strchr(mnem[i], '@')) {
	data[1] = (upperhex(value1, 0xffff) - address - 2) & 0xff;
	return 2;
      }
    case 2:
      data[0] = i;
      data[1] = upperhex(value1, 0xff);
      data[2] = upperhex(value2, 0xff);
      return 3;
    }
  }

  return 0;
}


/*
 * get the upper hex digits
 */
int upperhex(int value, int mask)
{
  if (value < 0) {
    value = -value;
    while (value > mask) {
      value >>= 4;
    }
    value = -value;
  }
  while (value > mask) {
    value >>= 4;
  }

  return value;
}


/* INC r */
static inline void inc(z80 *pcpu, byte *var8)
{
  (*var8)++;
  pcpu->f
    = ((*var8 & 0x80) ? SF : 0)
    | (*var8 ? 0: ZF)
    | (*var8 & YF)
    | ((*var8 & 0x0f) ? 0 : HF)
    | (*var8 & XF)
    | ((*var8 == 0x80) ? VF : 0)
    | 0
    | (pcpu->f & CF);
}


/* DEC r */
static inline void dec(z80 *pcpu, byte *var8)
{
  (*var8)--;
  pcpu->f
    = ((*var8 & 0x80) ? SF : 0)
    | (*var8 ? 0: ZF)
    | (*var8 & YF)
    | (((*var8 & 0x0f) == 0x0f) ? HF : 0)
    | (*var8 & XF)
    | ((*var8 == 0x7f) ? VF : 0)
    | NF
    | (pcpu->f & CF);
}


/* change (HL)/(IX+d)/(IY+d) operation */
static inline byte mreg(z80 *pcpu, void (*operation)(), word address, int offset)
{
  static byte tmp8;

  switch (pcpu->mcycle - offset) {
  case 1:
    break;
  case 2:
    tmp8 = z80_read(address);
    break;
  case 3:
    operation(pcpu, &tmp8);
    z80_write(address, tmp8);
    break;
  }
  return tmp8;
}


/* LD r,n */
static inline void ldrn(z80 *pcpu, byte *var8)
{
  switch (pcpu->mcycle) {
  case 1:						break;
  case 2:	*var8 = z80_read(pcpu->pc++);		break;
  }
}


/* LD (IX+d),r */
static inline void ldixr(z80 *pcpu, byte val8)
{
  static byte tmp8;
  static word tmp16;

  switch (pcpu->mcycle) {
  case 2:							break;
  case 3:	tmp8 = z80_read(pcpu->pc++);			break;
  case 4:	tmp16 = (word)((int32_t)IX + (int8_t)tmp8);	break;
  case 5:	z80_write(tmp16, val8);				break;
  }
}


/* LD (IY+d),r */
static inline void ldiyr(z80 *pcpu, byte val8)
{
  static byte tmp8;
  static word tmp16;

  switch (pcpu->mcycle) {
  case 2:							break;
  case 3:	tmp8 = z80_read(pcpu->pc++);			break;
  case 4:	tmp16 = (word)((int32_t)IY + (int8_t)tmp8);	break;
  case 5:	z80_write(tmp16, val8);				break;
  }
}


/* LD r,(IX+d) */
static inline void ldrix(z80 *pcpu, byte *var8)
{
  static byte tmp8;
  static word tmp16;

  switch (pcpu->mcycle) {
  case 2:
    break;
  case 3:
    tmp8 = z80_read(pcpu->pc++);
    break;
  case 4:
    tmp16 = (word)((int32_t)IX + (int8_t)tmp8);
    break;
  case 5:
    *var8 = z80_read(tmp16);
    pcpu->internal = tmp16 >> 8;
    break;
  }
}


/* LD r,(IY+d) */
static inline void ldriy(z80 *pcpu, byte *var8)
{
  static byte tmp8;
  static word tmp16;

  switch (pcpu->mcycle) {
  case 2:
    break;
  case 3:
    tmp8 = z80_read(pcpu->pc++);
    break;
  case 4:
    tmp16 = (word)((int32_t)IY + (int8_t)tmp8);
    break;
  case 5:
    *var8 = z80_read(tmp16);
    pcpu->internal = tmp16 >> 8;
    break;
  }
}


/* add hl/ix/iy,ss */
static inline void addw(z80 *pcpu, byte *high, byte *low, word val16)
{
  static uint32_t tmp32;
  int mcycle;

  if (high == &pcpu->h) {
    mcycle = pcpu->mcycle;
  } else {
    mcycle = pcpu->mcycle - 1;
  }

  switch (mcycle) {
  case 1:
    break;
  case 2:
    pcpu->internal = val16 >> 8;
    tmp32 = *low + (*high << 8) + val16;
    break;
  case 3:
    pcpu->f
      = (pcpu->f & SF)
      | (pcpu->f & ZF)
      | ((tmp32 >> 8) & YF)
      | ((*high ^ (val16 >> 8) ^ (tmp32 >> 8)) & HF)
      | ((tmp32 >> 8) & XF)
      | (pcpu->f & VF)
      | 0
      | ((tmp32 & 0x10000) ? CF : 0);
    *high = (tmp32 >> 8) & 0xff;
    *low = tmp32 & 0xff;
  }
}


/* INC ss */
static inline void incw(byte *high, byte *low)
{
  if (++(*low) == 0) {
    (*high)++;
  }
}


/* DEC ss */
static inline void decw(byte *high, byte *low)
{
  if ((*low)-- == 0) {
    (*high)--;
  }
}


/* JR e */
static inline void jr(z80 *pcpu, byte val8)
{
  word tmp16;
  tmp16 = pcpu->pc + ((val8 < 0x80) ? val8 : (val8 - 0x100));
  pcpu->internal = tmp16 >> 8;
  pcpu->pc = tmp16;
}


/* JR ss,e */
static inline void jrcnd(z80 *pcpu, byte flag, byte condition)
{
  static byte tmp8;

  switch (pcpu->mcycle) {
  case 1:
    break;
  case 2:
    tmp8 = z80_read(pcpu->pc++);
    if ((pcpu->f & flag) != condition) {
      pcpu->mcycle = 0;
    }
    break;
  case 3:
    jr(pcpu, tmp8);
    break;
  }
}


/* LD r,(HL) */
static inline void ldrhl(z80 *pcpu, byte *var8)
{
  switch (pcpu->mcycle) {
  case 1:				break;
  case 2:	*var8 = z80_read(HL);	break;
  }
}


/* LD (HL),r */
static inline void ldhlr(z80 *pcpu, byte val8)
{
  switch (pcpu->mcycle) {
  case 1:				break;
  case 2:	z80_write(HL, val8);	break;
  }
}


/* ADD A, */
static inline void add(z80 *pcpu, byte val8)
{
  word tmp16;

  tmp16 = pcpu->a + val8;
  pcpu->f
    = ((tmp16 & 0x80) ? SF : 0)
    | ((tmp16 & 0xff) ? 0 : ZF)
    | (tmp16 & YF)
    | ((pcpu->a ^ val8 ^ tmp16) & HF)
    | (tmp16 & XF)
    | (((pcpu->a ^ tmp16) & (val8 ^ tmp16) & 0x80) ? VF : 0)
    | 0
    | ((tmp16 & 0x100) ? CF : 0);
  pcpu->a = tmp16 & 0xff;
}


/* ADC A, */
static inline void adc(z80 *pcpu, byte val8)
{
  word tmp16;

  tmp16 = pcpu->a + val8 + (pcpu->f & CF);
  pcpu->f
    = (tmp16 & SF)
    | ((tmp16 & 0xff) ? 0: ZF)
    | (tmp16 & YF)
    | ((pcpu->a ^ val8 ^ tmp16) & HF)
    | (tmp16 & XF)
    | (((pcpu->a ^ tmp16) & (val8 ^ tmp16) & 0x80) ? VF : 0)
    | 0
    | ((tmp16 & 0x100) ? CF : 0);
  pcpu->a = tmp16 & 0xff;
}


/* SUB */
static inline void sub(z80 *pcpu, byte val8)
{
  word tmp16;

  tmp16 = pcpu->a - val8;
  pcpu->f
    = (tmp16 & SF)
    | (tmp16 ? 0: ZF)
    | (tmp16 & YF)
    | ((pcpu->a ^ val8 ^ tmp16) & HF)
    | (tmp16 & XF)
    | (((pcpu->a ^ val8) & (pcpu->a ^ tmp16) & 0x80) ? VF : 0)
    | NF
    | ((pcpu->a < val8) ? CF : 0);
  pcpu->a = tmp16 & 0xff;
}


/* SBC A, */
static inline void sbc(z80 *pcpu, byte val8)
{
  word tmp16;

  tmp16 = pcpu->a - val8 - (pcpu->f & CF);
  pcpu->f
    = (tmp16 & SF)
    | ((tmp16 & 0xff) ? 0: ZF)
    | (tmp16 & YF)
    | ((pcpu->a ^ val8 ^ tmp16) & HF)
    | (tmp16 & XF)
    | (((pcpu->a ^ val8) & (pcpu->a ^ tmp16) & 0x80) ? VF : 0)
    | NF
    | ((pcpu->a < val8 + (pcpu->f & CF)) ? CF : 0);
  pcpu->a = tmp16 & 0xff;
}


/* AND */
static inline void and(z80 *pcpu, byte val8)
{
  pcpu->a = pcpu->a & val8;
  pcpu->f = newflags[pcpu->a] | HF;
}


/* XOR */
static inline void xor(z80 *pcpu, byte val8)
{
  pcpu->a = pcpu->a ^ val8;
  pcpu->f = newflags[pcpu->a];
}
	  

/* OR */
static inline void or(z80 *pcpu, byte val8)
{
  pcpu->a = pcpu->a | val8;
  pcpu->f = newflags[pcpu->a];
}


/* CP */
static inline void cp(z80 *pcpu, byte val8)
{
  word tmp16;

  tmp16 = pcpu->a - val8;
  pcpu->f
    = (tmp16 & SF)
    | (tmp16 ? 0 : ZF)
    | (val8 & YF)
    | ((pcpu->a ^ val8 ^ tmp16) & HF)
    | (val8 & XF)
    | (((pcpu->a ^ val8) & (pcpu->a ^ tmp16) & 0x80) ? VF : 0)
    | NF
    | ((pcpu->a < val8) ? CF : 0);
}


/* RET cc */
static inline void retcnd(z80 *pcpu, byte flag, byte condition, int offset)
{
  static byte tmp8;

  switch (pcpu->mcycle + offset) {
  case 1:
    if ((pcpu->f & flag) != condition) {
      pcpu->mcycle = 0;
    }
    break;
  case 2:	tmp8 = z80_read(pcpu->sp++);			break;
  case 3:	pcpu->pc = tmp8 + (z80_read(pcpu->sp++) << 8);	break;
  }
}


/* PUSH */
static inline void push(z80 *pcpu, byte high, byte low)
{
  switch (pcpu->mcycle) {
  case 1:						break;
  case 2:	z80_write(--pcpu->sp, high);		break;
  case 3:	z80_write(--pcpu->sp, low);		break;
  }
}


/* PUSH IX/IY*/
static inline void pushixiy(z80 *pcpu, byte high, byte low)
{
  switch (pcpu->mcycle) {
  case 2:						break;
  case 3:	z80_write(--pcpu->sp, high);		break;
  case 4:	z80_write(--pcpu->sp, low);		break;
  }
}


/* POP */
static inline void pop(z80 *pcpu, byte *high, byte *low)
{
  switch (pcpu->mcycle) {
  case 1:						break;
  case 2:	*low = z80_read(pcpu->sp++);		break;
  case 3:	*high = z80_read(pcpu->sp++);		break;
  }
}


/* POP IX/IY*/
static inline void popixiy(z80 *pcpu, byte *high, byte *low)
{
  switch (pcpu->mcycle) {
  case 2:						break;
  case 3:	*low = z80_read(pcpu->sp++);		break;
  case 4:	*high = z80_read(pcpu->sp++);		break;
  }
}


/* JP cc, */
static inline void jpcnd(z80 *pcpu, byte flag, byte condition)
{
  static word tmp16;

  switch (pcpu->mcycle) {
  case 1:
    break;
  case 2:
    tmp16 = z80_read(pcpu->pc++);
    break;
  case 3:
    tmp16 += z80_read(pcpu->pc++) << 8;
    if ((pcpu->f & flag) == condition) {
      pcpu->pc = tmp16;
    }
    break;
  }
}


/* CALL cc, */
static inline void callcnd(z80 *pcpu, byte flag, byte condition)
{
  static word tmp16;

  switch (pcpu->mcycle) {
  case 1:
    break;
  case 2:
    tmp16 = z80_read(pcpu->pc++);
    break;
  case 3:
    tmp16 += z80_read(pcpu->pc++) << 8;
    if ((pcpu->f & flag) != condition) {
      pcpu->cycles -= 1;
      pcpu->mcycle = 0;
    }
    break;
  case 4:
    z80_write(--pcpu->sp, pcpu->pc >> 8);
    break;
  case 5:
    z80_write(--pcpu->sp, pcpu->pc & 0xff);
    pcpu->pc = tmp16;
    break;      
  }
}


/* RST */
static inline void rst(z80 *pcpu, word address)
{
  switch (pcpu->mcycle) {
  case 1:
    break;
  case 2:
    z80_write(--pcpu->sp, pcpu->pc >> 8);
    break;
  case 3:
    z80_write(--pcpu->sp, pcpu->pc & 0xff);
    pcpu->pc = address;
    break;      
  }
}


/* SBC HL, */
static inline void sbchl(z80 *pcpu, word val16)
{
  static uint32_t tmp32;

  switch (pcpu->mcycle) {
  case 3:
    tmp32 = HL - val16 - (pcpu->f & CF);
    pcpu->f
      = ((tmp32 >> 8) & SF)
      | ((tmp32 & 0xffff) ? 0 : ZF)
      | ((tmp32 >> 8) & YF)
      | ((pcpu->h ^ (val16 >> 8) ^ (tmp32 >> 8)) & HF)
      | ((tmp32 >> 8) & XF)
      | ((((pcpu->h << 8) ^ val16) & ((pcpu->h << 8) ^ tmp32) & 0x8000) ? VF : 0)
      | NF
      | ((tmp32 > 0xffff) ? CF : 0);
    break;
  case 4:
    pcpu->h = (tmp32 >> 8) & 0xff;
    pcpu->l = tmp32 & 0xff;
    break;
  }
}


/* ADC HL, */
static inline void adchl(z80 *pcpu, word val16)
{
  static uint32_t tmp32;

  switch (pcpu->mcycle) {
  case 3:
    tmp32 = HL + val16 + (pcpu->f & CF);
    pcpu->f
      = ((tmp32 >> 8) & SF)
      | ((tmp32 & 0xffff) ? 0 : ZF)
      | ((tmp32 >> 8) & YF)
      | ((pcpu->h ^ (val16 >> 8) ^ (tmp32 >> 8)) & HF)
      | ((tmp32 >> 8) & XF)
      | ((((pcpu->h << 8) ^ tmp32) & (val16 ^ tmp32) & 0x8000) ? VF : 0)
      | 0
      | ((tmp32 & 0x10000) ? CF : 0);
    break;
  case 4:
    pcpu->h = (tmp32 >> 8) & 0xff;
    pcpu->l = tmp32 & 0xff;
    break;
  }
}


/* LDI */
static inline void ldi(z80 *pcpu)
{
  block_copy(pcpu);
  if (pcpu->mcycle == 4) {
    incw(&pcpu->d, &pcpu->e);
    incw(&pcpu->h, &pcpu->l);
  }
}


/* LDD */
static inline void ldd(z80 *pcpu)
{
  block_copy(pcpu);
  if (pcpu->mcycle == 4) {
    decw(&pcpu->d, &pcpu->e);
    decw(&pcpu->h, &pcpu->l);
  }
}


/* block copy for LDI/LDD */
static inline void block_copy(z80 *pcpu)
{
  static byte tmp8;

  switch (pcpu->mcycle) {
  case 2:
    break;
  case 3:
    tmp8 = z80_read(HL);
    break;
  case 4:
    z80_write(DE, tmp8);
    tmp8 += pcpu->a;
    decw(&pcpu->b, &pcpu->c);
    pcpu->f
      = (pcpu->f & SF)
      | (pcpu->f & ZF)
      | ((tmp8 & 0x02) ? YF : 0)
      | 0
      | (tmp8 & XF)
      | (BC ? PF : 0)
      | 0
      | (pcpu->f & CF);
    break;
  }
}


/* CPI */
static inline void cpi(z80 *pcpu)
{
  block_search(pcpu);
  if (pcpu->mcycle == 4) {
    incw(&pcpu->h, &pcpu->l);
  }
}


/* CPD */
static inline void cpd(z80 *pcpu)
{
  block_search(pcpu);
  if (pcpu->mcycle == 4) {
    decw(&pcpu->h, &pcpu->l);
  }
}


/* block search for CPI/CPD */
static inline void block_search(z80 *pcpu)
{
  static byte tmp8;
  word tmp16;

  switch (pcpu->mcycle) {
  case 2:
    break;
  case 3:
    decw(&pcpu->b, &pcpu->c);
    tmp8 = z80_read(HL);
    break;
  case 4:
    tmp16 = pcpu->a - tmp8;
    pcpu->f
      = (tmp16 & SF)
      | (tmp16 ? 0 : ZF)
      | (pcpu->a ^ tmp8 ^ tmp16) & HF
      | (BC ? PF : 0)
      | NF
      | (pcpu->f & CF);
    tmp16 -= ((pcpu->f & HF) ? 1 : 0);
    pcpu->f
      |= ((tmp16 & 0x02) ? YF : 0)
      | (tmp16 & XF);
    break;
  }
}


/* repeat for LDIR/LDDR */
static inline void repbc(z80 *pcpu)
{
  switch (pcpu->mcycle) {
  case 4:
    if ((pcpu->f & PF) == 0) {
      pcpu->mcycle = 0;
    }
    break;
  case 5:
    pcpu->pc -= 2;
    break;
  }
}


/* repeat for CPIR/CPDR */
static inline void repcp(z80 *pcpu)
{
  switch (pcpu->mcycle) {
  case 4:
    if ((pcpu->f & PF) == 0 || (pcpu->f & ZF)) {
      pcpu->mcycle = 0;
    }
    break;
  case 5:
    pcpu->pc -= 2;
    break;
  }
}


/* repeat for INIR/INDR/OUTIR/OUTDR */
static inline void repb(z80 *pcpu)
{
  switch (pcpu->mcycle) {
  case 4:
    if (pcpu->b == 0) {
      pcpu->mcycle = 0;
    }
    break;
  case 5:
    pcpu->pc -= 2;
    break;
  }
}

  /* INI */
static inline void ini(z80 *pcpu)
{
  byte tmp8;
  word tmp16;
  
  tmp8 = block_input(pcpu);
  if (pcpu->mcycle == 4) {
    incw(&pcpu->h, &pcpu->l);
    tmp16 = tmp8 + ((pcpu->c + 1) & 0xff);
    pcpu->f
      = ((pcpu->b & 0x80) ? SF : 0)
      | (pcpu->b ? 0 : ZF)
      | (pcpu->b & YF)
      | ((tmp16 > 0xff) ? HF : 0)
      | (pcpu->b & XF)
      | parity_table[(tmp16 & 0x07) ^ pcpu->b]
      | ((tmp8 & 0x80) ? NF : 0)
      | ((tmp16 > 0xff) ? CF : 0);
  }
}


/* IND */
static inline void ind(z80 *pcpu)
{
  byte tmp8;
  word tmp16;
  
  tmp8 = block_input(pcpu);
  if (pcpu->mcycle == 4) {
    decw(&pcpu->h, &pcpu->l);
    tmp16 = tmp8 + ((pcpu->c - 1) & 0xff);
    pcpu->f
      = ((pcpu->b & 0x80) ? SF : 0)
      | (pcpu->b ? 0 : ZF)
      | (pcpu->b & YF)
      | ((tmp16 > 0xff) ? HF : 0)
      | (pcpu->b & XF)
      | parity_table[(tmp16 & 0x07) ^ pcpu->b]
      | ((tmp8 & 0x80) ? NF : 0)
      | ((tmp16 > 0xff) ? CF : 0);
  }
}


/* block input for INI/IND */
static inline byte block_input(z80 *pcpu)
{
  static byte tmp8;

  switch (pcpu->mcycle) {
  case 2:
    break;
  case 3:
    tmp8 = z80_in(BC);
    break;
  case 4:
    z80_write(HL, tmp8);
    pcpu->b--;
    break;
  }

  return tmp8;
}


/* OUTI */
static inline void outi(z80 *pcpu)
{
  byte tmp8;
  word tmp16;

  tmp8 = block_output(pcpu);
  if (pcpu->mcycle == 4) {
    incw(&pcpu->h, &pcpu->l);
    tmp16 = tmp8 + pcpu->l;
    pcpu->f
      = ((pcpu->b & 0x80) ? SF : 0)
      | (pcpu->b ? 0 : ZF)
      | (pcpu->b & YF)
      | ((tmp16 > 0xff) ? HF : 0)
      | (pcpu->b & XF)
      | parity_table[(tmp16 & 0x07) ^ pcpu->b]
      | ((tmp8 & 0x80) ? NF : 0)
      | ((tmp16 > 0xff) ? CF : 0);
  }
}


/* OUTD */
static inline void outd(z80 *pcpu)
{
  byte tmp8;
  word tmp16;

  tmp8 = block_output(pcpu);
  if (pcpu->mcycle == 4) {
    decw(&pcpu->h, &pcpu->l);
    tmp16 = tmp8 + pcpu->l;
    pcpu->f
      = ((pcpu->b & 0x80) ? SF : 0)
      | (pcpu->b ? 0 : ZF)
      | (pcpu->b & YF)
      | ((tmp16 > 0xff) ? HF : 0)
      | (pcpu->b & XF)
      | parity_table[(tmp16 & 0x07) ^ pcpu->b]
      | ((tmp8 & 0x80) ? NF : 0)
      | ((tmp16 > 0xff) ? CF : 0);
  }
}


/* block output for OUTI/OUTD */
static inline byte block_output(z80 *pcpu)
{
  static byte tmp8;

  switch (pcpu->mcycle) {
  case 2:						break;
  case 3:	tmp8 = z80_read(HL);			break;
  case 4:	pcpu->b--;	z80_out(BC, tmp8);	break;
  }

  return tmp8;
}

/* RLC */
static inline void rlc(z80 *pcpu, byte *var8)
{
  *var8 = (*var8 << 1) + (*var8 >> 7);
  pcpu->f = newflags[*var8] | ((*var8 & 0x01) ? CF : 0);
}


/* RRC */
static inline void rrc(z80 *pcpu, byte *var8)
{
  *var8 = (*var8 >> 1) + (*var8 << 7);
  pcpu->f = newflags[*var8] | ((*var8 & 0x80) ? CF : 0);
}


/* RL */
static inline void rl(z80 *pcpu, byte *var8)
{
  if (*var8 & 0x80) {
    *var8 = (*var8 << 1) + ((pcpu->f & CF) ? 0x01 : 0);
    pcpu->f = newflags[*var8] | CF;
  } else {
    *var8 = (*var8 << 1) + ((pcpu->f & CF) ? 0x01 : 0);
    pcpu->f = newflags[*var8];
  }
}


/* RR */
static inline void rr(z80 *pcpu, byte *var8)
{
  if (*var8 & 0x01) {
    *var8 = (*var8 >> 1) + ((pcpu->f & CF) ? 0x80 : 0);
    pcpu->f = newflags[*var8] | CF;
  } else {
    *var8 = (*var8 >> 1) + ((pcpu->f & CF) ? 0x80 : 0);
    pcpu->f = newflags[*var8];
  }
}


/* SLA */
static inline void sla(z80 *pcpu, byte *var8)
{
  pcpu->f = (*var8 & 0x80) ? CF : 0;
  *var8 = *var8 << 1;
  pcpu->f |= newflags[*var8];
}


/* SRA */
static inline void sra(z80 *pcpu, byte *var8)
{
  pcpu->f = (*var8 & 0x01) ? CF : 0;
  *var8 = (*var8 & 0x80) | (*var8 >> 1);
  pcpu->f |= newflags[*var8];
}


/* SLL */
static inline void sll(z80 *pcpu, byte *var8)
{
  pcpu->f = (*var8 & 0x80) ? CF : 0;
  *var8 = (*var8 << 1) + 0x01;
  pcpu->f |= newflags[*var8];
}


/* SRL */
static inline void srl(z80 *pcpu, byte *var8)
{
  pcpu->f = (*var8 & 0x01) ? CF : 0;
  *var8 = *var8 >> 1;
  pcpu->f |= newflags[*var8];
}


/* BIT */
static inline void bit(z80 *pcpu, int n, byte val8)
{
  byte tmp8;

  tmp8 = val8 & (1 << n);
  pcpu->f
    = (tmp8 & SF)
    | (tmp8 ? 0 : ZF)
    | (val8 & YF)
    | HF
    | (val8 & XF)
    | (tmp8 ? 0 : PF)
    | 0
    | (pcpu->f & CF);
}


/* BIT *,(HL) */
static inline void bitm(z80 *pcpu, int n)
{
  byte tmp8;
  
  switch (pcpu->mcycle) {
  case 2:
    break;
  case 3:
    tmp8 = z80_read(HL) & (1 << (n));
    pcpu->f
      = (tmp8 & SF)
      | (tmp8 ? 0 : ZF)
      | (pcpu->internal & YF)
      | HF
      | (pcpu->internal & XF)
      | (tmp8 ? 0 : PF)
      | 0
      | (pcpu->f & CF);
    break;
  }
}

/* RES *,(HL)/(IX+d)/(IY+d) */
static inline byte resm(z80 *pcpu, int n, word address, int offset)
{
  static byte tmp8;
  
  switch (pcpu->mcycle - offset) {
  case 2:						break;
  case 3:	tmp8 = z80_read(address) & ~(1 << n);	break;
  case 4:	z80_write(address, tmp8);		break;
  }
  return tmp8;
}


/* SET *,(HL) */
static inline byte setm(z80 *pcpu, int n, word address, int offset)
{
  static byte tmp8;
  
  switch (pcpu->mcycle - offset) {
  case 2:						break;
  case 3:	tmp8 = z80_read(address) | (1 << n);	break;
  case 4:	z80_write(address, tmp8);		break;
  }
  return tmp8;
}


/* DDCB/FDCB bit operation */
static inline byte ixyn(z80 *pcpu, int n, void (*operation)(), word address)
{
  static byte tmp8;

  switch (pcpu->mcycle) {
  case 5:
    tmp8 = z80_read(address);
    break;
  case 6:
    operation(pcpu, n, &tmp8);
    z80_write(address, tmp8);
    break;
  }
  return tmp8;
}


/* DDCB/FDCB bit operation */
static inline void bitixy(z80 *pcpu, int n, word address)
{
  byte tmp8;

  tmp8 = z80_read(address) & (1 << n);
  pcpu->f
    = (tmp8 & SF)
    | (tmp8 ? 0 : ZF)
    | ((address >> 8) & YF)
    | HF
    | ((address >> 8) & XF)
    | (tmp8 ? 0 : PF)
    | 0
    | (pcpu->f & CF);
}

static inline void intmode2(z80 *pcpu, word address)
{
  static byte tmp8;
  static word tmp16;

  switch (pcpu->mcycle) {
  case 1:
    tmp16 = address;
    break;
  case 2:
    z80_write(--pcpu->sp, pcpu->pc >> 8);
    break;
  case 3:
    z80_write(--pcpu->sp, pcpu->pc & 0xff);
    break;
  case 4:
    tmp8 = z80_read(tmp16);
    break;
  case 5:
    pcpu->pc = tmp8 + (z80_read(tmp16 + 1) << 8);
    break;      
  }
}

