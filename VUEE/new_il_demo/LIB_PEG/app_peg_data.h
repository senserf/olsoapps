/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#ifndef __app_peg_data_h__
#define	__app_peg_data_h__

#include "app_peg.h"
#include "tarp.h"

// PiComp
//
// I have only left those that appear to be needed (are in fact referenced
// across files)
//
extern const lword     host_id;
extern lword           master_ts;
extern lint            master_date;
extern wroomType       msg4tag;
extern wroomType       msg4ward;
extern tagDataType     tagArray [];
extern aggDataType     agg_data;
extern msgPongAckType  pong_ack;
extern aggEEDumpType   * agg_dump;
extern char            * cmd_line;
extern word            host_pl;
extern word            tag_auditFreq;
extern word            app_flags;
extern word            sync_freq;
extern word            plot_id;
extern word            pow_sup;

// PiComp
//
// Also the functions: there is no need to mention them all here. Static
// functions can be kept static.
//
void app_diag (const word, const char *, ...);
void net_diag (const word, const char *, ...);

void check_msg4tag (char * buf);
sint  check_msg_size (char * buf, word size, word repLevel);
void  check_tag (word state, word i, char** buf_out);
sint  find_tags (word tag, word what);
char * get_mem (word state, sint len);
void  init_tag (word i);
void  init_tags (void);
sint insert_tag (word tag);
void set_tagState (word i, tagStateType state, Boolean updEvTime);

void msg_findTag_in (word state, char * buf);
void msg_findTag_out (word state, char** buf_out, nid_t tag, nid_t peg);
void msg_master_in (char * buf);
void msg_master_out (word state, char** buf_out, nid_t peg);
void msg_pong_in (word state, char * buf, word rssi);
void msg_reportAck_in (char * buf);
void msg_reportAck_out (word state, char * buf, char** buf_out);
void msg_report_in (word state, char * buf);
void msg_report_out (word state, word tIndex, char** buf_out, word flags);
void msg_fwd_in (word state, char * buf, word size);
void msg_fwd_out (word state, char** buf_out, word size, nid_t tag, nid_t peg);
void copy_fwd_msg (word state, char** buf_out, char * buf, word size);
void msg_setPeg_in (char * buf);
word msg_trace_out (word t, word dir, word hlim);
void msg_trace_in (char * buf, word rssi);

void oss_findTag_in (word state, nid_t tag, nid_t peg);
void oss_setTag_in (word state, word tag, nid_t peg, word ev,
		word maj, word min, word span, word pl, word c_fl);
void oss_setPeg_in (word state, nid_t peg, word audi, word pl, word a_fl);
void oss_master_in (word state, nid_t peg);
void oss_report_out (char * buf);
void oss_traceAck_out (char * buf, word rssi);

void send_msg (char * buf, sint size);

void agg_init (void);
void fatal_err (word err, word w1, word w2, word w3);
void write_agg (word ti);
void write_mark (word what);
word handle_a_flags (word a_fl);
sint str_cmpn (const char * s1, const char * s2, sint n);
lint wall_date (lint s);

char * locatName (word id, word rssi);

// PiComp
//
// FSM announcements are only needed for PicOS. From the viewpoint of VUEE,
// all FSM's are global (within the set of one praxis).
//
fsm audit, mbeacon;

#endif
