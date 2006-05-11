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

#if	DM2100
#include "phys_dm2100.h"
#endif

#include "pinopts.h"

#define	IBUFLEN			64
#define	MIN_PACKET_LENGTH	20
#define	MAX_PACKET_LENGTH	32
#define	MAXPLEN			48

#define	RCV(a)	((a) [1])
#define	SER(a)	((a) [2])
#define	BOA(a)	((a) [3])

static	lword	CntSent = 0, CntRcvd = 0;

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

	len = tcv_left (packet) - 2;
	pow = packet [len >> 1];
	if (SER (packet) != SerNum) 
		ser_outf (RC_DISP,
		    "RCV(E): <%x> B%d %d len = %u, sn = %u [%u]\r\n",
			pow,
			BOA (packet),
			RCV (packet), len, SER (packet), SerNum);
	else
		ser_outf (RC_DISP,
		    "RCV(K): <%x> B%d %d len = %u, sn = %u\r\n",
			pow,
			BOA (packet),
			RCV (packet), len, SER (packet));

	SerNum = SER (packet) + 1;

#if 0
	if (RCV (packet) != ME || tcv_left (packet) < 18) {
		dmp_mem ();
		tcv_dumpqueues ();
	}
#endif

	tcv_endp (packet);
	CntRcvd++;

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
	BOA (packet) = BID;

	for (i = 8; i < PLen; i++)
		((byte*) packet) [i] = (byte)i;

	tcv_endp (packet);
	diag ("SNT: %u [%u]", Sernum, PLen);
	CntSent++;
	Sernum ++;

	delay (send_delay (), SN_SEND);

endprocess (1)

#define	BN_WAIT		0
#define	BN_SEND		2

process (bouncer, void)

	static address packet;
	address outpacket;
	word	bkf;

  entry (BN_WAIT)

	packet = tcv_rnp (BN_WAIT, sfd);
	if (RCV (packet) != ME) {
		tcv_endp (packet);
		proceed (BN_WAIT);
	}

  entry (BN_SEND)

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
#define	RS_RCMD		10
#define	RS_SOI		20
#define	RS_DON		30
#define	RS_SYI		40
#define	RS_SCC		50
#define	RS_SCI		60
#define	RS_SRC		62
#define	RS_SRD		65
#define RS_RPC		67
#define RS_SPC		68
#define RS_BID		69
#define	RS_STA		70
#define	RS_BKF		71
#define	RS_BND		73
#define	RS_DUM		75
#define	RS_ECO		78
#define	RS_SEC		80
#define	RS_SMO		86
#define	RS_SRE		87
#define	RS_PDO		95
#define	RS_FLW		100
#define	RS_FLR		110
#define	RS_FLE		120
#define	RS_SPI		130
#define	RS_GPI		140
#define	RS_ADC		150
#define	RS_CNT		160
#define	RS_SCN		170
#define	RS_SCM		180
#define	RS_NOT		190
#define	RS_CNS		200
#define	RS_FRE		210

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
#if DM2100
	phys_dm2100 (0, MAXPLEN);
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

	if (Action)
		do_start (Action);

  entry (RS_RCMD-2)

//	ser_outf (RS_RCMD-2,
diag (
	"\r\nTCV Clone Test\r\n"
	"Commands:\r\n"
	"m n      -> set own ID [%u]\r\n"
	"y n      -> set other ID [%u]\r\n"
	"c n      -> set clone count [%u]\r\n"
	"i n      -> set send interval (msec) [%u]\r\n"
	"j n      -> randomized component of send int (LS bits) [%u]\r\n"
	"d n      -> set receiver delay (msec) [%u]\r\n"
	"s n      -> start (0-stop, 1-xm, 2-rcv, 3-both, 4-bnc) [%u]\r\n"
	"b n      -> bounce delay (fixed) msec [%u]\r\n"
	"e        -> reset packet counters\r\n"
	"p        -> show packet counters [%lu, %lu]\r\n"
	"q n      -> set board ID [%u]\r\n"
	"t n      -> set bounce backoff (LS bits) [%u]\r\n"
	"f        -> ram dump\r\n"
	"g xx..xx -> echo UART input to the terminal\r\n"
	"o n      -> set mode [%u]\r\n"
#if CC1100
	"k n      -> select channel n [%u]\r\n"
	"r n v    -> set CC1100 reg n to v\r\n"
#endif
	"u d      -> enter power-down mode for d seconds\r\n"
	"w adr 2  -> write word to info flash\r\n"
	"v adr    -> read word from info flash\r\n"
        "x        -> erase info flash\r\n"
	"a n v    -> set pin n to v (see pin_read.c)\r\n"
	"z n      -> read pin n\r\n"
	"l p d    -> read analog pin p with delay d\r\n"
#if PULSE_MONITOR
	"C v e    -> counter on, v = value, e = edge: rising = 1\r\n"
	"S        -> stop counter\r\n"
	"M v      -> set comparator to v (-1 turns off)\r\n"
	"N 1/0 e  -> switch notifier on/off edge\r\n"
	"O        -> show status\r\n"
#endif
#if GLACIER
	"F i      -> freeze\r\n"
#endif
	,
		ME, YOU, CloneCount, SendInterval, SendRnd, ReceiverDelay,
		Action, BounceDelay, CntSent, CntRcvd, BID, BkfRnd, Mode,
		Channel
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
	if (ibuf [0] == 'j')
		proceed (RS_SRC);
	if (ibuf [0] == 'd')
		proceed (RS_SRD);
	if (ibuf [0] == 's')
		proceed (RS_STA);
	if (ibuf [0] == 'b')
		proceed (RS_BND);
	if (ibuf [0] == 'f')
		proceed (RS_DUM);
	if (ibuf [0] == 'e')
		proceed (RS_RPC);
	if (ibuf [0] == 'p')
		proceed (RS_SPC);
	if (ibuf [0] == 'q')
		proceed (RS_BID);
	if (ibuf [0] == 't')
		proceed (RS_BKF);
	if (ibuf [0] == 'g')
		proceed (RS_ECO);
	if (ibuf [0] == 'o')
		proceed (RS_SMO);
#if CC1100
	if (ibuf [0] == 'k')
		proceed (RS_SEC);
	if (ibuf [0] == 'r')
		proceed (RS_SRE);
#endif
	if (ibuf [0] == 'u')
		proceed (RS_PDO);
	if (ibuf [0] == 'w')
		proceed (RS_FLW);
	if (ibuf [0] == 'v')
		proceed (RS_FLR);
	if (ibuf [0] == 'x')
		proceed (RS_FLE);
	if (ibuf [0] == 'a')
		proceed (RS_SPI);	// Set pin
	if (ibuf [0] == 'z')
		proceed (RS_GPI);	// Get pin
	if (ibuf [0] == 'l')
		proceed (RS_ADC);	// Get analog pin
#if 	PULSE_MONITOR
	if (ibuf [0] == 'C')
		proceed (RS_CNT);
	if (ibuf [0] == 'S')
		proceed (RS_SCN);
	if (ibuf [0] == 'M')
		proceed (RS_SCM);
	if (ibuf [0] == 'N')
		proceed (RS_NOT);
	if (ibuf [0] == 'O')
		proceed (RS_CNS);
#endif
#if	GLACIER
	if (ibuf [0] == 'F')
		proceed (RS_FRE);
#endif

  entry (RS_RCMD+1)

	ser_out (RS_RCMD+1, "Illegal command or parameter\r\n");
	proceed (RS_RCMD-2);

  entry (RS_SOI)

	n = 7;
	scan (ibuf + 1, "%d", &n);
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

  entry (RS_SRC)

	n = -1;
	scan (ibuf + 1, "%d", &n);
	if (n < 0 || n > 15)
		proceed (RS_RCMD+1);
	SendRnd = (word) n;
	SendRndPat = 0;
	while (n--)
		SendRndPat |= (1 << n);
	proceed (RS_DON);

  entry (RS_BKF)

	n = -1;
	scan (ibuf + 1, "%d", &n);
	if (n < 0 || n > 15)
		proceed (RS_RCMD+1);
	BkfRnd = (word) n;
	BounceBackoff = 0;
	while (n--)
		BounceBackoff |= (1 << n);
	proceed (RS_DON);

  entry (RS_SRD)

	n = -1;
	scan (ibuf + 1, "%d", &n);
	if (n < 0)
		proceed (RS_RCMD+1);
	ReceiverDelay = n;
	proceed (RS_DON);

  entry (RS_STA)

	n = 0;
	scan (ibuf + 1, "%d", &n);
	if (n < 0 || n > 4)
		proceed (RS_RCMD+1);
	Action = (word) n;
	do_quit ();
	if (n != 0)
		do_start (n);
	proceed (RS_DON);

  entry (RS_BND)

	n = 0;
	scan (ibuf + 1, "%d", &n);
	BounceDelay = (word) n;
	proceed (RS_DON);

  entry (RS_DUM)

	dmp_mem ();
	tcv_dumpqueues ();
	proceed (RS_DON);

  entry (RS_RPC)

	CntSent = CntRcvd = 0;
	proceed (RS_DON);

  entry (RS_SPC)

	ser_outf (RS_SPC, "Sent = %lu, Received = %lu\r\n", CntSent, CntRcvd);
	proceed (RS_RCMD);

  entry (RS_ECO)

	ser_out (RS_ECO, ibuf + 1);

  entry (RS_ECO+1)

	ser_out (RS_ECO+1, "\r\n");
	proceed (RS_DON);

  entry (RS_BID)

	n = -1;
	scan (ibuf + 1, "%d", &n);
	if (n == -1)
		proceed (RS_RCMD+1);
	BID = n;
	proceed (RS_DON);

  entry (RS_SMO)

	n = -1;
	scan (ibuf + 1, "%d", &n);

	if (n < 0 || n >
#if CC1100
2
#endif
#if DM2200
7
#endif
#if DM2100
0
#endif
	)
		proceed (RS_RCMD+1);
	Mode = (byte) n;
	tcv_control (sfd, PHYSOPT_SETMODE, (address)(&n));
	proceed (RS_DON);

#if CC1100

  entry (RS_SEC)

	scan (ibuf + 1, "%d", &n);
	Channel = (byte) (n &= 0xff);
	tcv_control (sfd, PHYSOPT_SETCHANNEL, (address)(&n));
	proceed (RS_DON);

  entry (RS_SRE)

	v = -1;
	scan (ibuf + 1, "%x %x", &n, &v);
	if (v < 0)
		proceed (RS_RCMD+1);
	ba [0] = (byte) n;
	ba [1] = (byte) v;
	ba [2] = 255;

	tcv_control (sfd, PHYSOPT_SETPARAM, (address)ba);
	proceed (RS_DON);
#endif

  entry (RS_PDO)

	v = -1;
	scan (ibuf + 1, "%d", &v);
	if (v < 0)
		proceed (RS_RCMD+1);

	powerdown ();
	CntSent = seconds () + v;
	CntRcvd = 0;

  entry (RS_PDO+1)

	if (seconds () >= CntSent) {
		powerup ();
		proceed (RS_PDO+2);
	}
	CntRcvd++;
	delay (20, RS_PDO+1);
	release;

  entry (RS_PDO+2)

	ser_outf (RS_PDO+2, "Done, %lu wakeups\r\n", CntRcvd);
	proceed (RS_RCMD);

  entry (RS_FLW)

	scan (ibuf + 1, "%u %u", &a, &b);
	if (a >= IFLASH_SIZE)
		proceed (RS_RCMD+1);
	if_write (a, b);
	proceed (RS_RCMD);

  entry (RS_FLR)

	scan (ibuf + 1, "%u", &a);
	if (a >= IFLASH_SIZE)
		proceed (RS_RCMD+1);
	diag ("IF [%u] = %x", a, IFLASH [a]);
	proceed (RS_RCMD);

  entry (RS_FLE)

	if_erase (-1);
	proceed (RS_RCMD);

  entry (RS_SPI)

	a = b = 0;
	scan (ibuf + 1, "%d %d", &a, &b);
	pin_write (a, b);
	proceed (RS_DON);

  entry (RS_GPI)

	a = 0;
	scan (ibuf + 1, "%d", &a);
	diag ("PIN [%u] = %x", a, pin_read (a));
	proceed (RS_RCMD);

  entry (RS_ADC)

	a = b = 0;
	scan (ibuf + 1, "%d %d", &a, &b);

  entry (RS_ADC+1)

	n = pin_read_adc (RS_ADC+1, a, 1, b);

	diag ("PIN [%u] = %d [%x]", a, n, n);
	proceed (RS_RCMD);

#if	PULSE_MONITOR

  entry (RS_CNT)

	lw = -1;
	b = 0;
	scan (ibuf + 1, "%ld %d", &lw, &b);
	pmon_start_cnt (lw, b != 0);
	proceed (RS_DON);

  entry (RS_SCN)

	pmon_stop_cnt ();
	proceed (RS_DON);

  entry (RS_SCM)

	lw = -1;
	scan (ibuf + 1, "%ld", &lw);
	pmon_set_cmp (lw);
	proceed (RS_DON);

  entry (RS_NOT)

	a = b = 0;
	scan (ibuf + 1, "%d %d", &a, &b);
	if (a) {
		pmon_start_not (b != 0);
	} else {
		pmon_stop_not ();
	}
	proceed (RS_DON);

  entry (RS_CNS)

	out_pmon_state ();
	proceed (RS_RCMD);
	
#endif	/* PULSE_MONITOR */

#if 	GLACIER

  entry (RS_FRE)

	a = 0;
	scan (ibuf + 1, "%u", &a);
	freeze (a);
	diag ("OVER");
	proceed (RS_RCMD);
#endif

endprocess (1)
