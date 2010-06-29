/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "tcvphys.h"

#define	MIN_PACKET_LENGTH	24
#define	MAX_PACKET_LENGTH	42

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

#include "ser.h"
#include "serf.h"
#include "form.h"
#include "phys_cc1100.h"
#include "plug_null.h"

#define	IBUFLEN		82

#define	PKT_ACK		0x1234
#define	PKT_DAT		0xABCD

#define	ACK_LENGTH	12
#define MAXPLEN		(MAX_PACKET_LENGTH + 2)

static int sfd;

#define	RC_TRY		00
#define	RC_DATA		10
#define	RC_SACK		20
#define	RC_ACK		30

static	int	rkillflag = 0;
static	long	last_snt, last_rcv, last_ack;
static  char 	XMTon = 0, RCVon = 0;

static word gen_packet_length (void) {

	return ((rnd () % (MAX_PACKET_LENGTH - MIN_PACKET_LENGTH + 1)) +
			MIN_PACKET_LENGTH) & 0xFFE;

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
		last_ack = ntowl (((lword*)packet) [1]);
		proceed (RC_ACK);
	}

	// Data packet
	last_rcv = ntowl (((lword*)packet) [1]);

  entry (RC_DATA)

	ser_outf (RC_DATA, "RCV: [%x] %lu (len = %d), pow = %d qua = %d\r\n",
		packet [1],
		last_rcv, tcv_left (packet) - 2,
		((byte*)packet) [tcv_left (packet) - 1],
		((byte*)packet) [tcv_left (packet) - 2]
	);

	tcv_endp (packet);

	// Acknowledge it

  entry (RC_SACK)

	if (XMTon) {
		packet = tcv_wnp (RC_SACK, sfd, ACK_LENGTH);
		packet [0] = 0;
		packet [1] = PKT_ACK;
		((lword*)packet) [1] = wtonl (last_rcv);

		packet [4] = (word) entropy;
		tcv_endp (packet);
	}
	proceed (RC_TRY);

  entry (RC_ACK)

	ser_outf (RC_ACK, "ACK: %lu (len = %d)\r\n", last_ack,
		tcv_left (packet));

	tcv_endp (packet);
	trigger ((word)&last_ack);
	proceed (RC_TRY);

endprocess (1)

int rcv_start (void) {

	rkillflag = 0;
	tcv_control (sfd, PHYSOPT_RXON, NULL);
	if (!running (receiver)) {
		fork (receiver, NULL);
		RCVon = 1;
		return 1;
	}
	return 0;
}

int rcv_stop (void) {

	if (running (receiver)) {
		tcv_control (sfd, PHYSOPT_RXOFF, NULL);
		rkillflag = 1;
		trigger ((word) &rkillflag);
		RCVon = 0;
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
	static word packet_length = 12;

	word pl;
	int  pp;

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

	packet_length = gen_packet_length ();

	proceed (SN_NEXT);

  entry (SN_NEXT)

	if (tkillflag) {
		tkillflag = 0;
		finish;
	}
	wait ((word) &tkillflag, SN_SEND);

	packet = tcv_wnp (SN_NEXT, sfd, packet_length + 2);

	packet [0] = 0;
	packet [1] = PKT_DAT;

	// In words
	pl = packet_length / 2;
	((lword*)packet)[1] = wtonl (last_snt);

	for (pp = 4; pp < pl; pp++)
		packet [pp] = (word) entropy;

	tcv_endp (packet);

  entry (SN_NEXT+1)

	ser_outf (SN_NEXT+1, "SND %lu, len = %d\r\n", last_snt, packet_length);
	proceed (SN_SEND);

endprocess (1)

int snd_start (int del) {

	tdelay = del;
	tkillflag = 0;
	last_ack = last_snt;

	tcv_control (sfd, PHYSOPT_TXON, NULL);
	if (!running (sender)) {
		fork (sender, NULL);
		XMTon = 1;
		return 1;
	}

	return 0;
}

int snd_stop (void) {

	if (running (sender)) {
		tkillflag = 1;
		tcv_control (sfd, PHYSOPT_TXOFF, NULL);
		trigger ((word) &tkillflag);
		XMTon = 0;
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
#define	RS_QRCV		50
#define	RS_QXMT		60
#define	RS_QUIT		70
#define	RS_SSID		80

process (root, int)

	static char *ibuf;
	static int k, n1;
	static char *fmt, obuf [32];
	static word p [2];

  entry (RS_INIT)

	ibuf = (char*) umalloc (IBUFLEN);
	phys_cc1100 (0, MAXPLEN);
	tcv_plug (0, &plug_null);
	sfd = tcv_open (NONE, 0, 0);
	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

  entry (RS_RCMD-2)

	ser_out (RS_RCMD-2,
		"\r\nRF Ping Test\r\n"
		"Commands:\r\n"
		"s intvl  -> start/reset sending interval (2 secs default)\r\n"
		"r        -> start receiver\r\n"
		"o        -> stop receiver\r\n"
		"t        -> stop transmitter\r\n"
		"q        -> stop both\r\n"
		"i        -> set station Id\r\n"
		);


  entry (RS_RCMD)

	k = ser_in (RS_RCMD, ibuf, IBUFLEN-1);

	if (ibuf [0] == 's')
		proceed (RS_SND);
	if (ibuf [0] == 'r')
		proceed (RS_RCV);
	if (ibuf [0] == 'q')
		proceed (RS_QUIT);
	if (ibuf [0] == 'o')
		proceed (RS_QRCV);
	if (ibuf [0] == 't')
		proceed (RS_QXMT);
	if (ibuf [0] == 'i')
		proceed (RS_SSID);

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

  entry (RS_SSID)

	n1 = 0;
	scan (ibuf + 1, "%d", &n1);
	tcv_control (sfd, PHYSOPT_SETSID, (address) &n1);
	proceed (RS_RCMD);

endprocess (1)

#undef	RS_INIT	
#undef	RS_RCMD
#undef	RS_SND
#undef 	RS_RCV
#undef	RS_QRCV
#undef	RS_QXMT
#undef	RS_QUIT
#undef	RS_SSID
