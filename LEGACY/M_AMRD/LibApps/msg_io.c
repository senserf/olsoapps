/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"
#include "lib_app_if.h"
#include "msg_rtag.h"
#include "tarp.h"

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
	oss_traceAck_out (buf, size);
}

void msg_trace_out (word state, char** buf_out, id_t rcv) {

	if (*buf_out == NULL)
		*buf_out = get_mem (state, sizeof(msgTraceType));
	else
		memset (*buf_out, 0, sizeof(msgTraceType));

	in_header(*buf_out, msg_type) = msg_trace;
	in_header(*buf_out, rcv) = rcv;
}
