#ifndef __app_peg_data_h
#define	__app_peg_data_h

#include "app_peg.h"

extern const lword host_id;

extern word host_pl;
extern lword master_ts;
extern word app_flags;
extern word tag_auditFreq;
extern lint master_date;
extern word pow_sup;

extern tagDataType tagArray [];

extern wroomType msg4tag;
extern aggDataType agg_data;
extern msgPongAckType pong_ack;

extern word sync_freq;
extern word sat_mod;
extern word plot_id;

idiosyncratic void tmpcrap (word);
idiosyncratic void oss_report_out (char*);
idiosyncratic void app_diag (const word, const char*, ...);
idiosyncratic void net_diag (const word level, const char * fmt, ...);
idiosyncratic sint find_tags (word, word);
idiosyncratic char *get_mem (word, sint);
idiosyncratic void init_tag (word);
idiosyncratic void init_tags ();
idiosyncratic void set_tagState (word, tagStateType, Boolean);
idiosyncratic sint insert_tag (word);
idiosyncratic void check_tag (word, word, char**);
idiosyncratic void copy_fwd_msg (word, char**, char*, word);
idiosyncratic void send_msg (char*, sint);
idiosyncratic sint check_msg_size (char*, word, word);
idiosyncratic void write_mark (word);
idiosyncratic void write_agg (word);
idiosyncratic void check_msg4tag (char*);
idiosyncratic void agg_init ();
idiosyncratic void fatal_err (word, word, word, word);
idiosyncratic word handle_a_flags (word);
idiosyncratic sint str_cmpn (const char*, const char*, sint);
idiosyncratic lint wall_date (lint);
idiosyncratic void msg_report_out (word, word, char**, word);
idiosyncratic void msg_findTag_in (word, char*);
idiosyncratic void msg_findTag_out (word, char**, nid_t, nid_t);
idiosyncratic void msg_setPeg_in (char*);
idiosyncratic void msg_fwd_in (word, char*, word);
idiosyncratic void msg_fwd_out (word, char**, word, nid_t, nid_t);
idiosyncratic void msg_master_in (char*);
void msg_master_out (word, char**, nid_t);
idiosyncratic void msg_pong_in (word, char*, word);
idiosyncratic void msg_report_in (word, char*);
idiosyncratic void msg_reportAck_in (char*);
idiosyncratic void msg_reportAck_out (word, char*, char**);

fsm mbeacon;

#endif
