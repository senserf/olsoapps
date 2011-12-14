/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
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
#define	RS_SET		20
#define	RS_REA		30
#define	RS_ADC		40
#define	RS_DAC		50
#define	RS_DON		60

process (root, int)

	static char *ibuf;
	static int n, k;
	static word a, b, c;

  entry (RS_INIT)

	ibuf = (char*) umalloc (IBUFLEN);

  entry (RS_RCMD-2)

	ser_out (RS_RCMD-2,
		"\r\nPin Test\r\n"
		"Commands:\r\n"
		"s n v    -> set pin n to v (see pin_read.c)\r\n"
		"r n      -> read pin n\r\n"
		"a p d r  -> read analog pin p with delay d, reference r\r\n"
		"w p v r  -> write analog pin p with value v, reference r\r\n"
	);

  entry (RS_RCMD)

	ser_in (RS_RCMD, ibuf, IBUFLEN-1);

	if (ibuf [0] == 's')
		proceed (RS_SET);
	if (ibuf [0] == 'r')
		proceed (RS_REA);
	if (ibuf [0] == 'a')
		proceed (RS_ADC);
	if (ibuf [0] == 'w')
		proceed (RS_DAC);

  entry (RS_RCMD+1)

	ser_out (RS_RCMD+1, "Illegal command or parameter\r\n");
	proceed (RS_RCMD-2);

  entry (RS_SET)

	a = b = 0;
	scan (ibuf + 1, "%d %d", &a, &b);
	pin_write (a, b);
	proceed (RS_DON);

  entry (RS_REA)

	a = 0;
	scan (ibuf + 1, "%d", &a);
	diag ("PIN [%u] = %x", a, pin_read (a));
	proceed (RS_RCMD);

  entry (RS_ADC)

	a = b = c = 0;
	scan (ibuf + 1, "%d %d %d", &a, &b, &c);

  entry (RS_ADC+1)

	n = pin_read_adc (NONE, a, c, b);
	diag ("PIN [%u] = %d [%x]", a, n, n);
	proceed (RS_RCMD);

  entry (RS_DAC)

	a = b = c = 0;
	scan (ibuf + 1, "%d %d %d", &a, &b, &c);
	n = pin_write_dac (a, b, c);
	proceed (RS_DON);

  entry (RS_DON)

	ser_out (RS_DON, "Done\r\n");
	proceed (RS_RCMD);

	nodata;

endprocess
