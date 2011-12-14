/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2002 - 2006                    */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "tcvphys.h"
#include "sensors.h"
#include "board_pins.h"
#include "plug_null.h"

heapmem {10, 90};

#include "ser.h"
#include "serf.h"
#include "form.h"

#define	IBUFLEN		82
#define	MAXPLEN		48

lword until;		// Target second
char *ibuf;
word p [8];		// Operation parameters
int sfd;		// Session descriptor
address packet;

static void ipar () {

	word i;

	for (i = 0; i < sizeof (p) / 2; i++)
		p [i] = 0;

	scan (ibuf + 1, "%u %u %u %u %u %u %u %u", 
		p + 0, p + 1, p + 2, p + 3, p + 4, p + 5, p + 6, p + 7);
}

static void drain () {

	while ((packet = tcv_rnp (WNONE, sfd)) != NULL)
		tcv_endp (packet);
}

#define	RS_INIT		00
#define	RS_RCMD_M	10
#define	RS_RCMD		20
#define	RS_RCMD_U	25
#define	RS_RCMD_I	30
#define	RS_PAT		40
#define	RS_PAT_X	50
#define	RS_PAT_N	60
#define	RS_PAT_R	70
#define	RS_PAT_S	80
#define	RS_PAT_L	90
#define	RS_PAT_M	100
#define	RS_PAT_F	110
#define	RS_PAT_G	115
#define	RS_POW		120
#define	RS_RAT		130
#define	RS_FRE		140
#define	RS_XMI		150
#define	RS_XMI_N	160
#define	RS_RCV		170
#define	RS_RCV_S	180
#define	RS_LOP		190
#define	RS_LOP_M	200
#define	RS_ADC		210
#define	RS_ADC_A	215
#define	RS_POR		220

thread (root)

  entry (RS_INIT)

	ibuf = (char*) umalloc (IBUFLEN);
	phys_cc1100 (0, MAXPLEN);
	tcv_plug (0, &plug_null);
	sfd = tcv_open (NONE, 0, 0);
	p [0] = 1;
	tcv_control (sfd, PHYSOPT_SETRATE, p + 0);

  entry (RS_RCMD_M)

	ser_out (RS_RCMD_M,
		"\r\nPower Test\r\n"
		"Commands:\r\n"
		"p x r l f   -> xmit, rcv, loop, freeze (infinite)\r\n"
		"v n         -> xmit power level (default = 7)\r\n"
		"s n         -> xmit rate (default = 1)\r\n"
		"f n         -> freeze for n seconds\r\n"
		"x n         -> transmit for n seconds\r\n"
		"r n         -> receive for n seconds\r\n"
		"l n         -> loop for n seconds\r\n"
		"a n         -> read analog sensor for n seconds\r\n"
		"o d o       -> port setting 0-7 dir out\r\n"
	);

  entry (RS_RCMD)

	ser_out (RS_RCMD, "READY\r\n");

	// Start idle
	tcv_control (sfd, PHYSOPT_RXOFF, NULL);
	tcv_control (sfd, PHYSOPT_TXHOLD, NULL);

  entry (RS_RCMD_U)

	ser_in (RS_RCMD_U, ibuf, IBUFLEN-1);

	switch (ibuf [0]) {
		case 'p' : proceed (RS_PAT);
		case 'v' : proceed (RS_POW);
		case 's' : proceed (RS_RAT);
		case 'f' : proceed (RS_FRE);
		case 'x' : proceed (RS_XMI);
		case 'r' : proceed (RS_RCV);
		case 'l' : proceed (RS_LOP);
		case 'a' : proceed (RS_ADC);
		case 'o' : proceed (RS_POR);
	}

  entry (RS_RCMD_I)

	ser_out (RS_RCMD_I, "Illegal command or parameter\r\n");
	proceed (RS_RCMD_M);

// ============================================================================

  entry (RS_PAT)

	ipar ();

  entry (RS_PAT_X)

	if (p [0] == 0)
		proceed (RS_PAT_R);

	tcv_control (sfd, PHYSOPT_TXON, NULL);
	tcv_control (sfd, PHYSOPT_RXON, NULL);

	until = seconds () + p [0];

  entry (RS_PAT_N)

	if (until < seconds ()) {
		tcv_control (sfd, PHYSOPT_TXHOLD, NULL);
		delay (100, RS_PAT_R);
		release;
	}
	packet = tcv_wnp (RS_PAT_N, sfd, MAXPLEN - 2);
	packet [0] = 0;
	packet [1] = 0xBABA;
	tcv_endp (packet);

	delay (128, RS_PAT_N);
	release;

  entry (RS_PAT_R)

	if (p [1] == 0) {
		tcv_control (sfd, PHYSOPT_RXOFF, NULL);
		delay (100, RS_PAT_L);
		release;
	}

	until = seconds () + p [1];
	tcv_control (sfd, PHYSOPT_RXON, NULL);

  entry (RS_PAT_S)

	drain ();
	if (until < seconds ()) {
		tcv_control (sfd, PHYSOPT_RXOFF, NULL);
		delay (100, RS_PAT_L);
		release;
	}

	delay (256, RS_PAT_S);
	release;

  entry (RS_PAT_L)

	if (p [2] == 0)
		proceed (RS_PAT_F);
	until = seconds () + p [2];

  entry (RS_PAT_M)

	if (until >= seconds ()) {
		mdelay (100);
		proceed (RS_PAT_M);
	}

  entry (RS_PAT_F)

	if (p [3] == 0)
		proceed (RS_PAT_X);

	delay (100, RS_PAT_G);
	release;

  entry (RS_PAT_G)

	freeze (p [3]);
	proceed (RS_PAT_X);

// ============================================================================

  entry (RS_POW)

	ipar ();

	tcv_control (sfd, PHYSOPT_SETPOWER, p + 0);
	proceed (RS_RCMD);

  entry (RS_RAT)

	ipar ();

	tcv_control (sfd, PHYSOPT_SETRATE, p + 0);
	proceed (RS_RCMD);

  entry (RS_FRE)

	ipar ();

	freeze (p [0]);
	proceed (RS_RCMD);

  entry (RS_XMI)

	ipar ();
		
	tcv_control (sfd, PHYSOPT_TXON, NULL);
	tcv_control (sfd, PHYSOPT_RXON, NULL);

	until = seconds () + p [0];

  entry (RS_XMI_N)

	if (until < seconds ())
		proceed (RS_RCMD);

	packet = tcv_wnp (RS_XMI_N, sfd, MAXPLEN - 2);
	packet [0] = 0;
	packet [1] = 0xBACA;
	tcv_endp (packet);

	delay (128, RS_XMI_N);
	release;

  entry (RS_RCV)

	ipar ();

	until = seconds () + p [0];
	tcv_control (sfd, PHYSOPT_RXON, NULL);

  entry (RS_RCV_S)

	drain ();
	if (until < seconds ())
		proceed (RS_RCMD);

	delay (256, RS_RCV_S);
	release;

  entry (RS_LOP)

	ipar ();

	until = seconds () + p [0];

  entry (RS_LOP_M)

	if (until >= seconds ()) {
		mdelay (100);
		proceed (RS_LOP_M);
	}

	proceed (RS_RCMD);

  entry (RS_ADC)

	ipar ();

	until = seconds () + p [0];

  entry (RS_ADC_A)

	if (until >= seconds ()) {
		read_sensor (RS_ADC_A, 0, p+1);
		proceed (RS_ADC_A);
	}

	proceed (RS_RCMD);

  entry (RS_POR)

	if (p [0] > 7 || p [0] == 0)
		proceed (RS_RCMD_I);

	switch (p [0]) {

		case 1 : P1DIR = (byte) p [1]; P1OUT = (byte) p [2]; break;
		case 2 : P2DIR = (byte) p [1]; P2OUT = (byte) p [2]; break;
		case 3 : P3DIR = (byte) p [1]; P3OUT = (byte) p [2]; break;
		case 4 : P4DIR = (byte) p [1]; P4OUT = (byte) p [2]; break;
		case 5 : P5DIR = (byte) p [1]; P5OUT = (byte) p [2]; break;
		case 6 : P6DIR = (byte) p [1]; P6OUT = (byte) p [2]; break;
		case 7 : P7DIR = (byte) p [1]; P7OUT = (byte) p [2]; break;
	}

	proceed (RS_RCMD);
		
endthread
