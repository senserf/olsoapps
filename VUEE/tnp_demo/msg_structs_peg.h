#ifndef __msg_structs_peg_h
#define __msg_structs_peg_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "msg_tarp.h"

#define PEG_STR_LEN 16

typedef struct msgNewStruct {
	headerType      header;
	lword           passwd;
} msgNewType;

#define in_new(buf, field)   (((msgNewType *)(buf))->field)

typedef struct msgNewAckStruct {
	headerType      header;
	word            master;
	word		    hco:8;
	word		    spare:8;
	lword           mtime;
} msgNewAckType;

#define in_newAck(buf, field)   (((msgNewAckType *)(buf))->field)

typedef struct msgMasterStruct {
	headerType      header;
	lword           mtime;
} msgMasterType;

#define in_master(buf, field)   (((msgMasterType *)(buf))->field)

typedef struct msgReportStruct {
	headerType	header;
	lword		tStamp;
	lword		tagId;
	word		state;
	word 		count;
	word		flags; // events, housekeeping
} msgReportType;

#define in_report(buf, field)   (((msgReportType *)(buf))->field)

typedef struct msgReportAckStruct {
	headerType	header;
	lword		tagId;
	word		state;
	word		count; // may be useless here...
} msgReportAckType;

#define in_reportAck(buf, field)   (((msgReportAckType *)(buf))->field)

typedef struct msgFwdStruct {
	headerType      header;
	lword		    target;
	lword           passwd;
} msgFwdType;

#define in_fwd(buf, field)   (((msgFwdType *)(buf))->field)

typedef struct msgAlrmStruct {
	headerType      header;
	word		    level;
	word		    spare;
} msgAlrmType;

#define in_alrm(buf, field)   (((msgAlrmType *)(buf))->field)

typedef struct msgFindTagStruct {
	headerType	header;
	lword		target;
} msgFindTagType;

#define in_findTag(buf, field)   (((msgFindTagType *)(buf))->field)

typedef struct msgSetPegStruct {
	headerType      header;
	word		    level;
	word		    new_id;
	char            str[PEG_STR_LEN];
} msgSetPegType;

#define in_setPeg(buf, field)   (((msgSetPegType *)(buf))->field)

#endif
