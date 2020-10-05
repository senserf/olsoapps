/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__plug_ack_h_
#define	__plug_ack_h_

#include "sysio.h"

//+++ "plug_ack.c"

extern trueconst tcvplug_t plug_ack;

word n_free_hooks ();

//
// The first two bytes of a packet (folowing the network ID), mean this:
//	- the type (with 0xff standing for ACK)
//	- serial number modulo 256 (to match ACKs to packets)
//
#define	ptype(p)	(((byte*)p) [2])
#define	psernum(p)	(((byte*)p) [3])

#endif
