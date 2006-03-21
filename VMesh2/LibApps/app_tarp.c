/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.			*/
/* All rights reserved.							*/
/* ==================================================================== */
#include "sysio.h"
#include "msg_tarp.h"
#include "msg_gene.h"

nid_t	local_host, master_host;

#if 0
bool msg_isMaster (msg_t m) {
	return m == msg_master;
}

bool msg_isTrace (msg_t m) {
	return m == msg_trace || m == msg_traceAck ? YES : NO;
}

bool msg_isBind (msg_t m) {
	return m == msg_bind;
}
#endif

// header (called from raw buf tarp_rx sees) + msg + # of appended ids
int tr_offset (headerType * mb) {
	if (mb->msg_type == msg_trace)	// fwd dir
		return 2 + sizeof(msgTraceType) + sizeof(nid_t) * (mb->hoc -1);
	return 2 + sizeof(msgTraceAckType) + sizeof(nid_t) *
		(mb->hoc + ((msgTraceAckType *)mb)->fcount -1);
}

