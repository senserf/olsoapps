/* ==================================================================== */
/* Copyright (C) Olsonet Communications, 2012                           */
/* All rights reserved.                                                 */
/* ==================================================================== */

#include "sysio.h"
#include "sensors.h"
#include "oss.h"

#ifndef	__SMURPH__
#include "board_pins.h"
#else
#define	max6675_on() CNOP
#define	max6675_off() CNOP
#endif

// ============================================================================

#define	IBUFLEN		82
#define	THERMOCOUPLE	0	// Temperature sensor
#define	OVEN		0	// Oven PWM actuator

#define	MAXTEMP		3000	// 10 * degrees
#define	MAXSEGDUR	1000	// seconds

// ============================================================================

#define	LED_RED		0
#define	LED_GREEN	1
#define	LED_YELLOW	2

static byte led_stat [3];

static void led_toggle (word d) {

	led_stat [d] = 1 - led_stat [d];
	leds (d, led_stat [d]);
}

static void leds_clear () {

	fastblink (0);
	led_stat [0] = led_stat [1] = led_stat [2] = 0;
	leds (0, 0);
	leds (1, 0);
	leds (2, 0);
}

// ============================================================================

static word unpack2 (byte *b) {

	return (word)(b[0]) | ((word)(b[1]) << 8);
}

static void pack2 (byte *b, word val) {

	b [0] = (byte) val;
	b [1] = (byte) (val >> 8);
}

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
// The controllable span of error in degrees * 10. Any error larger than that
// in the down direction results in full throttle. The control value is scaled
// to the Span.
// ============================================================================
static	word	Span  = DEFAULT_SPAN,
		GainI = DEFAULT_INTEGRATOR_GAIN,
		GainD = DEFAULT_DIFFERENTIATOR_GAIN;

// Max setting of PW (pulse witdth)
#define	MAXPWI		1024

static	word	TA,	// Target temperature for this second
		CU,	// Current temperature at this second
		CV = 0;	// Control value

static	sint	ER;
		
static	lword	Integrator,
		Differentiator;

static 	Boolean	Reflow, Emulation;

// ============================================================================
// Here is a crude and naive oven model for tests =============================
// ============================================================================

word ModelTemp = MIN_OVEN_TEMP;

static void tupd () {
//
// Run every second
//
	word upd;

	if (CV) {
		// Heating
		upd = (word)(((lint)CV * HEATING_FACTOR) / ModelTemp);
		if (upd > MAX_HEATING)
			upd = MAX_HEATING;
		else if (upd < MIN_HEATING)
			upd = MIN_HEATING;
		if ((upd += ModelTemp) > MAX_OVEN_TEMP)
			ModelTemp = MAX_OVEN_TEMP;
		else
			ModelTemp = upd;
	} else {
		// Cooling
		upd = (word)(((lint)ModelTemp * COOLING_FACTOR) /
			MIN_OVEN_TEMP);
		if (upd > MAX_COOLING)
			upd = MAX_COOLING;
		else if (upd < MIN_COOLING)
			upd = MIN_COOLING;
		if ((upd = ModelTemp - upd) < MIN_OVEN_TEMP)
			ModelTemp = MIN_OVEN_TEMP;
		else
			ModelTemp = upd;
	}

	// Random fluctuations
	upd = rnd ();
	if (upd & 0x10)
		ModelTemp += (upd & 3);
	else
		ModelTemp -= (upd & 3);
}

fsm oven_model {

	state OM_INIT:

		ModelTemp = MIN_OVEN_TEMP;
		CV = 0;

	state OM_LOOP:

		if (!Emulation)
			finish;
		tupd ();
		delay (1020 + (rnd () & 3), OM_LOOP);
}

// ============================================================================
// ============================================================================

typedef	struct {

	word Value, Reading;

} calentry_t;

static const calentry_t Cal [] = THERMOCOUPLE_CALIBRATION;

#define	CalLength	(sizeof (Cal) / sizeof (calentry_t))

static void setOven () {

	if (!Emulation)
		write_actuator (WNONE, OVEN, &CV);
}

static word temp () {
//
// Reads the temp sensor and converts the reading to temperature in deg C * 10
//
	word a, b, c, v, v0, v1, r, r0, r1;

	if (Emulation)
		return ModelTemp;

	read_sensor (WNONE, THERMOCOUPLE, &r);

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

// ============================================================================
// ============================================================================

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
		if (ER) {
			// If ER == 0 (implying that Span is zero), retain
			// previous setting
			Integrator = 0;
			Differentiator = CU;
			CV = MAXPWI;
		}
		return;
	}

	// The first component: proportional to error, normalized to Span
	if (ER == 0)
		t = (lint)CV;
	else
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

static void init_target () {

	CS = FS = 0;
	CP = -1;
	UP = YES;
	FT = MIN_OVEN_TEMP;
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

static void out_reflow_status (word st) {

	byte *buf;

	if ((buf = oss_outu (st, 12)) == NULL)
		return;

	buf [0] = OSS_CMD_U_REP;
	buf [1] = Emulation;

	pack2 (buf +  2, CS);
	pack2 (buf +  4, CU);
	pack2 (buf +  6, CV);
	pack2 (buf +  8, CP);
	pack2 (buf + 10, TA);

	oss_send (buf);
}

static void out_heartbeat (word st, word t) {

	byte *buf;

	if ((buf = oss_outu (st, 8)) == NULL)
		return;

	buf [0] = OSS_CMD_U_HBE;
	buf [1] = Emulation;
	buf [2] = 0x31;
	buf [3] = 0x7A;

	pack2 (buf + 4, t);
	pack2 (buf + 6, CV);

	oss_send (buf);
}

fsm reflow {

	word t;

	state IDLE:

		// Heartbeat report
		t = temp ();
		led_toggle (LED_GREEN);

	state HEARTBEAT:

		out_heartbeat (HEARTBEAT, t);

		if (!Reflow) {
			delay (1024, IDLE);
			release;
		}

		// Initialize for reflow
		LS = seconds ();

	state START:

		lword cs;

		if ((cs = seconds ()) == LS) {
			// Wait for the nearest second boundary
			delay (1, START);
			release;
		}

		LS = cs;

		// Initial temperature
		CU = temp ();
		leds_clear ();
		init_target ();

	state LOOP:

		target ();

		if (TA == 0 || !Reflow)
			sameas FINISH;
		controller ();
		setOven ();

	state NOTIFY:

		out_reflow_status (NOTIFY);
		led_toggle (LED_YELLOW);

	state NEXT:

		lword cs;

		// Adjust to the next second boundary
		if ((cs = seconds ()) == LS) {
			delay (1, NEXT);
			release;
		}

		LS = cs;
		CS++;

		CU = temp ();
		sameas LOOP;

	state FINISH:

		Reflow = NO;
		CV = 0;
		setOven ();
		delay (1024, DONE);
		release;

	state DONE:

		leds_clear ();
		sameas IDLE;
}

// ============================================================================

static void rq_handler (word st, byte *cmd, word cmdlen) {


	switch (*cmd) {

		case OSS_CMD_RSC:

			// Reconnection
			Reflow = NO;
			killall (oven_model);
			Emulation = NO;
			oss_ack (st, OSS_CMD_ACK);
			return;

		case OSS_CMD_D_RUN: {

			word ns, m, i, du, tm;

			// Start reflow
			if (cmdlen < 16) {
SErr:
				oss_ack (st, OSS_CMD_NAK);
				return;
			}

			ns = cmd [1];

			if (ns > 24 || ns < 2 || (ns * 4) > (cmdlen - 8))
				goto SErr;

			Span  = unpack2 (cmd + 2);
			GainI = unpack2 (cmd + 4);
			GainD = unpack2 (cmd + 6);

			if (Span < MINIMUM_SPAN || Span > 1024)
				goto SErr;

			if (GainI > 1024 || GainD > 1024)
				goto SErr;

			NProfEntries = 0;
			m = 8;
			for (i = 0; i < ns; i++) {
				du = unpack2 (cmd + m);
				m += 2;
				tm = unpack2 (cmd + m);
				m += 2;
				if (du > MAXSEGDUR || tm > MAXTEMP)
					goto SErr;
				addProfEntry (du, tm);
			}

			oss_ack (st, OSS_CMD_ACK);

			Reflow = YES;
			return;

		}

		case OSS_CMD_D_ABT:

			Reflow = NO;
			oss_ack (st, OSS_CMD_ACK);
			return;

		case OSS_CMD_D_SET: {

			word ov;

			if (Reflow) {
				oss_ack (st, OSS_CMD_NAK);
				return;
			}

			if (cmdlen < 4)
				goto SErr;

			ov = unpack2 (cmd + 2);
			if (ov > 1024)
				goto SErr;

			oss_ack (st, OSS_CMD_ACK);

			CV = ov;
			setOven ();
			return;
		}

		case OSS_CMD_D_EMU:

			if (cmd [1]) {
				// Emulation on
				Emulation = YES;
				if (!running (oven_model))
					runfsm oven_model;
			} else {
				killall (oven_model);
				Emulation = NO;
			}

		default:

			goto SErr;

	}
}

// ============================================================================

fsm root {

	state INIT:

		leds_clear ();
		setOven ();
		// The thermocouple is constantly on
		max6675_on ();
		oss_init (rq_handler);

		leds_clear ();
		runfsm reflow;

		
		finish;
}
