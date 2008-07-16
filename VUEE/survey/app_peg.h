#ifndef __app_peg_h
#define __app_peg_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */

#define	THREADNAME(a)	a ## _peg

// Content of all LibApps

//+++ "lib_app_peg.c"
//+++ "msg_io_peg.c"
//+++ "app_diag_peg.c"

#include "sysio.h"
#include "msg_tarp.h"

// uart ON in human i/f, no master change, no autoack (unused)
#define DEF_APP_FLAGS 	0x0C

// level 2, rec 3, slack 0, fwd off
#define DEF_TARP	0xB0

#define DEF_XMT_THOLD	10
#define LOW_OSS_STACK	2
#define MAX_OSS_STACK	10

#define DEF_PL_FR	7
#define DEF_PL_TO	0

#define DEF_RA_FR	0
#define DEF_RA_TO	3

#define DEF_CH_FR	0
#define DEF_CH_TO	255

#define DEF_NID 	1
#define DEF_MHOST	0x0101

#define DEF_I_SIL	3
#define DEF_I_WARM	5
#define DEF_I_CYC	15

#define ST_INIT		0
#define ST_WARM		1
#define	ST_CYC		2
#define ST_COOL		3
#define ST_OFF		4

#define is_autoack	(app_flags & 1)
#define set_autoack	(app_flags |= 1)
#define clr_autoack	(app_flags &= ~1)

#define clr_master_chg  (app_flags &= ~2)
#define is_master_chg   (app_flags & 2)

#define clr_uart_out    (app_flags &= ~4)
#define is_uart_out     (app_flags & 4)
#define set_uart_out    (app_flags |= 4)

#define clr_fmt_hi	(app_flags &= ~8)
#define is_fmt_hi	(app_flags & 8)
#define set_fmt_hi	(app_flags |= 8)

#define clr_ping_out	(app_flags &= ~16)
#define is_ping_out	(app_flags & 16)
#define set_ping_out	(app_flags |= 16)

#define LED_R	0
#define LED_G	1
#define LED_B	2

#define LED_OFF	0
#define LED_ON	1
#define LED_BLINK 2

#define NVM_OSET (1024L << 8)

// Semaphores
#define CMD_READER	(&cmd_line)
#define CMD_WRITER	((&cmd_line)+1)
#define SILENC		(&touts)

typedef union {
	word	w;
	struct {
		word	pl:4; 
		word	ra:4;
		word	ch:8;
	} b;
} sattr_t;

typedef union {
	word	w;
	struct {
		word	sil:4;
		word	warm:4;
		word	cyc:8;
	} b;
} ival_t;

typedef struct {
	lword ts;
	word	mc;
	ival_t  iv;
} tout_t;

typedef struct {
	nid_t	mh;
	nid_t	lh;
	ival_t  iv;
	sattr_t attr[2];
} nvm_t;

#endif
