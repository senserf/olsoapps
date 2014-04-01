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

#define	SHOW_RENESAS_OUTPUT	1

// ============================================================================

sint	sfd = -1, sfu = -1;
word	rtries;
word	nodeid, master = MASTER_NODE_ID;

// ============================================================================

void sack (byte req, word add) {
//
// Send an ACK to RENESAS
//
	address pkt;

	if ((pkt = tcv_wnp (NULL, sfu, 5)) == NULL)
		return;

	((byte*)pkt) [0] = (0x80 | req);
	((byte*)pkt) [1] = 5 + 3;
	pkt [1] = add;
	((byte*)pkt) [4] = 0x06;

	tcv_endp (pkt);
}

fsm rreader {
//
// Reads and absorbs stuff arriving on the UART without any interpretation
//
	state RD_LOOP:

		address pkt;

#ifdef SHOW_RENESAS_OUTPUT
		address pkr;
		word len;
#endif
		pkt = tcv_rnp (RD_LOOP, sfu);

#ifdef SHOW_RENESAS_OUTPUT

		len = tcv_left (pkt);
#if 0
		pkr = tcv_wnp (WNONE, sfd, len + 8);
		if (pkr != NULL) {
			pkr [1] = host_id;
			pkr [2] = PKTYPE_RENESAS;
			memcpy (pkr + 3, pkt, len);
			tcv_endp (pkr);
		}
#endif
		if (len < 4)
			goto Ignore;

		if (pkt [1] != 0 && pkt [1] != nodeid)
			goto Ignore;

		switch (((byte*)pkt) [0]) {

		    case 0x81:

			// Assume ACK, can there be anything else
			if (((byte*)pkt) [4] == 0x06) {
				rtries = 0;
				trigger (&rtries);
			}
			break;

		    case 0x11:

			if (len < 6 || (pkr = tcv_wnp (NULL, sfu, 8)) == NULL)
				goto Ignore;

			((byte*)pkr) [0] = 0x91;
			((byte*)pkr) [1] = 11;

			pkr [1] = pkt [1];
			pkr [2] = pkt [2];
			pkr [3] = (pkt [2] == 0x0002) ? (nodeid == master) :
				nodeid;

			tcv_endp (pkr);
			break;

		    case 0x12:

			if (len < 8)
				goto Ignore;

			if (pkt [2] == 0x0001)
				nodeid = pkt [3];
			else
				master = pkt [3];

			sack (0x12, pkt [1]);
		}
Ignore:
#endif
		tcv_endp (pkt);
		proceed RD_LOOP;
}

fsm root {

	word hst, tst;
	byte rss, but, add, vlt, glb, xpw;

	state RS_INIT:

		word par;

		nodeid = HOSTID;

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

		rtries = RNTRIES;

	state SRENESAS:

		byte *pkt;

		pkt = (byte*) tcv_wnp (SRENESAS, sfu, 16);

		pkt [ 0] = 0x01;
		pkt [ 1] = 19;
		((address) pkt) [1] = host_id;
		((address) pkt) [2] = host_id;
		((address) pkt) [3] = hst;
		pkt [ 8] = but;
		pkt [ 9] = glb;
		pkt [10] = (byte) tst;
		pkt [11] = vlt;
		pkt [12] = rss;
		pkt [13] = xpw;
		pkt [14] = add;
		pkt [15] = 1;		// Dummy age

		tcv_endp ((address)pkt);

	state WRACK:

		if (rtries) {
			when (&rtries, WRACK);
			delay (32, RRETRY);
			release;
		}

		proceed WPACKET;

	state RRETRY:

		if (!rtries)
			proceed WPACKET;

		rtries--;
		sameas SRENESAS;
}
