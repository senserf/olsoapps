/* ============================================================================ */
/*                                                                              */
/* Copyright (C) Olsonet Communications Corporation, 2002, 2003                 */
/*                                                                              */
/* Permission is hereby granted, free of charge, to any person obtaining a copy */
/* of this software and associated documentation files (the "Software"), to     */
/* deal in the Software without restriction, including without limitation the   */
/* rights to use, copy, modify, merge, publish, distribute, sublicense, and/or  */
/* sell copies of the Software, and to permit persons to whom the Software is   */
/* furnished to do so, subject to the following conditions:                     */
/*                                                                              */
/* The above copyright notice and this permission notice shall be included in   */
/* all copies or substantial portions of the Software.                          */
/*                                                                              */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   */
/* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     */
/* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE  */
/* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER       */
/* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING      */
/* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS */
/* IN THE SOFTWARE.                                                             */
/*                                                                              */
/* ============================================================================ */

/*
 * A simple packet pinger testing the transceiver interface
 */

#include "sysio.h"
#include "tcvphys.h"

heapmem {100, 0};

#include "lcd.h"
#include "led.h"
#include "ser.h"
#include "serf.h"

//#include "trc.h"

/* =============== */
/* Packet receiver */
/* =============== */

#define	MAXPLEN	1516
#define	IBUFLEN	82

static char	*rpacket = NULL;
static int	rlength, rmode;

#define	RC_TRY		00
#define	RC_RAW		10
#define	RC_COOKED	20

process (receiver, void)

	nodata;

  entry (RC_TRY)

	rlength = io (RC_TRY, ETHERNET, READ, rpacket, MAXPLEN);
	rmode = ion (ETHERNET, CONTROL, NULL, ETHERNET_CNTRL_GMODE);
	ion (LEDS, CONTROL, ((char*)&rlength) + 1, LEDS_CNTRL_SET);
	if (rmode) {
		/* This is a cooked packet */
		rpacket [rlength] = '\0';
		dsp_lcd (rpacket, YES);
		proceed (RC_COOKED);
	}
	/* Raw */

  entry (RC_RAW)

	ser_outf (RC_RAW, "RCV (R): LEN = %d, %x %x\r\n", rlength,
		((word*)rpacket) [6], ((word*)rpacket) [7]);

	proceed (RC_TRY);

  entry (RC_COOKED)

	ser_outf (RC_COOKED, "RCV (C) : LEN = %d, '%s'\r\n", rlength, rpacket);

	proceed (RC_TRY);

endprocess (1)

int rcv_start (int mode, int prom) {

	char c;

	/* Reset */
	c = 0;
	ion (ETHERNET, CONTROL, &c, ETHERNET_CNTRL_PROMISC);
	ion (ETHERNET, CONTROL, &c, ETHERNET_CNTRL_MULTI);
	c = (char) mode;
	ion (ETHERNET, CONTROL, &c, ETHERNET_CNTRL_RMODE);

	if (prom) {
		c = 1;
		ion (ETHERNET, CONTROL, &c, ETHERNET_CNTRL_PROMISC);
	}
	if (rpacket == NULL) {
		rpacket = (char*) umalloc (MAXPLEN + 2);
		fork (receiver, NULL);
		return 1;
	}
	return 0;
}

int rcv_stop (void) {

	if (rpacket != NULL) {
		kill (running (receiver));
		ufree (rpacket);
		rpacket = NULL;
		return 1;
	}
	return 0;
}

#undef	RC_TRY
#undef	RC_RAW
#undef	RC_COOKED

/* ============= */
/* Packet sender */
/* ============= */

static	int	tdel, tmode, tcntr, tlength;
static	char	*tpacket;

#define	destadd	((word*)tpacket)
#define	protoid (*(((word*)tpacket) + 6))
#define	payload (tpacket + 14)

#define	htons(w) (((w) << 8) | (((w) >> 8) & 0xff))

#define	SN_SEND		00

process (sender, void)

	nodata;

  entry (SN_SEND)

	form (payload, "Pkt %d", tcntr);
	tlength = strlen (payload);

        if (!tmode) {
	  protoid = htons (tlength);
	  tlength += 14;
	}

  entry (SN_SEND+1)

	io (SN_SEND+1, ETHERNET, WRITE, tpacket, tlength);

  entry (SN_SEND+2)

	ser_outf (SN_SEND+2, "Sent packet %d\r\n", tcntr);
	tcntr++;
	delay (tdel, SN_SEND);

endprocess (1)

int snd_start (int mode, int delay, word *da) {

	char c;
	int rc;

	c = (char) mode;
	ion (ETHERNET, CONTROL, &c, ETHERNET_CNTRL_WMODE);
	rc = 0;

	if (tpacket == NULL) {
		tpacket = (char*) umalloc (MAXPLEN);
		fork (sender, NULL);
		rc = 1;
	}

	tdel = delay;
	tmode = mode;
	destadd [0] = htons (da [0]);
	destadd [1] = htons (da [1]);
	destadd [2] = htons (da [2]);
	tcntr = 0;
	return rc;
}

int snd_stop (void) {

	if (tpacket != NULL) {
		kill (running (sender));
		ufree (tpacket);
		tpacket = NULL;
		return 1;
	}
	return 0;
}

#undef	SN_SEND

/* ================= */
/* End packet sender */
/* ================= */

#define	RS_INIT		00
#define	RS_RCMD		10
#define	RS_SEND		20
#define	RS_RCV		30
#define	RS_SID		40
#define	RS_QUIT		50

process (root, int)

	static char *ibuf;
	static word da [3];


	static int k, n1, n2;
	static char *fmt, obuf [24];

  entry (RS_INIT)

	dsp_lcd ("PicOS ready     MACTEST", YES);
	/* Default card Id */
	k = 1;
	ion (ETHERNET, CONTROL, (char*)&k, ETHERNET_CNTRL_SETID);
	ibuf = (char*) umalloc (IBUFLEN);

  entry (RS_RCMD-1)

	ser_out (RS_RCMD-1,
		"\r\nEthernet interface test\r\n"
		"Commands:\r\n"
		"s m i d    -> start/reset sending\r\n"
		"         m == mode (0 - raw, 1 - cooked)\r\n"
		"         i == interval in milliseconds (>= 128)\r\n"
		"         d == destination address xxxxxxxx (if m == 0)\r\n"
		"r m p      -> start/reset receiving\r\n"
		"         m == mode (0 - raw, 1 - cooked, 2 - flex)\r\n"
		"         p == selectivity (0 - selective, 1 - promiscuous)\r\n"
		"i n        -> set card Id to n\r\n"
		"q          -> stop transmission and reception\r\n"
		);

  entry (RS_RCMD)

	k = ser_in (RS_RCMD, ibuf, IBUFLEN-1);

	if (ibuf [0] == 's')
		proceed (RS_SEND);
	if (ibuf [0] == 'r')
		proceed (RS_RCV);
	if (ibuf [0] == 'i')
		proceed (RS_SID);
	if (ibuf [0] == 'q')
		proceed (RS_QUIT);

  entry (RS_RCMD+1)

	ser_out (RS_RCMD+1, "Illegal command\r\n");
	proceed (RS_RCMD);

  entry (RS_SEND)

	/* Defaults */
	n1 = 0;
	n2 = 1024;
	da [0] = da [1] = da [2] = 0xffff;
	scan (ibuf + 1, "%d %d %x %x %x", &n1, &n2, da+0, da+1, da+2);

	if (n1 < 0 || n1 > 1)
		n1 = 0;
	if (n2 < 128)
		n2 = 128;

	if (snd_start (n1, n2, da))
		fmt = "Started sender, mode = %d, del = %d, da = %x%x%x\r\n";
	else
		fmt = "New sender params: mode = %d, del = %d, da = %x%x%x\r\n";

  entry (RS_SEND+1)

	ser_outf (RS_SEND+1, fmt, n1, n2, da [0], da [1], da [2]);
	proceed (RS_RCMD);

  entry (RS_RCV)

	n1 = n2 = 0;
	scan (ibuf + 1, "%d %d", &n1, &n2);

	if (n1 < 0 || n1 > 2)
		n1 = 0;
	if (n2 < 0 || n2 > 1)
		n2 = 0;

	if (rcv_start (n1, n2))
		fmt = "Started receiver, mode = %d/%d\r\n";
	else
		fmt = "New receiver mode = %d/%d\r\n";

  entry (RS_RCV+1)

	ser_outf (RS_RCV+1, fmt, n1, n2);
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

  entry (RS_SID)

	n1 = 0;
	scan (ibuf + 1, "%d", &n1);

	ion (ETHERNET, CONTROL, (char*)&n1, ETHERNET_CNTRL_SETID);

  entry (RS_SID+1)

	ser_outf (RS_SID+1, "Card Id set to %d\r\n", n1);
	proceed (RS_RCMD);

	nodata;

endprocess (1)

#undef	RS_INIT
#undef	RS_RCMD
#undef	RS_SEND
#undef	RS_RCV
#undef	RS_SID
#undef	RS_QUIT
