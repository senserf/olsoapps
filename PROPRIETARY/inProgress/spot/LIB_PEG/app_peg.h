#ifndef __app_peg_h
#define __app_peg_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2011                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// Content of all LibApps

//+++ "lib_app_peg.cc"
//+++ "msg_io_peg.cc"
//+++ "app_diag_peg.cc"

#include "sysio.h"
#include "msg_tarp.h"
#include "msg_structs_peg.h"
#include "commconst.h"

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
		word w;
		struct {
			word emptym :1;
			word mark   :3;
			word status :4;
			word spare1 :4; // setsen :4;
			word spare  :4;
	} f;
} statu_t;

typedef struct tagDataStruct {
	word	id;
	
	word	rssi:8;
	word	pl:4;
	word	state:4;
	
	word	count:8;
	word	rxperm:1;
	word	spare:4; // setsen:4;
	word	spare3:3;
	
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
	word sval [MAX_SENS];
} aggEEDataType; // 16 + MAX_SENS * 2

typedef struct aggDataStruct {
	aggEEDataType ee;
	lword eslot;
} aggDataType;

// agg flags over OSS
#define A_FL_EEW_COLL   1
#define A_FL_EEW_CONF   2
#define A_FL_EEW_OVER   4

/* app_flags definition [default]:
bit 0: no more used  [0]
bit 1: master changed (in TARP) [0]
bit 2: ee write collected [1]
bit 3: ee write confirmed [0]
bit 4: ee overwrite (cyclic stack) [0]
*/
#define DEF_APP_FLAGS   0x4

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
