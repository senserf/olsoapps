/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2013                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "netid.h"
#include "tcvphys.h"
#include "ser.h"
#include "serf.h"
#include "form.h"
#include "phys_cc1100.h"
#include "cc1100.h"
#include "plug_null.h"

// ============================================================================

sint	sfd = -1;

fsm root {

	word hst, sta, tem, xco, yco, zco;
	byte rss, qua;

	state RS_INIT:

		word par;

		phys_cc1100 (0, CC1100_MAXPLEN);
		tcv_plug (0, &plug_null);
		sfd = tcv_open (WNONE, 0, 0);
		if (sfd < 0) {
			diag ("Cannot open RF interface");
			halt ();
		}
		par = NETWORK_ID;
		tcv_control (sfd, PHYSOPT_SETSID, &par);
		tcv_control (sfd, PHYSOPT_ON, NULL);
		leds (0, 2);
		leds (1, 2);
		leds (2, 2);
		delay (3072, GO_AHEAD);
		release;

	state GO_AHEAD:

		leds (0, 0);
		leds (1, 0);
		leds (2, 0);

	state WPACKET:

		word len;
		address pkt;

		pkt = tcv_rnp (WPACKET, sfd);
		len = tcv_left (pkt);

		if (len < 6)
			goto Drop;

		rss = ((byte*)pkt) [len - 1];
		qua = ((byte*)pkt) [len - 2];

		// The sender
		hst = pkt [1];

		if (pkt [2] == PKTYPE_WAKE) {

			if (len < 8)
				goto Drop;

			sta = pkt [3];
			tcv_endp (pkt);
			proceed HANDLE_WAKE;
		}

		if (pkt [2] == PKTYPE_MOTION) {

			if (len < 18)
				goto Drop;

			sta = pkt [3];
			tem = pkt [4];
			xco = pkt [5];
			yco = pkt [6];
			zco = pkt [7];
			tcv_endp (pkt);
			proceed HANDLE_MOTION;
		}

		if (pkt [2] == PKTYPE_BUTTON) {

			if (len < 14)
				goto Drop;

			sta = pkt [3];
			tem = pkt [4];
			xco = pkt [5];
			tcv_endp (pkt);
			proceed HANDLE_BUTTON;
		}
Drop:
		tcv_endp (pkt);
		proceed WPACKET;

	state HANDLE_WAKE:

		ser_outf (HANDLE_WAKE, "WAKE: host = %u, rss = %u, qua = %u, "
			"count = %u\r\n",
				hst, rss, qua, sta);

	state SEND_ACK:

		address pkt;

		pkt = tcv_wnp (SEND_ACK, sfd, 10);

		pkt [1] = hst;
		pkt [2] = PKTYPE_ACK;
		pkt [3] = sta;

		tcv_endp (pkt);
		proceed WPACKET;

	state HANDLE_MOTION:

		ser_outf (HANDLE_MOTION, "MOTION: host = %u, rss = %u, "
			"qua = %u, S=%x, T=%x, X=%d, Y=%d, Z=%d\r\n",
				hst, rss, qua, sta, tem, xco, yco, zco);
		proceed WPACKET;

	state HANDLE_BUTTON:

		ser_outf (HANDLE_BUTTON, "BUTTON: host = %u, rss = %u, "
			"qua = %u, count = %u, B=%x, S=%x\r\n",
				hst, rss, qua, sta, tem, xco);
		sameas SEND_ACK;
}
