#ifndef __msg_structs_tag_h__
#define __msg_structs_tag_h__
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "msg_tarp.h"

typedef struct msgSetTagStruct {
	headerType	header;
	word		pow_levels;
	word		freq_maj;
	word		freq_min;
	word		rx_span;
	word		c_fl;
	word		plotid;
	lint		ds;
	lint		refdate;
	word		syfreq;
	word		ackflags;
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
	word		c_fl;
	word		spare;
} msgStatsTagType;

#define in_statsTag(buf, field)   (((msgStatsTagType *)(buf))->field)

typedef struct msgPongStruct {
	headerType      header;
	word		level:4;
	word		flags:4;
	word		alrms:4;
	word		pstatus:4; // this should be in pload, but...
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
	lint 	ds; 
	lword	eslot;
#ifdef BOARD_WARSAW_10SHT
	word	sval[11]; 	// 21 exceeds the packet length
#else
	word	sval[6];	// NUM_SENS
#endif
} pongPloadType;
#define in_pongPload(buf, field) (((pongPloadType *)(buf + \
				sizeof(msgPongType)))->field)
#define in_ppload(frm, field) (((pongPloadType *)(frm))->field)

typedef struct msgPongAckStruct {
	headerType	header;
	lint		ds;
	lint		refdate;
	word		syfreq;
	word		ackflags;
	word		plotid;
} msgPongAckType;
#define in_pongAck(buf, field) (((msgPongAckType *)(buf))->field)

#endif
