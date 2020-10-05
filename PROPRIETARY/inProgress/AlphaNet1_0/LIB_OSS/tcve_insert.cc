/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "plug_null.h"

///////////////// oss out ////////////////
// this is consistent with ser oss but I think it would be only needed for
// 'immediate acks' and retries. Acks that require context (and have longer
// and non-blocking retries) would need additional buffer for inventory and
// audit.
/////////////////////////////////////////////////////////////////
typedef struct osyncStruct {
	word type :8;
	word spare :8;
} osync_t;
static osync_t osync = {0,0};

static fifek_t oss_cb;
static sint oss_fd;
static char * urgent = NULL;

// if we compile with the 2 #ifs switched off, we'll be overwriting circ. buf;
// i.e. old lines will be lost (instead of new ones).
static word _oss_out (char * b, Boolean urg) {

	if (b == NULL)
		return 2;
		
	if (urg) {
		if (urgent) {
			app_diag_S ("urg owrite %x", urgent);
			ufree (urgent);
		}
		urgent = b;
		trigger (TRIG_OSSO);
		return 0;
	}
	
#if 1
	if (!fifek_full (&oss_cb)) {
#endif
		fifek_push (&oss_cb, b);
		trigger (TRIG_OSSO);
		return 0;
#if 1
	}
	// we (arbitrary) decide: don't overwrite
	app_diag_S ("OSS oflow (%u)", (word)seconds());
	ufree (b);
	return 1;
#endif
}
 
fsm perp_oss_out () {
	char * ptr;
	address pkt;
	word rtr;

	state CHECK:
		if (urgent == NULL && fifek_empty (&oss_cb)) {
			when (TRIG_OSSO, CHECK);
			release;
		}
		if (urgent)
			ptr = urgent;
		else
			ptr = fifek_pull (&oss_cb);
		
		if (ptr[1] < 5) { // wrong len
			app_diag_S ("OSSO len %d", (sint)ptr[1]);
			ufree (ptr);
			if (urgent)
				urgent = NULL;
			proceed CHECK;
		}
		
	state WNP:
		if (urgent) {
			pkt = tcv_wnpu (WNP, oss_fd, (sint)ptr[1] -3);
			urgent = NULL;
		} else {
			pkt = tcv_wnp (WNP, oss_fd, (sint)ptr[1] -3);
		}
		
		if (ptr[0] & 0x80) { // we do NOT expect acks / no retries
			rtr = 0;
			osync.type = 0;
		} else {
			rtr = 3; // 1 would be no retries but with a pause for ack
			osync.type = ptr[0];
		}
		
OutCrap:
		memcpy ((char *)pkt, ptr, (sint)ptr[1] -3);
		tcv_endp (pkt);
		
		if (rtr == 0)
			sameas DONE;
			
		when (TRIG_OSSIN, DONE);
#ifdef __SMURPH__
		delay (500, --rtr > 0 ? RTRY : NOACK);
#else
		delay (50, --rtr > 0 ? RTRY : NOACK);
#endif
		release;
		
	state RTRY:
		pkt = tcv_wnpu (RTRY, oss_fd, (sint)ptr[1] -3);
		goto OutCrap;
		
	state NOACK:
		app_diag_W ("NoAck %x", ((address)ptr)[0]);
		// fall through
	state DONE:
		ufree (ptr);
		osync.type = 0;
		proceed (CHECK);
}

/////////////////

fsm cmd_in;
void oss_ini () {

	phys_uart (1, 80, 0); // 80B, not used anywhere else(?)
	tcv_plug (1, &plug_null); // tarp is 0
	
	// we should be returning, the caller blinking, whatever
	if ((oss_fd = tcv_open (WNONE, 1, 1)) < 0) { // last arg, is the (null) plugin
		app_diag_F ("TCVE error");
		reset();
	}
	
	fifek_ini (&oss_cb, 15);
	if (!running (perp_oss_out))
		runfsm perp_oss_out;

	if (!running (cmd_in))
		runfsm cmd_in;
}

