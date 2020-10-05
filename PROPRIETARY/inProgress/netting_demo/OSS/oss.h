/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __oss_h__
#define __oss_h__

#include "sysio.h"

// interface-specific stuff
//+++ ossi_ser.cc

typedef struct outStruct {
	word	fl_b  :1;
	word	spare :15;
	char	* buf;
} out_t;

#endif
