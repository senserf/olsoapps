/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __app_h
#define __app_h
#include "sysio.h"
#include "msg_tarp.h"
typedef struct appCountStruct {
	word rcv;
	word snd;
	word fwd;
} appCountType;

typedef struct pongParamsStruct {
	word freq_maj;
	word freq_min;
	word pow_levels;
	word rx_span;
	word rx_lev;
} pongParamsType;

#endif
