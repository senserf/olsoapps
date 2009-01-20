/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2008                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "pinopts.h"

#include "ser.h"
#include "serf.h"
#include "form.h"

#define	IBUFLEN	80

#define	RS_INIT		0
#define	RS_RCMD		10
#define	RS_ONN		20
#define	RS_OFF		30
#define	RS_CLE		40
#define	RS_DIS		50
#define	RS_MAR		60
#define	RS_DON		100

#define	FS_LOOP	0

process (flasheur, void)

  word pos, led, sta;

  entry (FS_LOOP)

	while (1) {
		led = rnd ();
		pos = led & 0x1f;
		if (pos > 11) {
			if (pos > 27 || pos < 16)
				continue;
		}
		sta = (led >> 9) & 1;
		led = (led >> 8) & 1;
		break;
	}

	lcd_clear (0, 0);
	lcd_write (pos, "CIBER");
	leds (led, sta);

	delay (1024, FS_LOOP);

endprocess (1)
	
process (root, int)

	static char *ibuf, *sbuf;
	static int n, k;
	static word a, b, c;

  entry (RS_INIT)

	ibuf = (char*) umalloc (IBUFLEN);
	sbuf = (char*) umalloc (IBUFLEN);

  entry (RS_RCMD-2)

	ser_out (RS_RCMD-2,
		"\r\nLCD Test\r\n"
		"Commands:\r\n"
		"o p      -> on\r\n"
		"f        -> off\r\n"
		"e f n    -> clear\r\n"
		"w n str  -> display string\r\n"
		"m        -> marquee\r\n"
	);

  entry (RS_RCMD)

	ser_in (RS_RCMD, ibuf, IBUFLEN-1);

	if (ibuf [0] == 'o')
		proceed (RS_ONN);
	if (ibuf [0] == 'f')
		proceed (RS_OFF);
	if (ibuf [0] == 'e')
		proceed (RS_CLE);
	if (ibuf [0] == 'w')
		proceed (RS_DIS);
	if (ibuf [0] == 'm')
		proceed (RS_MAR);

  entry (RS_RCMD+1)

	ser_out (RS_RCMD+1, "Illegal command or parameter\r\n");
	proceed (RS_RCMD-2);

  entry (RS_MAR)

	if (running (flasheur))
		killall (flasheur);
	else
		fork (flasheur, NULL);
	proceed (RS_DON);

  entry (RS_ONN)

	a = 0;
	scan (ibuf + 1, "%x", &a);
	lcd_on (a);
	proceed (RS_DON);

  entry (RS_OFF)

	lcd_off ();
	proceed (RS_DON);

  entry (RS_CLE)

	a = b = 0;
	scan (ibuf + 1, "%d %d", &a, &b);
	lcd_clear (a, b);
	proceed (RS_DON);

  entry (RS_DIS)

	scan (ibuf + 1, "%d %s", &a, sbuf);
	lcd_write (a, sbuf);
	
  entry (RS_DON)

	ser_out (RS_DON, "Done\r\n");
	proceed (RS_RCMD);

	nodata;

endprocess (1)
