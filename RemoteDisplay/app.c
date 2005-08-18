/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2005                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "tcvphys.h"

/* ================================================================== */
/* This application receives (reliably) a file over the TCV interface */
/* and scrolls it on the LCD display. It requires a server running on */
/* a workstation connected to the same Ethernet.  A sample server for */
/* Linux is provided in Linux/sfile.c.                                */
/* ================================================================== */

heapmem {60, 40};

#include "lcd.h"
#include "beeper.h"
#include "phys_ether.h"
#include "plug_null.h"

#define	MAXFILESIZE	8192
#define	MAXPLEN		256
#define	TIMEOUT		4096	/* 4 seconds */

static address	pmem = NULL;

#define	pbyte(a)	(((char*)packet) [a])
#define	pstatus		(pbyte (0))
#define	ackcode		(((char*)ack) [0])

#define	packet		(*((address*)(pmem + 0)))
#define	ack		(*((address*)(pmem + 1)))
#define	flength		(*((word*)   (pmem + 2)))
#define	plength		(*((word*)   (pmem + 3)))
#define	efd		(*((word*)   (pmem + 4)))
#define	expected	(*((word*)   (pmem + 5)))
#define	status		(*((word*)   (pmem + 6)))
#define	thefile		((char*)     (pmem + 7) )

#define	PMEMSIZE	(7 * 2 + MAXFILESIZE)

#define	RF_WAITFILE	0
#define	RF_WPACKET	1
#define	RF_ACK		2
#define	RF_NAK		3
#define	RF_ABT		4
#define	RF_BEEP		5
#define	RF_BEEP1	6
#define	RF_BEEP2	7

static void accept () {

	int i;
	char c;

	for (i = 1; i < plength; i++) {
		if (flength == MAXFILESIZE - 1)
			/* ============================================== */
			/* Truncate at the maximum - 1 (for the sentinel) */
			/* but otherwise keep accepting packets           */
			/* ============================================== */
			return;
		c = pbyte (i);
		if (c < 0x20 || c > 0x7e)
			c = ' ';
		thefile [flength++] = c;
	}
}

static void complete () {

	thefile [flength] = '\0';
	dsp_lcd (thefile, YES);
}

process (root, void)

	nodata;

  entry (RF_WAITFILE)

	/* Make sure you have the memory */
	if (pmem == NULL) {
		pmem = umalloc (PMEMSIZE);
		/* Intialize the interface */
		phys_ether (0, 1, MAXPLEN);
		tcv_plug (0, &plug_null);
		dsp_lcd ("PicOS ready     RemoteDisplay", YES);
		efd = tcv_open (NONE, 0, 0);
	}

	/* Expecting a start of file packet */
	expected = 0;
	flength = 0;
	/* Starting */
	status = 0;

  entry (RF_WPACKET)

	delay (TIMEOUT, RF_NAK);
	packet = tcv_rnp (RF_WPACKET, efd);
	plength = tcv_left (packet);
	if (plength < 1 || (pstatus & 1) != expected) {
		tcv_endp (packet);
		proceed (RF_NAK);
	}
	if (status == 0 && (pstatus & 2) == 0) {
		tcv_endp (packet);
		proceed (RF_ABT);
	}
	status = 1;
	if (pstatus & 4)
		/* The last packet */
		status = 2;
	accept ();
	tcv_endp (packet);

  entry (RF_ACK)

	/* Prepare an ACK packet */
	ack = tcv_wnp (RF_ACK, efd, 1);
	ackcode = expected;
	tcv_endp (ack);

	if (status > 1) {
		/* This is the last packet */
		complete ();
		proceed (RF_BEEP);
	} else {
		expected = 1 - expected;
		proceed (RF_WPACKET);
	}

  entry (RF_BEEP)

	plength = 10;

  entry (RF_BEEP1)

	if (plength-- == 0)
		proceed (RF_WAITFILE);
	beep (1, 3, RF_BEEP2);

  entry (RF_BEEP2)

	beep (1, 4, RF_BEEP1);

  entry (RF_NAK)

	/* ======================================================= */
	/* If this cannot be granted, keep waiting for an incoming */
	/* packet rather than room for the NAK.                    */
	/* ======================================================= */
	ack = tcv_wnp (NONE, efd, 1);
	if (ack == NULL)
		proceed (RF_WPACKET);
	/* Send a single NAK byte */
	ackcode = 1 - expected;
	tcv_endp (ack);

	proceed (RF_WPACKET);

  entry (RF_ABT)

	ack = tcv_wnp (NONE, efd, 1);
	if (ack == NULL)
		proceed (RF_WPACKET);
	ackcode = 2;
	tcv_endp (ack);

	proceed (RF_WAITFILE);

endprocess (1)
