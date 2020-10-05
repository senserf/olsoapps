/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __app_h__
#define __app_h__

#include "sysio.h"
#include "msg.h"

#define DEL_QUANT	(10 << 10)
#define AUD_QUANT	(2* DEL_QUANT)
#define MAS_QUANT	(3* DEL_QUANT)
#define OSS_QUANT	MAX_WORD

// trigger / when ids
#define TRIGGER_BASE_ID	77

#define TRIG_MASTER	(TRIGGER_BASE_ID +0)
#define TRIG_BEAC_PUSH	(TRIGGER_BASE_ID +1)
#define TRIG_BEAC_DOWN	(TRIGGER_BASE_ID +2)
// end of trigger / when ids

typedef struct {
	word	id;
	word	frssi :8;
	word	brssi :8;
} odre_t;

typedef struct {
	msgOdrType	msg;
	odre_t		oe[10];
} odr_t;

typedef struct {
	msgDispType	msg;
	char		str[40];
} disp_t;

typedef struct {
	msgTraceType	msg; // just for consistency
} trac_t;

typedef struct {
	char *	mpt;
	byte	freq;
	word	vol;
	word	cur;
} beac_t;

typedef union {
	word w;
	struct {
		word tparam	:8;
		word polev	:3;
		word rx		:1;
		word stran	:1; // show transient odr (not really in FIM)
		word ofmt	:1; // oss fmt
		word spare	:2;
	} f;
} fim_t;

#endif
