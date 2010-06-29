/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "pinopts.h"

#include "ser.h"
#include "serf.h"
#include "form.h"

#define	IBUFLEN	82

const	word	RATE115	= 1152,
		RATE096 = 96;

// P1.5 ATT
// P1.6 ESC
// P1.7 RES

static void pinit () {

	_BIC (P1OUT, 0xE0);
	_BIC (P1SEL, 0xE0);
	_BIC (P1DIR, 0x20);
	_BIS (P1DIR, 0xC0);
}

static void escape () {

	_BIS (P1OUT, 0x40);
	mdelay (2);
	_BIC (P1OUT, 0x40);
}

static void mreset () {

	_BIS (P1OUT, 0x80);
	mdelay (100);
	_BIC (P1OUT, 0x80);
}

static word attention () {

	return (P1IN & 0x20) != 0;
}

thread (minput)

#define	MI_READ		0
#define	MI_WRITE	1

  static char c;

  entry (MI_READ)

	io (MI_READ, UART_B, READ, &c, 1);

  entry (MI_WRITE)

	io (MI_WRITE, UART_A, WRITE, &c, 1);
	proceed (MI_READ);

endthread

thread (root)

#define	RS_INI	0
#define	RS_MEN	1
#define	RS_RCM	2
#define	RS_WRI	3
#define	RS_RAT	4
#define	RS_ESC	5
#define	RS_RES	6
#define	RS_ATT	7
#define	RS_RAU	8

#define	RS_WRL	9
#define	RS_ERR	10

	static char *ibuf;
	static int n, k, p;

  entry (RS_INI)

	pinit ();
	ibuf = (char*) umalloc (IBUFLEN + 2);
	//ion (UART_A, CONTROL, (char*)(&RATE096), UART_CNTRL_SETRATE);
	//ion (UART_B, CONTROL, (char*)(&RATE096), UART_CNTRL_SETRATE);

	runthread (minput);

  entry (RS_MEN)

	ser_out (RS_MEN,
		"\r\nBlueTooth Test\r\n"
		"Commands:\r\n"
		"w string    -> write line to module\r\n"
		"r rate      -> set rate for module\r\n"
		"t rate      -> set rate for UART\r\n"
		"e           -> escape\r\n"
		"s           -> reset module\r\n"
		"a           -> view attention flag\r\n"
	);

  entry (RS_RCM)

	ser_in (RS_RCM, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {

	    case 'w': proceed (RS_WRI);
	    case 'r': proceed (RS_RAT);
	    case 't': proceed (RS_RAU);
	    case 'e': proceed (RS_ESC);
	    case 's': proceed (RS_RES);
	    case 'a': proceed (RS_ATT);

	}

  entry (RS_ERR)

	ser_out (RS_ERR, "Illegal command or parameter\r\n");
	proceed (RS_MEN);

  entry (RS_WRI)

	for (k = 1; ibuf [k] == ' '; k++);
	n = strlen (ibuf + k);

	ibuf [k + n    ] = '\r';
	ibuf [k + n + 1] = '\n';

	n += 2;
	ibuf [k + n    ] = '\0';

  entry (RS_WRL)

	while (n) {
		p = io (RS_WRL, UART_B, WRITE, ibuf + k, n);
		n -= p;
		k += p;
	}

	proceed (RS_RCM);

  entry (RS_RAT)

	n = 96;
	scan (ibuf + 1, "%d", &n);
	ion (UART_B, CONTROL, (char*)(&n), UART_CNTRL_SETRATE);
	proceed (RS_RCM);

  entry (RS_RAU)

	n = 96;
	scan (ibuf + 1, "%d", &n);
	ion (UART_A, CONTROL, (char*)(&n), UART_CNTRL_SETRATE);
	proceed (RS_RCM);

  entry (RS_ESC)

	escape ();
	proceed (RS_RCM);

  entry (RS_RES)

	mreset ();
	proceed (RS_RCM);

  entry (RS_ATT);

	ser_outf (RS_ATT, "ATT: %d", attention ());
	proceed (RS_RCM);

endthread
