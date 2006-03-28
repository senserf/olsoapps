#ifndef __nvm_h
#define __nvm_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
//+++ "nvm.c"
#include "sysio.h"

#if EEPROM_DRIVER
#define ESN_SIZE	1008
#define NVM_PAGE_SIZE	(EE_PAGE_SIZE >> 1)
#else
#define ESN_SIZE	48
#define NVM_PAGE_SIZE	16
#endif

#define SVEC_SIZE	(ESN_SIZE >> 4)
#define ESN_BSIZE	8
#define ESN_OSET	1
#define ESN_PACK	5

// IMPORTANT: always keep them TOGETHER and away from the ESN's space
#define NVM_NID		0
#define NVM_LH		(NVM_NID + 1)
#define NVM_MID		(NVM_NID + 2)
#define NVM_APP		(NVM_NID + 3)
// NVM_APP: b0-b2: encr; b3-b15 spare

extern void nvm_read (word pos, word * d, word wlen);
extern void nvm_write (word pos, const word * s, word wlen);
extern void nvm_erase();
extern word esn_count;
extern lword esns[];
extern int lookup_esn (lword * esn);
extern int lookup_esn_st (lword * esn);
extern int get_next (lword * esn, word st);
extern bool load_esns (char * buf);
extern void set_svec (int i, bool what);
extern int add_esn (lword * esn, int * pos);
extern int era_esn (lword * esn);
extern void app_reset (word lev);
extern word count_esn();
extern word s_count();

#endif

