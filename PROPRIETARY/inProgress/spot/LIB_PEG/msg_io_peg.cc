/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2014                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "commons.h"
#include "diag.h"
#include "inout.h"
#include "tag_mgr.h"
#include "form.h"

void msg_pong_in (char * buf, word rssi) {

	// 1s for hco and prox
	msgPongAckType  pong_ack = {{msg_pongAck,0,0,0,0,1,1,0}};

	if (needs_ack (in_header(buf, snd), buf + sizeof(headerType))) {
		pong_ack.header.rcv = in_header(buf, snd);
		pong_ack.dupeq = in_pong(buf, pd).dupeq;
		talk ((char *)&pong_ack, sizeof(msgPongAckType), TO_NET);
	} else 
		in_pong(buf, pd).noack = 1;

	app_diag_D ("Pong %u", in_header(buf, snd));
	ins_tag (buf, rssi);
}

void msg_fwd_in (char * buf, word siz) {
	msgFwdAckType ack = {{msg_fwdAck,0,0,0,0,0,0,0}};
        ack.header.rcv = in_header(buf, snd);
        ack.ref = in_fwd(buf, ref);
        talk ((char *)&ack, sizeof(msgFwdAckType), TO_NET);
        talk (buf, siz, TO_OSS);
}

void msg_report_in (char * buf, word siz) {

	msgReportAckType ack = {{msg_reportAck,0,0,0,0,0,0,0}};
	ack.header.rcv = in_header(buf, snd);
	ack.ref = in_report(buf, ref);
	ack.tagid = in_report(buf, tagid);
	talk ((char *)&ack, sizeof(msgReportAckType), TO_NET);
	talk (buf, siz, TO_OSS);
}

void msg_reportAck_in (char * buf) {
	char * b;

    if (del_tag (in_reportAck(buf, tagid), in_reportAck(buf, ref), 
		0, YES) > 1) {
	b = form (NULL, "Stale RAck %u #%u\r\n", 
		in_reportAck(buf, tagid), in_reportAck(buf, ref));

	if (b)
		talk (b, MAX_WORD, TO_OSS);

    } else {

	talk (buf, 77, TO_OSS); // note we get away with the lucky size

    }
}

