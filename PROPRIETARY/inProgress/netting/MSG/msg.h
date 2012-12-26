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
	word		refh;
	word		refl;
	word		rack  :1;
	word		len   :6;
	word		spare :9;
} msgDispType;

#define in_disp(buf, field)   (((msgDispType *)(buf))->field)

typedef struct msgOdrStruct {
	headerType      header;
	word		refh;
	word		refl;
	word		hok   :4;
	word		hko   :4;
	word		ret   :1; // return path?
	word		rack  :1; // request for ack
	word		spare :6;
} msgOdrType;

#define in_odr(buf, field)   (((msgOdrType *)(buf))->field)

typedef struct msgTraceStruct {
        headerType      header;
	lword		ref;
} msgTraceType;
#define in_trace(buf, field)  (((msgTraceType *)(buf))->field)

typedef struct msgTraceAckStruct {
        headerType      header;
	word		refh;
	word		refl;
        word            fcount;
} msgTraceAckType;
#define in_traceAck(buf, field)  (((msgTraceAckType *)(buf))->field)

#endif
