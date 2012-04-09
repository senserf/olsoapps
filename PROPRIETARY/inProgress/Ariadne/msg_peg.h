#ifndef __msg_peg_h
#define __msg_peg_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "app_peg.h"

typedef enum {
	msg_null, msg_master, msg_ping, msg_cmd, msg_sil, msg_stats
} msgType;

typedef struct msgMasterStruct {
	headerType      header;
	sattr_t		attr;
} msgMasterType;

#define in_master(buf, field)   (((msgMasterType *)(buf))->field)

typedef struct msgPingStruct {
	headerType      header;
	sattr_t		in_sattr;
	sattr_t		out_sattr;
	nid_t		id;
	word		pings;
	word		rssi:8;
	word		seqno:8;
} msgPingType;
	
#define in_ping(buf, field)   (((msgPingType *)(buf))->field)

typedef struct msgCmdStruct {
	headerType      header;
	word		cmd:8;
	word		spare:8;
	word		a;
} msgCmdType;

#define in_cmd(buf, field)	(((msgCmdType *)(buf))->field)

typedef struct msgSilStruct {
	headerType      header;
} msgSilType;

#define in_sil(buf, field)	(((msgSilType *)(buf))->field)

typedef struct msgStatsStruct {
	headerType      header;
	lword		hid;
	lword		secs;
	nid_t		lh;
	nid_t		mh;
	tout_t		to;
	sattr_t		satt[3];
	word		fl;
	word		mem;
	word		mmin;
} msgStatsType;

#define in_stats(buf, field)	(((msgStatsType *)(buf))->field)

#endif
