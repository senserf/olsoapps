/*
	Copyright 2002-2020 (C) Olsonet Communications Corporation
	Programmed by Pawel Gburzynski & Wlodek Olesinski
	All rights reserved

	This file is part of the PICOS platform

*/

#include "params.h"
#include "plug_null.h"
#include "ser.h"
#include "serf.h"
#include "phys_cc1100.h"
#include "cc1100.h"

#define	LY		0	// Yellow
#define	LR		2	// Red
#define	LG		1	// Green
#define	LN		15	// All off

#define	NB		1024	// Normal blink time
#define	FB		512	// Fast blink time
#define	VB		128	// Very fast blink time

typedef	struct {
	word	L:4,	// LED
		D:12;	// Time
} blink_t;

const blink_t PRST [] = { { LR, NB }, { LG, NB }, { LY, NB }, { 0, 0 } };

const blink_t PPNO [] = { { LR, VB }, { LG, VB }, { LY, VB }, { 0, 0 } };

const blink_t PPOK [] = { { LG, NB }, { LN, NB }, { 0, 0 } };

const blink_t PPS3 [] = { { LY, NB }, { LN, NB }, { LG, NB }, { LN, NB },
			  { 0, 0 } };

const blink_t PPS2 [] = { { LY, NB }, { LN, NB }, { 0, 0 } };

const blink_t PPS1 [] = { { LR, NB }, { LN, NB }, { LY, NB }, { LN, NB },
			  { 0, 0 } };

const blink_t PPS0 [] = { { LR, NB }, { LN, NB }, { 0, 0 } };

// LED scheme:
//
// 	RESET:		alternating R-G-Y
//	No msg for 1h:	alternating R-G-Y fast
//	None: 		G blink
//	s3:		G blink Y blink
//	s2:   		Y blink
//	s1:   		R blink Y blink
//	s0:   		R blink
//

// ============================================================================

blink_t *BP, *LP = NULL;

fsm blinker {

	int c;

	state LOOP:

		word LED, Delay;

		if (LP != BP) {
			LP = BP;
			c = 0;
		}

		LED = LP [c] . L;
		Delay = LP [c] . D;

		if (LP [++c] . D == 0)
			c = 0;

		leds (LR, 0);
		leds (LG, 0);
		leds (LY, 0);
		if (LED != LN)
			leds (LED, 1);

		delay (Delay, LOOP);
}

// ===========================================================================

const word NID = NETWORK_ID;

Boolean sen [4];

lword last_report_time = 0;

fsm monitor {

	state LOOP:

		if (last_report_time == 0)
			// Nothing to do yet
			sameas NEXT;

		if (seconds () - last_report_time >= REPORT_TIMEOUT) {
			BP = (blink_t*)PPNO;
			sameas NEXT;
		}

		if (sen [0])
			BP = (blink_t*)PPS0;
		else if (sen [1])
			BP = (blink_t*)PPS1;
		else if (sen [2])
			BP = (blink_t*)PPS2;
		else if (sen [3])
			BP = (blink_t*)PPS3;
		else
			BP = (blink_t*)PPOK;

	state NEXT:

		delay (MONITOR_INTERVAL, LOOP);
}

fsm root {

	sint SFD;
	address packet;

	state INIT:

		phys_cc1100 (0, CC1100_MAXPLEN);
		tcv_plug (0, &plug_null);
		SFD = tcv_open (WNONE, 0, 0);
		tcv_control (SFD, PHYSOPT_SETSID, (address) &NID);
		tcv_control (SFD, PHYSOPT_ON, NULL);
		BP = (blink_t*) PRST;
		runfsm blinker;
		runfsm monitor;

	state WPACKET:

		sint i;

		packet = tcv_rnp (WPACKET, SFD);

		if (tcv_left (packet) != 14)
			proceed SPURIOUS;
	
		if (packet [1] != MAGIC)
			proceed SPURIOUS;

		for (i = 0; i < 4; i++) 
			sen [i] = packet [2 + i] < TRIGGER_VALUE;

		last_report_time = seconds ();

	state DONE:

		ser_outf (DONE, "REP: %x, %x, %x, %x = %d, %d, %d, %d\r\n",
			packet [2 + 0],
			packet [2 + 1],
			packet [2 + 2],
			packet [2 + 3],
			sen [0],
			sen [1],
			sen [2],
			sen [3]);
ENDP:
		tcv_endp (packet);
		proceed WPACKET;

	state SPURIOUS:

		ser_outf (SPURIOUS, "SPU: <%d> %x\r\n",
			tcv_left (packet), packet [1]);

		goto ENDP;
}
