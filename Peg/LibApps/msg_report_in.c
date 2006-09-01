/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "lib_apps.h"
#include "msg_pegStructs.h"

extern nid_t local_host;

void msg_report_in (word state, char * buf) {
	char * out_buf = NULL;

	msg_reportAck_out (state, buf, &out_buf);
	if (out_buf) {
		send_msg (out_buf, sizeof(msgReportAckType));
		ufree (out_buf);
		out_buf = NULL;
	}
	oss_report_out (buf, oss_fmt);

	// master stacking came free
	if (master_host != local_host) {
		in_header(buf, rcv) = master_host;
		send_msg (buf, sizeof(msgReportType));
		in_header(buf, rcv) = local_host; // restore just in case
	}
}
