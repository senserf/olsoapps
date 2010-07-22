#ifndef	__plug_ack_h_
#define	__plug_ack_h_
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"

//+++ "plug_ack.c"

extern trueconst tcvplug_t plug_ack;
extern word NFreeHooks;

//
// The first two bytes of a packet (folowing the network ID), mean this:
//	- the type (with 0xff standing for ACK)
//	- serial number modulo 256 (to match ACKs to packets)
//
#define	ptype(p)	(((byte*)p) [2])
#define	psernum(p)	(((byte*)p) [3])

#endif
