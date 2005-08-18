#ifndef __lib_app_if_h
#define	__lib_app_if_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "msg_tarp.h"
#include "app_tarp_if.h"
#include "net.h"

//+++ "app_tarp.c" "msg_io.c" "oss_io.c"

extern void msg_master_in (char * buf);
extern void msg_trace_in (word state, char * buf);

extern word msg_traceAck_out (word state, char *buf, char** out_buf);

static inline void send_msg (char * buf, int size) {
	if (in_header(buf, rcv) == local_host)
		return;
	net_tx (NONE, buf, size);
}

static inline char * get_mem (word state, int len) {
	char * buf = (char *)umalloc (len);
	if (buf == NULL)
		umwait (state);
	return buf;
}

extern void oss_rpc_in (word state, char * in_buf);
#endif
