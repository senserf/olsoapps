/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __msg_tags_h
#define __msg_tags_h

#include "msg_tagStructs.h"

typedef enum {
	msg_null, msg_getTag, msg_getTagAck, msg_setTag, msg_setTagAck, msg_rpc, msg_pong,
	msg_new, msg_newAck, msg_master
} msgType;


#endif
