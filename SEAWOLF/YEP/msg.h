#ifndef	__msg_h__
#define	__msg_h__

#include "tarp.h"
//+++ "msg.c"

#define MSG_BEAC	1
#define MSG_ACT		2
#define MSG_AD		3
#define MSG_ADACK	4

typedef struct msgBeacStruct {
	headerType	header;
} msgBeacType;

#define in_beac(buf, field)	(((msgBeacType *)(buf))->field)

typedef struct msgActStruct {
	headerType      header;
	word		act :8;
	word		ref :8; // likely not needed: hshake's prev. act
} msgActType;

#define in_act(buf, field)	(((msgActType *)(buf))->field)

typedef struct msgAdStruct {
	headerType      header;
	word		ref :8; // may be many ads
	word		lev :3;
	word		len :5;
} msgAdType;

#define in_ad(buf, field)	(((msgAdType *)(buf))->field)

typedef struct msgAdAckStruct {
	headerType      header;
	word            ref :8;
	word		ack :8;
} msgAdAckType;

#define in_adAck(buf, field)	(((msgAdAckType *)(buf))->field)

// nothing else we do but beacons and acts - keep them ready
extern msgBeacType myBeac;
extern msgActType  myAct;

int msg_reply (word);
int msg_send (msg_t, nid_t, hop_t, word, word, word, char *);

// Expected by NET and TARP----------
int tr_offset (headerType*);
Boolean msg_isBind (msg_t);
Boolean msg_isTrace (msg_t);
Boolean msg_isMaster (msg_t);
Boolean msg_isNew (msg_t);
Boolean msg_isClear (byte);
void set_master_chg (void);
//------------------------------------
#endif

