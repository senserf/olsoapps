/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "msg.h"
#include "sealib.h"
#include "net.h"

msgBeacType	myBeac;
msgActType	myAct;

// ============================================================================

fsm beacon {

	state MSG_BEA_START:
		delay ((word)BEAC_FREQ * 1024 + rnd() % 2048, MSG_BEA_SEND);
		release;

	state MSG_BEA_SEND:
		handle_nbh (NBH_AUDIT, 0);
		// we'll be crying for all those shortcuts
		(void)net_tx (WNONE, (char *)&myBeac, sizeof(msgBeacType), 0);
		proceed MSG_BEA_START;
}

fsm rcv {

  state MSG_RC_TRY:
	if (rf_rcv.buf != NULL) {
		ufree (rf_rcv.buf);
		rf_rcv.buf = NULL;
		rf_rcv.len = 0;
	}

	if ((rf_rcv.len = net_rx (MSG_RC_TRY, &rf_rcv.buf, &rf_rcv.rss, 0)) <=
	    0)
		proceed MSG_RC_TRY;

	// map rssi
	if ((rf_rcv.rss >> 8) > 161)
		rf_rcv.rss = 3;
	else if ((rf_rcv.rss >> 8) > 140)
		rf_rcv.rss = 2;
	else
		rf_rcv.rss = 1;

	process_incoming ();
	proceed MSG_RC_TRY;

}

// ============================================================================

int msg_reply (word a) {

	if (in_header(rf_rcv.buf, msg_type) == MSG_AD) {
		in_header(rf_rcv.buf, msg_type) = MSG_ADACK;
		in_adAck(rf_rcv.buf, ack) = a;
	} else { // MSG_ACT
		in_act(rf_rcv.buf, ref) = in_act(rf_rcv.buf, act);
		in_act(rf_rcv.buf, act) = a;
	}
	in_header(rf_rcv.buf, rcv) = in_header(rf_rcv.buf, snd);
	in_header(rf_rcv.buf, hco) = 1;
	diag ("Rpl %u.%u to %u t%u", in_header (rf_rcv.buf, msg_type),
			in_header (rf_rcv.buf, seq_no),
			in_header (rf_rcv.buf, rcv), (word)seconds());
	return net_tx (WNONE, rf_rcv.buf, in_header(rf_rcv.buf, msg_type) ==
		       	MSG_AD ?  sizeof(msgActType) : sizeof(msgAdAckType), 0);
}

int msg_send (msg_t t, nid_t r, hop_t h, word pload,
		word lev, word len, char * ad) {

    char * mb;
    word ml;

    switch (t) {
	case MSG_ACT:
		ml = sizeof(msgActType);
		if ((mb = (char *)umalloc (ml)) == NULL)
			return -2;
		memset (mb, 0, ml);
		in_act(mb, act) = pload;
		break;

	case MSG_ADACK:
		ml = sizeof(msgAdAckType);
		if ((mb = (char *)umalloc (ml)) == NULL)
			return -2;
		memset (mb, 0, ml);
		in_adAck(mb, ack) = pload;
		break;

	case MSG_AD:
		ml = sizeof(msgAdType) + len;
		if ((mb = (char *)umalloc (ml)) == NULL)
			return -2;
		 memset (mb, 0, ml);
		 in_ad(mb, ref) = pload;
		 in_ad(mb, lev) = lev;
		 in_ad(mb, len) = len;
		 memcpy (mb + sizeof(msgAdType), ad, len);
		 break;

	default:
		 return -3;
    }
    in_header(mb, msg_type) = t;
    in_header(mb, rcv) = r;
    in_header(mb, hco) = h;
    diag ("Snd %u.%u to %u t%u", in_header(mb, msg_type), in_header(mb, seq_no),
		    in_header(mb, rcv), (word)seconds());
    return net_tx (WNONE, mb, ml, 0);
}

/*
 * "Virtual" stuff needed by NET & TARP =======================================
 */

int tr_offset (headerType *h) {
	return 0;
}

Boolean msg_isBind (msg_t m) {
	return NO;
}

Boolean msg_isTrace (msg_t m) {
	return NO;
}

Boolean msg_isMaster (msg_t m) {
	return (m == MSG_AD); // we'll see if mhopping makes sense at all
}

Boolean msg_isNew (msg_t m) {
	return NO;
}

Boolean msg_isClear (byte o) {
	return YES;
}

void set_master_chg () {
	return;
}
