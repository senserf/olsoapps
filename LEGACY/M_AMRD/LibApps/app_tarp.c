/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"
#include "msg_tarp.h"
#include "msg_rtag.h"

id_t	local_host = 777;
id_t	master_host = 777;

bool msg_isMaster (msg_t m) {
	return m == msg_master ? YES : NO;
}

bool msg_isProxy (msg_t m) {
	return NO;
}

bool msg_isTrace (msg_t m) {
	return m == msg_trace || m == msg_traceAck ? YES : NO;
}

// header (called from raw buf tarp_rx sees) + msg + # of appended ids
int tr_offset (headerType * mb) {
	if (mb->msg_type == msg_trace)	// fwd dir
		return 2 + sizeof(msgTraceType) + sizeof(id_t) * (mb->hoc -1);
	return 2 + sizeof(msgTraceAckType) + sizeof(id_t) *
		(mb->hoc + ((msgTraceAckType *)mb)->fcount -1);
}

