/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012                           */
/* All rights reserved.                                                 */
/* ==================================================================== */
#ifndef __msg_dcl_h__
#define	__msg_dcl_h__

#include "sysio.h"
#include "oss.h"
#include "tarp.h"

sint msg_trace_out (nid_t t, word dir, word hlim);
sint msg_stats_out (nid_t t);
sint msg_any_out (req_t * r);
void send_msg (char * buf, sint size);

fsm rcv, mbeacon;

//+++ msg.cc tarp_cue.cc

#endif
