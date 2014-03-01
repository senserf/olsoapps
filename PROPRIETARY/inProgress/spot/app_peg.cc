/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2014                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "commons.h"
#include "inout.h"
#include "looper.h"
#include "diag.h"
#include "tag_mgr.h"
#include "msg_io_peg.h"
#include "net.h"

fsm mbeacon;
static void init () {
	word pl = 7;
	master_host = DEF_MHOST;
	init_inout ();
	net_opt (PHYSOPT_SETPOWER, &pl);
	reset_tags();
	if (local_host == master_host) {
                tarp_ctrl.param = 0xB0; // level 2, rec 3, slack 0, fwd off
		runfsm mbeacon;
		tagList.block = YES;
	} else {
                tarp_ctrl.param = 0xB1; // level 2, rec 3, slack 0, fwd on
		runfsm looper;
	}
	runfsm hear;
}

fsm mbeacon {
	msgMasterType mf;
	word tmp; // use hold if long pauses become a feature

    state INI:	
	tmp = 0;
	memset (&mf, 0, sizeof(msgMasterType));
	mf.header.msg_type = msg_master;
	highlight_set (0, 0.0, NULL);

    state SEND:
	tmp = 0;
	talk ((char *)&mf, sizeof(msgMasterType), TO_NET);

    state DEL:
	if ((++tmp % 10) == 0)
		proceed SEND;
	when (TRIG_MBEAC, SEND);
	delay ((58 + (rnd() % 5)) << 10, DEL); // 60 +/- 2s
	release;

}

void process_incoming (char * buf, word size, word rssi) {

  switch (in_header(buf, msg_type)) {

	case msg_pong:
		if (tagList.block == NO)
			msg_pong_in (buf, rssi);
		return;

	case msg_report:
		msg_report_in (buf, size);
		return;

	case msg_reportAck:
		msg_reportAck_in (buf);
		return;

	case msg_fwd:
		msg_fwd_in (buf, size);
		return;

	case msg_fwdAck:
		talk (buf, size, TO_OSS);
		return;

	case msg_master:
		// handled in TARP, nothing more to do
		return;

	default:
		app_diag_S ("Got ?(%u)", in_header(buf, msg_type));

  }
}

// ==========================================================================

fsm root {

	state START:
#ifdef __SMURPH__
                // spread a bit synced node starts
                delay (rnd() % 1000, INIT);
                release;
#endif
	state INIT:
		init ();
		finish;
}

