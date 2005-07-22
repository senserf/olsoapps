#ifndef __msg_rtag_h
#define __msg_rtag_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "msg_rtagStructs.h"

typedef enum {
	msg_null, msg_master, msg_rpc, msg_trace, msg_traceAck, msg_info
} msgType;

#define APP_RTAG	0x0400
#define MSG_ENABLED	0x0400 // RTAG only
#define is_msg_enabled(type)	((type) & MSG_ENABLED)
#define is_app_valid(type)	((type) & APP_RTAG)

#endif
