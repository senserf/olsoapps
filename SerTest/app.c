/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "tcvphys.h"

heapmem {10, 90};

#include "lcd.h"
#include "ser.h"
#include "serf.h"
#include "phys_uart.h"
#include "plug_null.h"

/* ====================================================================== */
/* Ping through TCV over a serial port.  It  requires a peer connected to */
/* the board via a raw serial interface at 9600 bps. A sample program for */
/* Linux is in Linux/pingserial.c.                                        */
/* CHANNEL_MODE can be set to UART_PHYS_MODE_EMU and things should be OK. */
/* ====================================================================== */

#define	CHANNEL		UART_B
#define	CHANNEL_MODE	UART_PHYS_MODE_EMU	/* 0 or 1 */
#define	MAXPLEN		256
#define	IBUFLEN		82
#define	SERIAL_RATE	19200

static int sfd;

#define	RC_TRY		00
#define	RC_DUMP		10

static	int	rkillflag = 0;
static	long	nsent, nrcvd;

static void lcd_update (void) {

	char *fm, c;
	int k;

	fm = (char*) umalloc (34);
	form (fm, "%lu", nsent);
	k = strlen (fm);
	while (k < 16)
		fm [k++] = ' ';
	form (fm + 16, "%lu", nrcvd);
	dsp_lcd (fm, YES);
	c = ((((char)nrcvd) & 0x3) << 2) | (((char)nsent) & 0x3);
	ion (LEDS, CONTROL, &c, LEDS_CNTRL_SET);
	ufree (fm);
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

  entry (RC_DUMP)

	ser_outf (RC_DUMP, "RCV: LEN = %d, '%s'\r\n", tcv_left (packet),
		(char*)packet);
	tcv_endp (packet);
	nrcvd++;
	lcd_update ();
	proceed (RC_TRY);

endprocess (1)

int rcv_start (void) {

	rkillflag = 0;
	if (!running (receiver)) {
		fork (receiver, NULL);
		tcv_control (sfd, PHYSOPT_RXON, NULL);
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
#undef	RC_DUMP

/* ============= */
/* Packet sender */
/* ============= */

static	int	tdelay, tkillflag = 0;

#define	SN_SEND		00
#define	SN_ACQ		01

process (sender, void)

	static address packet;
	static char fmt [32];

	nodata;

  entry (SN_SEND)

	form (fmt, "Pkt %lu", nsent);

  entry (SN_ACQ)

	if (tkillflag) {
		tkillflag = 0;
		finish;
	}
	wait ((word) &tkillflag, SN_ACQ);
	packet = tcv_wnp (SN_SEND, sfd, strlen (fmt) + 1);
	strcpy ((char*)packet, fmt);
	tcv_endp (packet);

  entry (SN_ACQ+1)

	ser_outf (SN_ACQ+1, "Sent packet %lu\r\n", nsent);
	nsent++;
	lcd_update ();
	delay (tdelay, SN_SEND);

endprocess (1)

int snd_start (int del) {

	tdelay = del;
	tkillflag = 0;

	if (!running (sender)) {
		fork (sender, NULL);
		tcv_control (sfd, PHYSOPT_TXON, NULL);
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
#undef	SN_ACQ

/* ================= */
/* End packet sender */
/* ================= */

#define	RS_INIT		00
#define	RS_RCMD		10
#define	RS_SEND		20
#define	RS_RCV		30
#define	RS_PAR		40
#define	RS_QUIT		50

process (root, int)

	static char *ibuf;
	static int k, n1;
	static char *fmt, obuf [24];
	static word p [2];

  entry (RS_INIT)

	dsp_lcd ("PicOS ready     SERTEST", YES);
	ibuf = (char*) umalloc (IBUFLEN);
	p [0] = (word) SERIAL_RATE;
	ion (CHANNEL, CONTROL, (char*)p, UART_CNTRL_RATE);
	phys_uart (0, CHANNEL, CHANNEL_MODE, MAXPLEN);
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
		"\r\nSerial emulation test\r\n"
		"Commands:\r\n"
		"s intvl  -> start/reset sending\r\n"
		"r        -> start receiving\r\n"
		"p i v    -> change delay parameter i to v\r\n"
		"q        -> stop transmission and reception\r\n"
		);

  entry (RS_RCMD)

	k = ser_in (RS_RCMD, ibuf, IBUFLEN-1);

	if (ibuf [0] == 's')
		proceed (RS_SEND);
	if (ibuf [0] == 'r')
		proceed (RS_RCV);
	if (ibuf [0] == 'p')
		proceed (RS_PAR);
	if (ibuf [0] == 'q')
		proceed (RS_QUIT);

  entry (RS_RCMD+1)

	ser_out (RS_RCMD+1, "Illegal command or parameter\r\n");
	proceed (RS_RCMD);

  entry (RS_SEND)

	/* Default */
	n1 = 2048;
	scan (ibuf + 1, "%d", &n1);
	if (n1 < 256)
		n1 = 256;
	if (snd_start (n1))
		fmt = "Started sender, delay = %d\r\n";
	else
		fmt = "New sender delay = %d\r\n";

  entry (RS_SEND+1)

	ser_outf (RS_SEND+1, fmt, n1);
	proceed (RS_RCMD);

  entry (RS_RCV)

	if (rcv_start ())
		fmt = "Started receiver\r\n";
	else
		fmt = "Receiver already running\r\n";

  entry (RS_RCV+1)

	ser_outf (RS_RCV+1, fmt);
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

	if (scan (ibuf + 1, "%d %u", p, p+1) < 2)
		proceed (RS_RCMD+1);

	if (p [0] > 5)
		proceed (RS_RCMD+1);

	tcv_control (sfd, PHYSOPT_SETPARAM, p);

  entry (RS_PAR+1)

	ser_outf (RS_PAR+1, "Parameter %u set to %u\n", p [0], p [1]);

	proceed (RS_RCMD);

	nodata;

endprocess (1)

#undef	RS_INIT
#undef	RS_RCMD
#undef	RS_SEND
#undef	RS_RCV
#undef	RS_PAR
#undef	RS_QUIT
