/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __msg_rtag_h
#define __msg_rtag_h

#include "msg_rtagStructs.h"

#if 0
typedef enum {
	msg_null, msg_master, msg_rpc, msg_trace, msg_traceAck, msg_info,
	msg_cmd, msg_disp
} msgType;
#endif

typedef enum {
	msg_null, msg_master, msg_trace, msg_traceAck, msg_cmd, msg_bind,
	msg_new,
	msg_traceF, msg_traceFAck, msg_traceB, msg_traceBAck /* not needed? */
} msgType;

#define APP_RTAG	0x0400
#define MSG_ENABLED	0x0400 // RTAG only
#define is_msg_enabled(type)	((type) & MSG_ENABLED)
#define is_app_valid(type)	((type) & APP_RTAG)

#endif
