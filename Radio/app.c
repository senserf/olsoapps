/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include <ecog.h>
#include <ecog1.h>
#include "sysio.h"
#include "radio.h"

/*
 * Send/receive raw radio packets (no TCV). You can also use it to
 * monitor the received radio signal without actually receiving any
 * packets.
 *
 */

heapmem {100};

#include "ser.h"
#include "serf.h"

#define	X_PACKET_LENGTH	64	// Including CRC
#define	R_PACKET_LENGTH 128

static word 	xmt_delay,		// Inter-xmit delay
		xmt_count,
		xcv_lock,
		mtr_time,
		mtr_delay;

static char *rcv_buf, *xmt_buf;

#define	wrcbuf	((address)rcv_buf)
#define	wxmbuf	((address)xmt_buf)

#define	RC_READ		00
#define	RC_DISPL	20

#define	XM_WAIT		00
#define	XM_LOCK		20
#define	XM_XMIT		30

#define	MT_LOOP		00
#define	MT_OUT		10

static process (receiver, void)

	static int len;

	nodata;

  entry (RC_READ);

	rcvcancel ();

	if (xcv_lock) {
		// Locked by the transmitter
		wait ((word) &xcv_lock, RC_READ);
		release;
	}

	len = io (NONE, RADIO, READ, rcv_buf, R_PACKET_LENGTH);
	if (len == 0) {
		rcvwait (RC_READ);
		release;
	}

  entry (RC_DISPL)

	ser_outf (RC_DISPL,
		"RECEIVED (%d) = %x %x %x %x %x ... %x (%x%x)\r\n", len,
		wrcbuf [0],
		wrcbuf [1],
		wrcbuf [2],
		wrcbuf [3],
		wrcbuf [4],
		wrcbuf [X_PACKET_LENGTH/2 - 3],
		wrcbuf [X_PACKET_LENGTH/2 - 2],
		wrcbuf [X_PACKET_LENGTH/2 - 1]);

	rcvwait (RC_READ);

endprocess (1)

static process (xmitter, void)

	int i;

	nodata;

  entry (XM_WAIT)

	delay (xmt_delay, XM_LOCK);
	release;

  entry (XM_LOCK)

	xcv_lock = 1;
	/* Switch on the transmitter */
	xmt_buf [0] = 1;
	io (NONE, RADIO, CONTROL, xmt_buf, RADIO_CNTRL_XMTCTRL);
	delay (1, XM_XMIT);
	release;

  entry (XM_XMIT)

	ser_outf (XM_XMIT, "SENDING %d\r\n", xmt_count);

	wxmbuf [0] = 0xaaaa;
	wxmbuf [1] = xmt_count++;
	wxmbuf [2] = 0xffff;
	wxmbuf [3] = 0xbeef;

	for (i = 4; i < X_PACKET_LENGTH/2 - 2; i++)
		wxmbuf [i] = i + xmt_count;

	wxmbuf [i] = wxmbuf [i+1] = 0;

	io (NONE, RADIO, WRITE, xmt_buf, X_PACKET_LENGTH);

	/* Switch off the transmitter */
	xmt_buf [0] = 0;
	io (NONE, RADIO, CONTROL, xmt_buf, RADIO_CNTRL_XMTCTRL);
	xcv_lock = 0;
	trigger ((word)&xcv_lock);
	proceed (XM_WAIT);

endprocess (1)

static process (monitor, void)

	static lword cnt, tot;
	word tim;

	nodata;

  entry (MT_LOOP)

	tim = mtr_time;
	cnt = 0;
	tot = 0;
	utimer (&tim, YES);
	while (tim) {
		tot++;
		if (rcvhigh)
			cnt++;
	}
	utimer (&tim, NO);

  entry (MT_OUT)

	ser_outf (MT_OUT, "TOTAL/HITS = %lu/%lu\r\n", tot, cnt);

	delay (mtr_delay, MT_LOOP);

endprocess (1)

static void mtr_stop () {

	if (running (monitor))
		kill (running (monitor));
}

static void mtr_start (word lp, word del) {

	mtr_stop ();

	mtr_time = lp;
	mtr_delay = del;

	fork (monitor, NULL);
}

static void rcv_stop () {

	char c;

	if (running (receiver)) {
		c = 0;
		io (NONE, RADIO, CONTROL, &c, RADIO_CNTRL_RCVCTRL);
		kill (running (receiver));
	}
}

static void rcv_begin () {

	char c;

	rcv_stop ();

	if (rcv_buf == NULL)
		rcv_buf = (char*) umalloc (R_PACKET_LENGTH);

	c = 1;
	io (NONE, RADIO, CONTROL, &c, RADIO_CNTRL_RCVCTRL);

	fork (receiver, NULL);
}

static void xmt_stop () {

	if (running (xmitter))
		kill (running (xmitter));

	if (xcv_lock) {
		xcv_lock = 0;
		trigger ((word)&xcv_lock);
	}
}

static void xmt_begin (word idel) {

	xmt_stop ();

	if (xmt_buf == NULL)
		xmt_buf = (char*) umalloc (X_PACKET_LENGTH);

	xmt_delay = idel;

	fork (xmitter, NULL);
}

#define	RS_INIT		00
#define	RS_RCMD		10
#define	RS_XMIT		20
#define	RS_RECEIVE	30
#define	RS_MONITOR	40
#define	RS_SETRATE	50
#define	RS_CHECK	60
#define	RS_STOP		70

#define	IBUFSIZE	128

process (root, void)
/* =========================================== */
/* This is the main program of the application */
/* =========================================== */

	static char *ibuf = NULL;
	word del, lpt;

	nodata;

  entry (RS_INIT)

	if (ibuf == NULL)
		ibuf = (char*) umalloc (IBUFSIZE);

  entry (RS_RCMD-1)

	ser_out (RS_RCMD-1, "\r\n"
		"Commands: 'x d'     xmit packets spaced d msecs apart\r\n"
		"          'r'       receive\r\n"
		"          'm l d'   rcv status monitor\r\n"
		"          'b baud'  set bit rate\r\n"
		"          'c [0|1]  disable/enable checksum\r\n"
		"          's'       stop\r\n");

  entry (RS_RCMD)

	ser_in (RS_RCMD, ibuf, IBUFSIZE);

	if (ibuf [0] == 'x')
		proceed (RS_XMIT);
	if (ibuf [0] == 'r')
		proceed (RS_RECEIVE);
	if (ibuf [0] == 'm')
		proceed (RS_MONITOR);
	if (ibuf [0] == 'b')
		proceed (RS_SETRATE);
	if (ibuf [0] == 's')
		proceed (RS_STOP);
	if (ibuf [0] == 'c')
		proceed (RS_CHECK);

  entry (RS_RCMD+1)

	ser_outf (RS_RCMD+1, "Time: %lu -> Illegal command or parameter\r\n",
		seconds ());
	proceed (RS_RCMD-1);

  entry (RS_XMIT)

	if (scan (ibuf + 1, "%u", &del) < 1)
		proceed (RS_RCMD+1);

	xmt_begin (del);
	proceed (RS_RCMD-1);

  entry (RS_RECEIVE)

	rcv_begin ();
	proceed (RS_RCMD-1);

  entry (RS_MONITOR)

	if (scan (ibuf + 1, "%u %u", &lpt, &del) < 2)
		proceed (RS_RCMD+1);

	mtr_start (lpt, del);
	proceed (RS_RCMD-1);

  entry (RS_SETRATE)

	if (scan (ibuf + 1, "%u", &lpt) < 1)
		proceed (RS_RCMD+1);

	io (NONE, RADIO, CONTROL, (char*) &lpt, RADIO_CNTRL_CALIBRATE);
	proceed (RS_RCMD-1);

  entry (RS_CHECK)

	if (scan (ibuf + 1, "%u", &lpt) < 1)
		proceed (RS_RCMD+1);

	ibuf [0] = (char) lpt;
	io (NONE, RADIO, CONTROL, ibuf, RADIO_CNTRL_CHECKSUM);
	proceed (RS_RCMD-1);

  entry (RS_STOP)

	xmt_stop ();
	rcv_stop ();
	mtr_stop ();
	proceed (RS_RCMD-1);

endprocess (1)
