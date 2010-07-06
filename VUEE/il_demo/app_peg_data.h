/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifndef __SMURPH__
// dupa vuee barfs, must be included twice 
#include "app_peg.h"
#endif

#include "tarp.h"

#ifdef __dcx_def__

#ifndef __app_peg_data_defined__
#define __app_peg_data_defined__

#ifdef __SMURPH__
// in hostid.cc for PicOS
__CONST lword	host_id		__sinit (0xBACADEAD);
#endif

lword		master_ts	__sinit (0);
lword		pow_ts		__sinit (0);
lword		con_ts		__sinit (0); // these timestamps are wasteful
						// ...if we care
lint		master_date	__sinit (0);

lword		aud_lhtime;

wroomType	msg4tag		__sinit (NULL, 0);
wroomType	msg4ward	__sinit (NULL, 0);
tagDataType	tagArray [tag_lim];
aggDataType	agg_data;
msgPongAckType	pong_ack	__sinit ({msg_pongAck, 0}, 0);
aggEEDumpType	* agg_dump;
char 		* ui_ibuf	__sinit (NULL);
char		* ui_obuf	__sinit (NULL);
char		* cmd_line	__sinit (NULL);

char		* aud_buf	__sinit (NULL);

char 		* rcv_buf	__sinit (NULL);

word		host_pl		__sinit (7);
word		tag_auditFreq	__sinit (23);
word 		app_flags	__sinit (DEF_APP_FLAGS);
word		sync_freq	__sinit (0);
word		plot_id		__sinit (0);
word		pow_sup		__sinit (0);

word		aud_ind;

sint		rcv_psize 	__sinit (0);
word		rcv_rssi	__sinit (0);

int _da (tr_offset) (headerType*);
Boolean _da (msg_isBind) (msg_t m);
Boolean _da (msg_isTrace) (msg_t m);
Boolean _da (msg_isMaster) (msg_t m);
Boolean _da (msg_isNew) (msg_t m);
Boolean _da (msg_isClear) (byte o);
void _da (set_master_chg) (void);

#endif
#endif

// ============================================================================

#ifdef __dcx_dcl__

#ifdef __SMURPH__

#define host_id         _daprx (host_id)
#define master_ts       _daprx (master_ts)
#define pow_ts          _daprx (pow_ts)
#define con_ts		_daprx (con_ts)
#define master_date     _daprx (master_date)
#define aud_lhtime	_daprx (aud_lhtime)
#define msg4tag 	_daprx (msg4tag)
#define msg4ward 	_daprx (msg4ward)
#define tagArray 	_daprx (tagArray)
#define agg_data	_daprx (agg_data)
#define agg_dump	_daprx (agg_dump)
#define pong_ack	_daprx (pong_ack)
#define ui_ibuf		_daprx (ui_ibuf)
#define ui_obuf		_daprx (ui_obuf)
#define cmd_line	_daprx (cmd_line)
#define aud_buf		_daprx (aud_buf)
#define rcv_buf		_daprx (rcv_buf)
#define host_pl		_daprx (host_pl)
#define tag_auditFreq 	_daprx (tag_auditFreq)
#define app_flags	_daprx (app_flags)
#define sync_freq       _daprx (sync_freq)
#define plot_id         _daprx (plot_id)
#define pow_sup         _daprx (pow_sup)
#define aud_ind		_daprx (aud_ind)
#define rcv_psize	_daprx (rcv_psize)
#define rcv_rssi	_daprx (rcv_rssi)

#else
// PicOS
//+++ app_diag_peg.c lib_app_peg.c msg_io_peg.c hostid.c

extern const lword     host_id;
extern lword           master_ts;
extern lword           pow_ts;
extern lword	       con_ts;
extern lint            master_date;
extern lword           aud_lhtime;
extern wroomType       msg4tag;
extern wroomType       msg4ward;
extern tagDataType     tagArray [];
extern aggDataType     agg_data;
extern msgPongAckType  pong_ack;
extern aggEEDumpType   * agg_dump;
extern char            * ui_ibuf;
extern char            * ui_obuf;
extern char            * cmd_line;
extern char	       * aud_buf;
extern char	       * rcv_buf;
extern word            host_pl;
extern word            tag_auditFreq;
extern word            app_flags;
extern word            sync_freq;
extern word            plot_id;
extern word            pow_sup;
extern word            aud_ind;
extern sint	       rcv_psize;
extern word	       rcv_rssi;

// str_peg.h
extern const char ee_str[];
extern const char welcome_str[];
extern const char ill_str[];
extern const char not_in_maint_str[];
extern const char only_master_str[];
extern const char stats_str[];
extern const char statsCol_str[];
extern const char ifla_str[];
extern const char bad_str[];
extern const char clock_str[];
extern const char rep_str[];
extern const char repSum_str[];
extern const char repNo_str[];
extern const char dump_str[];
extern const char dumpmark_str[];
extern const char dumpend_str[];
extern const char plot_str[];
extern const char sync_str[];
extern const char impl_date_str[];

// PicOS
#endif

void show_ifla (void);
void read_ifla (void);
void save_ifla (void);
void stats (char * buf);
void app_diag (const word, const char *, ...);
void net_diag (const word, const char *, ...);

void  process_incoming (word state, char * buf, word size, word rssi);
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

void oss_findTag_in (word state, nid_t tag, nid_t peg);
void oss_setTag_in (word state, word tag, nid_t peg,
		word maj, word min, word span, word pl, word c_fl);
void oss_setPeg_in (word state, nid_t peg, word audi, word pl, word a_fl);
void oss_master_in (word state, nid_t peg);
void oss_report_out (char * buf);

void send_msg (char * buf, sint size);

void agg_init (void);
void fatal_err (word err, word w1, word w2, word w3);
void write_agg (word ti);
void write_mark (word what);
word r_a_d (void);
word handle_a_flags (word a_fl);
sint str_cmpn (const char * s1, const char * s2, sint n);
lint wall_date (lint s);

word map_rssi (word r);
//char * stateName (unsigned state);
char * locatName (word id, word rssi);
const char * markName (statu_t s);

#endif

// ============================================================================

#ifdef __dcx_ini__

	host_id		= (lword) preinit ("HID");
	master_ts	= 0;
	pow_ts		= 0;
	con_ts		= 0;
	master_date	= 0;
	msg4tag.buf	= NULL;
	msg4tag.tstamp	= 0;
	msg4ward.buf	= NULL;
	msg4ward.tstamp = 0;
	pong_ack.header.msg_type = msg_pongAck;
	ui_ibuf		= NULL;
	ui_obuf		= NULL;
	cmd_line	= NULL;
	aud_buf		= NULL;
	rcv_buf		= NULL;
	host_pl		= 7;
	tag_auditFreq	= 23;
	app_flags	= DEF_APP_FLAGS;
	sync_freq	= 0;
	plot_id		= 0;
	pow_sup		= 0;

	rcv_psize	= 0;
	rcv_rssi	= 0;

#endif
