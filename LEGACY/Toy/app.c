/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2007                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

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

#if	LEDS_DRIVER
#include "led.h"
#endif

#if	LCD_DRIVER
#include "lcd.h"
#endif

#include "ser.h"
#include "serf.h"
// #include "beeper.h"

#define	RS_INIT		00
#define	RS_RCMD		10
#define	RS_LCD		20
#define	RS_LCU		30
#define	RS_LED		40
#define	RS_LEF		45
#define	RS_CUA		50
#define RS_SDRAM	60
#define	RS_FORMAT	70
#define	RS_SFREE	80

#define	IBUFSIZE	128

process (root, void*)
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
	static lint vli;
	static lword vlu, vlx;

	nodata;

  entry (RS_INIT)

	// ser_select (1);

	if (ibuf == NULL)
		ibuf = (char*) umalloc (IBUFSIZE);

#if LCD_DRIVER
	dsp_lcd ("PicOS ready     TOY", YES);
#endif

  entry (RS_RCMD-1)

	ser_out (RS_RCMD-1, "\r\n"
		"Welcome to PicOS!\r\n"
		"Commands:\r\n"
#if LCD_DRIVER
 		"          'm...anytext...' (LCD)\r\n"
		"          'n...anytext...' (LCD upd)\r\n"
#endif

#if LEDS_DRIVER
		"          'b...hexdigits...' (LEDs)\r\n"
		"          'c led val' (LED)\r\n"
#endif
		"          'u number(0/1) rate' (UART change)\r\n"
#if SDRAM_PRESENT && ! ECOG_SIM
		"          's bufsize nkwords' (SDRAM test)\r\n"
#endif
		"          'f %d %u %x %c %s %ld %lu %lx' (format test)\r\n"
#if STACK_GUARD
		"          'v' (show free memory)\r\n"
#endif
		"          'h' (HALT)\r\n"
		"          'r' (RESET)\r\n");

  entry (RS_RCMD)

	ser_in (RS_RCMD, ibuf, IBUFSIZE);

#if LCD_DRIVER
	if (ibuf [0] == 'm')
		proceed (RS_LCD);
	if (ibuf [0] == 'n')
		proceed (RS_LCU);
#endif

#if LEDS_DRIVER
	if (ibuf [0] == 'b')
		proceed (RS_LED);
	if (ibuf [0] == 'c')
		proceed (RS_LEF);
#endif

	if (ibuf [0] == 'u')
		proceed (RS_CUA);

#if SDRAM_PRESENT
	if (ibuf [0] == 's')
		proceed (RS_SDRAM);
#endif

#if STACK_GUARD
	if (ibuf [0] == 'v')
		proceed (RS_SFREE);
#endif

	if (ibuf [0] == 'f')
		proceed (RS_FORMAT);
	if (ibuf [0] == 'r')
		reset ();
	if (ibuf [0] == 'h')
		halt ();

  entry (RS_RCMD+1)

	ser_out (RS_RCMD+1, "Illegal command or parameter\r\n");
	proceed (RS_RCMD-1);

#if LCD_DRIVER

  entry (RS_LCD)

	dsp_lcd (ibuf + 1, YES);
	proceed (RS_RCMD);

  entry (RS_LCU)

	upd_lcd (ibuf + 1);
	proceed (RS_RCMD);
#endif

#if LEDS_DRIVER
  entry (RS_LED)

	dsp_led (ibuf + 1, 256);
	proceed (RS_RCMD);

  entry (RS_LEF)

	if (scan (ibuf + 1, "%u %u", &n, &m) < 2 || n > 3 || m > 3)
		proceed (RS_RCMD+1);
	if (m == 3) {
		m = 2;
		fastblink (1);
	} else {
		fastblink (0);
	}
	leds (n, m);
	proceed (RS_RCMD);
#endif

  entry (RS_CUA)

	vu = 9600;
	if (scan (ibuf + 1, "%d %u", &np, &vu) < 1 || np < 0 || np > 1)
		proceed (RS_LCD-1);

#if ! ECOG_SIM
	ser_select (np);
	// if np=1 (for DUART B) this will distort the ECOG_SIM assumption
	// that only DUART A is connected to the terminal
#endif

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

	diag ("CH: %d", wsizeof (char));
	diag ("WD: %d", wsizeof (word));

  entry (RS_FORMAT+1)

        ser_out (RS_FORMAT+1, ibuf);
        proceed (RS_RCMD);

#if STACK_GUARD

  entry (RS_SFREE)

	ser_outf (RS_SFREE  , "Free stack space:  %u words\r\n", stackfree ());

  entry (RS_SFREE+1)

	ser_outf (RS_SFREE+1, "Static data size:  %u words\r\n", staticsize ());

  entry (RS_SFREE+2)

	ser_outf (RS_SFREE+2, "Mallocatable area: %u words\r\n", memfree (0,0));

	proceed (RS_RCMD);
#endif

endprocess
