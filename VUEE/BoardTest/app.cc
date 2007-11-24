/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "globals.h"
#include "threadhdrs.h"

#define	MIN_PACKET_LENGTH	24
#define	MAX_PACKET_LENGTH	46

#define	IBUFLEN		12

#define	PKT_ACK		0x1234
#define	PKT_DAT		0xABCD

#define MAXPLEN		(MAX_PACKET_LENGTH + 2)

#define RF_PLEV		1
#define RF_TOUT		5000
#define MSG_TOUT	2000

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
	pl = gen_packet_length();
	x_packet = tcv_wnp (SN_SEND, sfd, pl);

	x_packet [0] = 0;
	x_packet [1] = PKT_DAT;
	((lword*)x_packet) [1] = my_id;
	x_packet [4] = ++last_snt;

	// In words
	pl >>= 1;
	for (pp = 5; pp < pl; pp++)
		x_packet [pp] = (word) (entropy);
	tcv_endp (x_packet);
	delay (MSG_TOUT, SN_MISS);
	when (&last_snt, SN_SEND);
	release;

    entry (SN_MISS)
	if (last_run != 0) {
		diag ("missed %d %d", last_snt, last_run);
		last_run = 0;
		proceed (SN_SEND);
	}
	delay (rnd() % 127, SN_SEND);
	release;

endstrand


thread (root)

	lword	eea, lw, eemax;
	word	err, w;
	byte	b, b2;

    entry (RS_INIT)

      switch (if_read (IFLASH_SIZE -1)) {
	case TS_INIT:
		diag ("flashed virgin");
		eemax = ee_size (NULL, NULL) -1;

		for (eea = 0; eea < EE_WRITES; eea++) {
			
			if ((err = ee_read (eea, &b, 1)) != 0 ||
			    (err = ee_read (eemax - eea, &b2, 1)) != 0) {

				diag ("ee_read1 failed[%d] on %d", err,
					(word)eea);
				leds (0, 2);
				proceed (RS_MANUAL);
			}

			if (b != 0xff || b2 != 0xff) {
				diag ("eeprom not empty");
				leds (0, 2);
				proceed (RS_MANUAL);
			}
		}

		b = 0xAA;
		for (eea = 0; eea < EE_WRITES; eea++) {
			if ((err = ee_write (WNONE, eea, &b, 1)) != 0 ||
			    (err = ee_write (WNONE, eemax - eea, &b, 1)) != 0) {
				diag ("ee_write failed[%d] on %x", err,
					(word)eea);
				leds (0, 2);
				proceed (RS_MANUAL);
			}
		}
		if_write (IFLASH_SIZE -1, TS_EE);
		reset();

	case TS_EE:
	case TS_BUT:
		eemax = ee_size (NULL, NULL) -1;
		for (eea = 0; eea < EE_WRITES; eea++) {
			if ((err = ee_read (eea, &b, 1)) != 0 ||
			    (err = ee_read (eemax - eea, &b2, 1)) != 0) {
				diag ("ee_read2 failed[%d] on %x", err,
					(word)eea);
				leds (0, 2);
				proceed (RS_MANUAL);
			}

			if (b != 0xAA || b2 != 0xAA) {
				diag ("wrong ee writes %x %x %x",
					(word)eea, b, b2);
				leds (0, 2); 
				proceed (RS_MANUAL);
			}
		}

		if (if_read (IFLASH_SIZE -1) == TS_EE) {
			leds (2, 1);
			diag ("push button");
			proceed (RS_MANUAL);
		} else {
			if_write (IFLASH_SIZE -1, TS_RF);
			diag ("done with eeprom");
		}
		// falling through

	case TS_RF:
	case TS_DONE:
		phys_cc1100 (0, MAXPLEN);
		tcv_plug (0, &plug_null);
		if ((sfd = tcv_open (WNONE, 0, 0)) < 0) {
			diag ("Cannot open tcv interface");
			halt ();
		}
		tcv_control (sfd, PHYSOPT_RXON, NULL);
		tcv_control (sfd, PHYSOPT_TXON, NULL);
		w = RF_PLEV;
		tcv_control (sfd, PHYSOPT_SETPOWER, &w);
		my_id = rnd();
		runthread (receiver);
		if (if_read (IFLASH_SIZE -1) == TS_RF) {
			leds (1, 2);
			diag ("rf testing");
			runthread (sender);
			delay (RF_TOUT, RS_TOUT);
			when (&last_run, RS_DONE);
			release;
		}
		// TS_DONE 
		diag ("tested %d %d %d", if_read (IFLASH_SIZE -2),
				if_read (IFLASH_SIZE -3),
				if_read (IFLASH_SIZE -4));
		leds (1, 1);
		proceed (RS_MANUAL);

	default:
		diag ("bad FIM %d", if_read (IFLASH_SIZE -1));
		proceed (RS_MANUAL);
      } // switch

    entry (RS_DONE)
	if_write (IFLASH_SIZE -1, TS_DONE);
	if_write (IFLASH_SIZE -2, last_snt);
	if_write (IFLASH_SIZE -3, (word)(seconds() - rf_start));
	if_write (IFLASH_SIZE -4, max_rss);

	diag ("done %d %d %d", if_read (IFLASH_SIZE -2),
			if_read (IFLASH_SIZE -3),
			if_read (IFLASH_SIZE -4));
	leds (1, 1);
	proceed (RS_MANUAL);

    entry (RS_TOUT)
	diag ("rf timeout: no bouncer?");

    entry (RS_MANUAL)

	if ((ibuf = (char*) umalloc (IBUFLEN)) == NULL) {
		diag ("no memory");
		halt();
	}
	ibuf [0] = 0;

    entry (RS_OSS)

	ser_out (RS_OSS,
		"\r\nBoard Test\r\n"
		"Commands:\r\n"
		"+  -> start receiver\r\n"
		"-  -> stop receiver\r\n"
		"e  -> erase eeprom\r\n"
		"i  -> erase FIM\r\n"
		"d  -> run data\r\n"
		"q  -> reset\r\n"
		// awruk -> pushed reset button
	);

    entry (RS_CMD)
	if ((w = if_read (IFLASH_SIZE -1)) == TS_BUT || w == TS_RF) {
		delay (RF_TOUT, RS_TOUT);
		when (&last_run, RS_DONE);
	}

	(void)ser_in (RS_CMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {
	    case '+':
		tcv_control (sfd, PHYSOPT_RXON, NULL);
		proceed (RS_CMD);

	    case '-':
		tcv_control (sfd, PHYSOPT_RXOFF, NULL);
		proceed (RS_CMD);

	    case 'i':
		if_erase (-1);
		proceed (RS_CMD);

	    case 'q': reset();
		      
	    case 'e':
		if ((err = ee_erase (WNONE, 0, 0)) != 0)
			diag ("ee_erase err [%d]", err);
		proceed (RS_CMD);

	    case 'd':
		diag ("run? %d %d %d", if_read (IFLASH_SIZE -2),
			if_read (IFLASH_SIZE -3), if_read (IFLASH_SIZE -4));
		 proceed (RS_CMD);

	    case 'a':
		if (strlen(ibuf) > 4 && ibuf[4] == 'k' &&
				ibuf[2] == 'r' &&
				ibuf[3] == 'u' &&
				ibuf[1] == 'w') {
			if (if_read (IFLASH_SIZE -1) == TS_EE)
				if_write (IFLASH_SIZE -1, TS_BUT);
			reset();
		}
		// fall through
			
	    default:
		diag ("bad command");
		proceed (RS_OSS);
	}

endthread

praxis_starter (Node);
