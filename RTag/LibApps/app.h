#ifndef __app_h
#define __app_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"
#include "msg_tarp.h"

typedef struct appCountStruct {
	word rcv;
	word snd;
	word fwd;
} appCountType;

// CMD_HEAD_LEN: 1 nid_t 1 nid_t 1 1
#define CMD_HEAD_LEN	8
// RET_HEAD_LEN: 1 nid_t 1 nid_t 1 nid_t 1 1 1
#define RET_HEAD_LEN	12

// cmd_ctrl and cmd line semaphores
#define CMD_READER      ((word)&cmd_line)
#define CMD_WRITER      ((word)((&cmd_line)+1))
typedef struct cmdCtrlStruct {
	nid_t    s;
	nid_t    p;
	nid_t    t;
	byte 	s_q;
	byte 	p_q;
	byte 	t_q;
	byte 	opcode;
	byte 	opref;
	byte 	oprc;
	byte 	oplen;
	byte	fill; // max hops? beacon?
} cmdCtrlType;

/******************
cyc_ctrl bits:
sleeping unit (1 -- minutes) CYC_SU
active unit   (     ""     ) CYC_AU
in sleeping state            CYC_SLEEPING
******************/
#define CYC_SU	1
#define CYC_AU	2
#define CYC_SLEEPING 4

// this is from VMesh... only chg_master is used, so far...
/* app_flags ---------------------------
   b0 - autoack
   b1 - master changed (in tarp) flag
   b2-b3 - encryption key #
   b4    - encryption mode
   b5    - node can be a binder (1), or not (0)
   b6    - uart modes: cmd (1), dat (0)
   b7    - spare
   b8-b11 - timeout for reliable msgs
   b12-15 0 # of retries  -"-
   ---------------------------------------
DEF: retries 3, tout 10, 1b spare, uart mode 1, binder, encr data 0,
00, master chg 0, autoack 1
Set in app.c::read_eprom_and_init()
--------------------------------------*/
#define DEFAULT_APP_FLAGS       0x3A61
#define is_autoack      (app_flags & 1)
#define set_autoack     (app_flags |= 1)
#define clr_autoack     (app_flags &= ~1)

#define is_cmdmode      (app_flags & 64)
#define set_cmdmode     (app_flags |= 64)
#define clr_cmdmode     (app_flags &= ~64)

#define is_binder       (app_flags & 32)
#define set_binder      (app_flags |= 32)
#define clr_binder      (app_flags &= ~32)

#define set_retries(r)  (app_flags = (app_flags & 0x0FFF) | ((word)(r) << 12))
#define ack_retries     (app_flags >> 12)
#define ack_tout        ((app_flags >> 8) & 0x000F)
#define set_tout(t)     (app_flags = (app_flags & 0xF0FF) | ((word)((t) & 0x000F) << 8))

// set_ in app_tarp_if.h
#define clr_master_chg  (app_flags &= ~2)
#define is_master_chg   (app_flags & 2)

#define encr_data       ((app_flags >> 2) & 7)
#define set_encr_mode(m) (app_flags = (m) == 0 ? app_flags & ~16 : app_flags | 16)
#define set_encr_key(k) (app_flags = (app_flags & ~12) | ((k & 3) << 2))
#define set_encr_data(d) (app_flags = (app_flags & ~28) | ((d & 7) << 2))

#endif
