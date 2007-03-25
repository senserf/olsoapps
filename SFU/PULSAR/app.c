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
word 	ipwait;

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
#define	RS_RCMD		1

thread (root)

  word a;

  entry (RS_INIT)

	ser_out (RS_INIT,
		"\r\nPulsar\r\n"
		"Commands:\r\n"
		"n        -> beats per minute\r\n"
	);

	ipwait = 1024;	// 60 per minute

	if (!running (pulsar))
		runthread (pulsar);

  entry (RS_RCMD)

	ser_in (RS_RCMD, ibuf, 31);

	a = 0;
	scan (ibuf, "%u", &a);
	if (a < 10 || a > 200)
		proceed (RS_INIT);

	ipwait = (word)(((lword) 60 * 1024) / a);
	proceed (RS_RCMD);

endthread

