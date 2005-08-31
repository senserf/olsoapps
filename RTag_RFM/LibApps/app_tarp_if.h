#ifndef __app_tarp_if_h
#define	__app_tarp_if_h

/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "msg_tarp.h"
#include "msg_rtag.h"

extern id_t    local_host;
extern id_t    master_host;

extern bool msg_isMaster (msg_t m);
extern bool msg_isProxy (msg_t m);
extern bool msg_isTrace (msg_t m);
extern int tr_offset (headerType * mb);
#endif

