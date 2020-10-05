/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __oss_dcl_h__
#define	__oss_dcl_h__

#include "app.h"
#include "tarp.h"
#include "msg.h"

// to fill in by ossi
fsm ossi_in, ossi_out (char *);

void ossi_stats_out (char * b);
void ossi_trace_out (char * buf, word rssi);
void ossi_odr_out (char * b);
void ossi_disp_out (char * b);

#endif
