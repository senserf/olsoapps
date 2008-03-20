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

#define DEF_APP_FLAGS 	0x0001
#define is_autoack	(app_flags & 1)
#define set_autoack	(app_flags |= 1)
#define clr_autoack	(app_flags &= ~1)

#define clr_master_chg  (app_flags &= ~2)
#define is_master_chg   (app_flags & 2)

#define LI_MAX  6
#define NI_LEN  7
#define PEG_STR_LEN 15

#define INFO_DESC	1
#define INFO_BIZ	2
#define INFO_PRIV	4
#define INFO_SPARE	8
#define INFO_ACK	16
#define INFO_IN		(INFO_DESC | INFO_BIZ | INFO_PRIV)
#define LED_R	0
#define LED_G	1
#define LED_B	2
#define LED_N	3
#define LED_OFF	0
#define LED_ON	1
#define LED_BLINK 2

#define NVM_OSET 	(1024L << 8)
#define NVM_SLOT_NUM	16
#define NVM_SLOT_SIZE 	64

typedef enum {
	noTag, newTag, reportedTag, confirmedTag, matchedTag,
	fadingReportedTag, fadingConfirmedTag, fadingMatchedTag,
	goneTag, sumTag
} tagStateType;

typedef word profi_t;

typedef struct nvmDataStruct {
	word	id;
	profi_t	profi;
	char	nick[NI_LEN +1];
	char	desc[PEG_STR_LEN +1];
	char	dpriv[PEG_STR_LEN +1];
	char	dbiz[PEG_STR_LEN +1];
	word	local_inc;
	word	local_exc;
	char	spare[4]; // size: 64
} nvmDataType;

typedef struct tagDataStruct {
	lword   evTime;
	lword   lastTime;
	word	id;
	word	rssi:8;
	word	pl:4;
	word	state:4;
	word	intim:1;
	word	info:7;
	profi_t	profi;
	char	nick[NI_LEN +1];
	char	desc[PEG_STR_LEN +1];
} tagDataType;

typedef struct tagShortStruct {
	word	id;
	char	nick[NI_LEN +1];
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

#endif
