#ifndef __msg_h
#define __msg_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012                           */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "msg_tarp.h"

typedef enum {
        msg_null, msg_disp, msg_odr, msg_master,
        msg_trace, msg_trace1, msg_traceAck, msg_traceF,
       	msg_traceFAck, msg_traceB, msg_traceBAck
} msgType;

typedef struct msgMasterStruct {
	headerType	header;
} msgMasterType;

typedef struct msgDispStruct {
	headerType      header;
	word		ref :7;
	word		ret :1;
	word		len :6;
	word		spare :2;
} msgDispType;

#define in_disp(buf, field)   (((msgDispType *)(buf))->field)

typedef struct msgOdrStruct {
	headerType      header;
	word		ref :7;
	word		ret :1;
	word		hok :4;
	word		hko :4;
} msgOdrType;

#define in_odr(buf, field)   (((msgOdrType *)(buf))->field)

typedef struct msgTraceStruct {
        headerType      header;
} msgTraceType;

typedef struct msgTraceAckStruct {
        headerType      header;
        word            fcount;
} msgTraceAckType;
#define in_traceAck(buf, field)  (((msgTraceAckType *)(buf))->field)

#endif
