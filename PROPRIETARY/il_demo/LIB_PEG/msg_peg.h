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

typedef enum {
	msg_null, msg_setTag, msg_rpc, msg_pong, msg_pongAck, msg_statsTag, 
	msg_master, msg_report, msg_reportAck, msg_fwd, msg_findTag, msg_setPeg,
	msg_statsPeg, msg_trace, msg_traceAck, msg_traceF, msg_traceFAck, 
	msg_traceB, msg_traceBAck
} msgType;


#endif
#endif
