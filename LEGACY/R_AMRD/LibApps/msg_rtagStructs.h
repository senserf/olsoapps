/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __msg_rtagStructs_h
#define __msg_rtagStructs_h

#include "msg_tarp.h"

typedef struct msgMasterStruct {
	headerType	header;
	lword		mtime;
} msgMasterType;
#define in_master(buf, field)   (((msgMasterType *)buf)->field)

typedef struct msgRpcStruct {
	headerType      header;
} msgRpcType;
#define in_rpc(buf, field)   (((msgRpcType *)buf)->field)

typedef struct msgTraceStruct {
	headerType      header;
} msgTraceType;

typedef struct msgTraceAckStruct {
	headerType      header;
	word		fcount;
	//word 		spare;
} msgTraceAckType;
#define in_traceAck(buf, field)  (((msgTraceAckType *)buf)->field)

#endif
