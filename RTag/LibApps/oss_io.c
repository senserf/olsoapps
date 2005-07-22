/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.			*/
/* All rights reserved.							*/
/* ==================================================================== */
#include "sysio.h"
#include "msg_rtagStructs.h"
#include "lib_app_if.h"
#include "diag.h"
#include "form.h"
#include "diag.h"
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

void oss_info_in (word state, id_t rcv) {

	char * out_buf = NULL;
	msg_info_out (state, &out_buf, rcv);

	if (local_host == rcv) {
		in_header(out_buf, snd) = local_host;
		in_info(out_buf, ltime) = seconds();
		in_info(out_buf, host_id) = host_id;
		in_info(out_buf, m_host) = master_host;
		in_info(out_buf, m_delta) = master_delta;
		oss_info_out (out_buf, oss_fmt);
		ufree (out_buf);
		return;
	}
	send_msg (out_buf, sizeof(msgInfoType));
	if (beac_freq == 0 || running(beacon)) {
		ufree (out_buf);
	} else {
		if (fork (beacon, out_buf) == 0) {
			app_diag (D_SERIOUS, "Fork info failed");
			ufree (out_buf);
		} // beacon frees out_buf at exit
	}
}

void oss_info_out (char * buf, word fmt) {
#if UART_DRIVER
	char * lbuf;

	switch (fmt) {
		case OSS_HT:
			lbuf = form (NULL, "Info from (%lx:%lu) at %lu:\r\n"
				"m %lu td %ld, pl:rssi %u:%u\r\n",
				in_info(buf, host_id),
				in_header(buf, snd),
				in_info(buf, ltime),
				in_info(buf, m_host),
				in_info(buf, m_delta),
				in_info(buf, pl),
				in_header(buf, msg_type)); // hacked in rssi
			break;

		case OSS_TCL:
			// not yet
		default:
			app_diag (D_SERIOUS, "**Unsupported fmt %u", fmt);
			return;

	}
	if (fork (oss_out, lbuf) == 0 ) {
		app_diag (D_SERIOUS, "oss_out failed");
		ufree (lbuf);
	}
#endif
}

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
			app_diag (D_SERIOUS, "Fork info failed");
			ufree (out_buf);
		} // beacon frees out_buf at exit
	}
}

void oss_rpc_in (word state, char * in_buf) {
	id_t dst = 0xFFFFFFFF;
	word len;
	char * out_buf;
	char * sb = in_buf;
	scan (in_buf, "%lu", &dst);
	if (dst == 0xFFFFFFFF) {
		app_diag (D_SERIOUS, "Invalid dst");
		return;
	}
	while (isspace (*sb)) sb++;
	while (isdigit (*sb)) sb++;
	while (isspace (*sb)) sb++;
	len = sizeof(msgRpcType) + strlen (sb) +1;
	out_buf = get_mem (state, len);
	in_header(out_buf, msg_type) = msg_rpc;
	in_header(out_buf, rcv) = dst;
	in_rpc(out_buf, passwd) = 0xBACA;
	strcpy (out_buf+sizeof(msgRpcType), sb);
	send_msg (out_buf, len);
	if (beac_freq == 0 || running(beacon)) {
		ufree (out_buf);
	} else {
		// if this ain't kludge, what is?
		in_header(out_buf, snd) = len;

		if (fork (beacon, out_buf) == 0) {
			app_diag (D_SERIOUS, "Fork info failed");
			ufree (out_buf);
		} // beacon frees out_buf at exit
	}
}

#define MAX_TRACE 80
void oss_traceAck_out (char * buf, word len, word fmt) {
#if UART_DRIVER
	id_t h;
	char line[MAX_TRACE];
	char formt[6] = "->%lu";
	char one[16];
	char * lbuf = line;
	char * ptr = buf + sizeof(msgTraceAckType);
	int i = in_header(buf, hoc) + in_traceAck(buf, fcount) -1;
	int left = MAX_TRACE;
	if (fmt != OSS_HT) {
		app_diag (D_SERIOUS, "Unsuported fmt %u", fmt);
		return;
	}
	if (len <= sizeof(msgTraceAckType) + i * sizeof(id_t)) {
		app_diag (D_SERIOUS, "Inv trace len %u ? %u", len, i);
		return;
	}


	while (i) {
		memcpy (&h, ptr, sizeof(id_t));
		h = ntowl(h);
		form (one, formt, h);
		if ((left -= strlen(one)) <= 0) {
			app_diag (D_SERIOUS, "Short track line");
			return;
		}
		memcpy (lbuf, one, strlen (one));
		ptr += sizeof(id_t);
		lbuf += strlen (one);
		if (i == in_traceAck(buf, fcount)) {
			*lbuf++ = '<';
			*lbuf++ = '-';
			strcpy (formt, "%lu<-");
		}
		i--;
	}
	*lbuf = '\0';

	lbuf = form (NULL, "Trace (%u) %lu (%u):\r\n%s\r\n",
		in_traceAck(buf, fcount), in_header(buf, snd),
			in_header(buf, hoc), line);

	if (fork (oss_out, lbuf) == 0 ) {
		app_diag (D_SERIOUS, "oss_out failed");
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
			app_diag (D_SERIOUS, "Fork info failed");
			ufree (out_buf);
		} // beacon frees out_buf at exit
	}
}
