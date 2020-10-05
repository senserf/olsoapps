/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __msg_tag_h__
#ifndef	__msg_peg_h__

#define __msg_tag_h__

#include "msg_structs_tag.h"

typedef enum {
	msg_null, msg_setTag, msg_rpc, msg_pong, msg_pongAck, msg_statsTag,
	msg_master
} msgType;


#endif
#endif
