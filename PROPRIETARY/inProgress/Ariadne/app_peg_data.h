#ifndef	__app_peg_data_h__
#define	__app_peg_data_h__

#include "app_peg.h"

extern const lword 	host_id;
extern sattr_t 		sattr [];
extern tout_t 		touts;
extern word		sstate;
extern word		oss_stack;
extern word		cters [];
extern word 		app_flags;

// Methods/functions: need no EXTERN
void	stats (char * buf);
void	app_diag (const word, const char *, ...);
void	net_diag (const word, const char *, ...);
void	show_stuff (word s);

int 	check_msg_size (char * buf, word size, word repLevel);
char * 	get_mem (word state, int len);
void 	init (void);
void	set_rf (word);
void	set_state (word);

void 	msg_master_in (char * buf);
void 	msg_master_out (void);
void	msg_ping_in (char * buf, word rssi);
void	msg_ping_out (char * buf, word rssi);
void	msg_cmd_in (char * buf);
void	msg_cmd_out (byte cmd, word dst, word a);
void	msg_sil_in (char * buf);
void	msg_sil_out (void);
void	msg_stats_in (char * buf);
void	msg_stats_out (void);

void	oss_ping_out (char * buf, word rssi);

void 	send_msg (char * buf, int size);
void	strncpy (char *d, const char *s, sint n);

fsm m_cyc;
fsm m_sil;

#endif

