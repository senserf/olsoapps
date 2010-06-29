#ifndef __msg_rtagStructs_h
#define __msg_rtagStructs_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "msg_tarp.h"


// keep msg fields aligned to avoid holes between the header and
// the rest. ASSUMING 8B Tarp header:
typedef struct msgMasterStruct {
	headerType	header;
	lword		mtime;
	word		cyc_sp;
	byte		cyc_ctrl;
	byte 		cyc_ap;
} msgMasterType;
#define in_master(buf, field)   (((msgMasterType *)(buf))->field)

typedef struct msgCmdStruct {
	headerType      header;
	nid_t		s;
	nid_t		p;
	nid_t		t;
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
	lword		mtime;
	nid_t		msw_hid;
	nid_t		nid;
	nid_t		lh;
	nid_t		mid;
	word		cyc_sp;
	byte		cyc_ctrl;
	byte		cyc_ap;
	byte		range;
	byte		filler;
} msgBindType;
#define in_bind(buf, field)  (((msgBindType *)(buf))->field)

typedef struct msgNewStruct {
	headerType	header;
	lword		hid;
	nid_t		nid;
	nid_t		mid;
} msgNewType;
#define in_new(buf, field)  (((msgNewType *)(buf))->field)

#endif
