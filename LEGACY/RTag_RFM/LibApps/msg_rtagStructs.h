/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __msg_rtagStructs_h
#define __msg_rtagStructs_h

#include "msg_tarp.h"
#if 0
typedef struct msgInfoStruct {
	headerType      header;
	lword		ltime;
	lword		host_id;
	lint		m_delta;
	id_t		m_host;
	word		pl;
} msgInfoType;
#define in_info(buf, field)   (((msgInfoType *)buf)->field)
#endif
typedef struct msgMasterStruct {
	headerType	header;
	lword		mtime;
} msgMasterType;
#define in_master(buf, field)   (((msgMasterType *)buf)->field)
#if 0
typedef struct msgDispStruct {
	headerType      header;
	unsigned char	d_type;
	unsigned char	len;
} msgDispType;
#define in_disp(buf, field)	(((msgDispType *)buf)->field)

typedef struct msgRpcStruct {
	headerType      header;
	lword           passwd;
} msgRpcType;
#define in_rpc(buf, field)   (((msgRpcType *)buf)->field)
#endif
typedef struct msgCmdStruct {
	headerType      header;
	id_t		s;
	id_t		p;
	id_t		t;
	unsigned char	opcode;
	unsigned char	opref;
	unsigned char	oprc;
	unsigned char	oplen;
} msgCmdType;
#define in_cmd(buf, field)   (((msgCmdType *)buf)->field)

typedef struct msgTraceStruct {
	headerType      header;
} msgTraceType;

typedef struct msgTraceAckStruct {
	headerType      header;
	word		fcount;
} msgTraceAckType;
#define in_traceAck(buf, field)  (((msgTraceAckType *)buf)->field)

#endif
