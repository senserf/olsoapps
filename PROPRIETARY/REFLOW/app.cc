/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "sensors.h"

#ifndef	__SMURPH__
#include "board_pins.h"
#else
#define	max6675_on() CNOP
#define	max6675_off() CNOP
#endif

heapmem {10, 90};

#include "ser.h"
#include "serf.h"
#include "form.h"

// ============================================================================

#define	IBUFLEN		82
#define	THERMOCOUPLE	0	// Temperature sensor
#define	OVEN		0	// Oven PWM actuator

// ============================================================================

typedef	struct {

	word Value, Reading;

} calentry_t;

static const calentry_t Cal [] = THERMOCOUPLE_CALIBRATION;

#define	CalLength	(sizeof (Cal) / sizeof (calentry_t))

// ============================================================================

typedef	struct {

	word DeltaT,		// Time to reach the temperature from prev step
	     Temperature;	// Target temperature at the end of this step

} profentry_t;

#define	MINPROFENTRIES	3
#define	MAXPROFENTRIES	24

static profentry_t Prof [MAXPROFENTRIES]

#if	!defined(DEFAULT_PROFILE_N) || !defined(DEFAULT_PROFILE)
#undef	DEFAULT_PROFILE_N
#undef	DEFAULT_PROFILE
#define	DEFAULT_PROFILE_N	0
#endif

#if DEFAULT_PROFILE_N
	= DEFAULT_PROFILE
#endif

;

static word NProfEntries = DEFAULT_PROFILE_N;

// ============================================================================

static sint addProfEntry (word sec, word tmp) {

	word i, j;

	if (NProfEntries == MAXPROFENTRIES)
		return 1;

	Prof [NProfEntries].DeltaT = sec;
	Prof [NProfEntries].Temperature = tmp;
	NProfEntries++;
	return 0;
}

// ============================================================================

static word temp (word r) {
//
// Converts the sensor reading to temperature in deg C * 10
//
	word a, b, c, v, v0, v1, r0, r1;

	a = 0;
	b = CalLength - 1;

	while ((c = (a + b) >> 1) != a) {

		if (Cal [c].Reading < r)
			a = c;
		else if (Cal [c].Reading > r)
			b = c;
		else {
RetC:
			// A direct hit
			return Cal [c].Value;
		}
	}

	if (r < Cal [a].Reading) {
		c = a;
		goto RetC;
	}

	if (r > Cal [b].Reading) {
		c = b;
		goto RetC;
	}

	r0 = Cal [a].Reading;
	r1 = Cal [b].Reading;

	v0 = Cal [a].Value * 10;
	v1 = Cal [b].Value * 10;

	v = v0 + (word)(((lword)(v1 - v0) * (r - r0)) / (r1 - r0));

	return (v + 5) / 10;
}

static word OvenSetting;

static void setOven (word val) {

	OvenSetting = val;
	// This actuator needs no state
	write_actuator (WNONE, OVEN, &OvenSetting);
}

// ============================================================================

// The controllable span of error in degrees * 10. Any error larger than that
// in the down direction results in full throttle. The control value is scaled
// to the Span.
static	word	Span  = DEFAULT_SPAN,
		GainI = DEFAULT_INTEGRATOR_GAIN,
		GainD = DEFAULT_DIFFERENTIATOR_GAIN;

// Max setting of PW (pulse witdth)
#define	MAXPWI		1024

static	word	TA,	// Target temperature for this second
		CU,	// Current temperature at this second
		CV;	// Control value

static	sint	ER;
		
static	lword	Integrator,
		Differentiator;

static void controller () {

	lint t;

	ER = (sint) TA - (sint) CU;

	if (ER < 0) {
		// This part is not controlled, just switch the oven off
		Integrator = 0;
		Differentiator = CU;
		CV = 0;
		return;
	}

	if (ER >= Span) {
		// Large error, full throttle, but clear the integrator (not
		// sure if this is right)
		Integrator = 0;
		Differentiator = CU;
		CV = MAXPWI;
		return;
	}

	// The first component: proportional to error, normalized to Span
	t = (lint)ER * MAXPWI;

	// The integrator component: GainI is between 0 and 1024
	t += Integrator * GainI;

	// The differentiator component: GainD is between 0 and 1024
	t -= ((lint)CU - Differentiator) * GainD;

	// Normalize
	t /= Span;

	if (t < 0)
		CV = 0;
	else if (t > MAXPWI)
		CV = MAXPWI;
	else
		CV = (word) t;
		
	// We don't bound the integrator for now, let's see what happens
	Integrator += ER;

	Differentiator = CU;
}

// ============================================================================

word	FT,		// Final temperature at the end of segment
	ST,		// Starting temperature at the beginning of segment
	FS,		// Target time (final second of the segment)
	SS,		// Starting time of the segment
	CS,		// Current second of the run
	SD;		// Segment duration

sint	CP;		// Current profile segment

Boolean	UP,		// Direction (heating, cooling)
	TR;		// Target reached

lint	TD;		// Temperature difference across the segment

lword	LS;		// Last second

sint	Notifier;

static void init_target () {

	CS = FS = 0;
	CP = -1;
	UP = YES;
	FT = CU;
	Differentiator = CU;
	Integrator = 0;
	LS = seconds ();
	TR = YES;
}

static void target () {
//
// Target temperature at time TM
//
	if (!TR)
		// Target temperature not reached yet
		TR = (UP && CU >= FT || !UP && CU <= FT);

	if (TR && CS >= FS) {
		// Segment done
		if (++CP >= NProfEntries) {
			TA = 0;
			// Done
			return;
		}
		ST = FT;
		FT = Prof [CP] . Temperature;
		SS = CS;
		FS = CS + (SD = Prof [CP] . DeltaT);
		UP = (FT > CU);
		TD = (lint)FT - (lint)ST;
		TR = NO;
	}

	if (CS >= FS || SD == 0)
		TA = FT;
	else
		TA = (word) (ST + (sint) ( (TD * (CS - SS)) / SD ) );
}

// ============================================================================

fsm notifier {

	state LOOP:

		ser_outf (LOOP, "!T %u [P=%u,U=%u,T=%u] %u->%u, %u\r\n",
			CS, CP, FS, FT, CU, TA, CV);
			
	initial state WAIT_FOR_EVENT:

		when (&Notifier, LOOP);
}


fsm reflow {

	state INIT:

		LS = seconds ();

	state START:

		lword cs;
		word t;

		if ((cs = seconds ()) == LS) {
			delay (1, START);
			release;
		}

		LS = cs;

		// Initial temperature
		read_sensor (WNONE, THERMOCOUPLE, &t);
		CU = temp (t);
		Notifier = runfsm notifier;
		init_target ();

	state LOOP:

		target ();

		if (TA == 0)
			sameas FINISH;
		controller ();
		trigger (&Notifier);

	state NEXT:

		lword cs;
		word t;

		// Adjust to the next second boundary
		if ((cs = seconds ()) == LS) {
			delay (1, NEXT);
			release;
		}

		LS = cs;
		CS++;

		read_sensor (WNONE, THERMOCOUPLE, &t);
		CU = temp (t);
		sameas LOOP;

	state FINISH:

		ser_out (FINISH, "Finishing\r\n");

		delay (1024, DONE);
		release;

	state DONE:

		ser_out (DONE, "End Reflow Cycle\r\n");
		killall (notifier);
		finish;
}

// ============================================================================

static char ibuf [IBUFLEN], *ibp;
static Boolean EF;

static void nosp () {
//
// Skip blanks
//
	while (isspace (*ibp) || *ibp == ',')
		ibp++;
}

static Boolean eol () {
//
// True if ibuf is at EOL
//
	nosp ();
	return (*ibp == '\0');
}

static sint pnum () {
//
// Parse a sint
//
	char c;
	Boolean pos;
	sint res, tmp;

	nosp ();
	pos = YES;
	if (*ibp == '-') {
		pos = NO;
		ibp++;
	} else if (*ibp == '+') {
		ibp++;
	}

	if (!isdigit (*ibp)) {
		EF = YES;
		return 0;
	}

	for (res = 0; isdigit (*ibp); ibp++) {
		tmp = res * 10 - (*ibp - '0');
		if (tmp > res)
			// Overflow
			EF = YES;
		res = tmp;
	}

	if (pos) {
		if ((res = -res) < 0)
			EF = YES;
	}

	return res;
}

// ============================================================================

fsm root {

  word VV, TT;

  state RS_INIT:

	setOven (0);
	// The thermocouple is constantly on
	max6675_on ();

  state RS_BANNER:

	// Note: we can use the same protocol in the target (GUI) praxis.
	// There is no need for checksums/acks, because 1) the 's' command
	// can be used to verify a controller setting, 2) a running reflow
	// process will produce periodic reports

	ser_out (RS_BANNER,
		"\r\nCommands:\r\n"
		" g s i d    -> set parameters\r\n"
		" e          -> erase profile\r\n"
		" p d t ...  -> add profile entry\r\n"
		" s          -> show parameters\r\n"
		" r          -> run\r\n"
		" a          -> abort\r\n"
		" o n        -> set oven\r\n"
		" t          -> read temp\r\n"
	);

  state RS_RCMD:

	ser_out (RS_RCMD, "Ready:\r\n");

  state RS_READ:

	ser_in (RS_READ, ibuf, IBUFLEN);

	// Input, if we are running, force abort
	if (!running (reflow))
		sameas RS_PARSE;

	killall (reflow);
	killall (notifier);
	setOven (0);

  state RS_ABORT:

	ser_out (RS_ABORT, "Aborted\r\n");

  state RS_PARSE:

	ibp = ibuf + 1;
	EF = NO;

	switch (ibuf [0]) {

		case 'g' : proceed RS_SETGAIN;
		case 'e' : proceed RS_PROFERASE;
		case 'p' : proceed RS_PROFSET;
		case 's' : proceed RS_SHOW;
		case 'r' : proceed RS_RUN;
		case 'a' : proceed RS_RCMD;	// Do nothing
		case 'o' : proceed RS_OVEN;
		case 't' : proceed RS_TEMP;
	}

	proceed RS_BANNER;

  state RS_ERR:

	ser_out (RS_ERR, "Error!!!\r\n");
	proceed RS_RCMD;

  state RS_SETGAIN:

	word i;
	sint g;
	lint r [3];

	for (i = 0; i < 3; i++) {
		r [i] = pnum ();
		if (r [i] < 0 || r [1] > 1024)
			proceed RS_ERR;
	}

	// This one cannot be too small
	Span  = r [0] > 10 ? r [0] : 10;
	GainI = r [1];
	GainD = r [2];
	proceed RS_RCMD;

  state RS_PROFERASE:

	NProfEntries = 0;
	proceed RS_RCMD;

  state RS_PROFSET:

	sint s, t, r, d;

	while (!eol ()) {
		s = pnum ();
		if (EF || s < 0) {
PBad:
			NProfEntries = 0;
			proceed RS_ERR;
		}
		t = pnum ();
		if (EF || t < 0)
			goto PBad;
		// Check for decimal tenths
		r = t * 10;
		if (*ibp == '.') {
			ibp++;
			d = pnum ();
			if (EF || d < 0 || d > 9)
				goto PBad;
			r += d;
		}
		if (r < t)
			goto PBad;

		if (addProfEntry ((word)s, (word)r))
			goto PBad;
	}
	proceed RS_RCMD;

  state RS_SHOW:

	ser_outf (RS_SHOW, "S/I/D: %u %u %u, NPE: %u\r\n",
		Span, GainI, GainD, NProfEntries);

	VV = 0;

  state RS_SHOW_PROF:

	if (VV >= NProfEntries)
		proceed RS_RCMD;

	ser_outf (RS_SHOW_PROF, " P%u: %u %u.%u\r\n", VV,
		Prof [VV] . DeltaT,
		Prof [VV] . Temperature / 10,
		Prof [VV] . Temperature % 10);

	VV++;
	proceed RS_SHOW_PROF;

  state RS_RUN:

	if (NProfEntries < MINPROFENTRIES)
		proceed RS_ERR;

	runfsm reflow;
	proceed RS_RCMD;

  state RS_OVEN:

	word v;

	v = pnum ();
	if (EF || v < 0)
		v = 0;
	else if (v > 1024)
		v = 1024;

	setOven (v);
	proceed RS_RCMD;

  state RS_TEMP:

	read_sensor (RS_TEMP, THERMOCOUPLE, &VV);
	TT = temp (VV);

  state RS_TEMP_REPORT:

	ser_outf (RS_TEMP_REPORT, "T: %u.%u, %u <%x>\r\n",
		TT / 10, TT % 10, VV, VV);
	proceed RS_RCMD;
}
