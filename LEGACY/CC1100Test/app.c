/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

//
// This is RFPING with added hooks to test switchable rates for CC1100
//

#include "sysio.h"
#include "tcvphys.h"

#define	MIN_PACKET_LENGTH	24
#define	MAX_PACKET_LENGTH	32

heapmem {10, 90};

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
static	lint	last_snt, last_rcv, last_ack;
static  char 	XMTon = 0, RCVon = 0;

static word gen_packet_length (void) {

#if MIN_PACKET_LENGTH >= MAX_PACKET_LENGTH
	return MIN_PACKET_LENGTH;
#else
	return ((rnd () % (MAX_PACKET_LENGTH - MIN_PACKET_LENGTH + 1)) +
			MIN_PACKET_LENGTH) & 0xFFE;
#endif

}

thread (receiver)

	static address packet;

  entry (RC_TRY)

	if (rkillflag) {
		rkillflag = 0;
		finish;
	}
	wait (&rkillflag, RC_TRY);
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
#if LITTLE_ENDIAN
		((byte*)packet) [tcv_left (packet) - 1],
		((byte*)packet) [tcv_left (packet) - 2]
#else
		((byte*)packet) [tcv_left (packet) - 2],
		((byte*)packet) [tcv_left (packet) - 1]
#endif
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
	trigger (&last_ack);
	proceed (RC_TRY);

endthread

int rcv_start (void) {

	rkillflag = 0;
	tcv_control (sfd, PHYSOPT_RXON, NULL);
	if (!running (receiver)) {
		runthread (receiver);
		RCVon = 1;
		return 1;
	}
	return 0;
}

int rcv_stop (void) {

	if (running (receiver)) {
		tcv_control (sfd, PHYSOPT_RXOFF, NULL);
		rkillflag = 1;
		trigger (&rkillflag);
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

thread (sender)

	static address packet;
	static word packet_length = 12;

	word pl;
	int  pp;

  entry (SN_SEND)

	if (tkillflag) {
		tkillflag = 0;
		finish;
	}
	wait (&tkillflag, SN_SEND);

	if (last_ack != last_snt) {
		delay (tdelay, SN_NEXT);
		wait (&last_ack, SN_SEND);
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

	if (tkillflag) {
		tkillflag = 0;
		finish;
	}
	wait (&tkillflag, SN_SEND);

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

endthread

int snd_start (int del) {

	tdelay = del;
	tkillflag = 0;
	last_ack = last_snt;

	tcv_control (sfd, PHYSOPT_TXON, NULL);
	if (!running (sender)) {
		runthread (sender);
		XMTon = 1;
		return 1;
	}

	return 0;
}

int snd_stop (void) {

	if (running (sender)) {
		tkillflag = 1;
		tcv_control (sfd, PHYSOPT_TXOFF, NULL);
		trigger (&tkillflag);
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
#define	RS_POW		50
#define	RS_RCP		60
#define	RS_QRCV		63
#define	RS_QXMT		66
#define	RS_QUIT		70
#define	RS_SSID		75
#define	RS_RATE		90
#define	RS_URS		120
#define	RS_URG		130
#define	RS_AUTOSTART	200

const static word parm_power = 255;

thread (root)

	static char *ibuf;
	static int k, n1;
	static char *fmt, obuf [32];
	static word p [2];

  entry (RS_INIT)

	ibuf = (char*) umalloc (IBUFLEN);
	ibuf [0] = 0xff;
#if 0
	ibuf [0] = 0;
#endif

	phys_cc1100 (0, MAXPLEN);
	tcv_plug (0, &plug_null);
	sfd = tcv_open (NONE, 0, 0);
	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

	tcv_control (sfd, PHYSOPT_SETPOWER, (address) &parm_power);

  entry (RS_RCMD-2)

	ser_out (RS_RCMD-2,
		"\r\nRF Ping Test\r\n"
		"Commands:\r\n"
		"s intvl  -> start/reset sending interval (2 secs default)\r\n"
		"r        -> start receiver\r\n"
		"p v      -> set transmit power\r\n"
		"g        -> get received power\r\n"
		"o        -> stop receiver\r\n"
		"t        -> stop transmitter\r\n"
		"q        -> stop both\r\n"
		"i        -> set station Id\r\n"
		"R        -> select rate (0, 1, 2, 3)\r\n"
#if UART_RATE_SETTABLE
		"S r      -> set UART rate\r\n"
		"G        -> get UART rate\r\n"
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

	if (ibuf [0] == 's')
		proceed (RS_SND);
	if (ibuf [0] == 'r')
		proceed (RS_RCV);
#if UART_RATE_SETTABLE
	if (ibuf [0] == 'S')
		proceed (RS_URS);
	if (ibuf [0] == 'G')
		proceed (RS_URG);
#endif
	if (ibuf [0] == 'p')
		proceed (RS_POW);
	if (ibuf [0] == 'g')
		proceed (RS_RCP);
	if (ibuf [0] == 'q')
		proceed (RS_QUIT);
	if (ibuf [0] == 'o')
		proceed (RS_QRCV);
	if (ibuf [0] == 't')
		proceed (RS_QXMT);
	if (ibuf [0] == 'i')
		proceed (RS_SSID);
	if (ibuf [0] == 'R')
		proceed (RS_RATE);

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

  entry (RS_POW)

	/* Default */
	n1 = 0;
	scan (ibuf + 1, "%d", &n1);
	tcv_control (sfd, PHYSOPT_SETPOWER, (address)&n1);

  entry (RS_POW+1)

	ser_outf (RS_POW+1,
		"Transmitter power set to %d\r\n", n1);

	proceed (RS_RCMD);

  entry (RS_RCP)

	n1 = tcv_control (sfd, PHYSOPT_GETPOWER, NULL);

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

  entry (RS_SSID)

	n1 = 0;
	scan (ibuf + 1, "%d", &n1);
	tcv_control (sfd, PHYSOPT_SETSID, (address) &n1);
	proceed (RS_RCMD);

  entry (RS_RATE)

	p [0] = 0;
	scan (ibuf + 1, "%d", p+0);
	if (p [0] > 3)
		proceed (RS_RCMD+1);

	tcv_control (sfd, PHYSOPT_SETRATE, (address)p);

  entry (RS_RATE+1)

	ser_outf (RS_RATE+1, "Rate set to variant %d\r\n", p [0]);
	proceed (RS_RCMD);

#if UART_RATE_SETTABLE

  entry (RS_URS)

	scan (ibuf + 1, "%d", &p [0]);
	ion (UART, CONTROL, (char*) p, UART_CNTRL_SETRATE);
	proceed (RS_RCMD);

  entry (RS_URG)

	ion (UART, CONTROL, (char*) p, UART_CNTRL_GETRATE);

  entry (RS_URG+1)
	
	ser_outf (RS_URG+1, "Rate: %u[00]\r\n", p [0]);
	proceed (RS_RCMD);
#endif

  entry (RS_AUTOSTART)
	  
	snd_start (1024);
  	rcv_start ();

  	finish;

endthread
