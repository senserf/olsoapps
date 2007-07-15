#ifndef	__attribs_tag_h__
#define	__attribs_tag_h__

#ifdef	__SMURPH__
	// These are static
	char 	*_da (ui_ibuf), *_da (ui_obuf), *_da (cmd_line);
#endif

__EXTERN __CONST lword _da (host_id);
__EXTERN lword	_da (host_passwd);

__EXTERN appCountType _da (app_count);
__EXTERN pongParamsType _da (pong_params);

__EXTERN word	_da (app_flags);

// Methods/functions: need no EXTERN

void	_da (stats) ();
void	_da (app_diag) (const word, const char *, ...);
void	_da (net_diag) (const word, const char *, ...);

void	_da (process_incoming) (word state, char * buf, word size);
word 	_da (check_passwd) (lword p1, lword p2);
char * 	_da (get_mem) (word state, int len);
void 	_da (set_tag) (char * buf);

void 	_da (msg_getTag_in) (word state, char * buf);
void 	_da (msg_setTag_in) (word state, char * buf);
void 	_da (msg_getTagAck_out) (word state, char** buf_out, nid_t rcv,
		seq_t seq, word pass);

void 	_da (msg_setTagAck_out) (word state, char** buf_out, nid_t rcv,
		seq_t seq, word pass);

word 	_da (max_pwr) (word p_levs);
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
