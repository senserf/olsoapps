#ifndef __app_h
#define __app_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
//+++ "app_count.c"
//+++ "check_passwd.c"
//+++ "get_mem.c"
//+++ "msg_getTagAck_out.c"
//+++ "msg_getTag_in.c"
//+++ "msg_setTagAck_out.c"
//+++ "msg_setTag_in.c"
//+++ "pong_params.c"
//+++ "send_msg.c"
//+++ "set_tag.c"
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
