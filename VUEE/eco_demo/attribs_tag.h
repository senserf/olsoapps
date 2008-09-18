#ifndef	__attribs_tag_h__
#define	__attribs_tag_h__

__EXTERN __CONST lword _da (host_id);

__EXTERN lword	  _da (ref_time);
__EXTERN mclock_t _da (ref_clock);

__EXTERN pongParamsType _da (pong_params);

__EXTERN word	_da (app_flags);
__EXTERN char	*_da (ui_ibuf), *_da (ui_obuf), *_da (cmd_line);

__EXTERN sensDataType	_da (sens_data);
__EXTERN long		_da (lh_time);
__EXTERN sensEEDumpType *_da (sens_dump);

// Methods/functions: need no EXTERN

void	_da (stats) (void);
void	_da (app_diag) (const word, const char *, ...);
void	_da (net_diag) (const word, const char *, ...);

void	_da (process_incoming) (word state, char * buf, word size);
char * 	_da (get_mem) (word state, int len);

void 	_da (msg_setTag_in) (char * buf);
void	_da (msg_pongAck_in) (char * buf);

word 	_da (max_pwr) (word p_levs);
void 	_da (send_msg) (char * buf, int size);

void	_da (fatal_err) (word err, word w1, word w2, word w3);
void	_da (sens_init) ();
void	_da (init) ();
word	_da (r_a_d) (void);
void	_da (wall_time) (mclock_t *mc);
void	_da (upd_on_ack) (lword ts, lword rt);
word	_da (handle_c_flags) (word c_fl);
void	_da (tmpcrap) (word);

// Expected by NET and TARP

int _da (tr_offset) (headerType*);
Boolean _da (msg_isBind) (msg_t m);
Boolean _da (msg_isTrace) (msg_t m);
Boolean _da (msg_isMaster) (msg_t m);
Boolean _da (msg_isNew) (msg_t m);
Boolean _da (msg_isClear) (byte o);
void _da (set_master_chg) (void);

#endif
