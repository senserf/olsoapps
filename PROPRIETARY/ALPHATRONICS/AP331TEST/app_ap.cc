/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/
#include "sysio.h"
#include "phys_cc1100.h"
#include "plug_null.h"
#include "ser.h"
#include "serf.h"
#include "form.h"
#include "blink.h"

#include "netid.h"

sint rfc;

fsm ping_sender {

	word np;

	state PS_LOOP:

		address pkt;

		ser_outf (PS_LOOP, "ping %u\r\n", np);
		blink (1, 1, 64, 0, 128);

		if ((pkt = tcv_wnp (WNONE, rfc, 6)) != NULL) {
			pkt [1] = PING_MAGIC;
			tcv_endp (pkt);
			delay (PING_INTERVAL, PS_LOOP);
			np++;
		} else {
			delay (PING_INTERVAL/4, PS_LOOP);
		}

}

fsm radio_receiver {

	address pkt;
	char h;

	state RR_WAIT:

		pkt = tcv_rnp (RR_WAIT, rfc);

		if (tcv_left (pkt) < 18)
			goto Ignore;

		if (pkt [1] == REP_MAGIC) {

			// Report indication LED
			blink (1, 1, 128, 0, 32);

			if (pkt [2] < 3)
				blink (0, 3, 128, 128, 128);

			if (pkt [3] < 3)
			blink (2, 3, 128, 128, 128);
	
			if (pkt [4] < 3)
				blink (2, 1, 1024, 0, 512);
			h = 'R';

		} else if (pkt [1] == STA_MAGIC) {

			blink (1, 3, 128, 128, 64);
			h = 'D';
		} else
			goto Ignore;



	state RR_SHOW:

		ser_outf (RR_SHOW, "%c: %x %x %x %x %x %x\r\n",
			h,
			pkt [2],
			pkt [3],
			pkt [4],
			pkt [5],
			pkt [6],
			pkt [7]);
Ignore:
		tcv_endp (pkt);

		sameas RR_WAIT;
}

fsm root {

	state RS_INIT:

		word sid;

		phys_cc1100 (0, MAX_PACKET_LENGTH);
		tcv_plug (0, &plug_null);
		rfc = tcv_open (WNONE, 0, 0);
		sid = NETID;
		tcv_control (rfc, PHYSOPT_SETSID, &sid);
		tcv_control (rfc, PHYSOPT_RXON, NULL);

		runfsm radio_receiver;
		runfsm ping_sender;

		blink (0, 4, 512, 512, 2048);

		finish;
}
