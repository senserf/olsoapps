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

// make it illegal to the compiler
#if RADIO_OPTIONS & RADIO_OPTION_PXOPTIONS
#undef PHYSOPT_SETPOWER
#undef PHYSOPT_GETPOWER
#endif

#define SYSVER_MAJ 1
#define SYSVER_min 0x83

#ifndef	DEF_NID
#define DEF_NID ((word)(host_id >> 16))
#endif

#ifndef	DEF_CHAN
#define	DEF_CHAN 0
#endif

// #define DEF_MHOST DEF_NID either this or 1 below (March 2017, 1.83)
#ifndef DEF_MHOST
#define DEF_MHOST 1
// #define DEF_MHOST DEF_NID
#endif

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
#define TRIG_RFID	8

#define RC_OK		0
#define RC_EVAL		1	// 0xFF // -1
#define RC_EPAR		2	// 0xFE // -2
#define RC_EADDR	3	// 0xFD // -3
#define RC_ENIMP	4	// 0xFC // -4
#define RC_DUPOK	5	// 0xFB // -5
#define RC_ELEN		6	// 0xFA // -6
#define RC_ERES		7	// resources

#define CMD_GET		1
#define CMD_SET		2

#define CMD_SET_ASSOC	0x13
#define CMD_GET_ASSOC	0x14
#define CMD_CLR_ASSOC	0x15

// this should be fixed in 2.0 or even 1.5 if Renesas cooperates
#define CMD_RELAY_41	0x41
#define CMD_RELAY_42	0x42
#define CMD_RELAY_43	0x43

#define CMD_TRACE		0x51
#define CMD_ODR			0x52
#define CMD_NHOOD		0x53
#define CMD_RESET		0x54

#define CMD_FWD			0xA1
#define CMD_INJECT		0xA2

// CMD_GET's params
#define PAR_LH			0x01
#define ATTR_ESN		0x02
#define PAR_MID			0x03
#define PAR_NID			0x04
#define PAR_TARP		0x09
#define ATTR_TARP_CNT	0x0A
#define PAR_TAG_MGR		0x0B
#define PAR_AUDIT		0x0C
#define PAR_AUTOACK		0x0D
#define PAR_BEAC		0x0E
#define ATTR_VER		0x0F
#define PAR_PMOD		0x10
#define PAR_TARP_RSS	0x11
#define PAR_RFCHAN  	0x12
#define ATTR_UPTIME		0x1A
#define ATTR_MEM1		0x1B
#define ATTR_MEM2		0x1C
#define PAR_SNIFF		0x1D
// additional SET's
#define PAR_TARP_L		0x05
#define PAR_TARP_R		0x06
#define PAR_TARP_S		0x07
#define PAR_TARP_F		0x08

// reports
#define REP_EVENT		0
#define REP_RELAY		1
#define REP_FORWARD		0xA1
#define REP_LOCA		0xB1
#define REP_RFID		0xB2
#define REP_LOG			0xD1
#define REP_SNIFF		0xE1
#define REP_NHOOD		0xE2

// loca bursts / reporting for pongDataType.locat
#define LOCA_NONE		0
#define LOCA_FULL		1
#define LOCA_SHORT		2
#define LOCA_SPARE		3
// an attempt to scent shit with flowers
#define LOOP_ALRM_ID	2

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
	// note that loca field doesn't have to be filled on tags (in fact, it likely is better to have pegs decide)
    word	locat    :2;	// LOCA_NONE(0) - no loca bursts; LOCA_FULL(1) - full reps; LOCA_SHORT(2)
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

///////////// messages ////////////////////

typedef enum {
        msg_null, msg_pong, msg_pongAck,
        msg_master, msg_report, msg_reportAck, msg_fwd, msg_fwdAck, msg_ping, msg_loca,
		msg_rpc, msg_rpcAck, msg_sniff /* not really a msg */, msg_nh, msg_nhAck, msg_rfid
} msgType;

typedef struct msgLocaStruct {
    headerType  header;
	word		id;
	word		ref;
	byte		vec[32]; // LOCAVEC_SIZ in loca.h
} msgLocaType;
#define in_loca(buf, field)     (((msgLocaType *)(buf))->field)

typedef struct msgPingStruct {
    headerType  header;
	word		ref;
	word		slot;	
} msgPingType;
#define in_ping(buf, field)     (((msgPingType *)(buf))->field)

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

/////// keep them structs consistent, fields can be added...
typedef struct msgMasterStruct {
    headerType      header;
} msgMasterType;
#define in_master(buf, field)     (((msgMasterType *)(buf))->field)

typedef struct msgRpcStruct {
    headerType      header;
} msgRpcType;
#define in_rpc(buf, field)     (((msgRpcType *)(buf))->field)

typedef struct msgRpcAckStruct {
    headerType      header;
} msgRpcAckType;
#define in_rpcAck(buf, field)     (((msgRpcAckType *)(buf))->field)
////////

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
		word			optyp :8; // opcode now, but there should be just single 'relay'
        word            opref :8;
		word			len   :8; // I don't think we can trust RF packet's length (always even?)
		word			spare :8;
} msgFwdType;
#define in_fwd(buf, field)   (((msgFwdType *)(buf))->field)

typedef struct msgFwdAckStruct {
        headerType      header;
		word			optyp :8; // opcode now, but there should be just a single (typed within) 'relay'
        word            opref :8;
} msgFwdAckType;
#define in_fwdAck(buf, field)   (((msgFwdAckType *)(buf))->field)

typedef struct msgNhStruct {
        headerType      header;
		word			rsvp;
		word			ref :8;
        word            spare :8;
} msgNhType;
#define in_nh(buf, field)   (((msgNhType *)(buf))->field)

typedef struct msgNhAckStruct {
        headerType      header;
		word			ref :8;
        word            rss :8;
} msgNhAckType;
#define in_nhAck(buf, field)   (((msgNhAckType *)(buf))->field)

typedef struct msgRfidStruct {
        headerType      header;
		word			btyp :4;
		word			ttyp :4;
		word			next :8;
        word            cnt  :8;
		word			len  :8;
} msgRfidType;
#define in_rfid(buf, field)   (((msgRfidType *)(buf))->field)

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
word get_word (byte * buf, word n);
void set_pxopts (word n, word xp, word cav);
word get_pxopts ();

#endif
