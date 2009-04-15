#ifndef __app_peg_h
#define __app_peg_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009.                   */
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
#include "storage.h"

#include "ee2sd.h"

#define EE_AGG_SIZE	sizeof(aggEEDataType)
#define EE_AGG_MAX	(lword)(ee_size (NULL, NULL) / EE_AGG_SIZE -1)
#define EE_AGG_MIN	0L
// test: #define EE_AGG_MIN (EE_AGG_MAX -8)

#define NUM_SENS	5

#if EMPTY_STORAGE_FF
#define EMPTY_BYTE      0xFF
#define AGG_EMPTY       0xF
#else
#define EMPTY_BYTE	0
#define AGG_EMPTY	0
#endif

// not needed? #define AGG_IN_USE	0xC
#define AGG_COLLECTED	0x8

#if EMPTY_STORAGE_FF
#define AGG_CONFIRMED   0
#else
#define AGG_CONFIRMED	1
#endif

// plot marker
#define AGG_ALL		0xE

// mark is :3
#if EMPTY_STORAGE_FF
#define MARK_EMPTY      7
#else
#define MARK_EMPTY	0
#endif

#define MARK_BOOT	6
#define MARK_PLOT	5
#define MARK_SYNC	4
#define MARK_MCHG	3
#define MARK_DATE	2

#define ERR_EER		0xFFFE
#define ERR_SLOT	0xFFFC
// _FULL may be needed for important data retention
#define ERR_FULL	0xFFF8
#define ERR_MAINT	0xFFF0

#define LED_R   0
#define LED_G   1
#define LED_B   2

#define LED_OFF 0
#define LED_ON  1
#define LED_BLINK 2

#define SIY     (365L * 24 * 60 * 60)
#define SID     (24L * 60 * 60)
#define TIME_TOLER	2

typedef enum {
	noTag, newTag, reportedTag, confirmedTag,
	fadingReportedTag, fadingConfirmedTag, goneTag, sumTag
} tagStateType;

typedef union {
	long secs;
	struct {
		word f  :1;
		word yy :5;
		word dd :5;
		word h  :5; // just in case, no word crossing
		word mm :4;
		word m  :6;
		word s  :6;
	} dat;
} mdate_t;

typedef union {
		word b:8;
		struct {
			word emptym :1;
			word mark   :3;
			word status :4;
	} f;
	word spare :8;
} statu_t;

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
	statu_t s; // 1st byte in ee slot
	word sval [NUM_SENS]; // aligned
	long ds;
	long t_ds;
	lword t_eslot;
	word  tag;
	word sspare[3];
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

// agg flags over OSS
#define A_FL_EEW_COLL   1
#define A_FL_EEW_CONF   2
#define A_FL_EEW_OVER   4

/* app_flags definition [default]:
bit 0: satest (for monitoring via RF, unique to pegs) [0]
bit 1: master changed (in TARP) [0]
bit 2: ee write collected [1]
bit 3: ee write confirmed [0]
bit 4: ee overwrite (cyclic stack) [0]
bit 5: ee marker of empty slots [1]
*/
#define DEF_APP_FLAGS   0x2C

#define set_satest	(app_flags |= 1)
#define clr_satest	(app_flags &= ~1)
#define is_satest	(app_flags & 1)

#define clr_master_chg	(app_flags &= ~2)
#define is_master_chg	(app_flags & 2)

#define set_eew_coll	(app_flags |= 4)
#define clr_eew_coll	(app_flags &= ~4)
#define is_eew_coll	(app_flags & 4)

#define set_eew_conf    (app_flags |= 8)
#define clr_eew_conf    (app_flags &= ~8)
#define is_eew_conf     (app_flags & 8)

#define set_eew_over    (app_flags |= 16)
#define clr_eew_over    (app_flags &= ~16)
#define is_eew_over     (app_flags & 16)

#define set_eem_empty   (app_flags |= 32)
#define clr_eem_empty   (app_flags &= ~32)
#define is_eem_empty    (app_flags & 32)
#define ee_emptym	((app_flags >> 5) & 1)

#define tag_lim	20
#endif
