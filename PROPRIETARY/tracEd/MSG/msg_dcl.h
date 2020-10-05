/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __msg_dcl_h__
#define	__msg_dcl_h__

#include "sysio.h"
#include "oss.h"
#include "tarp.h"
#include "msg.h"

sint msg_trace_out (nid_t t, word dir, word hlim);
sint msg_stats_out (nid_t t);
sint msg_any_out (req_t * r);

fsm rcv, mbeacon, msgbeac;

extern mb_t beac;

//+++ msg.cc tarp_cue.cc

#endif
