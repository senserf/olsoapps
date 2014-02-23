/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2014                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

#ifndef BOARD_CHRONOS
#error CHRONOS only
#endif

#include "inout.h"
#include "looper.h"
#include "pong.h"
#include "chro.h"
#include "diag.h"
#include "variants.h"
#include "net.h"

static void init () {
        master_host = local_host; // I'm not sure what this is for...
        tarp_ctrl.param &= 0xFE; // routing off

	chro_init ();
	init_inout ();
	net_opt (PHYSOPT_RXOFF, NULL); // default is ON
	init_pframe();
        runfsm hear;
        runfsm looper;
	powerdown();

}

void process_incoming (char * buf, word size, word rssi) {

  switch (in_header(buf, msg_type)) {

	case msg_pongAck:
		if (in_pongAck(buf, dupeq) == 
				in_pong(pong_frame, pd).dupeq)
			trigger (TRIG_ACK);
		else
			app_diag_W ("Alien ack %u", in_pongAck(buf, dupeq));
		break;

	default:
		app_diag_D ("Msg %u fr %u (%u %u)", in_header(buf, msg_type),
			in_header(buf, snd), size, rssi);
  }

}

fsm root {

	entry RS_INIT:

#ifdef __SMURPH__
		delay (rnd() % 5000, RS_INIT1);
		release;
#endif

	entry RS_INIT1:
		init();
		finish;
}

