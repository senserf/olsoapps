/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __app_h
#define __app_h
#include "sysio.h"
#include "msg_tarp.h"

typedef enum {
	noTag, newTag, reportedTag, confirmedTag,
	fadingReportedTag, fadingConfirmedTag, goneTag, sumTag
} tagStateType;

typedef struct tagDataStruct {
	lword	id;
	word	state;
	word	count;
	lword	evTime;
	lword	lastTime;
} tagDataType;

typedef struct appCountStruct {
	word rcv;
	word snd;
	word fwd;
} appCountType;

// waiting room
typedef struct wroomStruct {
	char * buf;
	lword  tstamp;
} wroomType;

extern word       app_flags;
#define clr_master_chg	(app_flags &= ~2)
#define is_master_chg	(app_flags & 2)

#define tag_lim	16

#define OSS_HT 	0
#define OSS_TCL	1
#define oss_fmt	OSS_HT

#endif
