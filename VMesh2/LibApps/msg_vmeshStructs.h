#ifndef __msg_vmeshStructs_h
#define __msg_vmeshStructs_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "msg_tarp.h"


// keep msg fields aligned to avoid holes between the header and
// the rest. ASSUMING 8B Tarp header:
typedef struct msgMasterStruct {
	headerType	header;
	word		con;
	word		cyc;
} msgMasterType;
#define in_master(buf, field)   (((msgMasterType *)(buf))->field)

typedef struct msgCmdStruct {
	headerType      header;
	byte 		opcode;
	byte 		opref;
	byte 		oprc;
	byte 		oplen;
} msgCmdType;
#define in_cmd(buf, field)   (((msgCmdType *)(buf))->field)

typedef struct msgTraceStruct {
	headerType      header;
} msgTraceType;

typedef struct msgTraceAckStruct {
	headerType      header;
	word		fcount;
} msgTraceAckType;
#define in_traceAck(buf, field)  (((msgTraceAckType *)(buf))->field)

typedef struct msgBindStruct {
	headerType      header;
	word		esn_l;
	word		esn_h;
	nid_t		mid;
	nid_t		nid;
	nid_t		lh;
	word		con;
	byte		encr; // mode 1b, key 2b
	byte		spare;
} msgBindType;
#define in_bind(buf, field)  (((msgBindType *)(buf))->field)

typedef struct msgNewStruct {
	headerType	header;
	word		esn_l;
	word		esn_h;
} msgNewType;
#define in_new(buf, field)  (((msgNewType *)(buf))->field)

typedef struct msgBindReqStruct {
	headerType	header;
	word		esn_l;
	word		esn_h;
	nid_t		lh;
} msgBindReqType;
#define in_bindReq(buf, field)  (((msgBindReqType *)(buf))->field)

typedef struct msgBrStruct {
	headerType	header;
	word		con;
	word		esn_no;
	word		s_no;
} msgBrType;
#define in_br(buf, field)  (((msgBrType *)(buf))->field)

typedef struct msgAlrmStruct {
	headerType	header;
	byte		typ;
	byte		rev;
	word		esn_l;
	word		esn_h;
	byte		s;
	byte		rssi;
} msgAlrmType;
#define in_alrm(buf, field)  (((msgAlrmType *)(buf))->field)

typedef struct msgStStruct {
	headerType	header;
	byte		con;
	byte		count; // plenty of free bits here
} msgStType;
#define in_st(buf, field)  (((msgStType *)(buf))->field)

typedef struct msgStAckStruct {
	headerType	header;
	word		esn_l;
	word		esn_h;
} msgStAckType;
#define in_stAck(buf, field)  (((msgStAckType *)(buf))->field)

typedef struct msgStNackStruct {
	headerType	header;
} msgStNackType;
#define in_stNack(buf, field)  (((msgStNackType *)(buf))->field)

typedef struct msgIoStruct {
	headerType	header;
	lword		pload;
} msgIoType;
#define in_io(buf, field)	(((msgIoType *)(buf))->field)

typedef struct msgIoAckStruct {
	headerType	header;
	lword		pload;
} msgIoAckType;
#define in_ioAck(buf, field)	(((msgIoAckType *)(buf))->field)

#endif
