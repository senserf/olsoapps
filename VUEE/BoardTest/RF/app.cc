/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "globals.h"
#include "threadhdrs.h"
#include "tcvplug.h"

#define	MIN_PACKET_LENGTH	24
#define	MAX_PACKET_LENGTH	46

#define	IBUFLEN		12

#define	PKT_ACK		0x1234
#define	PKT_DAT		0xABCD

#define MAXPLEN		(MAX_PACKET_LENGTH + 2)

#define RF_PLEV		1
#define RF_TOUT		15000
#define MSG_TOUT	2000
#define RPC_QUIT	666

#define EE_WRITES	10
#define GOOD_RUN	50

#define TS_INIT	0xFFFF
#define TS_EE   0xFFFE
#define TS_BUT	0xFFFC
#define TS_RF	0xFFF8
#define TS_DONE	0xFFF0

static word gen_packet_length (void) {

	return ((rnd () % (MAX_PACKET_LENGTH - MIN_PACKET_LENGTH + 1)) +
			MIN_PACKET_LENGTH) & 0xFFE;

}

thread (receiver)

	address	tmpack;
	word	rss;

    entry (RC_TRY)

	r_packet = tcv_rnp (RC_TRY, sfd);

	if (r_packet [1] == PKT_ACK) {
		if (((lword*)r_packet) [1] == my_id &&
				r_packet [4] == last_snt) {
			if (rf_start == 0)
				rf_start = seconds();
			rss = ((byte*)r_packet) [tcv_left (r_packet) - 1];
			if (rss > max_rss)
				max_rss = rss;
			last_run++;
			trigger (&last_snt);
		}
		tcv_endp (r_packet); // ignore acks for others
		proceed (RC_TRY);
	}

    entry (RC_DATA)
	if (r_packet [5] == RPC_QUIT) {
		diag ("remote reset");
		reset();
	}
	if (r_packet [5] != plev) {
		plev = r_packet [5];
		tcv_control (sfd, PHYSOPT_SETPOWER, &plev);
		diag ("remote SETPOWER %u", plev);
	}

	tmpack = tcv_wnp (RC_DATA, sfd, tcv_left (r_packet));
	memcpy (tmpack, r_packet, tcv_left (r_packet));
	tcv_endp (r_packet);
	tmpack [1] = PKT_ACK;
	tcv_endp (tmpack);
	proceed (RC_TRY);

endthread

thread (sender)

    word pl;
    int  pp;

    entry (SN_SEND)

	if (last_run > GOOD_RUN) {
		trigger (&last_run);
		finish;
	}

	if (last_run == 1) {
		leds (0, 0); leds (1, 2);
	}

	pl = gen_packet_length();
	x_packet = tcv_wnp (SN_SEND, sfd, pl);

	x_packet [0] = 0;
	x_packet [1] = PKT_DAT;
	((lword*)x_packet) [1] = my_id;
	x_packet [4] = ++last_snt;
	x_packet [5] = plev;

	// In words
	pl >>= 1;
	for (pp = 6; pp < pl; pp++)
		x_packet [pp] = (word) rnd();
	tcv_endp (x_packet);
	if (plev == RPC_QUIT) {
		tcv_control (sfd, PHYSOPT_RXOFF, NULL);
		diag ("out after sent");
		proceed (SN_OUT);
	}
	delay (MSG_TOUT, SN_MISS);
	when (&last_snt, SN_SEND);
	release;

    entry (SN_MISS)
	if (last_run != 0) {
		diag ("missed %d %d", last_snt, last_run);
		last_run = 0;
		leds (1, 0); leds (0, 2);
		proceed (SN_SEND);
	}
	delay (rnd() % 127, SN_SEND);
	release;

    entry (SN_OUT)
	if (tcv_qsize (sfd, TCV_DSP_XMT)) {
		delay (200, SN_OUT);
		release;
	}
	reset();

endthread


thread (root)

	word	w;

    entry (RS_INIT)
	if ((ibuf = (char*) umalloc (IBUFLEN)) == NULL) {
		diag ("no memory");
		halt();
	}

	phys_cc1100 (0, MAXPLEN);
	tcv_plug (0, &plug_null);
	if ((sfd = tcv_open (WNONE, 0, 0)) < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

	tcv_control (sfd, PHYSOPT_RXON, NULL);
	tcv_control (sfd, PHYSOPT_TXON, NULL);
	plev = RF_PLEV;
	tcv_control (sfd, PHYSOPT_SETPOWER, &plev);
	my_id = rnd(); my_id <<= 16; my_id |= rnd();
	diag ("id %x %x pl %d", (word)(my_id >> 16), (word)(my_id), plev);
	runthread (receiver);
	runthread (sender);
	leds (0, 2);
	diag ("rf testing");
	delay (RF_TOUT, RS_OSS);
	when (&last_run, RS_DONE);
	release;

    entry (RS_DONE)
	diag ("done pl:%d rss:%d snt:%d ts:%d", plev, max_rss, last_snt,
			(word)(seconds() - rf_start));
	leds (1, 1);

    entry (RS_OSS)

	ser_out (RS_OSS,
		"\r\nRF Test\r\n"
		"Commands:\r\n"
		"p  -> set power level\r\n"
		"n  -> new run\r\n"
		"q  -> reset\r\n"
		"Q  -> try to reset both\r\n"
	);
	proceed (RS_CMD);

    entry (RS_TOUT)
	diag ("rf timeout");

    entry (RS_CMD)
	ibuf [0] = 0;
	if (last_run <= GOOD_RUN) {
		delay (RF_TOUT, RS_TOUT);
		when (&last_run, RS_DONE);
	}

	(void)ser_in (RS_CMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {
	    case 'p':
		w = RF_PLEV;
		scan (ibuf + 1, "%u", &w);

		if (w > 7) { // don't adjust
			diag ("bad plev %u", w);
			proceed (RS_CMD);
		}

		if (w != plev) {
			diag ("plev %u -> %u", plev, w);
			plev = w;
			tcv_control (sfd, PHYSOPT_SETPOWER, &plev);
		} else
			diag ("already at %u", plev);

		proceed (RS_CMD);

	    case 'n':
		if (running (sender)) {
			diag ("still running");
		} else {
			last_run = last_snt = 0;
			runthread (sender);
		}

		proceed (RS_CMD);

	    case 'q': reset();
		      
	    case 'Q':
		// try to trigger remote reset
		plev = RPC_QUIT;
		last_run = 0;

		if (!running (sender))
			runthread (sender);

		diag ("going down in up to %u ms", MSG_TOUT);
		delay (MSG_TOUT, RS_OUT);
		release;

	    default:
		diag ("bad command");
		proceed (RS_OSS);
	}

    entry (RS_OUT)
	reset();

endthread

praxis_starter (Node);
