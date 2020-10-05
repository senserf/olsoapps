/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __msg_vmesh_h
#define __msg_vmesh_h
#include "msg_vmeshStructs.h"
typedef enum {
	msg_null, msg_master, msg_trace, msg_traceAck,
      	msg_cmd, msg_new, msg_bindReq, msg_bind, msg_br,
	msg_alrm, msg_st_obsolete, msg_stAck, msg_stNack_obsolete,
	msg_nh, msg_nhAck,
        msg_traceF, msg_traceFAck, msg_traceB, msg_traceBAck,
	msg_io, msg_ioAck, msg_dat, msg_datAck
} msgType;

#endif
