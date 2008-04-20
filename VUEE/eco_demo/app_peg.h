#ifndef __app_peg_h
#define __app_peg_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#define THREADNAME(a)   a ## _peg

// Content of all LibApps

//+++ "lib_app_peg.c"
//+++ "msg_io_peg.c"
//+++ "app_diag_peg.c"

#include "sysio.h"
#include "msg_tarp.h"
#include "msg_structs_peg.h"

#define EE_AGG_SIZE	sizeof(aggEEDataType)
#define EE_AGG_MAX	(ee_size (NULL, NULL) / EE_AGG_SIZE -1)
#define EE_AGG_MIN	0

#define NUM_SENS	5

#define AGG_FF		0xFF
#define AGG_IN_USE	0xFC
#define AGG_COLLECTED	0xF8
#define AGG_CONFIRMED	0xF0

#define ERR_EER		0xFFFE
#define ERR_SLOT	0xFFFC
#define ERR_FULL	0xFFF8

typedef enum {
	noTag, newTag, reportedTag, confirmedTag,
	fadingReportedTag, fadingConfirmedTag, goneTag, sumTag
} tagStateType;

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

typedef struct tagDataStruct {
	word	id;
	
	word	rssi:8;
	word	pl:4;
	word	state:4;
	
	word	count:8;
	word	rxperm:1;
	word	spare7:7;
	
	word	freq;
	
	lword	evTime;
	lword	lastTime;
	reportPloadType rpload;
} tagDataType;

// waiting room
typedef struct wroomStruct {
	char * buf;
	lword  tstamp;
} wroomType;

// wasteful on demo purpose
typedef struct aggEEDataStruct {
	word status:8; // 1st in ee slot
	word spare:8;
	word sval[NUM_SENS]; // aligned
	lword ts;
	lword t_ts;
	lword t_eslot;
	word  tag;
	word sspare[4];
} aggEEDataType;
// for now, keep it at 2^N (32), we'll see about eeprom pages, etc.

typedef struct aggEEDumpStruct {
	aggEEDataType ee;
	lword	fr;
	lword	to;
	lword	ind;
	lword	cnt;
	nid_t	tag;
	word	dfin:1;
	word	upto:15;
} aggEEDumpType;

typedef struct aggDataStruct {
	aggEEDataType ee;
	lword eslot;
} aggDataType;

#define clr_master_chg	(app_flags &= ~2)
#define is_master_chg	(app_flags & 2)

#define tag_lim	20
#endif
