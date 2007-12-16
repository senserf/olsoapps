#ifndef __msg_structs_tag_h__
#define __msg_structs_tag_h__
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "msg_tarp.h"

typedef struct msgGetTagStruct {
	headerType	header;
} msgGetTagType;

#define in_getTag(buf, field)   (((msgGetTagType *)(buf))->field)

typedef struct msgGetTagAckStruct {
	headerType      header;
	lword		ltime;
	lword		host_ident;
	nid_t 		tag;
	word		pow_levels;
	word		freq_maj;
	word		freq_min;
	word		rx_span;
	word		ackSeq:8;
	word		spare:8;
} msgGetTagAckType;

#define in_getTagAck(buf, field)   (((msgGetTagAckType *)(buf))->field)

typedef struct msgSetTagStruct {
	headerType	header;
	word		node_addr;
	word		pow_levels;
	word		freq_maj;
	word		freq_min;
	word		rx_span;
} msgSetTagType;

#define in_setTag(buf, field)   (((msgSetTagType *)(buf))->field)

typedef struct msgSetTagAckStruct {
	headerType	header;
	nid_t 		tag;
	word		ack:8;
	word		ackSeq:8;
} msgSetTagAckType;

#define in_setTagAck(buf, field)   (((msgSetTagAckType *)(buf))->field)

typedef struct msgRpcStruct {
	headerType      header;
	lword           target;
} msgRpcType;

#define in_rpc(buf, field)   (((msgRpcType *)(buf))->field)

typedef struct msgPongStruct {
	headerType      header;
	word		level:4;
	word		flags:4;
	word		alrms:4;
	word		status:4; // this should be in pload, but...
	word		spare;
} msgPongType;

#define in_pong(buf, field)	(((msgPongType *)(buf))->field)
#define PONG_RXON 	0x0001
#define PONG_PLOAD	0x0002
#define in_pong_rxon(buf)	(((msgPongType *)(buf))->flags & PONG_RXON)
#define in_pong_pload(buf)	(((msgPongType *)(buf))->flags & PONG_PLOAD)

typedef struct pongPloadStruct {
	long 	ts; 
	lword	eslot;
	word	sval[3];	// NUM_SENS
	word	spare;
} pongPloadType;
#define in_pongPload(buf, field) (((pongPloadType *)(buf + \
				sizeof(msgPongType)))->field)
#define in_ppload(frm, field) (((pongPloadType *)(frm))->field)

typedef struct msgPongAckStruct {
	headerType	header;
	lword		ts;
	long		ref_t;
} msgPongAckType;
#define in_pongAck(buf, field) (((msgPongAckType *)(buf))->field)

#endif
