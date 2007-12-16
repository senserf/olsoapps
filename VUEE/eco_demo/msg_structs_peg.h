#ifndef __msg_structs_peg_h
#define __msg_structs_peg_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "msg_tarp.h"
#include "msg_structs_tag.h"

#define PEG_STR_LEN 16

typedef struct msgNewStruct {
	headerType      header;
} msgNewType;

#define in_new(buf, field)   (((msgNewType *)(buf))->field)

typedef struct msgNewAckStruct {
	headerType      header;
	word            master;
	word		hco:8;
	word		spare:8;
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
	long		tStamp;
	lword		tagId;
	word		state;
	word 		count;
	word		flags; // events, housekeeping
} msgReportType;
#define REP_FLAG_NOACK	1
#define REP_FLAG_PLOAD  2
#define in_report_noack(buf) (((msgReportType *)(buf))->flags & REP_FLAG_NOACK)
#define in_report_pload(buf) (((msgReportType *)(buf))->flags & REP_FLAG_PLOAD)

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
	lword		target;
} msgFwdType;

#define in_fwd(buf, field)   (((msgFwdType *)(buf))->field)

typedef struct msgAlrmStruct {
	headerType      header;
	word		level;
	word		spare;
} msgAlrmType;

#define in_alrm(buf, field)   (((msgAlrmType *)(buf))->field)

typedef struct msgFindTagStruct {
	headerType	header;
	lword		target;
} msgFindTagType;

#define in_findTag(buf, field)   (((msgFindTagType *)(buf))->field)

typedef struct msgSetPegStruct {
	headerType      header;
	word		level;
	word		new_id;
	char            str[PEG_STR_LEN];
} msgSetPegType;

#define in_setPeg(buf, field)   (((msgSetPegType *)(buf))->field)

typedef struct reportPloadStruct {
	pongPloadType ppload;
	long	ts;
	lword	eslot;
} reportPloadType;
#define in_reportPload(buf, field) (((reportPloadType *)(buf + \
				sizeof(msgReportType)))->field)
#define in_rpload(frm, field) (((reportPloadType *)(buf))->field)

#endif
