/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifndef __SMURPH__
// dupa vuee barfs, must be included twice 
#include "app_tag.h"
//#include "msg_tag.h"
#endif

#include "tarp.h"

#ifdef __dcx_def__

#ifndef __app_tag_data_defined__
#define __app_tag_data_defined__

__CONST lword	host_id		__sinit (0xBACADEAD);
lword		ref_ts		__sinit (0);
long		ref_date	__sinit (0);
long		lh_time;

sensDataType	sens_data;
pongParamsType	pong_params	__sinit (60, 5, 0x7777, 2048, 0, 0);
sensEEDumpType	* sens_dump	__sinit (NULL);
char 		* ui_ibuf	__sinit (NULL);
char		* ui_obuf	__sinit (NULL);
char		* cmd_line	__sinit (NULL);

char 		* rcv_buf	__sinit (NULL);

word 		app_flags	__sinit (DEF_APP_FLAGS);
word		plot_id		__sinit (0);

word		png_shift;

sint		rcv_psize 	__sinit (0);

char		png_frame [sizeof (msgPongType) + sizeof (sensDataType)];

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
#define ref_ts          _daprx (ref_ts)
#define ref_date        _daprx (ref_date)
#define lh_time		_daprx (lh_time)
#define sens_data	_daprx (sens_data)
#define pong_params	_daprx (pong_params)
#define sens_dump	_daprx (sens_dump)
#define ui_ibuf		_daprx (ui_ibuf)
#define ui_obuf		_daprx (ui_obuf)
#define cmd_line	_daprx (cmd_line)
#define rcv_buf		_daprx (rcv_buf)
#define app_flags	_daprx (app_flags)
#define plot_id         _daprx (plot_id)
#define png_shift       _daprx (png_shift)
#define rcv_psize	_daprx (rcv_psize)
#define png_frame	_daprx (png_frame)

#else
// PicOS
//+++ app_diag_tag.c lib_app_tag.c msg_io_tag.c

extern const lword     host_id;
extern lword           ref_ts;
extern long            ref_date;
extern long            lh_time;
extern sensDataType    sens_data;
extern pongParamsType  pong_params;
extern sensEEDumpType  * sens_dump;
extern char            * ui_ibuf;
extern char            * ui_obuf;
extern char            * cmd_line;
extern char	       * rcv_buf;
extern word            app_flags;
extern word            plot_id;
extern word            png_shift;
extern sint	       rcv_psize;
extern char	       png_frame[];

// str_tag.h
extern const char ee_str[];
extern const char welcome_str[];
extern const char ill_str[];
extern const char bad_str[];
extern const char stats_str[];
extern const char ifla_str[];
extern const char dump_str[];
extern const char dumpmark_str[];
extern const char dumpend_str[];

// PicOS
#endif

void next_col_time (void);
void show_ifla_t (void);
void read_ifla_t (void);
void save_ifla_t (void);
void stats (void);
void app_diag_t (const word, const char *, ...);
void net_diag_t (const word, const char *, ...);

void  process_incoming (word state, char * buf, word size);
char * get_mem_t (word state, sint len);

void msg_setTag_in (char * buf);
void msg_pongAck_in (char* buf);

word max_pwr (word p_levs);
void send_msg_t (char * buf, sint size);

void init (void);
void sens_init (void);
void fatal_err_t (word err, word w1, word w2, word w3);
void write_mark_t (word what);
word r_a_d_t (void);
void upd_on_ack (long ds, long rd, word syfr, word ackf, word pi);
word handle_c_flags (word c_fl);
void tmpcrap_t (word);
long wall_date_t (long s);

const char * markName_t (statu_t s);
const char * statusName (statu_t s);
word map_level (word l);

#endif

// ============================================================================

#ifdef __dcx_ini__

	host_id		= (lword) preinit ("HID");
	ref_ts		= 0;
	ref_date	= 0;
	pong_params.freq_maj = 60;
	pong_params.freq_min = 5;
	pong_params.pow_levels = 0x7777;
	pong_params.rx_span = 2048;
	pong_params.rx_lev = 0;
	pong_params.pload_lev = 0;
	sens_dump	= NULL;
	ui_ibuf		= NULL;
	ui_obuf		= NULL;
	cmd_line	= NULL;
	rcv_buf		= NULL;
	app_flags	= DEF_APP_FLAGS;
	plot_id		= 0;
	rcv_psize	= 0;

#endif
