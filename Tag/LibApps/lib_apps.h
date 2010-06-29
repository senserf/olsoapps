#ifndef __lib_apps_h
#define __lib_apps_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "app.h"
extern lword host_id;
extern nid_t local_host;

extern appCountType app_count;
extern pongParamsType pong_params;

extern word check_passwd (lword p1, lword p2);
extern char * get_mem (word state, int len);
extern void set_tag (char * buf);

extern void msg_getTag_in (word state, char * buf);
extern void msg_setTag_in (word state, char * buf);
extern void msg_getTagAck_out (word state, char** buf_out, nid_t rcv, seq_t seq,
                   word pass);
extern void msg_setTagAck_out (word state, char** buf_out, nid_t rcv, seq_t seq,
			       word pass);
extern word max_pwr (word p_levs);
extern void send_msg (char * buf, int size);

/*
extern long master_delta;
extern lword master_host;
extern wroomType msg4tag;
extern wroomType msg4ward;
extern tagDataType tagArray [tag_lim];
extern word tag_auditFreq;
extern word tag_eventGran;

extern void check_msg4tag (lword tag);
extern int check_msg_size (char * buf, word size, word repLevel);
extern void check_tag (word state, word i, char** buf_out);
extern int find_tag (lword tag);
extern char * get_mem (word state, int len);
extern void init_tag (word i);
extern void init_tags (void);
extern int insert_tag (lword tag);
extern void set_tagState (word i, tagStateType state, bool updEvTime);


extern void msg_findTag_in (word state, char * buf);
extern void msg_findTag_out (word state, char** buf_out, lword tag, lword peg);
extern void msg_master_in (char * buf);
extern void msg_master_out (word state, char** buf_out, lword peg);
extern void msg_pong_in (word state, char * buf);
extern void msg_reportAck_in (char * buf);
extern void msg_reportAck_out (word state, char * buf, char** buf_out);
extern void msg_report_in (word state, char * buf);
extern void msg_report_out (word state, int tIndex, char** buf_out);

extern void oss_findTag_in (word state, lword tag, lword peg);
extern void oss_master_in (word state, lword peg);
extern void oss_report_out (char * buf, word fmt);

extern void send_msg (char * buf, int size);
*/

#endif
