/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __lib_app_if_h
#define	__lib_app_if_h
#include "app.h"
#include "msg_tarp.h"

//+++ "app_tarp.c" "lib_app.c" "msg_io.c" "oss_io.c" "app_ser.c"

extern const lword ESN;
extern lword cyc_sp;
extern lword cyc_left;
extern lword io_creg;
extern lword io_pload;
extern nid_t net_id;
extern word app_flags;
extern word l_rssi;
extern word freqs;
extern byte * dat_ptr;
extern byte dat_seq;

// in app.h:
//#define beac_freq	(freqs & 0x00FF)
//#define audit_freq	(freqs >> 8)

extern word connect;
// in app.h:
//#define con_warn	(connect >> 12)
//#define con_bad	((connect >> 8) &0x0F)
//#define con_miss	(connect & 0x00FF)

extern byte * cmd_line;
extern cmdCtrlType cmd_ctrl;
extern brCtrlType br_ctrl;
extern cycCtrlType cyc_ctrl;
extern int shared_left;

extern int beacon (word, address);
extern int cyc_man (word, address);
extern int con_man (word, address);
extern int st_rep (word, address);
extern int io_rep (word, address);
extern int io_back (word, address);
extern int dat_rep (word, address);

// they're in app.c
extern int cmd_in (word, address);
extern int dat_in (word, address);

extern char * get_mem (word state, int len);

extern int app_ser_out (word st, char * m, bool cpy);

extern void msg_cmd_in (word state, char * buf);
extern void msg_master_in (char * buf);
extern void msg_trace_in (word state, char * buf);
extern void msg_traceAck_in (word state, char * buf);
extern void msg_bind_in (char * buf);
extern void msg_bindReq_in (char * buf);
extern void msg_new_in (char * buf);
extern void msg_alrm_in (char * buf);
extern void msg_br_in (char * buf);
extern void msg_stAck_in (char * buf);
extern void msg_io_in (char * buf);
extern void msg_ioAck_in (char * buf);
extern void msg_dat_in (char * buf); 
extern void msg_datAck_in (char * buf); 
extern void msg_nh_in (char * buf, word rssi);
extern void msg_nhAck_in (char * buf);

extern void msg_cmd_out (word state, char** buf_out);
extern void msg_master_out (word state, char** buf_out);
extern void msg_trace_out (word state, char** buf_out);
extern word msg_traceAck_out (word state, char *buf, char** out_buf);
extern void msg_bind_out (word state, char** buf_out);
extern bool msg_bindReq_out (char * buf, char** buf_out);
extern bool msg_new_out ();
extern bool msg_alrm_out (char * buf);
extern bool msg_br_out();
extern bool msg_stAck_out (char * buf);
extern bool msg_io_out ();
extern bool msg_ioAck_out (char * buf);
extern word msg_dat_out ();
extern bool msg_datAck_out (char * buf);
extern bool msg_nh_out ();
extern bool msg_nhAck_out (char * buf, char** buf_out, word rssi);

extern void send_msg (char * buf, int size);

extern void oss_ret_out (word state);

extern void oss_trace_in (word state);
extern void oss_traceAck_out (word state, char * buf);
extern void oss_bindReq_out (char * buf);
extern void oss_alrm_out (char * buf);
extern void oss_br_out (char * buf, bool acked);
extern void oss_io_out (char * buf, bool acked);
extern void oss_dat_out (char * buf, bool acked);
extern void oss_datack_out (char * buf);
extern void oss_nhAck_out (char * buf, word rssi);

extern void oss_master_in (word state);
extern void oss_set_in ();
extern void oss_get_in (word state);
extern void oss_bind_in (word state);
extern void oss_sack_in ();
extern void oss_io_in ();
extern void oss_ioack_in ();
extern void oss_dat_in ();
extern void oss_datack_in ();
extern void oss_reset_in ();
extern void oss_locale_in ();
extern void app_leds (const word act);
#endif
