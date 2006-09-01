#ifndef __app_tarp_if_h
#define	__app_tarp_if_h

/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "msg_tarp.h"
#include "msg_tags.h"

//+++ "app_flags.c" "net_id.c" "local_host.c" "host_id.c" "host_passwd.c" "master_host.c"  
extern nid_t    local_host;
extern nid_t    master_host;
extern nid_t    net_id;
extern word	app_flags;
extern int tr_offset (headerType * mb);
#define msg_isBind(m)	(NO)
#define msg_isTrace(m)	(NO)
#define msg_isMaster(m)	((m) == msg_master)
#define msg_isNew(m)	(NO)

// clr in app.h
#define set_master_chg	(app_flags |= 2)
#endif

