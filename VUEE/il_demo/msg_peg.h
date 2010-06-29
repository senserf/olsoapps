#ifndef __msg_peg_h__
#ifndef __msg_tag_h__

#define __msg_peg_h__
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "msg_structs_peg.h"

typedef enum {
	msg_null, msg_setTag, msg_rpc, msg_pong, msg_pongAck, msg_statsTag, 
	msg_master, msg_report, msg_reportAck, msg_fwd, msg_findTag, msg_setPeg,
	msg_statsPeg
} msgType;


#endif
#endif
