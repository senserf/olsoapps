/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "msg_tags.h"
#include "lib_apps.h"

void msg_setTagAck_out (word state, char** out_buf, nid_t rcv, seq_t seq,
			word pass) {

	if (*out_buf == NULL)
		*out_buf = get_mem (state, sizeof(msgSetTagAckType));
	else
		memset (*out_buf, 0, sizeof(msgSetTagAckType));

	in_header(*out_buf, msg_type) = msg_setTagAck;
	in_header(*out_buf, rcv) = rcv;
	in_setTagAck(out_buf, ackSeq) = seq;
	in_setTagAck(out_buf, ack) = (byte)pass;
}
