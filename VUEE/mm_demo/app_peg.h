#ifndef __app_peg_h
#define __app_peg_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#define	THREADNAME(a)	a ## _peg

// Content of all LibApps

//+++ "lib_app_peg.c"
//+++ "msg_io_peg.c"
//+++ "app_diag_peg.c"

#include "sysio.h"
#include "msg_tarp.h"

#define clr_master_chg  (app_flags &= ~2)
#define is_master_chg   (app_flags & 2)

#define LI_MAX  6
#define NI_LEN  8
#define PEG_STR_LEN 16

#define INFO_DESC	1
#define INFO_BIZ	2
#define INFO_PRIV	4
#define INFO_SPARE	8
#define INFO_ACK	16

#define LED_R	0
#define LED_G	1
#define LED_B	2
#define LED_N	3
#define LED_OFF	0
#define LED_ON	1
#define LED_BLINK 2

typedef enum {
	noTag, newTag, reportedTag, confirmedTag, matchedTag,
	fadingReportedTag, fadingConfirmedTag, fadingMatchedTag,
	goneTag, sumTag
} tagStateType;

typedef struct tagDataStruct {
	lword   evTime;
	lword   lastTime;
	word	id;
	word	rssi:8;
	word	pl:4;
	word	state:4;
	word	intim:1;
	word	info:7;
	word	profi;
	char	nick[NI_LEN];
	char	desc[PEG_STR_LEN];
} tagDataType;

typedef struct tagShortStruct {
	word	id;
	char	nick[NI_LEN];
} tagShortType;

typedef struct ledStateStruct {
	word	color:4;
	word	state:4;
	word	dura:8;
} ledStateType;

#define clr_master_chg	(app_flags &= ~2)
#define is_master_chg	(app_flags & 2)

#define OSS_ASCII_DEF	0
#define OSS_ASCII_RAW	1
#define oss_fmt	OSS_ASCII_DEF

typedef word profi_t;
#endif
