/*
 * MZ-80K/C & MZ-700 emulator on terminal
 *   CMT emulation
 *
 * Copyright (c) 2023, Yuichi Nakamura <yunk@ya2.so-net.ne.jp>
 * This software is released under 2-clause BSD license.
 */

#ifndef _MZ80CMT_H_
#define _MZ80CMT_H_

int mz80cmt_motorstat(void);
void mz80cmt_motoron(int stat, int cycle);
int mz80cmt_read(void);
void mz80cmt_write(int bit, int cycle);

char *mz80cmt_loadfilename(void);

#endif /* _MZ80CMT_H_ */
