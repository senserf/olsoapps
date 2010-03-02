#ifndef __msg_structs_peg_h
#define __msg_structs_peg_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
#include "msg_tarp.h"
#include "msg_structs_tag.h"

typedef struct msgMasterStruct {
	headerType	header;
	long		mdate;
	word		syfreq;
	word		plotid;
} msgMasterType;

#define in_master(buf, field)   (((msgMasterType *)(buf))->field)

typedef struct msgReportStruct {
	headerType	header;
	long		tStamp;

	word		tagid;
	word		rssi:8;
	word		pl:4;
	word		state:4;

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
	word		tagid;
	word		state;
	//word		count; // may be useless here...
} msgReportAckType;

#define in_reportAck(buf, field)   (((msgReportAckType *)(buf))->field)

typedef struct msgFwdStruct {
	headerType      header;
	nid_t		target;
} msgFwdType;

#define in_fwd(buf, field)   (((msgFwdType *)(buf))->field)

typedef struct msgFindTagStruct {
	headerType	header;
	nid_t		target;
} msgFindTagType;

#define in_findTag(buf, field)   (((msgFindTagType *)(buf))->field)

typedef struct msgSetPegStruct {
	headerType      header;
	word		audi;
	word		level;
	word		a_fl;
} msgSetPegType;

#define in_setPeg(buf, field)	(((msgSetPegType *)(buf))->field)

typedef struct msgStatsPegStruct {
	headerType      header;
	lword		hostid;
	lword		ltime;
	long		mts;
	lword		slot;
	word		audi;
	word		pl  :4;
	word		inp :12;
	word		mhost;
	word		mem;
	word		mmin;
	word		a_fl;
} msgStatsPegType;

#define in_statsPeg(buf, field)   (((msgStatsPegType *)(buf))->field)

typedef struct reportPloadStruct {
	pongPloadType ppload;
	long	ds;
	lword	eslot;
} reportPloadType;
#define in_reportPload(buf, field) (((reportPloadType *)(buf + \
				sizeof(msgReportType)))->field)
#define in_rpload(frm, field) (((reportPloadType *)(buf))->field)

#endif
