/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "msg_pegs.h"
#include "lib_apps.h"

void msg_reportAck_out (word state, char * buf, char** out_buf) {

	if (*out_buf == NULL)
		*out_buf = get_mem (state, sizeof(msgReportAckType));
	else
		memset (*out_buf, 0, sizeof(msgReportAckType));

	in_header(*out_buf, msg_type) = msg_reportAck;
	in_header(*out_buf, rcv) = in_header(buf, snd);
	in_reportAck(*out_buf, tagId) = in_report(buf, tagId);
	in_reportAck(*out_buf, state) = in_report(buf, state);
	in_reportAck(*out_buf, count) = in_report(buf, count);
}
