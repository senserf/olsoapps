/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "globals.h"
#include "threadhdrs.h"

thread (receiver)

    lword pn;
    address packet;

    entry (RC_TRY)

	packet = tcv_rnp (RC_TRY, sfd);

	if (receiving == NO) {
		// Drop it. The receiver is always on.
		tcv_endp (packet);
		proceed (RC_TRY);
	}
	r_sn = packet [1];
	if (r_sn >= MAX_PEER_COUNT) {
		// Illegal peer number, drop the packet
		tcv_endp (packet);
		proceed (RC_TRY);
	}
	// Serial number
	pn = ((lword*)packet) [1];
	received [r_sn] ++;
	if (last_rcv [r_sn] == LWNONE) {
		// Starting up
		last_rcv [r_sn] = pn;
	} else {
		if (++last_rcv [r_sn] != pn) {
			// Lost
			lost [r_sn] += pn - last_rcv [r_sn];
			last_rcv [r_sn] = pn;
		}
	}
	r_pl = tcv_left (packet) - 2;
	// RSSI
	r_sl = ((byte*)packet) [r_pl + 1];
	tcv_endp (packet);

    entry (RC_OUT)

	ser_outf (RC_OUT, "RCV: [%u] %u <%lu> %u\r\n", r_sn, r_pl,
		last_rcv [r_sn], r_sl);

	proceed (RC_TRY);

endthread

thread (sender)

    address packet;
    word i;

    entry (SN_SEND)

	packet = tcv_wnp (SN_SEND, sfd, packet_length + 2);
	packet [1] = NODE_ID;
	((lword*)packet) [1] = last_snt++;

	for (i = 4; i < packet_length/2; i++)
		packet [i] = i;

	tcv_endp (packet);

    entry (SN_OUT)

	ser_outf (SN_OUT, "SNT: %u <%lu>\r\n", packet_length, last_snt - 1);
	delay (send_interval, SN_SEND);

endthread

thread (root)

    word n;

    entry (RS_INIT)

	t_ibuf = (char*) umalloc (IBUFLEN);
	t_ibuf [0] = 0;

	phys_cc1100 (0, MAX_PACKET_LENGTH);
	tcv_plug (0, &plug_null);
	sfd = tcv_open (WNONE, 0, 0);
	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}
	// The receiver is always on
	tcv_control (sfd, PHYSOPT_RXON, NULL);
	runthread (receiver);
	for (n = 0; n < MAX_PEER_COUNT; n++) {
		last_rcv [n] = LWNONE;
		lost [n] = 0;
		received [n] = 0;
	}
	packet_length = 24;

    entry (RS_MENU)

	ser_out (RS_MENU,
		"\r\nRF Loss Test\r\n"
		"Commands:\r\n"
		"l n      -> set packet length (24 bytes default)\r\n"
		"s intvl  -> start/reset sending interval (2 secs default)\r\n"
		"r        -> start receiver (reset count)\r\n"
		"x p      -> set transmit power\r\n"
		"y        -> get current transmit power\r\n"
		"q        -> stop\r\n"
		"i        -> set station Id\r\n"
		"c        -> view counters\r\n"
	);

    entry (RS_RCMD)

	t_ibuf [0] = '\0';
	ser_in (RS_RCMD, t_ibuf, IBUFLEN-1);
	n = 0;
	switch (t_ibuf [0]) {
	    case 'l' :
		scan (t_ibuf + 1, "%u", &n);
		n = (n + 1) & ~1;
		if (n < MIN_PACKET_LENGTH || n > MAX_PACKET_LENGTH)
			proceed (RS_MENU);
		packet_length = n;
		break;
	    case 's' :
		scan (t_ibuf + 1, "%u", &n);
		if (n == 0)
			n = 1024;
		else if (n < 64)
			n = 64;
		send_interval = n;
		if (!running (sender)) {
			last_snt = 0;
			runthread (sender);
			tcv_control (sfd, PHYSOPT_TXON, NULL);
		}
		break;
	    case 'r' :
		for (n = 0; n < MAX_PEER_COUNT; n++) {
			last_rcv [n] = LWNONE;
			lost [n] = 0;
			received [n] = 0;
		}
		receiving = YES;
		proceed (RS_OK);
	    case 'p' :
		tcv_control (sfd, PHYSOPT_SETPOWER, &n);
		break;
	    case 'y' :
		proceed (RS_SP);
	    case 'q' :
		receiving = NO;
		killall (sender);
		// tcv_control (sfd, PHYSOPT_TXOFF, NULL);
		break;
	    case 'i' :
		tcv_control (sfd, PHYSOPT_SETSID, &n);
		break;
	    case 'c' :
		t_i = 0;
		proceed (RS_CNT);
	    default:
		proceed (RS_MENU);
	}

    entry (RS_OK)

	ser_out (RS_OK, "OK\r\n");
	proceed (RS_RCMD);

    entry (RS_SP)

	ser_outf (RS_SP, "P = %u\r\n", tcv_control (sfd, PHYSOPT_GETPOWER,
		NULL));
	proceed (RS_RCMD);

    entry (RS_CNT)

	if (t_i == MAX_PEER_COUNT)
		proceed (RS_OK);

	if (last_rcv [t_i] != LWNONE) {
		ser_outf (RS_CNT, " -> %u : RCV %lu, LOST %lu\r\n", t_i,
			received [t_i], lost [t_i]);
	}

	t_i++;
	proceed (RS_CNT);
		
endthread

praxis_starter (Node);
