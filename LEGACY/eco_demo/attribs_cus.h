/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__attribs_cus_h__
#define	__attribs_cus_h__

__EXTERN __CONST lword _da (host_id);
__EXTERN word _da (host_pl);
__EXTERN lword _da (master_ts);
__EXTERN lint _da (master_date);
__EXTERN wroomType _da (msg4tag);
__EXTERN wroomType _da (msg4ward);
__EXTERN tagDataType _da (tagArray) [tag_lim];
__EXTERN word _da (tag_auditFreq);
__EXTERN word _da (app_flags);
__EXTERN char *_da (ui_ibuf), *_da (ui_obuf), *_da (cmd_line);
__EXTERN aggDataType _da (agg_data);
__EXTERN aggEEDumpType *_da (agg_dump);
__EXTERN msgPongAckType _da (pong_ack);
__EXTERN word _da (sync_freq);
__EXTERN word _da (sat_mod);
__EXTERN word _da (plot_id);

// Methods/functions: need no EXTERN

void	_da (show_ifla) (void);
void	_da (read_ifla) (void);
void	_da (save_ifla) (void);
void	_da (stats) (char * buf);
void	_da (app_diag) (const word, const char *, ...);
void	_da (net_diag) (const word, const char *, ...);

void 	_da (process_incoming) (word state, char * buf, word size, word rssi);
void	_da (check_msg4tag) (char * buf);
sint 	_da (check_msg_size) (char * buf, word size, word repLevel);
void 	_da (check_tag) (word state, word i, char** buf_out);
sint 	_da (find_tags) (word tag, word what);
char * 	_da (get_mem) (word state, sint len);
void 	_da (init_tag) (word i);
void 	_da (init_tags) (void);
sint 	_da (insert_tag) (word tag);
void 	_da (set_tagState) (word i, tagStateType state, Boolean updEvTime);

void 	_da (msg_findTag_in) (word state, char * buf);
void 	_da (msg_findTag_out) (word state, char** buf_out, nid_t tag,
								nid_t peg);
void 	_da (msg_master_in) (char * buf);
sint	_da (msg_satest_out) (char * bin);
sint	_da (msg_rpc_out) (char * bin);
void	_da (sat_in) (void);
void	_da (sat_out) (char * buf);
void 	_da (msg_pong_in) (word state, char * buf, word rssi);
void 	_da (msg_reportAck_in) (char * buf);
void 	_da (msg_reportAck_out) (word state, char * buf, char** buf_out);
void 	_da (msg_report_in) (word state, char * buf);
void 	_da (msg_report_out) (word state, word tIndex, char** buf_out,
		word flags);
void 	_da (msg_fwd_in) (word state, char * buf, word size);
void 	_da (msg_fwd_out) (word state, char** buf_out, word size, nid_t tag,
							nid_t peg);
void 	_da (copy_fwd_msg) (word state, char** buf_out, char * buf, word size);
void    _da (msg_setPeg_in) (char * buf);

void 	_da (oss_findTag_in) (word state, nid_t tag, nid_t peg);
void 	_da (oss_setTag_in) (word state, word tag, nid_t peg,
		word maj, word min, word span, word pl, word c_fl);
void 	_da (oss_setPeg_in) (word state, nid_t peg, word audi, word pl,
	       	word a_fl);
void 	_da (oss_report_out) (char * buf);

void 	_da (send_msg) (char * buf, sint size);

void	_da (agg_init) (void);
void	_da (fatal_err) (word err, word w1, word w2, word w3);
void	_da (write_agg) (word ti);
word	_da (r_a_d) (void);
word	_da (handle_a_flags) (word a_fl);
void	_da (tmpcrap) (word);
sint	_da (str_cmpn) (const char * s1, const char * s2, sint n);
lint	_da (wall_date) (lint s);

// Expected by NET and TARP

int _da (tr_offset) (headerType*);
Boolean _da (msg_isBind) (msg_t m);
Boolean _da (msg_isTrace) (msg_t m);
Boolean _da (msg_isMaster) (msg_t m);
Boolean _da (msg_isNew) (msg_t m);
Boolean _da (msg_isClear) (byte o);
void _da (set_master_chg) (void);

#endif
