/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "globals.h"
#include "threadhdrs.h"

#define	MIN_PACKET_LENGTH	24
#define	MAX_PACKET_LENGTH	46

#define	IBUFLEN		82

#define	PKT_ACK		0x1234
#define	PKT_DAT		0xABCD

#define	ACK_LENGTH	12
#define MAXPLEN		(MAX_PACKET_LENGTH + 2)

static word gen_packet_length (void) {

#if MIN_PACKET_LENGTH >= MAX_PACKET_LENGTH
	return MIN_PACKET_LENGTH;
#else
	return ((rnd () % (MAX_PACKET_LENGTH - MIN_PACKET_LENGTH + 1)) +
			MIN_PACKET_LENGTH) & 0xFFE;
#endif

}

thread (receiver)

    entry (RC_TRY)

	if (rkillflag) {
		rkillflag = NO;
		RCVon = NO;
		finish;
	}

	when (&rkillflag, RC_TRY);
	r_packet = tcv_rnp (RC_TRY, sfd);

	if (r_packet [1] == PKT_ACK) {
		last_ack = ntowl (((lword*)r_packet) [1]);
		proceed (RC_ACK);
	}

	// Data packet
	last_rcv = ntowl (((lword*)r_packet) [1]);

    entry (RC_DATA)

	ser_outf (RC_DATA, "RCV: [%x] %lu (len = %d), pow = %d qua = %d\r\n",
		r_packet [1],
		last_rcv, tcv_left (r_packet) - 2,
		((byte*)r_packet) [tcv_left (r_packet) - 1],
		((byte*)r_packet) [tcv_left (r_packet) - 2]
	);

	tcv_endp (r_packet);

	// Acknowledge it

    entry (RC_SACK)

	if (XMTon) {
		r_packet = tcv_wnp (RC_SACK, sfd, ACK_LENGTH);
		r_packet [0] = 0;
		r_packet [1] = PKT_ACK;
		((lword*)r_packet) [1] = wtonl (last_rcv);

		r_packet [4] = (word) (entropy);
		tcv_endp (r_packet);
	}
	proceed (RC_TRY);

    entry (RC_ACK)

	ser_outf (RC_ACK, "ACK: %lu (len = %d)\r\n", last_ack,
		tcv_left (r_packet));

	tcv_endp (r_packet);
	trigger (&last_ack);
	proceed (RC_TRY);

endthread

__PUBLF (Node, int, rcv_start) () {

	rkillflag = 0;
	tcv_control (sfd, PHYSOPT_RXON, NULL);
	if (!RCVon) {
		runthread (receiver);
		RCVon = YES;
		return 1;
	}
	return 0;
}

__PUBLF (Node, int, rcv_stop) () {

	if (RCVon) {
		tcv_control (sfd, PHYSOPT_RXOFF, NULL);
		rkillflag = YES;
		trigger (&rkillflag);
		return 1;
	}

	return 0;
}

strand (sender, word)

    word pl;
    int  pp;

#define tdelay	((word)data)

    entry (SN_SEND)

	if (tkillflag) {
Finish:
		tkillflag = 0;
		XMTon = NO;
		finish;
	}
	when (&tkillflag, SN_SEND);

	if (last_ack != last_snt) {
		delay (tdelay, SN_NEXT);
		when (&last_ack, SN_SEND);
		release;
	}

	last_snt++;

	packet_length = gen_packet_length ();

	if (packet_length < 10)
		packet_length = 10;
	else if (packet_length > MAX_PACKET_LENGTH)
		packet_length = MAX_PACKET_LENGTH;

	proceed (SN_NEXT);

    entry (SN_NEXT)

	if (tkillflag)
		goto Finish;

	when (&tkillflag, SN_SEND);

	x_packet = tcv_wnp (SN_NEXT, sfd, packet_length + 2);

	x_packet [0] = 0;
	x_packet [1] = PKT_DAT;

	// In words
	pl = packet_length / 2;
	((lword*)x_packet)[1] = wtonl (last_snt);

	for (pp = 4; pp < pl; pp++)
		x_packet [pp] = (word) (entropy);

	tcv_endp (x_packet);

    entry (SN_NEXT1)

	ser_outf (SN_NEXT1, "SND %lu, len = %d\r\n", last_snt,
		packet_length);
	proceed (SN_SEND);

endstrand

__PUBLF (Node, int, snd_start) (int del) {

	tkillflag = NO;
	last_ack = last_snt;
	tcv_control (sfd, PHYSOPT_TXON, NULL);

	if (!XMTon) {
		runstrand (sender, del);
		XMTon = 1;
		return 1;
	}

	return 0;
}

__PUBLF (Node, int, snd_stop) () {

	tcv_control (sfd, PHYSOPT_TXOFF, NULL);
	if (XMTon) {
		tkillflag = YES;
		trigger (&tkillflag);
		return 1;
	}
	return 0;
}

thread (root)

    entry (RS_INIT)

	ibuf = (char*) umalloc (IBUFLEN);
	ibuf [0] = 0;

	phys_cc1100 (0, MAXPLEN);
	// WARNING: the SMURPH model assumes that the plugin is static, i.e.,
	// all nodes use the same plugin. This is easy to change later, but
	// ... for now ...
	// Thus, only the first tcv_plug operation is effective, and the
	// subsequent ones (with the same ordinal) will be ignored. It is
	// checked, however, whether the plugin is the same in all their
	// instances.
	tcv_plug (0, &plug_null);
	sfd = tcv_open (WNONE, 0, 0);
	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

    entry (RS_RCMDM2)

	ser_out (RS_RCMDM2,
		"\r\nRF Ping Test\r\n"
		"Commands:\r\n"
		"s intvl  -> start/reset sending interval (2 secs default)\r\n"
		"r        -> start receiver\r\n"
		"d i v    -> change phys parameter i to v\r\n"
		"x p      -> set transmit power\r\n"
		"y        -> get current transmit power\r\n"
		"g        -> get received power\r\n"
		"o        -> stop receiver\r\n"
		"t        -> stop transmitter\r\n"
		"q        -> stop both\r\n"
		"i        -> set station Id\r\n"
		"z        -> reset the node\r\n"
		"p n      -> read pin n\r\n"
		"u n v    -> set pin n\r\n"
		"a n r d  -> read analog pin\r\n"
	);

    entry (RS_RCMDM1)

	if ((unsigned char) ibuf [0] == 0xff)
		ser_out (RS_RCMDM1,
			"No command in 10 seconds -> start s 1024, r\r\n"
				);
    entry (RS_RCMD)

	if ((unsigned char) ibuf [0] == 0xff)
		delay (1024*10, RS_AUTOSTART);
  
	k = ser_in (RS_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {
	    case 's': proceed (RS_SND);
	    case 'r': proceed (RS_RCV);
	    case 'd': proceed (RS_PAR);
	    case 'q': proceed (RS_QUIT);
	    case 'o': proceed (RS_QRCV);
	    case 't': proceed (RS_QXMT);
	    case 'i': proceed (RS_SSID);
	    case 'z': proceed (RS_RES);
	    case 'p': proceed (RS_RPIN);
	    case 'u': proceed (RS_SPIN);
	    case 'a': proceed (RS_RANA);
	    case 'x': proceed (RS_SETP);
	    case 'y': proceed (RS_GETP);
	}

    entry (RS_RCMD1)

	ser_out (RS_RCMD1, "Illegal command or parameter\r\n");
	proceed (RS_RCMDM2);

    entry (RS_SND)

	/* Default */
	n1 = 2048;
	scan (ibuf + 1, "%d", &n1);
	if (n1 < 16)
		n1 = 16;
	snd_start (n1);

    entry (RS_SND1)

	ser_outf (RS_SND1, "Sender rate: %d\r\n", n1);
	proceed (RS_RCMD);

    entry (RS_RCV)

	rcv_start ();
	proceed (RS_RCMD);

    entry (RS_QRCV)

	rcv_stop ();
	proceed (RS_RCMD);

    entry (RS_QXMT)

	snd_stop ();
	proceed (RS_RCMD);

    entry (RS_QUIT)

	strcpy (obuf, "");
	if (rcv_stop ())
		strcat (obuf, "receiver");
	if (snd_stop ()) {
		if (strlen (obuf))
			strcat (obuf, " + ");
		strcat (obuf, "sender");
	}
	if (strlen (obuf))
		fmt = "Stopped: %s\r\n";
	else
		fmt = "No process stopped\r\n";

    entry (RS_QUIT1)

	ser_outf (RS_QUIT1, fmt, obuf);
	proceed (RS_RCMD);

    entry (RS_PAR)

	if (scan (ibuf + 1, "%u %u", p+0, p+1) < 2)
		proceed (RS_RCMD1);

	if (p [0] > 5)
		proceed (RS_RCMD1);

	tcv_control (sfd, PHYSOPT_SETPARAM, p);

    entry (RS_PAR1)

	ser_outf (RS_PAR1, "Parameter %u set to %u\r\n", p [0], p [1]);
	proceed (RS_RCMD);

    entry (RS_SSID)

	n1 = 0;
	scan (ibuf + 1, "%d", &n1);
	tcv_control (sfd, PHYSOPT_SETSID, (address) &n1);
	proceed (RS_RCMD);

    entry (RS_AUTOSTART)
	  
	snd_start (1024);
  	rcv_start ();
	finish;

    entry (RS_RES)

	reset ();
	// We should be killed past this

    entry (RS_RPIN)

	if (scan (ibuf + 1, "%u", p+0) < 1)
		proceed (RS_RCMD1);

	p [1] = pin_read (p [0]);

    entry (RS_RPIN1)

	ser_outf (RS_RPIN1, "P[%u] = %u\r\n", p [0], p [1]);
	proceed (RS_RCMD);

    entry (RS_SPIN)

	if (scan (ibuf + 1, "%u %u", p+0, p+1) < 2)
		proceed (RS_RCMD1);

	pin_write (p [0], p [1]);
	proceed (RS_RCMD);

    entry (RS_RANA)

	if (scan (ibuf + 1, "%u %u %u", p+0, &k, p+1) < 3)
		proceed (RS_RCMD1);

    entry (RS_RANA1)

	n1 = pin_read_adc (RS_RANA1, p [0], k, p [1]);

    entry (RS_RANA2)

	ser_outf (RS_RANA2, "A[%u] = %d\r\n", p [0], n1);
	proceed (RS_RCMD);

    entry (RS_SETP)

	if (scan (ibuf + 1, "%u", p+0) < 1)
		proceed (RS_RCMD1);

	tcv_control (sfd, PHYSOPT_SETPOWER, p);
	proceed (RS_RCMD);

    entry (RS_GETP)

	ser_outf (RS_GETP, "P = %d\r\n",
		tcv_control (sfd, PHYSOPT_GETPOWER, NULL));
	proceed (RS_RCMD);

endthread

praxis_starter (Node);
