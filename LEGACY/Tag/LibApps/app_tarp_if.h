/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __app_tarp_if_h
#define	__app_tarp_if_h

#include "sysio.h"
#include "msg_tarp.h"
#include "msg_tags.h"
  
extern nid_t    local_host;
extern nid_t    master_host;
extern nid_t    net_id;
extern word	app_flags;
extern int tr_offset (headerType * mb);
#define msg_isBind(m)	(NO)
#define msg_isTrace(m)	(NO)
#define msg_isMaster(m)	((m) == msg_master)
#define msg_isNew(m)	(NO)
#define	msg_isClear(a)	(YES)
// clr in app.h
#define set_master_chg()	(app_flags |= 2)
#endif

