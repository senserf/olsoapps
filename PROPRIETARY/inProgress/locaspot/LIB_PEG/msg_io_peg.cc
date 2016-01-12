/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2015                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "commons.h"
#include "diag.h"
#include "inout.h"
#include "tag_mgr.h"
#include "form.h"
#include "loca.h"

void msg_master_in (char * buf) {

	if (master_host != local_host)
		return;

	trigger (TRIG_MBEAC);
	app_diag_S ("master contra %u (%u)", in_header(buf, snd), (word)seconds());
}

void msg_ping_in (char * buf, word rssi) {
Clear:
	if (loca.id == 0) { // first elem
		loca.ts = seconds();
		loca.id = in_header(buf, snd);
		loca.ref = in_ping(buf, ref);
		loca.vec [in_ping(buf, slot)] = (word)rssi;
		return;
	}
	
	if (loca.id == in_header(buf, snd) && loca.ref == in_ping(buf, ref)) {
		loca.vec [in_ping(buf, slot)] = (word)rssi;
	} else {
		if (loca.id == in_header(buf, snd) || seconds() - loca.ts > LOCA_TOUT_PING) {
#if LOCA_TRAC
		diag ("LOCA clr ping %u %u %u", loca.id, in_header(buf, snd),
			(word)(seconds() - loca.ts));
#endif
		loca_out(YES);
		goto Clear;
		}
		// else another tag cut in early - ignore
	}
}

void msg_pong_in (char * buf, word rssi) {

	// 1s for hco and prox
	msgPongAckType  pong_ack = {{msg_pongAck,0,0,0,0,1,1,0}};

	// we've made ANY pong_in a loca watchdog
	if (loca.id != in_header(buf, snd) && loca.id != 0 && seconds() - loca.ts > LOCA_TOUT_PONG) {
#if LOCA_TRAC
		diag ("LOCA clr pong %u %u %u", loca.id, in_header(buf, snd),
			(word)(seconds() - loca.ts));
#endif
		loca_out(YES);
	}

	if (needs_ack (in_header(buf, snd), buf + sizeof(headerType), rssi)) {
		pong_ack.header.rcv = in_header(buf, snd);
		pong_ack.dupeq = in_pong(buf, pd).dupeq;
		talk ((char *)&pong_ack, sizeof(msgPongAckType), TO_NET);
	} else 
		in_pong(buf, pd).noack = 1;

	app_diag_D ("Pong %u", in_header(buf, snd));
	ins_tag (buf, rssi);
}

void msg_fwd_in (char * buf, word siz) {
	msgFwdAckType ack;
	if (in_fwd(buf, opref) & 0x80) {
		memset (&ack, 0, sizeof(msgFwdAckType));
		ack.header.msg_type = msg_fwdAck;
		ack.header.rcv = in_header(buf, snd);
		ack.optyp = in_fwd(buf, optyp);
		ack.opref = in_fwd(buf, opref);
		talk ((char *)&ack, sizeof(msgFwdAckType), TO_NET);
	}
    talk (buf, siz, TO_OSS);
}

void msg_report_in (char * buf, word siz) {
	msgReportAckType ack = {{msg_reportAck,0,0,0,0,0,0,0}};

	// eliminate unnecessary RACKs for heartbeat 'alarms' (in 1.5, not in 1.0)
	// or alrms with location data
	if (((pongDataType *)(buf + sizeof(msgReportType)))->alrm_id == 0 ||
		((pongDataType *)(buf + sizeof(msgReportType)))->locat) {
		app_diag_D ("No RACK %u #%u %u %u", in_header(buf, snd), in_report(buf, ref),
		((pongDataType *)(buf + sizeof(msgReportType)))->alrm_id,
		((pongDataType *)(buf + sizeof(msgReportType)))->locat);
	} else {
		ack.header.rcv = in_header(buf, snd);
		ack.ref = in_report(buf, ref);
		ack.tagid = in_report(buf, tagid);
		talk ((char *)&ack, sizeof(msgReportAckType), TO_NET);
	}
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

