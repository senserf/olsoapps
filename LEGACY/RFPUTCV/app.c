/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

// Test Line-mode UART over TCV + RFPing

#include "sysio.h"
#include "tcvphys.h"
#include "phys_uart.h"

#ifdef	PIN_TEST
#include "pinopts.h"
#endif

#define	ENCRYPT	0
#define	MIN_PACKET_LENGTH	24

#if RF24L01
#define	MAX_PACKET_LENGTH	30
#else
#define	MAX_PACKET_LENGTH	42
#endif

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

#include "form.h"

#if CC1100
#include "phys_cc1100.h"
#endif

#if DM2200
#include "phys_dm2200.h"
#endif

#include "plug_null.h"

#if ENCRYPT
#include "encrypt.h"

static const lword secret [4] = { 0xbabadead,0x12345678,0x98765432,0x6754a6cd };

#endif

#define	PKT_ACK		0x1234
#define	PKT_DAT		0xABCD

#define	ACK_LENGTH	12
#define MAXPLEN		(MAX_PACKET_LENGTH + 2)

static const char imess [] =
		"RF Ping Test\n"
		"Commands:\n"
		"s intvl  -> start/reset sending interval (2 secs default)\n"
		"r        -> start receiver\n"
		"d i v    -> change phys parameter i to v\n"
		"p v      -> set transmit power\n"
		"g        -> get received power\n"
		"o        -> stop receiver\n"
		"t        -> stop transmitter\n"
		"q        -> stop both\n"
		"i        -> set station Id\n"
#ifdef PIN_TEST
		"x p r    -> read ADC pin 'p' with reference r (0/1 1.5V/2.5V)\n"
		"y p v    -> set pin 'p' to v (0/1)\n"
		"z p      -> show the value of pin 'p'\n"
#endif

#if SDRAM_PRESENT
		"M b n    -> SDRAM test: n kwds, b bufsize\n"
#endif

#if STACK_GUARD
		"v        -> show unused stack space\n"
#endif

#if UART_RATE_SETTABLE
		"S r      -> set UART rate\n"
		"G        -> get UART rate\n"
#endif

#ifdef	BATTERY_TEST
		"B        -> battery test\n"
#endif
		;

static int sfd, ufd;

#define	RC_TRY		00
#define	RC_DATA		10
#define	RC_SACK		20
#define	RC_ACK		30

static	int	rkillflag = 0;
static	lint	last_snt, last_rcv, last_ack;
static  char 	XMTon = 0, RCVon = 0;

static wuart (word st, const char *fm, ... ) {

	word len;
	address packet;

	// Calculate the length
	len = vfsize (fm, va_par (fm));
	packet = tcv_wnp (st, ufd, len);
	vform ((char*)packet, fm, va_par (fm));
	tcv_endp (packet);
}

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
#if ENCRYPT
	decrypt (packet + 1, (tcv_left (packet) >> 1) - 2, secret);
#endif
	if (packet [1] == PKT_ACK) {
		last_ack = ntowl (((lword*)packet) [1]);
		proceed (RC_ACK);
	}

	// Data packet
	last_rcv = ntowl (((lword*)packet) [1]);

  entry (RC_DATA)

	wuart (RC_DATA, "RCV: [%x] %lu (len = %d), pow = %d qua = %d",
		packet [1], last_rcv, tcv_left (packet) - 2,
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
#if ENCRYPT
		encrypt (packet + 1, 4, secret);
#endif
		tcv_endp (packet);
	}
	proceed (RC_TRY);

  entry (RC_ACK)

	wuart (RC_ACK, "ACK: %lu (len = %d)", last_ack, tcv_left (packet));

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

#if ENCRYPT
	encrypt (packet + 1, pl-1, secret);
#endif
	tcv_endp (packet);

  entry (SN_NEXT+1)

	wuart (SN_NEXT+1, "SND %lu, len = %d", last_snt, packet_length);
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
#define	RS_CAL		80
#define	RS_STK		85
#define	RS_GADC		90
#define	RS_SPIN		100
#define	RS_GPIN		110
#define	RS_URS		120
#define	RS_URG		130
#define	RS_SDRAM	140
#define	RS_BTS		150

const static word parm_power = 255;

thread (root)

	static address pbuf, qbuf;
#define	ibuf ((char*)pbuf)

	static int k, n1;
	static const char *fmt; static char obuf [32];
	static word p [2];

#if SDRAM_PRESENT
	static word *mbuf, n, m, bp;
	static lword nw, i, j;
#endif

  entry (RS_INIT)

#if DM2200
	phys_dm2200 (0, MAXPLEN);
#endif

#if CC1100
	phys_cc1100 (0, MAXPLEN);
#endif

	phys_uart (1, 80, 0);
	tcv_plug (0, &plug_null);
	sfd = tcv_open (NONE, 0, 0);
	ufd = tcv_open (NONE, 1, 0);

	if (sfd < 0) {
		diag ("Cannot open RF interface");
		halt ();
	}

	if (ufd < 0) {
		diag ("Cannot open UART interface");
		halt ();
	}

	tcv_control (sfd, PHYSOPT_SETPOWER, (address) &parm_power);
	tcv_control (ufd, PHYSOPT_TXON, NULL);
	tcv_control (ufd, PHYSOPT_RXON, NULL);


  entry (RS_RCMD-3)

	// Send out the menu message
	fmt = imess;

  entry (RS_RCMD-2)

	// Where to end
	while (*fmt == '\n')
		fmt++;

	if (*fmt == '\0')
		proceed (RS_RCMD);

	// Find the end of line
	for (k = 0; fmt [k] != '\n' && fmt [k] != '\0'; k++);

  entry (RS_RCMD-1)

	qbuf = tcv_wnp (RS_RCMD-1, ufd, k + 2);
	strncpy ((char*)qbuf, fmt, k);
	tcv_endp (qbuf);
	fmt += k;
	proceed (RS_RCMD-2);

  entry (RS_RCMD)

	// Read UART packet
	if (pbuf) {
		tcv_endp (pbuf);
		pbuf = NULL;
	}
	pbuf = tcv_rnp (RS_RCMD, ufd);

	if (ibuf [0] == 's')
		proceed (RS_SND);
	if (ibuf [0] == 'r')
		proceed (RS_RCV);
#ifdef PIN_TEST
	if (ibuf [0] == 'x')
		proceed (RS_GADC);
	if (ibuf [0] == 'y')
		proceed (RS_SPIN);
	if (ibuf [0] == 'z')
		proceed (RS_GPIN);
#endif

#if SDRAM_PRESENT
	if (ibuf [0] == 'M')
		proceed (RS_SDRAM);
#endif

#if STACK_GUARD
	if (ibuf [0] == 'v')
		proceed (RS_STK);
#endif

#if UART_RATE_SETTABLE
	if (ibuf [0] == 'S')
		proceed (RS_URS);
	if (ibuf [0] == 'G')
		proceed (RS_URG);
#endif

#ifdef BATTERY_TEST
	if (ibuf [0] == 'B')
		proceed (RS_BTS);
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

  entry (RS_RCMD+1)

	wuart (RS_RCMD+1, "Illegal command or parameter");
	proceed (RS_RCMD-3);

  entry (RS_SND)

	/* Default */
	n1 = 2048;
	scan (ibuf + 1, "%d", &n1);
	if (n1 < 16)
		n1 = 16;
	snd_start (n1);

  entry (RS_SND+1)

	wuart (RS_SND+1, "Sender rate: %d", n1);
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

	wuart (RS_POW+1, "Transmitter power set to %d", n1);

	proceed (RS_RCMD);

  entry (RS_RCP)

	n1 = tcv_control (sfd, PHYSOPT_GETPOWER, NULL);

  entry (RS_RCP+1)

	wuart (RS_RCP+1, "Received power is %d", n1);

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
		fmt = "Stopped: %s";
	else
		fmt = "No process stopped";

  entry (RS_QUIT+1)

	wuart (RS_QUIT+1, fmt, obuf);

	proceed (RS_RCMD);

  entry (RS_SSID)

	n1 = 0;
	scan (ibuf + 1, "%d", &n1);
	tcv_control (sfd, PHYSOPT_SETSID, (address) &n1);
	proceed (RS_RCMD);

#if STACK_GUARD

  entry (RS_STK)

	wuart (RS_STK, "Free stack space = %d words", stackfree ());
	proceed (RS_RCMD);
#endif

#ifdef PIN_TEST

  entry (RS_GADC)

	p [0] = 0;
	p [1] = 0;
	scan (ibuf + 1, "%u %u", p+0, p+1);

  entry (RS_GADC+1)

	p [0] = pin_read_adc (RS_GADC+1, p [0], p [1], 4);

  entry (RS_GADC+2)

	wuart (RS_GADC+2, "Value: %u", p [0]);
	proceed (RS_RCMD);

  entry (RS_SPIN)

	p [1] = 0;
	p [1] = 0;
	scan (ibuf + 1, "%u %u", p+0, p+1);
	pin_write (p [0], p [1]);
	proceed (RS_RCMD);

  entry (RS_GPIN)

	p [0] = 1;
	scan (ibuf + 1, "%u", p+0);
	p [0] = pin_read (p [0]);

  entry (RS_GPIN+1)

	wuart (RS_GADC+1, "Value: %u", p [0]);
	proceed (RS_RCMD);

#endif

#if UART_RATE_SETTABLE

  entry (RS_URS)

	scan (ibuf + 1, "%d", &p [0]);
	tcv_control (ufd, PHYSOPT_SETRATE, p);
	proceed (RS_RCMD);

  entry (RS_URG)

	tcv_control (ufd, PHYSOPT_GETRATE, p);

  entry (RS_URG+1)
	
	wuart (RS_URG+1, "Rate: %u[00]", p [0]);
	proceed (RS_RCMD);
#endif

#if SDRAM_PRESENT

  entry (RS_SDRAM)

	if (scan (ibuf + 1, "%u %u", &m, &n) != 2)
		proceed (RS_RCMD+1);

	if (m == 0 || n == 0)
		proceed (RS_RCMD+1);

	mbuf = umalloc (m + m);

  entry (RS_SDRAM+1)

	wuart (RS_SDRAM+1, "Testing %u Kwords using a %u word buffer",
		n, m);

	delay (512, RS_SDRAM+2);
	release;

  entry (RS_SDRAM+2);

	nw = (lword)n * 1024;

	bp = 0;
	for (i = j = 0; i < nw; i++) {
		mbuf [bp++] = (word) (i + 1);
		if (bp == m) {
			/* Flush the buffer */
			ramput (j, mbuf, bp);
			j += bp;
			bp = 0;
		}
	}

	if (bp)
		/* The tail */
		ramput (j, mbuf, bp);

  entry (RS_SDRAM+3)

	wuart (RS_SDRAM+3, "Writing complete");

	delay (512, RS_SDRAM+4);
	release;

  entry (RS_SDRAM+4)

	for (i = 0; i < nw; ) {
		bp = (nw - i > m) ? m : (word) (nw - i);
		ramget (mbuf, i, bp);
		j = i + bp;
		bp = 0;
		while (i < j) {
			if (mbuf [bp] != (word) (i + 1))
				proceed (RS_SDRAM+6);
			bp++;
			i++;
		}
	}

  entry (RS_SDRAM+5)

	wuart (RS_SDRAM+5, "Test OK %x", mbuf [0]);
	ufree (mbuf);
	proceed (RS_RCMD);

  entry (RS_SDRAM+6)

	wuart (RS_SDRAM+6, "Error: %x%x %x -> %x",
				(word)((i >> 16) & 0xffff),
				(word)( i        & 0xffff),
				mbuf [bp],
				(word) (i + 1));
	ufree (mbuf);
	proceed (RS_RCMD);

#endif	/* SDRAM_PRESENT */

#ifdef	BATTERY_TEST

  entry	(RS_BTS)

	k = battery ();

  entry (RS_BTS+1)

	wuart (RS_BTS+1, "Battery status: %d", k);
	proceed (RS_RCMD);
#endif

endthread
