/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012        			*/
/* All rights reserved.							*/
/* ==================================================================== */

#include "app.h"
#include "app_dcl.h"
#include "msg_dcl.h"
#include "oss_dcl.h"
#include "net.h"

char * get_mem (word state, word len) {
	char * buf = (char *)umalloc (len);
	if (buf == NULL) {
		diag ("RAM out");

		if (state != WNONE) {
			umwait (state);
			release;
		}
#if 0
	       	else
			reset(); // is THIS good for ANY praxis?
#endif	
	} else
		memset (buf, 0, len); // in most cases, this is needed

	return buf;
}

void init () {
	word pl = 7; // DEF_PLEV

	master_host = 0; // DEF_MHOST
	net_id =  12345; // DEF_NID
	// tarp_ctrl.param = 0xA3 in tarp.h; level 2, rec 2, slack 1, fwd on
	// inited in tarp

	local_host = (word)host_id;

	if (NODE_TYPE == 1) // CHRONOS
		app_flags.f.btyp = 1; // this we'll need
	app_flags.f.hb = NODE_TYPE;     // this may be useful(?)

	if (net_init (INFO_PHYS_CC1100, INFO_PLUG_TARP) < 0) {
		// diag issued from failing net_init diag ("net_init");
		leds (LED_R, LED_BLINK);
		halt(); // dupa will it keep blinking?
	}

	//dupa leds_all OFF needed? BOARD def?

	net_opt (PHYSOPT_SETSID, &net_id);

	if (app_flags.f.btyp == 1) {
		 net_opt (PHYSOPT_RXOFF, NULL);
		 app_flags.f.rx = 0;
	} else {
		net_opt (PHYSOPT_RXON, NULL);
		app_flags.f.rx = 1;
	}
	net_opt (PHYSOPT_SETPOWER, &pl);
	runfsm rcv;
	runfsm ossi_init;
}

