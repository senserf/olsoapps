/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2011                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#ifndef __app_col_data_h__
#define	__app_col_data_h__

#include "app_col.h"
#include "tarp.h"

extern const lword     host_id;
extern lword           ref_ts;
extern lint            ref_date;
extern lint            lh_time;
extern sensDatumType   sens_data;
extern pongParamsType  pong_params;
extern word            app_flags;
extern word            plot_id;
extern word	       buttons;

void next_col_time (void);
void app_diag_t (const word, const char *, ...);
void net_diag_t (const word, const char *, ...);

void  process_incoming (word state, char * buf, word size, word rssi);
char * get_mem_t (word state, sint len);

void msg_setTag_in (char * buf, word rssi);
void msg_pongAck_in (char* buf, word rssi);

word max_pwr (word p_levs);
void send_msg_t (char * buf, sint size);

void fatal_err_t (word err, word w1, word w2, word w3);
void upd_on_ack (lint ds, lint rd, word syfr, word pi,
		word fr, word rssi);
word handle_c_flags (word c_fl);
lint wall_date_t (lint s);
void do_butt (word b);

// PiComp
//
// Announcement needed for PicOS only
//
fsm sens, rxsw, pong;

#endif
