/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012        			*/
/* All rights reserved.							*/
/* ==================================================================== */

#include "tarp.h"
#include "msg.h"

/*
 * "Virtual" stuff needed by NET & TARP =======================================
 */

// Here we have the actual definitions of TARP's "idiosyncratic" functions.
// Nothing tricky. The keyword is ignored by PicOS,
// while in VUEE it makes the functions node methods. In this case, we need
// that because they are supposed to override virtual methods.
// (The other case is of name clashes between multiple app_*.cc)
idiosyncratic int tr_offset (headerType *h) {
	sint i;
	if (h->msg_type == msg_trace || h->msg_type == msg_traceF ||
			h->msg_type == msg_trace1) // fwd dir
		return 2 + sizeof(msgTraceType) + 2 * (h->hoc -1);

	i = 2 + sizeof(msgTraceAckType) + 2 * h->hoc;
	if (h->msg_type == msg_traceAck) // birectional
		i += 2 * (((msgTraceAckType *)h)->fcount -1);

	return i;
}

idiosyncratic Boolean msg_isBind (msg_t m) {
	return NO;
}

idiosyncratic Boolean msg_isTrace (msg_t m) {
	m &= 0x3F;
	return (m == msg_trace || m == msg_traceAck || m == msg_traceF ||
			m == msg_traceBAck || m == msg_trace1);
}

idiosyncratic Boolean msg_isMaster (msg_t m) {
	return (m == msg_master);
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

