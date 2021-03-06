/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __app_peg_h
#define __app_peg_h

//+++ "lib_app_peg.c" "msg_io_peg.c" trueconst_peg.cc

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

#if ANDROIDEMO
#define PEG_STR_LEN 33
#else
#define PEG_STR_LEN 15
#endif

#define INFO_DESC	1
#define INFO_BIZ	2
#define INFO_PRIV	4
#define INFO_NBUZZ	8
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
// this could be reduced to 64 if PEG_STR is back to 15
#define NVM_SLOT_SIZE 	128

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
#if ANDROIDEMO
	char	spare[10];
#else
	char	spare[64]; // NVM_SLOT_SIZE: 128
#endif
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

typedef struct nbComStruct {
	word	id;
	word	what:1;
	word	spare:1;
	word	dhook:6;
	word	vect:8;
	char	memo[NI_LEN +1];
} nbuComType;

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
