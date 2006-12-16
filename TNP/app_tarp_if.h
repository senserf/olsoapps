#ifndef __app_tarp_if_h__
#define	__app_tarp_if_h__

/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifndef	__SMURPH__

// For SMURPH, this is accomplished by virtual functions declared in
// attribs_tag.h

#include "sysio.h"
#include "msg_tarp.h"
#include "msg_tag.h"
#include "msg_peg.h"

// This file is only needed by PICOS. FIXME: need a way to include it
// automatically despite name suffix.
  
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

#endif

