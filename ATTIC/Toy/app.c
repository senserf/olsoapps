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

/* ==================================================================== */
/* This is the Toy application. It blinks the leds and scrolls messages */
/* through the LCD display.                                             */
/*                                                                      */
/* It also tests a few things on the side.                              */
/* ==================================================================== */

/*
 * There is no TCV, so all dynamic memory goes to umalloc
 */
heapmem {100};

#include "led.h"
#include "lcd.h"
#include "ser.h"
#include "beeper.h"

#define	RS_INIT		00
#define	RS_RCMD		10
#define	RS_LCD		20
#define	RS_LCU		30
#define	RS_LED		40
#define	RS_CUA		50
#define RS_SDRAM	60
#define	RS_FORMAT	70

#define	IBUFSIZE	128

process (root, void)
/* =========================================== */
/* This is the main program of the application */
/* =========================================== */

	static char *ibuf = NULL;
	static word *mbuf;
	static word n, m, bp;
	static lword nw, i, j;
	static int np;
	static int q, vi;
	static word vu, vx;
	static char vc;
	static char vs [32];
	static long int vli;
	static lword vlu, vlx;

	nodata;

  entry (RS_INIT)

	if (ibuf == NULL)
		ibuf = (char*) umalloc (IBUFSIZE);

	dsp_lcd ("PicOS ready     TOY", YES);

  entry (RS_RCMD-1)

	ser_out (RS_RCMD-1, "\r\n"
		"Welcome to PicOS!\r\n"
		"Commands: 'm...anytext...' (LCD)\r\n"
		"          'n...anytext...' (LCD upd)\r\n"
		"          'b...hexdigits...' (LEDs)\r\n"
		"          'u number(0/1) rate' (UART change)\r\n"
#if SDRAM_PRESENT && ! ECOG_SIM
		"          's bufsize nkwords' (SDRAM test)\r\n"
#endif
		"          'f %d %u %x %c %s %ld %lu %lx' (format test)\r\n"
		"          'h' (HALT)\r\n"
		"          'r' (RESET)\r\n");

  entry (RS_RCMD)

	ser_in (RS_RCMD, ibuf, IBUFSIZE);

	if (ibuf [0] == 'm')
		proceed (RS_LCD);
	if (ibuf [0] == 'n')
		proceed (RS_LCU);
	if (ibuf [0] == 'b')
		proceed (RS_LED);
	if (ibuf [0] == 'u')
		proceed (RS_CUA);
#if SDRAM_PRESENT
	if (ibuf [0] == 's')
		proceed (RS_SDRAM);
#endif
	if (ibuf [0] == 'f')
		proceed (RS_FORMAT);
	if (ibuf [0] == 'r')
		reset ();
	if (ibuf [0] == 'h')
		halt ();

  entry (RS_LCD-1)

	ser_out (RS_LCD-1, "Illegal command or parameter\r\n");
	proceed (RS_RCMD-1);

  entry (RS_LCD)

	dsp_lcd (ibuf + 1, YES);
	proceed (RS_RCMD);

  entry (RS_LCU)

	upd_lcd (ibuf + 1);
	proceed (RS_RCMD);

  entry (RS_LED)

	dsp_led (ibuf + 1, 256);
	proceed (RS_RCMD);

  entry (RS_CUA)

	vu = 9600;
	if (scan (ibuf + 1, "%d %u", &np, &vu) < 1 || np < 0 || np > 1)
		proceed (RS_LCD-1);

#if ! ECOG_SIM
	ser_select (np);
	// if np=1 (for DUART B) this will distort the ECOG_SIM assumption
	// that only DUART A is connected to the terminal
#endif

        ion (np, CONTROL, (char*)(&vu), UART_CNTRL_RATE);


  entry (RS_CUA+1)

	ser_outf (RS_CUA+1, "Port %d, rate %u", np, vu);
	proceed (RS_RCMD);

#if SDRAM_PRESENT

  entry (RS_SDRAM)

	if (scan (ibuf + 1, "%u %u", &m, &n) != 2)
		proceed (RS_LCD-1);

	if (m == 0 || n == 0)
		proceed (RS_LCD-1);

	mbuf = umalloc (m + m);

  entry (RS_SDRAM+1)

	ser_outf (RS_SDRAM+1, "Testing %u Kwords using a %u word buffer\r\n",
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

	ser_out (RS_SDRAM+3, "Writing complete\r\n");

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

	ser_outf (RS_SDRAM+5, "Test OK %x\r\n", mbuf [0]);
	ufree (mbuf);
	proceed (RS_RCMD);

  entry (RS_SDRAM+6)

	ser_outf (RS_SDRAM+6, "Error: %x%x %x -> %x\r\n",
				(word)((i >> 16) & 0xffff),
				(word)( i        & 0xffff),
				mbuf [bp],
				(word) (i + 1));
	ufree (mbuf);
	proceed (RS_RCMD);
#endif

  entry (RS_FORMAT)

	vi = 0;
	vu = vx = 0;
	vc = 'x';
	strcpy (vs, "--empty--");
	vli = 0;
	vlu = vlx = 0;

	q = scan (ibuf + 1, "%d %u %x %c %s %ld %lu %lx",
				&vi, &vu, &vx, &vc, vs, &vli, &vlu, &vlx);

	form (ibuf, "%d ** %d, %u, %x, %c, %s, %ld, %lu, %lx **\r\n", q,
				vi, vu, vx, vc, vs, vli, vlu, vlx);

  entry (RS_FORMAT+1)

        ser_out (RS_FORMAT+1, ibuf);
        proceed (RS_RCMD);

endprocess (1)






