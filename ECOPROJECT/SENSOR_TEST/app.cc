/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2011                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "sensors.h"
#include "board_pins.h"

#ifdef	__pi_cma_3000_bring_up
#include "cma_3000.h"
#define	CMA3000
#endif

heapmem {10, 90};

#include "ser.h"
#include "serf.h"
#include "form.h"

#define	IBUFLEN		82
#define	MAXVALLEN	16

static word vl;			// Sensor value length
static word sen [MAXVALLEN];	// Sensor value

fsm outval {
//
// Write sensor value
//
	word cnt;

	state INIT:

		if (vl == 1) {
			ser_outf (INIT, "Value: %x [%u]\r\n", sen [0], sen [0]);
			finish;
		}

		ser_outf (INIT, "========\r\n");
		cnt = 0;

	state NEXT:

		if (cnt >= vl)
			// Done
			finish;

		ser_outf (NEXT, "Value [%d] = %x [%u]\r\n", cnt,
					sen [cnt], sen [cnt]);
		cnt++;
		sameas NEXT;
}
		
fsm root {

	char *ibuf;
	word v, x;

  state RS_INIT:

	ibuf = (char*) umalloc (IBUFLEN);
	vl = 1;

  state RS_BANNER:

	ser_out (RS_BANNER,
		"\r\nSensor Test\r\n"
		"Commands:\r\n"
#ifdef CMA3000
		"C n      -> CMA3000 on, mode n\r\n"
		"F        -> CMA3000 off\r\n"
#endif
		"s n      -> set sensor value length (words)\r\n"
		"r s      -> read value of sensor s\r\n"
		"c s d    -> read value of sensor s continually at d int\r\n"
#if 0
		"x        -> turn reference on\r\n"
		"t        -> test on\r\n"
		"e        -> test off\r\n"
#endif
		);

  state RS_RCMD:

	ser_in (RS_RCMD, ibuf, IBUFLEN);

	switch (ibuf [0]) {
#ifdef CMA3000
		case 'C' : proceed RS_CMO;
		case 'F' : proceed RS_CMF;
#endif
		case 's' : proceed RS_SVAL;
		case 'r' : proceed RS_GSEN;
		case 'c' : proceed RS_CSEN;
#if 0
		case 'x' : proceed RS_CSET;
		case 't' : proceed RS_CTST;
		case 'e' : proceed RS_CTSE;
#endif
	}

  state RS_ERR:

	ser_out (RS_ERR, "Illegal command or parameter\r\n");
	proceed RS_BANNER;

  state RS_SVAL:

	vl = 0;
	scan (ibuf + 1, "%u", &vl);
	if (vl == 0)
		vl = 1;
	else if (vl > 16)
		vl = 16;
	proceed RS_RCMD;

  state RS_GSEN:

	v = 0;
	scan (ibuf + 1, "%u", &v);
	bzero (sen, sizeof (sen));

  state RS_GSEN_READ:

	read_sensor (RS_GSEN_READ, v, sen);
	join (runfsm outval, RS_RCMD);
	release;

  state RS_CSEN:

	v = 0;
	x = 1024;
	scan (ibuf + 1, "%u %u", &v, &x);
	bzero (sen, sizeof (sen));

  state RS_CSEN_READ:

	read_sensor (RS_CSEN_READ, v, sen);
	join (runfsm outval, RS_CSEN_NEXT);
	release;

  state RS_CSEN_NEXT:

	delay (x, RS_CSEN_READ);
	release;

#if 0
  state RS_CSET:

	EREF_ON;
	proceed RS_RCMD;

  state RS_CTST:

	SENSOR_TEST;
	proceed RS_RCMD;

  state RS_CTSE:

	SENSOR_TEST_END;
	proceed RS_RCMD;

#endif

#ifdef CMA3000

  state RS_CMO:

	v = 0;
	scan (ibuf + 1, "%u", &v);
	cma_3000_on (v);
	proceed RS_RCMD;

  state RS_CMF:

	cma_3000_off ();
	proceed RS_RCMD;

#endif

}
