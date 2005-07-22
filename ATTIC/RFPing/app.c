/* ============================================================================ */
/*                                                                              */
/* Copyright (C) Olsonet Communications Corporation, 2002, 2004                 */
/*                                                                              */
/* Permission is hereby granted, free of charge, to any person obtaining a copy */
/* of this software and associated documentation files (the "Software"), to     */
/* deal in the Software without restriction, including without limitation the   */
/* rights to use, copy, modify, merge, publish, distribute, sublicense, and/or  */
/* sell copies of the Software, and to permit persons to whom the Software is   */
/* furnished to do so, subject to the following conditions:                     */
/*                                                                              */
/* The above copyright notice and this permission notice shall be included in   */
/* all copies or substantial portions of the Software.                          */
/*                                                                              */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   */
/* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     */
/* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  */
/* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER       */
/* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      */
/* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS */
/* IN THE SOFTWARE.                                                             */
/*                                                                              */
/* ============================================================================ */

#include "sysio.h"
#include "tcvphys.h"

/*
 * You should run this application on two boards at the same time. When
 * started (the 's' command), it will periodically send a packet over
 * the radio channel awaiting an acknowledgment from the other board
 * before proceeding with the next packet. Also, it will be responding
 * with acknowledgments to packets received from the other board.
 *
 */

#define	USE_LCD		1

heapmem {10, 90};

#if USE_LCD
#include "lcd.h"
#endif

#include "ser.h"
#include "form.h"

#if CHIPCON
#include "phys_chipcon.h"
#endif

#if RADIO_DRIVER
#include "phys_radio.h"
#endif

#if RF24G
#include "phys_rf24g.h"
#endif

#include "plug_null.h"

#define	MAXPLEN		256
#define	IBUFLEN		82

#define	PKT_ACK		1
#define	PKT_DAT		0

static int sfd;

#define	RC_TRY		00
#define	RC_DATA		10
#define	RC_SACK		20
#define	RC_ACK		30

static	int	rkillflag = 0;
static	long	last_snt, last_rcv, last_ack;

static void lcd_update (void) {

#if USE_LCD

	char *fm;
	int k;

	fm = (char*) umalloc (34);
	form (fm, "%lu", last_snt);
	k = strlen (fm);
	while (k < 16)
		fm [k++] = ' ';
	form (fm + 16, "%lu", last_rcv);
	upd_lcd (fm);
	ufree (fm);
#endif

}

process (receiver, void)

	static address packet;

	nodata;

  entry (RC_TRY)

	if (rkillflag) {
		rkillflag = 0;
		finish;
	}
	wait ((word) &rkillflag, RC_TRY);
	packet = tcv_rnp (RC_TRY, sfd);

	if (packet [1] == PKT_ACK) {
		last_ack = ((lword*)packet) [1];
		proceed (RC_ACK);
	}

	// Data packet
	last_rcv = ((lword*)packet) [1];

  entry (RC_DATA)

#if     CHIPCON
 	// Show received power level indication with CHIPCON
	ser_outf (RC_DATA, "RCV: %lu (len = %d), pow = %x\r\n", last_rcv,
		tcv_left (packet), packet [(tcv_left (packet) >> 1) - 1]);
#else
	ser_outf (RC_DATA, "RCV: %lu (len = %d)\r\n", last_rcv,
		tcv_left (packet));
#endif

	tcv_endp (packet);

	// Acknowledge it

  entry (RC_SACK)

	packet = tcv_wnp (RC_SACK, sfd, 12);
	packet [0] = 0;
	packet [1] = PKT_ACK;
	((lword*)packet) [1] = last_rcv;
	tcv_endp (packet);
	lcd_update ();
	proceed (RC_TRY);

  entry (RC_ACK)

	ser_outf (RC_ACK, "ACK: %lu (len = %d)\r\n", last_ack,
		tcv_left (packet));
	tcv_endp (packet);
	trigger ((word)&last_ack);
	lcd_update ();
	proceed (RC_TRY);

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

#undef	RC_TRY
#undef	RC_DATA
#undef	RC_SACK
#undef	RC_ACK

/* ============= */
/* Packet sender */
/* ============= */

static	int	tdelay, tkillflag = 0;

#define	SN_SEND		00
#define	SN_NEXT		01

process (sender, void)

	static address packet;

	nodata;

  entry (SN_SEND)

	if (tkillflag) {
		tkillflag = 0;
		finish;
	}
	wait ((word) &tkillflag, SN_SEND);

	if (last_ack != last_snt) {
		delay (tdelay, SN_NEXT);
		wait ((word) &last_ack, SN_SEND);
		release;
	}

	last_snt++;
	proceed (SN_NEXT);

  entry (SN_NEXT)

	if (tkillflag) {
		tkillflag = 0;
		finish;
	}
	wait ((word) &tkillflag, SN_SEND);

	packet = tcv_wnp (SN_NEXT, sfd, 12);
	packet [0] = 0;
	packet [1] = 0;
	((lword*)packet)[1] = last_snt;
	tcv_endp (packet);

  entry (SN_NEXT+1)

	ser_outf (SN_NEXT+1, "SND %lu\r\n", last_snt);
	lcd_update ();
	proceed (SN_SEND);

endprocess (1)

int snd_start (int del) {

	tdelay = del;
	tkillflag = 0;
	last_ack = last_snt;

	tcv_control (sfd, PHYSOPT_TXON, NULL);
	if (!running (sender)) {
		fork (sender, NULL);
		return 1;
	}

	return 0;
}

int snd_stop (void) {

	if (running (sender)) {
		tkillflag = 1;
		tcv_control (sfd, PHYSOPT_TXOFF, NULL);
		trigger ((word) &tkillflag);
		return 1;
	}
	return 0;
}

#undef	SN_SEND
#undef	SN_NEXT

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

process (root, int)

	static char *ibuf;
	static int k, n1;
	static char *fmt, obuf [24];
	static word p [2];

  entry (RS_INIT)

#if USE_LCD
	dsp_lcd ("PicOS ready     RF PING", YES);
#endif
	ibuf = (char*) umalloc (IBUFLEN);

#if CHIPCON
	// Configure CHIPCON for 19,200 bps
	phys_chipcon (0, 0, MAXPLEN, 192 /* 192 768 384 */);
#endif

#if RF24G
	phys_rf24g (
		0, 		/* PHY ID */
		0xe7,		/* Group address */
		0,		/* Mode: 0-promiscuous, !=0 -> address */
		2,		/* Channel */
		1000 		/* rate: 250, 1000 */
	);
#endif

#if RADIO_DRIVER
	phys_radio (0, 0, MAXPLEN);
#endif
	tcv_plug (0, &plug_null);
	sfd = tcv_open (NONE, 0, 0);
	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}
	tcv_control (sfd, PHYSOPT_TXOFF, NULL);
	tcv_control (sfd, PHYSOPT_RXOFF, NULL);

  entry (RS_RCMD-1)

	ser_out (RS_RCMD-1,
		"\r\nRF Ping Test\r\n"
		"Commands:\r\n"
		"s intvl  -> start/reset sending interval (2 secs default)\r\n"
		"r        -> start receiver\r\n"
		"d i v    -> change phys parameter i to v\r\n"
#if CHIPCON == 0 && RF24G == 0
		// Not available
		"c btime  -> recalibrate the transceiver\r\n"
#endif
		"p v      -> set transmit power\r\n"
#if RF24G == 0
		"g        -> get received power\r\n"
#endif
		"o        -> stop receiver\r\n"
		"t        -> stop transmitter\r\n"
		"q        -> stop both\r\n"
		);

  entry (RS_RCMD)

	k = ser_in (RS_RCMD, ibuf, IBUFLEN-1);

	if (ibuf [0] == 's')
		proceed (RS_SND);
	if (ibuf [0] == 'r')
		proceed (RS_RCV);
	if (ibuf [0] == 'd')
		proceed (RS_PAR);
#if CHIPCON == 0
	if (ibuf [0] == 'c')
		proceed (RS_CAL);
#endif
	if (ibuf [0] == 'p')
		proceed (RS_POW);
#if RF24G == 0
	if (ibuf [0] == 'g')
		proceed (RS_RCP);
#endif
	if (ibuf [0] == 'q')
		proceed (RS_QUIT);
	if (ibuf [0] == 'o')
		proceed (RS_QRCV);
	if (ibuf [0] == 't')
		proceed (RS_QXMT);

  entry (RS_RCMD+1)

	ser_out (RS_RCMD+1, "Illegal command or parameter\r\n");
	proceed (RS_RCMD-1);

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

  entry (RS_POW)

	/* Default */
	n1 = 0;
	scan (ibuf + 1, "%d", &n1);

#if CHIPCON == 0 && RF24G == 0
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

#if RF24G == 0
  entry (RS_RCP)

#if CHIPCON == 0
	n1 = io (NONE, RADIO, CONTROL, NULL, RADIO_CNTRL_READPOWER);
#else
	// No RADIO device for CHIPCON
	n1 = tcv_control (sfd, PHYSOPT_GETPOWER, NULL);
#endif

  entry (RS_RCP+1)

	ser_outf (RS_RCP+1,
		"Received power is %d\r\n", n1);

	proceed (RS_RCMD);
#endif

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

#if CHIPCON == 0 && RF24G == 0

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
