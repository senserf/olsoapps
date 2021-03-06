/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __app_tag_data_h__
#define	__app_tag_data_h__

#include "app_tag.h"
#include "tarp.h"

extern lword           ref_ts;
extern lint            ref_date;
extern lint            lh_time;
extern sensDataType    sens_data;
extern pongParamsType  pong_params;
extern word            app_flags;
extern word            plot_id;

void next_col_time (void);
void app_diag_t (const word, const char *, ...);
void net_diag_t (const word, const char *, ...);

void  process_incoming (word state, char * buf, word size);
char * get_mem_t (word state, sint len);

void msg_setTag_in (char * buf);
void msg_pongAck_in (char* buf);

word max_pwr (word p_levs);
void send_msg_t (char * buf, sint size);

void fatal_err_t (word err, word w1, word w2, word w3);
void write_mark_t (word what);
void upd_on_ack (lint ds, lint rd, word syfr, word pi);
word handle_c_flags (word c_fl);
lint wall_date_t (lint s);

// PiComp
//
// Announcement needed for PicOS only
//
fsm sens, rxsw, pong;

#endif
