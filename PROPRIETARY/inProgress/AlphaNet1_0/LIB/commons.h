/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef __commons_h__
#define __commons_h__

//+++ commons.cc

/* All(?) common structs & defines */
#include "sysio.h"
#include "msg_tarp.h"

#define DEF_NID 77
#define DEF_MHOST 1

#define LED_R   0
#define LED_G   1
#define LED_B   2
#define LED_4	3

// Need to remove the third led from RADIO_USE_LEDS
#define	MASTER_STATUS_LED LED_B

#define LED_OFF 0
#define LED_ON  1
#define LED_BLINK 2

#define TO_ALL	0
#define TO_NET	1
#define TO_OSS	2

#define TRIG_ALRM	1
#define TRIG_ACK	2
#define TRIG_RONIN	3
#define TRIG_DORO	4
#define TRIG_OSSO	5
#define TRIG_MBEAC	6
#define TRIG_OSSIN	7

/////////// tag-related structs ////////////////

typedef struct pongParamsStruct {
    word retry_del 	:6; // times in seconds
    word retry_nr 	:3;
	word rx_span 	:3;
	word spare	:4;
    word pow_levels;
} pongParamsType;

typedef struct pongDataStruct {
    word    btyp     :4;
    word    plev     :3;
    word    alrm_id  :3;
    word    alrm_seq :4;
    word    fl2      :2;    // board-specific flag... filler

    word    len      :6;
    word 	trynr    :3;
	word	dupeq	 :4;
	word	noack	 :1;	// doesn't really belong here, but this place is handy
    word	spare    :2;
} pongDataType; // 4B (+len bytes of pload serialized after this)

typedef struct tagDataStruct {
	char  * nel;
	word	audTime;
	word	refTime;
    word    tagid;
    word    rssi  :8;
	word	marka :3;
	word	spare :5;
} tagDataType;
#define in_tdt(b, f) (((tagDataType *)(b))->f)
#define in_pdt(b, f) (((pongDataType *)(b + sizeof(tagDataType)))->f)
#define in_ppt(b, f) (((pongDataType *)(b + sizeof(tagDataType) + \
		sizeof(pongDataType)))->f)


/* the list's element is serialized tagDataType, pongDataType and board-spec
   pongPloadType. All malloced in one piece, pointed at by nel (next element).
   ((tagDataType *)(nel))->nel is the next element.
*/
typedef struct tagListStruct {
	word	alrms :8;
	word	evnts :8;
	word	marka :3;
	word	block :1;
	word	spare :12; // we can have app flags here...
	char  * nel;
} tagListType;

typedef struct roguemStruct {
	lword 	ts;
	word	nid;
	word	cnt :8;
	word	hops :8;
} roguemType;

///////////// messages ////////////////////

typedef enum {
        msg_null, msg_pong, msg_pongAck,
        msg_master, msg_report, msg_reportAck, msg_fwd, msg_fwdAck
} msgType;


typedef struct msgPongStruct {
    headerType      header;
	pongDataType	pd;
} msgPongType;
#define in_pong(buf, field)     (((msgPongType *)(buf))->field)
// pongPloadType variants are board-specofoc
#define in_pongPload(buf, field) (((pongPloadType *)(buf + \
                                sizeof(msgPongType)))->field)

typedef struct msgPongAckStruct {
    headerType      header;
	word		dupeq 	 :4;
	word		spare    :12;
} msgPongAckType;
#define in_pongAck(buf, field)     (((msgPongAckType *)(buf))->field)

typedef struct msgMasterStruct {
    headerType      header;
} msgMasterType;
#define in_master(buf, field)     (((msgPongType *)(buf))->field)

typedef struct msgReportStruct {
    headerType  header;
    word        ref;
    word        tagid;
	word		rssi :8;
	word		ago  :8;
} msgReportType; // follows serialized pdt & ppt structured as above
#define in_report(buf, field)   (((msgReportType *)(buf))->field)

typedef struct msgReportAckStruct {
        headerType      header;
        word            ref;
        word            tagid;
} msgReportAckType;
#define in_reportAck(buf, field)   (((msgReportAckType *)(buf))->field)

typedef struct msgFwdStruct {
        headerType      header;
        word            ref;
} msgFwdType;
#define in_fwd(buf, field)   (((msgFwdType *)(buf))->field)

typedef struct msgFwdAckStruct {
        headerType      header;
        word            ref;
} msgFwdAckType;
#define in_fwdAck(buf, field)   (((msgFwdAckType *)(buf))->field)

/////////////////// fifek ////////////////////
// small (up to 15) circ buffer of (alloc'ed) buffers
//
typedef struct fifekStruct {
        char  **b;
        word    h :4;
        word    t :4;
        word    n :4;
        word    s :4;
} fifek_t; // illegal? , *fifek_pt;

void fifek_ini (fifek_t *fif, word siz);
void fifek_reset (fifek_t *fif, word siz);
Boolean fifek_empty (fifek_t *fif);
Boolean fifek_full (fifek_t *fif);
void fifek_push (fifek_t *fif, char * el);
char * fifek_pull (fifek_t *fif);

///////////////////////////////////////////

char * get_mem (word len, Boolean reset);

#endif
