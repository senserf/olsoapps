#ifndef __lib_app_if_h
#define	__lib_app_if_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "app.h"
#include "msg_tarp.h"

#include "app_tarp_if.h"

//+++ "app_tarp.c" "lib_app.c" "msg_io.c" "oss_io.c"

extern id_t net_id;
extern lint master_delta;
extern word beac_freq;

extern char * cmd_line;

extern int beacon (word, address);
extern char * get_mem (word state, int len);

extern void msg_traceAck_in (char * buf, word size);

extern void msg_master_out (word state, char** buf_out, id_t rcv);
extern void msg_trace_out (word state, char** buf_out, id_t rcv);

extern void send_msg (char * buf, int size);

extern void oss_trace_in (word state, id_t rcv);
extern void oss_rpc_in (word state, char * in_buf);
extern void oss_master_in (word state, id_t rcv);

extern void oss_traceAck_out (char * buf, word len);
#endif
