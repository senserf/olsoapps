#ifndef __msg_pegs_h
#define __msg_pegs_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "msg_tagStructs.h"
#include "msg_pegStructs.h"

typedef enum {
	msg_null, msg_getTag, msg_getTagAck, msg_setTag, msg_setTagAck, msg_rpc, msg_pong,
	msg_new, msg_newAck, msg_master, msg_report, msg_reportAck,
	msg_fwd, msg_alrm, msg_findTag, msg_setPeg
} msgType;


#endif
