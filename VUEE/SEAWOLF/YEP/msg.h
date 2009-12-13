#ifndef	__msg_h__
#define	__msg_h__

#include "vuee.h"

#include "tarp.h"
//+++ "msg.c"

#include "msg_types.h"

#define in_beac(buf, field)	(((msgBeacType *)(buf))->field)
#define in_act(buf, field)	(((msgActType *)(buf))->field)
#define in_ad(buf, field)	(((msgAdType *)(buf))->field)
#define in_adAck(buf, field)	(((msgAdAckType *)(buf))->field)

#ifdef	__SMURPH__

#define	myBeac		_daprx (myBeac)
#define	myAct		_daprx (myAct)

threadhdr (beacon, SeaNode) {

	states { MSG_BEA_START, MSG_BEA_SEND };

	perform;
};

threadhdr (rcv, SeaNode) {

	states { MSG_RC_TRY };

	perform;
};

#else

procname (beacon);
procname (rcv);

// nothing else we do but beacons and acts - keep them ready
extern msgBeacType myBeac;
extern msgActType  myAct;

// Expected by NET and TARP----------
// In VUEE, they are node methods ---
int tr_offset (headerType*);
Boolean msg_isBind (msg_t);
Boolean msg_isTrace (msg_t);
Boolean msg_isMaster (msg_t);
Boolean msg_isNew (msg_t);
Boolean msg_isClear (byte);
void set_master_chg (void);
//------------------------------------

#endif

int msg_reply (word);
int msg_send (msg_t, nid_t, hop_t, word, word, word, char *);

#endif
