#ifndef __app_tarp_if_h
#define	__app_tarp_if_h

/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "msg_tarp.h"
#include "msg_gene.h"

extern nid_t    local_host;
extern nid_t    master_host;
extern nid_t    net_id;
extern word	app_flags;
extern int tr_offset (headerType * mb);
#define msg_isBind(m)	((m) == msg_bind)
#define msg_isTrace(m)	((m) == msg_trace || (m) == msg_traceAck)
#define msg_isMaster(m)	((m) == msg_master)
#define msg_isNew(m)	((m) == msg_new)

// clr in app.h
#define set_master_chg	(app_flags |= 2)
#endif

