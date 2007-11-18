/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"

heapmem {10, 90};

#include "ser.h"
#include "serf.h"
#include "form.h"

#define	IBUFLEN		82

#define	RS_INIT		00
#define	RS_RCMD		10
#define	RS_GADC		20
#define	RS_GADA		30
#define	RS_SPIN		40
#define	RS_GPIN		50
#define	RS_RSHT		60

thread (root)

	static char *ibuf;
	static word p [5];

  entry (RS_INIT)

	ibuf = (char*) umalloc (IBUFLEN);

  entry (RS_RCMD-2)

	ser_out (RS_RCMD-2,
		"\r\nSensor Test\r\n"
		"Commands:\r\n"
		"x p r d  -> read ADC pin 'p', ref r, del d\r\n"
		"v p r h d n -> read ADC pin avg: p, r, hold, del, n\r\n"
		"y p v    -> set pin 'p' to v (0/1)\r\n"
		"z p      -> show the value of pin 'p'\r\n"
		"r w      -> read SHT sensor value (0-temp, 1-humid)\r\n"
		);

  entry (RS_RCMD)

	ser_in (RS_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {
		case 'x' : proceed (RS_GADC);
		case 'v' : proceed (RS_GADA);
		case 'y' : proceed (RS_SPIN);
		case 'z' : proceed (RS_GPIN);
		case 'r' : proceed (RS_RSHT);
	}

  entry (RS_RCMD+1)

	ser_out (RS_RCMD+1, "Illegal command or parameter\r\n");
	proceed (RS_RCMD-2);

  entry (RS_GADC)

	p [0] = 0;
	p [1] = 0;
	p [2] = 1;
	scan (ibuf + 1, "%u %u %u", p+0, p+1, p+2);

  entry (RS_GADC+1)

	p [0] = pin_read_adc (RS_GADC+1, p [0], p [1], p [2]);

  entry (RS_GADC+2)

	ser_outf (RS_GADC+2, "Value: %u\r\n", p [0]);
	proceed (RS_RCMD);

  entry (RS_GADA)

	p [0] = 0;
	p [1] = 0;
	p [2] = 0;
	p [3] = 0;
	p [4] = 1;
	scan (ibuf + 1, "%u %u %u %u %u", p+0, p+1, p+2, p+3, p+4);

  entry (RS_GADA+1)

	p [0] = pin_read_adc_avg (RS_GADA+1, p [0], p [1], p [2], p [3], p [4]);

  entry (RS_GADA+2)

	ser_outf (RS_GADA+2, "Value: %u\r\n", p [0]);
	proceed (RS_RCMD);

  entry (RS_SPIN)

	p [0] = 0;
	p [1] = 0;
	scan (ibuf + 1, "%u %u", p+0, p+1);
	pin_write (p [0], p [1]);
	proceed (RS_RCMD);

  entry (RS_GPIN)

	p [0] = 1;
	scan (ibuf + 1, "%u", p+0);
	p [0] = pin_read (p [0]);

  entry (RS_GPIN+1)

	ser_outf (RS_GADC+1, "Value: %u\r\n", p [0]);

  entry (RS_RSHT)

	p [0] = 0;
	scan (ibuf + 1, "%u", p+0);

  entry (RS_RSHT+1)

	p [1] = shtxx_read (RS_RSHT+1, p [0]);

  entry (RS_RSHT+2)

	ser_outf (RS_RSHT+2, "Value: %u [%x]\r\n", p [1], p [1]);
	proceed (RS_RCMD);

endthread
