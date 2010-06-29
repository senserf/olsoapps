#ifndef __app_tarp_if_h
#define	__app_tarp_if_h

/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "msg_tarp.h"
#include "msg_rtag.h"

extern nid_t    local_host;
extern nid_t    master_host;
extern nid_t    net_id;
extern word     app_flags;
extern int tr_offset (headerType * mb);
#define msg_isBind(m)   (((m) & 0x3F) == msg_bind)
#define msg_isTrace(m)  (((m) & 0x3F) == msg_trace || \
		((m) & 0x3F) == msg_traceAck || \
		((m) & 0x3F) == msg_traceF || \
		((m) & 0x3F) == msg_traceBAck)
#define msg_isMaster(m) (((m) & 0x3F) == msg_master)
#define msg_isNew(m)    (((m) & 0x3F) == msg_new)
#define msg_isClear(m)  (msg_isBind(m) || msg_isTrace(m) || msg_isNew(m) || \
		((m) & 0x3F) == msg_traceB || \
		((m) & 0x3F) == msg_traceFAck)

// clr_ in app.h
#define set_master_chg()  (app_flags |= 2)

#endif

