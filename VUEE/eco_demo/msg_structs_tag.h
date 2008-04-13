#ifndef __msg_structs_tag_h__
#define __msg_structs_tag_h__
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "msg_tarp.h"

typedef struct msgSetTagStruct {
	headerType	header;
	word		pow_levels;
	word		freq_maj;
	word		freq_min;
	word		rx_span;
} msgSetTagType;

#define in_setTag(buf, field)   (((msgSetTagType *)(buf))->field)

typedef struct msgStatsTagStruct {
	headerType	header;
	lword		hostid;
	lword		ltime;
	lword		slot;
	word		maj;
	word		min;
	word		span;
	word		pl;
	word		mem;
	word		mmin;
} msgStatsTagType;

#define in_statsTag(buf, field)   (((msgStatsTagType *)(buf))->field)

typedef struct msgRpcStruct {
	headerType      header;
	nid_t           target;
} msgRpcType;

#define in_rpc(buf, field)   (((msgRpcType *)(buf))->field)

typedef struct msgPongStruct {
	headerType      header;
	word		level:4;
	word		flags:4;
	word		alrms:4;
	word		status:4; // this should be in pload, but...
	word		freq;
} msgPongType;

#define in_pong(buf, field)	(((msgPongType *)(buf))->field)
#define PONG_RXON 	1
#define PONG_PLOAD	2
#define PONG_RXPERM	4

#define in_pong_rxon(buf)	(((msgPongType *)(buf))->flags & PONG_RXON)
#define in_pong_pload(buf)	(((msgPongType *)(buf))->flags & PONG_PLOAD)
#define in_pong_rxperm(buf)	(((msgPongType *)(buf))->flags & PONG_RXPERM)

typedef struct pongPloadStruct {
	lword 	ts; 
	lword	eslot;
	word	sval[5];	// NUM_SENS
	word	spare;
} pongPloadType;
#define in_pongPload(buf, field) (((pongPloadType *)(buf + \
				sizeof(msgPongType)))->field)
#define in_ppload(frm, field) (((pongPloadType *)(frm))->field)

typedef struct msgPongAckStruct {
	headerType	header;
	lword		ts;
	lword		reftime;
} msgPongAckType;
#define in_pongAck(buf, field) (((msgPongAckType *)(buf))->field)

#endif
