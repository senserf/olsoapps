#ifndef	__attribs_peg_h__
#define	__attribs_peg_h__

__EXTERN __CONST lword _da (host_id);
__EXTERN word _da (host_pl);

__EXTERN tagDataType  _da (tagArray) [LI_MAX];
__EXTERN tagShortType _da (ignArray) [LI_MAX];
__EXTERN tagShortType _da (monArray) [LI_MAX];
__EXTERN nbuComType   _da (nbuArray) [LI_MAX];

__EXTERN word _da (tag_auditFreq);
__EXTERN word _da (tag_eventGran);
__EXTERN word _da (app_flags);
__EXTERN char *_da (ui_ibuf), *_da (ui_obuf), *_da (cmd_line);

__EXTERN profi_t _da (profi_att), _da (p_inc), _da (p_exc);
__EXTERN char _da (desc_att) [PEG_STR_LEN +1];
__EXTERN char _da (d_biz) [PEG_STR_LEN +1];
__EXTERN char _da (d_priv) [PEG_STR_LEN +1];
__EXTERN char _da (d_alrm) [PEG_STR_LEN +1];
__EXTERN char _da (nick_att) [NI_LEN +1];
__EXTERN ledStateType _da (led_state);

// Methods/functions: need no EXTERN

void	_da (stats) (word);
void	_da (app_diag) (const word, const char *, ...);
void	_da (net_diag) (const word, const char *, ...);

void 	_da (process_incoming) (word state, char * buf, word size, word rssi);
int 	_da (check_msg_size) (char * buf, word size, word repLevel);
void 	_da (check_tag) (word i);
int 	_da (find_tag) (word tag);
int	_da (find_ign) (word tag);
int	_da (find_mon) (word tag);
int	_da (find_nbu) (word tag);
char * 	_da (get_mem) (word state, int len);
void 	_da (init_tag) (word i);
void	_da (init_ign) (word i);
void	_da (init_mon) (word i);
void	_da (init_nbu) (word i);
void 	_da (init_tags) (void);
int 	_da (insert_tag) (char * buf);
int	_da (insert_ign) (word tag, char * nick);
int	_da (insert_mon) (word tag, char * nick);
int	_da (insert_nbu) (word id, word w, word v, word h, char * s);
void 	_da (set_tagState) (word i, tagStateType state, Boolean updEvTime);
void	_da (nbuVec) (char *s, byte b);

void 	_da (msg_profi_in) (char * buf, word rssi);
void 	_da (msg_profi_out) (nid_t peg);
void	_da (msg_data_in) (char * buf);
void	_da (msg_data_out) (nid_t peg, word info);
void	_da (msg_alrm_in) (char * buf);
void	_da (msg_alrm_out) (nid_t peg, word level, char * desc);

void 	_da (oss_profi_out) (word ind);
void	_da (oss_data_out) (word ind);
void 	_da (oss_alrm_out) (char * buf);
void	_da (oss_nvm_out) (nvmDataType * buf, word slot);

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
