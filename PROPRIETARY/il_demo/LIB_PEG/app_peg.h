/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __app_peg_h
#define __app_peg_h

#include "sysio.h"
#include "msg_tarp.h"
#include "msg_structs_peg.h"
#include "storage.h"

#include "ee2sd.h"

#define EE_AGG_SIZE	sizeof(aggEEDataType)
#define EE_AGG_MAX	(lword)(ee_size (NULL, NULL) / EE_AGG_SIZE -1)
#define EE_AGG_MIN	0L
// test: #define EE_AGG_MIN (EE_AGG_MAX -8)

#define NUM_SENS	6

#define IS_BYTE_EMPTY(x)	((x) == 0 || (x) == 0xFF)
#define IS_AGG_EMPTY(x)		((x) == 0 || (x) == 0xF)
#define AGG_EMPTY	0xF
// not needed: #define AGG_IN_USE	0xC
#define AGG_COLLECTED	0x8

#define AGG_CONFIRMED   1

// plot marker
#define AGG_ALL		0xE

// mark is :3 (sort-of substatus, not checked for 'emptiness' in sd / eprom)
#define MARK_EMPTY	7
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
	lint secs;
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
	statu_t s; // 1st 2 bytes in ee slot
	word	tag;
	lint ds;
	lint t_ds;
	lword t_eslot;
	word sval [NUM_SENS];
} aggEEDataType; // 16 + NUM_SENS * 2

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

/* app_flags definition [default]:
bit 0: not used [0]
bit 1: master changed (in TARP) [0]
bit 2: ee write collected [0]
bit 3: ee write confirmed [0]
bit 4: ee overwrite (cyclic stack) [0]

note that on tags the 2nd byte is used for alarms
*/
#define DEF_APP_FLAGS   0x0

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

#define tag_lim	20
#endif
