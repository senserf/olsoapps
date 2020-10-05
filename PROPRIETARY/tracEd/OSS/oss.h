/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __oss_h__
#define __oss_h__

#include "sysio.h"

// interface-specific stuff
#include "ossi.h"

// this is NOT an attempt at general structs. They should be bytestreams,
// along the ones in VMesh. Same about reusing the same structs for in/out.

#define MAX_ARGC	3

typedef struct cmdStruct {
	word	code  :8;
	word	arg_c :8;
	word	argv_w [MAX_ARGC];
	union {
		char	* buf;
		word	size;
	};
} cmd_t;

typedef struct reqStruct {
	word src;
	cmd_t * cmd;
} req_t;

typedef struct mbStruct {
	char 	* b;
	word	f :6;
	word	s :6;
	word	spare :4;
} mb_t;

#endif
