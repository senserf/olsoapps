/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"
#include "msg_tarp.h"
#include "msg_rtag.h"

nid_t	local_host, master_host;

// header (called from raw buf tarp_rx sees) + msg + # of appended ids
int tr_offset (headerType * mb) {
	int i;
	if (mb->msg_type == msg_trace || mb->msg_type == msg_traceF) // fwd dir
		return 2 + sizeof(msgTraceType) + sizeof(nid_t) * (mb->hoc -1);
	i = 2 + sizeof(msgTraceAckType) + sizeof(nid_t) * mb->hoc;
	if (mb->msg_type == msg_traceAck) // birectional
		i += sizeof(nid_t) * (((msgTraceAckType *)mb)->fcount -1);
	return i;
}

