#ifndef	__attribs_peg_h__
#define	__attribs_peg_h__

#ifdef	__SMURPH__

	char	*_da (ui_ibuf), *_da (ui_obuf), *_da (cmd_line);

#endif

__EXTERN __CONST lword _da (host_id);
__EXTERN lword _da (host_password);
__EXTERN word _da (host_pl);
__EXTERN appCountType _da (app_count);
__EXTERN lint _da (master_delta);
__EXTERN wroomType _da (msg4tag);
__EXTERN wroomType _da (msg4ward);
__EXTERN tagDataType _da (tagArray) [tag_lim];
__EXTERN word _da (tag_auditFreq);
__EXTERN word _da (tag_eventGran);
__EXTERN word _da (app_flags);

// Methods/functions: need no EXTERN

void	_da (stats) ();
void	_da (app_diag) (const word, const char *, ...);
void	_da (net_diag) (const word, const char *, ...);

word	_da (countTags) ();

void 	_da (process_incoming) (word state, char * buf, word size, word rssi);
void	_da (check_msg4tag) (nid_t tag);
int 	_da (check_msg_size) (char * buf, word size, word repLevel);
void 	_da (check_tag) (word state, word i, char** buf_out);
int 	_da (find_tag) (lword tag);
char * 	_da (get_mem) (word state, int len);
void 	_da (init_tag) (word i);
void 	_da (init_tags) (void);
int 	_da (insert_tag) (lword tag);
void 	_da (set_tagState) (word i, tagStateType state, Boolean updEvTime);

void 	_da (msg_findTag_in) (word state, char * buf);
void 	_da (msg_findTag_out) (word state, char** buf_out, lword tag,
								nid_t peg);
void 	_da (msg_master_in) (char * buf);
void 	_da (msg_master_out) (word state, char** buf_out, nid_t peg);
void 	_da (msg_pong_in) (word state, char * buf, word rssi);
void 	_da (msg_reportAck_in) (char * buf);
void 	_da (msg_reportAck_out) (word state, char * buf, char** buf_out);
void 	_da (msg_report_in) (word state, char * buf);
void 	_da (msg_getTagAck_in) (word state, char * buf, word size);
void 	_da (msg_setTagAck_in) (word state, char * buf, word size);
void 	_da (msg_report_out) (word state, int tIndex, char** buf_out);
void 	_da (msg_fwd_in) (word state, char * buf, word size);
void 	_da (msg_fwd_out) (word state, char** buf_out, word size, lword tag,
							nid_t peg, lword pass);
void 	_da (copy_fwd_msg) (word state, char** buf_out, char * buf, word size);

void 	_da (oss_findTag_in) (word state, lword tag, nid_t peg);
void 	_da (oss_getTag_in) (word state, lword tag, nid_t peg, lword pass);
void 	_da (oss_setTag_in) (word state, lword tag, nid_t peg, lword pass,
		nid_t nid, word	in_maj, word in_min, word in_pl, word in_span,
			lword npass);
void 	_da (oss_setPeg_in) (word state, nid_t peg, nid_t nid, word pl,
								char * str);
void 	_da (oss_master_in) (word state, nid_t peg);
void 	_da (oss_report_out) (char * buf, word fmt);
void 	_da (oss_setTag_out) (char * buf, word fmt);
void 	_da (oss_getTag_out) (char * buf, word fmt);

void 	_da (send_msg) (char * buf, int size);

#ifdef	__SMURPH__

// For PICOS, this stuff is included from app_tarp_if_tag.h (as macros). For
// SMURPH, we need a bunch of virtual functions.

virtual Boolean _da (msg_isBind) (msg_t m) { return NO; };
virtual Boolean _da (msg_isTrace) (msg_t m) { return NO; };
virtual Boolean _da (msg_isMaster) (msg_t m) { return m == msg_master; };
virtual Boolean _da (msg_isNew) (msg_t m) { return NO; }
virtual Boolean _da (msg_isClear) (byte o) { return YES; };
virtual void _da (set_master_chg) () { _da (app_flags) |= 2; };

#endif	/* SMURPH */

#endif
