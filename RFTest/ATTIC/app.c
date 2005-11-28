/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "tcvphys.h"

/*
 * You should run this application on two boards at the same time. When
 * started (the 's' command), it will periodically send a packet over
 * the radio channel awaiting an acknowledgment from the other board
 * before proceeding with the next packet. Also, it will be responding
 * with acknowledgments to packets received from the other board.
 *
 * If no command is entered over the serial interface within 10 seconds,
 * the application starts with "s 1024\nr", i.e., it turns the receiver
 * on and begins to send packets at 1 sec intervals.
 *
 */

heapmem {10, 90};

#if 0
extern word zz_rrates [];
#endif

#include "ser.h"
#include "serf.h"
#include "form.h"

#if CC1000
#include "phys_cc1000.h"
#endif

#if DM2100
#include "phys_dm2100.h"
#endif

#if RADIO_DRIVER
#include "phys_radio.h"
#endif

#include "plug_null.h"

#define	MAXPLEN		32
#define	IBUFLEN		64

static int sfd;

byte	rkillflag = 0, xkillflag = 0;

#define	RC_TRY	0
#define	RC_SHOW	1
#define	RC_DUMP	2

process (receiver, void)

	static address packet;
	static k;

	nodata;

  entry (RC_TRY)

	if (rkillflag) {
		rkillflag = 0;
		finish;
	}
	wait ((word) &rkillflag, RC_TRY);
	packet = tcv_rnp (RC_TRY, sfd);

  entry (RC_SHOW)

#if     CC1000 || DM2100
 	// Show RSSI
	ser_outf (RC_SHOW, "RCV: (len = %d), pow = %x:", tcv_left (packet),
		packet [(tcv_left (packet) >> 1) - 1]);
#else
	ser_outf (RC_SHOW, "RCV: (len = %d):", tcv_left (packet));
#endif
	k = 0;

  entry (RC_DUMP)

	if (k == tcv_left (packet)) {
		ser_out (RC_DUMP, "\r\n");
		tcv_endp (packet);
		proceed (RC_TRY);
	}

	ser_outf (RC_DUMP, " %x", packet [k]);
	k++;
	proceed (RC_DUMP);

endprocess (1)

int rcv_start (void) {

	rkillflag = 0;
	tcv_control (sfd, PHYSOPT_RXON, NULL);
	if (!running (receiver)) {
		fork (receiver, NULL);
		return 1;
	}
	return 0;
}

int rcv_stop (void) {

	if (running (receiver)) {
		tcv_control (sfd, PHYSOPT_RXOFF, NULL);
		rkillflag = 1;
		trigger ((word) &rkillflag);
		return 1;
	}
	return 0;
}

/* ============= */
/* Packet sender */
/* ============= */

static	int	tdelay;

#define	SN_SEND		00
#define	SN_NEXT		01

process (sender, void)

	static address packet;
	static word number;

	nodata;

  entry (SN_SEND)

	if (xkillflag) {
		xkillflag = 0;
		finish;
	}
	wait ((word) &xkillflag, SN_SEND);

	packet = tcv_wnp (SN_SEND, sfd, 12);
	packet [0] = 0x1234;
	packet [1] = number;
	packet [2] = 0x0F0F;
	packet [3] = 0xABCD;
	tcv_endp (packet);

  entry (SN_NEXT)

	ser_outf (SN_NEXT, "SND: %x %x %x %x\r\n",
		packet [0], packet [1], packet [2], packet [3]);
	number++;
	delay (tdelay, SN_SEND);

endprocess (1)

int snd_start (int del) {

	tdelay = del;
	xkillflag = 0;

	tcv_control (sfd, PHYSOPT_TXON, NULL);
	if (!running (sender)) {
		fork (sender, NULL);
		return 1;
	}

	return 0;
}

int snd_stop (void) {

	if (running (sender)) {
		xkillflag = 1;
		tcv_control (sfd, PHYSOPT_TXOFF, NULL);
		trigger ((word) &xkillflag);
		return 1;
	}
	return 0;
}

/* ================= */
/* End packet sender */
/* ================= */

#define	RS_INIT		00
#define	RS_RCMD		10
#define	RS_SND		20
#define RS_RCV		30
#define	RS_PAR		40
#define	RS_POW		50
#define	RS_RCP		60
#define	RS_QRCV		63
#define	RS_QXMT		66
#define	RS_QUIT		70
#define	RS_CAL		80
#define	RS_STK		85
#define	RS_AUTOSTART	90

#define	RS_RRA		100

#if CC1000
const static word parm_power = 1;
#endif

process (root, int)

	static char *ibuf;
	static int k, n1;
	static char *fmt, obuf [24];
	static word p [2];

  entry (RS_INIT)

	ibuf = (char*) umalloc (IBUFLEN);
	ibuf [0] = 0xff;

#if CC1000
	// Configure CC1000 for 19,200 bps
	phys_cc1000 (0, MAXPLEN, 192 /* 192 768 384 */);
#endif

#if DM2100
	phys_dm2100 (0, MAXPLEN);
#endif

#if RADIO_DRIVER
	// Generic
	phys_radio (0, 0, MAXPLEN);
#endif

	tcv_plug (0, &plug_null);
	sfd = tcv_open (NONE, 0, 0);
	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

#if CC1000
	tcv_control (sfd, PHYSOPT_SETPOWER, (address) &parm_power);
#endif

  entry (RS_RCMD-2)

	ser_out (RS_RCMD-2,
		"\r\nRF Test\r\n"
		"Commands:\r\n"
		"s intvl  -> start/reset sending interval (2 secs default)\r\n"
		"r        -> start receiver\r\n"
		"d i v    -> change phys parameter i to v\r\n"
#if CC1000 == 0 && DM2100 == 0
		// Not available
		"c btime  -> recalibrate the transceiver\r\n"
#endif

#if DM2100 == 0
		"p v      -> set transmit power\r\n"
#endif

		"g        -> get received power\r\n"
		"o        -> stop receiver\r\n"
		"t        -> stop transmitter\r\n"
		"q        -> stop both\r\n"
#if STACK_GUARD
		"v        -> show unused stack space\r\n"
#endif

#if 0
		"m        -> show rates\r\n"
		"m a v    -> modify rate 'a' to 'v'\r\n"
#endif
		);

  entry (RS_RCMD-1)

	if ((unsigned char) ibuf [0] == 0xff)
		ser_out (RS_RCMD-1,
			"No command in 10 seconds -> start s 1024, r\r\n"
			);

  entry (RS_RCMD)

	if ((unsigned char) ibuf [0] == 0xff)
		delay (1024*10, RS_AUTOSTART);
  
	k = ser_in (RS_RCMD, ibuf, IBUFLEN-1);


	if (ibuf [0] == 'm')
		proceed (RS_RRA);

	if (ibuf [0] == 's')
		proceed (RS_SND);
	if (ibuf [0] == 'r')
		proceed (RS_RCV);
	if (ibuf [0] == 'd')
		proceed (RS_PAR);
#if STACK_GUARD
	if (ibuf [0] == 'v')
		proceed (RS_STK);
#endif

#if CC1000 == 0 && DM2100 == 0
	if (ibuf [0] == 'c')
		proceed (RS_CAL);
#endif

#if DM2100 == 0
	if (ibuf [0] == 'p')
		proceed (RS_POW);
#endif

	if (ibuf [0] == 'g')
		proceed (RS_RCP);
	if (ibuf [0] == 'q')
		proceed (RS_QUIT);
	if (ibuf [0] == 'o')
		proceed (RS_QRCV);
	if (ibuf [0] == 't')
		proceed (RS_QXMT);

  entry (RS_RCMD+1)

	ser_out (RS_RCMD+1, "Illegal command or parameter\r\n");
	proceed (RS_RCMD-2);

  entry (RS_SND)

	/* Default */
	n1 = 2048;
	scan (ibuf + 1, "%d", &n1);
	if (n1 < 16)
		n1 = 16;
	snd_start (n1);

  entry (RS_SND+1)

	ser_outf (RS_SND+1, "Sender rate: %d\r\n", n1);
	proceed (RS_RCMD);

  entry (RS_RCV)

	rcv_start ();
	proceed (RS_RCMD);

#if DM2100 == 0
  entry (RS_POW)

	/* Default */
	n1 = 0;
	scan (ibuf + 1, "%d", &n1);

#if CC1000 == 0
	io (NONE, RADIO, CONTROL, (char*) &n1, RADIO_CNTRL_SETPOWER);
#else
	// There's no RADIO device, SETPOWER is available via
	// tcv_control
	tcv_control (sfd, PHYSOPT_SETPOWER, (address)&n1);
#endif

  entry (RS_POW+1)

	ser_outf (RS_POW+1,
		"Transmitter power set to %d\r\n", n1);

	proceed (RS_RCMD);
#endif /* ~DM 2100 */

  entry (RS_RCP)

#if CC1000 == 0 && DM2100 == 0
	n1 = io (NONE, RADIO, CONTROL, NULL, RADIO_CNTRL_READPOWER);
#else
	// No RADIO device for CC1000 & DM2100
	n1 = tcv_control (sfd, PHYSOPT_GETPOWER, NULL);
#endif

  entry (RS_RCP+1)

	ser_outf (RS_RCP+1,
		"Received power is %d\r\n", n1);

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

  entry (RS_QUIT+1)

	ser_outf (RS_QUIT+1, fmt, obuf);
	proceed (RS_RCMD);

  entry (RS_PAR)

	if (scan (ibuf + 1, "%u %u", p+0, p+1) < 2)
		proceed (RS_RCMD+1);

	if (p [0] > 5)
		proceed (RS_RCMD+1);

	tcv_control (sfd, PHYSOPT_SETPARAM, p);

  entry (RS_PAR+1)

	ser_outf (RS_PAR+1, "Parameter %u set to %u\r\n", p [0], p [1]);

	proceed (RS_RCMD);

#if CC1000 == 0 && DM2100 == 0

  entry (RS_CAL)

	/* The default */
	n1 = 19200;
	scan (ibuf + 1, "%d", &n1);
	io (NONE, RADIO, CONTROL, (char*) &n1, RADIO_CNTRL_CALIBRATE);

  entry (RS_CAL+1)

	ser_outf (RS_CAL+1,
		"Transceiver recalibrated to bit rate = %u bps\r\n", n1);

	proceed (RS_RCMD);
#endif

#if STACK_GUARD

  entry (RS_STK)

	ser_outf (RS_STK, "Free stack space = %d words\r\n", stackfree ());
	proceed (RS_RCMD);
#endif

  entry (RS_AUTOSTART)
	  
	snd_start (1024);
  	rcv_start ();

  	finish;

#if 0
  entry (RS_RRA)

	if (scan (ibuf + 1, "%u %u", p+0, p+1) < 2)
		proceed (RS_RRA+1);

	// Set
	if (p [0] > 4)
		proceed (RS_RCMD+1);

	zz_rrates [p[0]] = p [1];
	proceed (RS_RCMD);

  entry (RS_RRA+1)

	ser_outf (RS_RRA+1, "Rcv rates: %u, %u, %u, %u, %u\r\n",
		zz_rrates [0],
		zz_rrates [1],
		zz_rrates [2],
		zz_rrates [3],
		zz_rrates [4]
	);
	proceed (RS_RCMD);
#endif

	nodata;

endprocess (1)

#undef	RS_INIT
#undef	RS_RCMD
#undef	RS_SND
#undef  RS_RCV
#undef	RS_RCV
#undef	RS_PAR
#undef	RS_QUIT
#undef  RS_CAL
