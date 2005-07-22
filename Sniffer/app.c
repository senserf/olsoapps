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

#include "sysio.h"
#include "tcvphys.h"

heapmem {10, 90};

#include "lcd.h"
#include "ser.h"
#include "serf.h"
#include "form.h"
#include "phys_ether.h"
#include "plug_null.h"

/* ========================= */
/* A simple Etherent sniffer */
/* ========================= */

static int efd;
static address packet;
static word firstw, nwords;
static unsigned long npackets = 0;
static char lcd_buf [33] = "                                " ;
/*                          01234567890123456789012345678901 */

static void lcd_encu (int pos, int nd, word n) {

	char enc [6]; char *ep; int k, m;

	form (ep = enc, "%u", n);
	if ((k = (m = strlen (ep)) - nd) > 0) {
		ep += k;
		m = nd;
	} else {
		while (k++)
			lcd_buf [pos++] = '0';
	}
	memcpy (lcd_buf + pos, ep, m);
}

static void lcd_encl (int pos, int nd, lword n) {

	char enc [12]; char *ep; int k, m;

	form (ep = enc, "%lu", n);
	if ((k = (m = strlen (ep)) - nd) > 0) {
		ep += k;
		m = nd;
	} else {
		while (k++)
			lcd_buf [pos++] = '0';
	}
	memcpy (lcd_buf + pos, ep, m);
}

#define	MS_RUN 00

static process (memstat, void)

	static word rc = 0;
	word failures;

	nodata;

  entry (MS_RUN)

	/* Free, failures, ticks */

	lcd_encu (11, 5, memfree (1, &failures));
	lcd_encu (28, 1, failures);
	lcd_encu (30, 2, rc);

	if ((rc & 0xf) == 0) {
		rc = 0;
		lcd_encu (16, 5, maxfree (1, &failures));
		lcd_encu (22, 5, failures);
	}

	upd_lcd (lcd_buf);
	rc++;

	delay (1024, MS_RUN);

endprocess (1)

#define	RC_TRY		00
#define	RC_OUT		10
#define	RC_LF		20
#define	RC_ENP		30

static process (sniffer, void)

	static word np, length;
	char c;

	nodata;

  entry (RC_TRY)

	packet = tcv_rnp (RC_TRY, efd);
	length = tcv_left (packet);

  entry (RC_OUT)

	ser_outf (RC_OUT, "===> %d bytes\r\n", length);
	length = (length + 1) / 2;
	np = 0;
	npackets++;
	lcd_encl (0, 10, npackets);
	upd_lcd (lcd_buf);
	c = (char) npackets;
	ion (LEDS, CONTROL, &c, LEDS_CNTRL_SET);

  entry (RC_OUT+1)

	if (np >= nwords || np + firstw >= length) {
		if ((np % 8) != 0)
			proceed (RC_LF);
		else
			proceed (RC_ENP);
	}

  entry (RC_OUT+2)

	ser_outf (RC_OUT+2, "%x ", packet [firstw + np]);
	np++;
	if ((np % 8) != 0)
		proceed (RC_OUT+1);

  entry (RC_OUT+3)

	ser_out (RC_OUT+3, "\r\n");
	proceed (RC_OUT+1);

  entry (RC_LF)

	ser_out (RC_LF, "\r\n");

  entry (RC_ENP)

	tcv_endp (packet);
	proceed (RC_TRY);

endprocess (1)

#undef	RC_TRY
#undef	RC_OUT
#undef	RC_LF

int sniff_start (int fb, int nb) {

	firstw = fb / 2;
	if ((nb & 1) == 0 && (fb & 1) != 0)
		nb++;
	nwords = (nb + 1) / 2;

	if (!running (memstat))
		fork (memstat, NULL);

	if (!running (sniffer)) {
		// tcv_control (efd, PHYSOPT_TXON, NULL);
		tcv_control (efd, PHYSOPT_RXON, NULL);
		fork (sniffer, NULL);
		return 1;
	}
	return 0;
}

void sniff_stop (void) {

	if (running (sniffer)) {
		// tcv_control (efd, PHYSOPT_TXOFF, NULL);
		tcv_control (efd, PHYSOPT_RXOFF, NULL);
		kill (running (sniffer));
	}
	if (running (memstat))
		kill (running (memstat));
}

#define	RS_INIT		00
#define	RS_RCMD		10
#define	RS_START	20
#define	RS_STOP		30

process (root, int)

	static int k, n1, n2;
	static char cmd, *fmt;

  entry (RS_INIT)

	dsp_lcd ("PicOS ready     SNIFFER", YES);
	/* Initalize the Ethernet interface */
	phys_ether (0, 0, 0);
	tcv_plug (0, &plug_null);
	cmd = 1;
	ion (ETHERNET, CONTROL, &cmd, ETHERNET_CNTRL_PROMISC);
	efd = tcv_open (NONE, 0, 0);
	if (efd < 0) {
		diag ("Cannot open tcv interface");
		halt ();
	}
	tcv_control (efd, PHYSOPT_TXOFF, NULL);
	tcv_control (efd, PHYSOPT_RXOFF, NULL);

  entry (RS_RCMD-1)

	ser_out (RS_RCMD-1,
		"\r\nEthernet sniffer\r\n"
		"Commands:\r\n"
		"s starting_byte length      (start)\r\n"
		"h                           (stop)\r\n"
			);
  entry (RS_RCMD)

	k = ser_inf (RS_RCMD, "%c %d %d", &cmd, &n1, &n2);

	if (cmd == 's')
		proceed (RS_START);
	if (cmd == 'h')
		proceed (RS_STOP);

  entry (RS_RCMD+1)

	ser_out (RS_RCMD+1, "Illegal command\r\n");
	proceed (RS_RCMD);

  entry (RS_START)

	if (k < 2 || n1 < 0)
		n1 = 0;

	if (k < 3 || n2 < 1 || n2 > 1500)
		n2 = 32;

	if (sniff_start (n1, n2))
		fmt = "Started sniffer, from = %d, length = %d\r\n";
	else
		fmt = "New sniffer parameters, from = %d, length = %d\r\n";

  entry (RS_START+1)

	ser_outf (RS_START+1, fmt, n1, n2);
	proceed (RS_RCMD);

  entry (RS_STOP)

	sniff_stop ();
	proceed (RS_RCMD);

	nodata;

endprocess (1)
