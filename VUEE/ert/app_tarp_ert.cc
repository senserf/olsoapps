/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009 			*/
/* All rights reserved.							*/
/* ==================================================================== */
#include "vuee_ert.h"

#include "msg_tarp.h"
#include "msg_ert.h"

__PUBLF (NodeErt, Boolean, msg_isBind) (msg_t m) {
	return  (m & 0x3F) == msg_bind;
}

__PUBLF (NodeErt, Boolean, msg_isTrace) (msg_t m) {
	m &= 0x3F;
	return (m == msg_trace || m == msg_traceAck || m == msg_traceF ||
			m == msg_traceBAck);
}

__PUBLF (NodeErt, Boolean, msg_isMaster) (msg_t m) {
	return (m & 0x3F) == msg_master;
}

__PUBLF (NodeErt, Boolean, msg_isNew) (msg_t m) {
	return (m & 0x3F) == msg_new;
}

__PUBLF (NodeErt, Boolean, msg_isClear) (msg_t m) {
	if (msg_isBind (m) || msg_isTrace (m) || msg_isNew (m))
		return YES;
	m &= 0x3F;
	return (m == msg_traceB) || (m == msg_traceFAck);
}

// clr_ in app.h
__PUBLF (NodeErt, void, set_master_chg) () {
	app_flags |= 2;
}

// header (called from raw buf tarp_rx sees) + msg + # of appended ids
__PUBLF (NodeErt, int, tr_offset) (headerType * mb) {
	int i;
	if (mb->msg_type == msg_trace || mb->msg_type == msg_traceF) // fwd dir
		return 2 + sizeof(msgTraceType) + sizeof(nid_t) *
		       ((mb->hoc & 0x7F) -1);
	i = 2 + sizeof(msgTraceAckType) + sizeof(nid_t) * (mb->hoc & 0x7F);
	if (mb->msg_type == msg_traceAck) // birectional
		i += sizeof(nid_t) * (((msgTraceAckType *)mb)->fcount -1);
	return i;
}

