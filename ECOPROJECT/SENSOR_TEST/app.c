/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "sensors.h"
#include "board_pins.h"

heapmem {10, 90};

#include "ser.h"
#include "serf.h"
#include "form.h"

#define	IBUFLEN		82

#define	RS_INIT		00
#define	RS_RCMD		10
#define	RS_GSEN		20
#define	RS_CSEN		30
#define	RS_CSET		40

thread (root)

	static char *ibuf;
	static word p [5];

  entry (RS_INIT)

	ibuf = (char*) umalloc (IBUFLEN);

  entry (RS_RCMD-2)

	ser_out (RS_RCMD-2,
		"\r\nSensor Test\r\n"
		"Commands:\r\n"
		"r s      -> read value of sensor s\r\n"
		"c s d    -> read value of sensor s continually at d int\r\n"
		"x        -> turn reference on\r\n"
		);

  entry (RS_RCMD)

	ser_in (RS_RCMD, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {
		case 'r' : proceed (RS_GSEN);
		case 'c' : proceed (RS_CSEN);
		case 'x' : proceed (RS_CSET);
	}

  entry (RS_RCMD+1)

	ser_out (RS_RCMD+1, "Illegal command or parameter\r\n");
	proceed (RS_RCMD-2);

  entry (RS_GSEN)

	p [0] = 0;
	scan (ibuf + 1, "%u", p+0);

  entry (RS_GSEN+1)

	read_sensor (RS_GSEN+1, p [0], p+1);

  entry (RS_GSEN+2)

	ser_outf (RS_GSEN+2, "Value: %u\r\n", p [1]);
	proceed (RS_RCMD);

  entry (RS_CSEN)

	p [0] = 0;
	p [1] = 1024;
	scan (ibuf + 1, "%u", p+0);

  entry (RS_CSEN+1)

	read_sensor (RS_CSEN+1, p [0], p+2);

  entry (RS_CSEN+2)

	ser_outf (RS_GSEN+2, "Value: %u\r\n", p [2]);
	delay (p [1], RS_CSEN+1);
	release;

  entry (RS_CSET)

	EREF_ON;
	proceed (RS_RCMD);

endthread
