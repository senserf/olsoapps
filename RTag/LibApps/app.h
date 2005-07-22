#ifndef __app_h
#define __app_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "sysio.h"

#define OSS_HT  0
#define OSS_TCL 1
#define oss_fmt OSS_HT

typedef struct appCountStruct {
	word rcv;
	word snd;
	word fwd;
} appCountType;

#endif
