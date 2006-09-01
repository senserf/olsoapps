#ifndef __msg_tags_h
#define __msg_tags_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "msg_tagStructs.h"

typedef enum {
	msg_null, msg_getTag, msg_getTagAck, msg_setTag, msg_setTagAck, msg_rpc, msg_pong,
	msg_new, msg_newAck, msg_master
} msgType;


#endif
