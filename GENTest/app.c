/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "tcvphys.h"
#include "tcvplug.h"

heapmem {10, 90};

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

static word gen_packet_length (void) {

	static	word	rndseed = 12345;

	rndseed = (entropy + 1 + rndseed) * 6751;
	return ((rndseed % (MAX_PACKET_LENGTH - MIN_PACKET_LENGTH + 1)) +
			MIN_PACKET_LENGTH) & 0xFFE;
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

	if (desc == NULL || (*ses = desc [phy]) == NONE)
		return TCV_DSP_PASS;

	if (RCV (p) != ME)
		return TCV_DSP_DROP;

	// Clone the packet
	for (i = 0; i < CloneCount; i++) {
        	if ((dup = tcvp_new (len-2, TCV_DSP_XMT, *ses)) == NULL) {
	        	diag ("Clone failed");
        	} else {
            		memcpy ((char*) dup, (char*) p, len-2);
	        } 
	}

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

	nodata;

  entry (0)

	packet = tcv_rnp (0, sfd);
	diag ("RCV: %d len = %u, sn = %u", RCV (packet), tcv_left (packet),
		SER (packet));

	tcv_endp (packet);

	delay (ReceiverDelay, 0);

endprocess (1)

#define	SN_SEND		00
#define	SN_NEXT		10

process (sender, void)

	static word PLen, Sernum;
	address packet;

	nodata;

  entry (SN_SEND)

	PLen = gen_packet_length ();

  entry (SN_NEXT)

	packet = tcv_wnp (SN_NEXT, sfd, PLen);
	// Network ID
	packet [0] = 0;

	RCV (packet) = YOU;
	SER (packet) = Sernum;
	tcv_endp (packet);
	diag ("SNT: %u [%u]", Sernum, PLen);
	Sernum ++;

	delay (SendInterval, SN_SEND);

endprocess (1)

void do_start () {

	if (!running (receiver))
		fork (receiver, NULL);

	if (!running (sender))
		fork (sender, NULL);

	tcv_control (sfd, PHYSOPT_RXON, NULL);
	tcv_control (sfd, PHYSOPT_TXON, NULL);
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
#define	RS_QUI		80

process (root, int)

	static char *ibuf;
	word n;

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
		"i n      -> set send interval (seconds)\r\n"
		"d n      -> set receiver delay\r\n"
		"s        -> start\r\n"
		"q        -> stop\r\n"
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
	SendInterval = n * 1024;
	proceed (RS_DON);

  entry (RS_SRD)

	n = -1;
	scan (ibuf + 1, "%d", &n);
	if (n < 0)
		proceed (RS_RCMD+1);
	ReceiverDelay = n * 1024;
	proceed (RS_DON);

  entry (RS_STA)

	do_start ();
	proceed (RS_DON);

  entry (RS_QUI)

	do_quit ();
	proceed (RS_DON);

endprocess (1)
