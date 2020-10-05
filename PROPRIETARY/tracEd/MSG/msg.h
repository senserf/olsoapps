/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __msg_h
#define __msg_h
#include "msg_tarp.h"
#include "app.h"

typedef enum {
        msg_null, msg_any, msg_statsTag, msg_master,
        msg_stats, msg_trace, msg_trace1, msg_traceAck, msg_traceF,
       	msg_traceFAck, msg_traceB, msg_traceBAck
} msgType;

typedef struct msgMasterStruct {
	headerType	header;
} msgMasterType;

typedef struct msgAnyStruct {
	headerType      header;
} msgAnyType;

typedef struct msgStatsStruct {
	headerType      header;
	lword		ltime;
	word		mhost;
	appfl_t		fl;
	word		mem;
	word		mmin;
	word		stack;
	word		batter; // bater->bat conflicts with global
} msgStatsType;

#define in_stats(buf, field)   (((msgStatsType *)(buf))->field)

typedef struct msgTraceStruct {
        headerType      header;
} msgTraceType;

typedef struct msgTraceAckStruct {
        headerType      header;
        word            fcount;
} msgTraceAckType;
#define in_traceAck(buf, field)  (((msgTraceAckType *)(buf))->field)

#endif
