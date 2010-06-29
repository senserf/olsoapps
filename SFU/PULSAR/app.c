/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "pinopts.h"

#include "ser.h"
#include "serf.h"
#include "form.h"

char	ibuf [32];
word 	ipwait, llimit, hlimit, current;

void set_interval () {

	ipwait = (word)(((lword) 60 * 1024) / current);
}



#define	PU_START	0
#define	PU_OFF		1

thread (pulsar)


    entry (PU_START)

	pin_write (0, 1);
	delay (100, PU_OFF);
	release;

    entry (PU_OFF)

	pin_write (0, 0);

	if (ipwait < 100)
		delay (1024, PU_START);
	else
		delay (ipwait - 100, PU_START);
	release;

endthread


#define	RS_INIT		0
#define	RS_SET		1
#define	RS_CHANGE	2

thread (root)

  word del;

  entry (RS_INIT)

	current = 65;

	runthread (pulsar);

  entry (RS_SET)

	ser_outf (RS_SET, "RATE: %d\r\n", current);

	set_interval ();

	del = rnd () & 0x3f;

	if (del < 5)
		del = 5;

	if (del > 60)
		del = 60;

 	delay (del * 1024, RS_CHANGE);
	release;

  entry (RS_CHANGE)

	do {
		current = 40 + rnd () % 0x7ff;
	} while (current > 145);

	proceed (RS_SET);

endthread

