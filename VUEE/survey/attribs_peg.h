#ifndef	__attribs_peg_h__
#define	__attribs_peg_h__

__EXTERN __CONST lword 	_da (host_id);

__EXTERN sattr_t 	_da (sattr) [4];
__EXTERN tout_t 	_da (touts);
__EXTERN word 		_da (sstate);
__EXTERN word		_da (oss_stack);
__EXTERN word		_da (cters) [3];

__EXTERN word 		_da (app_flags);
__EXTERN char *_da (ui_ibuf), *_da (ui_obuf), *_da (cmd_line);

// Methods/functions: need no EXTERN
void	_da (stats) (char * buf);
void	_da (app_diag) (const word, const char *, ...);
void	_da (net_diag) (const word, const char *, ...);
void	_da (show_stuff) (word s);


void 	_da (process_incoming) (word state, char * buf, word size, word rssi);
int 	_da (check_msg_size) (char * buf, word size, word repLevel);
char * 	_da (get_mem) (word state, int len);
void 	_da (init) (void);
int	_da (next_cyc) (void);
void	_da (set_rf) (word);
void	_da (set_state) (word);

void 	_da (msg_master_in) (char * buf);
void 	_da (msg_master_out) (void);
void	_da (msg_ping_in) (char * buf, word rssi);
void	_da (msg_ping_out) (char * buf, word rssi);
void	_da (msg_cmd_in) (char * buf);
void	_da (msg_cmd_out) (byte cmd, word dst, word a);
void	_da (msg_sil_in) (char * buf);
void	_da (msg_sil_out) (void);
void	_da (msg_stats_in) (char * buf);
void	_da (msg_stats_out) (void);

void	_da (oss_ping_out) (char * buf, word rssi);

void 	_da (send_msg) (char * buf, int size);
void	_da (strncpy) (char *d, const char *s, sint n);

// Expected by NET and TARP
int _da (tr_offset) (headerType*);
Boolean _da (msg_isBind) (msg_t m);
Boolean _da (msg_isTrace) (msg_t m);
Boolean _da (msg_isMaster) (msg_t m);
Boolean _da (msg_isNew) (msg_t m);
Boolean _da (msg_isClear) (byte o);
void _da (set_master_chg) (void);
#endif

