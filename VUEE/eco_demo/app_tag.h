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
#define SENS_COLL_TIME	3000
#define NUM_SENS	5

#define SENS_FF		0xF
#define SENS_IN_USE	0xC
#define SENS_COLLECTED	0x8
#define SENS_CONFIRMED	0x0
// this one is a wildcard for reporting only
#define SENS_ALL	0xE

#define ERR_EER		0xFFFE
#define ERR_SLOT	0xFFFC
#define ERR_FULL	0xFFF8
#define ERR_MAINT	0xFFF0

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

typedef union {
	byte b;
	struct {
		word emptym :1;
		word spare  :3;
		word status :4;
	} f;
} statu_t;

typedef struct pongParamsStruct {
	word freq_maj;
	word freq_min;
	word pow_levels;
	word rx_span;
	word rx_lev:8;
	word pload_lev:8;
} pongParamsType;

typedef struct sensEEDataStruct {
	statu_t	s; // keep it byte 0 of the ee slot
	word spare  :8;
	word sval [NUM_SENS];
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
	statu_t s;
	word    dfin   :1;
	word	spare  :7;
} sensEEDumpType;

typedef struct sensDataStruct {
	sensEEDataType ee;
	lword eslot;
} sensDataType;

/* app_flags definition [default]:
   bit 0: spare [0]
   bit 1: master chganged (in TARP) [0]
   bit 2: ee write collected [1]
   bit 3: ee write confirmed [0]
   bit 4: ee overwrite (cyclic stack) [1]
   bit 5: ee marker of empty slots [1]
   */
#define DEF_APP_FLAGS   0x34

// master_cgh not used, but needed for TARP

#define set_eew_coll    (app_flags |= 4)
#define clr_eew_coll    (app_flags &= ~4)
#define is_eew_coll     (app_flags & 4)

#define set_eew_conf    (app_flags |= 8)
#define clr_eew_conf    (app_flags &= ~8)
#define is_eew_conf     (app_flags & 8)

#define set_eew_over    (app_flags |= 16)
#define clr_eew_over    (app_flags &= ~16)
#define is_eew_over     (app_flags & 16)

#define set_eem_empty   (app_flags |= 32)
#define clr_eem_empty   (app_flags &= ~32)
#define is_eem_empty    (app_flags & 32)
#define ee_emptym       ((app_flags >> 5) & 1)


#endif
