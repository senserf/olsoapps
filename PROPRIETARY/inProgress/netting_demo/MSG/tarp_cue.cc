/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012        			*/
/* All rights reserved.							*/
/* ==================================================================== */

#include "tarp.h"
#include "msg.h"

/*
 * "Virtual" stuff needed by NET & TARP =======================================
 */

idiosyncratic int tr_offset (headerType *h) {
	sint i;
	byte mt;

	mt = tarp_mType (h->msg_type);

	if (mt == msg_trace || mt == msg_traceF || mt == msg_trace1) // fwd dir
		return 2 + sizeof(msgTraceType) + 2 * (h->hoc -1);

	i = 2 + sizeof(msgTraceAckType) + 2 * h->hoc;
	if (mt == msg_traceAck) // bidirectional
		i += 2 * (((msgTraceAckType *)h)->fcount -1);

	return i;
}

idiosyncratic Boolean msg_isBind (msg_t m) {
	return NO;
}

idiosyncratic Boolean msg_isTrace (msg_t m) {
	m &= TARP_MSGTYPE_MASK;
	return (m == msg_trace || m == msg_traceAck || m == msg_traceF ||
			m == msg_traceBAck || m == msg_trace1);
}

idiosyncratic Boolean msg_isMaster (msg_t m) {
	return ((m & TARP_MSGTYPE_MASK) == msg_master);
}

/*
1 - not this msg
2 - handle
(0 - don't pay attention even to dummy acks)
*/
idiosyncratic word  guide_rtr (headerType *  b) {
	return (b->rcv == 0) ? 1 : 2;
}

idiosyncratic Boolean msg_isNew (msg_t m) {
	return NO;
}

idiosyncratic Boolean msg_isClear (byte o) {
	return YES;
}

idiosyncratic void set_master_chg () {
	return; // we don't care  app_flags.f.m_chg = 1;
}

