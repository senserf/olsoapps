/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#ifndef	__msg_h__
#define	__msg_h__

//+++ "msg.c"

#include "msg_types.h"

#define in_beac(buf, field)	(((msgBeacType *)(buf))->field)
#define in_act(buf, field)	(((msgActType *)(buf))->field)
#define in_ad(buf, field)	(((msgAdType *)(buf))->field)
#define in_adAck(buf, field)	(((msgAdAckType *)(buf))->field)

fsm beacon;
fsm rcv;

// nothing else we do but beacons and acts - keep them ready
extern msgBeacType myBeac;
extern msgActType  myAct;

// Expected by NET and TARP----------
int tr_offset (headerType*);
Boolean msg_isBind (msg_t);
Boolean msg_isTrace (msg_t);
Boolean msg_isMaster (msg_t);
Boolean msg_isNew (msg_t);
Boolean msg_isClear (byte);
void set_master_chg ();
//------------------------------------

typedef word hop_t;

int msg_reply (word);
int msg_send (msg_t, nid_t, hop_t, word, word, word, char *);

#endif
