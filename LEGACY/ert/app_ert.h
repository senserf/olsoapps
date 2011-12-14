#ifndef __app_ert_h
#define __app_ert_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2009                    */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "msg_tarp.h"

// CMD_HEAD_LEN: nid_t 1 1
#define CMD_HEAD_LEN	4
// RET_HEAD_LEN: nid_t 1 1 1
#define RET_HEAD_LEN	5

// cmd_ctrl and cmd line semaphores
#define CMD_READER      (&cmd_line)
#define CMD_WRITER      (&cmd_line +1)
typedef struct cmdCtrlStruct {
	nid_t	s;
	nid_t	t;
	byte 	opcode;
	byte 	opref;
	byte 	oprc;
	byte 	oplen;
} cmdCtrlType;

#define BEAC_TRIG (&freqs)
#define CON_TRIG  (&connect)

#define beac_freq	(freqs & 0x00FF)
#define audit_freq	(freqs >> 8)
#define con_warn	(connect >> 12)
#define con_bad		((connect >> 8) &0x0F)
#define con_miss	(connect & 0x00FF)


/**************
LED params:
CON_LED -- connectivity LED number
LED_PIN0-1 -- when CON_ON_PINS, these pins signal conn. LED status
**************/
#define CON_LED 0
#define LED_ON	1
#define LED_OFF 0
#define LED_BLINK 2
#define LED_PIN0 8
#define LED_PIN1 9
#ifndef CON_ON_PINS
#define CON_ON_PINS 0
#endif

typedef struct brCtrlStruct {
	lword	dont_esn;
	byte	dont_s;
	byte	flags;
	word	rep_freq;
} brCtrlType;

#define ST_ACKS	(&br_ctrl)
#define ST_REPTRIG (&br_ctrl.rep_freq)

#define DAT_ACK_TRIG (&dat_ptr)
#define IO_ACK_TRIG (&io_pload)

#define	DAT_ACK		1
#define IO_ACK		2
#define BR_STACK	4
#define BR_STNACK_OBSOLETE	8

// br_ctrl meant "bridge control" in Genesis, but we use
// spare bits for identical reliable delivery for VMesh IO snf DAT
#define is_datACK	(br_ctrl.flags & DAT_ACK)
#define set_datACK	(br_ctrl.flags |= DAT_ACK)
#define clr_datACK	(br_ctrl.flags &= ~DAT_ACK)

#define is_ioACK	(br_ctrl.flags & IO_ACK)
#define set_ioACK	(br_ctrl.flags |= IO_ACK)
#define clr_ioACK	(br_ctrl.flags &= ~IO_ACK)

#define is_brSTACK      (br_ctrl.flags & BR_STACK)
#define set_brSTACK     (br_ctrl.flags |= BR_STACK)
#define clr_brSTACK     (br_ctrl.flags &= ~BR_STACK)

#if 0
#define is_brSTNACK     (br_ctrl.flags & BR_STNACK)
#define set_brSTNACK    (br_ctrl.flags |= BR_STNACK)
#define clr_brSTNACK    (br_ctrl.flags &= ~BR_STNACK)
#endif

typedef struct cycCtrlStruct {
	word	mod	:2;
	word	st	:2;
	word	prep	:12;
} cycCtrlType;
#define CYC_ST_DIS  0
#define CYC_ST_ENA  1
#define CYC_ST_PREP 2
#define CYC_ST_SLEEP 3
#define CYC_MOD_NET 0
#define CYC_MOD_PNET 1
#define CYC_MOD_PON 2
#define CYC_MOD_POFF 3
#define CYC_MSG_FORCE_ENA 0xEFFF
#define CYC_MSG_FORCE_DIS 0xDFFF

// 90 s default sleep time, master sync & rest, mode, and state 
#define DEFAULT_CYC_SP  	90 
#define DEFAULT_CYC_M_SYNC	60
#define DEFAULT_CYC_M_REST	30

/* app_flags ---------------------------
b0 - autoack
b1 - master changed (in tarp) flag
b2-b3 - encryption key #
b4    - encryption mode
b5    - node can be a binder (1), or not (0)
b6    - uart modes: cmd (1), dat (0)
b7    - uart dat: cr (1), no termination (0)
b8-b11 - timeout for reliable msgs
b12-15 0 # of retries  -"-
---------------------------------------
DEF: retries 3, tout 10, uart dat cr, uart mode cmd, binder, encr data 0,
00, master chg 0, autoack 1
Set in app.c::read_eprom_and_init()
--------------------------------------*/ 
#define DEFAULT_APP_FLAGS	0x3A61
#define is_autoack	(app_flags & 1)
#define set_autoack	(app_flags |= 1)
#define clr_autoack	(app_flags &= ~1)

#define is_crmode      (app_flags & 128)
#define set_crmode     (app_flags |= 128)
#define clr_crmode     (app_flags &= ~128)

#define is_cmdmode	(app_flags & 64)
#define set_cmdmode	(app_flags |= 64)
#define clr_cmdmode	(app_flags &= ~64)

#define is_binder	(app_flags & 32)
#define set_binder	(app_flags |= 32)
#define clr_binder	(app_flags &= ~32)

#define set_retries(r)  (app_flags = (app_flags & 0x0FFF) | ((word)(r) << 12))
#define ack_retries	(app_flags >> 12)
#define ack_tout	((app_flags >> 8) & 0x000F)
#define set_tout(t)	(app_flags = (app_flags & 0xF0FF) | ((word)((t) & 0x000F) << 8))

// set_ in app_tarp_if.h
#define clr_master_chg  (app_flags &= ~2)
#define is_master_chg	(app_flags & 2)

#define encr_data	((app_flags >> 2) & 7)
#define set_encr_mode(m) (app_flags = (m) == 0 ? app_flags & ~16 : app_flags | 16)
#define set_encr_key(k) (app_flags = (app_flags & ~12) | ((k & 3) << 2))
#define set_encr_data(d) (app_flags = (app_flags & ~28) | ((d & 7) << 2))

#endif
