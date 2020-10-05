/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __msg_tagStructs_h
#define __msg_tagStructs_h

#include "msg_tarp.h"

typedef struct msgGetTagStruct {
	headerType	header;
	lword		passwd;
} msgGetTagType;
#define in_getTag(buf, field)   (((msgGetTagType *)(buf))->field)

typedef struct msgGetTagAckStruct {
	headerType      header;
	lword		ltime;
	lword		host_id;
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
	lword		passwd;
	lword		npasswd;
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
	lword           passwd;
} msgRpcType;
#define in_rpc(buf, field)   (((msgRpcType *)(buf))->field)

typedef struct msgPongStruct {
	headerType      header;
	word		level;
	word		flags;
} msgPongType;
#define in_pong(buf, field)   (((msgPongType *)(buf))->field)
#define PONG_RXON 0x0001
#define in_pong_rxon(buf)     (((msgPongType *)(buf))->flags & PONG_RXON)
#endif
