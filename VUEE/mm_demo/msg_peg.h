#ifndef __msg_peg_h
#define __msg_peg_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "msg_tarp.h"

typedef enum {
	msg_null, msg_profi, msg_data, msg_alrm
} msgTypee;

typedef struct msgAlrmStruct {
	headerType      header;
	profi_t		profi;
	word		level;
	char		nick[NI_LEN +1];
	char		desc[PEG_STR_LEN +1];
} msgAlrmType;

#define in_alrm(buf, field)   (((msgAlrmType *)(buf))->field)

typedef struct msgProfiStruct {
	headerType      header;
	profi_t		profi;
	word		pl:4;
	word		spare:12;
	char		nick[NI_LEN +1];
} msgProfiType;
	
#define in_profi(buf, field)   (((msgProfiType *)(buf))->field)

typedef struct msgDataStruct {
	headerType      header;
	char		desc[PEG_STR_LEN +1];
	word		info;
} msgDataType;

#define in_data(buf, field)	(((msgDataType *)(buf))->field)

#endif
