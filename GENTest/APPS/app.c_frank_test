/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
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

#if	CC1100
#include "phys_cc1100.h"
#endif

#if	DM2200
#include "phys_dm2200.h"
#endif

#define	IBUFLEN			64
#define	MIN_PACKET_LENGTH	20
#define	MAX_PACKET_LENGTH	32
#define	MAXPLEN			48

#define	RCV(a)	((a) [1])
#define	SER(a)	((a) [2])
#define	BOA(a)	((a) [3])

static	word	CntSent = 0, CntRcvd = 0;

static int sfd;

static word 	ME = 1,
		YOU = 1,
		ReceiverDelay = 0,
		BounceDelay = 0,
		CloneCount = 0,
		SendInterval = 1024,
		SendRndPat = 0,
		BounceBackoff = 0,
		SendRnd = 0,
		BID = 0;

static byte	Action = 1, Channel = 0, Mode = 0, SndRnd = 0, BkfRnd;

static word gen_packet_length (void) {

	return ((rnd () % (MAX_PACKET_LENGTH - MIN_PACKET_LENGTH + 1)) +
			MIN_PACKET_LENGTH) & 0xFFE;
}

static word gen_backoff (void) {

	return (rnd () & BounceBackoff);
}

static word send_delay (void) {

	int n;

	if (SendRnd == 0)
		return SendInterval;

	// Randomize

	if ((rnd () & 0x80))
		n = (int) (SendInterval + (rnd () & SendRndPat));
	else
		n = (int) (SendInterval - (rnd () & SendRndPat));

	if (n < 0)
		n = 0;

	return (word) n;
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
#if 0
		diag ("ME BAD: %x", (word)p);
		dmp_mem ();
		tcv_dumpqueues ();
#endif
		return TCV_DSP_DROP;
	}

	if (len < MIN_PACKET_LENGTH || len > MAX_PACKET_LENGTH) {
		diag ("ME OK, PL BAD: %x", (word)p);
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

#define	RC_WAIT		0
#define	RC_DISP		1

process (receiver, void)

	static address packet;
	static word SerNum = 0;
	word len, pow;

	nodata;

  entry (RC_WAIT)

	packet = tcv_rnp (RC_WAIT, sfd);

  entry (RC_DISP)

	leds (2, 1);
	len = tcv_left (packet) - 2;
	pow = packet [len >> 1];
	diag ("R: len = %u, sn = %u", len, SER (packet));
	SerNum = SER (packet) + 1;

#if 0
	if (RCV (packet) != ME || tcv_left (packet) < 18) {
		dmp_mem ();
		tcv_dumpqueues ();
	}
#endif

	tcv_endp (packet);
	CntRcvd++;
	diag ("S: %d, R %d", CntSent, CntRcvd);
	leds (2, 0);

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

	leds (1, 1);

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
	BOA (packet) = BID;

	for (i = 8; i < PLen; i++)
		((byte*) packet) [i] = (byte)i;

	tcv_endp (packet);
	diag ("SNT: %u [%u]", Sernum, PLen);
	CntSent++;
	Sernum ++;
	leds (1, 0);

	delay (send_delay (), SN_SEND);

endprocess (1)

#define	BN_WAIT		0
#define	BN_SEND		2

process (bouncer, void)

	static address packet;
	address outpacket;
	word	bkf;

  entry (BN_WAIT)

	leds (2, 0);

	packet = tcv_rnp (BN_WAIT, sfd);
	if (RCV (packet) != ME) {
		tcv_endp (packet);
		proceed (BN_WAIT);
	}

  entry (BN_SEND)

	leds (2, 1);
	outpacket = tcv_wnp (BN_SEND, sfd, tcv_left (packet));
	memcpy ((char*) outpacket, (char*) packet, tcv_left (packet));
	tcv_endp (packet);
	RCV (outpacket) = YOU;
	BOA (outpacket) = BID;

	if (BounceDelay)
		mdelay (BounceDelay);

	if (BounceBackoff) {
		bkf = gen_backoff ();
		tcv_control (sfd, PHYSOPT_CAV, &bkf);
	}

	tcv_endp (outpacket);
	proceed (BN_WAIT);

endprocess (1)

void do_start (int mode) {

	if (mode & 2) {
		if (!running (receiver))
			fork (receiver, NULL);
		tcv_control (sfd, PHYSOPT_RXON, NULL);
	}

	if (mode & 1) {
		if (!running (sender))
			fork (sender, NULL);
		tcv_control (sfd, PHYSOPT_TXON, NULL);
	}

	if (mode == 4) {
		if (!running (bouncer))
			fork (bouncer, NULL);
		tcv_control (sfd, PHYSOPT_RXON, NULL);
		tcv_control (sfd, PHYSOPT_TXON, NULL);
	}
}

void do_quit () {

	tcv_control (sfd, PHYSOPT_RXOFF, NULL);
	tcv_control (sfd, PHYSOPT_TXOFF, NULL);
	killall (receiver);
	killall (sender);
	killall (bouncer);
}

#if	PULSE_MONITOR

void	out_pmon_state () {

	lword lw;

	diag ("PMON STATE: %x", pmon_get_state ());
	lw = pmon_get_cnt ();
	diag ("CNT: %x %x",
		(word)((lw >> 16) & 0xffff),
		(word)((lw      ) & 0xffff) );
	lw = pmon_get_cmp ();
	diag ("CMP: %x %x",
		(word)((lw >> 16) & 0xffff),
		(word)((lw      ) & 0xffff) );
}

#define	PM_START	0
#define	PM_NOTIFIER	1
#define	PM_COUNTER	2

process (pulse_monitor, void)

	entry (PM_START)

		if (pmon_pending_not ())
			diag ("NOTIFIER PENDING 1");

		if (pmon_pending_cmp ())
			diag ("COUNTER PENDING 1");

		wait (PMON_NOTEVENT, PM_NOTIFIER);
		wait (PMON_CNTEVENT, PM_COUNTER);
		release;

	entry (PM_NOTIFIER)

		diag ("NOTIFIER EVENT");
		out_pmon_state ();
		pmon_pending_not ();
		proceed (PM_START);

	entry (PM_COUNTER);

		diag ("COMPARATOR EVENT");
		out_pmon_state ();
		pmon_pending_cmp ();
		proceed (PM_START);
endprocess (1)

#endif	/* PULSE_MONITOR */

#define	RS_INIT		00

process (root, int)

	static char *ibuf;
	int n, v;
	byte ba [4];
	static word a, b;
	long lw;

  entry (RS_INIT)

	ibuf = (char*) umalloc (IBUFLEN);
#if CC1100
	phys_cc1100 (0, MAXPLEN);
#endif
#if DM2200
	phys_dm2200 (0, MAXPLEN);
#endif
	tcv_plug (0, &plug_test);
	sfd = tcv_open (NONE, 0, 0);
	if (sfd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}

#if	PULSE_MONITOR

	fork (pulse_monitor, NULL);
#endif

	pin_write (8, 2);

	if (pin_read (8) & 1) {
		// The bouncer
		diag ("BNC");
		leds (0, 1);
		do_start (4);
	} else {
		diag ("XCV");
		leds (0, 2);
		do_start (3);
	}

	finish;

endprocess (1);
