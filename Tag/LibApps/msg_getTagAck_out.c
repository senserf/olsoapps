/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "msg_tags.h"
#include "lib_apps.h"

extern lword host_id;

void msg_getTagAck_out (word state, char** out_buf, nid_t rcv, seq_t seq, word pass) {

	if (*out_buf == NULL)
		*out_buf = get_mem (state, sizeof(msgGetTagAckType));
	else
		memset (*out_buf, 0, sizeof(msgGetTagAckType));

	in_header(*out_buf, msg_type) = msg_getTagAck;
	in_header(*out_buf, rcv) = rcv;
	in_getTagAck(out_buf, ackSeq) = seq;
	in_getTagAck(out_buf, ltime) = seconds();
	in_getTagAck(out_buf, host_id) = host_id;
	if(pass) {
		in_getTagAck(out_buf, pow_levels) = pong_params.pow_levels;
		in_getTagAck(out_buf, freq_maj) = pong_params.freq_maj;
		in_getTagAck(out_buf, freq_min) = pong_params.freq_min;
		in_getTagAck(out_buf, rx_span) = pong_params.rx_span;
		in_getTagAck(out_buf, spare) = 0xff;
	}
}
