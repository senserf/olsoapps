#ifndef __msg_rtagStructs_h
#define __msg_rtagStructs_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "msg_tarp.h"

typedef struct msgInfoStruct {
	headerType      header;
	lword		ltime;
	id_t		host_id;
	id_t		m_host;
	long		m_delta;
	word		pl;
	word		spare;
} msgInfoType;
#define in_info(buf, field)   (((msgInfoType *)buf)->field)

typedef struct msgMasterStruct {
	headerType	header;
	lword		mtime;
} msgMasterType;
#define in_master(buf, field)   (((msgMasterType *)buf)->field)

typedef struct msgRpcStruct {
	headerType      header;
	lword           passwd;
} msgRpcType;
#define in_rpc(buf, field)   (((msgRpcType *)buf)->field)

typedef struct msgTraceStruct {
	headerType      header;
} msgTraceType;

typedef struct msgTraceAckStruct {
	headerType      header;
	word		fcount;
	word 		spare;
} msgTraceAckType;
#define in_traceAck(buf, field)  (((msgTraceAckType *)buf)->field)

#endif
