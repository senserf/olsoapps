#ifndef __lib_apps_h
#define __lib_apps_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "app.h"

extern appCountType app_count;
extern long master_delta;
extern word master_host;
extern wroomType msg4tag;
extern wroomType msg4ward;
extern tagDataType tagArray [tag_lim];
extern word tag_auditFreq;
extern word tag_eventGran;

extern void check_msg4tag (nid_t tag);
extern int check_msg_size (char * buf, word size, word repLevel);
extern void check_tag (word state, word i, char** buf_out);
extern int find_tag (lword tag);
extern char * get_mem (word state, int len);
extern void init_tag (word i);
extern void init_tags (void);
extern int insert_tag (lword tag);
extern void set_tagState (word i, tagStateType state, bool updEvTime);


extern void msg_findTag_in (word state, char * buf);
extern void msg_findTag_out (word state, char** buf_out, lword tag, nid_t peg);
extern void msg_master_in (char * buf);
extern void msg_master_out (word state, char** buf_out, nid_t peg);
extern void msg_pong_in (word state, char * buf, word rssi);
extern void msg_reportAck_in (char * buf);
extern void msg_reportAck_out (word state, char * buf, char** buf_out);
extern void msg_report_in (word state, char * buf);
extern void msg_getTagAck_in (word state, char * buf, word size);
extern void msg_setTagAck_in (word state, char * buf, word size);
extern void msg_report_out (word state, int tIndex, char** buf_out);
extern void msg_fwd_in (word state, char * buf, word size);
extern void msg_fwd_out (word state, char** buf_out, word size, lword tag, nid_t peg, lword pass);
extern void copy_fwd_msg (word state, char** buf_out, char * buf, word size);

extern void oss_findTag_in (word state, lword tag, nid_t peg);
extern void oss_getTag_in (word state, lword tag, nid_t peg, lword pass);
extern void oss_setTag_in (word state, lword tag, nid_t peg, lword pass, nid_t nid,
			               word	in_maj, word in_min, word in_pl, word in_span, lword npass);

extern void oss_master_in (word state, nid_t peg);
extern void oss_report_out (char * buf, word fmt);
extern void oss_setTag_out (char * buf, word fmt);
extern void oss_getTag_out (char * buf, word fmt);

extern void send_msg (char * buf, int size);

#endif
