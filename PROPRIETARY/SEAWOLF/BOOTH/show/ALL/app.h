#ifndef __app_h
#define __app_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2013.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

//+++ "lib_app.c"

#include "sysio.h"	// so we read options.sys for ANDROIDEMO

#define DEF_NID         13
#define PINDA_ID	99

#define NI_LEN  7

#if ANDROIDEMO
#define PEG_STR_LEN 33
#else
#define PEG_STR_LEN 15
#endif

typedef word profi_t;

// see this crap in trueconst_sui.cc... and this is NOT funny
#define PROF_SUI	1
#define PROF_LCD	2
#define PROF_KIOSK	4
#define PROF_SENS	8
#define PROF_RXOFF	16
#define PROF_TAG	32
#define PROF_HUNT	64
#define PROF_VIRT	128

//+++ "lib_app.c"
// in lib_app.cc:
char * 	get_mem (word state, int len);
void 	send_msg (char * buf, int size);
void shuffle_hunt (word * a);

#endif
