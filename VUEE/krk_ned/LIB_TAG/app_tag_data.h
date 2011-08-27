#ifndef __app_tag_data_h
#define	__app_tag_data_h

#include "app_tag.h"

extern const lword host_id;

extern lword ref_ts;
extern lint ref_date;
extern word app_flags;
extern word plot_id;
extern pongParamsType pong_params;
extern sensDataType sens_data;
extern lint lh_time;

idiosyncratic void tmpcrap (word);
idiosyncratic void app_diag (const word, const char*, ...);
idiosyncratic void net_diag (const word level, const char * fmt, ...);
idiosyncratic char *get_mem (word, sint);
idiosyncratic void send_msg (char*, sint);
word max_pwr (word);
word handle_c_flags (word);
void next_col_time ();
idiosyncratic lint wall_date (lint);
idiosyncratic void write_mark (word);
void msg_pongAck_in (char*);
void upd_on_ack (lint, lint, word, word, word);
void msg_setTag_in (char*);

#endif
