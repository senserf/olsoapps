/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2010                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "node.h"
#include "sysio.h"
#include "ser.h"
#include "serf.h"
#include "rtc_cc430.h"

#define	IBUF_LENGTH		82

#ifdef	__SMURPH__

threadhdr (root, Node) {

	states { RS_INIT, RS_RCMD_M, RS_RCMD, RS_ECHO, RS_SETRATE, RS_GETDONE,
			RS_SETDONE, RS_CLOCK, RS_CDONE, RS_CSHOW, RS_COUT };

	perform;
};

#define	w	_dan (Node, w)
#define	res	_dan (Node, res)
#define	td	_dan (Node, td)
#define	ibuf	_dan (Node, ibuf)

#else	/* PICOS */

#include "app_data.h"

#define	RS_INIT		0
#define	RS_RCMD_M	1
#define	RS_RCMD		2
#define	RS_ECHO		3
#define	RS_SETRATE	4
#define	RS_GETDONE	5
#define	RS_SETDONE	6
#define	RS_CLOCK	7
#define	RS_CDONE	8
#define	RS_CSHOW	9
#define	RS_COUT		10

#endif	/* VUEE or PICOS */

// ============================================================================

thread (root)

  word tmp [7];

  entry (RS_INIT)

	ibuf = (char*) umalloc (IBUF_LENGTH);

  entry (RS_RCMD_M)

	ser_out (RS_RCMD_M,
		"\r\nRF S-R example\r\n"
		"Command:\r\n"
		"r n   -> read or change UART rate\r\n"
		"c ... -> read or set the RTC\r\n"
		"anything else is echoed back\r\n"
	);

  entry (RS_RCMD)

	ser_in (RS_RCMD, ibuf, IBUF_LENGTH);

	switch (ibuf [0]) {

		case 'r': proceed (RS_SETRATE);
		case 'c': proceed (RS_CLOCK);
	}

  entry (RS_ECHO)

	ser_outf (RS_ECHO, "%s\r\n", ibuf);
	proceed (RS_RCMD);

  entry (RS_SETRATE)

	w = 0;
	scan (ibuf + 1, "%u", &w);
	if (w != 0) {
		// Set the rate
		res = ion (UART_A, CONTROL, (char*)&w, UART_CNTRL_SETRATE);
		proceed (RS_SETDONE);
	}

	// Get rate
	w = 0;
	res = ion (UART_A, CONTROL, (char*)&w, UART_CNTRL_GETRATE);

  entry (RS_GETDONE)

	ser_outf (RS_GETDONE, "Rate = %u, code = %d\r\n", w, res);
	proceed (RS_RCMD);

  entry (RS_SETDONE)

	ser_outf (RS_SETDONE, "Done, code = %d\r\n", res);
	proceed (RS_RCMD);

  entry (RS_CLOCK)

	if (scan (ibuf + 1, "%u %u %u %u %u %u %u",
		tmp+0, tmp+1, tmp+2, tmp+3, tmp+4, tmp+5, tmp+6) < 7)
			proceed (RS_CSHOW);

	// Set
	td.year   = (byte) (tmp [0]);
	td.month  = (byte) (tmp [1]);
	td.day    = (byte) (tmp [2]);
	td.dow    = (byte) (tmp [3]);
	td.hour   = (byte) (tmp [4]);
	td.minute = (byte) (tmp [5]);
	td.second = (byte) (tmp [6]);

	rtc_set (&td);

  entry (RS_CDONE)

	ser_out (RS_CDONE, "Done\r\n");
	proceed (RS_RCMD);

  entry (RS_CSHOW)

	memset (&td, 0, sizeof (td));
	rtc_get (&td);

  entry (RS_COUT)

	ser_outf (RS_COUT, "Time: %u %u %u %u %u %u %u\r\n",
		td.year   ,
		td.month  ,
		td.day    ,
		td.dow    ,
		td.hour   ,
		td.minute ,
		td.second );

	proceed (RS_RCMD);
	
endthread

praxis_starter (Node);
