#ifndef __app_tag_h__
#define __app_tag_h__
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#define	THREADNAME(a)	a ## _tag

//+++ "lib_app_tag.c"
//+++ "msg_io_tag.c"
//+++ "app_diag_tag.c"

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
