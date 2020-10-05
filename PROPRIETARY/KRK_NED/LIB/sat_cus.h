/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __sat_cus_h
#define __sat_cus_h

// for now
#define IS_DEFSATGAT	((host_id & 0xFFFF0000) == 0x5A7E0000)
#define IS_SATGAT	(sat_mod != SATMOD_NO)
#define SATWRAP		"AT+CMGS=TextMsg\r5,0,9,"
#define SATWRAPLEN	22
#define MAX_SATLEN	47
// MAX_SATLEN guard should be derived from max packet length
// FIXME check if net.c:radio_len() is wasteful
//

#define SATMOD_NO       0
#define SATMOD_YES      1

#endif
