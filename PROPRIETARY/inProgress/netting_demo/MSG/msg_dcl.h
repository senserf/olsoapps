/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __msg_dcl_h__
#define	__msg_dcl_h__

#include "sysio.h"

void send_msg (char * buf, sint size);

fsm rcv, mbeacon, msgbeac;

//+++ msg.cc tarp_cue.cc

#endif
