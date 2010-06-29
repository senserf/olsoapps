/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.			*/
/* All rights reserved.							*/
/* ==================================================================== */
#include "sysio.h"
#include "lib_app_if.h"
#include "msg_rtag.h"
#include "tarp.h"

extern long master_delta;

void msg_master_in (char * buf) {
	in_master(buf, mtime) = ntowl(in_master(buf, mtime));
	master_delta = in_master(buf, mtime) - seconds();
	master_host  = in_header(buf, snd); // blindly, for now
}

word msg_traceAck_out (word state, char * buf, char** out_buf) {
	word len = sizeof(msgTraceAckType) +
		in_header(buf, hoc) * sizeof(id_t);
	if (*out_buf == NULL)
		*out_buf = get_mem (state, len);
	else
		memset (*out_buf, 0, len);


	in_header(*out_buf, msg_type) = msg_traceAck;
	in_header(*out_buf, rcv) = in_header(buf, snd);
	in_traceAck(*out_buf, fcount) = in_header(buf, hoc);

	// fwd part
	memcpy (*out_buf + sizeof(msgTraceAckType), buf + sizeof(msgTraceType),
		sizeof(id_t) * (in_header(buf, hoc) -1));

	// this node is counted in hoc, but is not appended yet
	memcpy (*out_buf +len - sizeof(id_t), (char *)&local_host, sizeof(id_t));
	return len;
}

void msg_trace_in (word state, char * buf) {
	char * out_buf = NULL;
	word len;

	len = msg_traceAck_out (state, buf, &out_buf);
	if (out_buf) {
		send_msg (out_buf, len);
		ufree (out_buf);
		out_buf = NULL;
	}
}

