#ifndef __app_h
#define __app_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2004.                   */
/* All rights reserved.                                                 */
/* ==================================================================== */
// Content of all LibApps
//+++ "app_count.c"
//+++ "check_msg4tag.c"
//+++ "check_msg_size.c"
//+++ "copy_fwd_msg.c"
//+++ "check_tag.c"
//+++ "find_tag.c"
//+++ "get_mem.c"
//+++ "init_tag.c"
//+++ "init_tags.c"
//+++ "insert_tag.c"
//+++ "master_delta.c"
//+++ "master_host.c"
//+++ "msg4tag.c"
//+++ "msg4ward.c"
//+++ "msg_findTag_in.c"
//+++ "msg_findTag_out.c"
//+++ "msg_fwd_in.c"
//+++ "msg_fwd_out.c"
//+++ "msg_master_in.c"
//+++ "msg_master_out.c"
//+++ "msg_pong_in.c"
//+++ "msg_reportAck_in.c"
//+++ "msg_reportAck_out.c"
//+++ "msg_report_in.c"
//+++ "msg_setTagAck_in.c"
//+++ "msg_getTagAck_in.c"
//+++ "msg_report_out.c"
//+++ "oss_findTag_in.c"
//+++ "oss_getTag_in.c"
//+++ "oss_master_in.c"
//+++ "oss_report_out.c"
//+++ "oss_setTag_in.c"
//+++ "send_msg.c"
//+++ "set_tagState.c"
//+++ "tagArray.c"
//+++ "tag_auditFreq.c"
//+++ "tag_eventGran.c"
#include "sysio.h"
#include "msg_tarp.h"

typedef enum {
	noTag, newTag, reportedTag, confirmedTag,
	fadingReportedTag, fadingConfirmedTag, goneTag, sumTag
} tagStateType;

typedef struct tagDataStruct {
	lword	id;
	word	state;
	word	count;
	lword	evTime;
	lword	lastTime;
} tagDataType;

typedef struct appCountStruct {
	word rcv;
	word snd;
	word fwd;
} appCountType;

// waiting room
typedef struct wroomStruct {
	char * buf;
	lword  tstamp;
} wroomType;

#define tag_lim	4

#define OSS_HT 	0
#define OSS_TCL	1
#define oss_fmt	OSS_HT

#endif
