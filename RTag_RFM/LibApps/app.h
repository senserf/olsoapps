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

// CMD_HEAD_LEN: 1 id_t 1 id_t 1 1
#define CMD_HEAD_LEN	8
// RET_HEAD_LEN: 1 id_t 1 id_t 1 id_t 1 1 1
#define RET_HEAD_LEN	12

// cmd_ctrl and cmd line semaphores
#define CMD_READER      ((word)&cmd_line)
#define CMD_WRITER      ((word)((&cmd_line)+1))
typedef struct cmdCtrlStruct {
	id_t    s;
	id_t    p;
	id_t    t;
	unsigned char	s_q;
	unsigned char	p_q;
	unsigned char	t_q;
	unsigned char	beacon; // later?
	unsigned char	opcode;
	unsigned char	opref;
	unsigned char	oprc;
	unsigned char	oplen;
} cmdCtrlType;

#endif
