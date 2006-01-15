/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "tcvphys.h"
#include "tcvplug.h"

heapmem {10, 90};

void	dmp_mem (void);
void	tcv_dumpqueues (void);

#include "ser.h"
#include "serf.h"
#include "form.h"

#include "phys_cc1100.h"

#define	IBUFLEN			24
#define	MIN_PACKET_LENGTH	20
#define	MAX_PACKET_LENGTH	32
#define	MAXPLEN			48

#define	RCV(a)	((a) [1])
#define	SER(a)	((a) [2])

static int sfd;

static word ME, YOU, ReceiverDelay = 0, CloneCount = 1, SendInterval = 1024;

static word rndseed = 12345;

#define	rnd_cycle 	rndseed = (entropy + 1 + rndseed) * 6751

static word gen_packet_length (void) {

	rnd_cycle;
	return ((rndseed % (MAX_PACKET_LENGTH - MIN_PACKET_LENGTH + 1)) +
			MIN_PACKET_LENGTH) & 0xFFE;
}

static word delta (void) {

	rnd_cycle;

	if ((rndseed & 0x100))
		return (rndseed | 0xfff0) + 1;
	else
		return (rndseed & 0x000f);
}

static int tcv_ope (int, int, va_list);
static int tcv_clo (int, int);
static int tcv_rcv (int, address, int, int*, tcvadp_t*);
static int tcv_frm (address, int, tcvadp_t*);
static int tcv_out (address);
static int tcv_xmt (address);

const tcvplug_t plug_test =
		{ tcv_ope, tcv_clo, tcv_rcv, tcv_frm, tcv_out, tcv_xmt, NULL,
			0x0011 /* Plugin Id */ };

static int *desc = NULL;

static int tcv_ope (int phy, int fd, va_list plid) {
/*
 * This is very simple - we are allowed to have one descriptor per phy.
 */
	int i;

	if (desc == NULL) {
		desc = (int*) umalloc (sizeof (int) * TCV_MAX_PHYS);
		if (desc == NULL)
			syserror (EMALLOC, "plug_null tcv_ope");
		for (i = 0; i < TCV_MAX_PHYS; i++)
			desc [i] = NONE;
	}

	/* phy has been verified by TCV */
	if (desc [phy] != NONE)
		return ERROR;

	desc [phy] = fd;
	return 0;
}

static int tcv_clo (int phy, int fd) {

	/* phy/fd has been verified */

	if (desc == NULL || desc [phy] != fd)
		return ERROR;

	desc [phy] = NONE;
	return 0;
}

static int tcv_rcv (int phy, address p, int len, int *ses, tcvadp_t *bounds) {

	int i;
	address dup;

	// Simulate processing time
	mdelay (len);

	if (desc == NULL || (*ses = desc [phy]) == NONE)
		return TCV_DSP_PASS;

	if (RCV (p) != ME) {
		diag ("ME BAD: %x", (word)p);
		dmp_mem ();
		tcv_dumpqueues ();
		return TCV_DSP_DROP;
	}

	if (len < MIN_PACKET_LENGTH || len > MAX_PACKET_LENGTH) {
		diag ("ME OK: %x", (word)p);
		dmp_mem ();
		tcv_dumpqueues ();
		goto SkipClone;
	}

	// Clone the packet
	for (i = 0; i < CloneCount; i++) {
        	if ((dup = tcvp_new (len, TCV_DSP_XMT, *ses)) == NULL) {
	        	diag ("Clone failed");
        	} else {
            		memcpy ((char*) dup, (char*) p, len);
	        } 
	}

SkipClone:

	bounds->head = bounds->tail = 0;

	return TCV_DSP_RCV;
}

static int tcv_frm (address p, int phy, tcvadp_t *bounds) {

	return bounds->head = bounds->tail = 0;
}

static int tcv_out (address p) {

	return TCV_DSP_XMT;

}

static int tcv_xmt (address p) {

	return TCV_DSP_DROP;
}

/* ======================================================================= */

process (receiver, void)

	static address packet;
	static word SerNum = 0;
	word ser;

	nodata;

  entry (0)

	packet = tcv_rnp (0, sfd);
	ser = SER (packet);

	if (ser != SerNum) {
		diag ("RCV(E): %d len = %u, sn = %u [%u]", RCV (packet),
			tcv_left (packet), ser, SerNum);
	} else {
		diag ("RCV: %d len = %u, sn = %u", RCV (packet),
			tcv_left (packet), ser);
	}

	SerNum = ser + 1;

	if (RCV (packet) != ME || tcv_left (packet) < 18) {
		dmp_mem ();
		tcv_dumpqueues ();
	}

	tcv_endp (packet);

	delay (ReceiverDelay, 0);

endprocess (1)

#define	SN_SEND		00
#define	SN_NEXT		10

process (sender, void)

	static word PLen, Sernum;
	address packet;
	int i;
	word w;

	nodata;

  entry (SN_SEND)

	PLen = gen_packet_length ();
	if (PLen < MIN_PACKET_LENGTH)
		PLen = MIN_PACKET_LENGTH;
	else if (PLen > MAX_PACKET_LENGTH)
		PLen = MAX_PACKET_LENGTH;

  entry (SN_NEXT)

	packet = tcv_wnp (SN_NEXT, sfd, PLen);
	// Network ID
	packet [0] = 0;

	RCV (packet) = YOU;
	SER (packet) = Sernum;

	for (i = 6; i < PLen; i++)
		((byte*) packet) [i] = (byte)i;

	tcv_endp (packet);
	diag ("SNT: %u [%u]", Sernum, PLen);
	Sernum ++;

	delay (SendInterval + delta (), SN_SEND);

endprocess (1)

void do_start (int mode) {

	if (mode == 0 || mode == 2) {
		if (!running (receiver))
			fork (receiver, NULL);
		tcv_control (sfd, PHYSOPT_RXON, NULL);
	}

	if (mode == 1 || mode == 2) {
		if (!running (sender))
			fork (sender, NULL);
		tcv_control (sfd, PHYSOPT_TXON, NULL);
	}
}

void do_quit () {

	tcv_control (sfd, PHYSOPT_RXOFF, NULL);
	tcv_control (sfd, PHYSOPT_TXOFF, NULL);
	killall (receiver);
	killall (sender);
}

#define	RS_INIT		00
#define	RS_RCMD		10
#define	RS_SOI		20
#define	RS_DON		30
#define	RS_SYI		40
#define	RS_SCC		50
#define	RS_SCI		60
#define	RS_SRD		65
#define	RS_STA		70
#define	RS_DUM		75
#define	RS_QUI		80

process (root, int)

	static char *ibuf;
	int n;

  entry (RS_INIT)

	ibuf = (char*) umalloc (IBUFLEN);
	phys_cc1100 (0, MAXPLEN);
	tcv_plug (0, &plug_test);
	sfd = tcv_open (NONE, 0, 0);
	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

  entry (RS_RCMD-2)

	ser_out (RS_RCMD-2,
		"\r\nTCV Clone Test\r\n"
		"Commands:\r\n"
		"m n      -> set own ID\r\n"
		"y n      -> set other ID\r\n"
		"c n      -> set clone count\r\n"
		"i n      -> set send interval (msec)\r\n"
		"d n      -> set receiver delay (msec)\r\n"
		"s n      -> start (0-rc, 1-xm, 2-both)\r\n"
		"q        -> stop\r\n"
		"f        -> ram dump\r\n"
	);

  entry (RS_RCMD)
  
	ibuf [0] = ' ';
	ser_in (RS_RCMD, ibuf, IBUFLEN-1);

	if (ibuf [0] == 'm')
		proceed (RS_SOI);
	if (ibuf [0] == 'y')
		proceed (RS_SYI);
	if (ibuf [0] == 'c')
		proceed (RS_SCC);
	if (ibuf [0] == 'i')
		proceed (RS_SCI);
	if (ibuf [0] == 'd')
		proceed (RS_SRD);
	if (ibuf [0] == 's')
		proceed (RS_STA);
	if (ibuf [0] == 'q')
		proceed (RS_QUI);
	if (ibuf [0] == 'f')
		proceed (RS_DUM);

  entry (RS_RCMD+1)

	ser_out (RS_RCMD+1, "Illegal command or parameter\r\n");
	proceed (RS_RCMD-2);

  entry (RS_SOI)

	n = -1;
	scan (ibuf + 1, "%d", &n);
	if (n < 0)
		proceed (RS_RCMD+1);
	ME = n;
	
  entry (RS_DON)

	ser_out (RS_DON, "Done\r\n");
	proceed (RS_RCMD);

  entry (RS_SYI)

	n = -1;
	scan (ibuf + 1, "%d", &n);
	if (n < 0)
		proceed (RS_RCMD+1);
	YOU = n;
	proceed (RS_DON);

  entry (RS_SCC)

	n = -1;
	scan (ibuf + 1, "%d", &n);
	if (n < 0)
		proceed (RS_RCMD+1);
	CloneCount = n;
	proceed (RS_DON);

  entry (RS_SCI)

	n = -1;
	scan (ibuf + 1, "%d", &n);
	if (n <= 0)
		proceed (RS_RCMD+1);
	SendInterval = n;
	proceed (RS_DON);

  entry (RS_SRD)

	n = -1;
	scan (ibuf + 1, "%d", &n);
	if (n < 0)
		proceed (RS_RCMD+1);
	ReceiverDelay = n;
	proceed (RS_DON);

  entry (RS_STA)

	n = -1;
	scan (ibuf + 1, "%d", &n);
	if (n < 0 || n > 2)
		proceed (RS_RCMD+1);
	do_start (n);
	proceed (RS_DON);

  entry (RS_QUI)

	do_quit ();
	proceed (RS_DON);

  entry (RS_DUM)

	dmp_mem ();
	tcv_dumpqueues ();
	proceed (RS_DON);

endprocess (1)
