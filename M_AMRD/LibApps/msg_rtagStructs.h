#ifndef __msg_rtagStructs_h
#define __msg_rtagStructs_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "msg_tarp.h"

typedef struct msgMasterStruct {
	headerType	header;
	lword		mtime;
} msgMasterType;
#define in_master(buf, field)   (((msgMasterType *)buf)->field)

typedef struct msgRpcStruct {
	headerType      header;
} msgRpcType;

typedef struct msgTraceStruct {
	headerType      header;
} msgTraceType;

typedef struct msgTraceAckStruct {
	headerType      header;
	word		fcount;
} msgTraceAckType;
#define in_traceAck(buf, field)  (((msgTraceAckType *)buf)->field)

#endif
