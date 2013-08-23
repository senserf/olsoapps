#ifndef	__app_peg_data_h
#define	__app_peg_data_h

#include "app_peg.h"

extern word host_pl;

extern tagDataType  tagArray [];
extern tagShortType ignArray [];
extern tagShortType monArray [];
extern nbuComType   nbuArray [];

extern word tag_eventGran;
extern word app_flags;

extern profi_t profi_att, p_inc, p_exc;
extern char desc_att [PEG_STR_LEN +1];
extern char d_biz [PEG_STR_LEN +1];
extern char d_priv [PEG_STR_LEN +1];
extern char d_alrm [PEG_STR_LEN +1];
extern char nick_att [NI_LEN +1];
extern ledStateType led_state;

extern trueconst char oss_out_f_str[], d_event[][12], d_nbu[][12],
       welcome_str[], hs_str[], ill_str[], bad_str[], stats_str[],
       profi_ascii_def[], profi_ascii_raw[], alrm_ascii_def[],
       alrm_ascii_raw[], nvm_ascii_def[], nvm_local_ascii_def[],
       nb_imp_str[], cur_tag_str[], be_ign_str[], be_mon_str[],
       nb_imp_el_str[], be_ign_el_str[], be_mon_el_str[], hz_str[],
       he_str[];

// Methods/functions: need no EXTERN

void	app_diag (const word, const char *, ...);
void	net_diag (const word, const char *, ...);

int 	check_msg_size (char * buf, word size, word repLevel);
void 	check_tag (word i);
int 	find_tag (word tag);
int 	find_ign (word tag);
int	find_mon (word tag);
int	find_nbu (word tag);
char * 	get_mem (word state, int len);
void 	init_tag (word i);
void	init_ign (word i);
void	init_mon (word i);
void	init_nbu (word i);
void 	init_tags (void);
int 	insert_tag (char * buf);
int	insert_ign (word tag, char * nick);
int	insert_mon (word tag, char * nick);
int	insert_nbu (word id, word w, word v, word h, char * s);
void 	set_tagState (word i, tagStateType state, Boolean updEvTime);
void	nbuVec (char *s, byte b);

void 	msg_profi_in (char * buf, word rssi);
void 	msg_profi_out (nid_t peg);
void	msg_data_in (char * buf);
void	msg_data_out (nid_t peg, word info);
void	msg_alrm_in (char * buf);
void	msg_alrm_out (nid_t peg, word level, char * desc);

void 	oss_profi_out (word ind, word list);
void	oss_data_out (word ind);
void 	oss_alrm_out (char * buf);
void	oss_nvm_out (nvmDataType * buf, word slot);

void 	send_msg (char * buf, int size);
#endif
