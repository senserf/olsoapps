#ifndef __msg_col_h__
#define __msg_col_h__
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2011.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "msg_structs_col.h"

typedef enum {
	msg_null, msg_setTag, msg_rpc, msg_pong, msg_pongAck, msg_statsTag,
	msg_master
} msgType;


#endif
