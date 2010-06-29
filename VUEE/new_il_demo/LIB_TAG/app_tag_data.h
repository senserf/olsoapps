/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#ifndef __app_tag_data_h__
#define	__app_tag_data_h__

#include "app_tag.h"
#include "tarp.h"

extern const lword     host_id;
extern lword           ref_ts;
extern long            ref_date;
extern long            lh_time;
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
void upd_on_ack (long ds, long rd, word syfr, word ackf, word pi);
word handle_c_flags (word c_fl);
long wall_date_t (long s);

// PiComp
//
// Announcement needed for PicOS only
//
fsm sens, rxsw, pong;

#endif
