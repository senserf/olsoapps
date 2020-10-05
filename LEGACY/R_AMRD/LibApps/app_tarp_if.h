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
#include "msg_rtag.h"

extern id_t    local_host;
extern id_t    master_host;

extern bool msg_isMaster (msg_t m);
extern bool msg_isProxy (msg_t m);
extern bool msg_isTrace (msg_t m);
extern int tr_offset (headerType * mb);
#endif

