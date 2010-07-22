/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "tcvphys.h"
#include "ser.h"
#include "serf.h"
#include "form.h"
#include "phys_cc1100.h"
#include "plug_ack.h"
#include "pinopts.h"
#include "sensors.h"
#include "actuators.h"

// This is RFPing using the acknowledging NULL plugin. I have removed the
// praxis-level acknowledgments leaving everything else pretty much as it
// was.

#define	MIN_PACKET_LENGTH	24
#define	MAX_PACKET_LENGTH	46

#define	IBUFLEN		82

#define	PKT_DAT		0x01	// This is the "type" byte of a regular (data)
				// packet

#define MAXPLEN		(MAX_PACKET_LENGTH + 2)

heapmem {10, 90};

// ============================================================================

int	sfd = -1;
word	tdelay;

// ============================================================================

static word gen_packet_length (void) {

#if MIN_PACKET_LENGTH >= MAX_PACKET_LENGTH
	return MIN_PACKET_LENGTH;
#else
	return ((rnd () % (MAX_PACKET_LENGTH - MIN_PACKET_LENGTH + 1)) +
			MIN_PACKET_LENGTH) & 0xFFE;
#endif

}

static void radio_switch (Boolean on) {
//
// Note: because of the internal acknowledgments, both components (RX, TX) must
// be on for reception (as well as for transmission)

	if (on) {
		tcv_control (sfd, PHYSOPT_RXON, NULL);
		tcv_control (sfd, PHYSOPT_TXON, NULL);
	} else {
		tcv_control (sfd, PHYSOPT_RXOFF, NULL);
		tcv_control (sfd, PHYSOPT_TXOFF, NULL);
	}
}

fsm receiver {

    shared address packet;

    entry RC_TRY:

	packet = tcv_rnp (RC_TRY, sfd);

    entry RC_DATA:

	ser_outf (RC_DATA, "RCV: [%x] %lu (len = %d), pow = %d qua = %d\r\n",
		packet [1], ((lword*)packet) [1],
		tcv_left (packet) - 2,
		((byte*)packet) [tcv_left (packet) - 1],
		((byte*)packet) [tcv_left (packet) - 2]
	);

	tcv_endp (packet);
	proceed RC_TRY;
}

fsm sender {

    word pl;
    int  pp;

    shared lint last_snt = 0;
    shared address packet;
    shared word packet_length;

    entry SN_SEND:

	packet_length = gen_packet_length ();

	if (packet_length < 10)
		packet_length = 10;
	else if (packet_length > MAX_PACKET_LENGTH)
		packet_length = MAX_PACKET_LENGTH;


    entry SN_NEXT:

	// We control congestion in the transmission queue via a simple
	// handshake with the plugin
	if (NFreeHooks == 0) {
		// The plugin has run out of hooks, wait until one is freed
		when (&NFreeHooks, SN_NEXT);
		release;
	}

	packet = tcv_wnp (SN_NEXT, sfd, packet_length + 2);

	packet [0] = 0;	// This will be filled in by the PHY
	ptype (packet) = PKT_DAT;
	psernum (packet) = (byte) last_snt;

	// In words
	pl = packet_length / 2;
	((lword*)packet)[1] = last_snt;

	for (pp = 4; pp < pl; pp++)
		packet [pp] = (word) (entropy);

	tcv_endp (packet);

    entry SN_OUTM:

	ser_outf (SN_OUTM, "SND %lu, len = %u, fhooks = %u\r\n",
		last_snt, packet_length, NFreeHooks);
	last_snt++;
	delay (tdelay, SN_SEND);
}

int rcv_start () {

	radio_switch (YES);
	if (!running (receiver)) {
		runfsm receiver;
		return 1;
	}
	return 0;
}

int rcv_stop () {

	if (running (receiver)) {
		killall (receiver);
		return 1;
	}
	if (!running (sender))
		radio_switch (NO);
	return 0;
}

int snd_start (int del) {

	radio_switch (YES);
	tdelay = del;
	if (!running (sender)) {
		runfsm sender;
		return 1;
	}

	return 0;
}

int snd_stop () {

	if (running (sender)) {
		killall (sender);
		return 1;
	}
	if (!running (receiver))
		radio_switch (NO);
	return 0;
}

#ifdef	PMON_NOTEVENT

fsm pin_monitor {

    shared lint CNT, CMP;
    shared word STA;
    shared const char *MSG;

    entry PM_START:

	if (pmon_pending_not ()) {
		MSG = "NOTIFIER PENDING\r\n";
		proceed PM_OUT;
	}
	if (pmon_pending_cmp ()) {
		MSG = "COUNTER PENDING\r\n";
		proceed PM_OUT;
	}
	when (PMON_NOTEVENT, PM_NOTIFIER);
	when (PMON_CNTEVENT, PM_COUNTER);
	release;

  entry PM_OUT:

	ser_out (PM_OUT, MSG);
	proceed PM_START;

  entry PM_NOTIFIER:

	MSG = "NOTIFIER EVENT\r\n";
	proceed PM_EVENT;

  entry PM_COUNTER:

	MSG = "COUNTER EVENT\r\n";

  entry PM_EVENT:

	ser_out (PM_EVENT, MSG);

	STA = pmon_get_state ();
	CNT = pmon_get_cnt ();
	CMP = pmon_get_cmp ();

  entry PM_EVENT1:

	ser_outf (PM_EVENT1, "STATE: %x, CNT: %lu, CMP: %lu\r\n",
		STA, CNT, CMP);
	pmon_pending_cmp ();
	pmon_pending_not ();

	proceed PM_START;
}

#endif	/* PMON_NOTEVENT */

fsm watchdog {

    entry WA_START:

	watchdog_start ();

    entry WA_WAIT:

	watchdog_clear ();
	delay (300, WA_WAIT);
}

fsm root {

    shared char *ibuf;
    shared sint k, n1;
    shared const char *fmt;
    shared char obuf [32];
    shared word p [2];
    shared lint lp;

    entry RS_INIT:

	ibuf = (char*) umalloc (IBUFLEN);
	ibuf [0] = 0;

	// runfsm watchdog;

	phys_cc1100 (0, MAXPLEN);
	// WARNING: the SMURPH model assumes that the plugin is static, i.e.,
	// all nodes use the same plugin. This is easy to change later, but
	// ... for now ...
	// Thus, only the first tcv_plug operation is effective, and the
	// subsequent ones (with the same ordinal) will be ignored. It is
	// checked, however, whether the plugin is the same in all their
	// instances.
	tcv_plug (0, &plug_ack);
	sfd = tcv_open (WNONE, 0, 0);
	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

    entry RS_RCMDM2:

	ser_out (RS_RCMDM2,
		"\r\nRF Ping Test\r\n"
		"Commands:\r\n"
		"s intvl  -> snd int\r\n"
		"r        -> rcv\r\n"
		"d q v    -> physopt\r\n"
		"x p      -> xmt pwr\r\n"
		"y        -> get xmt pwr\r\n"
		"o        -> stop rcv\r\n"
		"t        -> stop xmt\r\n"
		"q        -> stop rf\r\n"
		"i        -> set sid\r\n"
		"z        -> reset\r\n"
		"p n      -> read pin\r\n"
		"u n v    -> set pin\r\n"
		"a n r d  -> read ADC pin\r\n"
		"w n v r  -> write DAC pin\r\n"
#ifdef PMON_NOTEVENT
		"C c e    -> start cnt\r\n"
		"P c      -> set cmp\r\n"
		"G        -> get cnt\r\n"
		"D        -> stop cnt\r\n"
		"N e      -> start ntf\r\n"
		"M        -> stop ntf\r\n"
		"X        -> start mtr\r\n"
		"Y        -> stop mtr\r\n"
#endif
		"S n      -> read n-th sensor\r\n"
		"A n v    -> set n-th actuator\r\n"
		"K        -> force watchdog reset\r\n"
	);

    entry RS_RCMDM1:

	if ((unsigned char) ibuf [0] == 0xff)
		ser_out (RS_RCMDM1,
			"No command in 10 seconds -> start s 1024, r\r\n"
				);
    entry RS_RCMD:

	if ((unsigned char) ibuf [0] == 0xff)
		delay (1024*10, RS_AUTOSTART);
  
	k = ser_in (RS_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {
	    case 's': proceed RS_SND;
	    case 'r': proceed RS_RCV;
	    case 'd': proceed RS_PAR;
	    case 'x': proceed RS_SETP;
	    case 'y': proceed RS_GETP;
	    case 'o': proceed RS_QRCV;
	    case 't': proceed RS_QXMT;
	    case 'q': proceed RS_QUIT;
	    case 'i': proceed RS_SSID;
	    case 'z': proceed RS_RES;
	    case 'p': proceed RS_RPIN;
	    case 'u': proceed RS_SPIN;
	    case 'a': proceed RS_RANA;
	    case 'w': proceed RS_WANA;
#ifdef PMON_NOTEVENT
	    case 'C': proceed RS_PSCN;
	    case 'P': proceed RS_PSCM;
	    case 'G': proceed RS_PGCN;
	    case 'D': proceed RS_PQCN;
	    case 'N': proceed RS_PSNT;
	    case 'M': proceed RS_PQNT;
	    case 'X': proceed RS_PSMT;
	    case 'Y': proceed RS_PQMT;
#endif
	    case 'S': proceed RS_GETS;
	    case 'A': proceed RS_SETA;
	    case 'K': {
			killall (watchdog);
			proceed RS_RCMD;
		      }
	}

    entry RS_RCMD1:

	ser_out (RS_RCMD1, "Illegal command or parameter\r\n");
	proceed RS_RCMDM2;

    entry RS_SND:

	/* Default */
	n1 = 2048;
	scan (ibuf + 1, "%d", &n1);
	if (n1 < 16)
		n1 = 16;
	snd_start (n1);

    entry RS_SND1:

	ser_outf (RS_SND1, "Sender rate: %d\r\n", n1);
	proceed RS_RCMD;

    entry RS_RCV:

	rcv_start ();
	proceed RS_RCMD;

    entry RS_QRCV:

	rcv_stop ();
	proceed RS_RCMD;

    entry RS_QXMT:

	snd_stop ();
	proceed RS_RCMD;

    entry RS_QUIT:

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

    entry RS_QUIT1:

	ser_outf (RS_QUIT1, fmt, obuf);
	proceed RS_RCMD;

    entry RS_PAR:

	p [1] = 0;
	if (scan (ibuf + 1, "%u %u", p+0, p+1) < 1)
		proceed RS_RCMD1;

	k = tcv_control (sfd, *(p+0), p+1);

    entry RS_PAR1:

	ser_outf (RS_PAR1, "Completed: %d %u\r\n", k, p [1]);
	proceed RS_RCMD;

    entry RS_SSID:

	n1 = 0;
	scan (ibuf + 1, "%d", &n1);
	tcv_control (sfd, PHYSOPT_SETSID, (address) &n1);
	proceed RS_RCMD;

    entry RS_AUTOSTART:
	  
	snd_start (1024);
  	rcv_start ();
	finish;

    entry RS_RES:

	reset ();
	// We should be killed past this

    entry RS_RPIN:

	if (scan (ibuf + 1, "%u", p+0) < 1)
		proceed RS_RCMD1;

	p [1] = pin_read (p [0]);

    entry RS_RPIN1:

	ser_outf (RS_RPIN1, "P[%u] = %u\r\n", p [0], p [1]);
	proceed RS_RCMD;

    entry RS_SPIN:

	if (scan (ibuf + 1, "%u %u", p+0, p+1) < 2)
		proceed RS_RCMD1;

	pin_write (p [0], p [1]);
	proceed RS_RCMD;

    entry RS_RANA:

	if (scan (ibuf + 1, "%u %u %u", p+0, &k, p+1) < 3)
		proceed RS_RCMD1;

    entry RS_RANA1:

	n1 = pin_read_adc (RS_RANA1, p [0], k, p [1]);

    entry RS_RANA2:

	ser_outf (RS_RANA2, "A[%u] = %d\r\n", p [0], n1);
	proceed RS_RCMD;

    entry RS_WANA:

	if (scan (ibuf + 1, "%u %u %u", p+0, &k, p+1) < 3)
		proceed RS_RCMD1;

	pin_write_dac (p [0], k, p [1]);
	proceed RS_RCMD;

#ifdef PMON_NOTEVENT

    entry RS_PSCN:

	k = 0;
	lp = 0;
	scan (ibuf + 1, "%lu %u", &lp, &k);
	pmon_start_cnt (lp, (Boolean) k);
	proceed RS_RCMD;

    entry RS_PSCM:

	if (scan (ibuf + 1, "%lu", &lp) < 1)
		proceed RS_RCMD1;

	pmon_set_cmp (lp);
	proceed RS_RCMD;

    entry RS_PGCN:

	lp = pmon_get_cnt ();

    entry RS_PGCN1:

	ser_outf (RS_PGCN1, "Counter = %lu\r\n", lp);
	proceed RS_RCMD;

    entry RS_PQCN:

	pmon_stop_cnt ();
	proceed RS_RCMD;

    entry RS_PSNT:

	k = 0;
	scan (ibuf + 1, "%u", &k);
	pmon_start_not ((Boolean) k);
	proceed RS_RCMD;

    entry RS_PQNT:

	pmon_stop_not ();
	proceed RS_RCMD;

    entry RS_PSMT:

	if (running (pin_monitor))
		proceed RS_PSMT1;

	runfsm pin_monitor;
	proceed RS_RCMD;

    entry RS_PSMT1:

	ser_out (RS_PSMT1, "Already running!\r\n");
	proceed RS_RCMD;

    entry RS_PQMT:

	killall (pin_monitor);
	proceed RS_RCMD;

#endif /* PMON_NOTEVENT */

    entry RS_SETP:

	if (scan (ibuf + 1, "%u", p+0) < 1)
		proceed RS_RCMD1;

	tcv_control (sfd, PHYSOPT_SETPOWER, p);
	proceed RS_RCMD;

    entry RS_GETP:

	ser_outf (RS_GETP, "P = %d\r\n",
		tcv_control (sfd, PHYSOPT_GETPOWER, NULL));
	proceed RS_RCMD;

    entry RS_GETS:

	if (scan (ibuf + 1, "%u", p+0) < 1)
		proceed RS_RCMD1;

    entry RS_GETS1:
	// Assume these sensors handle 2-byte values
	read_sensor (RS_GETS1, p [0], p + 1);

    entry RS_GETS2:

	ser_outf (RS_GETS2, "V = %u\r\n", p [1]);
	proceed RS_RCMD;

    entry RS_SETA:

	if (scan (ibuf + 1, "%u %u", p+0, p+1) < 1)
		proceed RS_RCMD1;

    entry RS_SETA1:

	write_actuator (RS_SETA1, p [0], p + 1);
	proceed RS_RCMD;
}
