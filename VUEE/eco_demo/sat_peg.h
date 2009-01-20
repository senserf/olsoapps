#ifndef __sat_peg_h
#define __sat_peg_h
/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2009.                          */
/* All rights reserved.                                                 */
/* ==================================================================== */

// for now
#define IS_DEFSATGAT	((host_id & 0xFFFF0000) == 0x5A7E0000)
#define IS_SATGAT	(sat_mod != SATMOD_NO)
#define SATWRAP		"AT+CMGS=TextMsg\r5,0,9,"
#define SATWRAPLEN	22
#define MAX_SATLEN	47

#define SATCMD_GET	"AT+CMGS=GetRxMsg\r"
#define SATCMD_STATUS	"AT+CMGS=SystemStatus\rG\032"
#define SATCMD_QUEUE	"AT+GetMsgQueueStatus\r"

#define SATATT_MSG	"+CMGS: \"Tex"
#define SAT_MSGLEN	11

#define SATIN_RNG	"PDTRING"
#define SAT_RNGLEN	7

#define SATIN_QUE	"+GetMsgQue"
#define SAT_QUELEN	10
#define SAT_QUEOSET	20
#define SAT_QUEMINLEN	23
#define SAT_QUEFORM	"%u,%u"
#define SAT_QUEFULL	40

#define SATIN_STA	"+CMGS: \"Sys"
#define SAT_STALEN	10
#define SAT_STAOSET	30
#define SAT_STAMINLEN	43
#define SAT_STAFORM	"%u %u:%u:%u"

#define SATMOD_NO	0
#define SATMOD_YES	1
#define SATMOD_UNINIT	2
#define SATMOD_FULL	3
// we're waiting on multiline; may be more than rx later
#define SATMOD_WAITRX	4

#endif
