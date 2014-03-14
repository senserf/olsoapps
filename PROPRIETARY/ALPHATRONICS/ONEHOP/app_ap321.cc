/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2014                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "netid.h"
#include "tcvphys.h"
#include "phys_cc1100.h"
#include "phys_uart.h"
#include "plug_null.h"

#ifndef	__SMURPH__
#include "cc1100.h"
#endif

// ============================================================================

sint	sfd = -1, sfu = -1;

fsm rreader {
//
// Reads and absorbs stuff arriving on the UART without any interpretation
//
	state RD_LOOP:

		address pkt = tcv_rnp (RD_LOOP, sfu);
		tcv_endp (pkt);
		proceed RD_LOOP;
}

fsm root {

	word hst, tst;
	byte rss, but, add, vlt, glb, xpw;

	state RS_INIT:

		word par;

		phys_cc1100 (0, CC1100_MAXPLEN);
		phys_uart (1, MAX_RENESAS_MESSAGE_LENGTH, 0);
		tcv_plug (0, &plug_null);
		sfd = tcv_open (WNONE, 0, 0);
		sfu = tcv_open (WNONE, 1, 0);
		if (sfd < 0)
			syserror (ENODEVICE, "RF!");
		if (sfu < 0)
			syserror (ENODEVICE, "UA!");
		par = NETWORK_ID;
		tcv_control (sfd, PHYSOPT_SETSID, &par);
		tcv_control (sfd, PHYSOPT_ON, NULL);
		leds (0, 2);
		leds (1, 2);
		leds (2, 2);
		runfsm rreader;

#if 0
	state FIRST_MSG:

		address pkt;

		// Send some dummy message to check if it works at all
		pkt = tcv_wnp (FIRST_MSG, sfu, 2);
		pkt [0] = 0x5555;
		tcv_endp (pkt);

	state FIRST_PKT:

		address pkt;

		pkt = tcv_wnp (FIRST_PKT, sfd, 8);
		pkt [1] = 0x5555;
		pkt [2] = 0xAAAA;
		tcv_endp (pkt);
#endif
		delay (2048, GO_AHEAD);
		release;

	state GO_AHEAD:

		leds (0, 0);
		leds (1, 0);
		leds (2, 0);
		proceed ACKNOWLEDGE;

	state WPACKET:

		word len;
		address pkt;

		pkt = tcv_rnp (WPACKET, sfd);
		len = tcv_left (pkt);

		// These are only event packets of fixed length
		// diag ("GOT PKT");

		if (len < 16 || pkt [2] != PKTYPE_BUTTON) {
			// Accept only these
			// diag ("BAD PKT");
			tcv_endp (pkt);
			proceed WPACKET;
		}

		// diag ("OK");

		rss = ((byte*)pkt) [len - 1];

		// The sender
		hst = pkt [1];

		// Event number (aka timestamp)
		tst = pkt [3];

		// Button
		but = (byte) (pkt [4] >> 8);

		// Address
		add = (byte) pkt [4];

		// Voltage
		if (pkt [5] < 1000)
			pkt [5] = 1000;

		vlt = (byte)((pkt [5] - 1000) >> 3);

		// XMIT power
		xpw = (byte)(pkt [6]);

		// Local/global
		glb = (but & 0x80) >> 7;
		but &= 0x7F;

		tcv_endp (pkt);

	state ACKNOWLEDGE:

		address pkt;

		pkt = tcv_wnp (ACKNOWLEDGE, sfd, 10);

		pkt [1] = hst;
		pkt [2] = PKTYPE_ACK;
		pkt [3] = tst;

		tcv_endp (pkt);

	state SRENESAS:

		byte *pkt;

		pkt = (byte*) tcv_wnp (SRENESAS, sfu, 13);

		pkt [ 0] = 0x01;
		pkt [ 1] = 16;
		((address) pkt) [1] = host_id;
		((address) pkt) [2] = hst;
		pkt [ 6] = but;
		pkt [ 7] = glb;
		pkt [ 8] = (byte) tst;
		pkt [ 9] = vlt;
		pkt [10] = rss;
		pkt [11] = xpw;
		pkt [12] = add;

		tcv_endp ((address)pkt);

		proceed WPACKET;
}
