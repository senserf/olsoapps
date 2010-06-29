/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.			*/
/* All rights reserved.							*/
/* ==================================================================== */
#include "sysio.h"
#include "msg_rtagStructs.h"
#include "lib_app_if.h"
#include "form.h"
#include "ser.h"
#include "tarp.h"

#if UART_DRIVER

#define OO_RETRY 00
process (oss_out, char)
	entry (OO_RETRY)
		ser_out (OO_RETRY, data);
		ufree (data);
		finish;
endprocess (0)
#undef  OO_RETRY

#endif

void oss_master_in (word state, id_t rcv) {

	char * out_buf = NULL;

	// shortcut to set master_host
	if (local_host == rcv) {
		master_host = rcv;
		master_delta = 0;
		return;
	}

	msg_master_out (state, &out_buf, rcv);
	send_msg (out_buf, sizeof(msgMasterType));
	if (beac_freq == 0 || running(beacon)) {
		ufree (out_buf);
	} else {
		if (fork (beacon, out_buf) == 0) {
			diag ("Fork info failed");
			ufree (out_buf);
		} // beacon frees out_buf at exit
	}
}

void oss_rpc_in (word state, char * in_buf) {
	id_t dst = 0xFFFF;
	char cmd = 'd';
	word len;
	char * out_buf;
	scan (in_buf, "%u %c", &dst, &cmd);
	if (dst == 0xFFFF) {
		diag ("Invalid dst");
		return;
	}

	// here, it's just 1 char:
	len = sizeof(msgRpcType) + 2;
	out_buf = get_mem (state, len);
	in_header(out_buf, msg_type) = msg_rpc;
	in_header(out_buf, rcv) = dst;
	out_buf[sizeof(msgRpcType)] = cmd;
	out_buf[sizeof(msgRpcType)+1] = '\0';
	send_msg (out_buf, len);
	if (beac_freq == 0 || running(beacon)) {
		ufree (out_buf);
	} else {
		// if this ain't kludge, what is?
		in_header(out_buf, snd) = len;

		if (fork (beacon, out_buf) == 0) {
			diag ("Fork info failed");
			ufree (out_buf);
		} // beacon frees out_buf at exit
	}
}

#define MAX_TRACE 80
void oss_traceAck_out (char * buf, word len) {
#if UART_DRIVER
	id_t h;
	char line[MAX_TRACE];
	char formt[5] = "->%u";
	char one[16];
	char * lbuf = line;
	char * ptr = buf + sizeof(msgTraceAckType);
	int i = in_header(buf, hoc) + in_traceAck(buf, fcount) -1;
	int left = MAX_TRACE;

	if (len < sizeof(msgTraceAckType) + i * sizeof(id_t)) {
		diag ("Inv trace len %u ? %u", len, i);
		return;
	}

	while (i) {
		memcpy (&h, ptr, sizeof(id_t));
		form (one, formt, h);
		if ((left -= strlen(one)) <= 0) {
			diag ("Short track line");
			return;
		}
		memcpy (lbuf, one, strlen (one));
		ptr += sizeof(id_t);
		lbuf += strlen (one);
		if (i == in_traceAck(buf, fcount)) {
			*lbuf++ = '<';
			*lbuf++ = '-';
			strcpy (formt, "%u<-");
		}
		i--;
	}
	*lbuf = '\0';

	lbuf = form (NULL, "Trace (%u) %u (%u):\r\n%s\r\n",
		in_traceAck(buf, fcount), in_header(buf, snd),
			in_header(buf, hoc), line);

	if (fork (oss_out, lbuf) == 0 ) {
		diag ("oss_out failed");
		ufree (lbuf);
	}
#endif
}
#undef MAX_TRACE

void oss_trace_in (word state, id_t dst) {

	char * out_buf = NULL;

	msg_trace_out (state, &out_buf, dst);
	send_msg (out_buf, sizeof(msgTraceType));
	if (beac_freq == 0 || running(beacon)) {
		ufree (out_buf);
	} else {
		if (fork (beacon, out_buf) == 0) {
			diag ("Fork info failed");
			ufree (out_buf);
		} // beacon frees out_buf at exit
	}
}
