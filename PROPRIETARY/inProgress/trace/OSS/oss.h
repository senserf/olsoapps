#ifndef __oss_h__
#define __oss_h__
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

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

#endif
