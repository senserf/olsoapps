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

extern appCountType app_count;
extern lword host_id;
//extern nid_t net_id;
extern long master_delta;
extern word pow_level;
extern word beac_freq;
extern byte cyc_ctrl;
extern byte cyc_ap;
extern word cyc_sp;

extern char * cmd_line;
extern cmdCtrlType cmd_ctrl;

extern int beacon (word, address);
extern int cyc_man (word, address);

extern char * get_mem (word state, int len);

extern void msg_master_in (char * buf);
//extern void msg_info_in (char * buf);
extern void msg_trace_in (word state, char * buf);
extern void msg_cmd_in (word state, char * buf);
extern void msg_bind_in (word state, char * buf);
//extern void msg_disp_in (word state, char * buf);

//extern void msg_master_out (word state, char** buf_out, nid_t rcv);
extern void msg_trace_out (word state, char** buf_out, nid_t rcv);
//extern void msg_info_out (word state, char** buf_out, nid_t rcv);
extern word msg_traceAck_out (word state, char *buf, char** out_buf);
extern void msg_bind_out (word state, char** buf_out);

extern void send_msg (char * buf, int size);

extern void oss_ret_out (word state);

extern void oss_trace_in (word state);
extern void oss_traceAck_out (word state, char * buf);
extern void oss_new_out (word state, char * buf);
//extern void oss_rpc_in (word state, char * in_buf);
extern void oss_master_in (word state);
extern void oss_set_in ();
extern void oss_info_in (word state);
extern void oss_bind_in (word state);

//extern void oss_info_in (word state);
//extern void oss_info_out (char * buf, word fmt);
#endif
