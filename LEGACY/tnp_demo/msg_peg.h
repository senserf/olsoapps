/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __msg_peg_h__
#ifndef __msg_tag_h__

#define __msg_peg_h__

#include "msg_structs_peg.h"
#include "msg_structs_tag.h"

typedef enum {
	msg_null, msg_getTag, msg_getTagAck, msg_setTag, msg_setTagAck,
	msg_rpc, msg_pong, msg_new, msg_newAck, msg_master, msg_report,
	msg_reportAck, msg_fwd, msg_alrm, msg_findTag, msg_setPeg
} msgType;


#endif
#endif
