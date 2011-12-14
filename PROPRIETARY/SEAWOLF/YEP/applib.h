#ifndef	__applib_h__
#define	__applib_h__

#include "vuee.h"

#include "lcdg_dispman.h"
#include "sealists.h"

#include "applib_types.h"

//+++ "applib.c"

word handle_nbh (word what, word id);
word handle_ad (word what, word plod);
void init_glo ();
void process_incoming ();
void update_line (word, word, word, word);
void paint_nh ();
void paint_scr (word);
void display_rec (word);
lcdg_dm_obj_t *mkrmenu ();

#ifdef	__SMURPH__

#define	nbh_menu	_daprx (nbh_menu)
#define lcd_menu	_daprx (lcd_menu)
#define	rf_rcv		_daprx (rf_rcv)
#define	ad_rcv		_daprx (ad_rcv)
#define	cxt_flag	_daprx (cxt_flag)
#define ad_buf		_daprx (ad_buf)
#define curr_rec	_daprx (curr_rec)
#define	host_id		_daprx (host_id)

#else

extern nbh_menu_t 	nbh_menu;
extern lcdg_dm_men_t   *lcd_menu;
extern rf_rcv_t		rf_rcv, ad_rcv;
extern cflags_t		cxt_flag;
extern char *		ad_buf;
extern sea_rec_t *	curr_rec;

extern const lword host_id;

#endif

#endif
