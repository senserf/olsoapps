/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"

/* ================================================================== */
/* To test boards with Nokia 6100 LCD display (and a few other things */
/* along the way.                                                     */
/* ================================================================== */

heapmem {100};

#if	LEDS_DRIVER
#include "led.h"
#endif

#include "ser.h"
#include "serf.h"

//#include "bmp.h"

#define	RS_INIT		00
#define	RS_RCMD		10
#define	RS_ERR		11
#define	RS_LRES		30
#define	RS_LOFF		35
#define	RS_LGEN		40
#define	RS_SETI		42
#define	RS_DISP		44
#define	RS_ERASE	50
#define	RS_RCMN		100

#define	IBUFSIZE	128

static byte *PIXT = NULL;
static word NPIX = 0;

process (root, void)
/* =========================================== */
/* This is the main program of the application */
/* =========================================== */

	static char *ibuf = NULL;
	word i, j, n, m, c [4];

  entry (RS_INIT)

	if (ibuf == NULL)
		ibuf = (char*) umalloc (IBUFSIZE);

	ser_out (RS_INIT, "\r\n"
		"Commands:\r\n"
		"  r              : reset\r\n"
		"  R n            : LCD reset\r\n"
		"  F              : LCD off\r\n"
		"  G m n c c c c  : generate a pix table\r\n"
		"  S x y w h m    : set image\r\n"
		"  D x y          : render the pix table\r\n"
		"  E n            : erase to color\r\n"
	);

  entry (RS_RCMD)

	ser_in (RS_RCMD, ibuf, IBUFSIZE);

	switch (ibuf [0]) {

		case 'r' : reset ();
		case 'R' : proceed (RS_LRES);
		case 'F' : proceed (RS_LOFF);
		case 'G' : proceed (RS_LGEN);
		case 'S' : proceed (RS_SETI);
		case 'D' : proceed (RS_DISP);
		case 'E' : proceed (RS_ERASE);
	}

  entry (RS_ERR)

	ser_out (RS_ERR, "Illegal command or parameter\r\n");
	proceed (RS_INIT);

  entry (RS_LRES)

	n = 0;
	scan (ibuf+1, "%u", &n);
	lcdg_on ((byte)n);
	proceed (RS_RCMN);

  entry (RS_LOFF)

	lcdg_off ();
	proceed (RS_RCMN);

  entry (RS_LGEN)

	// Generate a pix table
	m = 0;
	n = 16;
	c [0] = 0; c [1] = 0; c [2] = 0xffff; c [3] = 0xffff;
	scan (ibuf+1, "%u %u %x %x %x %x", &m, &n, c+0, c+1, c+2, c+3);
	if (PIXT != NULL)
		ufree (PIXT);

	i = m ? n : (n + 1) * 12 / 8;

	if ((PIXT = (byte*) umalloc (i)) == NULL)
		proceed (RS_ERR);

	NPIX = n;

	if (m) {
		for (i = 0; i < n; i++)
			PIXT [i] = (byte) c [i & 0x3];
	} else {
		m = (n + 1) / 2;
		i = 0;
		j = 0;
		while (m--) {
			PIXT [i] = (byte)  (c [j] >> 4); i++;
			PIXT [i] = (byte) ((c [j] & 0xf) << 4);
			if (j == 3)
				j = 0;
			else
				j++;
			PIXT [i] |= (byte) ((c [j] >> 8) & 0xf); i++;
			PIXT [i] = (byte) c [j]; i++;
			if (j == 3)
				j = 0;
			else
				j++;
		}
	}

	proceed (RS_RCMN);

  entry (RS_SETI)

	
	c [0] = 0; c [1] = 0; c [2] = 130; c [3] = 130;
	scan (ibuf+1, "%u %u %u %u %u", c+0, c+1, c+2, c+3, &m);

	lcdg_set (
		(byte)(c[0]),
		(byte)(c[1]),
		(byte)(c[2]),
		(byte)(c[3]), m);

	proceed (RS_RCMN);

  entry (RS_DISP)

	if (PIXT == NULL)
		proceed (RS_ERR);

	c [0] = 0;
	c [1] = 0;
	scan (ibuf+1, "%u %u", c+0, c+1);

	lcdg_render ((byte)(c[0]), (byte)(c[1]), PIXT, NPIX);
	proceed (RS_RCMN);
	
  entry (RS_ERASE)

	m = 0;
	scan (ibuf+1, "%u", &m);
	lcdg_clear ((byte)m);
	proceed (RS_RCMN);

  entry (RS_RCMN)

	ser_out (RS_RCMN, "Done\r\n");
	proceed (RS_RCMD);
	
endprocess (1)
