/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.			*/
/* All rights reserved.							*/
/* ==================================================================== */
#include "sysio.h"
#include "lib_app_if.h"
#include "msg_rtag.h"
#include "diag.h"
#include "tarp.h"

/* this kills the compiler... looks like sheer # of ((( is the 'offender'
void msg_info_in (char * buf) {
	in_info(buf, ltime) = ntowl(in_info(buf, ltime));
	in_info(buf, host_id) = ntowl(in_info(buf, host_id));
	in_info(buf, m_host) = ntowl(in_info(buf, m_host));
	in_info(buf, m_delta) = ntowl(in_info(buf, m_delta));
	oss_info_out (buf, oss_fmt);
}
*/
void msg_info_in (char * buf) {
	lword *i; 
	i = &in_info(buf, ltime);
	*i = *i<<16 | *i>>16;
	i = &in_info(buf, host_id);
	*i = *i<<16 | *i>>16;
	i = &in_info(buf, m_host);
	*i = *i<<16 | *i>>16;
	i = &in_info(buf, m_delta);
	*i = *i<<16 | *i>>16;
	oss_info_out (buf, oss_fmt);
}

void msg_info_out (word state, char** buf_out, id_t rcv) {

	if (*buf_out == NULL)
		*buf_out = get_mem (state, sizeof(msgInfoType));
	else
		memset (*buf_out, 0, sizeof(msgInfoType));

	in_header(*buf_out, msg_type) = msg_info;
	in_header(*buf_out, rcv) = rcv;
	in_info(*buf_out, ltime) = wtonl(seconds());
	in_info(*buf_out, host_id) = wtonl(host_id);
	in_info(*buf_out, m_host) = wtonl(master_host);
	in_info(*buf_out, m_delta) = wtonl(master_delta);
	in_info(*buf_out, pl) = pow_level;
}

void msg_master_in (char * buf) {
	in_master(buf, mtime) = ntowl(in_master(buf, mtime));
	master_delta = in_master(buf, mtime) - seconds();
	master_host  = in_header(buf, snd); // blindly, for now
	app_diag (D_INFO, "Set master to %lu at %ld", master_host,
			master_delta);
}

void msg_master_out (word state, char** buf_out, id_t rcv) {

	if (*buf_out == NULL)
		*buf_out = get_mem (state, sizeof(msgMasterType));
	else
		memset (*buf_out, 0, sizeof(msgMasterType));

	in_header(*buf_out, msg_type) = msg_master;
	in_header(*buf_out, rcv) = rcv;
	in_master(*buf_out, mtime) = wtonl(seconds());
}

void msg_traceAck_in (char * buf, word size) {
	oss_traceAck_out (buf, size, oss_fmt);
}

word msg_traceAck_out (word state, char * buf, char** out_buf) {
	id_t llh = wtonl(local_host);
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

	// note that this node is counted in hoc, but is not appended yet
	memcpy (*out_buf +len - sizeof(id_t), (char *)&llh, sizeof(id_t));
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

void msg_trace_out (word state, char** buf_out, id_t rcv) {

	if (*buf_out == NULL)
		*buf_out = get_mem (state, sizeof(msgTraceType));
	else
		memset (*buf_out, 0, sizeof(msgTraceType));

	in_header(*buf_out, msg_type) = msg_trace;
	in_header(*buf_out, rcv) = rcv;
}
