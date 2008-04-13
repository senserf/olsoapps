#ifndef __app_tag_h__
#define __app_tag_h__
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#define THREADNAME(a)   a ## _tag

//+++ "lib_app_tag.c"
//+++ "msg_io_tag.c"
//+++ "app_diag_tag.c"

#include "sysio.h"
#include "msg_tarp.h"

#define EE_SENS_SIZE	sizeof(sensEEDataType)
#define EE_SENS_MAX	(ee_size (NULL, NULL) / EE_SENS_SIZE -1)
#define EE_SENS_MIN	0

// check what it really is
#define SENS_COLL_TIME	1500
#define NUM_SENS	5

#define SENS_FF		0xFF
#define SENS_IN_USE	0xFC
#define SENS_COLLECTED	0xF8
#define SENS_CONFIRMED	0xF0

#define ERR_EER		0xFFFE
#define ERR_SLOT	0xFFFC
#define ERR_FULL	0xFFF8

typedef union {
	lword sec;
	struct {
		word d:11;
		word h:5;
		word m:6;
		word s:6;
		word spare:3;
		word f:1;
	} hms;
} mclock_t;

typedef struct pongParamsStruct {
	word freq_maj;
	word freq_min;
	word pow_levels;
	word rx_span;
	word rx_lev:8;
	word pload_lev:8;
} pongParamsType;

typedef struct sensEEDataStruct {
	word status:8; 	// keep it byte 0 of the ee slot
	word spare:8;
	word sval[NUM_SENS];
	lword ts; 	// keep it aligned
} sensEEDataType;
// for now, keep it at 2^N (16), we'll see about eeprom pages, etc.

typedef struct sensEEDumpStruct {
	sensEEDataType ee;
	lword   fr;
	lword   to;
	lword   ind;
	lword   cnt;
	word    upto;
	word	status:8;
	word    dfin:1;
	word	spare:7;
} sensEEDumpType;

typedef struct sensDataStruct {
	sensEEDataType ee;
	lword eslot;
} sensDataType;

#endif
