#ifndef __oss_h__
#define __oss_h__
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"

// interface-specific stuff
//+++ ossi_ser.cc

typedef struct outStruct {
	word	fl_b  :1;
	word	spare :15;
	char	* buf;
} out_t;

#endif
