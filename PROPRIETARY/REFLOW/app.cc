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

	word Second, Temperature;

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

	for (i = 0; i < NProfEntries; i++)
		if (Prof [i].Second >= sec)
			break;

	if (i == NProfEntries || Prof [i].Second > sec) {
		// Must grow the array
		if (NProfEntries == MAXPROFENTRIES)
			return 1;
		// Grow the array
		for (j = NProfEntries; j > i; j--)
			Prof [j] = Prof [j - 1];
		NProfEntries++;
	}

	Prof [i].Second = sec;
	Prof [i].Temperature = tmp;
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

// The absolute value of error will be truncated to this much (in deg C * 10),
// i.e., 100 deg, to give us some value to normalize things to. Generally, it
// makes sense to assume that the heater should be set to max if the error is
// larger than something reasonably large
#define	ERROR_BOUND	1000

// Max setting of PW (pulse witdth)
#define	MAXPWI		1024

// Gain multiplier: after multiplication, the gain is divided by ERROR_BOUND.
// For GAINMULT == 100 and, say, GainP == 1, one degree error (10) translates
// into one unit of PW.
#define	GAINMULT	100

// Max gain (for sanity). We set it to an absurdly high value where 1 deg error
// translates into full PW. Decent values should be reasonably small.
#define	MAXGAIN		MAXPWI

// Max integrator bound
#define	MAXIBOUND	10

// Gain multiplied by MAXPWI for normalization
static	lint	GainP = DEFAULT_GAIN_P * GAINMULT,
		GainI = DEFAULT_GAIN_I * GAINMULT,
		GainD = DEFAULT_GAIN_D * GAINMULT;

static	sint	Integrator,
		Differentiator;

static	sint 	MinIntegrator = -DEFAULT_INTG_B * ERROR_BOUND,
		MaxIntegrator = +DEFAULT_INTG_B * ERROR_BOUND;

static	sint	ER;		// Error
static	word	CV,		// Control value, i.e., PW from 0 to MAXPWI
		TA,		// Target temperature
		CU,		// Current temperature
		TM,		// Elapsed time in seconds
		T0,		// Initial temperature
		PX;		// Profile index

static lword	LS;		// Last second

static	lint	TG, PP, II, DD;	// We expose them all for the show

static void controller () {
//
	ER = (sint) TA - (sint) CU;

	// the error is in deg C * 10, but truncated at some max

	if (ER > ERROR_BOUND)
		ER = ERROR_BOUND;
	else if (ER < -ERROR_BOUND)
		ER = -ERROR_BOUND;

	if ((Integrator += ER) > MaxIntegrator)
		Integrator = MaxIntegrator;
	else if (Integrator < MinIntegrator)
		Integrator = MinIntegrator;

	PP = (GainP * ER) / ERROR_BOUND;
	II = (GainI * Integrator) / ERROR_BOUND;
	DD = (GainD * (Differentiator - CU)) / ERROR_BOUND;

	Differentiator = CU;

	TG = PP + II + DD;

	if (TG < 0)
		CV = 0;
	else if (TG > MAXPWI)
		CV = MAXPWI;
	else
		CV = (word) TG;
}

void target () {
//
// Target temperature at time TM
//
	sint t0, t1;
	word s0, s1;
	lint tmp;

	while (1) {
		// Locate the right end of the interval
		if (PX >= NProfEntries) {
			TA = 0;
			return;
		}
		if (TM <= Prof [PX].Second)
			break;
		PX++;
	}

	// The left end
	if (PX == 0) {
		// We are at the left end
		t0 = (sint)T0;
		s0 = 0;
	} else {
		t0 = (sint)(Prof [PX-1].Temperature);
		s0 = Prof [PX-1].Second;
	}

	t1 = (sint)(Prof [PX].Temperature);
	s1 = Prof [PX].Second;

	// Interpolate
	if (s1 == s0) {
		TA = (word) t1;
		return;
	}

	tmp = (t1 - t0);
	tmp = (tmp * (TM - s0))/(s1 - s0);
	TA = (word)(t0 + (sint)tmp);
}

// ============================================================================

sint Notifier;

fsm notifier {

	state LOOP:

		ser_outf (LOOP, "!%u %u->%u %u (%ld, %ld, %ld, %ld)\r\n",
			TM, CU, TA, CV, TG, PP, II, DD);

	initial state WAIT_FOR_EVENT:

		when (&Notifier, LOOP);
}

fsm reflow {

	state INIT:

		word t;

		// Initial temperature
		read_sensor (WNONE, THERMOCOUPLE, &t);
		Differentiator = CU = T0 = temp (t);
		Integrator = 0;
		LS = seconds ();
		TM = (word)(-1);
		Notifier = runfsm notifier;
		PX = 0;

	state LOOP:

		lword cs;
		word t;

		// Adjust to the next second boundary
		if ((cs = seconds ()) == LS) {
			delay (1, LOOP);
			release;
		}

		TM += (word)(cs - LS);
		LS = cs;

		read_sensor (WNONE, THERMOCOUPLE, &t);
		CU = temp (t);
		target ();

		if (TA == 0)
			// This means we are done
			sameas FINISH;

		controller ();
		setOven (CV);

		ptrigger (Notifier, &Notifier);

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
		" g p i d    -> set gain\r\n"
		" i b        -> set integrator bound\r\n"
		" e          -> erase profile\r\n"
		" p s t ...  -> add profile entry\r\n"
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
		case 'i' : proceed RS_SETINTEGRATOR;
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
		g = pnum ();
		if (EF || g < 0 || g > MAXGAIN)
			proceed RS_ERR;
		r [i] = (lint) GAINMULT * g;
	}

	GainP = r [0];
	GainI = r [1];
	GainD = r [2];
	proceed RS_RCMD;

  state RS_SETINTEGRATOR:

	sint g;

	g = pnum ();
	if (EF || g < 0 || g > MAXIBOUND)
		proceed RS_ERR;

	MinIntegrator = -g;
	MaxIntegrator =  g;
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

	ser_outf (RS_SHOW, "G: %u %u %u, I: %u, P: %u\r\n",
		(word)(GainP / GAINMULT),
		(word)(GainI / GAINMULT),
		(word)(GainD / GAINMULT),
		(word)(MaxIntegrator / ERROR_BOUND),
		NProfEntries);

	VV = 0;

  state RS_SHOW_PROF:

	if (VV >= NProfEntries)
		proceed RS_RCMD;

	ser_outf (RS_SHOW_PROF, " P%u: %u %u.%u\r\n", VV,
		Prof [VV] . Second,
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
